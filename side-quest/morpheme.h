#ifndef MORPHEME_H
#define MORPHEME_H

/*
 * morpheme.h — suffix/prefix decomposer + character-level prior
 *
 * Strips affixes from words and returns coordinate transforms.
 * Rules from primes.md structural patterns section.
 *
 * This replaces the dictionary expansion approach. Instead of
 * hardcoding 2000 words, we derive coordinates for any word
 * from its root + affixes. 333 known words → unlimited coverage.
 *
 * The transforms don't need to be perfect on first encounter.
 * They put the word in the right REGION. The running average
 * across encounters will converge to the true coordinate.
 */

#include "wave.h"

/* ================================================================
 * Result of morpheme analysis
 * ================================================================ */

typedef struct {
    char   root[32];       /* stripped root word, lowercase          */
    int    root_len;       /* length of root                        */
    coord4 delta;          /* coordinate delta to ADD to root coord  */
    int    q_flip;         /* flip Q sign before adding delta        */
    int    all_flip;       /* flip all signs before adding delta     */
    int    found_affix;    /* nonzero if any affix was stripped       */
} MorphemeResult;


/* ================================================================
 * Suffix table — longest match first
 * ================================================================ */

typedef struct {
    const char *suffix;
    int         len;
    int8_t      dt, dd, dm, dq;   /* coordinate delta */
    int         min_root;          /* minimum root length after strip */
} SuffixRule;

static const SuffixRule suffix_rules[] = {
    /* -tion/-sion: verb → noun (add M) */
    { "ation",  5,   0, 0, +1, 0,   3 },
    { "ition",  5,   0, 0, +1, 0,   3 },
    { "ution",  5,   0, 0, +1, 0,   3 },
    { "tion",   4,   0, 0, +1, 0,   2 },
    { "sion",   4,   0, 0, +1, 0,   2 },

    /* -ness: adj → noun (add M) */
    { "ness",   4,   0, 0, +1, 0,   3 },

    /* -ment: verb → noun (add M) */
    { "ment",   4,   0, 0, +1, 0,   3 },

    /* -able/-ible: verb → adj (add Q) */
    { "able",   4,   0, 0,  0, +1,  3 },
    { "ible",   4,   0, 0,  0, +1,  3 },

    /* -ful/-less: noun → adj (±Q) */
    { "less",   4,   0, 0,  0, -1,  3 },
    { "ful",    3,   0, 0,  0, +1,  3 },

    /* -ous/-ious: noun → adj (add Q) */
    { "ious",   4,   0, 0,  0, +1,  3 },
    { "ous",    3,   0, 0,  0, +1,  3 },

    /* -ing: ongoing (add T + M for loop) */
    { "ning",   4,  +1, 0, +1, 0,   3 },  /* running → run + ing */
    { "ting",   4,  +1, 0, +1, 0,   3 },  /* sitting → sit + ing (handle double) */
    { "ing",    3,  +1, 0, +1, 0,   2 },

    /* -er/-or: verb → agent noun (add M) */
    { "ier",    3,   0, 0, +1, 0,   3 },
    { "er",     2,   0, 0, +1, 0,   2 },
    { "or",     2,   0, 0, +1, 0,   2 },

    /* -ist: noun → person (add M+Q) */
    { "ist",    3,   0, 0, +1, +1,  3 },

    /* -ly: adj/verb → adverb (add Q) */
    { "ly",     2,   0, 0,  0, +1,  3 },

    /* -ed: past tense (add T) */
    { "ied",    3,  +1, 0,  0, 0,   2 },  /* carried → carry */
    { "ed",     2,  +1, 0,  0, 0,   2 },

    /* -ize/-ise: noun/adj → verb (add T) */
    { "ize",    3,  +1, 0,  0, 0,   3 },
    { "ise",    3,  +1, 0,  0, 0,   3 },

    /* -ify: noun/adj → verb (add T) */
    { "ify",    3,  +1, 0,  0, 0,   3 },

    /* -al/-ial: noun → adj (add Q) */
    { "ial",    3,   0, 0,  0, +1,  3 },
    { "al",     2,   0, 0,  0, +1,  3 },

    /* -ity: adj → noun (add M) */
    { "ity",    3,   0, 0, +1, 0,   3 },

    /* -es/-s: plural (add M) — last because short */
    { "ies",    3,   0, 0, +1, 0,   2 },  /* cities → city */
    { "es",     2,   0, 0, +1, 0,   3 },
    { "s",      1,   0, 0, +1, 0,   3 },

    { NULL, 0, 0, 0, 0, 0, 0 }
};


/* ================================================================
 * Prefix table
 * ================================================================ */

typedef struct {
    const char *prefix;
    int         len;
    int8_t      dt, dd, dm, dq;
    int         q_flip;            /* flip Q of root */
    int         all_flip;          /* flip all signs of root */
    int         min_root;
} PrefixRule;

static const PrefixRule prefix_rules[] = {
    /* negation prefixes: flip Q */
    { "un",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "dis",    3,   0, 0, 0, 0,   1, 0,  3 },
    { "im",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "in",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "ir",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "il",     2,   0, 0, 0, 0,   1, 0,  3 },
    { "non",    3,   0, 0, 0, 0,   1, 0,  3 },

    /* anti-: flip all signs */
    { "anti",   4,   0, 0, 0, 0,   0, 1,  3 },

    /* mis-: flip Q (wrong quality) */
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


/* ================================================================
 * Morpheme analysis
 * ================================================================ */

static inline int morph_streq(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

/*
 * Analyze a word for affixes.
 *
 * Tries one prefix and one suffix. Returns the root and
 * the coordinate transform to apply to the root's coordinate.
 *
 * The word must be lowercase and null-terminated.
 */
static MorphemeResult morpheme_analyze(const char *word, int len) {
    MorphemeResult mr;
    memset(&mr, 0, sizeof(mr));

    if (len < 3 || len > 31) {
        /* too short for affix stripping, or too long */
        memcpy(mr.root, word, (size_t)(len < 31 ? len : 31));
        mr.root[len < 31 ? len : 31] = '\0';
        mr.root_len = len < 31 ? len : 31;
        return mr;
    }

    const char *start = word;
    int remaining = len;
    coord4 delta = { .t = 0, .d = 0, .m = 0, .q = 0 };
    int q_flip = 0, all_flip = 0;
    int found = 0;

    /* try prefix first */
    for (int i = 0; prefix_rules[i].prefix; i++) {
        const PrefixRule *p = &prefix_rules[i];
        if (remaining >= p->len + p->min_root &&
            morph_streq(start, p->prefix, p->len)) {
            start += p->len;
            remaining -= p->len;
            delta.t += p->dt;
            delta.d += p->dd;
            delta.m += p->dm;
            delta.q += p->dq;
            q_flip = p->q_flip;
            all_flip = p->all_flip;
            found = 1;
            break;
        }
    }

    /* try suffix */
    for (int i = 0; suffix_rules[i].suffix; i++) {
        const SuffixRule *s = &suffix_rules[i];
        if (remaining >= s->len + s->min_root &&
            morph_streq(start + remaining - s->len, s->suffix, s->len)) {
            remaining -= s->len;
            delta.t += s->dt;
            delta.d += s->dd;
            delta.m += s->dm;
            delta.q += s->dq;
            found = 1;
            break;
        }
    }

    /* copy root */
    if (remaining > 31) remaining = 31;
    memcpy(mr.root, start, (size_t)remaining);
    mr.root[remaining] = '\0';
    mr.root_len = remaining;
    mr.delta = delta;
    mr.q_flip = q_flip;
    mr.all_flip = all_flip;
    mr.found_affix = found;

    return mr;
}


/*
 * Apply the morpheme transform to a root coordinate.
 *
 * 1. If all_flip: negate all dimensions of root
 * 2. If q_flip: negate Q of root
 * 3. Add delta
 */
static coord4 morpheme_apply(coord4 root, const MorphemeResult *mr) {
    coord4 c = root;

    if (mr->all_flip) {
        c.t = -c.t;
        c.d = -c.d;
        c.m = -c.m;
        c.q = -c.q;
    }

    if (mr->q_flip) {
        c.q = -c.q;
    }

    /* add delta, clamping to int8_t */
    int t = (int)c.t + (int)mr->delta.t;
    int d = (int)c.d + (int)mr->delta.d;
    int m = (int)c.m + (int)mr->delta.m;
    int q = (int)c.q + (int)mr->delta.q;

    c.t = (int8_t)(t > 127 ? 127 : t < -128 ? -128 : t);
    c.d = (int8_t)(d > 127 ? 127 : d < -128 ? -128 : d);
    c.m = (int8_t)(m > 127 ? 127 : m < -128 ? -128 : m);
    c.q = (int8_t)(q > 127 ? 127 : q < -128 ? -128 : q);

    return c;
}


/*
 * Character-level wave byte prior.
 *
 * Averages the coord4 of each letter's wave byte encoding.
 * This gives a weak phonetic-semantic signal (~1-5% from
 * sound symbolism research). Nonzero when all else fails.
 *
 * The point: even a word with no known root, no matching affix,
 * in a context of all unknowns, still gets a nonzero coordinate.
 * The (0,0,0,0) bootstrap problem is dead.
 */
static coord4 char_level_prior(const char *word, int len) {
    int st = 0, sd = 0, sm = 0, sq = 0;
    int count = 0;

    for (int i = 0; i < len; i++) {
        uint8_t ch = (uint8_t)word[i];
        /* lowercase */
        if (ch >= 'A' && ch <= 'Z') ch += 32;
        uint8_t wb = utf8_to_wave(ch);
        if (wb != 0x00) {
            coord4 c = wave_decode(wb);
            st += c.t;
            sd += c.d;
            sm += c.m;
            sq += c.q;
            count++;
        }
    }

    coord4 r = { .t = 0, .d = 0, .m = 0, .q = 0 };
    if (count > 0) {
        r.t = (int8_t)(st / count);
        r.d = (int8_t)(sd / count);
        r.m = (int8_t)(sm / count);
        r.q = (int8_t)(sq / count);
    }

    /*
     * If the average rounds to all zeros (common for short words),
     * use a minimal M-bias. Words are substance. Better than nothing.
     */
    if (r.t == 0 && r.d == 0 && r.m == 0 && r.q == 0 && count > 0) {
        r.m = +1;
    }

    return r;
}

#endif /* MORPHEME_H */
