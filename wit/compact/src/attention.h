#ifndef ATTENTION_H
#define ATTENTION_H

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "wave.h"

/*
 * attention.h — 4-head dimensional attention, component 2.2
 *
 * WHY THIS IS NOT TRANSFORMER ATTENTION
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Transformers compute attention as:
 *   Attention(Q, K, V) = softmax(Q K^T / sqrt(d_k)) V
 * where Q, K, V are LEARNED linear projections of the input.
 * The model learns WHAT to attend to during training.
 *
 * Here, attention is COMPUTED from the coordinate system:
 *   weight[i] = 1/(1 + |dist along dim|) * rc_decay(positional_dist)
 *
 * No Q/K/V weight matrices.  No learned projections.  No backpropagation.
 * The coordinate distance IS the attention score.  Phonetically similar
 * positions attend to each other because they ARE geometrically close.
 *
 * This works because the coordinate space has structure by construction
 * (wave.h).  Transformer attention must LEARN the structure from data.
 * We get it for free from phonetics.
 *
 * WHY EXACTLY 4 HEADS
 * ~~~~~~~~~~~~~~~~~~~
 * Transformers use 8, 16, 32, or 96 heads — arbitrary.  Some heads
 * empirically specialize (positional, syntactic), confirming that
 * dimensional structure EXISTS.  But it's discovered during training.
 *
 * The lattice has 4 orthogonal dimensions.  4 heads, one per dimension:
 *   T-head: temporal attention (when → when, verb tense agreement)
 *   D-head: spatial attention (where → where, preposition→landmark)
 *   M-head: substantive attention (what → what, noun→noun agreement)
 *   Q-head: qualitative attention (how → how, adjective→noun)
 *
 * Each head attends ONLY along its dimension.  The T-head ignores
 * D/M/Q differences entirely.  This is dimensional decomposition:
 * a 4D attention problem becomes four independent 1D problems.
 *
 * (Prediction 1 from primes.md: "4 dimensional heads should outperform
 * 4 arbitrary heads at equal parameter count.")
 *
 * WHY RC DECAY INSTEAD OF 1/sqrt(d_k)
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Transformers scale attention by 1/sqrt(d_k) — a CONSTANT.
 * Every token at every position gets the same scaling.
 *
 * RC decay is PER-TOKEN: relevance = e^(-t/RC) where:
 *   t = positional distance in the walk
 *   R = resistance (connection impedance: base + clause penalty)
 *   C = capacitance (|mu| — substance weight of the token)
 *
 * Content words (nouns, proper names) have high C — they persist long.
 * Function words ("the", "of") have low C — they decay fast.
 * The decay IS the attention mask.  No learned mask, no arbitrary window.
 *
 * (Prediction 13: "RC-decay attention should handle long documents
 * better than fixed windows.")
 *
 * WHY 25% SPARSITY
 * ~~~~~~~~~~~~~~~~
 * Most attention weights are near-zero.  Only ~25% of positions are
 * relevantly connected.  Computing the other 75% is wasted work.
 *
 * The lattice predicts 25% fill convergence from Michaelis-Menten +
 * Nyquist + Shannon.  Sparse attention with coordinate-distance
 * selection should match full attention at 4x less compute.
 *
 * (Prediction 7: "OPTIMAL sparsity is 25%, and the pattern should
 * follow coordinate distance.")
 *
 * THE INPUT IS A WALK
 * ~~~~~~~~~~~~~~~~~~~
 * A walk is a sequence of word coordinates — a sentence mapped to
 * coord4 through the prime decomposition (primes_lookup.h).
 *
 * Each position in the walk needs a context vector: "given the
 * surrounding coordinates, what does this position predict about
 * what comes next?"  The 4 heads compute this by dimensional
 * decomposition.
 */


/* ================================================================
 * Constants
 * ================================================================ */

/*
 * Default RC time constant.  Controls how fast attention decays
 * with positional distance.
 *
 * RC = 8 means relevance halves roughly every 5-6 positions
 * (e^(-5.5/8) ≈ 0.50).  For a walk of 32 words, positions more
 * than ~16 steps away contribute < 14% weight.
 *
 * This replaces the fixed context window.  There is no hard cutoff.
 * Distant positions still contribute, just exponentially less.
 */
#define ATTN_RC_DEFAULT   8.0f

/*
 * Default sparsity: attend to nearest 25%.
 * From the fill convergence (Prediction 7).
 */
#define ATTN_SPARSITY_DEFAULT  0.25f


/* ================================================================
 * AttentionResult — output of the attention mechanism
 * ================================================================
 *
 * context: the weighted average coordinate across the walk.
 *          Each dimension comes from its own head.
 *
 * confidence: how peaked the attention weights are.
 *             1.0 = all weight on one position (perfect focus).
 *             1/N = uniform (no information, attention is confused).
 *             Measured as average concentration across the 4 heads.
 */

typedef struct {
    coord4 context;       /* integer-rounded for bin lookup */
    float  context_f[4];  /* float-precision: [τ, χ, μ, φ] for equation eval */
    float  confidence;
} AttentionResult;


/* ================================================================
 * Token capacitance — how much information a coordinate carries
 * ================================================================
 *
 * This replaces the constant 1/sqrt(d_k) scaling in transformers.
 * Each token gets its own decay rate based on its content.
 *
 * Capacitance = |mu| (absolute value of the mass/substance dimension).
 * The lattice says C = 2mc^2/h, proportional to MASS.
 *
 *   High |mu| = content words (nouns, concrete things) = high C = persist
 *   Low  |mu| = function words (articles, prepositions) = low C = decay fast
 *
 * No lookup table.  The coordinate IS the capacitance.
 */

/* token_capacitance and rc_relevance: canonical source is distance.h */
#include "distance.h"

/* Adapter: attention.h internally uses (pos_delta, token, rc_base) signature.
 * distance.h uses (word, walk_distance, clause_depth) signature.
 * Bridge with a thin wrapper. */
static inline float attn_rc_relevance(int pos_delta, coord4 context_token,
                                      float rc_base) {
    (void)rc_base; /* use distance.h's default model */
    int d = pos_delta;
    if (d < 0) d = -d;
    return rc_relevance(context_token, d, 0);
}


/* ================================================================
 * dim_attend — attend along a single dimension
 * ================================================================
 *
 * This is the core primitive.  One attention head for one dimension.
 *
 * For each context position, the weight is:
 *   w[i] = (1 / (1 + |context[i].dim - query.dim|)) * rc_decay(i)
 *
 * The first factor is COORDINATE DISTANCE along this dimension only.
 * The T-head only cares about T-distance.  Two words at T=+1 attend
 * strongly to each other even if they differ wildly in D, M, Q.
 * This is dimensional decomposition — each head is an expert on
 * one axis of meaning.
 *
 * The second factor is RC DECAY: positional distance weighted by
 * the context token's information content.  Distant tokens get
 * less weight, but high-information tokens resist the decay.
 *
 * The output is the weighted average coordinate value along this
 * dimension.  The T-head produces a single float: the T-value
 * that best represents the temporal context.
 *
 * In a transformer, this would require:
 *   Q = input @ W_Q    (learned projection)
 *   K = input @ W_K    (learned projection)
 *   V = input @ W_V    (learned projection)
 *   scores = Q @ K^T / sqrt(d_k)
 *   weights = softmax(scores)
 *   output = weights @ V
 *
 * Here it's:
 *   scores[i] = coord_distance(dim) * rc_decay(pos)
 *   weights = normalize(scores)
 *   output = weights @ coord_values(dim)
 *
 * No W_Q, W_K, W_V.  No learned parameters.  The coordinate
 * distance IS the Q-K similarity.  The coordinate value IS the V.
 *
 * Parameters:
 *   walk:        array of coord4 (the sentence/walk)
 *   walk_len:    number of positions
 *   query_pos:   which position we're computing context for
 *   dim:         which dimension (0=T, 1=D, 2=M, 3=Q)
 *   weights_out: optional output buffer for attention weights (can be NULL)
 *   start_pos:   first position to attend to (0 for full, query_pos-limited for causal)
 *   end_pos:     one past last position to attend to
 *   rc:          RC time constant
 *
 * Returns: weighted average value along this dimension.
 */

static inline int8_t coord4_dim(coord4 c, int dim) {
    switch (dim) {
    case 0: return c.t;
    case 1: return c.x;
    case 2: return c.m;
    case 3: return c.q;
    default: return 0;
    }
}

static inline float dim_attend_range(const coord4 *walk, int walk_len,
                                     int query_pos, int dim,
                                     float *weights_out,
                                     int start_pos, int end_pos,
                                     float rc) {
    /*
     * The query value: what we're comparing everything against.
     * In transformer terms, this is Q — but it's just the coordinate.
     */
    int8_t qval = coord4_dim(walk[query_pos], dim);

    float total_weight = 0.0f;
    float weighted_sum = 0.0f;

    for (int i = start_pos; i < end_pos; i++) {
        if (i == query_pos) {
            /* Don't attend to yourself.  Self-attention in transformers
             * includes self, but that's because Q/K projections transform
             * the representation.  Here the coordinate IS the value —
             * attending to yourself just returns yourself.  No information
             * gained.  Skip it. */
            if (weights_out) weights_out[i] = 0.0f;
            continue;
        }

        /*
         * Coordinate distance along this dimension.
         * This IS the Q-K similarity score, without any learned projection.
         *
         * 1/(1 + |delta|) maps:
         *   delta=0  → weight 1.0  (same value = maximum relevance)
         *   delta=1  → weight 0.5  (one step away)
         *   delta=2  → weight 0.33 (two steps)
         *   ...
         * Hyperbolic decay, not Gaussian.  Matches the coordinate
         * geometry: discrete lattice points, not continuous space.
         *
         * Note: this per-dimension 1/(1+|delta|) IS L-infinity attention
         * decomposed along each axis.  Each head independently applies
         * the Chebyshev metric on its own dimension.  The 4-head
         * combination reconstructs the full L-infinity structure.
         */
        int8_t cval = coord4_dim(walk[i], dim);
        int delta = (int)qval - (int)cval;
        if (delta < 0) delta = -delta;
        float coord_weight = 1.0f / (1.0f + (float)delta);

        /*
         * RC decay by positional distance.
         * This IS the attention mask, content-aware.
         */
        float decay = attn_rc_relevance(query_pos - i, walk[i], rc);

        float w = coord_weight * decay;
        if (weights_out) weights_out[i] = w;
        total_weight += w;

        /*
         * Accumulate weighted value.
         * In transformer terms, this is the V contribution.
         * But V is just the coordinate value — no learned projection.
         */
        weighted_sum += w * (float)cval;
    }

    /* Zero out weights outside the range (for weights_out cleanliness) */
    if (weights_out) {
        for (int i = 0; i < start_pos; i++)
            weights_out[i] = 0.0f;
        for (int i = end_pos; i < walk_len; i++)
            weights_out[i] = 0.0f;
    }

    /* Normalize: weights sum to 1 */
    if (total_weight > 1e-10f) {
        if (weights_out) {
            for (int i = start_pos; i < end_pos; i++)
                weights_out[i] /= total_weight;
        }
        return weighted_sum / total_weight;
    }

    /* Fallback: no context available (walk_len <= 1, or degenerate case).
     * Return the query's own value.  This is the identity: if you have
     * no context, the best prediction is yourself. */
    return (float)qval;
}


/*
 * dim_attend — convenience wrapper: full-range attention along one dimension.
 *
 * Attends to ALL positions in the walk (not causal, not sparse).
 * This is the bidirectional variant — reading, not writing.
 * (Prediction 16: reading = boson = bidirectional is fine.)
 */
static inline float dim_attend(const coord4 *walk, int walk_len,
                               int query_pos, int dim,
                               float *weights_out) {
    return dim_attend_range(walk, walk_len, query_pos, dim,
                            weights_out, 0, walk_len, ATTN_RC_DEFAULT);
}


/* ================================================================
 * Attention concentration — how peaked are the weights?
 * ================================================================
 *
 * Measures whether attention is focused or diffuse.
 * Uses the inverse of entropy, normalized to [0, 1].
 *
 * concentration = 1 - H(weights) / log2(N)
 *   - H = 0 (all weight on one token) → concentration = 1.0
 *   - H = log2(N) (uniform) → concentration = 0.0
 *
 * This is the confidence signal.  High concentration means
 * the head found a clear relationship.  Low concentration means
 * the dimension doesn't discriminate well at this position.
 *
 * In transformers, there's no built-in confidence metric.
 * You'd have to compute entropy of softmax output post-hoc.
 * Here it's a natural byproduct.
 */

static inline float attention_concentration(const float *weights,
                                            int start, int end) {
    float entropy = 0.0f;
    int count = 0;

    for (int i = start; i < end; i++) {
        if (weights[i] > 1e-10f) {
            entropy -= weights[i] * log2f(weights[i]);
            count++;
        }
    }

    if (count <= 1) return 1.0f;  /* only one token: perfectly focused */

    float max_entropy = log2f((float)count);
    if (max_entropy < 1e-10f) return 1.0f;

    float conc = 1.0f - entropy / max_entropy;
    if (conc < 0.0f) conc = 0.0f;  /* numerical safety */
    return conc;
}


/* ================================================================
 * lattice_attention — run all 4 heads, combine
 * ================================================================
 *
 * Each head attends along its dimension independently.
 * The combined context is (T-head output, D-head output,
 * M-head output, Q-head output) — one float per dimension,
 * rounded to the nearest integer to produce a coord4.
 *
 * Why rounding?  The coordinate space is discrete.  The lattice
 * has integer coordinates.  The weighted average may produce
 * fractional values (e.g., T-head says 0.7), which we round
 * to the nearest lattice point (T=+1).  This is quantization —
 * the same as wave function collapse.  The continuous attention
 * output collapses to a discrete coordinate.
 *
 * Confidence is the average concentration across all 4 heads.
 * When all heads are focused, the model is confident.
 * When any head is diffuse, overall confidence drops.
 */

static inline AttentionResult lattice_attention_range(
        const coord4 *walk, int walk_len, int query_pos,
        int start_pos, int end_pos, float rc) {

    AttentionResult result;
    float total_conc = 0.0f;

    /*
     * We need a temporary weights buffer for concentration measurement.
     * Stack-allocate for walks up to 1024 positions.  Beyond that,
     * the caller should be using sparse_attention anyway.
     */
    float wbuf[1024];
    float *weights = (walk_len <= 1024) ? wbuf : NULL;
    /* If walk is too long for stack, skip concentration measurement */

    float dim_vals[4];
    for (int dim = 0; dim < 4; dim++) {
        dim_vals[dim] = dim_attend_range(walk, walk_len, query_pos, dim,
                                         weights, start_pos, end_pos, rc);
        if (weights) {
            total_conc += attention_concentration(weights,
                                                  start_pos, end_pos);
        } else {
            total_conc += 0.5f;  /* assume moderate confidence if we can't measure */
        }
    }

    /*
     * Round to nearest lattice point.
     * This is wave function collapse: continuous → discrete.
     * roundf(0.7) = 1.0,  roundf(-0.3) = 0.0,  roundf(1.6) = 2.0
     *
     * For shell-1 inputs (values in {-1, 0, +1}), the weighted average
     * is always in [-1, +1], so rounding stays in shell 1.
     * For word-level coordinates (larger magnitudes), rounding can
     * produce any integer — the context inherits the semantic scale.
     */
    result.context.t = (int8_t)roundf(dim_vals[0]);
    result.context.x = (int8_t)roundf(dim_vals[1]);
    result.context.m = (int8_t)roundf(dim_vals[2]);
    result.context.q = (int8_t)roundf(dim_vals[3]);

    result.confidence = total_conc / 4.0f;

    return result;
}


/*
 * lattice_attention — full bidirectional attention over the whole walk.
 */
static inline AttentionResult lattice_attention(const coord4 *walk,
                                                int walk_len,
                                                int query_pos) {
    return lattice_attention_range(walk, walk_len, query_pos,
                                  0, walk_len, ATTN_RC_DEFAULT);
}


/* ================================================================
 * causal_attention — only attend to past positions
 * ================================================================
 *
 * This is Prediction 16: Chirality = Causal Processing Direction.
 *
 * The quaternion right-hand rule locks a processing chirality:
 * cause → effect, not effect → cause.  In language:
 * subject → verb → object.  In time: past → present → future.
 *
 * For GENERATION (writing = fermion = exclusive = one direction),
 * causal attention is fundamental, not a design choice.  You can
 * only condition on what came before.
 *
 * For UNDERSTANDING (reading = boson = fan-out = bidirectional),
 * use lattice_attention instead.  Reading and writing have
 * different chirality.
 *
 * Implementation: same as lattice_attention, but end_pos = query_pos.
 * Positions AT or AFTER query_pos are masked out.  This is the
 * causal mask — but derived from physics, not convention.
 */

static inline AttentionResult causal_attention(const coord4 *walk,
                                               int walk_len,
                                               int query_pos) {
    if (query_pos <= 0) {
        /* No past context.  Return own coordinate with zero confidence. */
        AttentionResult r;
        r.context = walk[0];
        r.confidence = 0.0f;
        return r;
    }
    return lattice_attention_range(walk, walk_len, query_pos,
                                  0, query_pos, ATTN_RC_DEFAULT);
}


/* ================================================================
 * sparse_attention — attend to nearest 25% by coordinate proximity
 * ================================================================
 *
 * Prediction 7: Sparse Attention at 25%.
 *
 * "Most attention weights are near-zero.  Only ~25% of positions
 * are relevantly connected.  Computing attention over the other
 * 75% is wasted work."
 *
 * The sparsity is NOT random (like BigBird) and NOT windowed
 * (like Longformer).  It's COORDINATE-BASED: attend to the
 * positions whose coordinates are closest to the query, measured
 * by Chebyshev distance (L-infinity) in the full 4D space.
 *
 * Algorithm:
 *   1. Compute Chebyshev distance from query to every position.
 *   2. Select the nearest (sparsity * walk_len) positions.
 *   3. Run dim_attend_range over only those positions.
 *
 * For walk_len = 100 and sparsity = 0.25, we attend to 25 positions.
 * The 75 most distant positions are skipped entirely — not softmax-
 * zeroed, but never computed.  This is 4x less work.
 *
 * Why Chebyshev distance for selection?
 *   - Chebyshev = L-infinity = max per-dimension distance.
 *   - The lattice defines shells by max|exponent| (degree).
 *   - Two points on the same shell have Chebyshev distance 0 from
 *     the shell boundary.  L-infinity IS the shell metric.
 *   - Cheap to compute: four subtractions, four absolute values,
 *     one max.
 *   - On a discrete lattice, Chebyshev distance counts how many
 *     shells apart two points are.  It's the natural distance
 *     metric for the degree-based lattice structure.
 *
 * The selected positions then get full dimensional attention
 * (per-dimension distance + RC decay).  Sparsity handles the
 * WHAT-to-attend-to question; dimensional attention handles
 * HOW-MUCH-to-attend.
 */

static inline AttentionResult sparse_attention(const coord4 *walk,
                                               int walk_len,
                                               int query_pos,
                                               float sparsity) {
    if (sparsity <= 0.0f) sparsity = ATTN_SPARSITY_DEFAULT;
    if (sparsity > 1.0f) sparsity = 1.0f;

    int k = (int)(sparsity * (float)walk_len);
    if (k < 1) k = 1;
    if (k >= walk_len) {
        /* Attending to everything — just use full attention */
        return lattice_attention(walk, walk_len, query_pos);
    }

    /*
     * Step 1: compute Chebyshev distance from query to every position.
     *
     * We use a simple selection approach: for small walks (< 1024),
     * compute all distances, then pick the k smallest.  For the
     * walk sizes this system targets (sentences = 5-50 words,
     * paragraphs = 50-200 words), this is always fast.
     */
    coord4 q = walk[query_pos];

    /* Distance + index pairs for selection */
    int dists[1024];
    int indices[1024];
    int n = (walk_len <= 1024) ? walk_len : 1024;

    for (int i = 0; i < n; i++) {
        /* Chebyshev: max of absolute per-dimension differences */
        int dt = (int)q.t - (int)walk[i].t; if (dt < 0) dt = -dt;
        int dx = (int)q.x - (int)walk[i].x; if (dx < 0) dx = -dx;
        int dm = (int)q.m - (int)walk[i].m; if (dm < 0) dm = -dm;
        int dq = (int)q.q - (int)walk[i].q; if (dq < 0) dq = -dq;
        int max = dt;
        if (dx > max) max = dx;
        if (dm > max) max = dm;
        if (dq > max) max = dq;
        dists[i] = max;
        indices[i] = i;
    }

    /*
     * Step 2: partial selection — find the k nearest positions.
     *
     * Simple selection sort for the top k.  For k=25 and n=100,
     * this is 25*100 = 2500 comparisons.  A proper quickselect
     * would be O(n), but the constant factor makes it slower for
     * n < ~500.  Keep it simple.
     */
    for (int picked = 0; picked < k; picked++) {
        int best = picked;
        for (int j = picked + 1; j < n; j++) {
            if (dists[j] < dists[best])
                best = j;
        }
        /* Swap into position */
        if (best != picked) {
            int tmp_d = dists[picked];
            dists[picked] = dists[best];
            dists[best] = tmp_d;

            int tmp_i = indices[picked];
            indices[picked] = indices[best];
            indices[best] = tmp_i;
        }
    }

    /*
     * Step 3: build a sparse walk from the selected positions and
     * run full dimensional attention over it.
     *
     * We create a compact sub-walk of k positions, remembering the
     * original positional distances (for RC decay) by storing the
     * index mapping.
     *
     * But dim_attend_range works on contiguous arrays with positional
     * indexing.  Instead, we'll compute attention manually over the
     * sparse set.  This is the sparse kernel.
     */
    AttentionResult result;
    float total_conc = 0.0f;
    float dim_vals[4];

    for (int dim = 0; dim < 4; dim++) {
        int8_t qval = coord4_dim(q, dim);
        float total_weight = 0.0f;
        float weighted_sum = 0.0f;
        float sparse_weights[1024];
        int weight_count = 0;

        for (int p = 0; p < k; p++) {
            int idx = indices[p];
            if (idx == query_pos) {
                sparse_weights[weight_count++] = 0.0f;
                continue;
            }

            int8_t cval = coord4_dim(walk[idx], dim);
            int delta = (int)qval - (int)cval;
            if (delta < 0) delta = -delta;
            float coord_weight = 1.0f / (1.0f + (float)delta);

            float decay = attn_rc_relevance(query_pos - idx, walk[idx],
                                       ATTN_RC_DEFAULT);
            float w = coord_weight * decay;
            sparse_weights[weight_count++] = w;
            total_weight += w;
            weighted_sum += w * (float)cval;
        }

        /* Normalize for concentration measurement */
        if (total_weight > 1e-10f) {
            dim_vals[dim] = weighted_sum / total_weight;
            for (int p = 0; p < weight_count; p++)
                sparse_weights[p] /= total_weight;
            total_conc += attention_concentration(sparse_weights,
                                                  0, weight_count);
        } else {
            dim_vals[dim] = (float)qval;
            total_conc += 0.0f;
        }
    }

    result.context.t = (int8_t)roundf(dim_vals[0]);
    result.context.x = (int8_t)roundf(dim_vals[1]);
    result.context.m = (int8_t)roundf(dim_vals[2]);
    result.context.q = (int8_t)roundf(dim_vals[3]);
    result.confidence = total_conc / 4.0f;

    return result;
}


/* ================================================================
 * Causal sparse attention — the generation kernel
 * ================================================================
 *
 * Combines both constraints:
 *   - Chirality: only look backward (Prediction 16)
 *   - Sparsity: only attend to nearest 25% (Prediction 7)
 *
 * This is the kernel used for autoregressive generation.
 * At each position, it looks at the past, selects the most
 * coordinate-proximate 25% of past positions (by Chebyshev
 * distance), and computes 4-head dimensional attention over them.
 *
 * The result predicts: "given my recent walk, what coordinate
 * comes next?"  Feed that into the density map to get the
 * actual probability distribution over wave bytes.
 */

static inline AttentionResult causal_sparse_attention(
        const coord4 *walk, int walk_len, int query_pos,
        float sparsity) {

    (void)walk_len;  /* causal: only [0, query_pos) matters */

    if (query_pos <= 0) {
        AttentionResult r;
        r.context = walk[0];
        r.context_f[0] = (float)walk[0].t;
        r.context_f[1] = (float)walk[0].x;
        r.context_f[2] = (float)walk[0].m;
        r.context_f[3] = (float)walk[0].q;
        r.confidence = 0.0f;
        return r;
    }

    /*
     * Build a causal sub-walk: only positions [0, query_pos).
     * Then apply sparse selection within that sub-walk.
     *
     * We reuse sparse_attention on a truncated walk.
     * The walk pointer stays the same, but we pretend it's
     * shorter — only `query_pos` elements.  sparse_attention
     * will only look at indices [0, query_pos).  Since
     * query_pos is now == walk_len for the sub-walk, it won't
     * attend to itself (handled by the i == query_pos check).
     *
     * Actually, we need to pass the actual query_pos so RC
     * decay uses the real positional distance.  Construct
     * a temporary that includes the query at the end.
     */

    /* Simpler approach: manually implement the causal sparse kernel */
    if (sparsity <= 0.0f) sparsity = ATTN_SPARSITY_DEFAULT;
    if (sparsity > 1.0f) sparsity = 1.0f;

    int past_len = query_pos;  /* number of positions before query */
    int k = (int)(sparsity * (float)past_len);
    if (k < 1) k = 1;

    coord4 q = walk[query_pos];

    /* Compute Chebyshev distances to all past positions */
    int dists[1024];
    int indices[1024];
    int n = (past_len <= 1024) ? past_len : 1024;

    for (int i = 0; i < n; i++) {
        /* Chebyshev: max of absolute per-dimension differences */
        int dt = (int)q.t - (int)walk[i].t; if (dt < 0) dt = -dt;
        int dx = (int)q.x - (int)walk[i].x; if (dx < 0) dx = -dx;
        int dm = (int)q.m - (int)walk[i].m; if (dm < 0) dm = -dm;
        int dq = (int)q.q - (int)walk[i].q; if (dq < 0) dq = -dq;
        int max = dt;
        if (dx > max) max = dx;
        if (dm > max) max = dm;
        if (dq > max) max = dq;
        dists[i] = max;
        indices[i] = i;
    }

    if (k > n) k = n;

    /* Select k nearest past positions */
    for (int picked = 0; picked < k; picked++) {
        int best = picked;
        for (int j = picked + 1; j < n; j++) {
            if (dists[j] < dists[best])
                best = j;
        }
        if (best != picked) {
            int tmp_d = dists[picked];
            dists[picked] = dists[best];
            dists[best] = tmp_d;
            int tmp_i = indices[picked];
            indices[picked] = indices[best];
            indices[best] = tmp_i;
        }
    }

    /* Run 4-head attention over the sparse causal set */
    AttentionResult result;
    float total_conc = 0.0f;
    float dim_vals[4];

    for (int dim = 0; dim < 4; dim++) {
        int8_t qval = coord4_dim(q, dim);
        float total_weight = 0.0f;
        float weighted_sum = 0.0f;
        float sw[1024];
        int wc = 0;

        for (int p = 0; p < k; p++) {
            int idx = indices[p];
            int8_t cval = coord4_dim(walk[idx], dim);
            int delta = (int)qval - (int)cval;
            if (delta < 0) delta = -delta;
            float coord_weight = 1.0f / (1.0f + (float)delta);
            float decay = attn_rc_relevance(query_pos - idx, walk[idx],
                                       ATTN_RC_DEFAULT);
            float w = coord_weight * decay;
            sw[wc++] = w;
            total_weight += w;
            weighted_sum += w * (float)cval;
        }

        if (total_weight > 1e-10f) {
            dim_vals[dim] = weighted_sum / total_weight;
            for (int p = 0; p < wc; p++)
                sw[p] /= total_weight;
            total_conc += attention_concentration(sw, 0, wc);
        } else {
            dim_vals[dim] = (float)qval;
        }
    }

    result.context.t = (int8_t)roundf(dim_vals[0]);
    result.context.x = (int8_t)roundf(dim_vals[1]);
    result.context.m = (int8_t)roundf(dim_vals[2]);
    result.context.q = (int8_t)roundf(dim_vals[3]);
    result.context_f[0] = dim_vals[0];
    result.context_f[1] = dim_vals[1];
    result.context_f[2] = dim_vals[2];
    result.context_f[3] = dim_vals[3];
    result.confidence = total_conc / 4.0f;

    return result;
}


/*
 * Float-walk attention — operates on float4 walk instead of coord4.
 *
 * "The path IS the function." — mind/mind
 *
 * The walk polynomial is continuous. Attention over float values
 * produces continuously-varying output. Each generated step shifts
 * the trajectory, breaking deterministic cycles.
 */
static inline AttentionResult causal_sparse_attention_f(
        const float4 *walk, int walk_len, int query_pos,
        float sparsity) {

    AttentionResult result;
    memset(&result, 0, sizeof(result));

    if (query_pos <= 0) {
        result.context_f[0] = walk[0].t;
        result.context_f[1] = walk[0].x;
        result.context_f[2] = walk[0].m;
        result.context_f[3] = walk[0].q;
        result.context = float4_to_coord4(walk[0]);
        result.confidence = 0.0f;
        return result;
    }

    (void)walk_len;
    if (sparsity <= 0.0f) sparsity = ATTN_SPARSITY_DEFAULT;
    if (sparsity > 1.0f) sparsity = 1.0f;

    int past_len = query_pos;
    int k = (int)(sparsity * (float)past_len);
    if (k < 1) k = 1;

    float4 q = walk[query_pos];

    /* Chebyshev distances on FLOAT values */
    float dists[1024];
    int indices[1024];
    int n = (past_len <= 1024) ? past_len : 1024;

    for (int i = 0; i < n; i++) {
        float dt = fabsf(q.t - walk[i].t);
        float dx = fabsf(q.x - walk[i].x);
        float dm = fabsf(q.m - walk[i].m);
        float dq = fabsf(q.q - walk[i].q);
        float mx = dt;
        if (dx > mx) mx = dx;
        if (dm > mx) mx = dm;
        if (dq > mx) mx = dq;
        dists[i] = mx;
        indices[i] = i;
    }

    if (k > n) k = n;

    /* select k nearest */
    for (int picked = 0; picked < k; picked++) {
        int best = picked;
        for (int j = picked + 1; j < n; j++)
            if (dists[j] < dists[best]) best = j;
        if (best != picked) {
            float tmp_d = dists[picked]; dists[picked] = dists[best]; dists[best] = tmp_d;
            int tmp_i = indices[picked]; indices[picked] = indices[best]; indices[best] = tmp_i;
        }
    }

    /* 4-head attention over float walk */
    float dim_vals[4];
    float total_conc = 0.0f;

    for (int dim = 0; dim < 4; dim++) {
        float qval;
        switch(dim) { case 0: qval=q.t; break; case 1: qval=q.x; break;
                      case 2: qval=q.m; break; default: qval=q.q; break; }

        float total_weight = 0.0f, weighted_sum = 0.0f;
        float sw[1024];
        int wc = 0;

        for (int p = 0; p < k; p++) {
            int idx = indices[p];
            float cval;
            switch(dim) { case 0: cval=walk[idx].t; break; case 1: cval=walk[idx].x; break;
                          case 2: cval=walk[idx].m; break; default: cval=walk[idx].q; break; }

            float delta = fabsf(qval - cval);
            float coord_weight = 1.0f / (1.0f + delta);
            /* RC decay: use position distance, capacitance from mu */
            float cap = fabsf(walk[idx].m);
            if (cap < 0.5f) cap = 0.5f;
            float R = 1.0f;
            float tau_rc = R * cap;
            float decay = expf(-(float)(query_pos - idx) / tau_rc);
            float w = coord_weight * decay;
            sw[wc++] = w;
            total_weight += w;
            weighted_sum += w * cval;
        }

        if (total_weight > 1e-10f) {
            dim_vals[dim] = weighted_sum / total_weight;
            for (int p = 0; p < wc; p++) sw[p] /= total_weight;
            total_conc += attention_concentration(sw, 0, wc);
        } else {
            dim_vals[dim] = qval;
        }
    }

    result.context.t = (int8_t)roundf(dim_vals[0]);
    result.context.x = (int8_t)roundf(dim_vals[1]);
    result.context.m = (int8_t)roundf(dim_vals[2]);
    result.context.q = (int8_t)roundf(dim_vals[3]);
    result.context_f[0] = dim_vals[0];
    result.context_f[1] = dim_vals[1];
    result.context_f[2] = dim_vals[2];
    result.context_f[3] = dim_vals[3];
    result.confidence = total_conc / 4.0f;

    return result;
}


#endif /* ATTENTION_H */
