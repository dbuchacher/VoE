/*
 * lattice_chat.c -- interactive lattice-native intelligence chat
 *
 * The end-to-end pipeline:
 *   Input -> tokenize -> word coordinate lookup -> build walk ->
 *   attention (4D heads + RC decay) -> density map query ->
 *   bond-typed prediction -> generate next word -> output
 *
 * No neural network. No training. No weights.
 * Word coordinates come from primes_lookup.h (333 words).
 * Density map comes from scanning text (one pass, no gradients).
 * Attention is computed from the coordinate geometry.
 *
 * Modes:
 *   ./lattice_chat --ingest data/wiki.txt   # learn vocabulary + density
 *   ./lattice_chat --load                   # interactive chat
 *   ./lattice_chat                          # chat with primes-only vocab
 *
 * Build:
 *   cc -O3 -Wall -march=native -o lattice_chat lattice_chat.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "wave.h"
#include "tokenize.h"
#include "primes_lookup.h"
#include "distance.h"
#include "attention.h"
#include "bonds.h"
#include "density.h"

/* ================================================================
 * Learner -- memetic context vocabulary
 * ================================================================
 *
 * The learner builds word coordinates from observed context.
 * For words not in primes_lookup.h, it derives a coordinate by
 * averaging the coordinates of neighboring words in the text.
 *
 * This is the "dark matter" mechanism: every word occurrence
 * contributes to the vocabulary, whether or not it was queried.
 *
 * Format: open-addressed hash table. Each slot stores a word
 * (lowercased, up to 31 bytes) and its accumulated coordinate.
 */

#define LEARNER_CAP    65536
#define LEARNER_MASK   (LEARNER_CAP - 1)
#define LEARNER_MAXWORD 31

typedef struct {
    char     word[LEARNER_MAXWORD + 1];  /* null-terminated, lowercase */
    coord4   coord;
    uint32_t count;                      /* times seen */
    uint8_t  occupied;
} LearnerSlot;

typedef struct {
    LearnerSlot slots[LEARNER_CAP];
    int         n_words;
} Learner;

static uint32_t learner_hash(const char *w, int len) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++) {
        uint8_t c = (uint8_t)w[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

static LearnerSlot *learner_find(Learner *L, const char *word, int len) {
    if (len > LEARNER_MAXWORD) len = LEARNER_MAXWORD;
    uint32_t h = learner_hash(word, len) & LEARNER_MASK;
    for (int probe = 0; probe < 128; probe++) {
        LearnerSlot *s = &L->slots[(h + probe) & LEARNER_MASK];
        if (!s->occupied) return NULL;
        if ((int)strlen(s->word) == len &&
            prime_streq(s->word, len, word, len))
            return s;
    }
    return NULL;
}

static LearnerSlot *learner_insert(Learner *L, const char *word, int len,
                                    coord4 coord) {
    if (len > LEARNER_MAXWORD) len = LEARNER_MAXWORD;
    if (L->n_words >= LEARNER_CAP * 3 / 4) return NULL;  /* 75% load */

    uint32_t h = learner_hash(word, len) & LEARNER_MASK;
    for (int probe = 0; probe < 128; probe++) {
        LearnerSlot *s = &L->slots[(h + probe) & LEARNER_MASK];
        if (!s->occupied) {
            for (int i = 0; i < len; i++)
                s->word[i] = (char)tok_lower((uint8_t)word[i]);
            s->word[len] = '\0';
            s->coord = coord;
            s->count = 1;
            s->occupied = 1;
            L->n_words++;
            return s;
        }
        if ((int)strlen(s->word) == len &&
            prime_streq(s->word, len, word, len)) {
            /* already exists -- update running average */
            float n = (float)s->count;
            float inv = 1.0f / (n + 1.0f);
            s->coord.t = (int8_t)roundf((n * s->coord.t + coord.t) * inv);
            s->coord.d = (int8_t)roundf((n * s->coord.d + coord.d) * inv);
            s->coord.m = (int8_t)roundf((n * s->coord.m + coord.m) * inv);
            s->coord.q = (int8_t)roundf((n * s->coord.q + coord.q) * inv);
            s->count++;
            return s;
        }
    }
    return NULL;
}

/*
 * Look up a word: learner first, primes_lookup fallback.
 * Returns the coordinate. Sets *found to 1 if known, 0 if unknown.
 */
static coord4 vocab_lookup(Learner *L, const char *word, int len, int *found) {
    /* learner first */
    if (L) {
        LearnerSlot *s = learner_find(L, word, len);
        if (s) {
            if (found) *found = 1;
            return s->coord;
        }
    }

    /* primes_lookup fallback */
    prime_result pr = prime_lookup(word, len);
    if (pr.status == PRIME_FOUND) {
        if (found) *found = 1;
        return pr.c;
    }

    /* unknown: derive from character wave bytes (phonetic hash) */
    if (found) *found = 0;
    int st = 0, sd = 0, sm = 0, sq = 0;
    int count = 0;
    for (int i = 0; i < len; i++) {
        uint8_t wb = utf8_to_wave((uint8_t)word[i]);
        if (wb != 0x00) {
            coord4 c = wave_decode(wb);
            st += c.t; sd += c.d; sm += c.m; sq += c.q;
            count++;
        }
    }
    coord4 r;
    if (count > 0) {
        r.t = (int8_t)(st / count);
        r.d = (int8_t)(sd / count);
        r.m = (int8_t)(sm / count);
        r.q = (int8_t)(sq / count);
    } else {
        r.t = 0; r.d = 0; r.m = 0; r.q = 0;
    }
    return r;
}

/* ---- RNG helpers ---- */

static uint64_t rng_state = 0;

static void rng_seed(uint64_t s) {
    rng_state = s ? s : 1;
}

/* xorshift64* -- fast, decent quality */
static uint64_t rng_next(void) {
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

/* uniform float in [0, 1) */
static float rng_float(void) {
    return (float)(rng_next() >> 11) / (float)(1ULL << 53);
}

/* ---- temperature parameter ---- */
static float g_temperature = 0.7f;

/*
 * Reverse lookup with diversity: find the top N closest known words
 * to a coordinate, then sample one weighted by proximity.
 *
 * This handles the collision problem -- multiple words at the same
 * coordinate all get a chance, weighted by how close they are.
 *
 * exclude/n_exclude: list of words to skip (recently generated).
 * temperature controls sampling sharpness (lower = more deterministic).
 */

#define REVERSE_TOP_N 5

typedef struct {
    const char *word;
    float       dist;
} ReverseCandidate;

/* insert into sorted top-N array (ascending by dist) */
static void rc_insert(ReverseCandidate *top, int *n, int cap,
                       const char *word, float dist) {
    if (*n < cap) {
        top[*n].word = word;
        top[*n].dist = dist;
        (*n)++;
        for (int i = *n - 1; i > 0 && top[i].dist < top[i-1].dist; i--) {
            ReverseCandidate tmp = top[i];
            top[i] = top[i-1];
            top[i-1] = tmp;
        }
        return;
    }
    if (dist >= top[cap - 1].dist) return;
    top[cap - 1].word = word;
    top[cap - 1].dist = dist;
    for (int i = cap - 1; i > 0 && top[i].dist < top[i-1].dist; i--) {
        ReverseCandidate tmp = top[i];
        top[i] = top[i-1];
        top[i-1] = tmp;
    }
}

static const char *vocab_reverse_ex(Learner *L, coord4 target,
                                     const char **exclude, int n_exclude) {
    ReverseCandidate top[REVERSE_TOP_N];
    int n_top = 0;
    const char *best_excluded = NULL;
    float best_excluded_dist = 1e9f;

    /* search primes table */
    for (int i = 0; i < (int)PRIME_WORD_COUNT; i++) {
        coord4 c;
        c.t = prime_words[i].t;
        c.d = prime_words[i].d;
        c.m = prime_words[i].m;
        c.q = prime_words[i].q;
        float d = coord4_distance(target, c);
        int exc = 0;
        for (int e = 0; e < n_exclude; e++)
            if (exclude[e] && strcmp(prime_words[i].word, exclude[e]) == 0)
                { exc = 1; break; }
        if (exc) {
            if (d < best_excluded_dist) {
                best_excluded_dist = d;
                best_excluded = prime_words[i].word;
            }
            continue;
        }
        rc_insert(top, &n_top, REVERSE_TOP_N, prime_words[i].word, d);
    }

    /* search learner */
    if (L) {
        for (int i = 0; i < LEARNER_CAP; i++) {
            LearnerSlot *s = &L->slots[i];
            if (!s->occupied) continue;
            if (s->word[0] >= '0' && s->word[0] <= '9') continue;
            if (s->word[1] == '\0') continue;

            float d = coord4_distance(target, s->coord);
            int exc = 0;
            for (int e = 0; e < n_exclude; e++)
                if (exclude[e] && strcmp(s->word, exclude[e]) == 0)
                    { exc = 1; break; }
            if (exc) {
                if (d < best_excluded_dist) {
                    best_excluded_dist = d;
                    best_excluded = s->word;
                }
                continue;
            }
            rc_insert(top, &n_top, REVERSE_TOP_N, s->word, d);
        }
    }

    if (n_top == 0) {
        return best_excluded ? best_excluded : "...";
    }

    /* weighted random sampling from top N candidates.
     * Weight = 1/(1 + dist), raised to 1/temperature for sharpness. */
    float weights[REVERSE_TOP_N];
    float sum = 0.0f;
    float inv_temp = 1.0f / (g_temperature + 0.01f);
    for (int i = 0; i < n_top; i++) {
        float w = 1.0f / (1.0f + top[i].dist);
        w = powf(w, inv_temp);
        weights[i] = w;
        sum += w;
    }

    float r = rng_float() * sum;
    float acc = 0.0f;
    for (int i = 0; i < n_top; i++) {
        acc += weights[i];
        if (r < acc) return top[i].word;
    }
    return top[n_top - 1].word;
}

/* convenience wrapper without exclusion list */
#define vocab_reverse(L, target) vocab_reverse_ex(L, target, NULL, 0)

/* ================================================================
 * Transition density -- bigram-level coordinate transitions
 * ================================================================
 *
 * The global density map tells you "which coordinates are common."
 * The transition density tells you "given this coordinate, which
 * coordinates follow?" This captures word-order information that
 * the global density cannot.
 *
 * Implementation: for each shell-1 source bin, store counts of
 * which shell-1 target bin follows. 81 * 81 = 6561 entries.
 * Fits comfortably in ~26 KB.
 */

#define TRANS_BINS (DENSITY_S1_BINS * DENSITY_S1_BINS)

typedef struct {
    uint32_t counts[TRANS_BINS];  /* [src * 81 + dst] */
    uint64_t total;
} TransitionMap;

static inline void trans_add(TransitionMap *tm, coord4 from, coord4 to) {
    int src = coord4_to_bin_s1(from);
    int dst = coord4_to_bin_s1(to);
    tm->counts[src * DENSITY_S1_BINS + dst]++;
    tm->total++;
}

/*
 * Query: given source coordinate, sample k destination bins
 * proportional to their transition probability. Temperature-scaled.
 * Returns k unique results (or fewer if not enough non-zero bins).
 */
static int trans_predict(const TransitionMap *tm, coord4 from,
                          int k, coord4 *results) {
    int src = coord4_to_bin_s1(from);
    const uint32_t *row = &tm->counts[src * DENSITY_S1_BINS];

    /* sum for this row */
    uint32_t row_sum = 0;
    for (int i = 0; i < DENSITY_S1_BINS; i++)
        row_sum += row[i];

    if (row_sum == 0) return 0;

    /* temperature-scaled probabilities */
    float scores[DENSITY_S1_BINS];
    float inv_temp = 1.0f / (g_temperature + 0.01f);
    float total = 0.0f;
    for (int i = 0; i < DENSITY_S1_BINS; i++) {
        if (row[i] == 0) {
            scores[i] = 0.0f;
        } else {
            float p = (float)row[i] / (float)row_sum;
            scores[i] = powf(p, inv_temp);
            total += scores[i];
        }
    }

    if (total <= 0.0f) return 0;

    /* sample k unique bins */
    int found = 0;
    float used[DENSITY_S1_BINS];
    for (int i = 0; i < DENSITY_S1_BINS; i++) used[i] = scores[i];

    for (int pick = 0; pick < k && pick < DENSITY_S1_BINS; pick++) {
        float r = rng_float() * total;
        float acc = 0.0f;
        int chosen = -1;
        for (int i = 0; i < DENSITY_S1_BINS; i++) {
            if (used[i] <= 0.0f) continue;
            acc += used[i];
            if (r < acc) { chosen = i; break; }
        }
        if (chosen < 0) break;
        results[found++] = bin_s1_to_coord(chosen);
        total -= used[chosen];
        used[chosen] = 0.0f;
    }
    return found;
}

/* save/load transition map */
#define TRANS_MAGIC 0x4E415254  /* "TRAN" */

static int trans_save(const TransitionMap *tm, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }
    uint32_t magic = TRANS_MAGIC;
    fwrite(&magic, 4, 1, f);
    fwrite(&tm->total, 8, 1, f);
    fwrite(tm->counts, sizeof(uint32_t), TRANS_BINS, f);
    fclose(f);
    return 0;
}

static TransitionMap *trans_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint32_t magic;
    if (fread(&magic, 4, 1, f) != 1 || magic != TRANS_MAGIC) {
        fclose(f);
        return NULL;
    }
    TransitionMap *tm = (TransitionMap *)calloc(1, sizeof(TransitionMap));
    if (!tm) { fclose(f); return NULL; }
    if (fread(&tm->total, 8, 1, f) != 1) goto fail;
    if (fread(tm->counts, sizeof(uint32_t), TRANS_BINS, f) != TRANS_BINS)
        goto fail;
    fclose(f);
    return tm;
fail:
    free(tm);
    fclose(f);
    return NULL;
}


/* ================================================================
 * Learner save/load
 * ================================================================ */

#define LEARNER_MAGIC 0x4E524C56  /* "VLRN" */

static int learner_save(const Learner *L, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }

    uint32_t magic = LEARNER_MAGIC;
    fwrite(&magic, 4, 1, f);

    uint32_t n = (uint32_t)L->n_words;
    fwrite(&n, 4, 1, f);

    for (int i = 0; i < LEARNER_CAP; i++) {
        const LearnerSlot *s = &L->slots[i];
        if (!s->occupied) continue;
        uint8_t wlen = (uint8_t)strlen(s->word);
        fwrite(&wlen, 1, 1, f);
        fwrite(s->word, 1, wlen, f);
        fwrite(&s->coord, sizeof(coord4), 1, f);
        fwrite(&s->count, 4, 1, f);
    }

    fclose(f);
    return 0;
}

static Learner *learner_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    uint32_t magic;
    if (fread(&magic, 4, 1, f) != 1 || magic != LEARNER_MAGIC) {
        fclose(f);
        return NULL;
    }

    Learner *L = (Learner *)calloc(1, sizeof(Learner));
    if (!L) { fclose(f); return NULL; }

    uint32_t n;
    if (fread(&n, 4, 1, f) != 1) goto fail;

    for (uint32_t i = 0; i < n; i++) {
        uint8_t wlen;
        if (fread(&wlen, 1, 1, f) != 1) goto fail;
        if (wlen > LEARNER_MAXWORD) goto fail;

        char word[LEARNER_MAXWORD + 1];
        if (fread(word, 1, wlen, f) != wlen) goto fail;
        word[wlen] = '\0';

        coord4 coord;
        uint32_t count;
        if (fread(&coord, sizeof(coord4), 1, f) != 1) goto fail;
        if (fread(&count, 4, 1, f) != 1) goto fail;

        /* insert directly (don't average -- use stored coord) */
        uint32_t h = learner_hash(word, wlen) & LEARNER_MASK;
        for (int probe = 0; probe < 128; probe++) {
            LearnerSlot *s = &L->slots[(h + probe) & LEARNER_MASK];
            if (!s->occupied) {
                memcpy(s->word, word, wlen + 1);
                s->coord = coord;
                s->count = count;
                s->occupied = 1;
                L->n_words++;
                break;
            }
        }
    }

    fclose(f);
    return L;

fail:
    free(L);
    fclose(f);
    return NULL;
}


/* ================================================================
 * The pipeline: sentence -> walk -> attention -> predict -> generate
 * ================================================================ */

#define MAX_WALK   256
#define MAX_GEN    40

/*
 * Build a walk from a text string.
 * Tokenizes, looks up each word coordinate, fills walk[].
 * Returns the walk length.
 * Also fills word_strs/word_lens for reverse reference.
 */
static int build_walk(Learner *L, const char *text, int text_len,
                       coord4 *walk, int max_walk,
                       Token *toks, int max_toks) {
    int ntok = tokenize_line(text, text_len, toks, max_toks);
    int wlen = 0;

    for (int i = 0; i < ntok && wlen < max_walk; i++) {
        if (toks[i].is_punct) continue;  /* skip punctuation in the walk */

        char word_buf[64];
        int wl = toks[i].len;
        if (wl > 63) wl = 63;
        for (int j = 0; j < wl; j++)
            word_buf[j] = (char)tok_lower(toks[i].start[j]);
        word_buf[wl] = '\0';

        int found = 0;
        walk[wlen] = vocab_lookup(L, word_buf, wl, &found);
        wlen++;
    }

    return wlen;
}

/*
 * Predict the next word given a walk.
 *
 * Pipeline:
 *   1. Run causal sparse attention on the walk -> context vector
 *   2. Use transition map as PRIMARY predictor (born-indexed knowledge)
 *   3. Fall back to density map if no transition data
 *   4. Bond-guided coordinate shift for sentence-level coherence
 *   5. Weighted random reverse lookup (top 5, proximity-sampled)
 *
 * step_idx is the generation step (0-based).
 * recent/n_recent: words to exclude from reverse lookup (avoid cycles).
 * recent_bonds/n_recent_bonds: bond types of recent transitions for
 *   sentence-level coherence tracking.
 */
static const char *predict_next(Learner *L, DensityMap *dm, TransitionMap *tm,
                                 const coord4 *walk, int walk_len,
                                 int step_idx,
                                 const char **recent, int n_recent,
                                 const BondType *recent_bonds,
                                 int n_recent_bonds) {
    if (walk_len == 0) return "...";

    /* 1. Causal sparse attention over the walk */
    AttentionResult attn = causal_sparse_attention(
        walk, walk_len, walk_len - 1, ATTN_SPARSITY_DEFAULT);

    coord4 context = attn.context;
    coord4 last = walk[walk_len - 1];
    coord4 predicted = context;

    /* 2. Transition map is the PRIMARY predictor.
     *    Records what coordinates actually followed this coordinate
     *    in real text -- this is the born-indexed knowledge. */
    int used_transition = 0;
    if (tm && walk_len > 0) {
        coord4 trans_cands[6];
        int ntrans = trans_predict(tm, last, 6, trans_cands);

        if (ntrans > 0) {
            used_transition = 1;

            /* Build a combined candidate pool: transition + density.
             * Score each, then sample proportionally (weighted random). */
            #define POOL_MAX 16
            coord4 pool[POOL_MAX];
            float pool_scores[POOL_MAX];
            int pool_n = 0;

            /* transition candidates (high priority) */
            for (int i = 0; i < ntrans && pool_n < POOL_MAX; i++) {
                float d_attn = coord4_distance(trans_cands[i], context);
                float score = 1.5f / (1.0f + d_attn);
                if (dm) {
                    float prob = density_probability(dm, trans_cands[i]);
                    score += prob * 2.0f;
                }
                pool[pool_n] = trans_cands[i];
                pool_scores[pool_n] = score;
                pool_n++;
            }

            /* density candidates if available */
            if (dm) {
                coord4 dcands[8];
                int nd = density_nearest(dm, context, 8, dcands);
                for (int i = 0; i < nd && pool_n < POOL_MAX; i++) {
                    float d_attn = coord4_distance(dcands[i], context);
                    float d_trans = coord4_distance(dcands[i],
                                                     trans_cands[0]);
                    float score = 1.0f / (1.0f + d_attn) +
                                  0.5f / (1.0f + d_trans);
                    pool[pool_n] = dcands[i];
                    pool_scores[pool_n] = score;
                    pool_n++;
                }
            }

            /* weighted random sampling from pool */
            if (pool_n > 0) {
                float inv_temp = 1.0f / (g_temperature + 0.01f);
                float psum = 0.0f;
                for (int i = 0; i < pool_n; i++) {
                    pool_scores[i] = powf(pool_scores[i], inv_temp);
                    psum += pool_scores[i];
                }
                float r = rng_float() * psum;
                float pacc = 0.0f;
                predicted = pool[pool_n - 1];
                for (int i = 0; i < pool_n; i++) {
                    pacc += pool_scores[i];
                    if (r < pacc) { predicted = pool[i]; break; }
                }
            }
            #undef POOL_MAX
        }
    }

    /* 3. Fall back to density map if no transition data */
    if (!used_transition && dm) {
        coord4 dcands[8];
        int nd = density_nearest(dm, context, 8, dcands);
        if (nd > 0) {
            float inv_temp = 1.0f / (g_temperature + 0.01f);
            float dscores[8];
            float dsum = 0.0f;
            for (int i = 0; i < nd; i++) {
                float d = coord4_distance(dcands[i], context);
                float s = 1.0f / (1.0f + d);
                dscores[i] = powf(s, inv_temp);
                dsum += dscores[i];
            }
            float r = rng_float() * dsum;
            float dacc = 0.0f;
            predicted = dcands[nd - 1];
            for (int i = 0; i < nd; i++) {
                dacc += dscores[i];
                if (r < dacc) { predicted = dcands[i]; break; }
            }
        }
    }

    /* 4. Bond-guided coordinate shift for sentence-level coherence.
     *
     * English walks cycle through dimensions: M->T->M->D
     * (noun -> verb -> noun -> preposition).
     *
     * If the last 3 bonds are all the same type, force the next word
     * to activate a DIFFERENT force dimension. */
    {
        int force_dim = -1;  /* -1 = no override */

        /* detect bond repetition in recent history */
        if (n_recent_bonds >= 3) {
            BondType b0 = recent_bonds[n_recent_bonds - 1];
            BondType b1 = recent_bonds[n_recent_bonds - 2];
            BondType b2 = recent_bonds[n_recent_bonds - 3];
            if (b0 == b1 && b1 == b2 && b0 != BOND_NONE) {
                /* 3 identical bonds: advance the dimension cycle */
                int dim_cycle[4] = {2, 0, 1, 3}; /* M, T, D, Q */
                int cur_dim = -1;
                if (b0 & BOND_R)       cur_dim = 2;
                else if (b0 & BOND_P)  cur_dim = 0;
                else if (b0 & BOND_C)  cur_dim = 1;
                else if (b0 & BOND_W)  cur_dim = 3;
                if (cur_dim >= 0) {
                    for (int i = 0; i < 4; i++) {
                        if (dim_cycle[i] == cur_dim) {
                            force_dim = dim_cycle[(i + 1) % 4];
                            break;
                        }
                    }
                }
            }
        }

        /* no data at all -- pure lattice navigation */
        if (!dm && !tm) {
            BondType last_bond = BOND_NONE;
            if (walk_len >= 2)
                last_bond = classify_bond(walk[walk_len - 2], last);

            int dim_cycle[4] = {2, 0, 1, 3};
            int phase = step_idx % 4;
            int active_dim = dim_cycle[phase];
            int8_t shift = (int8_t)((rng_next() & 1) ? 1 : -1);

            if (bond_has_force(last_bond, BOND_R) && active_dim == 2)
                active_dim = 0;
            if (bond_has_force(last_bond, BOND_P) && active_dim == 0)
                active_dim = 1;
            if (force_dim >= 0) active_dim = force_dim;

            switch (active_dim) {
            case 0: predicted.t = last.t + shift; break;
            case 1: predicted.d = last.d + shift; break;
            case 2: predicted.m = last.m + shift; break;
            case 3: predicted.q = last.q + shift; break;
            }
        } else if (force_dim >= 0) {
            /* nudge predicted coordinate toward the forced dimension */
            int8_t shift = (int8_t)((rng_next() & 1) ? 1 : -1);
            switch (force_dim) {
            case 0: predicted.t += shift; break;
            case 1: predicted.d += shift; break;
            case 2: predicted.m += shift; break;
            case 3: predicted.q += shift; break;
            }
        }
    }

    /* 5. Avoid repeating the last word: if predicted == last, nudge */
    if (predicted.t == last.t && predicted.d == last.d &&
        predicted.m == last.m && predicted.q == last.q) {
        int8_t shift = (int8_t)((rng_next() & 1) ? 1 : -1);
        int dim = (int)(rng_next() % 4);
        switch (dim) {
        case 0: predicted.t += shift; break;
        case 1: predicted.d += shift; break;
        case 2: predicted.m += shift; break;
        case 3: predicted.q += shift; break;
        }
    }

    /* 6. Reverse lookup: coordinate -> word (excluding recent).
     *    vocab_reverse_ex does top-5 weighted random sampling. */
    return vocab_reverse_ex(L, predicted, recent, n_recent);
}


/*
 * Pick a random word from a DIFFERENT coordinate region than recent words.
 * Used when cycle detection needs to force genuine variety.
 * Avoids the shell-1 bins that recent walk entries occupy.
 */
static const char *pick_random_word(Learner *L,
                                     const coord4 *walk, int walk_len,
                                     const char **recent, int n_recent) {
    /* collect shell-1 bins of recent walk entries to avoid */
    int avoid_bins[8];
    int n_avoid = 0;
    for (int i = walk_len - 1; i >= 0 && n_avoid < 8; i--) {
        int bin = coord4_to_bin_s1(walk[i]);
        int dup = 0;
        for (int j = 0; j < n_avoid; j++)
            if (avoid_bins[j] == bin) { dup = 1; break; }
        if (!dup) avoid_bins[n_avoid++] = bin;
    }

    /* collect candidates from primes table NOT in avoided bins */
    const char *cands[64];
    int nc = 0;
    for (int i = 0; i < (int)PRIME_WORD_COUNT && nc < 64; i++) {
        coord4 c;
        c.t = prime_words[i].t; c.d = prime_words[i].d;
        c.m = prime_words[i].m; c.q = prime_words[i].q;
        int bin = coord4_to_bin_s1(c);
        int bad = 0;
        for (int j = 0; j < n_avoid; j++)
            if (avoid_bins[j] == bin) { bad = 1; break; }
        if (bad) continue;
        for (int j = 0; j < n_recent; j++)
            if (recent[j] && strcmp(prime_words[i].word, recent[j]) == 0)
                { bad = 1; break; }
        if (bad) continue;
        cands[nc++] = prime_words[i].word;
    }

    if (nc == 0) return "something";
    return cands[(int)(rng_next() % (uint64_t)nc)];
}


/*
 * Generate a continuation from a walk.
 *
 * Key improvements:
 *   - Excludes last 6 words from selection (stronger repetition penalty)
 *   - Detects cycles up to length 8
 *   - On cycle, picks a random word from a DIFFERENT coordinate region
 *   - Tracks bond sequence for sentence-level coherence
 *   - Stops on sentence-ending punctuation (. ? !) or 50 words
 *   - Can write to buffer (for --test mode) or stdout
 */

#define GEN_HISTORY 12
#define MAX_GEN_WORDS 50

static int generate_to_buf(Learner *L, DensityMap *dm, TransitionMap *tm,
                            coord4 *walk, int walk_len, int max_gen,
                            char *buf, int bufsize) {
    const char *history[GEN_HISTORY];
    int hist_len = 0;
    BondType bond_history[GEN_HISTORY];
    int bond_len = 0;
    int perturb_count = 0;
    int buf_pos = 0;
    int total_words = 0;

    if (max_gen > MAX_GEN_WORDS) max_gen = MAX_GEN_WORDS;

    for (int g = 0; g < max_gen; g++) {
        /* exclude the last 6 words to force variety */
        int n_excl = hist_len < 6 ? hist_len : 6;
        const char *excl[6];
        for (int i = 0; i < n_excl; i++)
            excl[i] = history[hist_len - n_excl + i];

        const char *word = predict_next(L, dm, tm, walk, walk_len, g,
                                         excl, n_excl,
                                         bond_history, bond_len);

        if (!word || strcmp(word, "...") == 0) break;

        /* cycle detection: check for repeating patterns of length 1-8 */
        int is_cycle = 0;
        if (hist_len >= 2) {
            int max_period = hist_len / 2;
            if (max_period > 8) max_period = 8;
            for (int period = 1; period <= max_period; period++) {
                int match = 1;
                for (int k = 0; k < period && match; k++) {
                    if (strcmp(history[hist_len - 1 - k],
                              history[hist_len - 1 - period - k]) != 0)
                        match = 0;
                }
                if (match && strcmp(word,
                        history[hist_len - period]) == 0) {
                    is_cycle = 1;
                    break;
                }
            }
        }

        if (is_cycle) {
            perturb_count++;
            /* pick a random word from a DIFFERENT coordinate region */
            word = pick_random_word(L, walk, walk_len,
                                     (const char **)history, hist_len);
            if (!word) break;

            /* after too many perturbations, stop gracefully */
            if (perturb_count >= 6) {
                if (buf) {
                    int wl = (int)strlen(word);
                    if (buf_pos + wl + 1 < bufsize) {
                        buf[buf_pos++] = ' ';
                        memcpy(buf + buf_pos, word, (size_t)wl);
                        buf_pos += wl;
                    }
                } else {
                    printf(" %s", word);
                    fflush(stdout);
                }
                total_words++;
                break;
            }
        } else {
            perturb_count = 0;
        }

        /* emit word */
        if (buf) {
            int wl = (int)strlen(word);
            if (buf_pos + wl + 1 < bufsize) {
                buf[buf_pos++] = ' ';
                memcpy(buf + buf_pos, word, (size_t)wl);
                buf_pos += wl;
            }
        } else {
            printf(" %s", word);
            fflush(stdout);
        }
        total_words++;

        /* record in history (circular) */
        if (hist_len < GEN_HISTORY) {
            history[hist_len++] = word;
        } else {
            for (int i = 0; i < GEN_HISTORY - 1; i++)
                history[i] = history[i + 1];
            history[GEN_HISTORY - 1] = word;
        }

        /* extend the walk and record bond */
        int found;
        coord4 wc = vocab_lookup(L, word, (int)strlen(word), &found);

        if (walk_len > 0) {
            BondType bt = classify_bond(walk[walk_len - 1], wc);
            if (bond_len < GEN_HISTORY) {
                bond_history[bond_len++] = bt;
            } else {
                for (int i = 0; i < GEN_HISTORY - 1; i++)
                    bond_history[i] = bond_history[i + 1];
                bond_history[GEN_HISTORY - 1] = bt;
            }
        }

        if (walk_len < MAX_WALK) {
            walk[walk_len++] = wc;
        } else {
            for (int i = 0; i < MAX_WALK - 1; i++)
                walk[i] = walk[i + 1];
            walk[MAX_WALK - 1] = wc;
        }

        /* stop on sentence-ending punctuation */
        {
            int wl = (int)strlen(word);
            char last_ch = word[wl - 1];
            if (last_ch == '.' || last_ch == '?' || last_ch == '!')
                break;
        }
    }

    if (buf) {
        buf[buf_pos] = '\0';
    } else {
        printf("\n");
        fflush(stdout);
    }
    return total_words;
}

/* convenience wrapper: generate to stdout */
static void generate(Learner *L, DensityMap *dm, TransitionMap *tm,
                      coord4 *walk, int walk_len, int max_gen) {
    generate_to_buf(L, dm, tm, walk, walk_len, max_gen, NULL, 0);
}


/* ================================================================
 * Ingestion -- process a text file, learn vocabulary + density
 * ================================================================ */

static void ingest_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(1); }

    fprintf(stderr, "ingesting %s...\n", path);

    Learner *L = (Learner *)calloc(1, sizeof(Learner));
    DensityMap *dm = density_create();
    TransitionMap *tm = (TransitionMap *)calloc(1, sizeof(TransitionMap));

    if (!L || !dm || !tm) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }

    /* read line by line */
    char line[4096];
    Token toks[512];
    long lines = 0;
    long words_total = 0;
    coord4 prev_coord = {0, 0, 0, 0};
    int have_prev = 0;

    /* context window for deriving unknown word coordinates */
    coord4 context_window[8];
    int ctx_len = 0;

    while (fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        int ntok = tokenize_line(line, len, toks, 512);

        for (int i = 0; i < ntok; i++) {
            if (toks[i].is_punct) continue;

            char word_buf[64];
            int wl = toks[i].len;
            if (wl > 63) wl = 63;
            for (int j = 0; j < wl; j++)
                word_buf[j] = (char)tok_lower(toks[i].start[j]);
            word_buf[wl] = '\0';

            /* look up coordinate */
            prime_result pr = prime_lookup(word_buf, wl);
            coord4 wc;

            if (pr.status == PRIME_FOUND) {
                wc = pr.c;
            } else if (ctx_len > 0) {
                /* derive from context: average of surrounding known words */
                int st = 0, sd = 0, sm = 0, sq = 0;
                for (int j = 0; j < ctx_len; j++) {
                    st += context_window[j].t;
                    sd += context_window[j].d;
                    sm += context_window[j].m;
                    sq += context_window[j].q;
                }
                wc.t = (int8_t)(st / ctx_len);
                wc.d = (int8_t)(sd / ctx_len);
                wc.m = (int8_t)(sm / ctx_len);
                wc.q = (int8_t)(sq / ctx_len);
            } else {
                /* fallback: phonetic hash */
                int st = 0, sd = 0, sm = 0, sq = 0;
                int cnt = 0;
                for (int j = 0; j < wl; j++) {
                    uint8_t wb = utf8_to_wave((uint8_t)word_buf[j]);
                    if (wb) {
                        coord4 c = wave_decode(wb);
                        st += c.t; sd += c.d; sm += c.m; sq += c.q;
                        cnt++;
                    }
                }
                if (cnt > 0) {
                    wc.t = (int8_t)(st / cnt);
                    wc.d = (int8_t)(sd / cnt);
                    wc.m = (int8_t)(sm / cnt);
                    wc.q = (int8_t)(sq / cnt);
                } else {
                    wc.t = 0; wc.d = 0; wc.m = 0; wc.q = 0;
                }
            }

            /* learn the word */
            learner_insert(L, word_buf, wl, wc);

            /* add to density map */
            density_add(dm, wc);

            /* add to transition map */
            if (have_prev) {
                trans_add(tm, prev_coord, wc);
            }
            prev_coord = wc;
            have_prev = 1;

            /* update context window (circular) */
            if (ctx_len < 8) {
                context_window[ctx_len++] = wc;
            } else {
                for (int j = 0; j < 7; j++)
                    context_window[j] = context_window[j + 1];
                context_window[7] = wc;
            }

            words_total++;
        }

        lines++;
        if (lines % 100000 == 0) {
            fprintf(stderr, "  %ldK lines, %ldK words, %d vocab...\r",
                    lines / 1000, words_total / 1000, L->n_words);
        }
    }

    fclose(f);

    fprintf(stderr, "\ndone: %ld lines, %ld words, %d vocabulary entries\n",
            lines, words_total, L->n_words);

    /* normalize density */
    density_normalize(dm);

    /* save everything */
    fprintf(stderr, "saving vocabulary...\n");
    learner_save(L, "lattice_vocab.bin");

    fprintf(stderr, "saving density map...\n");
    density_save(dm, "lattice_density.bin");

    fprintf(stderr, "saving transition map...\n");
    trans_save(tm, "lattice_trans.bin");

    /* print stats */
    density_print_stats(dm);
    fprintf(stderr, "\ntransition map: %lu transitions\n",
            (unsigned long)tm->total);

    fprintf(stderr, "\nsaved:\n");
    fprintf(stderr, "  lattice_vocab.bin    (%d words)\n", L->n_words);
    fprintf(stderr, "  lattice_density.bin  (%lu entries)\n",
            (unsigned long)dm->total);
    fprintf(stderr, "  lattice_trans.bin    (%lu transitions)\n",
            (unsigned long)tm->total);

    free(L);
    density_free(dm);
    free(tm);
}


/* ================================================================
 * Interactive chat loop
 * ================================================================ */

static void print_walk_debug(const coord4 *walk, int walk_len) {
    fprintf(stderr, "  walk [%d]: ", walk_len);
    for (int i = 0; i < walk_len && i < 12; i++) {
        fprintf(stderr, "(%+d,%+d,%+d,%+d) ",
                walk[i].t, walk[i].d, walk[i].m, walk[i].q);
    }
    if (walk_len > 12) fprintf(stderr, "...");
    fprintf(stderr, "\n");

    if (walk_len >= 2) {
        BondType b = classify_bond(walk[walk_len - 2], walk[walk_len - 1]);
        fprintf(stderr, "  last bond: %s (%s)\n",
                bond_name(b), bond_lattice_name(b));
    }

    if (walk_len > 0) {
        AttentionResult attn = causal_sparse_attention(
            walk, walk_len, walk_len - 1, ATTN_SPARSITY_DEFAULT);
        fprintf(stderr, "  attention context: (%+d,%+d,%+d,%+d) conf=%.2f\n",
                attn.context.t, attn.context.d,
                attn.context.m, attn.context.q,
                attn.confidence);
    }
}

static void interactive(Learner *L, DensityMap *dm, TransitionMap *tm,
                         int debug) {
    char input[2048];
    coord4 walk[MAX_WALK];
    Token toks[256];

    int vocab_size = L ? L->n_words : 0;
    int prime_count = (int)PRIME_WORD_COUNT;

    fprintf(stderr, "lattice chat\n");
    fprintf(stderr, "  vocabulary: %d learned + %d primes\n",
            vocab_size, prime_count);
    if (dm) fprintf(stderr, "  density: %lu entries\n",
                    (unsigned long)dm->total);
    if (tm) fprintf(stderr, "  transitions: %lu\n",
                    (unsigned long)tm->total);
    fprintf(stderr, "  type /debug to toggle debug output\n");
    fprintf(stderr, "  type /quit to exit\n\n");

    for (;;) {
        printf("you: ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;

        /* strip trailing newline */
        int ilen = (int)strlen(input);
        while (ilen > 0 && (input[ilen-1] == '\n' || input[ilen-1] == '\r'))
            input[--ilen] = '\0';

        if (ilen == 0) continue;

        /* commands */
        if (strcmp(input, "/quit") == 0 || strcmp(input, "/q") == 0)
            break;
        if (strcmp(input, "/debug") == 0) {
            debug = !debug;
            fprintf(stderr, "debug %s\n", debug ? "on" : "off");
            continue;
        }
        if (strncmp(input, "/lookup ", 8) == 0) {
            const char *w = input + 8;
            int wl = (int)strlen(w);
            int found;
            coord4 c = vocab_lookup(L, w, wl, &found);
            const char *nearest = vocab_reverse(L, c);
            fprintf(stderr, "  \"%s\" -> (%+d,%+d,%+d,%+d) %s [%c] nearest: %s\n",
                    w, c.t, c.d, c.m, c.q,
                    found ? "found" : "derived",
                    coord4_pos(c), nearest);
            continue;
        }

        /* build walk from input */
        int walk_len = build_walk(L, input, ilen, walk, MAX_WALK,
                                   toks, 256);

        if (walk_len == 0) {
            printf("bot: ...\n");
            continue;
        }

        if (debug) {
            print_walk_debug(walk, walk_len);
        }

        /* generate response */
        printf("bot:");
        fflush(stdout);

        generate(L, dm, tm, walk, walk_len, MAX_GEN);

        if (debug) {
            fprintf(stderr, "  walk after generation [%d]\n", walk_len);
        }
    }

    fprintf(stderr, "\n");
}


/* ================================================================
 * Main
 * ================================================================ */

/*
 * Self-test: generate 5 different responses to "what is" and verify
 * they are all different from each other.
 */
static int run_self_test(void) {
    fprintf(stderr, "self-test: generating 5 responses to \"what is\"\n");

    /* no learner, no density, no transitions -- primes only */
    char results[5][2048];
    const char *prompt = "what is";
    int prompt_len = (int)strlen(prompt);

    for (int trial = 0; trial < 5; trial++) {
        coord4 walk[MAX_WALK];
        Token toks[256];
        int walk_len = build_walk(NULL, prompt, prompt_len, walk,
                                   MAX_WALK, toks, 256);
        if (walk_len == 0) {
            fprintf(stderr, "FAIL: could not build walk from \"%s\"\n",
                    prompt);
            return 1;
        }

        results[trial][0] = '\0';
        int nwords = generate_to_buf(NULL, NULL, NULL, walk, walk_len,
                                      MAX_GEN_WORDS,
                                      results[trial],
                                      (int)sizeof(results[trial]));
        printf("  [%d] (%2d words):%s\n", trial + 1, nwords,
               results[trial]);
    }

    /* check that all 5 are different */
    int unique = 1;
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (strcmp(results[i], results[j]) == 0) {
                fprintf(stderr,
                    "FAIL: response %d and %d are identical:\n"
                    "  \"%s\"\n", i + 1, j + 1, results[i]);
                unique = 0;
            }
        }
    }

    if (unique) {
        printf("PASS: all 5 responses are different\n");
        return 0;
    } else {
        printf("FAIL: some responses were identical\n");
        return 1;
    }
}


int main(int argc, char **argv) {
    rng_seed((uint64_t)time(NULL));

    int mode_ingest = 0;
    int mode_load = 0;
    int mode_test = 0;
    int debug = 0;
    const char *ingest_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ingest") == 0 && i + 1 < argc) {
            mode_ingest = 1;
            ingest_path = argv[++i];
        } else if (strcmp(argv[i], "--load") == 0) {
            mode_load = 1;
        } else if (strcmp(argv[i], "--debug") == 0) {
            debug = 1;
        } else if (strcmp(argv[i], "--test") == 0) {
            mode_test = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            fprintf(stderr,
                "usage: lattice_chat [options]\n"
                "\n"
                "  --ingest FILE   process text file, build vocabulary + density\n"
                "  --load          load saved vocabulary + density, enter chat\n"
                "  --debug         show walk/attention debug info\n"
                "  --test          run self-test (generate 5 varied responses)\n"
                "  -h, --help      this message\n"
                "\n"
                "with no options: chat using primes-only vocabulary (333 words)\n"
                "\n"
                "example:\n"
                "  ./lattice_chat --ingest data/wiki.txt\n"
                "  ./lattice_chat --load\n"
                "  ./lattice_chat --load --debug\n"
                "  ./lattice_chat --test\n"
            );
            return 0;
        } else {
            fprintf(stderr, "unknown option: %s (try --help)\n", argv[i]);
            return 1;
        }
    }

    /* ingest mode */
    if (mode_ingest) {
        ingest_file(ingest_path);
        return 0;
    }

    /* test mode */
    if (mode_test) {
        return run_self_test();
    }

    /* load or bare mode */
    Learner *L = NULL;
    DensityMap *dm = NULL;
    TransitionMap *tm = NULL;

    if (mode_load) {
        fprintf(stderr, "loading vocabulary...\n");
        L = learner_load("lattice_vocab.bin");
        if (L) fprintf(stderr, "  %d words\n", L->n_words);
        else fprintf(stderr, "  (not found, using primes only)\n");

        fprintf(stderr, "loading density map...\n");
        dm = density_load("lattice_density.bin");
        if (dm) fprintf(stderr, "  %lu entries\n", (unsigned long)dm->total);
        else fprintf(stderr, "  (not found)\n");

        fprintf(stderr, "loading transition map...\n");
        tm = trans_load("lattice_trans.bin");
        if (tm) fprintf(stderr, "  %lu transitions\n",
                        (unsigned long)tm->total);
        else fprintf(stderr, "  (not found)\n");

        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, "no --load: using primes-only vocabulary (%d words)\n\n",
                (int)PRIME_WORD_COUNT);
    }

    interactive(L, dm, tm, debug);

    /* cleanup */
    if (L) free(L);
    if (dm) density_free(dm);
    if (tm) free(tm);

    return 0;
}
