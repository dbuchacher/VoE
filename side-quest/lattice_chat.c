/*
 * lattice_chat.c -- lattice-native intelligence: ingest + chat
 *
 * Pipeline:
 *   Input -> tokenize -> learner (morpheme + bond + context) ->
 *   attention (4D heads + RC decay) -> density/transition ->
 *   bond-guided predict -> reverse lookup -> output
 *
 * No neural network. No training. No dictionary lookup table.
 * The learner IS the vocabulary. Bond patterns are the seed.
 * Density map IS the model. The equation provides everything.
 *
 * Modes:
 *   ./lattice_chat --ingest FILE   learn vocabulary + density from text
 *   ./lattice_chat --load          interactive chat with saved data
 *   ./lattice_chat                 chat with primes-only (65 anchors)
 *
 * Build:
 *   cc -O3 -Wall -march=native -o lattice_chat lattice_chat.c learner.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "wave.h"
#include "tokenize.h"
#include "distance.h"
#include "attention.h"
#include "bonds.h"
#include "density.h"
#include "learner.h"


/* ================================================================
 * RNG — xorshift64*
 * ================================================================ */

static uint64_t rng_state = 0;

static void rng_seed(uint64_t s) { rng_state = s ? s : 1; }

static uint64_t rng_next(void) {
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static float rng_float(void) {
    return (float)(rng_next() >> 11) / (float)(1ULL << 53);
}


/* ================================================================
 * Temperature
 * ================================================================ */

static float g_temperature = 0.7f;


/* ================================================================
 * Transition map — 81x81 coordinate bigrams
 * ================================================================ */

#define TRANS_BINS (DENSITY_S1_BINS * DENSITY_S1_BINS)

typedef struct {
    uint32_t counts[TRANS_BINS];
    uint64_t total;
} TransitionMap;

static inline void trans_add(TransitionMap *tm, coord4 from, coord4 to) {
    int src = coord4_to_bin_s1(from);
    int dst = coord4_to_bin_s1(to);
    tm->counts[src * DENSITY_S1_BINS + dst]++;
    tm->total++;
}

static int trans_predict(const TransitionMap *tm, coord4 from,
                         int k, coord4 *results) {
    int src = coord4_to_bin_s1(from);
    const uint32_t *row = &tm->counts[src * DENSITY_S1_BINS];

    uint32_t row_sum = 0;
    for (int i = 0; i < DENSITY_S1_BINS; i++)
        row_sum += row[i];
    if (row_sum == 0) return 0;

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
        fclose(f); return NULL;
    }
    TransitionMap *tm = calloc(1, sizeof(TransitionMap));
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
 * Reverse lookup with temperature sampling
 * ================================================================
 *
 * Uses learner_reverse to get top-N candidates, then samples
 * one weighted by proximity and temperature.
 */

#define REVERSE_TOP_N 8

static const char *reverse_sample(const Learner *lr, coord4 target,
                                  const char **exclude, int n_exclude) {
    ReverseResult cands[REVERSE_TOP_N];
    int n = learner_reverse(lr, target, cands, REVERSE_TOP_N,
                            exclude, n_exclude);
    if (n == 0) return "...";

    /* weighted random: weight = 1/(1+dist), raised to 1/temperature */
    float weights[REVERSE_TOP_N];
    float sum = 0.0f;
    float inv_temp = 1.0f / (g_temperature + 0.01f);
    for (int i = 0; i < n; i++) {
        float w = 1.0f / (1.0f + cands[i].dist);
        w = powf(w, inv_temp);
        weights[i] = w;
        sum += w;
    }

    float r = rng_float() * sum;
    float acc = 0.0f;
    for (int i = 0; i < n; i++) {
        acc += weights[i];
        if (r < acc) return cands[i].word;
    }
    return cands[n - 1].word;
}


/* ================================================================
 * Walk construction
 * ================================================================ */

#define MAX_WALK    256
#define CTX_WINDOW  8

static int build_walk(Learner *lr, const char *text, int text_len,
                      coord4 *walk, int max_walk,
                      Token *toks, int max_toks) {
    int ntok = tokenize_line(text, text_len, toks, max_toks);
    int wlen = 0;

    /* context window for the learner */
    coord4 ctx[CTX_WINDOW];
    int ctx_len = 0;

    for (int i = 0; i < ntok && wlen < max_walk; i++) {
        if (toks[i].is_punct) continue;

        char word_buf[64];
        int wl = toks[i].len;
        if (wl > 63) wl = 63;
        for (int j = 0; j < wl; j++)
            word_buf[j] = (char)tok_lower(toks[i].start[j]);
        word_buf[wl] = '\0';

        coord4 wc = learner_process_word(lr, word_buf, wl, ctx, ctx_len);
        walk[wlen++] = wc;

        /* update context window */
        if (ctx_len < CTX_WINDOW) {
            ctx[ctx_len++] = wc;
        } else {
            for (int j = 0; j < CTX_WINDOW - 1; j++)
                ctx[j] = ctx[j + 1];
            ctx[CTX_WINDOW - 1] = wc;
        }
    }

    return wlen;
}


/* ================================================================
 * Prediction pipeline
 * ================================================================
 *
 * 1. Causal sparse attention → context vector
 * 2. Transition map (primary: what follows this coordinate?)
 * 3. Density map (fallback: what's common near this context?)
 * 4. Bond-guided dimension cycling (sentence coherence)
 * 5. Reverse lookup through learner vocabulary
 */

static const char *predict_next(Learner *lr, DensityMap *dm, TransitionMap *tm,
                                const coord4 *walk, int walk_len,
                                int step_idx,
                                const char **recent, int n_recent,
                                const BondType *recent_bonds,
                                int n_recent_bonds) {
    if (walk_len == 0) return "...";

    /* 1. attention */
    AttentionResult attn = causal_sparse_attention(
        walk, walk_len, walk_len - 1, ATTN_SPARSITY_DEFAULT);

    coord4 context = attn.context;
    coord4 last = walk[walk_len - 1];
    coord4 predicted = context;

    /* 2. transition map */
    int used_transition = 0;
    if (tm && walk_len > 0) {
        coord4 trans_cands[6];
        int ntrans = trans_predict(tm, last, 6, trans_cands);

        if (ntrans > 0) {
            used_transition = 1;

            #define POOL_MAX 16
            coord4 pool[POOL_MAX];
            float pool_scores[POOL_MAX];
            int pool_n = 0;

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

            if (dm) {
                coord4 dcands[8];
                int nd = density_nearest(dm, context, 8, dcands);
                for (int i = 0; i < nd && pool_n < POOL_MAX; i++) {
                    float d_attn = coord4_distance(dcands[i], context);
                    float d_trans = coord4_distance(dcands[i], trans_cands[0]);
                    float score = 1.0f / (1.0f + d_attn) +
                                  0.5f / (1.0f + d_trans);
                    pool[pool_n] = dcands[i];
                    pool_scores[pool_n] = score;
                    pool_n++;
                }
            }

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

    /* 3. density fallback */
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

    /* 4. bond-guided dimension cycling */
    {
        int force_dim = -1;

        if (n_recent_bonds >= 3) {
            BondType b0 = recent_bonds[n_recent_bonds - 1];
            BondType b1 = recent_bonds[n_recent_bonds - 2];
            BondType b2 = recent_bonds[n_recent_bonds - 3];
            if (b0 == b1 && b1 == b2 && b0 != BOND_NONE) {
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

        /* no data: pure lattice navigation */
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
            int8_t shift = (int8_t)((rng_next() & 1) ? 1 : -1);
            switch (force_dim) {
            case 0: predicted.t += shift; break;
            case 1: predicted.d += shift; break;
            case 2: predicted.m += shift; break;
            case 3: predicted.q += shift; break;
            }
        }
    }

    /* 5. avoid repeating last coordinate exactly */
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

    /* 6. reverse lookup through learner vocabulary */
    return reverse_sample(lr, predicted, recent, n_recent);
}


/* ================================================================
 * Pick a random word from a different coordinate region
 * ================================================================ */

static const char *pick_random_word(Learner *lr,
                                    const coord4 *walk, int walk_len,
                                    const char **recent, int n_recent) {
    /* pick a random coordinate NOT in recent bins */
    coord4 target;
    for (int attempt = 0; attempt < 20; attempt++) {
        target.t = (int8_t)((rng_next() % 3) - 1);
        target.d = (int8_t)((rng_next() % 3) - 1);
        target.m = (int8_t)((rng_next() % 3) - 1);
        target.q = (int8_t)((rng_next() % 3) - 1);

        /* check it's not in a recent bin */
        int bad = 0;
        int tbin = coord4_to_bin_s1(target);
        for (int i = walk_len - 1; i >= 0 && i >= walk_len - 8; i--) {
            if (coord4_to_bin_s1(walk[i]) == tbin) { bad = 1; break; }
        }
        if (!bad) break;
    }

    return reverse_sample(lr, target, recent, n_recent);
}


/* ================================================================
 * Generation
 * ================================================================ */

#define GEN_HISTORY   12
#define MAX_GEN_WORDS 50

static int generate_to_buf(Learner *lr, DensityMap *dm, TransitionMap *tm,
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
        int n_excl = hist_len < 6 ? hist_len : 6;
        const char *excl[6];
        for (int i = 0; i < n_excl; i++)
            excl[i] = history[hist_len - n_excl + i];

        const char *word = predict_next(lr, dm, tm, walk, walk_len, g,
                                        excl, n_excl,
                                        bond_history, bond_len);

        if (!word || strcmp(word, "...") == 0) break;

        /* cycle detection: patterns of length 1-8 */
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
            word = pick_random_word(lr, walk, walk_len,
                                    (const char **)history, hist_len);
            if (!word) break;
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

        /* emit */
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

        /* history (circular) */
        if (hist_len < GEN_HISTORY) {
            history[hist_len++] = word;
        } else {
            for (int i = 0; i < GEN_HISTORY - 1; i++)
                history[i] = history[i + 1];
            history[GEN_HISTORY - 1] = word;
        }

        /* extend walk: look up word coordinate via learner */
        WordEntry *we = learner_lookup(lr, word, (int)strlen(word));
        coord4 wc = we ? we->coord : char_level_prior(word, (int)strlen(word));

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

        /* stop on sentence ender */
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

static void generate(Learner *lr, DensityMap *dm, TransitionMap *tm,
                     coord4 *walk, int walk_len, int max_gen) {
    generate_to_buf(lr, dm, tm, walk, walk_len, max_gen, NULL, 0);
}


/* ================================================================
 * Ingestion — process text, learn vocabulary + build density
 * ================================================================ */

static void ingest_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(1); }

    fprintf(stderr, "ingesting %s...\n", path);

    Learner *lr = learner_create();
    if (!lr) { fprintf(stderr, "out of memory\n"); exit(1); }
    learner_seed_primes(lr);

    DensityMap *dm = density_create();
    TransitionMap *tm = calloc(1, sizeof(TransitionMap));
    if (!dm || !tm) { fprintf(stderr, "out of memory\n"); exit(1); }

    char line[4096];
    Token toks[512];
    long lines = 0;
    long words_total = 0;
    coord4 prev_coord = {0, 0, 0, 0};
    int have_prev = 0;

    coord4 ctx[CTX_WINDOW];
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

            /* the learner handles everything: prime lookup, morpheme
             * decomposition, bond-pattern priors, char-level fallback,
             * born-indexing on first encounter, refinement on re-encounter */
            coord4 wc = learner_process_word(lr, word_buf, wl, ctx, ctx_len);

            /* density */
            density_add(dm, wc);

            /* transitions */
            if (have_prev)
                trans_add(tm, prev_coord, wc);
            prev_coord = wc;
            have_prev = 1;

            /* context window */
            if (ctx_len < CTX_WINDOW) {
                ctx[ctx_len++] = wc;
            } else {
                for (int j = 0; j < CTX_WINDOW - 1; j++)
                    ctx[j] = ctx[j + 1];
                ctx[CTX_WINDOW - 1] = wc;
            }

            words_total++;
        }

        lines++;
        if (lines % 100000 == 0) {
            fprintf(stderr, "  %ldK lines, %ldK words, %u vocab...\r",
                    lines / 1000, words_total / 1000, lr->vocab_count);
        }
    }

    fclose(f);

    fprintf(stderr, "\ndone: %ld lines, %ld words, %u vocabulary entries\n",
            lines, words_total, lr->vocab_count);

    density_normalize(dm);

    fprintf(stderr, "saving vocabulary...\n");
    learner_save(lr, "lattice_vocab.bin");

    fprintf(stderr, "saving density map...\n");
    density_save(dm, "lattice_density.bin");

    fprintf(stderr, "saving transition map...\n");
    trans_save(tm, "lattice_trans.bin");

    /* stats */
    density_print_stats(dm);
    learner_stats(lr);
    fprintf(stderr, "\ntransition map: %lu transitions\n",
            (unsigned long)tm->total);

    fprintf(stderr, "\nsaved:\n");
    fprintf(stderr, "  lattice_vocab.bin    (%u words)\n", lr->vocab_count);
    fprintf(stderr, "  lattice_density.bin  (%lu entries)\n",
            (unsigned long)dm->total);
    fprintf(stderr, "  lattice_trans.bin    (%lu transitions)\n",
            (unsigned long)tm->total);

    learner_free(lr);
    density_free(dm);
    free(tm);
}


/* ================================================================
 * Interactive chat
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

static void interactive(Learner *lr, DensityMap *dm, TransitionMap *tm,
                        int debug) {
    char input[2048];
    coord4 walk[MAX_WALK];
    Token toks[256];

    fprintf(stderr, "lattice chat\n");
    fprintf(stderr, "  vocabulary: %u words (%u primes)\n",
            lr->vocab_count, lr->total_prime_hits);  /* approximate */
    if (dm) fprintf(stderr, "  density: %lu entries\n",
                    (unsigned long)dm->total);
    if (tm) fprintf(stderr, "  transitions: %lu\n",
                    (unsigned long)tm->total);
    fprintf(stderr, "  type /debug to toggle debug output\n");
    fprintf(stderr, "  type /stats for vocabulary statistics\n");
    fprintf(stderr, "  type /quit to exit\n\n");

    for (;;) {
        printf("you: ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;

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
        if (strcmp(input, "/stats") == 0) {
            learner_stats(lr);
            continue;
        }
        if (strncmp(input, "/lookup ", 8) == 0) {
            const char *w = input + 8;
            int wl = (int)strlen(w);
            WordEntry *we = learner_lookup(lr, w, wl);
            if (we) {
                fprintf(stderr, "  \"%s\" -> (%+d,%+d,%+d,%+d) conf=%u\n",
                        w, we->coord.t, we->coord.d,
                        we->coord.m, we->coord.q,
                        we->confidence);
            } else {
                /* try processing it to see what it would get */
                coord4 ctx[1] = {{ .t = 0, .d = 0, .m = 0, .q = 0 }};
                coord4 c = learner_process_word(lr, w, wl, ctx, 0);
                fprintf(stderr, "  \"%s\" -> (%+d,%+d,%+d,%+d) [derived, now born-indexed]\n",
                        w, c.t, c.d, c.m, c.q);
            }
            continue;
        }

        /* build walk */
        int walk_len = build_walk(lr, input, ilen, walk, MAX_WALK, toks, 256);

        if (walk_len == 0) {
            printf("bot: ...\n");
            continue;
        }

        if (debug) print_walk_debug(walk, walk_len);

        printf("bot:");
        fflush(stdout);
        generate(lr, dm, tm, walk, walk_len, MAX_GEN_WORDS);

        if (debug)
            fprintf(stderr, "  walk after generation [%d]\n", walk_len);
    }

    fprintf(stderr, "\n");
}


/* ================================================================
 * Self-test
 * ================================================================ */

static int run_self_test(Learner *lr) {
    fprintf(stderr, "self-test: generating 5 responses to \"what is\"\n");

    char results[5][2048];
    const char *prompt = "what is";
    int prompt_len = (int)strlen(prompt);

    for (int trial = 0; trial < 5; trial++) {
        coord4 walk[MAX_WALK];
        Token toks[256];
        int walk_len = build_walk(lr, prompt, prompt_len, walk,
                                  MAX_WALK, toks, 256);
        if (walk_len == 0) {
            fprintf(stderr, "FAIL: could not build walk from \"%s\"\n", prompt);
            return 1;
        }

        results[trial][0] = '\0';
        int nwords = generate_to_buf(lr, NULL, NULL, walk, walk_len,
                                     MAX_GEN_WORDS,
                                     results[trial],
                                     (int)sizeof(results[trial]));
        printf("  [%d] (%2d words):%s\n", trial + 1, nwords, results[trial]);
    }

    int unique = 1;
    for (int i = 0; i < 5; i++)
        for (int j = i + 1; j < 5; j++)
            if (strcmp(results[i], results[j]) == 0) {
                fprintf(stderr, "FAIL: response %d and %d identical\n",
                        i + 1, j + 1);
                unique = 0;
            }

    printf("%s\n", unique ? "PASS: all 5 different" : "FAIL: duplicates");
    return unique ? 0 : 1;
}


/* ================================================================
 * Main
 * ================================================================ */

int main(int argc, char **argv) {
    rng_seed((uint64_t)time(NULL));

    int mode_ingest = 0, mode_load = 0, mode_test = 0, debug = 0;
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
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stderr,
                "usage: lattice_chat [options]\n\n"
                "  --ingest FILE   process text, build vocabulary + density\n"
                "  --load          load saved data, enter chat\n"
                "  --debug         show walk/attention debug info\n"
                "  --test          run self-test\n"
                "  -h, --help      this message\n\n"
                "with no options: chat using 65 prime anchors only\n\n"
                "example:\n"
                "  ./lattice_chat --ingest data/wiki.txt\n"
                "  ./lattice_chat --load\n"
                "  ./lattice_chat --load --debug\n"
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

    /* create learner (always — it's the foundation) */
    Learner *lr = NULL;
    DensityMap *dm = NULL;
    TransitionMap *tm = NULL;

    if (mode_load) {
        fprintf(stderr, "loading vocabulary...\n");
        lr = learner_load("lattice_vocab.bin");
        if (lr) {
            fprintf(stderr, "  %u words loaded\n", lr->vocab_count);
        } else {
            fprintf(stderr, "  not found, creating fresh learner\n");
            lr = learner_create();
            learner_seed_primes(lr);
        }

        fprintf(stderr, "loading density map...\n");
        dm = density_load("lattice_density.bin");
        if (dm) fprintf(stderr, "  %lu entries\n", (unsigned long)dm->total);
        else fprintf(stderr, "  not found\n");

        fprintf(stderr, "loading transition map...\n");
        tm = trans_load("lattice_trans.bin");
        if (tm) fprintf(stderr, "  %lu transitions\n",
                        (unsigned long)tm->total);
        else fprintf(stderr, "  not found\n");

        fprintf(stderr, "\n");
    } else {
        lr = learner_create();
        if (!lr) { fprintf(stderr, "out of memory\n"); return 1; }
        learner_seed_primes(lr);
        fprintf(stderr, "primes-only mode (%u anchors)\n\n", lr->vocab_count);
    }

    /* test mode */
    if (mode_test) {
        int ret = run_self_test(lr);
        learner_free(lr);
        if (dm) density_free(dm);
        if (tm) free(tm);
        return ret;
    }

    interactive(lr, dm, tm, debug);

    learner_free(lr);
    if (dm) density_free(dm);
    if (tm) free(tm);

    return 0;
}
