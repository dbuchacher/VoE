#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>
#include "lattice.h"

/*
 * model.h — lattice-native sequence model
 *
 * Architecture:
 *   Input:  context window of N wave bytes, each decoded to 4D coords
 *   Layer 1: project 4D × N → hidden_dim  (flatten context + linear)
 *   Layer 2: hidden_dim → hidden_dim       (linear + ReLU)
 *   Layer 3: hidden_dim → 81              (output logits, one per shell-1 wave byte)
 *
 * No embedding layer — coordinates ARE the 4-dimensional embedding.
 * No BPE tokenizer — wave bytes ARE the tokens.
 *
 * This is the smallest thing that can learn. We grow from here.
 */

#define CTX_LEN    128     /* context window: 128 wave bytes */
#define INPUT_DIM  (CTX_LEN * 4)  /* 128 × 4D = 512 floats */
#define HIDDEN_DIM 1024    /* internal width — use the hardware */
#define OUTPUT_DIM 81      /* 3^4 shell-1 wave bytes */

/* ── Weights ─────────────────────────────────────────────────── */

typedef struct {
    /* layer 1: input → hidden */
    float w1[HIDDEN_DIM][INPUT_DIM];    /* weights */
    float b1[HIDDEN_DIM];               /* bias */

    /* layer 2: hidden → hidden */
    float w2[HIDDEN_DIM][HIDDEN_DIM];
    float b2[HIDDEN_DIM];

    /* layer 3: hidden → output */
    float w3[OUTPUT_DIM][HIDDEN_DIM];
    float b3[OUTPUT_DIM];
} Weights;

/* ── Activations (forward pass state) ────────────────────────── */

typedef struct {
    float input[INPUT_DIM];         /* flattened 4D context */
    float h1[HIDDEN_DIM];           /* after layer 1 + ReLU */
    float h1_pre[HIDDEN_DIM];       /* before ReLU (for backward) */
    float h2[HIDDEN_DIM];           /* after layer 2 + ReLU */
    float h2_pre[HIDDEN_DIM];       /* before ReLU */
    float logits[OUTPUT_DIM];       /* raw output scores */
    float probs[OUTPUT_DIM];        /* softmax probabilities */
} Activations;

/* ── Gradients (same shape as weights) ───────────────────────── */

typedef Weights Gradients;

/* ── Wave byte ↔ output index mapping ────────────────────────── */

/*
 * Shell-1 wave bytes: 3 values per dim (-1,0,+1) = 81 combinations.
 * Map to indices 0-80 for the output layer.
 *
 * Index = (t+1)*27 + (d+1)*9 + (m+1)*3 + (q+1)
 * where t,d,m,q ∈ {-1, 0, +1}
 */
static inline int coord_to_index(coord4 c) {
    return (c.t + 1) * 27 + (c.d + 1) * 9 + (c.m + 1) * 3 + (c.q + 1);
}

static inline coord4 index_to_coord(int idx) {
    coord4 c;
    c.q = (idx % 3) - 1; idx /= 3;
    c.m = (idx % 3) - 1; idx /= 3;
    c.d = (idx % 3) - 1; idx /= 3;
    c.t = (idx % 3) - 1;
    return c;
}

/* ── Init ────────────────────────────────────────────────────── */

static inline float randf(void) {
    return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
}

static void model_init(Weights *w) {
    /*
     * Xavier-ish init scaled for our dimensions.
     * Layer 1: fan_in = INPUT_DIM (256), fan_out = HIDDEN_DIM (128)
     * Scale = sqrt(2 / (fan_in + fan_out))
     */
    float s1 = sqrtf(2.0f / (INPUT_DIM + HIDDEN_DIM));
    float s2 = sqrtf(2.0f / (HIDDEN_DIM + HIDDEN_DIM));
    float s3 = sqrtf(2.0f / (HIDDEN_DIM + OUTPUT_DIM));

    for (int i = 0; i < HIDDEN_DIM; i++) {
        for (int j = 0; j < INPUT_DIM; j++)
            w->w1[i][j] = randf() * s1;
        w->b1[i] = 0.0f;
    }
    for (int i = 0; i < HIDDEN_DIM; i++) {
        for (int j = 0; j < HIDDEN_DIM; j++)
            w->w2[i][j] = randf() * s2;
        w->b2[i] = 0.0f;
    }
    for (int i = 0; i < OUTPUT_DIM; i++) {
        for (int j = 0; j < HIDDEN_DIM; j++)
            w->w3[i][j] = randf() * s3;
        w->b3[i] = 0.0f;
    }
}

/* ── Forward pass ────────────────────────────────────────────── */

static void model_forward(const Weights *w, Activations *a,
                           const uint8_t *context /* CTX_LEN wave bytes */) {
    /* encode context: each wave byte → 4 floats */
    for (int i = 0; i < CTX_LEN; i++) {
        coord4 c = wave_decode(context[i]);
        a->input[i * 4 + 0] = (float)c.t;
        a->input[i * 4 + 1] = (float)c.d;
        a->input[i * 4 + 2] = (float)c.m;
        a->input[i * 4 + 3] = (float)c.q;
    }

    /* layer 1: linear + ReLU */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < HIDDEN_DIM; i++) {
        float sum = w->b1[i];
        for (int j = 0; j < INPUT_DIM; j++)
            sum += w->w1[i][j] * a->input[j];
        a->h1_pre[i] = sum;
        a->h1[i] = sum > 0.0f ? sum : 0.0f;  /* ReLU */
    }

    /* layer 2: linear + ReLU */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < HIDDEN_DIM; i++) {
        float sum = w->b2[i];
        for (int j = 0; j < HIDDEN_DIM; j++)
            sum += w->w2[i][j] * a->h1[j];
        a->h2_pre[i] = sum;
        a->h2[i] = sum > 0.0f ? sum : 0.0f;
    }

    /* layer 3: linear (logits) */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < OUTPUT_DIM; i++) {
        float sum = w->b3[i];
        for (int j = 0; j < HIDDEN_DIM; j++)
            sum += w->w3[i][j] * a->h2[j];
        a->logits[i] = sum;
    }

    /* softmax */
    float max_logit = a->logits[0];
    for (int i = 1; i < OUTPUT_DIM; i++)
        if (a->logits[i] > max_logit) max_logit = a->logits[i];

    float sum_exp = 0.0f;
    for (int i = 0; i < OUTPUT_DIM; i++) {
        a->probs[i] = expf(a->logits[i] - max_logit);
        sum_exp += a->probs[i];
    }
    for (int i = 0; i < OUTPUT_DIM; i++)
        a->probs[i] /= sum_exp;
}

/* ── Loss: cross-entropy ─────────────────────────────────────── */

static float model_loss(const Activations *a, int target_idx) {
    float p = a->probs[target_idx];
    if (p < 1e-10f) p = 1e-10f;
    return -logf(p);
}

/* ── Backward pass ───────────────────────────────────────────── */

static void model_backward(const Weights *w, const Activations *a,
                            Gradients *g, int target_idx) {
    memset(g, 0, sizeof(Gradients));

    /* output gradient: softmax + cross-entropy = probs - one_hot */
    float dlogits[OUTPUT_DIM];
    for (int i = 0; i < OUTPUT_DIM; i++)
        dlogits[i] = a->probs[i];
    dlogits[target_idx] -= 1.0f;

    /* layer 3 gradients */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < OUTPUT_DIM; i++) {
        for (int j = 0; j < HIDDEN_DIM; j++)
            g->w3[i][j] = dlogits[i] * a->h2[j];
        g->b3[i] = dlogits[i];
    }

    /* backprop through layer 3 to get dh2 */
    float dh2[HIDDEN_DIM];
    memset(dh2, 0, sizeof(dh2));
    #pragma omp parallel for schedule(static) reduction(+:dh2[:HIDDEN_DIM])
    for (int j = 0; j < HIDDEN_DIM; j++)
        for (int i = 0; i < OUTPUT_DIM; i++)
            dh2[j] += w->w3[i][j] * dlogits[i];

    /* ReLU backward */
    for (int i = 0; i < HIDDEN_DIM; i++)
        if (a->h2_pre[i] <= 0.0f) dh2[i] = 0.0f;

    /* layer 2 gradients */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < HIDDEN_DIM; i++) {
        for (int j = 0; j < HIDDEN_DIM; j++)
            g->w2[i][j] = dh2[i] * a->h1[j];
        g->b2[i] = dh2[i];
    }

    /* backprop through layer 2 to get dh1 */
    float dh1[HIDDEN_DIM];
    memset(dh1, 0, sizeof(dh1));
    #pragma omp parallel for schedule(static) reduction(+:dh1[:HIDDEN_DIM])
    for (int j = 0; j < HIDDEN_DIM; j++)
        for (int i = 0; i < HIDDEN_DIM; i++)
            dh1[j] += w->w2[i][j] * dh2[i];

    /* ReLU backward */
    for (int i = 0; i < HIDDEN_DIM; i++)
        if (a->h1_pre[i] <= 0.0f) dh1[i] = 0.0f;

    /* layer 1 gradients */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < HIDDEN_DIM; i++) {
        for (int j = 0; j < INPUT_DIM; j++)
            g->w1[i][j] = dh1[i] * a->input[j];
        g->b1[i] = dh1[i];
    }
}

/* ── SGD update ──────────────────────────────────────────────── */

static void model_update(Weights *w, const Gradients *g, float lr) {
    float *wp = (float *)w;
    const float *gp = (const float *)g;
    int n = sizeof(Weights) / sizeof(float);
    for (int i = 0; i < n; i++)
        wp[i] -= lr * gp[i];
}

/* ── Save / Load ─────────────────────────────────────────────── */

static int model_save(const Weights *w, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(w, sizeof(Weights), 1, f);
    fclose(f);
    return 0;
}

static int model_load(Weights *w, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    if (fread(w, sizeof(Weights), 1, f) != 1) { fclose(f); return -1; }
    fclose(f);
    return 0;
}

/* ── Sample from output distribution ─────────────────────────── */

static int model_sample(const Activations *a, float temperature) {
    float probs[OUTPUT_DIM];

    if (temperature < 0.01f) {
        /* greedy: pick argmax */
        int best = 0;
        for (int i = 1; i < OUTPUT_DIM; i++)
            if (a->probs[i] > a->probs[best]) best = i;
        return best;
    }

    /* temperature-scaled sampling */
    float max_l = a->logits[0];
    for (int i = 1; i < OUTPUT_DIM; i++)
        if (a->logits[i] > max_l) max_l = a->logits[i];

    float sum = 0.0f;
    for (int i = 0; i < OUTPUT_DIM; i++) {
        probs[i] = expf((a->logits[i] - max_l) / temperature);
        sum += probs[i];
    }
    for (int i = 0; i < OUTPUT_DIM; i++)
        probs[i] /= sum;

    /* sample */
    float r = (float)rand() / (float)RAND_MAX;
    float cumsum = 0.0f;
    for (int i = 0; i < OUTPUT_DIM; i++) {
        cumsum += probs[i];
        if (r <= cumsum) return i;
    }
    return OUTPUT_DIM - 1;
}

#endif /* MODEL_H */
