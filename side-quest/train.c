/*
 * train.c — train the lattice model on wave byte sequences
 *
 * Usage:
 *   bzcat data/enwiki-latest-pages-articles.xml.bz2 | ./wiki2wave > data/wiki.wave
 *   ./train data/wiki.wave
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "model.h"

#define BATCH_SIZE     32
#define LEARN_RATE     0.001f
#define PRINT_EVERY    100
#define SAVE_EVERY     5000
#define GEN_EVERY      2000

static Weights    *weights;
static Activations *acts;
static Gradients  *grads;
static Gradients  *grad_accum;

/*
 * Training data: mmap'd be ideal but for now just load a chunk into heap.
 * 14GB file — we load 256MB at a time, cycle through.
 */
#define DATA_BUF_SIZE (256 * 1024 * 1024)  /* 256MB chunks */
static uint8_t *data_buf;
static long data_len = 0;
static long data_pos = 0;
static FILE *data_file = NULL;
static long file_offset = 0;
static long file_size = 0;

static long load_chunk(void) {
    if (!data_file) return -1;
    fseek(data_file, file_offset, SEEK_SET);
    data_len = fread(data_buf, 1, DATA_BUF_SIZE, data_file);
    data_pos = 0;
    if (data_len < CTX_LEN + 1) {
        /* wrap to start of file */
        file_offset = 0;
        fseek(data_file, 0, SEEK_SET);
        data_len = fread(data_buf, 1, DATA_BUF_SIZE, data_file);
        data_pos = 0;
        return 0; /* signal epoch */
    }
    file_offset += data_len - CTX_LEN; /* overlap for context continuity */
    return data_len;
}

static int open_data(const char *path) {
    data_file = fopen(path, "rb");
    if (!data_file) { perror(path); return -1; }
    fseek(data_file, 0, SEEK_END);
    file_size = ftell(data_file);
    fseek(data_file, 0, SEEK_SET);
    file_offset = 0;
    fprintf(stderr, "data file: %s (%.1f GB)\n", path, (float)file_size / (1024*1024*1024));
    return load_chunk();
}

static int get_sample(uint8_t *context, int *target) {
    if (data_pos + CTX_LEN + 1 >= data_len) {
        long r = load_chunk();
        if (r == 0) return 0;  /* epoch boundary */
        if (r < 0) return -1;
    }

    memcpy(context, &data_buf[data_pos], CTX_LEN);
    uint8_t target_byte = data_buf[data_pos + CTX_LEN];

    if (target_byte == 0xFF) {
        data_pos += CTX_LEN + 1;
        return -1;
    }

    coord4 tc = wave_decode(target_byte);
    *target = coord_to_index(tc);
    data_pos++;
    return 1;
}

static char wave_to_char(uint8_t wb) {
    if (wb == 0x00) return ' ';
    for (int ch = 'a'; ch <= 'z'; ch++) {
        uint8_t test = utf8_to_wave((uint8_t)ch);
        if (test == wb) return (char)ch;
    }
    return '?';
}

static void generate_sample(int len, float temperature) {
    uint8_t context[CTX_LEN];
    int start = rand() % (data_len - CTX_LEN - 1);
    memcpy(context, &data_buf[start], CTX_LEN);

    fprintf(stderr, "  gen: [");
    for (int i = CTX_LEN - 20; i < CTX_LEN; i++)
        fprintf(stderr, "%c", wave_to_char(context[i]));
    fprintf(stderr, "]→ ");

    for (int i = 0; i < len; i++) {
        model_forward(weights, acts, context);
        int idx = model_sample(acts, temperature);
        coord4 c = index_to_coord(idx);
        uint8_t wb = wave_encode(c);
        fprintf(stderr, "%c", wave_to_char(wb));
        memmove(context, context + 1, CTX_LEN - 1);
        context[CTX_LEN - 1] = wb;
    }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <data.wave>\n", argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL));

    /* heap allocate everything — model is ~8MB with hidden=1024 */
    weights    = calloc(1, sizeof(Weights));
    acts       = calloc(1, sizeof(Activations));
    grads      = calloc(1, sizeof(Gradients));
    grad_accum = calloc(1, sizeof(Gradients));
    data_buf   = malloc(DATA_BUF_SIZE);
    if (!weights || !acts || !grads || !grad_accum || !data_buf) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    fprintf(stderr, "lattice model: ctx=%d, hidden=%d, output=%d\n",
            CTX_LEN, HIDDEN_DIM, OUTPUT_DIM);
    fprintf(stderr, "parameters: %ld (%.1f MB)\n",
            (long)(sizeof(Weights) / sizeof(float)),
            (float)sizeof(Weights) / (1024*1024));
    fprintf(stderr, "threads: %d\n", omp_get_max_threads());

    if (model_load(weights, "model.bin") == 0) {
        fprintf(stderr, "loaded weights from model.bin\n");
    } else {
        fprintf(stderr, "initializing random weights\n");
        model_init(weights);
    }

    if (open_data(argv[1]) < 0) return 1;

    long step = 0, epoch = 0;
    float loss_sum = 0.0f;
    int loss_count = 0, batch_count = 0;
    uint8_t context[CTX_LEN];
    int target;
    struct timespec t_start, t_now;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    fprintf(stderr, "training...\n");

    for (;;) {
        int r = get_sample(context, &target);
        if (r == 0) {
            epoch++;
            fprintf(stderr, "\n=== epoch %ld ===\n", epoch);
            if (epoch >= 5) break;
            continue;
        }
        if (r == -1) continue;

        model_forward(weights, acts, context);
        float loss = model_loss(acts, target);
        loss_sum += loss;
        loss_count++;

        model_backward(weights, acts, grads, target);

        /* accumulate gradients */
        float *ap = (float *)grad_accum;
        const float *gp = (const float *)grads;
        long n = sizeof(Gradients) / sizeof(float);
        for (long i = 0; i < n; i++)
            ap[i] += gp[i];
        batch_count++;

        if (batch_count >= BATCH_SIZE) {
            float scale = 1.0f / (float)batch_count;
            for (long i = 0; i < n; i++)
                ap[i] *= scale;
            model_update(weights, grad_accum, LEARN_RATE);
            memset(grad_accum, 0, sizeof(Gradients));
            batch_count = 0;
        }

        step++;

        if (step % PRINT_EVERY == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t_now);
            double elapsed = (t_now.tv_sec - t_start.tv_sec) +
                             (t_now.tv_nsec - t_start.tv_nsec) * 1e-9;
            float avg_loss = loss_sum / (float)loss_count;
            fprintf(stderr, "step %7ld | loss %.4f | %.0f steps/s | epoch %ld | %.0fs\n",
                    step, avg_loss, (float)step / elapsed, epoch, elapsed);
            loss_sum = 0.0f;
            loss_count = 0;
        }

        if (step % GEN_EVERY == 0)
            generate_sample(80, 0.8f);

        if (step % SAVE_EVERY == 0) {
            model_save(weights, "model.bin");
            fprintf(stderr, "  saved model.bin\n");
        }
    }

    model_save(weights, "model.bin");
    fprintf(stderr, "saved model.bin\n");
    fprintf(stderr, "\nfinal generation:\n");
    generate_sample(200, 0.8f);
    generate_sample(200, 0.5f);
    generate_sample(200, 0.0f);  /* greedy */

    free(weights); free(acts); free(grads); free(grad_accum); free(data_buf);
    if (data_file) fclose(data_file);
    return 0;
}
