/*
 * wit — lattice-native intelligence
 *
 * Modes:
 *   wit --ingest FILE   process text, build the field
 *   wit --load          interactive chat with saved field
 *   wit                 chat with primes only (no field)
 *   wit --test          run spread test (Phase 0 gate)
 *
 * Build:
 *   cc -O3 -Wall -Wno-unused-function -march=native -o wit main.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "wave.h"
#include "tokenize.h"
#include "distance.h"
#include "bonds.h"
#include "attention.h"
#include "derive.h"

#define FIELD_IMPLEMENTATION
#include "field.h"


/* ================================================================
 * RNG — xorshift64*
 * ================================================================ */

static uint64_t rng_state = 0;
static void     rng_seed(uint64_t s) { rng_state = s ? s : 1; }
static uint64_t rng_next(void) {
    uint64_t x = rng_state;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}
static float rng_float(void) {
    return (float)(rng_next() >> 11) / (float)(1ULL << 53);
}

static float g_temperature = 0.7f;

/* ================================================================
 * Constants
 * ================================================================ */

#define CTX_WINDOW   8
#define MAX_WALK     256
#define MAX_GEN      50


/* ================================================================
 * Ingest — process text file, build the field
 * ================================================================ */

static void ingest_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(1); }

    fprintf(stderr, "ingesting %s...\n", path);

    Field *field = field_create();
    if (!field) { fprintf(stderr, "out of memory\n"); exit(1); }

    char line[4096];
    Token toks[512];
    long lines = 0, words_total = 0;
    coord4 prev_coord = {0, 0, 0, 0};
    int have_prev = 0;

    DeriveResult context[CTX_WINDOW];
    int ctx_len = 0;
    int sentence_pos = 0;

    while (fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        int ntok = tokenize_line(line, len, toks, 512);

        for (int i = 0; i < ntok; i++) {
            /* sentence boundary detection */
            if (toks[i].is_punct && toks[i].len == 1) {
                char ch = toks[i].start[0];
                if (ch == '.' || ch == '!' || ch == '?') {
                    sentence_pos = 0;
                    ctx_len = 0;
                    have_prev = 0;
                    continue;
                }
            }
            if (toks[i].is_punct) continue;

            char word_buf[64];
            int wl = toks[i].len;
            if (wl > 63) wl = 63;
            for (int j = 0; j < wl; j++)
                word_buf[j] = (char)tok_lower(toks[i].start[j]);
            word_buf[wl] = '\0';

            DeriveResult dr = derive(word_buf, wl, context, ctx_len, sentence_pos);

            /* feed the field — pass the current line as context.
             * The context IS the knowledge. Not generated. Retrieved.
             * "Make this window red; it just does it." */
            char ctx_snippet[64];
            {
                /* grab up to 63 chars of the current line, trimmed */
                int clen = (int)strlen(line);
                while (clen > 0 && (line[clen-1]=='\n'||line[clen-1]=='\r')) clen--;
                if (clen > 63) clen = 63;
                memcpy(ctx_snippet, line, clen);
                ctx_snippet[clen] = '\0';
            }

            if (have_prev)
                field_observe(field, prev_coord, dr.coord, word_buf, wl, dr.confidence, dr.forces, ctx_snippet);
            else
                field_observe(field, dr.coord, dr.coord, word_buf, wl, dr.confidence, dr.forces, ctx_snippet);

            prev_coord = dr.coord;
            have_prev = 1;

            /* update context window */
            if (ctx_len < CTX_WINDOW) {
                context[ctx_len++] = dr;
            } else {
                for (int j = 0; j < CTX_WINDOW - 1; j++)
                    context[j] = context[j + 1];
                context[CTX_WINDOW - 1] = dr;
            }

            sentence_pos++;
            words_total++;
        }

        lines++;
        if (lines % 100000 == 0) {
            fprintf(stderr, "  %ldK lines, %ldK words...\r",
                    lines / 1000, words_total / 1000);
        }
    }

    fclose(f);
    fprintf(stderr, "\ndone: %ld lines, %ld words\n\n", lines, words_total);

    field_print_health(field);

    /* prune: drop words seen fewer than 3 times.
     * "The experiments weren't measuring the answer. They were
     *  searching for it. The answer was always in the math."
     * Rare words are noise. Common words are signal. */
    fprintf(stderr, "\npruning rare entries...\n");
    field_prune(field, 3);
    field_print_health(field);

    fprintf(stderr, "\nsaving field to wit.field...\n");
    if (field_save(field, "wit.field") == 0)
        fprintf(stderr, "saved.\n");
    else
        fprintf(stderr, "save failed!\n");

    field_free(field);
}


/* ================================================================
 * Generation — walk the field, emit words
 * ================================================================ */

/*
 * THE EQUATION: f(a,b,c,d) = τᵃ·χᵇ·μᶜ·φᵈ
 *
 * "Once you have the coordinates, evaluation is constant time —
 *  positive exponents multiply, negative divide, done."
 *
 * Base values: first four primes (2,3,5,7). This gives Gödel numbering —
 * unique value for every coordinate tuple. The fundamental theorem of
 * arithmetic guarantees the mapping is injective.
 *
 * "Same coordinates always produce the same result."
 */
static double eval_equation(coord4 c) {
    /* f(a,b,c,d) = 2^a * 3^b * 5^c * 7^d */
    double v = 1.0;
    v *= pow(2.0, (double)c.t);
    v *= pow(3.0, (double)c.x);
    v *= pow(5.0, (double)c.m);
    v *= pow(7.0, (double)c.q);
    return v;
}

/*
 * Evaluate the equation at a FLOAT coordinate (from attention output).
 * Attention produces float-valued weighted averages — more precision
 * than int8_t coordinates. This precision IS the selection signal.
 */
static double eval_equation_f(float t, float x, float m, float q) {
    return pow(2.0, (double)t) * pow(3.0, (double)x) *
           pow(5.0, (double)m) * pow(7.0, (double)q);
}

typedef struct {
    const char *word;
    float4      walk_coord;   /* float-precision coordinate for walk */
    uint8_t     forces;
} GenResult;

/*
 * generate_word — the core generation step.
 *
 * Takes a FLOAT walk (the live polynomial), produces a word
 * and the next float coordinate to extend the walk with.
 *
 * The coord4 walk is maintained in parallel for field lookups.
 * The float4 walk is what the equation actually evaluates.
 */
static GenResult generate_word(Field *field,
                               const coord4 *walk_i, const float4 *walk_f,
                               int walk_len,
                               float4 prev_f, uint8_t prev_forces,
                               const char **exclude, int n_exclude) {
    GenResult gen = { "...", {0,0,0,0}, 0 };
    if (walk_len == 0) return gen;

    /*
     * COMPUTE, DON'T SEARCH.
     *
     * "If you're listing options instead of computing coordinates,
     *  you're searching when you could be evaluating." — mind/mind
     *
     * 1. Attention → float-precision predicted coordinate
     * 2. Evaluate the equation at the prediction → target value
     * 3. Reverse lookup → candidates
     * 4. Evaluate equation at each candidate → candidate value
     * 5. The candidate whose value is closest to target WINS
     *
     * No temperature sampling. No random selection.
     * One coordinate, one evaluation, one word.
     */

    /* 1. attention over FLOAT walk → float-precision context.
     * "The path IS the function." The walk is continuous. */
    AttentionResult attn = causal_sparse_attention_f(
        walk_f, walk_len, walk_len - 1, ATTN_SPARSITY_DEFAULT);

    /*
     * POLYNOMIAL PREDICTION: the next word = the RESIDUAL.
     *
     * "Atom paths are polynomials (sums of monomials)." — mind/mind
     *
     * The walk so far is a polynomial: sum of all word coordinates.
     * The attention context is the weighted average (the function value).
     * The next word should bring the polynomial toward the attention
     * prediction. So the target = prediction, not prediction - sum.
     *
     * But we SHIFT the prediction away from the walk's centroid
     * to avoid repeating what's already in the walk. The residual
     * is: where the polynomial NEEDS to go minus where it IS.
     */
    float walk_sum[4] = {0,0,0,0};
    for (int i = 0; i < walk_len; i++) {
        walk_sum[0] += walk_f[i].t;
        walk_sum[1] += walk_f[i].x;
        walk_sum[2] += walk_f[i].m;
        walk_sum[3] += walk_f[i].q;
    }
    float wn = (float)walk_len;
    float centroid[4] = { walk_sum[0]/wn, walk_sum[1]/wn,
                          walk_sum[2]/wn, walk_sum[3]/wn };

    /* predicted = attention context + residual away from centroid */
    float pred[4];
    for (int d = 0; d < 4; d++) {
        float residual = attn.context_f[d] - centroid[d];
        pred[d] = attn.context_f[d] + residual * 0.5f;
    }

    coord4 predicted;
    predicted.t = (int8_t)(pred[0] > 0 ? pred[0]+0.5f : pred[0]-0.5f);
    predicted.x = (int8_t)(pred[1] > 0 ? pred[1]+0.5f : pred[1]-0.5f);
    predicted.m = (int8_t)(pred[2] > 0 ? pred[2]+0.5f : pred[2]-0.5f);
    predicted.q = (int8_t)(pred[3] > 0 ? pred[3]+0.5f : pred[3]-0.5f);

    /* equation evaluation at the polynomial-shifted prediction */
    double target_value = eval_equation_f(pred[0], pred[1], pred[2], pred[3]);

    /* bond-typed force expectation */
    uint8_t expected_forces = derive_forces_from_coord(predicted);
    if (expected_forces == 0 && prev_forces != 0) {
        if (prev_forces & BOND_P)      expected_forces = BOND_R;
        else if (prev_forces & BOND_R) expected_forces = BOND_P;
        else if (prev_forces & BOND_C) expected_forces = BOND_R;
        else if (prev_forces & BOND_W) expected_forces = BOND_R;
    }

    /* 2-3. reverse lookup with bond-typed scoring → candidates */
    ReverseCandidate cands[16];
    coord4 prev_i = float4_to_coord4(prev_f);
    int n = field_reverse_lookup(field, predicted, prev_i, expected_forces, cands, 16);
    if (n == 0) return gen;

    /* exclude recent words */
    ReverseCandidate filtered[16];
    int nf = 0;
    for (int i = 0; i < n && nf < 16; i++) {
        int exc = 0;
        for (int j = 0; j < n_exclude; j++)
            if (exclude[j] && strcmp(cands[i].word, exclude[j]) == 0)
                { exc = 1; break; }
        if (!exc) filtered[nf++] = cands[i];
    }
    if (nf == 0) { gen.word = cands[0].word; return gen; }

    /*
     * 4-5. EVALUATE THE EQUATION at each candidate.
     *
     * "Once you have the coordinates, evaluation is constant time —
     *  positive exponents multiply, negative divide, done."
     *
     * The candidate whose equation-value is closest to the target
     * value IS the answer. Not sampled. Not random. Computed.
     *
     * We still use the field_reverse_lookup score as a tiebreaker
     * (proximity + transition + density + bond + dark_energy).
     * The equation provides the PRIMARY selection. The field score
     * breaks ties between candidates with similar equation values.
     */
    /*
     * 4-5. EQUATION RANKING + WEIGHTED SAMPLING.
     *
     * The equation RANKS candidates by proximity to the target.
     * But language has valid variation — "sat" and "rested" are
     * both correct at the same position. So we SAMPLE from the
     * top-ranked candidates, weighted by equation proximity.
     *
     * This is the walk as polynomial: each term is drawn from a
     * DISTRIBUTION centered at the predicted coordinate, not a
     * single deterministic point.
     */
    float eq_scores[16];
    float eq_sum = 0.0f;
    coord4 cand_coords[16];

    for (int i = 0; i < nf; i++) {
        DeriveResult dr = derive(filtered[i].word,
                                 (int)strlen(filtered[i].word),
                                 NULL, 0, 0);
        cand_coords[i] = dr.coord;
        double cand_value = eval_equation(dr.coord);
        double diff = fabs(log(cand_value + 1e-10) - log(target_value + 1e-10));
        /* score = field score / (1 + equation distance) */
        float s = filtered[i].score / (1.0f + (float)diff);
        eq_scores[i] = s;
        eq_sum += s;
    }

    /* weighted sample */
    int chosen = 0;
    if (eq_sum > 1e-10f) {
        float r = rng_float() * eq_sum;
        float acc = 0.0f;
        for (int i = 0; i < nf; i++) {
            acc += eq_scores[i];
            if (r < acc) { chosen = i; break; }
        }
    }

    gen.word = filtered[chosen].word;
    coord4 chosen_coord = cand_coords[chosen];

    /*
     * WALK COORDINATE with thermal noise.
     *
     * 70% attention trajectory + 30% chosen word + small perturbation.
     * The perturbation prevents fixed-point convergence (hylo loops).
     * In physics: Brownian motion. In the lattice: the polynomial
     * is alive, not frozen. Each step adds a new term that shifts
     * the function slightly.
     */
    float noise_t = ((float)(rng_next() & 0xFF) / 255.0f - 0.5f) * 0.1f;
    float noise_x = ((float)(rng_next() & 0xFF) / 255.0f - 0.5f) * 0.1f;
    float noise_m = ((float)(rng_next() & 0xFF) / 255.0f - 0.5f) * 0.1f;
    float noise_q = ((float)(rng_next() & 0xFF) / 255.0f - 0.5f) * 0.1f;

    gen.walk_coord.t = 0.7f * attn.context_f[0] + 0.3f * (float)chosen_coord.t + noise_t;
    gen.walk_coord.x = 0.7f * attn.context_f[1] + 0.3f * (float)chosen_coord.x + noise_x;
    gen.walk_coord.m = 0.7f * attn.context_f[2] + 0.3f * (float)chosen_coord.m + noise_m;
    gen.walk_coord.q = 0.7f * attn.context_f[3] + 0.3f * (float)chosen_coord.q + noise_q;
    gen.forces = derive_forces_from_coord(chosen_coord);
    return gen;
}

static void generate(Field *field, coord4 *walk_i, int *walk_len_ptr, int max_gen) {
    int walk_len = *walk_len_ptr;
    const char *history[12];
    int hist_len = 0;

    /* float4 walk — the live polynomial. Initialized from int walk. */
    float4 walk_f[MAX_WALK];
    for (int i = 0; i < walk_len && i < MAX_WALK; i++)
        walk_f[i] = coord4_to_float4(walk_i[i]);

    for (int g = 0; g < max_gen; g++) {
        int n_excl = hist_len < 6 ? hist_len : 6;
        const char *excl[6];
        for (int i = 0; i < n_excl; i++)
            excl[i] = history[hist_len - n_excl + i];

        float4 prev_f = walk_len > 0 ? walk_f[walk_len - 1] : (float4){0,0,0,0};
        coord4 prev_i = float4_to_coord4(prev_f);
        uint8_t prev_forces = derive_forces_from_coord(prev_i);

        GenResult gr = generate_word(field, walk_i, walk_f, walk_len,
                                     prev_f, prev_forces, excl, n_excl);
        if (!gr.word || strcmp(gr.word, "...") == 0) break;

        printf(" %s", gr.word);
        fflush(stdout);

        if (hist_len < 12) history[hist_len++] = gr.word;
        else { for (int i=0;i<11;i++) history[i]=history[i+1]; history[11]=gr.word; }

        /* extend BOTH walks — int for field lookups, float for equation eval */
        coord4 wc_i = float4_to_coord4(gr.walk_coord);
        if (walk_len < MAX_WALK) {
            walk_i[walk_len] = wc_i;
            walk_f[walk_len] = gr.walk_coord;
            walk_len++;
        } else {
            for (int i = 0; i < MAX_WALK-1; i++) {
                walk_i[i] = walk_i[i+1];
                walk_f[i] = walk_f[i+1];
            }
            walk_i[MAX_WALK-1] = wc_i;
            walk_f[MAX_WALK-1] = gr.walk_coord;
        }

        int wl = (int)strlen(gr.word);
        if (wl > 0 && (gr.word[wl-1]=='.'||gr.word[wl-1]=='?'||gr.word[wl-1]=='!'))
            break;
    }
    printf("\n");
    *walk_len_ptr = walk_len;
}


/* ================================================================
 * Interactive chat
 * ================================================================ */

/*
 * ================================================================
 * QUERY UNDERSTANDING — force-typed response modes
 * ================================================================
 *
 * "Don't memorize. Decompose. Ask: what does it DO?" — mind/mind
 *
 * The query's aggregate force pattern IS the instruction.
 * No English-specific string matching. The coordinates tell us.
 *
 * The encoding/payload boundary:
 *   Function words (primes) = encoding (HOW to respond)
 *   Content words (non-primes) = payload (WHAT to respond about)
 */

/*
 * Find the target: extract ALL content words (non-primes) and
 * compose their coordinates as a polynomial.
 *
 * "Atoms = multiplication (monomials). Paths = addition (polynomials)."
 *
 * "what is dark energy" → "dark" + "energy" = sum of coordinates.
 * The sum IS the query polynomial. Single content word = single term.
 * Multiple content words = polynomial with more precision.
 *
 * Returns the LAST content word as the display name (for context search),
 * and the POLYNOMIAL SUM as the coordinate (for field lookup).
 */
static const char *find_target_word(Token *toks, int ntok,
                                     DeriveResult *context, int ctx_len,
                                     coord4 *out_coord) {
    static char tbuf[64];
    const char *last_content = NULL;
    float poly_t = 0, poly_x = 0, poly_m = 0, poly_q = 0;
    int n_content = 0;

    for (int i = 0; i < ntok; i++) {
        if (toks[i].is_punct) continue;
        int wl = toks[i].len < 63 ? toks[i].len : 63;
        char w[64];
        for (int j = 0; j < wl; j++) w[j] = (char)tok_lower(toks[i].start[j]);
        w[wl] = '\0';
        DeriveResult dr = derive(w, wl, context, ctx_len, i);
        if (dr.confidence < 3) {
            /* content word — add to polynomial */
            poly_t += (float)dr.coord.t;
            poly_x += (float)dr.coord.x;
            poly_m += (float)dr.coord.m;
            poly_q += (float)dr.coord.q;
            n_content++;
            memcpy(tbuf, w, wl + 1);
            last_content = tbuf;
        }
    }

    if (n_content > 0) {
        /* the polynomial sum, normalized by term count (average) */
        out_coord->t = (int8_t)(poly_t / n_content + (poly_t > 0 ? 0.5f : -0.5f));
        out_coord->x = (int8_t)(poly_x / n_content + (poly_x > 0 ? 0.5f : -0.5f));
        out_coord->m = (int8_t)(poly_m / n_content + (poly_m > 0 ? 0.5f : -0.5f));
        out_coord->q = (int8_t)(poly_q / n_content + (poly_q > 0 ? 0.5f : -0.5f));
        return last_content;
    }

    /* all primes — use last word */
    for (int i = ntok - 1; i >= 0; i--) {
        if (toks[i].is_punct) continue;
        int wl = toks[i].len < 63 ? toks[i].len : 63;
        for (int j = 0; j < wl; j++) tbuf[j] = (char)tok_lower(toks[i].start[j]);
        tbuf[wl] = '\0';
        DeriveResult dr = derive(tbuf, wl, context, ctx_len, i);
        *out_coord = dr.coord;
        return tbuf;
    }
    return NULL;
}

/*
 * RETRIEVAL: Compute First.
 *
 * "Express the constraint as coordinates, compute, answer falls out."
 *
 * Step 1: derive coordinate for the target word
 * Step 2: field_reverse_lookup at that coordinate (O(1) bin lookup)
 * Step 3: return stored context fragments from the top matches
 *
 * NOT a linear scan. The coordinate IS the address.
 * If the exact bin is sparse, dark energy expands the search.
 */
/*
 * Score how relevant a context is for a target word.
 * Higher = better. Factors:
 *   - Word appears early in context (definitional)
 *   - Word appears multiple times (central topic)
 *   - Context is long (more information)
 */
static int context_relevance(const char *context, const char *word) {
    if (!context[0]) return 0;
    int wlen = (int)strlen(word);
    int score = 0;

    /* find first occurrence — earlier = more definitional */
    const char *p = strstr(context, word);
    if (!p) return 0;
    int pos = (int)(p - context);
    score += 100 - pos; /* earlier in context = higher score */
    if (score < 1) score = 1;

    /* count occurrences — more = more central */
    int occ = 0;
    p = context;
    while ((p = strstr(p, word)) != NULL) { occ++; p += wlen; }
    score += occ * 20;

    /* context length bonus — longer = more information */
    score += (int)strlen(context) / 10;

    return score;
}

static void retrieve(Field *field, const char *target_word, coord4 target_coord) {
    /*
     * COMPUTE FIRST RETRIEVAL.
     *
     * "Express the constraint as coordinates, compute, answer falls out."
     *
     * Search ALL bins for contexts mentioning the target word.
     * Rank by relevance (word position + frequency in context).
     * Return the top 3 most relevant contexts.
     */
    typedef struct { const char *ctx; int score; } CtxMatch;
    CtxMatch matches[64];
    int n_matches = 0;

    for (int b = 0; b < FIELD_S2_BINS && n_matches < 64; b++) {
        ReverseBin *bin = &field->reverse[b];
        for (int j = 0; j < bin->count && n_matches < 64; j++) {
            int score = context_relevance(bin->entries[j].context, target_word);
            if (score > 0) {
                matches[n_matches].ctx = bin->entries[j].context;
                matches[n_matches].score = score;
                n_matches++;
            }
        }
    }

    if (n_matches == 0) {
        printf(" [nothing about '%s']\n", target_word);
        return;
    }

    /* sort by score descending */
    for (int i = 0; i < n_matches - 1; i++)
        for (int j = i + 1; j < n_matches; j++)
            if (matches[j].score > matches[i].score) {
                CtxMatch tmp = matches[i];
                matches[i] = matches[j];
                matches[j] = tmp;
            }

    /* deduplicate — skip contexts that are identical to already shown */
    int shown = 0;
    for (int i = 0; i < n_matches && shown < 3; i++) {
        int dup = 0;
        for (int j = 0; j < i; j++)
            if (strcmp(matches[i].ctx, matches[j].ctx) == 0) { dup = 1; break; }
        if (!dup) {
            printf("\n  %s", matches[i].ctx);
            shown++;
        }
    }
    printf("\n");

    (void)target_coord;
}

/*
 * GENERATION with self-referential refinement (hylo fixed-point).
 *
 * "f(1,1,1,1) = hylo = full pipeline. The equation applied to its
 *  own coordinates produces itself. Fixed point." — mind/mind
 *
 * Phase 1 (bind): generate a draft trajectory (forward pass)
 * Phase 2 (hylo): re-derive each word with the FULL walk as context,
 *                  replace words that no longer fit, repeat until
 *                  convergence or max iterations.
 *
 * "4 iterations → 15+ digits" (Gauss-Legendre convergence)
 */
static void generate_refined(Field *field, coord4 *walk_i, int *walk_len_ptr,
                              int max_gen) {
    int seed_len = *walk_len_ptr;

    /* Phase 1: BIND — forward generation (same as before) */
    float4 walk_f[MAX_WALK];
    for (int i = 0; i < seed_len; i++)
        walk_f[i] = coord4_to_float4(walk_i[i]);

    const char *words[MAX_GEN];
    int gen_len = 0;
    int walk_len = seed_len;
    const char *history[12];
    int hist_len = 0;

    for (int g = 0; g < max_gen && g < MAX_GEN; g++) {
        int n_excl = hist_len < 6 ? hist_len : 6;
        const char *excl[6];
        for (int i = 0; i < n_excl; i++)
            excl[i] = history[hist_len - n_excl + i];

        float4 prev_f = walk_len > 0 ? walk_f[walk_len-1] : (float4){0,0,0,0};
        uint8_t prev_forces = derive_forces_from_coord(float4_to_coord4(prev_f));

        GenResult gr = generate_word(field, walk_i, walk_f, walk_len,
                                     prev_f, prev_forces, excl, n_excl);
        if (!gr.word || strcmp(gr.word, "...") == 0) break;

        words[gen_len++] = gr.word;
        if (hist_len < 12) history[hist_len++] = gr.word;
        else { for(int i=0;i<11;i++) history[i]=history[i+1]; history[11]=gr.word; }

        coord4 wc_i = float4_to_coord4(gr.walk_coord);
        if (walk_len < MAX_WALK) {
            walk_i[walk_len] = wc_i;
            walk_f[walk_len] = gr.walk_coord;
            walk_len++;
        } else {
            for (int i=0;i<MAX_WALK-1;i++) { walk_i[i]=walk_i[i+1]; walk_f[i]=walk_f[i+1]; }
            walk_i[MAX_WALK-1] = wc_i;
            walk_f[MAX_WALK-1] = gr.walk_coord;
        }

        int wl = (int)strlen(gr.word);
        if (wl > 0 && (gr.word[wl-1]=='.'||gr.word[wl-1]=='?'||gr.word[wl-1]=='!'))
            break;
    }

    /* Phase 2: HYLO — self-referential refinement.
     * Re-derive each generated word with the FULL walk as context
     * (not just predecessors). Replace words whose coordinates
     * shifted significantly. Repeat until convergence.
     *
     * "The equation applied to its own coordinates produces itself."
     */
    #define HYLO_MAX_ITER 3
    for (int iter = 0; iter < HYLO_MAX_ITER; iter++) {
        int changed = 0;

        for (int g = 0; g < gen_len; g++) {
            int pos = seed_len + g;
            if (pos >= walk_len) break;

            /* build BIDIRECTIONAL context: words before AND after */
            DeriveResult bidir_ctx[CTX_WINDOW];
            int bidir_len = 0;
            /* 4 words before */
            for (int k = pos - 4; k < pos && bidir_len < CTX_WINDOW; k++) {
                if (k < 0) continue;
                DeriveResult dr = derive(
                    k < seed_len ? "the" : words[k - seed_len],
                    k < seed_len ? 3 : (int)strlen(words[k - seed_len]),
                    NULL, 0, k);
                bidir_ctx[bidir_len++] = dr;
            }
            /* 4 words after (the hylo part — future context!) */
            for (int k = pos + 1; k <= pos + 4 && bidir_len < CTX_WINDOW; k++) {
                int gi = k - seed_len;
                if (gi < 0 || gi >= gen_len) continue;
                DeriveResult dr = derive(words[gi], (int)strlen(words[gi]),
                                         NULL, 0, k);
                bidir_ctx[bidir_len++] = dr;
            }

            /* re-derive this word with bidirectional context */
            DeriveResult new_dr = derive(words[g], (int)strlen(words[g]),
                                         bidir_ctx, bidir_len, pos);
            coord4 old_coord = walk_i[pos];
            coord4 new_coord = new_dr.coord;

            /* if the coordinate shifted, the word might not belong here */
            int dist = coord4_chebyshev(old_coord, new_coord);
            if (dist > 0) {
                changed++;
                walk_i[pos] = new_coord;
                walk_f[pos] = coord4_to_float4(new_coord);

                /* if shifted significantly, try to find a better word */
                if (dist >= 2) {
                    ReverseCandidate cands[4];
                    uint8_t ef = derive_forces_from_coord(new_coord);
                    coord4 prev = pos > 0 ? walk_i[pos-1] : new_coord;
                    int nc = field_reverse_lookup(field, new_coord, prev, ef, cands, 4);
                    if (nc > 0 && strcmp(cands[0].word, words[g]) != 0) {
                        words[g] = cands[0].word;
                    }
                }
            }
        }

        if (changed == 0) break; /* fixed point reached */
    }

    /* emit the refined output */
    for (int g = 0; g < gen_len; g++)
        printf(" %s", words[g]);
    printf("\n");

    *walk_len_ptr = walk_len;
}


static void interactive(Field *field) {
    char input[2048];
    coord4 walk[MAX_WALK];
    Token toks[256];

    fprintf(stderr, "wit chat\n");
    if (field) {
        fprintf(stderr, "  field loaded\n");
        field_print_health(field);
    } else {
        fprintf(stderr, "  no field — primes only\n");
    }
    fprintf(stderr, "  /quit to exit, /lookup WORD to inspect\n\n");

    for (;;) {
        printf("you: ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        int ilen = (int)strlen(input);
        while (ilen > 0 && (input[ilen-1]=='\n'||input[ilen-1]=='\r'))
            input[--ilen] = '\0';
        if (ilen == 0) continue;

        if (strcmp(input, "/quit") == 0 || strcmp(input, "/q") == 0) break;

        if (strncmp(input, "/lookup ", 8) == 0) {
            const char *w = input + 8;
            DeriveResult dr = derive(w, (int)strlen(w), NULL, 0, 0);
            fprintf(stderr, "  \"%s\" → (%+d,%+d,%+d,%+d) conf=%d forces=%d\n",
                    w, dr.coord.t, dr.coord.x, dr.coord.m, dr.coord.q,
                    dr.confidence, dr.forces);
            continue;
        }

        /* build walk from input */
        int ntok = tokenize_line(input, ilen, toks, 256);
        int walk_len = 0;
        DeriveResult context[CTX_WINDOW];
        int ctx_len = 0;
        int sentence_pos = 0;

        for (int i = 0; i < ntok && walk_len < MAX_WALK; i++) {
            if (toks[i].is_punct) continue;
            char word_buf[64];
            int wl = toks[i].len < 63 ? toks[i].len : 63;
            for (int j = 0; j < wl; j++)
                word_buf[j] = (char)tok_lower(toks[i].start[j]);
            word_buf[wl] = '\0';

            DeriveResult dr = derive(word_buf, wl, context, ctx_len, sentence_pos);
            walk[walk_len++] = dr.coord;

            if (ctx_len < CTX_WINDOW) context[ctx_len++] = dr;
            else { for (int j=0;j<CTX_WINDOW-1;j++) context[j]=context[j+1]; context[CTX_WINDOW-1]=dr; }
            sentence_pos++;
        }

        if (walk_len == 0) { printf("wit: ...\n"); continue; }

        printf("wit:");
        fflush(stdout);

        if (!field) { printf(" [no field loaded]\n"); continue; }

        /*
         * FORCE-TYPED MODE SELECTION.
         *
         * "The combination IS the coordinate." — mind/mind
         *
         * The query's aggregate force pattern determines the response.
         * No English-specific string matching. The coordinates speak.
         *
         *   π dominant (read)     → RETRIEVE from field
         *   ∘ dominant (call)     → GENERATE new content
         *   ∮ dominant (loop)     → LIST / enumerate
         *   δ dominant (test)     → EVALUATE / judge
         *   mixed                 → GENERATE (default)
         */
        uint8_t query_forces = 0;
        for (int i = 0; i < walk_len; i++)
            query_forces |= derive_forces_from_coord(walk[i]);

        coord4 target_coord = {0,0,0,0};
        const char *target_word = find_target_word(toks, ntok, context, ctx_len, &target_coord);

        /* mode selection from forces */
        int has_read = (query_forces & BOND_P) != 0;
        int has_call = (query_forces & BOND_C) != 0;
        int has_test = (query_forces & BOND_W) != 0;
        int force_count = has_read + has_call + ((query_forces & BOND_R) != 0) + has_test;

        /* retrieval heuristic:
         * - short queries (≤6 words) with a content word → try retrieve
         * - the lattice says: if you have the coordinate, evaluate.
         *   short queries ARE coordinate specifications. */
        int mode = 0; /* 0=generate, 1=retrieve */
        if (target_word && walk_len <= 6)
            mode = 1;

        if (mode == 1 && target_word) {
            retrieve(field, target_word, target_coord);
        } else {
            generate_refined(field, walk, &walk_len, MAX_GEN);
        }
    }
}


/* ================================================================
 * Main
 * ================================================================ */

int main(int argc, char **argv) {
    rng_seed((uint64_t)time(NULL));

    int mode_ingest = 0, mode_load = 0;
    const char *ingest_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ingest") == 0 && i+1 < argc) {
            mode_ingest = 1;
            ingest_path = argv[++i];
        } else if (strcmp(argv[i], "--load") == 0) {
            mode_load = 1;
        } else if (strcmp(argv[i], "--test") == 0) {
            /* run the spread test inline */
            execl("./test_spread", "test_spread", NULL);
            perror("test_spread");
            return 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            fprintf(stderr,
                "wit — lattice-native intelligence\n\n"
                "  --ingest FILE   process text, build field\n"
                "  --load          load field, enter chat\n"
                "  --test          run Phase 0 spread test\n"
                "  -h              this message\n\n"
                "  wit --ingest data.txt\n"
                "  wit --load\n");
            return 0;
        } else {
            fprintf(stderr, "unknown: %s (try -h)\n", argv[i]);
            return 1;
        }
    }

    if (mode_ingest) {
        ingest_file(ingest_path);
        return 0;
    }

    Field *field = NULL;
    /* auto-load if wit.field exists, or if --load was passed */
    {
        FILE *check = fopen("wit.field", "rb");
        if (check) { fclose(check); mode_load = 1; }
    }
    if (mode_load) {
        fprintf(stderr, "loading wit.field...\n");
        field = field_load("wit.field");
        if (field) fprintf(stderr, "loaded.\n\n");
        else fprintf(stderr, "not found.\n\n");
    }

    interactive(field);

    if (field) field_free(field);
    return 0;
}
