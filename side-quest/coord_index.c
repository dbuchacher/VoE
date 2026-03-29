/*
 * coord_index.c — born-index over word COORDINATES for next-word prediction
 *
 * Component 1.4 of the lattice-native intelligence system.
 *
 * This is the density map that replaces trained weights. Instead of running
 * gradient descent to learn which outputs follow which inputs, we directly
 * COUNT which coordinate regions follow which context regions. One pass.
 *
 * The key move: we don't index exact sequences. We quantize coordinates
 * into bins (shell-1: 81 bins) and context windows into fingerprints
 * (weighted centroids). Similar words land in the same bin. Similar
 * contexts produce the same fingerprint. The geometry does the work
 * that an embedding layer would have to learn.
 *
 * What this replaces in a neural net:
 *   - Embedding layer:     coordinates ARE the embedding (lattice.h)
 *   - Attention/context:   fingerprint IS the context summary
 *   - Output weights:      bin counts ARE the output distribution
 *   - Training:            one-pass counting IS the learning
 *
 * What it doesn't do (yet):
 *   - Multi-resolution bins (shell 2+ would give finer bins)
 *   - Multiple fingerprint strategies (weighted sum is just the first)
 *   - Backoff (if fingerprint not found, try coarser quantization)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "coord_index.h"

/* ── Coordinate utilities ────────────────────────────────────── */

/*
 * Pack coord4 → 32-bit key.
 *
 * Layout: [t as uint8][d as uint8][m as uint8][q as uint8]
 * int8_t values reinterpreted as uint8_t for bit packing.
 * This gives a unique key for every possible coord4 in [-128,+127].
 */
uint32_t coord4_pack(coord4 c) {
    return ((uint32_t)(uint8_t)c.t << 24) |
           ((uint32_t)(uint8_t)c.d << 16) |
           ((uint32_t)(uint8_t)c.m <<  8) |
           ((uint32_t)(uint8_t)c.q <<  0);
}

coord4 coord4_unpack(uint32_t key) {
    coord4 c;
    c.t = (int8_t)(uint8_t)(key >> 24);
    c.d = (int8_t)(uint8_t)(key >> 16);
    c.m = (int8_t)(uint8_t)(key >>  8);
    c.q = (int8_t)(uint8_t)(key >>  0);
    return c;
}

/*
 * Clamp an integer to a range. Used everywhere.
 */
static inline int clamp_i(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/*
 * Quantize one dimension to shell-1: {-1, 0, +1}.
 *
 * Negative values → -1, zero → 0, positive → +1.
 * This is the coarsest possible binning. It works because
 * the coordinate system already encodes semantics — even
 * the sign carries meaning (voiced/voiceless, front/back, etc).
 */
static inline int quantize_dim(int8_t val) {
    if (val < 0) return -1;
    if (val > 0) return +1;
    return 0;
}

/*
 * coord_to_bin: map any coord4 → bin index 0-80.
 *
 * Each dimension clamped to {-1, 0, +1}, then encoded as:
 *   bin = (t+1)*27 + (d+1)*9 + (m+1)*3 + (q+1)
 *
 * The center bin (0,0,0,0) = index 40. This is silence/identity.
 * Bin 0 = (-1,-1,-1,-1). Bin 80 = (+1,+1,+1,+1) = 'a'/'w'.
 */
int coord_to_bin(coord4 c) {
    int t = quantize_dim(c.t) + 1;  /* 0, 1, or 2 */
    int d = quantize_dim(c.d) + 1;
    int m = quantize_dim(c.m) + 1;
    int q = quantize_dim(c.q) + 1;
    return t * 27 + d * 9 + m * 3 + q;
}

/*
 * bin_to_coord: inverse of coord_to_bin.
 * Returns the centroid coordinate of the bin.
 */
coord4 bin_to_coord(int bin) {
    coord4 c;
    c.q = (int8_t)((bin % 3) - 1); bin /= 3;
    c.m = (int8_t)((bin % 3) - 1); bin /= 3;
    c.d = (int8_t)((bin % 3) - 1); bin /= 3;
    c.t = (int8_t)((bin % 3) - 1);
    return c;
}

/*
 * Context fingerprint: compress a window of N word coordinates
 * into a single coord4 summary.
 *
 * Algorithm:
 *   For each dimension, compute weighted sum where weight[i] = i+1.
 *   Position 0 (oldest word) gets weight 1.
 *   Position n-1 (newest word) gets weight n.
 *   This gives exponential-ish recency bias without actual exp().
 *
 *   Then divide by total weight to get weighted average,
 *   and clamp to [-3, +3].
 *
 * Why [-3, +3]? Shell-1 coordinates are {-1, 0, +1}.
 * A window of 8 words averaging +1 gives +1. But a burst of
 * recent high values can push the average above 1. Allowing
 * [-3, +3] keeps some of that intensity signal. This gives
 * 7 effective levels per dimension = 7^4 = 2401 distinct fingerprints.
 *
 * The lossy compression is the point: nearby contexts hash together.
 */
coord4 coord4_fingerprint(const coord4 *context, int n) {
    coord4 fp;

    if (n <= 0) {
        fp.t = fp.d = fp.m = fp.q = 0;
        return fp;
    }

    /*
     * Weighted sum. Weights: 1, 2, 3, ..., n.
     * Total weight = n*(n+1)/2.
     */
    int sum_t = 0, sum_d = 0, sum_m = 0, sum_q = 0;
    int total_weight = 0;

    for (int i = 0; i < n; i++) {
        int w = i + 1;  /* recency weight: oldest=1, newest=n */
        sum_t += context[i].t * w;
        sum_d += context[i].d * w;
        sum_m += context[i].m * w;
        sum_q += context[i].q * w;
        total_weight += w;
    }

    /*
     * Divide by total weight to get weighted average.
     * Multiply by 3 to spread the range: a pure +1 average becomes +3.
     * This uses the full [-3, +3] range for shell-1 inputs.
     *
     * Integer arithmetic: (sum * 3 + total/2) / total for rounding.
     * The +total/2 gives round-to-nearest instead of truncation.
     */
    int half = total_weight / 2;
    fp.t = (int8_t)clamp_i((sum_t * 3 + half) / total_weight, -3, 3);
    fp.d = (int8_t)clamp_i((sum_d * 3 + half) / total_weight, -3, 3);
    fp.m = (int8_t)clamp_i((sum_m * 3 + half) / total_weight, -3, 3);
    fp.q = (int8_t)clamp_i((sum_q * 3 + half) / total_weight, -3, 3);

    return fp;
}


/* ── Hash table internals ────────────────────────────────────── */

/*
 * FNV-1a hash of a 32-bit key.
 * Same family as index.c but operating on 4 bytes instead of CTX_N.
 */
static uint32_t hash_key(uint32_t key, uint32_t mask) {
    uint32_t h = 2166136261u;
    h ^= (key >>  0) & 0xFF; h *= 16777619u;
    h ^= (key >>  8) & 0xFF; h *= 16777619u;
    h ^= (key >> 16) & 0xFF; h *= 16777619u;
    h ^= (key >> 24) & 0xFF; h *= 16777619u;
    return h & mask;
}

/*
 * Find or create a slot for the given key.
 * Returns pointer to the slot, or NULL if table is full (probe limit hit).
 * Sets *is_new = 1 if this is a newly occupied slot.
 */
static CISlot *table_find_or_create(CoordIndex *ci, uint32_t key, int *is_new) {
    uint32_t h = hash_key(key, ci->hash_mask);
    *is_new = 0;

    for (int probe = 0; probe < CIDX_MAX_PROBE; probe++) {
        uint32_t idx = (h + probe) & ci->hash_mask;
        CISlot *s = &ci->table[idx];

        if (!s->occupied) {
            /* empty slot: claim it */
            s->key = key;
            s->occupied = 1;
            *is_new = 1;
            return s;
        }

        if (s->key == key) {
            /* exact match */
            return s;
        }

        ci->collisions++;
    }

    /* probe limit reached. table too full. */
    return NULL;
}

/*
 * Lookup only — don't create.
 */
static CISlot *table_lookup(CoordIndex *ci, uint32_t key) {
    uint32_t h = hash_key(key, ci->hash_mask);

    for (int probe = 0; probe < CIDX_MAX_PROBE; probe++) {
        uint32_t idx = (h + probe) & ci->hash_mask;
        CISlot *s = &ci->table[idx];

        if (!s->occupied) return NULL;
        if (s->key == key) return s;
    }

    return NULL;
}


/* ── Public API ──────────────────────────────────────────────── */

CoordIndex *coord_index_create(void) {
    CoordIndex *ci = calloc(1, sizeof(CoordIndex));
    if (!ci) return NULL;

    ci->hash_bits = CIDX_HASH_BITS;
    ci->hash_size = CIDX_HASH_SIZE;
    ci->hash_mask = CIDX_HASH_MASK;

    ci->table = calloc(ci->hash_size, sizeof(CISlot));
    if (!ci->table) {
        free(ci);
        return NULL;
    }

    fprintf(stderr, "coord_index: %u slots, %.1f MB\n",
            ci->hash_size,
            (float)(ci->hash_size * sizeof(CISlot)) / (1024 * 1024));

    return ci;
}

void coord_index_free(CoordIndex *ci) {
    if (!ci) return;
    free(ci->table);
    free(ci);
}

/*
 * Add one observation to the index.
 *
 * "After seeing context with fingerprint `context_fingerprint`,
 *  the next word had coordinate `next_word`."
 *
 * We quantize next_word to its bin (0-80) and increment the count
 * in the fingerprint's slot. That's it. No weights. No gradients.
 * Just counting what follows what, in coordinate space.
 */
void coord_index_add(CoordIndex *ci, coord4 context_fingerprint, coord4 next_word) {
    uint32_t key = coord4_pack(context_fingerprint);
    int bin = coord_to_bin(next_word);

    int is_new = 0;
    CISlot *s = table_find_or_create(ci, key, &is_new);

    if (!s) {
        /* table full, drop this observation */
        return;
    }

    if (is_new) {
        ci->unique_contexts++;
    }

    /* saturating increment — don't wrap around on overflow */
    if (s->counts[bin] < UINT32_MAX) {
        s->counts[bin]++;
    }

    ci->total_adds++;
}

/*
 * Query: what's the probability distribution over bins given this context?
 *
 * Normalizes the raw counts to probabilities.
 * If fingerprint not found, returns uniform distribution (1/81 each).
 * Returns 1 if found, 0 if not.
 */
int coord_index_query(CoordIndex *ci, coord4 context_fingerprint, float *probs_out) {
    uint32_t key = coord4_pack(context_fingerprint);
    CISlot *s = table_lookup(ci, key);

    if (!s) {
        /* unknown context: uniform prior */
        float uniform = 1.0f / CIDX_BINS;
        for (int i = 0; i < CIDX_BINS; i++)
            probs_out[i] = uniform;
        return 0;
    }

    /* sum all counts */
    uint64_t total = 0;
    for (int i = 0; i < CIDX_BINS; i++)
        total += s->counts[i];

    if (total == 0) {
        /* slot exists but empty (shouldn't happen, but be safe) */
        float uniform = 1.0f / CIDX_BINS;
        for (int i = 0; i < CIDX_BINS; i++)
            probs_out[i] = uniform;
        return 0;
    }

    /* normalize counts → probabilities */
    float inv_total = 1.0f / (float)total;
    for (int i = 0; i < CIDX_BINS; i++)
        probs_out[i] = (float)s->counts[i] * inv_total;

    return 1;
}


/* ── Save / Load ─────────────────────────────────────────────── */

/*
 * File format:
 *
 *   Header (16 bytes):
 *     uint32_t magic     = 0x43494458  ("CIDX")
 *     uint32_t version   = 1
 *     uint32_t hash_bits = log2 of table size
 *     uint32_t n_entries = number of occupied slots
 *
 *   For each occupied slot:
 *     uint32_t slot_index   (position in table, for direct placement on load)
 *     uint32_t key          (packed fingerprint)
 *     uint32_t counts[81]   (bin frequencies)
 *
 *   Total per entry: 4 + 4 + 324 = 332 bytes.
 *   Only occupied slots written. Sparse table → small file.
 */

#define CIDX_MAGIC   0x43494458u  /* "CIDX" */
#define CIDX_VERSION 1

int coord_index_save(CoordIndex *ci, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }

    /* header */
    uint32_t magic   = CIDX_MAGIC;
    uint32_t version = CIDX_VERSION;
    uint32_t bits    = (uint32_t)ci->hash_bits;
    uint32_t entries = (uint32_t)ci->unique_contexts;

    fwrite(&magic,   4, 1, f);
    fwrite(&version, 4, 1, f);
    fwrite(&bits,    4, 1, f);
    fwrite(&entries, 4, 1, f);

    /* write occupied slots */
    long written = 0;
    for (uint32_t i = 0; i < ci->hash_size; i++) {
        CISlot *s = &ci->table[i];
        if (!s->occupied) continue;

        fwrite(&i,         4, 1, f);               /* slot index */
        fwrite(&s->key,    4, 1, f);               /* packed fingerprint */
        fwrite(s->counts,  sizeof(uint32_t), CIDX_BINS, f);  /* bin counts */
        written++;
    }

    fprintf(stderr, "coord_index: saved %ld contexts to %s (%.1f MB)\n",
            written, path,
            (float)(16 + written * (4 + 4 + CIDX_BINS * 4)) / (1024 * 1024));

    fclose(f);
    return 0;
}

CoordIndex *coord_index_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }

    /* read header */
    uint32_t magic, version, bits, entries;
    if (fread(&magic,   4, 1, f) != 1 ||
        fread(&version, 4, 1, f) != 1 ||
        fread(&bits,    4, 1, f) != 1 ||
        fread(&entries, 4, 1, f) != 1) {
        fprintf(stderr, "coord_index: truncated header in %s\n", path);
        fclose(f);
        return NULL;
    }

    if (magic != CIDX_MAGIC) {
        fprintf(stderr, "coord_index: bad magic in %s (got 0x%08X, want 0x%08X)\n",
                path, magic, CIDX_MAGIC);
        fclose(f);
        return NULL;
    }

    if (version != CIDX_VERSION) {
        fprintf(stderr, "coord_index: unsupported version %u in %s\n", version, path);
        fclose(f);
        return NULL;
    }

    /* allocate index with the saved table size */
    CoordIndex *ci = calloc(1, sizeof(CoordIndex));
    if (!ci) { fclose(f); return NULL; }

    ci->hash_bits = (int)bits;
    ci->hash_size = 1u << bits;
    ci->hash_mask = ci->hash_size - 1;

    ci->table = calloc(ci->hash_size, sizeof(CISlot));
    if (!ci->table) {
        free(ci);
        fclose(f);
        return NULL;
    }

    /* read entries — placed directly by slot index */
    long loaded = 0;
    for (uint32_t e = 0; e < entries; e++) {
        uint32_t slot_idx, key;

        if (fread(&slot_idx, 4, 1, f) != 1) break;
        if (fread(&key,      4, 1, f) != 1) break;

        if (slot_idx >= ci->hash_size) {
            fprintf(stderr, "coord_index: corrupt slot index %u (table size %u)\n",
                    slot_idx, ci->hash_size);
            break;
        }

        CISlot *s = &ci->table[slot_idx];
        s->key = key;
        s->occupied = 1;

        if (fread(s->counts, sizeof(uint32_t), CIDX_BINS, f) != CIDX_BINS) break;

        loaded++;
    }

    ci->unique_contexts = loaded;

    fprintf(stderr, "coord_index: loaded %ld contexts from %s\n", loaded, path);

    fclose(f);
    return ci;
}


/* ── Statistics ──────────────────────────────────────────────── */

void coord_index_stats(CoordIndex *ci) {
    fprintf(stderr, "\n=== coord_index stats ===\n");
    fprintf(stderr, "table size:       %u slots (2^%d)\n",
            ci->hash_size, ci->hash_bits);
    fprintf(stderr, "memory:           %.1f MB\n",
            (float)(ci->hash_size * sizeof(CISlot)) / (1024 * 1024));
    fprintf(stderr, "unique contexts:  %ld\n", ci->unique_contexts);
    fprintf(stderr, "total adds:       %ld\n", ci->total_adds);
    fprintf(stderr, "collisions:       %ld\n", ci->collisions);
    fprintf(stderr, "load factor:      %.1f%%\n",
            100.0f * ci->unique_contexts / ci->hash_size);

    if (ci->unique_contexts == 0) {
        fprintf(stderr, "(index is empty)\n\n");
        return;
    }

    /*
     * Scan all occupied slots to compute distribution statistics.
     * How concentrated are the observations? If most contexts
     * point to just 1-2 bins, the index is sharp (good).
     * If spread across all 81, it's noisy (need more data or finer bins).
     */
    uint64_t total_observations = 0;
    uint64_t max_single_context = 0;
    long     n_sharp = 0;           /* contexts where top bin > 50% */
    int      global_bin_counts[CIDX_BINS] = {0};

    for (uint32_t i = 0; i < ci->hash_size; i++) {
        CISlot *s = &ci->table[i];
        if (!s->occupied) continue;

        uint64_t ctx_total = 0;
        uint32_t ctx_max_bin = 0;

        for (int b = 0; b < CIDX_BINS; b++) {
            ctx_total += s->counts[b];
            if (s->counts[b] > ctx_max_bin)
                ctx_max_bin = s->counts[b];
            if (s->counts[b] > 0)
                global_bin_counts[b]++;
        }

        total_observations += ctx_total;
        if (ctx_total > max_single_context)
            max_single_context = ctx_total;

        /* sharp = dominant bin has >50% of mass */
        if (ctx_total > 0 && ctx_max_bin * 2 > ctx_total)
            n_sharp++;
    }

    fprintf(stderr, "total observations: %lu\n", (unsigned long)total_observations);
    fprintf(stderr, "avg obs/context:    %.1f\n",
            (float)total_observations / ci->unique_contexts);
    fprintf(stderr, "max obs (single):   %lu\n", (unsigned long)max_single_context);
    fprintf(stderr, "sharp contexts:     %ld / %ld (%.1f%%)\n",
            n_sharp, ci->unique_contexts,
            100.0f * n_sharp / ci->unique_contexts);

    /* how many bins are actually used across all contexts? */
    int bins_used = 0;
    for (int b = 0; b < CIDX_BINS; b++)
        if (global_bin_counts[b] > 0)
            bins_used++;

    fprintf(stderr, "bins used:          %d / %d\n", bins_used, CIDX_BINS);

    /* show top 5 most popular bins */
    fprintf(stderr, "top bins by # contexts:\n");
    for (int rank = 0; rank < 5 && rank < bins_used; rank++) {
        int best = -1;
        int best_count = -1;
        for (int b = 0; b < CIDX_BINS; b++) {
            if (global_bin_counts[b] > best_count) {
                best_count = global_bin_counts[b];
                best = b;
            }
        }
        if (best < 0) break;

        coord4 bc = bin_to_coord(best);
        fprintf(stderr, "  bin %2d (%+d,%+d,%+d,%+d): %d contexts\n",
                best, bc.t, bc.d, bc.m, bc.q, best_count);
        global_bin_counts[best] = -1;  /* mark as reported */
    }

    fprintf(stderr, "========================\n\n");
}
