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
 * DeriveResult — coordinate + confidence + force pattern
 * ================================================================ */

typedef struct {
    coord4  coord;
    uint8_t confidence;  /* 0=char-level, 1=position, 2=morpheme, 3=prime */
    uint8_t forces;      /* 4-bit bond mask: what this word DOES
                          *   bit 0 (BOND_P) = reads/writes data
                          *   bit 1 (BOND_C) = calls something
                          *   bit 2 (BOND_R) = iterates/loops
                          *   bit 3 (BOND_W) = tests/evaluates
                          * Derived from "How To Think" (mind line 49):
                          *   "Don't memorize. Decompose. Ask: what does it DO?" */
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
    uint8_t     forces;   /* 4-bit bond mask: the word's force pattern */
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
 * Derive force pattern from a coordinate.
 *
 * For non-prime words, the force pattern comes from the dimensional
 * signature: which dimensions are active (nonzero) selects which
 * forces apply.  Even exponents = atom (pure math), odd = force.
 *
 * At shell 1 (|coord| <= 1), every nonzero dimension is odd,
 * so active = force.  At higher shells, even nonzero values
 * (|2|, |4|, ...) are atoms on that dimension — force is not
 * active there.
 */
static inline uint8_t derive_forces_from_coord(coord4 c) {
    uint8_t f = 0;
    /* Odd exponents = force active on that dimension */
    if (c.t != 0 && ((c.t < 0 ? -c.t : c.t) & 1)) f |= BOND_P;
    if (c.x != 0 && ((c.x < 0 ? -c.x : c.x) & 1)) f |= BOND_C;
    if (c.m != 0 && ((c.m < 0 ? -c.m : c.m) & 1)) f |= BOND_R;
    if (c.q != 0 && ((c.q < 0 ? -c.q : c.q) & 1)) f |= BOND_W;
    return f;
}

/*
 * Build the prime table at first use.
 *
 * 65 entries: 59 NSM primes from learner.c + 5 pronouns
 * (he, she, it, we, they) + the/a (already in seed_primes).
 *
 * The 'forces' field encodes what the word DOES per "How To Think":
 *   BOND_P (1) = reads/writes data  (pi)
 *   BOND_C (2) = calls something    (comp)
 *   BOND_R (4) = iterates/loops     (contour)
 *   BOND_W (8) = tests/evaluates    (delta)
 */
static const DeriveHashEntry *derive_prime_table(void) {
    static DeriveHashEntry tbl[DERIVE_PRIME_SLOTS];
    static int built = 0;
    if (built) return tbl;

    memset(tbl, 0, sizeof(tbl));

    /* Seed list: word, t, x, m, q, forces
     *
     * Force assignments from mind/mind "How To Think" + decomposition:
     *   P=1 (pi: reads/writes)  C=2 (call)  R=4 (loop)  W=8 (test)
     *
     * Verbs of action:    do/happen → P(1), move/go → P|C(3)
     * Verbs of state:     live → P|R(5), die → P|R(5)
     * Mental predicates:  think → P|C|R|W(15), know/feel → P|R|W(13)
     *                     see/hear → P|W(9), want → P|R|W(13)
     * Speech:             say → P|C(3)
     * Existence:          is/was/have → R(4)
     * Transfer:           give/take/make/come → P|C(3)
     * Substantives:       nouns/pronouns → R(4) (pure substance)
     * Determiners:        the/this/a → C|W(10) (conditional call)
     * Quantifiers:        one/two/some/all/much/more → R(4) (substance)
     * Evaluators:         good/bad/true → W(8) (test)
     * Descriptors:        big/small → W(8) (test)
     * Logic:              not/if/and/but/or → W(8) (test)
     * Time:               now/before/after → P(1) (temporal)
     * Place:              where/here/above/below/in/on → C(2) (spatial)
     * Degree:             very → W(8) (evaluative)
     * Connectives:        because → P|C|W(11), can → P|W(9)
     */
    static const struct {
        const char *w;
        int8_t t, x, m, q;
        uint8_t forces;
    } seeds[] = {

        /* 1. Substantives -- WHO/WHAT (mu axis) → R (substance) */
        { "i",         0,  0, +1, +1,   BOND_R },
        { "you",       0,  0, +1, -1,   BOND_R },
        { "someone",   0,  0, +1,  0,   BOND_R },
        { "something", 0,  0, +1,  0,   BOND_R },
        { "people",    0,  0, +2,  0,   BOND_R },
        { "body",      0, +1, +1,  0,   BOND_R },

        /* 2. Determiners -- REFERENCE → C|W (maybe: conditional call) */
        { "this",      0, +1,  0, +1,   BOND_C|BOND_W },
        { "same",      0,  0,  0, +1,   BOND_W },
        { "other",     0,  0,  0, -1,   BOND_W },

        /* 3. Quantifiers -- HOW MUCH → R (substance) */
        { "one",       0,  0, +1,  0,   BOND_R },
        { "two",       0,  0, +2,  0,   BOND_R },
        { "some",      0,  0, +1, -1,   BOND_R },
        { "all",       0,  0, +2, +1,   BOND_R },
        { "much",      0,  0, +2,  0,   BOND_R },
        { "more",      0,  0, +1, +1,   BOND_R },

        /* 4. Evaluators -- GOOD/BAD → W (test) */
        { "good",      0,  0,  0, +1,   BOND_W },
        { "bad",       0,  0,  0, -1,   BOND_W },
        { "true",      0,  0,  0, +1,   BOND_W },

        /* 5. Descriptors -- SIZE → W (test) */
        { "big",       0,  0, +2,  0,   BOND_W },
        { "small",     0,  0, -2,  0,   BOND_W },

        /* 6. Mental predicates -- INNER STATES */
        { "think",    +1,  0, +1, +1,   BOND_P|BOND_C|BOND_R|BOND_W }, /* hylo */
        { "know",      0,  0, +1, +1,   BOND_P|BOND_R|BOND_W },
        { "want",     +1,  0, +1, +1,   BOND_P|BOND_R|BOND_W },
        { "feel",      0,  0, +1, +1,   BOND_P|BOND_R|BOND_W },
        { "see",       0, +1,  0, +1,   BOND_P|BOND_W },   /* filter */
        { "hear",      0, +1,  0, +1,   BOND_P|BOND_W },   /* filter */

        /* 7. Speech -- COMMUNICATION → P|C (fold: read then call) */
        { "say",      +1,  0,  0, +1,   BOND_P|BOND_C },

        /* 8. Actions -- CHANGE → P (read/temporal) */
        { "do",       +1,  0,  0,  0,   BOND_P },
        { "happen",   +1,  0,  0,  0,   BOND_P },
        { "move",     +1, +1,  0,  0,   BOND_P|BOND_C },  /* fold */

        /* 9. Existence -- BEING → R (substance/loop) */
        { "is",        0,  0, +1, +1,   BOND_R },
        { "was",      +1,  0, +1, +1,   BOND_R },
        { "have",      0,  0, +1, +1,   BOND_R },

        /* 10. Life → P|R (slurp: read + loop) */
        { "live",     +1,  0, +1,  0,   BOND_P|BOND_R },
        { "die",      -1,  0, -1,  0,   BOND_P|BOND_R },

        /* 11. Motion / Transfer → P|C (fold: read + call) */
        { "go",       +1, +1,  0,  0,   BOND_P|BOND_C },
        { "come",     +1, -1,  0,  0,   BOND_P|BOND_C },
        { "make",     +1,  0, +1,  0,   BOND_P|BOND_R },
        { "take",     +1, +1, -1,  0,   BOND_P|BOND_C },
        { "give",     +1, +1, +1,  0,   BOND_P|BOND_C },

        /* 12. Time -- WHEN → P (temporal) */
        { "now",      +1,  0,  0,  0,   BOND_P },
        { "before",   +2,  0,  0,  0,   BOND_P },
        { "after",    -2,  0,  0,  0,   BOND_P },

        /* 13. Place -- WHERE → C (spatial) */
        { "where",     0, +1,  0, +1,   BOND_C },
        { "here",      0, +1,  0, +1,   BOND_C },
        { "above",     0, +2,  0,  0,   BOND_C },
        { "below",     0, -2,  0,  0,   BOND_C },
        { "in",        0, -1,  0,  0,   BOND_C },
        { "on",        0, +1,  0,  0,   BOND_C },

        /* 14. Logic -- STRUCTURE → W (test/delta) */
        { "not",       0,  0,  0, -1,   BOND_W },
        { "if",        0,  0,  0, +1,   BOND_W },
        { "because",  +1, +1,  0, +1,   BOND_P|BOND_C|BOND_W },
        { "can",      +1,  0,  0, +1,   BOND_P|BOND_W },

        /* 15. Degree -- MODIFIERS → W (test) */
        { "very",      0,  0,  0, +2,   BOND_W },

        /* 16. Connectives -- LOGIC GATES → W (test) */
        { "and",       0,  0,  0, +1,   BOND_W },
        { "but",       0,  0,  0, -1,   BOND_W },
        { "or",        0,  0,  0, +1,   BOND_W },

        /* 17. Articles -- DETERMINERS → C|W (maybe) */
        { "the",       0, +1,  0, +1,   BOND_C|BOND_W },
        { "a",         0, +1,  0, -1,   BOND_C|BOND_W },

        /* 18. Pronouns -- SUBSTANCE+REFERENCE → R (substance) */
        { "he",        0,  0, +1, +1,   BOND_R },
        { "she",       0,  0, +1, +1,   BOND_R },
        { "it",        0,  0, +1, -1,   BOND_R },
        { "we",        0,  0, +2, +1,   BOND_R },
        { "they",      0,  0, +2, -1,   BOND_R },

        { NULL, 0, 0, 0, 0, 0 }
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
                tbl[slot].forces  = seeds[i].forces;
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
 * Force-based position default — replaces the 10-slot cycle
 * ================================================================
 *
 * The lattice says sentence structure = bond sequences.
 * Instead of an ad-hoc 10-slot cycle, we predict the next word's
 * expected force pattern from sentence position using the simplest
 * lattice-motivated model:
 *
 *   Position 0: W (delta/test)    — determiners, conjunctions, signals
 *   Position 1: R (contour/loop)  — nouns (substance)
 *   Position 2: P (pi/temporal)   — verbs (action)
 *   Position 3+: alternate R, P   — noun-verb-noun-verb
 *
 * This uses FORCES (what it does) not DIMENSIONS (where it sits).
 * The force pattern maps to a coordinate bias via derive_bond_regions.
 *
 * Returns the expected force pattern (4-bit mask) for a given
 * sentence position.
 */
static inline uint8_t derive_position_forces(int sentence_pos) {
    /*
     * 8-slot cycle: 4 forward forces + 4 backward (antimatter).
     * Covers all 4 dimensions in both directions.
     *
     *   0: δ  (test/signal)   → φ+  (determiner)
     *   1: ∮  (substance)     → μ+  (noun)
     *   2: π  (temporal)      → τ+  (verb)
     *   3: ∘  (spatial)       → χ+  (preposition)
     *   4: δ̄  (guard)         → φ-  (negation)
     *   5: ∮̄  (absence)       → μ-  (void/nothing)
     *   6: π̄  (past/passive)  → τ-  (past participle)
     *   7: ∘̄  (inside/return) → χ-  (interior)
     */
    static const uint8_t cycle[8] = {
        BOND_W, BOND_R, BOND_P, BOND_C,
        BOND_W, BOND_R, BOND_P, BOND_C,
    };
    int idx = sentence_pos % 8;
    if (idx < 0) idx += 8;
    return cycle[idx];
}

/*
 * Position default coordinate: 8-slot cycle covering all 4 dimensions
 * in both positive (forward) and negative (antimatter) directions.
 * This replaces the bond_regions lookup for position defaults because
 * bond_regions is all-positive — it can't produce negative coordinates.
 */
static const coord4 derive_pos_defaults[8] = {
    { .t =  0, .x =  0, .m =  0, .q = +1 },  /* δ forward:  φ+  */
    { .t =  0, .x =  0, .m = +1, .q =  0 },  /* ∮ forward:  μ+  */
    { .t = +1, .x =  0, .m =  0, .q =  0 },  /* π forward:  τ+  */
    { .t =  0, .x = +1, .m =  0, .q =  0 },  /* ∘ forward:  χ+  */
    { .t =  0, .x =  0, .m =  0, .q = -1 },  /* δ̄ backward: φ-  */
    { .t =  0, .x =  0, .m = -1, .q =  0 },  /* ∮̄ backward: μ-  */
    { .t = -1, .x =  0, .m =  0, .q =  0 },  /* π̄ backward: τ-  */
    { .t =  0, .x = -1, .m =  0, .q =  0 },  /* ∘̄ backward: χ-  */
};

static inline coord4 derive_position_default(int sentence_pos) {
    int idx = sentence_pos % 8;
    if (idx < 0) idx += 8;
    return derive_pos_defaults[idx];
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
        result.forces = prime->forces;
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
                result.forces = derive_forces_from_coord(c);
                return result;
            }
        }
    }

    /* ── Priority 3: Walk position from bond pattern (conf=1) ─ */
    /*    ONLY if last context entry has confidence >= 2.        */

    if (ctx_len > 0 && context[ctx_len - 1].confidence >= 2) {
        coord4 prev = context[ctx_len - 1].coord;

        /*
         * ANTIMATTER PRINCIPLE: a bond means a dimension CHANGED.
         * If prev was positive on that dimension, the new word is
         * negative — the overbar direction.  pi-bar is the antimatter of pi.
         *
         * After "was" (tau=+1), if tau changes -> new word gets tau=-1
         * After "the" (phi=+1), if phi changes -> new word gets phi=-1
         * After "in"  (chi=-1), if chi changes -> new word gets chi=+1
         *
         * For ACTIVE dimensions (bond says changed): flip predecessor's sign.
         * For INACTIVE dimensions (bond says unchanged): inherit the
         * predecessor's ACTUAL VALUE, preserving magnitude.
         *
         * ZERO CASE: If prev is zero on a dimension and the bond says
         * it changed, flipping zero is meaningless.  Use the position
         * default for that dimension instead — it represents the
         * structural expectation for this sentence slot.
         */
        coord4 pos_default = derive_position_default(sentence_pos);
        BondType bond = classify_bond(prev, pos_default);

        coord4 bias;

        /* T dimension */
        if (bond & BOND_P) {
            /* active: flip, or use pos_default if prev is zero */
            bias.t = prev.t ? (int8_t)-prev.t : pos_default.t;
        } else {
            /* inactive: inherit predecessor's actual value */
            bias.t = prev.t;
        }

        /* X dimension */
        if (bond & BOND_C) {
            bias.x = prev.x ? (int8_t)-prev.x : pos_default.x;
        } else {
            bias.x = prev.x;
        }

        /* M dimension */
        if (bond & BOND_R) {
            bias.m = prev.m ? (int8_t)-prev.m : pos_default.m;
        } else {
            bias.m = prev.m;
        }

        /* Q dimension */
        if (bond & BOND_W) {
            bias.q = prev.q ? (int8_t)-prev.q : pos_default.q;
        } else {
            bias.q = prev.q;
        }

        result.coord = bias;
        result.confidence = 1;
        result.forces = derive_forces_from_coord(bias);
        return result;
    }

    /* ── Priority 4: Character-level prior (confidence=0) ──── */
    /*    Combined with position default.                       */
    /*                                                          */
    /*    ENCODING/PAYLOAD BOUNDARY:                            */
    /*    Position (payload/structure) has priority.             */
    /*    Char-level (encoding/phonetics) fills in only where   */
    /*    structure has no opinion.  This respects the boundary  */
    /*    from mind/mind: encoding must not leak into payload.   */

    coord4 char_coord = derive_char_prior(word, len);
    coord4 pos_coord  = derive_position_default(sentence_pos);

    /*
     * Position default = payload (structural expectation).
     * Char prior = encoding (phonetic signal).
     * Structure takes priority.  Phonetics fills gaps.
     */
    result.coord.t = pos_coord.t ? pos_coord.t : char_coord.t;
    result.coord.x = pos_coord.x ? pos_coord.x : char_coord.x;
    result.coord.m = pos_coord.m ? pos_coord.m : char_coord.m;
    result.coord.q = pos_coord.q ? pos_coord.q : char_coord.q;

    result.confidence = 0;
    result.forces = derive_forces_from_coord(result.coord);
    return result;
}


#endif /* DERIVE_H */
