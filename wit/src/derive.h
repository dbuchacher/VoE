#ifndef DERIVE_H
#define DERIVE_H

/*
 * derive.h -- coordinate derivation for the WIT system
 *
 * PURE FUNCTION.  No state.  No storage.  No side effects.
 *
 * Given a word, its context, and sentence position, returns
 * a 4D coordinate and a confidence level.
 *
 * Priority chain (strict first-match):
 *   3  prime lookup     — 65 fixed anchors in a hash table
 *   2  morpheme decomp  — strip affixes, recurse to root
 *   1  walk position    — bond pattern from high-confidence predecessor
 *   0  char-level prior — sign-of-sum with 33% threshold
 *
 * The confidence field prevents low-quality guesses from
 * cascading through the context window.
 */

#include "wave.h"
#include "bonds.h"

#include <string.h>


/* ================================================================
 * DeriveResult — coordinate + confidence
 * ================================================================ */

typedef struct {
    coord4  coord;
    uint8_t confidence;  /* 0=char-level, 1=position, 2=morpheme, 3=prime */
} DeriveResult;


/* ================================================================
 * Prime lookup table — 128-slot open-addressed hash (FNV-1a)
 * ================================================================
 *
 * 65 single-word primes at ~50% load factor.
 * Coordinates from learner.c seed_primes() + pronouns.
 *
 * Field names: .t=tau, .x=chi, .m=mu, .q=phi.
 * (The side-quest files used .d for chi; we renamed to .x.)
 */

#define DERIVE_PRIME_SLOTS 128
#define DERIVE_PRIME_MASK  (DERIVE_PRIME_SLOTS - 1)

typedef struct {
    const char *word;
    int8_t      len;
    coord4      coord;
} DeriveHashEntry;

/*
 * FNV-1a hash for short ASCII strings.
 */
static inline uint32_t derive_fnv1a(const char *s, int len) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++) {
        h ^= (uint8_t)s[i];
        h *= 16777619u;
    }
    return h;
}

/*
 * Build the prime table at first use.
 *
 * 65 entries: 59 NSM primes from learner.c + 5 pronouns
 * (he, she, it, we, they) + the/a (already in seed_primes).
 */
static const DeriveHashEntry *derive_prime_table(void) {
    static DeriveHashEntry tbl[DERIVE_PRIME_SLOTS];
    static int built = 0;
    if (built) return tbl;

    memset(tbl, 0, sizeof(tbl));

    /* Seed list: word, t, x, m, q */
    static const struct { const char *w; int8_t t, x, m, q; } seeds[] = {

        /* 1. Substantives -- WHO/WHAT (mu axis) */
        { "i",         0,  0, +1, +1 },
        { "you",       0,  0, +1, -1 },
        { "someone",   0,  0, +1,  0 },
        { "something", 0,  0, +1,  0 },
        { "people",    0,  0, +2,  0 },
        { "body",      0, +1, +1,  0 },

        /* 2. Determiners -- REFERENCE (phi axis) */
        { "this",      0, +1,  0, +1 },
        { "same",      0,  0,  0, +1 },
        { "other",     0,  0,  0, -1 },

        /* 3. Quantifiers -- HOW MUCH (mu magnitude) */
        { "one",       0,  0, +1,  0 },
        { "two",       0,  0, +2,  0 },
        { "some",      0,  0, +1, -1 },
        { "all",       0,  0, +2, +1 },
        { "much",      0,  0, +2,  0 },
        { "more",      0,  0, +1, +1 },

        /* 4. Evaluators -- GOOD/BAD (phi axis) */
        { "good",      0,  0,  0, +1 },
        { "bad",       0,  0,  0, -1 },
        { "true",      0,  0,  0, +1 },

        /* 5. Descriptors -- SIZE (mu magnitude) */
        { "big",       0,  0, +2,  0 },
        { "small",     0,  0, -2,  0 },

        /* 6. Mental predicates -- INNER STATES */
        { "think",    +1,  0, +1, +1 },
        { "know",      0,  0, +1, +1 },
        { "want",     +1,  0, +1, +1 },
        { "feel",      0,  0, +1, +1 },
        { "see",       0, +1,  0, +1 },
        { "hear",      0, +1,  0, +1 },

        /* 7. Speech -- COMMUNICATION */
        { "say",      +1,  0,  0, +1 },

        /* 8. Actions -- CHANGE (tau axis) */
        { "do",       +1,  0,  0,  0 },
        { "happen",   +1,  0,  0,  0 },
        { "move",     +1, +1,  0,  0 },

        /* 9. Existence -- BEING */
        { "is",        0,  0, +1, +1 },
        { "was",      +1,  0, +1, +1 },
        { "have",      0,  0, +1, +1 },

        /* 10. Life (tau + mu) */
        { "live",     +1,  0, +1,  0 },
        { "die",      -1,  0, -1,  0 },

        /* 11. Motion / Transfer */
        { "go",       +1, +1,  0,  0 },
        { "come",     +1, -1,  0,  0 },
        { "make",     +1,  0, +1,  0 },
        { "take",     +1, +1, -1,  0 },
        { "give",     +1, +1, +1,  0 },

        /* 12. Time -- WHEN (tau axis) */
        { "now",      +1,  0,  0,  0 },
        { "before",   +2,  0,  0,  0 },
        { "after",    -2,  0,  0,  0 },

        /* 13. Place -- WHERE (chi axis) */
        { "where",     0, +1,  0, +1 },
        { "here",      0, +1,  0, +1 },
        { "above",     0, +2,  0,  0 },
        { "below",     0, -2,  0,  0 },
        { "in",        0, -1,  0,  0 },
        { "on",        0, +1,  0,  0 },

        /* 14. Logic -- STRUCTURE (phi axis) */
        { "not",       0,  0,  0, -1 },
        { "if",        0,  0,  0, +1 },
        { "because",  +1, +1,  0, +1 },
        { "can",      +1,  0,  0, +1 },

        /* 15. Degree -- MODIFIERS */
        { "very",      0,  0,  0, +2 },

        /* 16. Connectives -- LOGIC GATES */
        { "and",       0,  0,  0, +1 },
        { "but",       0,  0,  0, -1 },
        { "or",        0,  0,  0, +1 },

        /* 17. Articles -- DETERMINERS (chi+phi) */
        { "the",       0, +1,  0, +1 },
        { "a",         0, +1,  0, -1 },

        /* 18. Pronouns -- SUBSTANCE+REFERENCE (mu+phi) */
        { "he",        0,  0, +1, +1 },
        { "she",       0,  0, +1, +1 },
        { "it",        0,  0, +1, -1 },
        { "we",        0,  0, +2, +1 },
        { "they",      0,  0, +2, -1 },

        { NULL, 0, 0, 0, 0 }
    };

    for (int i = 0; seeds[i].w; i++) {
        int len = (int)strlen(seeds[i].w);
        uint32_t h = derive_fnv1a(seeds[i].w, len) & DERIVE_PRIME_MASK;

        /* linear probing */
        for (int j = 0; j < DERIVE_PRIME_SLOTS; j++) {
            int slot = (int)((h + (uint32_t)j) & DERIVE_PRIME_MASK);
            if (!tbl[slot].word) {
                tbl[slot].word = seeds[i].w;
                tbl[slot].len  = (int8_t)len;
                tbl[slot].coord.t = seeds[i].t;
                tbl[slot].coord.x = seeds[i].x;
                tbl[slot].coord.m = seeds[i].m;
                tbl[slot].coord.q = seeds[i].q;
                break;
            }
            /* skip duplicates (same word already inserted) */
            if (tbl[slot].len == len &&
                memcmp(tbl[slot].word, seeds[i].w, (size_t)len) == 0)
                break;
        }
    }

    built = 1;
    return tbl;
}

/*
 * Look up a word in the prime table.
 * Returns NULL if not found.
 */
static inline const DeriveHashEntry *derive_prime_lookup(const char *word, int len) {
    const DeriveHashEntry *tbl = derive_prime_table();
    uint32_t h = derive_fnv1a(word, len) & DERIVE_PRIME_MASK;

    for (int j = 0; j < DERIVE_PRIME_SLOTS; j++) {
        int slot = (int)((h + (uint32_t)j) & DERIVE_PRIME_MASK);
        if (!tbl[slot].word) return NULL;  /* empty slot = miss */
        if (tbl[slot].len == len &&
            memcmp(tbl[slot].word, word, (size_t)len) == 0)
            return &tbl[slot];
    }
    return NULL;
}


/* ================================================================
 * Morpheme decomposition — suffix/prefix rules from morpheme.h
 * ================================================================ */

typedef struct {
    const char *suffix;
    int         len;
    int8_t      dt, dx, dm, dq;
    int         min_root;
} DeriveSuffixRule;

static const DeriveSuffixRule derive_suffix_rules[] = {
    /* -tion/-sion: verb -> noun (add mu) */
    { "ation",  5,   0, 0, +1, 0,   3 },
    { "ition",  5,   0, 0, +1, 0,   3 },
    { "ution",  5,   0, 0, +1, 0,   3 },
    { "tion",   4,   0, 0, +1, 0,   2 },
    { "sion",   4,   0, 0, +1, 0,   2 },

    /* -ness: adj -> noun (add mu) */
    { "ness",   4,   0, 0, +1, 0,   3 },

    /* -ment: verb -> noun (add mu) */
    { "ment",   4,   0, 0, +1, 0,   3 },

    /* -able/-ible: verb -> adj (add phi) */
    { "able",   4,   0, 0,  0, +1,  3 },
    { "ible",   4,   0, 0,  0, +1,  3 },

    /* -ful/-less: noun -> adj (+-phi) */
    { "less",   4,   0, 0,  0, -1,  3 },
    { "ful",    3,   0, 0,  0, +1,  3 },

    /* -ous/-ious: noun -> adj (add phi) */
    { "ious",   4,   0, 0,  0, +1,  3 },
    { "ous",    3,   0, 0,  0, +1,  3 },

    /* -ing: ongoing (add tau + mu for loop) */
    { "ning",   4,  +1, 0, +1, 0,   3 },
    { "ting",   4,  +1, 0, +1, 0,   3 },
    { "ing",    3,  +1, 0, +1, 0,   2 },

    /* -er/-or: verb -> agent noun (add mu) */
    { "ier",    3,   0, 0, +1, 0,   3 },
    { "er",     2,   0, 0, +1, 0,   2 },
    { "or",     2,   0, 0, +1, 0,   2 },

    /* -ist: noun -> person (add mu+phi) */
    { "ist",    3,   0, 0, +1, +1,  3 },

    /* -ly: adj/verb -> adverb (add phi) */
    { "ly",     2,   0, 0,  0, +1,  3 },

    /* -ed: past tense (add tau) */
    { "ied",    3,  +1, 0,  0, 0,   2 },
    { "ed",     2,  +1, 0,  0, 0,   2 },

    /* -ize/-ise: noun/adj -> verb (add tau) */
    { "ize",    3,  +1, 0,  0, 0,   3 },
    { "ise",    3,  +1, 0,  0, 0,   3 },

    /* -ify: noun/adj -> verb (add tau) */
    { "ify",    3,  +1, 0,  0, 0,   3 },

    /* -al/-ial: noun -> adj (add phi) */
    { "ial",    3,   0, 0,  0, +1,  3 },
    { "al",     2,   0, 0,  0, +1,  3 },

    /* -ity: adj -> noun (add mu) */
    { "ity",    3,   0, 0, +1, 0,   3 },

    /* -es/-s: plural (add mu) -- last because short */
    { "ies",    3,   0, 0, +1, 0,   2 },
    { "es",     2,   0, 0, +1, 0,   3 },
    { "s",      1,   0, 0, +1, 0,   3 },

    { NULL, 0, 0, 0, 0, 0, 0 }
};

typedef struct {
    const char *prefix;
    int         len;
    int8_t      dt, dx, dm, dq;
    int         q_flip;
    int         all_flip;
    int         min_root;
} DerivePrefixRule;

static const DerivePrefixRule derive_prefix_rules[] = {
    /* negation prefixes: flip phi */
    { "un",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "dis",    3,   0, 0, 0, 0,   1, 0,  3 },
    { "im",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "in",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "ir",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "il",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "non",    3,   0, 0, 0, 0,   1, 0,  3 },

    /* anti-: flip all signs */
    { "anti",   4,   0, 0, 0, 0,   0, 1,  3 },

    /* mis-: flip phi (wrong quality) */
    { "mis",    3,   0, 0, 0, 0,   1, 0,  3 },

    /* temporal prefixes */
    { "re",     2,  +1, 0, 0, 0,   0, 0,  3 },
    { "pre",    3,  +2, 0, 0, 0,   0, 0,  3 },
    { "post",   4,  -2, 0, 0, 0,   0, 0,  3 },

    /* spatial prefixes */
    { "out",    3,   0,+1, 0, 0,   0, 0,  3 },
    { "inter",  5,   0,+1, 0, 0,   0, 0,  3 },
    { "trans",  5,   0,+2, 0, 0,   0, 0,  3 },
    { "sub",    3,   0,-1, 0, 0,   0, 0,  3 },
    { "super",  5,   0,+2, 0,+1,   0, 0,  3 },

    /* substance prefixes */
    { "co",     2,   0, 0,+1, 0,   0, 0,  3 },

    /* magnitude prefixes */
    { "over",   4,   0, 0,+1, 0,   0, 0,  3 },
    { "under",  5,   0, 0,-1, 0,   0, 0,  3 },

    { NULL, 0, 0, 0, 0, 0, 0, 0, 0 }
};

/*
 * String comparison helper (no null terminator needed).
 */
static inline int derive_streq(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

/*
 * Clamp int to int8_t range.
 */
static inline int8_t derive_clamp8(int v) {
    if (v > 127)  return 127;
    if (v < -128) return -128;
    return (int8_t)v;
}


/* ================================================================
 * Bond default regions — 16 entries from learner.c
 * ================================================================
 *
 * The bond between the previous word and the current position
 * gives a 4-bit mask (0-15).  Index into this table to get the
 * expected dimensional bias for the current word.
 */

static const coord4 derive_bond_regions[16] = {
    /*  0: identity    -- no change, same region                   */
    { .t =  0, .x =  0, .m =  0, .q =  0 },
    /*  1: P (force)   -- tau activated, verb/action               */
    { .t = +1, .x =  0, .m =  0, .q =  0 },
    /*  2: C (path)    -- chi activated, spatial/preposition       */
    { .t =  0, .x = +1, .m =  0, .q =  0 },
    /*  3: P+C (trav)  -- tau+chi, motion verb (run, go, come)    */
    { .t = +1, .x = +1, .m =  0, .q =  0 },
    /*  4: R (cycle)   -- mu activated, noun/substance             */
    { .t =  0, .x =  0, .m = +1, .q =  0 },
    /*  5: P+R (full)  -- tau+mu, temporal substance (live, grow) */
    { .t = +1, .x =  0, .m = +1, .q =  0 },
    /*  6: C+R (iter)  -- chi+mu, spatial substance (place-noun)  */
    { .t =  0, .x = +1, .m = +1, .q =  0 },
    /*  7: P+C+R (acc) -- tau+chi+mu, complex (give, take, make)  */
    { .t = +1, .x = +1, .m = +1, .q =  0 },
    /*  8: W (link)    -- phi activated, adjective/evaluator      */
    { .t =  0, .x =  0, .m =  0, .q = +1 },
    /*  9: P+W (contact) -- tau+phi, perception (see, hear)       */
    { .t = +1, .x =  0, .m =  0, .q = +1 },
    /* 10: C+W (enable)  -- chi+phi, determiner/conditional       */
    { .t =  0, .x = +1, .m =  0, .q = +1 },
    /* 11: P+C+W (part)  -- tau+chi+phi, transformation verb      */
    { .t = +1, .x = +1, .m =  0, .q = +1 },
    /* 12: R+W (scale)   -- mu+phi, pronoun/quantifier            */
    { .t =  0, .x =  0, .m = +1, .q = +1 },
    /* 13: P+R+W (near)  -- tau+mu+phi, progressive (mental verb) */
    { .t = +1, .x =  0, .m = +1, .q = +1 },
    /* 14: C+R+W (bal)   -- chi+mu+phi, causation (because)       */
    { .t =  0, .x = +1, .m = +1, .q = +1 },
    /* 15: P+C+R+W (con) -- all active, hylo (think, full)        */
    { .t = +1, .x = +1, .m = +1, .q = +1 },
};


/* ================================================================
 * Position default — phi->mu->tau->mu->chi (5-slot cycle)
 * ================================================================
 *
 * When no reliable predecessor (confidence <= 1), use the
 * sentence position to pick a default bias.  Matches ~45%
 * of English SVO structure.
 */

static const coord4 derive_pos_cycle[5] = {
    { .t =  0, .x =  0, .m =  0, .q = +1 },  /* phi:  determiner */
    { .t =  0, .x =  0, .m = +1, .q =  0 },  /* mu:   subject    */
    { .t = +1, .x =  0, .m =  0, .q =  0 },  /* tau:  verb       */
    { .t =  0, .x =  0, .m = +1, .q =  0 },  /* mu:   object     */
    { .t =  0, .x = +1, .m =  0, .q =  0 },  /* chi:  prep       */
};

static inline coord4 derive_position_default(int sentence_pos) {
    int idx = sentence_pos % 5;
    if (idx < 0) idx += 5;
    return derive_pos_cycle[idx];
}


/* ================================================================
 * Character-level prior — sign-of-sum with 33% threshold
 * ================================================================
 *
 * Sum wave byte coordinates per dimension.
 * If sum > count/3 -> +1, if sum < -count/3 -> -1, else 0.
 *
 * NOT average (collapses to zero).
 * NOT dominant dimension (collapses mu -> phi).
 * No mu=+1 fallback.  If phonetics are balanced, (0,0,0,0) is honest.
 */

static inline coord4 derive_char_prior(const char *word, int len) {
    int st = 0, sx = 0, sm = 0, sq = 0;
    int count = 0;

    for (int i = 0; i < len; i++) {
        uint8_t ch = (uint8_t)word[i];
        if (ch >= 'A' && ch <= 'Z') ch = (uint8_t)(ch + 32);
        uint8_t wb = utf8_to_wave(ch);
        if (wb != 0x00) {
            coord4 c = wave_decode(wb);
            st += c.t;
            sx += c.x;
            sm += c.m;
            sq += c.q;
            count++;
        }
    }

    coord4 r = { .t = 0, .x = 0, .m = 0, .q = 0 };
    if (count == 0) return r;

    int thr = (count + 2) / 3;
    r.t = (int8_t)(st > thr ? +1 : st < -thr ? -1 : 0);
    r.x = (int8_t)(sx > thr ? +1 : sx < -thr ? -1 : 0);
    r.m = (int8_t)(sm > thr ? +1 : sm < -thr ? -1 : 0);
    r.q = (int8_t)(sq > thr ? +1 : sq < -thr ? -1 : 0);
    /* No mu=+1 fallback.  Position bias handles the rest. */

    return r;
}


/* ================================================================
 * derive() — the core function
 * ================================================================
 *
 * word + context + position -> DeriveResult
 *
 * The word must be lowercase ASCII (caller's job to normalize).
 * Context is the preceding DeriveResults (up to 8 typically).
 * sentence_pos is 0-based position within the current sentence.
 */

static DeriveResult derive(const char *word, int len,
                           const DeriveResult *context, int ctx_len,
                           int sentence_pos)
{
    DeriveResult result;
    memset(&result, 0, sizeof(result));

    /* ── Priority 1: Prime lookup (confidence=3) ───────────── */

    const DeriveHashEntry *prime = derive_prime_lookup(word, len);
    if (prime) {
        result.coord = prime->coord;
        result.confidence = 3;
        return result;
    }

    /* ── Priority 2: Morpheme decomposition (confidence=2) ─── */

    if (len >= 3 && len <= 31) {
        const char *start = word;
        int remaining = len;
        int8_t dt = 0, dx = 0, dm = 0, dq = 0;
        int q_flip = 0, all_flip = 0;
        int found_affix = 0;

        /* try one prefix */
        for (int i = 0; derive_prefix_rules[i].prefix; i++) {
            const DerivePrefixRule *p = &derive_prefix_rules[i];
            if (remaining >= p->len + p->min_root &&
                derive_streq(start, p->prefix, p->len)) {
                start += p->len;
                remaining -= p->len;
                dt += p->dt;
                dx += p->dx;
                dm += p->dm;
                dq += p->dq;
                q_flip = p->q_flip;
                all_flip = p->all_flip;
                found_affix = 1;
                break;
            }
        }

        /* try one suffix */
        for (int i = 0; derive_suffix_rules[i].suffix; i++) {
            const DeriveSuffixRule *s = &derive_suffix_rules[i];
            if (remaining >= s->len + s->min_root &&
                derive_streq(start + remaining - s->len, s->suffix, s->len)) {
                remaining -= s->len;
                dt += s->dt;
                dx += s->dx;
                dm += s->dm;
                dq += s->dq;
                found_affix = 1;
                break;
            }
        }

        if (found_affix && remaining > 0 && remaining < len) {
            /*
             * Recurse to derive the root.
             * Pass the SAME context and sentence_pos.
             */
            DeriveResult root_result = derive(start, remaining,
                                              context, ctx_len,
                                              sentence_pos);

            /*
             * Only promote to confidence=2 if root was actually
             * found at some confidence (prime or further morpheme).
             * If root fell through to char-level, the morpheme
             * decomposition is speculative but still better than
             * raw char-level alone.
             */
            if (root_result.confidence >= 1) {
                coord4 c = root_result.coord;

                /* apply transforms */
                if (all_flip) {
                    c.t = (int8_t)-c.t;
                    c.x = (int8_t)-c.x;
                    c.m = (int8_t)-c.m;
                    c.q = (int8_t)-c.q;
                }
                if (q_flip) {
                    c.q = (int8_t)-c.q;
                }

                /* add delta, clamping */
                c.t = derive_clamp8((int)c.t + (int)dt);
                c.x = derive_clamp8((int)c.x + (int)dx);
                c.m = derive_clamp8((int)c.m + (int)dm);
                c.q = derive_clamp8((int)c.q + (int)dq);

                result.coord = c;
                result.confidence = 2;
                return result;
            }
        }
    }

    /* ── Priority 3: Walk position from bond pattern (conf=1) ─ */
    /*    ONLY if last context entry has confidence >= 2.        */

    if (ctx_len > 0 && context[ctx_len - 1].confidence >= 2) {
        coord4 prev = context[ctx_len - 1].coord;

        /*
         * The bond between prev coord and the position default
         * gives a 4-bit mask.  Index into bond_default_regions
         * to get the dimensional bias for this position.
         */
        coord4 pos_default = derive_position_default(sentence_pos);
        BondType bond = classify_bond(prev, pos_default);
        coord4 bias = derive_bond_regions[bond & 0xF];

        result.coord = bias;
        result.confidence = 1;
        return result;
    }

    /* ── Priority 4: Character-level prior (confidence=0) ──── */
    /*    Combined with position default.                       */

    coord4 char_coord = derive_char_prior(word, len);
    coord4 pos_coord  = derive_position_default(sentence_pos);

    /*
     * Combine: char_prior provides phonetic signal,
     * position default provides structural signal.
     * Use char_prior where nonzero, fall back to position default.
     */
    result.coord.t = char_coord.t ? char_coord.t : pos_coord.t;
    result.coord.x = char_coord.x ? char_coord.x : pos_coord.x;
    result.coord.m = char_coord.m ? char_coord.m : pos_coord.m;
    result.coord.q = char_coord.q ? char_coord.q : pos_coord.q;

    result.confidence = 0;
    return result;
}


#endif /* DERIVE_H */
