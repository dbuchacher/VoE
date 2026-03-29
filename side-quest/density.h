#ifndef DENSITY_H
#define DENSITY_H

#include <stdint.h>
#include <stdlib.h>
#include "distance.h"  /* for coord4_manhattan */
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "wave.h"

/*
 * density.h -- 4D density map over the born-index coordinate space
 *
 * Component 1.5: sits between the raw index (index.c) and prediction.
 *
 * WHY THIS REPLACES TRAINING
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~
 * A neural network learns a function f(context) -> distribution by adjusting
 * millions of weights over many gradient descent steps. The weights encode
 * statistical regularities in the training data -- but indirectly, through
 * a lossy optimization process.
 *
 * The density map encodes those same regularities directly:
 *   - Scan every entry in the born-index (one pass, no gradients)
 *   - For each entry's 4D coordinate, increment the corresponding bin
 *   - The resulting histogram IS the probability distribution
 *
 * This works because the coordinate space has structure by construction.
 * Phonetically similar sounds have adjacent coordinates (lattice.h).
 * So the density map is automatically smooth -- nearby bins have similar
 * counts -- without any learned smoothing kernel.
 *
 * THE "DARK MATTER" PREDICTION
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Neural networks only learn from queried training examples. Unqueried data
 * has zero effect on the weights.
 *
 * In the born-index, EVERY indexed entry contributes to the density map,
 * whether or not it was ever queried. An entry that was indexed but never
 * looked up still increments its bin, still affects the probability of
 * nearby coordinates. The unqueried entries are "dark matter" -- invisible
 * to a query-based model, but their gravitational effect (statistical mass)
 * shapes the density landscape.
 *
 * This is the formalization: the density map captures what neural nets lose.
 *
 * SIZE COMPARISON
 * ~~~~~~~~~~~~~~~
 * model.h Weights struct: ~4.3 MB (INPUT_DIM * HIDDEN_DIM * 4 bytes, etc.)
 * Shell 1 density:        324 bytes  (81 bins * 4 bytes)
 * Shell 2 density:        2,500 bytes (625 bins * 4 bytes)
 * Total density map:      2,824 bytes
 *
 * Shell 1 fits in ONE cache line (64 bytes covers 16 bins; 2 cache lines
 * cover the whole thing on most architectures). The entire "model" lives
 * in L1 cache. No memory bandwidth bottleneck. No GPU required.
 */

/* ── Shell dimensions ───────────────────────────────────────────── */

#define DENSITY_S1_BINS  81     /* 3^4: each dim in {-1, 0, +1}  */
#define DENSITY_S2_BINS  625    /* 5^4: each dim in {-2,-1,0,+1,+2} */

/* ── DensityMap ─────────────────────────────────────────────────── */

/*
 * Two concentric shells of resolution.
 *
 * Shell 1 (coarse): 81 bins. Every coordinate in the born-index, clamped
 * to {-1,0,+1} per dimension, maps to exactly one bin. This gives the
 * broad shape of the distribution -- where is the mass?
 *
 * Shell 2 (fine): 625 bins. Same coordinates clamped to {-2..+2}.
 * This resolves the tails -- which extended coordinates actually get used?
 *
 * Both shells are populated from the same single scan. No hierarchy,
 * no multi-resolution "training". Just two binning granularities.
 *
 * counts_s1/s2: raw hit counts (how many index entries map to each bin)
 * probs_s1/s2:  normalized probabilities (after density_normalize)
 * total:        total entries added (for normalization denominator)
 */
typedef struct {
    uint32_t counts_s1[DENSITY_S1_BINS];
    uint32_t counts_s2[DENSITY_S2_BINS];
    float    probs_s1[DENSITY_S1_BINS];
    float    probs_s2[DENSITY_S2_BINS];
    uint64_t total;     /* total entries added across all bins */
} DensityMap;

/* ── Coordinate-to-bin mapping ──────────────────────────────────── */

/*
 * Clamp a value to a range and return it shifted to 0-based.
 * For shell 1: clamp to [-1,+1], return val+1 (range 0..2)
 * For shell 2: clamp to [-2,+2], return val+2 (range 0..4)
 */
static inline int clamp_i8(int8_t v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/*
 * coord4 -> shell 1 bin index (0..80)
 *
 * Each dimension clamped to {-1, 0, +1}, then packed in base 3:
 *   bin = (t+1)*27 + (d+1)*9 + (m+1)*3 + (q+1)
 *
 * This is the same formula as coord_to_index() in model.h, but with
 * explicit clamping so coordinates outside shell 1 still map somewhere.
 * A coordinate at (0, +3, -1, +1) clamps to (0, +1, -1, +1) and lands
 * in the same bin as its shell-1 neighbor. The density captures this:
 * extended coordinates add mass to the nearest shell-1 bin, making
 * shell-1 predictions aware of shell-2 population without needing
 * shell-2 resolution.
 */
static inline int coord4_to_bin_s1(coord4 c) {
    int t = clamp_i8(c.t, -1, 1) + 1;  /* 0..2 */
    int d = clamp_i8(c.d, -1, 1) + 1;
    int m = clamp_i8(c.m, -1, 1) + 1;
    int q = clamp_i8(c.q, -1, 1) + 1;
    return t * 27 + d * 9 + m * 3 + q;
}

/*
 * coord4 -> shell 2 bin index (0..624)
 *
 * Each dimension clamped to {-2, -1, 0, +1, +2}, packed in base 5:
 *   bin = (t+2)*125 + (d+2)*25 + (m+2)*5 + (q+2)
 */
static inline int coord4_to_bin_s2(coord4 c) {
    int t = clamp_i8(c.t, -2, 2) + 2;  /* 0..4 */
    int d = clamp_i8(c.d, -2, 2) + 2;
    int m = clamp_i8(c.m, -2, 2) + 2;
    int q = clamp_i8(c.q, -2, 2) + 2;
    return t * 125 + d * 25 + m * 5 + q;
}

/*
 * Reverse: shell 1 bin index (0..80) -> coord4
 * Unpacks base-3 digits back to {-1, 0, +1} per dimension.
 */
static inline coord4 bin_s1_to_coord(int bin) {
    coord4 c;
    c.q = (int8_t)((bin % 3) - 1); bin /= 3;
    c.m = (int8_t)((bin % 3) - 1); bin /= 3;
    c.d = (int8_t)((bin % 3) - 1); bin /= 3;
    c.t = (int8_t)((bin % 3) - 1);
    return c;
}

/*
 * Reverse: shell 2 bin index (0..624) -> coord4
 * Unpacks base-5 digits back to {-2, -1, 0, +1, +2} per dimension.
 */
static inline coord4 bin_s2_to_coord(int bin) {
    coord4 c;
    c.q = (int8_t)((bin % 5) - 2); bin /= 5;
    c.m = (int8_t)((bin % 5) - 2); bin /= 5;
    c.d = (int8_t)((bin % 5) - 2); bin /= 5;
    c.t = (int8_t)((bin % 5) - 2);
    return c;
}

/* ── Create / destroy ───────────────────────────────────────────── */

static inline DensityMap *density_create(void) {
    DensityMap *dm = (DensityMap *)calloc(1, sizeof(DensityMap));
    return dm;
}

static inline void density_free(DensityMap *dm) {
    free(dm);
}

/* ── Add entry ──────────────────────────────────────────────────── */

/*
 * Increment the density at a coordinate.
 * Called once per born-index entry during the single-pass scan.
 *
 * Both shells are updated simultaneously: the same coordinate
 * maps to one bin in shell 1 (coarse) and one bin in shell 2 (fine).
 * No data is lost, no data is duplicated -- just two views of the
 * same mass at different resolutions.
 */
static inline void density_add(DensityMap *dm, coord4 word_coord) {
    int b1 = coord4_to_bin_s1(word_coord);
    int b2 = coord4_to_bin_s2(word_coord);

    dm->counts_s1[b1]++;
    dm->counts_s2[b2]++;
    dm->total++;
}

/* ── Normalize ──────────────────────────────────────────────────── */

/*
 * Convert raw counts to probability distributions.
 * Call once after all entries have been added.
 *
 * After this, probs_s1[i] = counts_s1[i] / total for all i,
 * and likewise for shell 2. The probabilities sum to 1.0 within
 * each shell (independently).
 *
 * This IS the "training" step. One division per bin. No epochs,
 * no learning rate, no loss function. The distribution is exact.
 */
static inline void density_normalize(DensityMap *dm) {
    if (dm->total == 0) return;

    float inv_total = 1.0f / (float)dm->total;

    for (int i = 0; i < DENSITY_S1_BINS; i++)
        dm->probs_s1[i] = (float)dm->counts_s1[i] * inv_total;

    for (int i = 0; i < DENSITY_S2_BINS; i++)
        dm->probs_s2[i] = (float)dm->counts_s2[i] * inv_total;
}

/* ── Query probability ──────────────────────────────────────────── */

/*
 * Return the probability mass at a query coordinate.
 *
 * Uses shell 2 if the coordinate has any component outside {-1,0,+1},
 * otherwise uses shell 1 (higher statistical confidence from fewer bins).
 *
 * This is the core prediction primitive. "What is the probability that
 * the next wave byte has this coordinate?" The answer comes from counting,
 * not from a learned function approximation.
 */
static inline float density_probability(const DensityMap *dm, coord4 query) {
    /* check if query fits in shell 1 */
    if (query.t >= -1 && query.t <= 1 &&
        query.d >= -1 && query.d <= 1 &&
        query.m >= -1 && query.m <= 1 &&
        query.q >= -1 && query.q <= 1) {
        return dm->probs_s1[coord4_to_bin_s1(query)];
    }
    return dm->probs_s2[coord4_to_bin_s2(query)];
}

/* ── Nearest dense bins ─────────────────────────────────────────── */

/*
 * Manhattan distance — use coord4_manhattan from primes_lookup.h
 * (canonical location for coordinate math utilities).
 */

/*
 * Find the k bins with highest density near the query coordinate.
 *
 * "Near" means: sort all non-empty bins by a score that combines
 * proximity (low distance) with density (high probability).
 *
 *   score = probability / (1 + distance)
 *
 * A bin right on top of the query with probability 0.1 scores 0.1.
 * A bin 2 steps away with probability 0.3 scores 0.1.
 * This naturally balances "what's likely" with "what's close" --
 * the geometric analog of attention.
 *
 * Operates on shell 1 (81 bins). Shell 2 refinement is a separate step.
 * Returns actual number of results written (may be < k if fewer
 * non-empty bins exist).
 */
static inline int density_nearest(const DensityMap *dm, coord4 query,
                                  int k, coord4 *results) {
    /*
     * 81 bins is small enough to score exhaustively.
     * No spatial index needed. This is the advantage of a tiny model:
     * brute force IS the optimal algorithm when N fits in cache.
     */

    /* score every shell-1 bin */
    float scores[DENSITY_S1_BINS];
    for (int i = 0; i < DENSITY_S1_BINS; i++) {
        if (dm->counts_s1[i] == 0) {
            scores[i] = -1.0f;  /* empty bin: exclude */
            continue;
        }
        coord4 bin_coord = bin_s1_to_coord(i);
        int dist = coord4_manhattan(query, bin_coord);
        scores[i] = dm->probs_s1[i] / (1.0f + (float)dist);
    }

    /*
     * Selection: find top-k by repeated max scan.
     * For k << 81, this is faster than sorting. And k is typically
     * small (1-5 for prediction, maybe 10 for exploration).
     */
    int found = 0;
    for (int pick = 0; pick < k && pick < DENSITY_S1_BINS; pick++) {
        int best = -1;
        float best_score = -1.0f;
        for (int i = 0; i < DENSITY_S1_BINS; i++) {
            if (scores[i] > best_score) {
                best_score = scores[i];
                best = i;
            }
        }
        if (best < 0 || best_score <= 0.0f) break;  /* no more non-empty bins */

        results[found++] = bin_s1_to_coord(best);
        scores[best] = -1.0f;  /* remove from future picks */
    }

    return found;
}

/* ── Save / Load ────────────────────────────────────────────────── */

/*
 * File format:
 *   4 bytes: magic "DENS" (0x534E4544)
 *   8 bytes: total entry count (uint64_t)
 *   81 * 4 bytes: shell 1 counts
 *   625 * 4 bytes: shell 2 counts
 *
 * Total file size: 4 + 8 + 324 + 2500 = 2,836 bytes.
 * The entire model serializes in under 3 KB.
 *
 * Probabilities are NOT saved -- they are recomputed on load by
 * density_normalize(). This keeps the file format simple and means
 * you can merge density maps by summing counts and re-normalizing.
 */

#define DENSITY_MAGIC 0x534E4544  /* "DENS" in little-endian */

static inline int density_save(const DensityMap *dm, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }

    uint32_t magic = DENSITY_MAGIC;
    fwrite(&magic, 4, 1, f);
    fwrite(&dm->total, 8, 1, f);
    fwrite(dm->counts_s1, sizeof(uint32_t), DENSITY_S1_BINS, f);
    fwrite(dm->counts_s2, sizeof(uint32_t), DENSITY_S2_BINS, f);

    fclose(f);
    return 0;
}

static inline DensityMap *density_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }

    uint32_t magic;
    if (fread(&magic, 4, 1, f) != 1 || magic != DENSITY_MAGIC) {
        fprintf(stderr, "density_load: bad magic in %s\n", path);
        fclose(f);
        return NULL;
    }

    DensityMap *dm = density_create();
    if (!dm) { fclose(f); return NULL; }

    if (fread(&dm->total, 8, 1, f) != 1) goto fail;
    if (fread(dm->counts_s1, sizeof(uint32_t), DENSITY_S1_BINS, f)
        != DENSITY_S1_BINS) goto fail;
    if (fread(dm->counts_s2, sizeof(uint32_t), DENSITY_S2_BINS, f)
        != DENSITY_S2_BINS) goto fail;

    fclose(f);

    /* recompute probabilities from loaded counts */
    density_normalize(dm);
    return dm;

fail:
    fprintf(stderr, "density_load: truncated file %s\n", path);
    density_free(dm);
    fclose(f);
    return NULL;
}

/* ── Stats ──────────────────────────────────────────────────────── */

/*
 * Print distribution statistics to stderr.
 *
 * Shows: total entries, occupied bins, sparsity, top bins by count,
 * and the entropy of the distribution (bits). Low entropy = peaky
 * distribution = strong prediction signal. High entropy = flat =
 * the density map has little to say.
 */
static inline void density_print_stats(const DensityMap *dm) {
    fprintf(stderr, "=== density map ===\n");
    fprintf(stderr, "total entries:  %lu\n", (unsigned long)dm->total);

    /* shell 1 stats */
    int s1_occupied = 0;
    uint32_t s1_max = 0;
    int s1_max_bin = 0;
    for (int i = 0; i < DENSITY_S1_BINS; i++) {
        if (dm->counts_s1[i] > 0) s1_occupied++;
        if (dm->counts_s1[i] > s1_max) {
            s1_max = dm->counts_s1[i];
            s1_max_bin = i;
        }
    }

    fprintf(stderr, "\nshell 1: %d / %d bins occupied (%.0f%% sparse)\n",
            s1_occupied, DENSITY_S1_BINS,
            100.0f * (1.0f - (float)s1_occupied / DENSITY_S1_BINS));

    /* entropy of shell 1 */
    float entropy_s1 = 0.0f;
    for (int i = 0; i < DENSITY_S1_BINS; i++) {
        if (dm->probs_s1[i] > 0.0f)
            entropy_s1 -= dm->probs_s1[i] * log2f(dm->probs_s1[i]);
    }
    fprintf(stderr, "shell 1 entropy: %.2f bits (max %.2f = log2(%d))\n",
            entropy_s1, log2f(DENSITY_S1_BINS), DENSITY_S1_BINS);

    /* top 5 shell 1 bins */
    fprintf(stderr, "shell 1 top bins:\n");
    float scores[DENSITY_S1_BINS];
    for (int i = 0; i < DENSITY_S1_BINS; i++)
        scores[i] = dm->probs_s1[i];

    for (int pick = 0; pick < 5 && pick < s1_occupied; pick++) {
        int best = 0;
        for (int i = 1; i < DENSITY_S1_BINS; i++)
            if (scores[i] > scores[best]) best = i;

        coord4 c = bin_s1_to_coord(best);
        fprintf(stderr, "  bin %2d  (%+d,%+d,%+d,%+d)  count=%u  prob=%.4f\n",
                best, c.t, c.d, c.m, c.q,
                dm->counts_s1[best], dm->probs_s1[best]);
        scores[best] = -1.0f;
    }

    /* shell 2 stats */
    int s2_occupied = 0;
    for (int i = 0; i < DENSITY_S2_BINS; i++)
        if (dm->counts_s2[i] > 0) s2_occupied++;

    float entropy_s2 = 0.0f;
    for (int i = 0; i < DENSITY_S2_BINS; i++) {
        if (dm->probs_s2[i] > 0.0f)
            entropy_s2 -= dm->probs_s2[i] * log2f(dm->probs_s2[i]);
    }

    fprintf(stderr, "\nshell 2: %d / %d bins occupied (%.0f%% sparse)\n",
            s2_occupied, DENSITY_S2_BINS,
            100.0f * (1.0f - (float)s2_occupied / DENSITY_S2_BINS));
    fprintf(stderr, "shell 2 entropy: %.2f bits (max %.2f = log2(%d))\n",
            entropy_s2, log2f(DENSITY_S2_BINS), DENSITY_S2_BINS);

    /* memory footprint */
    fprintf(stderr, "\nmemory: %lu bytes counts + %lu bytes probs = %lu bytes total\n",
            (unsigned long)(sizeof(dm->counts_s1) + sizeof(dm->counts_s2)),
            (unsigned long)(sizeof(dm->probs_s1) + sizeof(dm->probs_s2)),
            (unsigned long)sizeof(DensityMap));
    fprintf(stderr, "file size: %lu bytes\n",
            (unsigned long)(4 + 8 +
                DENSITY_S1_BINS * sizeof(uint32_t) +
                DENSITY_S2_BINS * sizeof(uint32_t)));

    (void)s1_max_bin; /* used implicitly in top-5 loop */
    (void)s1_max;
}

#endif /* DENSITY_H */
