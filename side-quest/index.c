/*
 * index.c — born-index wave byte sequences for O(1) next-byte prediction
 *
 * Reads wave byte data, builds n-gram frequency table.
 * For every N-byte context, counts what byte follows.
 * Saves index to disk. One pass. No training.
 *
 * Usage:
 *   ./index data/wiki.wave data/wiki.idx
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/*
 * N-gram context length. Longer = more coherent but sparser.
 * 8 bytes gives word-level patterns in English (~1.5 words average).
 */
#define CTX_N 8

/*
 * Hash table for n-gram → next-byte frequency.
 *
 * Key: CTX_N bytes (the context)
 * Value: 256 × uint32 counts (frequency of each possible next byte)
 *
 * We use a large flat hash table with open addressing.
 * Each slot: CTX_N bytes key + 1 byte occupied flag + 256 × uint16 counts
 * (uint16 to save memory — saturates at 65535, good enough for frequency)
 */

#define HASH_BITS  24                    /* 16M slots */
#define HASH_SIZE  (1 << HASH_BITS)
#define HASH_MASK  (HASH_SIZE - 1)

typedef struct {
    uint8_t  key[CTX_N];
    uint8_t  occupied;
    uint8_t  _pad[7];       /* alignment */
    uint16_t counts[256];   /* next-byte frequency */
} Slot;

/* FNV-1a hash */
static uint32_t hash_context(const uint8_t *ctx) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < CTX_N; i++) {
        h ^= ctx[i];
        h *= 16777619u;
    }
    return h & HASH_MASK;
}

static Slot *table;
static long collisions = 0;
static long unique_contexts = 0;
static long total_samples = 0;

static void table_insert(const uint8_t *ctx, uint8_t next_byte) {
    uint32_t h = hash_context(ctx);

    /* linear probing */
    for (int probe = 0; probe < 64; probe++) {
        uint32_t idx = (h + probe) & HASH_MASK;
        Slot *s = &table[idx];

        if (!s->occupied) {
            /* new entry */
            memcpy(s->key, ctx, CTX_N);
            s->occupied = 1;
            if (s->counts[next_byte] < 65535)
                s->counts[next_byte]++;
            unique_contexts++;
            return;
        }

        if (memcmp(s->key, ctx, CTX_N) == 0) {
            /* existing entry — increment count */
            if (s->counts[next_byte] < 65535)
                s->counts[next_byte]++;
            return;
        }

        collisions++;
    }

    /* probe limit reached — drop this sample (table too full) */
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

/* ── Index builder ───────────────────────────────────────────── */

static int build_index(const char *data_path) {
    FILE *f = fopen(data_path, "rb");
    if (!f) { perror(data_path); return -1; }

    /* get file size */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    fprintf(stderr, "indexing %.1f GB of wave data (ctx=%d)...\n",
            (float)file_size / (1024*1024*1024), CTX_N);

    /* read in chunks, index everything */
    #define READ_BUF (64 * 1024 * 1024)
    uint8_t *buf = malloc(READ_BUF + CTX_N);
    if (!buf) { fprintf(stderr, "out of memory\n"); return -1; }

    uint8_t overlap[CTX_N];
    int have_overlap = 0;
    long bytes_read = 0;
    struct timespec t_start;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    while (1) {
        /* prepend overlap from previous chunk */
        int offset = 0;
        if (have_overlap) {
            memcpy(buf, overlap, CTX_N);
            offset = CTX_N;
        }

        long n = fread(buf + offset, 1, READ_BUF, f);
        if (n <= 0) break;
        n += offset;
        bytes_read += n - offset;

        /* index all positions in this chunk */
        for (long i = 0; i + CTX_N < n; i++) {
            uint8_t *ctx = &buf[i];
            uint8_t next = buf[i + CTX_N];

            /* skip if context contains separator (0xFF) */
            int has_sep = 0;
            for (int j = 0; j <= CTX_N; j++) {
                if (ctx[j] == 0xFF) { has_sep = 1; break; }
            }
            if (has_sep) continue;

            table_insert(ctx, next);
            total_samples++;

            if (total_samples % 50000000 == 0) {
                struct timespec t_now;
                clock_gettime(CLOCK_MONOTONIC, &t_now);
                double elapsed = (t_now.tv_sec - t_start.tv_sec) +
                                 (t_now.tv_nsec - t_start.tv_nsec) * 1e-9;
                fprintf(stderr, "\r  %.1f GB indexed | %ld M samples | %ld M contexts | %.0f M/s",
                        (float)bytes_read / (1024*1024*1024),
                        total_samples / 1000000,
                        unique_contexts / 1000000,
                        (float)total_samples / elapsed / 1000000);
            }
        }

        /* save overlap for next chunk */
        if (n >= CTX_N) {
            memcpy(overlap, &buf[n - CTX_N], CTX_N);
            have_overlap = 1;
        }

        /* check if table is getting too full (>75%) */
        if (unique_contexts > HASH_SIZE * 3 / 4) {
            fprintf(stderr, "\nhash table 75%% full (%ld / %d), stopping index\n",
                    unique_contexts, HASH_SIZE);
            break;
        }
    }

    struct timespec t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) +
                     (t_end.tv_nsec - t_start.tv_nsec) * 1e-9;

    fprintf(stderr, "\ndone: %ld samples, %ld unique contexts, %ld collisions, %.1fs\n",
            total_samples, unique_contexts, collisions, elapsed);

    free(buf);
    fclose(f);
    return 0;
}

/* ── Save/load index ─────────────────────────────────────────── */

static int save_index(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }

    /* header */
    uint32_t magic = 0x57415645; /* "WAVE" */
    uint32_t ctx_n = CTX_N;
    uint32_t hash_size = HASH_SIZE;
    fwrite(&magic, 4, 1, f);
    fwrite(&ctx_n, 4, 1, f);
    fwrite(&hash_size, 4, 1, f);

    /* only write occupied slots to save space */
    long written = 0;
    for (long i = 0; i < HASH_SIZE; i++) {
        if (table[i].occupied) {
            uint32_t slot_idx = (uint32_t)i;
            fwrite(&slot_idx, 4, 1, f);
            fwrite(table[i].key, CTX_N, 1, f);
            fwrite(table[i].counts, sizeof(uint16_t), 256, f);
            written++;
        }
    }

    fprintf(stderr, "saved %ld contexts to %s (%.1f MB)\n",
            written, path,
            (float)(written * (4 + CTX_N + 512)) / (1024*1024));
    fclose(f);
    return 0;
}

static int load_index(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }

    uint32_t magic, ctx_n, hash_size;
    fread(&magic, 4, 1, f);
    fread(&ctx_n, 4, 1, f);
    fread(&hash_size, 4, 1, f);

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

    fprintf(stderr, "loaded %ld contexts from %s\n", loaded, path);
    fclose(f);
    unique_contexts = loaded;
    return 0;
}

/* ── Generator ───────────────────────────────────────────────── */

static uint8_t sample_next(const uint8_t *ctx, float temperature) {
    Slot *s = table_lookup(ctx);
    if (!s) {
        /* back off: try shifting context left, padding with space */
        uint8_t shorter[CTX_N];
        for (int shift = 1; shift < CTX_N; shift++) {
            memset(shorter, ' ', shift);
            memcpy(shorter + shift, ctx + shift, CTX_N - shift);
            s = table_lookup(shorter);
            if (s) break;
        }
        if (!s) return ' '; /* truly no match */
    }

    if (temperature < 0.01f) {
        /* greedy */
        int best = 0;
        for (int i = 1; i < 256; i++)
            if (s->counts[i] > s->counts[best]) best = i;
        return (uint8_t)best;
    }

    /* weighted random sampling */
    uint32_t total = 0;
    for (int i = 0; i < 256; i++) total += s->counts[i];
    if (total == 0) return 0x00;

    uint32_t r = rand() % total;
    uint32_t cumsum = 0;
    for (int i = 0; i < 256; i++) {
        cumsum += s->counts[i];
        if (r < cumsum) return (uint8_t)i;
    }
    return 0x00;
}

/* quick reverse: wave byte → printable char */
static char wb_to_char(uint8_t wb) {
    if (wb == 0x00) return ' ';
    if (wb == 0xFF) return '|';
    /* brute force reverse lookup — fine for display */
    for (int ch = 'a'; ch <= 'z'; ch++) {
        /* inline the lookup to avoid needing lattice.h here */
        /* we'll match against known wave bytes */
    }
    /* just show the hex nibble as a letter proxy */
    int v = (wb >> 4) & 0xF;
    if (v < 10) return '0' + v;
    return 'a' + (v - 10);
}

static void generate(const uint8_t *seed, int seed_len, int gen_len, float temp) {
    uint8_t ctx[CTX_N];

    /* pad or truncate seed to CTX_N */
    memset(ctx, 0x00, CTX_N);
    int copy = seed_len < CTX_N ? seed_len : CTX_N;
    memcpy(ctx + CTX_N - copy, seed, copy);

    for (int i = 0; i < gen_len; i++) {
        uint8_t next = sample_next(ctx, temp);
        fputc(next, stdout);

        memmove(ctx, ctx + 1, CTX_N - 1);
        ctx[CTX_N - 1] = next;
    }
}

/* ── Main ────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage:\n");
        fprintf(stderr, "  build index:  %s build <data.wave> <output.idx>\n", argv[0]);
        fprintf(stderr, "  generate:     %s gen <index.idx> [seed_text] [length]\n", argv[0]);
        fprintf(stderr, "  stats:        %s stats <index.idx>\n", argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL));

    /* allocate hash table */
    table = calloc(HASH_SIZE, sizeof(Slot));
    if (!table) {
        fprintf(stderr, "can't allocate %lu MB for hash table\n",
                (unsigned long)(HASH_SIZE * sizeof(Slot)) / (1024*1024));
        return 1;
    }
    fprintf(stderr, "hash table: %d slots, %lu MB\n",
            HASH_SIZE, (unsigned long)(HASH_SIZE * sizeof(Slot)) / (1024*1024));

    if (strcmp(argv[1], "build") == 0) {
        if (argc < 4) { fprintf(stderr, "need: build <data> <output>\n"); return 1; }
        if (build_index(argv[2]) < 0) return 1;
        save_index(argv[3]);

    } else if (strcmp(argv[1], "gen") == 0) {
        if (argc < 3) { fprintf(stderr, "need: gen <index>\n"); return 1; }
        if (load_index(argv[2]) < 0) return 1;

        int gen_len = 500;
        if (argc >= 5) gen_len = atoi(argv[4]);

        /* seed from command line or random from data */
        uint8_t seed[CTX_N];
        memset(seed, ' ', CTX_N); /* pad with spaces */
        if (argc >= 4 && strcmp(argv[3], "-") != 0) {
            const char *s = argv[3];
            int slen = strlen(s);
            if (slen >= CTX_N) {
                /* use last CTX_N bytes */
                memcpy(seed, s + slen - CTX_N, CTX_N);
            } else {
                /* right-align: pad left with spaces */
                memcpy(seed + CTX_N - slen, s, slen);
            }
        } else {
            /* random seed: find a random occupied slot */
            for (long i = rand() % HASH_SIZE; ; i = (i+1) & HASH_MASK)
                if (table[i].occupied) { memcpy(seed, table[i].key, CTX_N); break; }
        }

        fprintf(stderr, "generating %d bytes...\n", gen_len);
        generate(seed, CTX_N, gen_len, 0.8f);
        printf("\n");

    } else if (strcmp(argv[1], "stats") == 0) {
        if (load_index(argv[2]) < 0) return 1;

        /* count distribution of context frequencies */
        long total_count = 0;
        long max_count = 0;
        for (long i = 0; i < HASH_SIZE; i++) {
            if (!table[i].occupied) continue;
            long sum = 0;
            for (int j = 0; j < 256; j++) sum += table[i].counts[j];
            total_count += sum;
            if (sum > max_count) max_count = sum;
        }
        fprintf(stderr, "unique contexts: %ld\n", unique_contexts);
        fprintf(stderr, "total observations: %ld\n", total_count);
        fprintf(stderr, "avg obs/context: %.1f\n",
                (float)total_count / unique_contexts);
        fprintf(stderr, "max obs for single context: %ld\n", max_count);

    } else {
        fprintf(stderr, "unknown command: %s\n", argv[1]);
        return 1;
    }

    free(table);
    return 0;
}
