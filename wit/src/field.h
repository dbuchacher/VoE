#ifndef FIELD_H
#define FIELD_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "wave.h"
#include "distance.h"

/*
 * field.h — the model
 *
 * All accumulated state in one structure.  Three components at two
 * resolutions.  This is what a neural network's 140GB of weights
 * compress into when the coordinate space has structure by construction.
 *
 * Hot path (shell 1, L1-resident):
 *
 *   Density map:  81 shell-1 bins + 625 shell-2 bins.
 *     How many words were observed at each coordinate region.
 *     The "what's common here" field.
 *
 *   Transition map:  81x81 = 6561 entries.
 *     Which coordinate region follows which.
 *     The "what comes next" field.  Coarse direction signal.
 *     Shell 1 is sufficient — "a verb follows a noun," not
 *     "this exact verb follows this exact noun."
 *
 * Both use log-counts: stored value = log(1 + raw_count).
 * "the" at 5M and "serendipity" at 50 are 5 orders of magnitude
 * apart in raw counts but ~10:1 in log-space.  Standard IR practice
 * (TF-IDF uses log-frequency for exactly this reason).
 *
 * Cold path (shell 2, L3-resident):
 *
 *   Reverse index:  625 bins (shell 2, 5^4).
 *     Each bin holds a dynamic array of word entries.
 *     Coordinate-indexed, not word-indexed.
 *     The word is stored WHERE it was observed.
 *     No cross-bin deduplication.  Per-bin dedup only.
 *
 * Memory:
 *   density:     81*8 + 625*8 + 8 = ~6 KB
 *   transitions: 6561*8 + 8       = ~52 KB
 *   reverse:     625 bins * ~160 entries avg = dynamic
 *   Total hot path: ~58 KB (fits in L1/L2 easily)
 *
 * File format: magic "WFLD", version 1, little-endian.
 */


/* ── Shell dimensions ───────────────────────────────────────────── */

#define FIELD_S1_BINS   81      /* 3^4: each dim in {-1, 0, +1}    */
#define FIELD_S2_BINS   625     /* 5^4: each dim in {-2..+2}       */
#define FIELD_TRANS_BINS (FIELD_S1_BINS * FIELD_S1_BINS)  /* 6561   */


/* ── Coordinate-to-bin mapping ──────────────────────────────────── */

static inline int field_clamp(int8_t v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/*
 * coord4 -> shell 1 bin index (0..80)
 * Each dimension clamped to {-1, 0, +1}, packed base 3:
 *   bin = (t+1)*27 + (x+1)*9 + (m+1)*3 + (q+1)
 */
static inline int coord4_to_bin_s1(coord4 c) {
    int t = field_clamp(c.t, -1, 1) + 1;
    int x = field_clamp(c.x, -1, 1) + 1;
    int m = field_clamp(c.m, -1, 1) + 1;
    int q = field_clamp(c.q, -1, 1) + 1;
    return t * 27 + x * 9 + m * 3 + q;
}

/*
 * coord4 -> shell 2 bin index (0..624)
 * Each dimension clamped to {-2..+2}, packed base 5:
 *   bin = (t+2)*125 + (x+2)*25 + (m+2)*5 + (q+2)
 */
static inline int coord4_to_bin_s2(coord4 c) {
    int t = field_clamp(c.t, -2, 2) + 2;
    int x = field_clamp(c.x, -2, 2) + 2;
    int m = field_clamp(c.m, -2, 2) + 2;
    int q = field_clamp(c.q, -2, 2) + 2;
    return t * 125 + x * 25 + m * 5 + q;
}

/* Reverse: shell 1 bin -> coord4 */
static inline coord4 bin_s1_to_coord4(int bin) {
    coord4 c;
    c.q = (int8_t)((bin % 3) - 1); bin /= 3;
    c.m = (int8_t)((bin % 3) - 1); bin /= 3;
    c.x = (int8_t)((bin % 3) - 1); bin /= 3;
    c.t = (int8_t)((bin % 3) - 1);
    return c;
}

/* Reverse: shell 2 bin -> coord4 */
static inline coord4 bin_s2_to_coord4(int bin) {
    coord4 c;
    c.q = (int8_t)((bin % 5) - 2); bin /= 5;
    c.m = (int8_t)((bin % 5) - 2); bin /= 5;
    c.x = (int8_t)((bin % 5) - 2); bin /= 5;
    c.t = (int8_t)((bin % 5) - 2);
    return c;
}


/* ── Reverse index types ────────────────────────────────────────── */

typedef struct {
    char     word[32];
    coord4   full_coord;    /* exact coordinate, not just bin */
    uint32_t count;         /* times seen at this bin */
} ReverseEntry;

typedef struct {
    ReverseEntry *entries;
    int count;
    int capacity;
} ReverseBin;


/* ── The Field ──────────────────────────────────────────────────── */

typedef struct {
    /* shell 1, L1-resident */
    uint32_t density_s1[FIELD_S1_BINS];
    float    density_s1_log[FIELD_S1_BINS];     /* log(1 + count) */
    uint32_t density_s2[FIELD_S2_BINS];
    float    density_s2_log[FIELD_S2_BINS];
    uint64_t density_total;

    uint32_t trans[FIELD_TRANS_BINS];            /* transition counts */
    float    trans_log[FIELD_TRANS_BINS];        /* log(1 + count) */
    uint64_t trans_total;

    /* shell 2, L3-resident */
    ReverseBin reverse[FIELD_S2_BINS];
} Field;


/* ── Health diagnostics ─────────────────────────────────────────── */

typedef struct {
    double shell1_entropy;      /* over 81 bins, target > 4.5 */
    double shell2_entropy;      /* over 625 bins, target > 6.0 */
    double max_shell1_share;    /* largest shell-1 bin, target < 0.15 */
    double max_shell2_share;    /* largest shell-2 bin, target < 0.03 */
} FieldHealth;


/* ── Reverse lookup result ──────────────────────────────────────── */

typedef struct {
    const char *word;
    float score;
} ReverseCandidate;


/* ── API ────────────────────────────────────────────────────────── */

Field      *field_create(void);
void        field_free(Field *f);

void        field_observe(Field *f, coord4 from, coord4 to,
                          const char *word, int len, uint8_t confidence);

int         field_save(const Field *f, const char *path);
Field      *field_load(const char *path);

FieldHealth field_health(const Field *f);
void        field_print_health(const Field *f);

int         field_reverse_lookup(const Field *f, coord4 target, coord4 prev,
                                 ReverseCandidate *results, int max_results);


/* ── Implementation ─────────────────────────────────────────────── */

#ifdef FIELD_IMPLEMENTATION


/* ── confidence -> weight ───────────────────────────────────────── */

static float conf_weight(uint8_t confidence) {
    if (confidence == 0) return 0.25f;
    if (confidence == 1) return 0.5f;
    return 1.0f;  /* 2+ */
}


/* ── create / free ──────────────────────────────────────────────── */

Field *field_create(void) {
    Field *f = (Field *)calloc(1, sizeof(Field));
    return f;
}

void field_free(Field *f) {
    if (!f) return;
    for (int i = 0; i < FIELD_S2_BINS; i++)
        free(f->reverse[i].entries);
    free(f);
}


/* ── observe ────────────────────────────────────────────────────── */

/*
 * Record one observation: word appeared at coordinate 'to',
 * preceded by coordinate 'from'.
 *
 * Updates density (both shells), transition map, and reverse index.
 * Confidence affects density/transition weight but every word gets
 * born-indexed in reverse regardless.
 */
void field_observe(Field *f, coord4 from, coord4 to,
                   const char *word, int len, uint8_t confidence)
{
    float w = conf_weight(confidence);

    /* density — shell 1 */
    int b1 = coord4_to_bin_s1(to);
    f->density_s1[b1] += (uint32_t)(w + 0.5f);  /* round to nearest */
    f->density_s1_log[b1] = logf(1.0f + (float)f->density_s1[b1]);

    /* density — shell 2 */
    int b2 = coord4_to_bin_s2(to);
    f->density_s2[b2] += (uint32_t)(w + 0.5f);
    f->density_s2_log[b2] = logf(1.0f + (float)f->density_s2[b2]);

    f->density_total++;

    /* transition map — shell 1 only */
    int from_b1 = coord4_to_bin_s1(from);
    int tidx = from_b1 * FIELD_S1_BINS + b1;
    f->trans[tidx] += (uint32_t)(w + 0.5f);
    f->trans_log[tidx] = logf(1.0f + (float)f->trans[tidx]);
    f->trans_total++;

    /* reverse index — shell 2 bin, per-bin dedup */
    ReverseBin *bin = &f->reverse[b2];

    /* truncate word to fit */
    int wlen = len;
    if (wlen > 31) wlen = 31;

    /* search for existing entry (linear scan, ~160 avg) */
    for (int i = 0; i < bin->count; i++) {
        if (strncmp(bin->entries[i].word, word, wlen) == 0 &&
            bin->entries[i].word[wlen] == '\0') {
            bin->entries[i].count++;
            return;
        }
    }

    /* new entry — grow if needed */
    if (bin->count >= bin->capacity) {
        int newcap = bin->capacity ? bin->capacity * 2 : 16;
        ReverseEntry *newbuf = (ReverseEntry *)realloc(
            bin->entries, newcap * sizeof(ReverseEntry));
        if (!newbuf) return;  /* OOM: silently drop */
        bin->entries = newbuf;
        bin->capacity = newcap;
    }

    ReverseEntry *e = &bin->entries[bin->count++];
    memset(e, 0, sizeof(*e));
    memcpy(e->word, word, wlen);
    e->word[wlen] = '\0';
    e->full_coord = to;
    e->count = 1;
}


/* ── health diagnostics ─────────────────────────────────────────── */

FieldHealth field_health(const Field *f) {
    FieldHealth h = {0};

    if (f->density_total == 0) return h;

    double inv = 1.0 / (double)f->density_total;

    /* shell 1 entropy + max share */
    double max1 = 0.0;
    for (int i = 0; i < FIELD_S1_BINS; i++) {
        double p = (double)f->density_s1[i] * inv;
        if (p > max1) max1 = p;
        if (p > 0.0)
            h.shell1_entropy -= p * log2(p);
    }
    h.max_shell1_share = max1;

    /* shell 2 entropy + max share */
    double max2 = 0.0;
    for (int i = 0; i < FIELD_S2_BINS; i++) {
        double p = (double)f->density_s2[i] * inv;
        if (p > max2) max2 = p;
        if (p > 0.0)
            h.shell2_entropy -= p * log2(p);
    }
    h.max_shell2_share = max2;

    return h;
}

void field_print_health(const Field *f) {
    FieldHealth h = field_health(f);

    fprintf(stderr, "=== field health ===\n");
    fprintf(stderr, "density total:      %lu\n", (unsigned long)f->density_total);
    fprintf(stderr, "transition total:   %lu\n", (unsigned long)f->trans_total);

    /* shell 1 */
    int s1_occ = 0;
    for (int i = 0; i < FIELD_S1_BINS; i++)
        if (f->density_s1[i] > 0) s1_occ++;
    fprintf(stderr, "\nshell 1: %d / %d bins occupied (%.0f%% sparse)\n",
            s1_occ, FIELD_S1_BINS,
            100.0 * (1.0 - (double)s1_occ / FIELD_S1_BINS));
    fprintf(stderr, "  entropy:   %.2f bits (max %.2f)\n",
            h.shell1_entropy, log2(FIELD_S1_BINS));
    fprintf(stderr, "  max share: %.4f %s\n",
            h.max_shell1_share,
            h.max_shell1_share < 0.15 ? "(ok)" : "(WARNING > 15%)");

    /* shell 2 */
    int s2_occ = 0;
    for (int i = 0; i < FIELD_S2_BINS; i++)
        if (f->density_s2[i] > 0) s2_occ++;
    fprintf(stderr, "\nshell 2: %d / %d bins occupied (%.0f%% sparse)\n",
            s2_occ, FIELD_S2_BINS,
            100.0 * (1.0 - (double)s2_occ / FIELD_S2_BINS));
    fprintf(stderr, "  entropy:   %.2f bits (max %.2f)\n",
            h.shell2_entropy, log2(FIELD_S2_BINS));
    fprintf(stderr, "  max share: %.4f %s\n",
            h.max_shell2_share,
            h.max_shell2_share < 0.03 ? "(ok)" : "(WARNING > 3%)");

    /* reverse index stats */
    int total_entries = 0;
    int max_bin_entries = 0;
    int nonempty_bins = 0;
    for (int i = 0; i < FIELD_S2_BINS; i++) {
        int n = f->reverse[i].count;
        total_entries += n;
        if (n > max_bin_entries) max_bin_entries = n;
        if (n > 0) nonempty_bins++;
    }
    fprintf(stderr, "\nreverse index: %d entries across %d / %d bins\n",
            total_entries, nonempty_bins, FIELD_S2_BINS);
    if (nonempty_bins > 0)
        fprintf(stderr, "  avg %.1f entries/bin, max %d\n",
                (double)total_entries / nonempty_bins, max_bin_entries);
}


/* ── reverse lookup pipeline ────────────────────────────────────── */

/*
 * 6-step pipeline:
 *   1. Map target to shell-2 bin
 *   2. Get entries in that bin
 *   3. Rank by L2 distance from target to entry's full_coord
 *   4. Weight by transition compatibility from prev's shell-1 bin
 *   5. Combine: score = proximity * transition
 *   6. Return top-N sorted by score
 */
int field_reverse_lookup(const Field *f, coord4 target, coord4 prev,
                         ReverseCandidate *results, int max_results)
{
    /* 1. target -> shell-2 bin */
    int b2 = coord4_to_bin_s2(target);
    const ReverseBin *bin = &f->reverse[b2];

    if (bin->count == 0 || max_results <= 0)
        return 0;

    int prev_s1 = coord4_to_bin_s1(prev);

    /* 3-5: score every entry in the bin */
    int n = bin->count;

    /* temp scoring arrays — stack-allocate up to a limit, heap above */
    #define FIELD_STACK_LIMIT 512
    float  stack_scores[FIELD_STACK_LIMIT];
    int    stack_indices[FIELD_STACK_LIMIT];
    float *scores;
    int   *indices;

    if (n <= FIELD_STACK_LIMIT) {
        scores  = stack_scores;
        indices = stack_indices;
    } else {
        scores  = (float *)malloc(n * sizeof(float));
        indices = (int *)malloc(n * sizeof(int));
        if (!scores || !indices) {
            free(scores); free(indices);
            return 0;
        }
    }

    for (int i = 0; i < n; i++) {
        const ReverseEntry *e = &bin->entries[i];

        /* proximity: inverse L2 distance */
        float dist = coord4_distance(target, e->full_coord);
        float proximity = 1.0f / (1.0f + dist);

        /* transition compatibility */
        int entry_s1 = coord4_to_bin_s1(e->full_coord);
        float trans_score = f->trans_log[prev_s1 * FIELD_S1_BINS + entry_s1];
        /* floor at 0.1 so untransitioned pairs aren't completely dead */
        if (trans_score < 0.1f) trans_score = 0.1f;

        scores[i]  = proximity * trans_score;
        indices[i] = i;
    }

    /* 6. top-N selection by repeated max scan */
    int found = 0;
    for (int pick = 0; pick < max_results && pick < n; pick++) {
        int best = -1;
        float best_score = -1.0f;
        for (int i = 0; i < n; i++) {
            if (scores[i] > best_score) {
                best_score = scores[i];
                best = i;
            }
        }
        if (best < 0 || best_score <= 0.0f) break;

        results[found].word  = bin->entries[indices[best]].word;
        results[found].score = best_score;
        found++;
        scores[best] = -1.0f;  /* exclude from future picks */
    }

    if (n > FIELD_STACK_LIMIT) {
        free(scores);
        free(indices);
    }
    #undef FIELD_STACK_LIMIT

    return found;
}


/* ── save / load ────────────────────────────────────────────────── */

#define FIELD_MAGIC   0x444C4657  /* "WFLD" little-endian */
#define FIELD_VERSION 1

int field_save(const Field *f, const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp) { perror(path); return -1; }

    /* header */
    uint32_t magic   = FIELD_MAGIC;
    uint32_t version = FIELD_VERSION;
    fwrite(&magic,   4, 1, fp);
    fwrite(&version, 4, 1, fp);

    /* density */
    fwrite(&f->density_total, 8, 1, fp);
    fwrite(f->density_s1, sizeof(uint32_t), FIELD_S1_BINS, fp);
    fwrite(f->density_s2, sizeof(uint32_t), FIELD_S2_BINS, fp);

    /* transitions */
    fwrite(&f->trans_total, 8, 1, fp);
    fwrite(f->trans, sizeof(uint32_t), FIELD_TRANS_BINS, fp);

    /* reverse index: per-bin count, then entries */
    for (int i = 0; i < FIELD_S2_BINS; i++) {
        const ReverseBin *bin = &f->reverse[i];
        uint32_t cnt = (uint32_t)bin->count;
        fwrite(&cnt, 4, 1, fp);
        for (int j = 0; j < bin->count; j++) {
            const ReverseEntry *e = &bin->entries[j];
            fwrite(e->word, 1, 32, fp);
            fwrite(&e->full_coord.t, 1, 1, fp);
            fwrite(&e->full_coord.x, 1, 1, fp);
            fwrite(&e->full_coord.m, 1, 1, fp);
            fwrite(&e->full_coord.q, 1, 1, fp);
            fwrite(&e->count, 4, 1, fp);
        }
    }

    fclose(fp);
    return 0;
}

Field *field_load(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) { perror(path); return NULL; }

    /* header */
    uint32_t magic, version;
    if (fread(&magic,   4, 1, fp) != 1 || magic != FIELD_MAGIC) {
        fprintf(stderr, "field_load: bad magic in %s\n", path);
        fclose(fp);
        return NULL;
    }
    if (fread(&version, 4, 1, fp) != 1 || version != FIELD_VERSION) {
        fprintf(stderr, "field_load: unsupported version %u in %s\n",
                version, path);
        fclose(fp);
        return NULL;
    }

    Field *f = field_create();
    if (!f) { fclose(fp); return NULL; }

    /* density */
    if (fread(&f->density_total, 8, 1, fp) != 1) goto fail;
    if (fread(f->density_s1, sizeof(uint32_t), FIELD_S1_BINS, fp)
        != FIELD_S1_BINS) goto fail;
    if (fread(f->density_s2, sizeof(uint32_t), FIELD_S2_BINS, fp)
        != FIELD_S2_BINS) goto fail;

    /* recompute log versions */
    for (int i = 0; i < FIELD_S1_BINS; i++)
        f->density_s1_log[i] = logf(1.0f + (float)f->density_s1[i]);
    for (int i = 0; i < FIELD_S2_BINS; i++)
        f->density_s2_log[i] = logf(1.0f + (float)f->density_s2[i]);

    /* transitions */
    if (fread(&f->trans_total, 8, 1, fp) != 1) goto fail;
    if (fread(f->trans, sizeof(uint32_t), FIELD_TRANS_BINS, fp)
        != FIELD_TRANS_BINS) goto fail;

    /* recompute log versions */
    for (int i = 0; i < FIELD_TRANS_BINS; i++)
        f->trans_log[i] = logf(1.0f + (float)f->trans[i]);

    /* reverse index */
    for (int i = 0; i < FIELD_S2_BINS; i++) {
        uint32_t cnt;
        if (fread(&cnt, 4, 1, fp) != 1) goto fail;

        ReverseBin *bin = &f->reverse[i];
        if (cnt > 0) {
            bin->entries = (ReverseEntry *)malloc(cnt * sizeof(ReverseEntry));
            if (!bin->entries) goto fail;
            bin->capacity = (int)cnt;
            bin->count    = (int)cnt;

            for (int j = 0; j < (int)cnt; j++) {
                ReverseEntry *e = &bin->entries[j];
                if (fread(e->word, 1, 32, fp) != 32) goto fail;
                int8_t coords[4];
                if (fread(coords, 1, 4, fp) != 4) goto fail;
                e->full_coord.t = coords[0];
                e->full_coord.x = coords[1];
                e->full_coord.m = coords[2];
                e->full_coord.q = coords[3];
                if (fread(&e->count, 4, 1, fp) != 1) goto fail;
            }
        }
    }

    fclose(fp);
    return f;

fail:
    fprintf(stderr, "field_load: truncated file %s\n", path);
    field_free(f);
    fclose(fp);
    return NULL;
}


#endif /* FIELD_IMPLEMENTATION */
#endif /* FIELD_H */
