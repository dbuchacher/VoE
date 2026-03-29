/*
 * chat.c — interactive chat with the born-indexed model
 *
 * Type a question, get a continuation from the index.
 * It completes your text based on what Wikipedia says after similar contexts.
 *
 * Usage:
 *   ./chat data/wiki.idx
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define CTX_N 8

#define HASH_BITS  24
#define HASH_SIZE  (1 << HASH_BITS)
#define HASH_MASK  (HASH_SIZE - 1)

typedef struct {
    uint8_t  key[CTX_N];
    uint8_t  occupied;
    uint8_t  _pad[7];
    uint16_t counts[256];
} Slot;

static Slot *table;

static uint32_t hash_context(const uint8_t *ctx) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < CTX_N; i++) {
        h ^= ctx[i];
        h *= 16777619u;
    }
    return h & HASH_MASK;
}

static Slot *table_lookup(const uint8_t *ctx) {
    uint32_t h = hash_context(ctx);
    for (int probe = 0; probe < 64; probe++) {
        uint32_t idx = (h + probe) & HASH_MASK;
        Slot *s = &table[idx];
        if (!s->occupied) return NULL;
        if (memcmp(s->key, ctx, CTX_N) == 0) return s;
    }
    return NULL;
}

static int load_index(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }

    uint32_t magic, ctx_n, hash_size;
    if (fread(&magic, 4, 1, f) != 1) { fclose(f); return -1; }
    if (fread(&ctx_n, 4, 1, f) != 1) { fclose(f); return -1; }
    if (fread(&hash_size, 4, 1, f) != 1) { fclose(f); return -1; }

    if (magic != 0x57415645 || ctx_n != CTX_N || hash_size != HASH_SIZE) {
        fprintf(stderr, "bad index file\n");
        fclose(f);
        return -1;
    }

    long loaded = 0;
    uint32_t slot_idx;
    while (fread(&slot_idx, 4, 1, f) == 1) {
        Slot *s = &table[slot_idx];
        if (fread(s->key, CTX_N, 1, f) != 1) break;
        if (fread(s->counts, sizeof(uint16_t), 256, f) != 256) break;
        s->occupied = 1;
        loaded++;
    }

    fclose(f);
    fprintf(stderr, "loaded %ld contexts\n", loaded);
    return 0;
}

static uint8_t sample_next(const uint8_t *ctx, float temperature) {
    Slot *s = table_lookup(ctx);

    /* backoff: try shorter matches */
    if (!s) {
        uint8_t shorter[CTX_N];
        for (int shift = 1; shift < CTX_N - 2; shift++) {
            memset(shorter, ' ', shift);
            memcpy(shorter + shift, ctx + shift, CTX_N - shift);
            s = table_lookup(shorter);
            if (s) break;
        }
        if (!s) return ' ';
    }

    if (temperature < 0.01f) {
        int best = 0;
        for (int i = 1; i < 256; i++)
            if (s->counts[i] > s->counts[best]) best = i;
        return (uint8_t)best;
    }

    uint32_t total = 0;
    for (int i = 0; i < 256; i++) total += s->counts[i];
    if (total == 0) return ' ';

    uint32_t r = rand() % total;
    uint32_t cumsum = 0;
    for (int i = 0; i < 256; i++) {
        cumsum += s->counts[i];
        if (r < cumsum) return (uint8_t)i;
    }
    return ' ';
}

static void generate(const char *prompt, int gen_len, float temp) {
    uint8_t ctx[CTX_N];
    int plen = strlen(prompt);

    /* use last CTX_N bytes of prompt as context */
    if (plen >= CTX_N) {
        memcpy(ctx, prompt + plen - CTX_N, CTX_N);
    } else {
        memset(ctx, ' ', CTX_N);
        memcpy(ctx + CTX_N - plen, prompt, plen);
    }

    for (int i = 0; i < gen_len; i++) {
        uint8_t next = sample_next(ctx, temp);
        putchar(next);
        fflush(stdout);

        memmove(ctx, ctx + 1, CTX_N - 1);
        ctx[CTX_N - 1] = next;

        /* stop on double newline (natural paragraph break) */
        if (i > 20 && ctx[CTX_N-1] == '\n' && ctx[CTX_N-2] == '\n')
            break;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <index.idx>\n", argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL));

    table = calloc(HASH_SIZE, sizeof(Slot));
    if (!table) { fprintf(stderr, "out of memory\n"); return 1; }

    fprintf(stderr, "loading index...\n");
    if (load_index(argv[1]) < 0) return 1;
    fprintf(stderr, "ready.\n\n");

    char line[4096];

    for (;;) {
        printf("\033[1;35myou:\033[0m ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        /* strip newline */
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        if (len == 0) continue;
        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) break;

        printf("\033[1;36mbot:\033[0m ");
        fflush(stdout);
        generate(line, 500, 0.8f);
        printf("\n\n");
    }

    free(table);
    return 0;
}
