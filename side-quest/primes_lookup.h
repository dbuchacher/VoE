#ifndef PRIMES_LOOKUP_H
#define PRIMES_LOOKUP_H

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "wave.h"

/*
 * primes_lookup.h — English words to 4D lattice coordinates
 *
 * Component 1.3: semantic prime decomposition lookup.
 *
 * Maps words to (T, D, M, Q) using Natural Semantic Metalanguage.
 * 65 universal primes have fixed coordinates (from primes.md).
 * Every word decomposes into primes; word coord = sum of prime coords.
 *
 * Dimensional signatures (dominant axis = part of speech):
 *   T-heavy = verb/action     D-heavy = preposition/spatial
 *   M-heavy = noun/substance  Q-heavy = adjective/quality
 *
 * Lookup via open-addressed hash table. All static, no allocation.
 */

/* ── Result type ─────────────────────────────────────────────── */

#define PRIME_FOUND   0
#define PRIME_UNKNOWN 1

typedef struct {
    coord4 c;
    int    status;  /* PRIME_FOUND or PRIME_UNKNOWN */
} prime_result;

/* ── Coordinate arithmetic ───────────────────────────────────── */

static inline coord4 coord4_add(coord4 a, coord4 b) {
    coord4 r;
    r.t = a.t + b.t;
    r.d = a.d + b.d;
    r.m = a.m + b.m;
    r.q = a.q + b.q;
    return r;
}

static inline coord4 coord4_sub(coord4 a, coord4 b) {
    coord4 r;
    r.t = a.t - b.t;
    r.d = a.d - b.d;
    r.m = a.m - b.m;
    r.q = a.q - b.q;
    return r;
}

static inline coord4 coord4_neg(coord4 a) {
    coord4 r;
    r.t = -a.t;
    r.d = -a.d;
    r.m = -a.m;
    r.q = -a.q;
    return r;
}

/* coord4_distance and coord4_manhattan: canonical source is distance.h */
#include "distance.h"

/*
 * Map coord4 to a density-map bin index.
 *
 * Clamps each dimension to [-4, +4] (9 values), then packs into
 * a flat index: 9^4 = 6561 bins. Covers shells 1-3 with room.
 * Higher magnitudes clamp to the boundary bin.
 */
static inline int coord4_to_index(coord4 c) {
    int t = c.t + 4; if (t < 0) t = 0; if (t > 8) t = 8;
    int d = c.d + 4; if (d < 0) d = 0; if (d > 8) d = 8;
    int m = c.m + 4; if (m < 0) m = 0; if (m > 8) m = 8;
    int q = c.q + 4; if (q < 0) q = 0; if (q > 8) q = 8;
    return t * 729 + d * 81 + m * 9 + q;
}

#define DENSITY_BINS 6561  /* 9^4 */

/* ── Hash table for word lookup ──────────────────────────────── */

/*
 * FNV-1a hash, case-insensitive, length-bounded.
 * Good distribution, fast, no branches in the hot loop.
 */
static inline uint32_t prime_hash(const char *word, int len) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++) {
        uint8_t ch = (uint8_t)word[i];
        /* lowercase: ASCII letters only */
        if (ch >= 'A' && ch <= 'Z') ch += 32;
        h ^= ch;
        h *= 16777619u;
    }
    return h;
}

/* case-insensitive compare, length-bounded */
static inline int prime_streq(const char *a, int alen,
                              const char *b, int blen) {
    if (alen != blen) return 0;
    for (int i = 0; i < alen; i++) {
        uint8_t ca = (uint8_t)a[i];
        uint8_t cb = (uint8_t)b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
    }
    return 1;
}

/*
 * Open-addressed hash table. 512 slots for ~200 entries = ~40% load.
 * Power of 2 for fast modulo. Entries are compile-time constant.
 */

typedef struct {
    const char *word;
    int8_t      len;
    int8_t      t, d, m, q;
} prime_entry;

#define PRIME_TABLE_SIZE 512
#define PRIME_TABLE_MASK (PRIME_TABLE_SIZE - 1)

/*
 * Word coordinate table.
 *
 * Sources, in priority order:
 *   1. Direct NSM primes from primes.md sections 1-16
 *   2. Explicit examples from primes.md structural patterns
 *   3. Decomposed via prime addition following primes.md rules
 *
 * Dimensional signature check on every entry:
 *   Nouns:        M-dominant    Verbs:        T-dominant
 *   Adjectives:   Q-dominant    Prepositions: D-dominant
 *   Pronouns:     M+Q           Conjunctions: Q
 *   Adverbs:      Q+T           Determiners:  D+Q
 */

/* sentinel: empty slot */
#define PE(w, T, D, M, Q) { w, sizeof(w)-1, T, D, M, Q }

static const prime_entry prime_words[] = {

    /* ── NSM PRIMES (sections 1-16 of primes.md) ────────────── */

    /* 1. Substantives */
    PE("i",         0, 0,+1,+1),
    PE("you",       0, 0,+1,-1),
    PE("someone",   0, 0,+1, 0),
    PE("something", 0, 0,+1, 0),
    PE("people",    0, 0,+2, 0),
    PE("body",      0,+1,+1, 0),

    /* 2. Relational */
    PE("part",      0,+1,+1, 0),
    PE("kind",      0, 0,+1,+1),

    /* 3. Determiners */
    PE("this",      0,+1, 0,+1),
    PE("same",      0, 0, 0,+1),
    PE("other",     0, 0, 0,-1),

    /* 4. Quantifiers */
    PE("one",       0, 0,+1, 0),
    PE("two",       0, 0,+2, 0),
    PE("some",      0, 0,+1,-1),
    PE("all",       0, 0,+2,+1),
    PE("much",      0, 0,+2, 0),

    /* 5. Evaluators */
    PE("good",      0, 0, 0,+1),
    PE("bad",       0, 0, 0,-1),

    /* 6. Descriptors */
    PE("big",       0, 0,+2, 0),
    PE("small",     0, 0,-2, 0),

    /* 7. Mental predicates */
    PE("think",    +1, 0,+1,+1),
    PE("know",      0, 0,+1,+1),
    PE("want",     +1, 0,+1,+1),
    PE("feel",      0, 0,+1,+1),
    PE("see",       0,+1, 0,+1),
    PE("hear",      0,+1, 0,+1),

    /* 8. Speech */
    PE("say",      +1, 0, 0,+1),
    PE("words",     0, 0,+1,+1),
    PE("true",      0, 0, 0,+1),

    /* 9. Actions */
    PE("do",       +1, 0, 0, 0),
    PE("happen",   +1, 0, 0, 0),
    PE("move",     +1,+1, 0, 0),

    /* 10. Existence */
    PE("be",        0,+1,+1, 0),

    /* 11. Possession */
    PE("mine",      0, 0,+1,+1),

    /* 12. Life */
    PE("live",     +1, 0,+1, 0),
    PE("die",      -1, 0,-1, 0),

    /* 13. Time primes */
    PE("when",     +1, 0, 0,+1),
    PE("now",      +1, 0, 0, 0),
    PE("before",   +2, 0, 0, 0),
    PE("after",    -2, 0, 0, 0),

    /* 14. Place primes */
    PE("where",     0,+1, 0,+1),
    PE("here",      0,+1, 0,+1),
    PE("above",     0,+2, 0, 0),
    PE("below",     0,-2, 0, 0),
    PE("far",       0,+2, 0, 0),
    PE("near",      0,+1, 0, 0),
    PE("side",      0,+1, 0,-1),
    PE("inside",    0,-1,+1, 0),
    PE("touch",     0,+1,+1,+1),

    /* 15. Logic primes */
    PE("not",       0, 0, 0,-1),
    PE("maybe",     0, 0, 0,+1),
    PE("can",      +1, 0, 0,+1),
    PE("because",  +1,+1, 0,+1),
    PE("if",        0, 0, 0,+1),

    /* 16. Degree primes */
    PE("very",      0, 0, 0,+2),
    PE("more",      0, 0,+1,+1),
    PE("like",      0, 0, 0,+1),  /* LIKE/AS (similarity) */

    /* ── PRONOUNS (pronoun table in primes.md) ───────────────── */

    /* I already in substantives above */
    PE("me",        0, 0,+1,+1),
    /* you already above */
    PE("he",        0, 0,+1,-1),
    PE("she",       0, 0,+1,-1),
    PE("him",       0, 0,+1,-1),
    PE("her",       0, 0,+1,-1),
    PE("it",        0, 0,+1, 0),
    PE("we",        0, 0,+2,+1),
    PE("us",        0, 0,+2,+1),
    PE("they",      0, 0,+2,-1),
    PE("them",      0, 0,+2,-1),

    /* ── ARTICLES (articles section in primes.md) ────────────── */

    PE("the",       0,+1, 0,+1),
    PE("a",         0, 0, 0,+1),
    PE("an",        0, 0, 0,+1),  /* = a, same coordinates */
    PE("that",      0,+2, 0,+1),
    PE("every",     0, 0,+2,+1),
    PE("no",        0, 0, 0,-1),

    /* ── COMMON VERBS ────────────────────────────────────────── */
    /*
     * Decomposed from primes. T-dominant.
     * is/was = BE prime, temporal variants
     * have = MINE + DO = possession-action
     * go = MOVE = DO + spatial
     * come = MOVE toward = inverse of go
     * make = DO + substance
     * take = DO + spatial + inverse-substance
     * give = DO + spatial + substance
     * run = MOVE (canonical from primes.md note: +1,+1,0,0)
     * get = DO + substance (acquisition)
     */

    PE("is",        0,+1,+1, 0),  /* BE: existence at location */
    PE("was",      +2,+1,+1, 0),  /* BE + BEFORE (past) */
    PE("are",       0,+1,+1, 0),  /* BE plural = BE */
    PE("were",     +2,+1,+1, 0),  /* BE + BEFORE plural */
    PE("am",        0,+1,+1, 0),  /* BE first person = BE */
    PE("been",     +2,+1,+1, 0),  /* BE + past participle */
    PE("being",    +1,+1,+1, 0),  /* BE + ongoing */
    PE("have",     +1, 0,+1,+1),  /* DO + possession = temporal substance+quality */
    PE("has",      +1, 0,+1,+1),  /* = have */
    PE("had",      +2, 0,+1,+1),  /* have + BEFORE */
    PE("go",       +1,+1, 0, 0),  /* MOVE = temporal + spatial */
    PE("went",     +2,+1, 0, 0),  /* go + BEFORE */
    PE("gone",     +2,+1, 0, 0),  /* go + past participle */
    PE("going",    +1,+1, 0, 0),  /* go + ongoing (same base) */
    PE("come",     +1,+1, 0, 0),  /* MOVE toward (same as go, context differs) */
    PE("came",     +2,+1, 0, 0),  /* come + BEFORE */
    PE("make",     +1, 0,+1, 0),  /* DO + substance */
    PE("made",     +2, 0,+1, 0),  /* make + BEFORE */
    PE("take",     +1,+1,-1, 0),  /* DO + spatial + inverse-M (antonym of give) */
    PE("took",     +2,+1,-1, 0),  /* take + BEFORE */
    PE("give",     +1,+1,+1, 0),  /* DO + spatial + substance */
    PE("gave",     +2,+1,+1, 0),  /* give + BEFORE */
    PE("run",      +1,+1, 0, 0),  /* MOVE (canonical: primes.md note) */
    PE("get",      +1, 0,+1, 0),  /* DO + substance (acquire) */
    PE("got",      +2, 0,+1, 0),  /* get + BEFORE */
    PE("put",      +1,+1,+1, 0),  /* DO + spatial + substance (place) */
    PE("set",      +1,+1,+1, 0),  /* DO + spatial + substance (= put) */
    PE("keep",     +1, 0,+1,+1),  /* DO + substance + quality (hold) */
    PE("let",      +1, 0, 0,+1),  /* DO + quality (permit) = CAN */
    PE("begin",    +1, 0, 0,+1),  /* DO + quality (start signal) */
    PE("start",    +1, 0, 0,+1),  /* = begin */
    PE("stop",     -1, 0, 0,-1),  /* inverse of begin */
    PE("end",      -1, 0, 0,-1),  /* = stop */
    PE("turn",     +1,+1, 0, 0),  /* MOVE (rotational) */
    PE("show",     +1,+1, 0,+1),  /* DO + spatial + quality (make visible) */
    PE("tell",     +1, 0, 0,+1),  /* = SAY */
    PE("ask",      +1, 0, 0,+1),  /* SAY + question (temporal signal) */
    PE("try",      +1, 0, 0,+1),  /* DO + quality (attempt) */
    PE("need",     +2, 0,+2,+1),  /* WANT intensified (primes.md intensity) */
    PE("use",      +1, 0,+1, 0),  /* DO + substance */
    PE("find",     +1,+1, 0,+1),  /* MOVE + quality (locate) */
    PE("found",    +2,+1, 0,+1),  /* find + BEFORE */
    PE("call",     +1, 0, 0,+1),  /* SAY (invoke) */
    PE("work",     +1, 0,+1, 0),  /* DO + substance (effort) */
    PE("look",     +1,+1, 0,+1),  /* SEE + temporal (active looking) */
    PE("leave",    +1,+1,-1, 0),  /* MOVE + inverse-substance (depart) */
    PE("left",     +2,+1,-1, 0),  /* leave + BEFORE */
    PE("read",     +1, 0,+1,+1),  /* DO + substance + quality (perceive text) */
    PE("write",    +1, 0,+1,+1),  /* DO + substance + quality (produce text) */
    PE("open",      0,+1,+1, 0),  /* spatial + substance (primes.md antonyms) */
    PE("close",     0,-1,-1, 0),  /* inverse of open */
    PE("stand",    +1,+1,+1, 0),  /* BE + temporal (active position) */
    PE("sit",      +1,+1,+1, 0),  /* = stand (different posture, same coords) */
    PE("hold",     +1, 0,+1,+1),  /* DO + substance + quality */
    PE("bring",    +1,+1,+1, 0),  /* MOVE + substance (carry toward) */
    PE("play",     +1, 0,+1,+1),  /* DO + substance + quality (primes.md) */
    PE("pay",      +1,+1,-1, 0),  /* DO + spatial + inverse-M (transfer away) */
    PE("help",     +1, 0,+1,+1),  /* DO + substance + quality (assist) */
    PE("meet",     +1,+1,+1,+1),  /* MOVE + substance + quality (contact) */
    PE("learn",    +1, 0,+1,+1),  /* KNOW + temporal (acquiring knowledge) */
    PE("change",   +1, 0,+1,-1),  /* DO + substance + OTHER */
    PE("follow",   +1,+1, 0, 0),  /* MOVE (behind) */
    PE("create",   +1, 0,+2,+1),  /* DO + high substance + quality */
    PE("build",    +1, 0,+2, 0),  /* DO + high substance */
    PE("grow",     +1, 0,+1,+1),  /* LIVE + quality (increase) */
    PE("remember", +2, 0,+1,+1),  /* KNOW + BEFORE (primes.md example) */
    PE("forget",   +2, 0,-1,-1),  /* KNOW inverted + BEFORE (primes.md antonyms) */
    PE("eat",      +1, 0,+1, 0),  /* DO + substance (consume) */
    PE("drink",    +1, 0,+1, 0),  /* DO + substance (consume liquid) */
    PE("sleep",   -1, 0,+1, 0),   /* inverse-temporal + substance (rest) */
    PE("walk",     +1,+1, 0, 0),  /* MOVE (lower intensity than run) */
    PE("speak",    +1, 0, 0,+1),  /* = SAY */
    PE("talk",     +1, 0, 0,+1),  /* = SAY */
    PE("wait",    +1, 0, 0,-1),   /* DO + inverse-quality (temporal, uncertain) */
    PE("send",    +1,+1,-1, 0),   /* MOVE + inverse-substance (dispatch) */
    PE("fall",    +1,+2, 0, 0),   /* MOVE + high spatial (downward) */
    PE("break",   +1, 0,-1,-1),   /* DO + inverse substance + BAD */
    PE("love",     0,+1,+2,+2),   /* spatial + high substance + high quality (primes.md) */
    PE("hate",     0, 0,+2,-2),   /* high substance + high negative quality */
    PE("kill",    +1, 0,-1,-1),   /* DO + DIE = temporal + inverse life */

    /* ── COMMON NOUNS ────────────────────────────────────────── */
    /*
     * M-dominant. Decomposed from primes.
     * Substance words: core M, with D/T/Q for context.
     */

    PE("time",     +1, 0,+1, 0),  /* temporal substance (primes.md: SOME TIME) */
    PE("man",       0, 0,+1,+1),  /* SOMEONE + self-quality (gendered) */
    PE("woman",     0, 0,+1,+1),  /* SOMEONE + self-quality (gendered) */
    PE("day",      +1, 0,+1, 0),  /* temporal substance (time unit) */
    PE("year",     +2, 0,+1, 0),  /* A LONG TIME + substance */
    PE("way",       0,+1,+1, 0),  /* spatial substance = PATH */
    PE("thing",     0, 0,+1, 0),  /* = SOMETHING */
    PE("world",     0,+1,+2, 0),  /* spatial + high substance */
    PE("life",     +1, 0,+1, 0),  /* = LIVE (as noun, T from temporal nature) */
    PE("hand",      0,+1,+1, 0),  /* BODY + spatial (limb at location) */
    PE("place",     0,+1,+1, 0),  /* spatial + substance */
    PE("case",      0, 0,+1,+1),  /* substance + quality (instance) */
    PE("week",     +1, 0,+1, 0),  /* temporal substance (time unit) */
    PE("point",     0,+1, 0,+1),  /* spatial + quality (location signal) */
    PE("number",    0, 0,+2, 0),  /* pure substance magnitude (quantity) */
    PE("group",     0, 0,+2, 0),  /* = PEOPLE (plural substance) */
    PE("fact",      0, 0,+1,+1),  /* substance + TRUE = substance + quality */
    PE("name",      0, 0,+1,+1),  /* WORDS (substance that carries signal) */
    PE("head",      0,+1,+1, 0),  /* BODY + ABOVE (spatial body part) */
    PE("home",      0,+1,+1,+1),  /* spatial + substance + quality (place+value) */
    PE("water",     0, 0,+2, 0),  /* high substance (mass noun) */
    PE("room",      0,+1,+1, 0),  /* spatial + substance (container) */
    PE("mother",    0, 0,+1,+1),  /* SOMEONE + quality (caregiver) */
    PE("father",    0, 0,+1,+1),  /* SOMEONE + quality (caregiver) */
    PE("child",     0, 0,+1, 0),  /* SOMEONE (young = low magnitude) */
    PE("night",    +1, 0,+1,-1),  /* temporal substance + inverse-quality (dark) */
    PE("eye",       0,+1, 0,+1),  /* = SEE (organ of perception) */
    PE("door",      0,+1,+1, 0),  /* spatial + substance (opening) */
    PE("book",      0, 0,+1,+1),  /* substance + quality (WORDS carrier) */
    PE("word",      0, 0,+1,+1),  /* = WORDS prime */
    PE("food",      0, 0,+2, 0),  /* high substance (nourishment) */
    PE("friend",    0, 0,+1,+1),  /* SOMEONE + GOOD */
    PE("power",    +1, 0,+2,+1),  /* temporal + high substance + quality */
    PE("money",     0, 0,+2,+1),  /* high substance + quality (value) */
    PE("earth",     0,+2,+2, 0),  /* high spatial + high substance */
    PE("light",     0,+1, 0,+2),  /* spatial + high quality (primes.md) */
    PE("sun",       0,+2,+2,+1),  /* high spatial + high substance + quality */
    PE("fire",     +1, 0,+1,+2),  /* temporal + substance + high quality (primes.md) */
    PE("air",       0,+1,+1, 0),  /* spatial + substance */
    PE("king",      0, 0,+2,+1),  /* high substance + quality (authority) */
    PE("god",       0,+2,+2,+2),  /* high spatial + substance + quality (ultimate) */
    PE("death",    -1, 0,-1, 0),  /* = DIE (as noun) */
    PE("war",      +1, 0,+2,-2),  /* temporal + high substance + high negative */
    PE("mind",     +1, 0,+1,+1),  /* = THINK (as noun) */
    PE("heart",     0, 0,+1,+2),  /* substance + high quality (emotional center) */
    PE("idea",      0, 0,+1,+1),  /* substance + quality (mental content) */
    PE("state",     0,+1,+1,+1),  /* spatial + substance + quality (condition) */
    PE("city",      0,+2,+2, 0),  /* high spatial + high substance */
    PE("land",      0,+2,+2, 0),  /* high spatial + high substance */
    PE("family",    0, 0,+2,+1),  /* plural substance + quality (kin) */

    /* ── COMMON ADJECTIVES ───────────────────────────────────── */
    /*
     * Q-dominant. Quality/evaluation words.
     * good/bad already in primes. big/small already in descriptors.
     */

    PE("new",       0, 0, 0,+1),  /* quality: fresh (GOOD) */
    PE("old",      +2, 0, 0,-1),  /* temporal + inverse quality (primes.md: brave old wise king) */
    PE("great",     0, 0,+2,+2),  /* BIG + GOOD = high substance + high quality */
    PE("little",    0, 0,-1, 0),  /* inverse-SMALL (lower magnitude) */
    PE("long",     +2, 0,+1, 0),  /* A LONG TIME (temporal extent + substance) */
    PE("high",      0,+2, 0,+1),  /* ABOVE + quality */
    PE("young",    -2, 0, 0,+1),  /* inverse-temporal + quality (opposite of old) */
    PE("right",     0, 0, 0,+1),  /* = GOOD = TRUE (correct) */
    PE("last",     +2, 0, 0,-1),  /* = old (temporal + inverse quality) */
    PE("first",     0, 0,+1,+1),  /* ONE + ordinal quality (primes.md numbers) */
    PE("next",     -2, 0, 0,+1),  /* AFTER + quality (upcoming) */
    PE("different", 0, 0, 0,-1),  /* = OTHER */
    PE("sure",      0, 0, 0,+2),  /* = VERY quality (certain) */
    PE("real",      0, 0,+1,+1),  /* substance + TRUE */
    PE("full",      0, 0,+1, 0),  /* substance present (full-empty schema) */
    PE("empty",     0, 0,-1, 0),  /* substance absent */
    PE("free",      0, 0, 0,+1),  /* quality: unbound (= ENABLEMENT) */
    PE("strong",    0, 0,+2,+1),  /* high substance + quality */
    PE("hard",      0, 0,+2,-1),  /* high substance + negative quality (difficult) */
    PE("dark",      0, 0, 0,-2),  /* high negative quality (inverse of light) */
    PE("hot",       0, 0, 0,+2),  /* high positive quality (primes.md intensity) */
    PE("cold",      0, 0, 0,-2),  /* high negative quality */
    PE("fast",     +2,+1, 0, 0),  /* high temporal + spatial (primes.md antonyms) */
    PE("slow",     -2,+1, 0, 0),  /* inverse of fast */
    PE("happy",     0, 0,+1,+2),  /* substance + high quality (primes.md synonyms) */
    PE("wrong",     0, 0, 0,-1),  /* = BAD */
    PE("clear",     0,+1, 0,+2),  /* spatial + high quality */
    PE("white",     0,+1, 0,+1),  /* spatial + quality (light color) */
    PE("black",     0,+1, 0,-1),  /* spatial + inverse quality (dark color) */
    PE("red",       0,+1, 0,+1),  /* spatial + quality (primes.md adjective examples) */
    PE("dead",     -1, 0,-1, 0),  /* = DIE */
    PE("whole",     0,+1,+2,+1),  /* spatial + high substance + quality (PART-WHOLE) */
    PE("able",     +1, 0, 0,+1),  /* = CAN */
    PE("possible",  0, 0, 0,+1),  /* = MAYBE */
    PE("important", 0, 0,+1,+2),  /* substance + high quality */
    PE("beautiful", 0,+1, 0,+4),  /* spatial + very high quality (primes.md example) */

    /* ── COMMON PREPOSITIONS ─────────────────────────────────── */
    /*
     * D-dominant. Spatial/positional words.
     */

    PE("in",        0,-1, 0, 0),  /* INSIDE without substance (primes.md) */
    PE("on",        0,+1, 0, 0),  /* SURFACE (spatial positive) */
    PE("at",        0,+1, 0, 0),  /* = ON (location at point) */
    PE("to",        0,+1, 0, 0),  /* spatial (direction toward) */
    PE("from",      0,+1, 0,-1),  /* SIDE prime: spatial + inverse quality (primes.md) */
    PE("with",      0,+1,+1, 0),  /* spatial + substance (accompaniment) */
    PE("by",        0,+1, 0, 0),  /* NEAR (spatial proximity) */
    PE("for",       0,+1, 0,+1),  /* spatial + quality (purpose/benefit) */
    PE("of",        0,+1,+1, 0),  /* PART (spatial subset relation) */
    PE("about",     0,+1, 0,+1),  /* spatial + quality (surrounding/concerning) */
    PE("between",   0,+2, 0, 0),  /* high spatial (two reference points) */
    PE("through",   0,+2, 0, 0),  /* high spatial (traversal) */
    PE("into",      0,-1,+1, 0),  /* INSIDE = negative spatial + substance */
    PE("over",      0,+2, 0, 0),  /* = ABOVE */
    PE("under",     0,-2, 0, 0),  /* = BELOW */
    PE("up",        0,+2, 0, 0),  /* = ABOVE (direction) */
    PE("down",      0,-2, 0, 0),  /* = BELOW (direction) */
    PE("out",       0,+1,-1, 0),  /* inverse of INSIDE (spatial + inverse substance) */
    PE("off",       0,+1,-1, 0),  /* = out (separation) */
    PE("away",      0,+2, 0,-1),  /* FAR + inverse quality */
    PE("back",      0,-1, 0, 0),  /* spatial inverse (behind/return) */
    PE("around",    0,+1, 0, 0),  /* spatial (surrounding) */
    PE("against",   0,+1, 0,-1),  /* spatial + inverse quality (opposition) */
    PE("across",    0,+2, 0, 0),  /* high spatial (traversal) */
    PE("along",     0,+2, 0, 0),  /* high spatial (parallel) */
    PE("behind",    0,-1, 0, 0),  /* = back */
    PE("without",   0,+1,-1, 0),  /* with inverted: spatial + inverse substance */

    /* ── COMMON CONJUNCTIONS ─────────────────────────────────── */
    /*
     * Q-dominant. Logic/gate words.
     */

    PE("and",       0, 0, 0,+1),  /* GOOD/SAME = test pass = join (primes.md) */
    PE("but",       0, 0, 0,-1),  /* OTHER/BAD = guard = contrast (primes.md) */
    PE("or",        0, 0, 0,+1),  /* MAYBE = alternative (quality signal) */
    /* if already in logic primes */
    /* because already in logic primes */
    /* when already in time primes */
    PE("while",    +1, 0, 0,+1),  /* temporal + quality (AT THE SAME TIME) */
    PE("although",  0, 0, 0,-1),  /* = BUT (concessive) */
    PE("so",        0, 0, 0,+1),  /* = BECAUSE result (quality signal) */
    PE("yet",       0, 0, 0,-1),  /* = BUT (temporal contrast) */
    PE("than",      0, 0, 0,-1),  /* comparison signal (evaluative) */
    PE("whether",   0, 0, 0,+1),  /* = IF (alternative question) */
    PE("since",    +2, 0, 0,+1),  /* BEFORE + quality (temporal cause) */
    PE("until",    -2, 0, 0,-1),  /* AFTER + inverse quality (temporal limit) */
    PE("unless",    0, 0, 0,-1),  /* IF + NOT = conditional + negation */
    PE("once",     +1, 0,+1, 0),  /* ONE + temporal */

    /* ── COMMON ADVERBS ──────────────────────────────────────── */
    /*
     * Q+T signature. Quality of change.
     * not/very/much/now/here/there already defined above.
     */

    PE("then",     +2, 0, 0, 0),  /* = BEFORE (temporal, past-reference) */
    PE("there",     0,+1, 0, 0),  /* spatial (distal location, no signal) */
    PE("always",   +2, 0, 0,+1),  /* A LONG TIME + quality (permanent) */
    PE("never",    +2, 0, 0,-1),  /* A LONG TIME + NOT */
    PE("often",    +1, 0, 0,+1),  /* temporal + quality (frequent) */
    PE("still",    +1, 0, 0,+1),  /* temporal + quality (ongoing) */
    PE("just",      0, 0, 0,+1),  /* = SAME (exactly, only) */
    PE("also",      0, 0, 0,+1),  /* = AND (additive) */
    PE("already",  +2, 0, 0,+1),  /* BEFORE + quality (completed) */
    PE("again",    +2, 0, 0, 0),  /* = BEFORE (repetition = re-temporal) */
    PE("well",      0, 0, 0,+1),  /* = GOOD (quality of manner) */
    PE("almost",    0, 0, 0,+1),  /* NEAR (quality) + signal */
    PE("only",      0, 0, 0,+1),  /* = ONE quality (exclusive) */
    PE("even",      0, 0, 0,+1),  /* = SAME (emphasis/surprise) */
    PE("really",    0, 0, 0,+2),  /* = VERY (TRUE intensified) */
    PE("quite",     0, 0, 0,+1),  /* moderate quality amplification */
    PE("enough",    0, 0,+1,+1),  /* substance + quality (sufficient) */
    PE("too",       0, 0, 0,+2),  /* = VERY (excess) */
    PE("perhaps",   0, 0, 0,+1),  /* = MAYBE */
    PE("quickly",  +1, 0, 0,+1),  /* temporal + quality (primes.md adverb) */
    PE("slowly",   +1, 0, 0,-1),  /* temporal + inverse quality (primes.md adverb) */
    PE("together",  0,+1,+2, 0),  /* spatial + plural substance */

    /* ── MODAL VERBS (primes.md modality section) ────────────── */

    PE("must",     +1, 0, 0,+3),  /* DO + absolute quality */
    PE("shall",    +1, 0, 0,+2),  /* DO + strong quality */
    PE("should",   +1, 0, 0,+2),  /* DO + strong quality */
    PE("will",     +1, 0, 0,+1),  /* DO + quality (intention) */
    PE("would",    +1, 0, 0,+1),  /* DO + quality (conditional) */
    PE("could",    +1, 0, 0, 0),  /* DO (weak possibility) */
    PE("may",       0, 0, 0, 0),  /* neutral (permission) */
    PE("might",     0, 0, 0,-1),  /* inverse quality (remote possibility) */

    /* ── QUESTION WORDS ──────────────────────────────────────── */

    /* when, where already above */
    PE("what",      0, 0,+1,+1),  /* substance + signal */
    PE("who",       0, 0,+1,+1),  /* substance + signal (person) */
    PE("which",     0, 0,+1,+1),  /* substance + signal (selection) */
    PE("how",       0, 0, 0,+1),  /* quality signal (method) */
    PE("why",      +1,+1, 0,+1),  /* = BECAUSE as question */

    /* ── MISC HIGH-FREQUENCY ─────────────────────────────────── */

    PE("yes",       0, 0, 0,+1),  /* = GOOD = TRUE */
    PE("ok",        0, 0, 0,+1),  /* = GOOD (shell 1, informal) */
    PE("please",    0, 0, 0,+1),  /* quality signal (request) */
    PE("thank",     0, 0, 0,+1),  /* quality signal (gratitude) */
    PE("sorry",     0, 0, 0,-1),  /* inverse quality (regret) */
    PE("each",      0, 0,+1,+1),  /* ONE + quality (distributive) */
    PE("own",       0, 0,+1,+1),  /* = MINE */
    PE("such",      0, 0, 0,+2),  /* = VERY (intensifier) */
    PE("these",     0,+1,+2,+1),  /* THIS + plural (spatial + plural + signal) */
    PE("those",     0,+2,+2,+1),  /* THAT + plural */
    PE("many",      0, 0,+2, 0),  /* = MUCH */
    PE("few",       0, 0,-1, 0),  /* inverse of MUCH */
    PE("both",      0, 0,+2,+1),  /* TWO + quality (inclusive) */
    PE("most",      0, 0,+2,+1),  /* = ALL (near-maximum) */
    PE("any",       0, 0,+1,-1),  /* = SOME (substance, uncertain) */
    PE("another",   0, 0,+1,-1),  /* ONE + OTHER */
    PE("itself",    0, 0,+1,+1),  /* IT + SAME (reflexive) */
    PE("nothing",   0, 0,-1,-1),  /* inverse of SOMETHING */
    PE("everything",0, 0,+2,+1),  /* = ALL */
};

#define PRIME_WORD_COUNT (sizeof(prime_words) / sizeof(prime_words[0]))

/* ── Hash table construction ─────────────────────────────────── */

/*
 * Slots: word index into prime_words[], or -1 if empty.
 * Built once on first call (lazy init with a flag).
 */

static int prime_table_slots[PRIME_TABLE_SIZE];
static int prime_table_ready = 0;

static void prime_table_init(void) {
    if (prime_table_ready) return;

    for (int i = 0; i < PRIME_TABLE_SIZE; i++)
        prime_table_slots[i] = -1;

    for (int i = 0; i < (int)PRIME_WORD_COUNT; i++) {
        uint32_t h = prime_hash(prime_words[i].word, prime_words[i].len);
        int slot = h & PRIME_TABLE_MASK;

        /* linear probe for open slot */
        while (prime_table_slots[slot] != -1) {
            slot = (slot + 1) & PRIME_TABLE_MASK;
        }
        prime_table_slots[slot] = i;
    }

    prime_table_ready = 1;
}

/* ── Public lookup ───────────────────────────────────────────── */

/*
 * prime_lookup: word string + length -> coord4 + status.
 *
 * Returns PRIME_FOUND if the word is in the table.
 * Returns PRIME_UNKNOWN with (0,0,0,0) if not found.
 */
static inline prime_result prime_lookup(const char *word, int len) {
    prime_result r;
    r.c.t = 0; r.c.d = 0; r.c.m = 0; r.c.q = 0;
    r.status = PRIME_UNKNOWN;

    if (len <= 0) return r;

    prime_table_init();

    uint32_t h = prime_hash(word, len);
    int slot = h & PRIME_TABLE_MASK;

    for (int probe = 0; probe < PRIME_TABLE_SIZE; probe++) {
        int idx = prime_table_slots[slot];
        if (idx == -1) return r;  /* empty slot = not found */

        if (prime_streq(word, len,
                        prime_words[idx].word, prime_words[idx].len)) {
            r.c.t = prime_words[idx].t;
            r.c.d = prime_words[idx].d;
            r.c.m = prime_words[idx].m;
            r.c.q = prime_words[idx].q;
            r.status = PRIME_FOUND;
            return r;
        }

        slot = (slot + 1) & PRIME_TABLE_MASK;
    }

    return r;  /* full table scan, not found (shouldn't happen at 40% load) */
}

/*
 * prime_lookup_str: null-terminated string convenience wrapper.
 */
static inline prime_result prime_lookup_str(const char *word) {
    return prime_lookup(word, (int)strlen(word));
}

/* ── Decomposition helpers ───────────────────────────────────── */

/*
 * Classify part of speech from coordinates.
 * Returns the dominant dimension character: 'T', 'D', 'M', 'Q'.
 * On tie, priority: M > T > Q > D (nouns most common, then verbs).
 */
static inline char coord4_pos(coord4 c) {
    int at = c.t < 0 ? -c.t : c.t;
    int ad = c.d < 0 ? -c.d : c.d;
    int am = c.m < 0 ? -c.m : c.m;
    int aq = c.q < 0 ? -c.q : c.q;

    int max = am;
    char pos = 'M';  /* noun */

    if (at > max) { max = at; pos = 'T'; }  /* verb */
    if (aq > max) { max = aq; pos = 'Q'; }  /* adjective */
    if (ad > max) { max = ad; pos = 'D'; }  /* preposition */

    return pos;
}

/*
 * Shell number: which concentric shell the coordinate lives on.
 * Shell = max absolute coordinate value.
 * Shell 0 = origin (identity/silence).
 * Shell 1 = fundamental primes.
 * Shell 2+ = compounds and intensified forms.
 */
static inline int coord4_shell(coord4 c) {
    int at = c.t < 0 ? -c.t : c.t;
    int ad = c.d < 0 ? -c.d : c.d;
    int am = c.m < 0 ? -c.m : c.m;
    int aq = c.q < 0 ? -c.q : c.q;

    int max = at;
    if (ad > max) max = ad;
    if (am > max) max = am;
    if (aq > max) max = aq;
    return max;
}

/*
 * Are two words synonyms? (distance <= 2 per primes.md)
 */
static inline int coord4_is_synonym(coord4 a, coord4 b) {
    return coord4_distance(a, b) <= 2.0f;
}

/*
 * Are two words antonyms? (differ by sign flip in 1-2 dimensions,
 * with all other dimensions matching)
 */
static inline int coord4_is_antonym(coord4 a, coord4 b) {
    int flips = 0;
    if (a.t != 0 && a.t == -b.t) flips++;
    else if (a.t != b.t) return 0;
    if (a.d != 0 && a.d == -b.d) flips++;
    else if (a.d != b.d) return 0;
    if (a.m != 0 && a.m == -b.m) flips++;
    else if (a.m != b.m) return 0;
    if (a.q != 0 && a.q == -b.q) flips++;
    else if (a.q != b.q) return 0;
    return flips >= 1 && flips <= 2;
}

#endif /* PRIMES_LOOKUP_H */
