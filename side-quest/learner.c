/*
 * learner.c — component 3.1: memetic context learner
 *
 * Learns word coordinates from context. No training. No gradients.
 * Born-indexed on first encounter, refined by running average.
 *
 * Three layers:
 *   Layer 0: 16 bond patterns (hardcoded schemas, the seed)
 *   Layer 1: ~65 semantic primes (fixed anchors, confidence = MAX)
 *   Layer 2: everything else (memetic, context-learned, lifelong)
 *
 * The learning algorithm:
 *
 *   1. Unknown word appears at walk position P.
 *   2. The bond between position P-1 and P tells us which
 *      dimensions changed, which constrains the coordinate region.
 *   3. The centroid of surrounding known words gives a baseline.
 *   4. The bond constraint biases the centroid toward the expected
 *      dimensional signature (noun → M-heavy, verb → T-heavy, etc).
 *   5. Store immediately with confidence=1.
 *   6. On re-encounter: weighted average pulls coordinate toward
 *      its "true" position in the space.
 *
 * This mirrors how children learn words:
 *   - They have innate schemas (bond patterns) BEFORE any words.
 *   - They learn primes first (mama, no, more, want).
 *   - Every other word is learned from context: "The X is Y" —
 *     the child knows "the" and "is" and infers X is a noun,
 *     Y is an adjective, from the surrounding known words.
 *   - Each encounter refines understanding. No explicit teaching.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "learner.h"


/* ================================================================
 * FNV-1a hash — same pattern as other lattice components
 * ================================================================
 *
 * We hash the lowercased word bytes. The hash is used for open-
 * addressing lookup in the 64K vocabulary table.
 */

static uint32_t fnv1a(const char *data, int len) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < len; i++) {
        h ^= (uint8_t)data[i];
        h *= 16777619u;
    }
    return h;
}


/* ================================================================
 * Vocabulary hash table operations
 * ================================================================
 *
 * Open addressing with linear probing. 64K slots.
 * Empty slots have word[0] == '\0'.
 *
 * The table is intentionally oversized. At typical vocabulary
 * sizes (10K-50K words), the load factor stays under 0.8.
 * Linear probing is cache-friendly and simple.
 */

/*
 * Lowercase a word into a buffer. Returns the length clamped to
 * LEARNER_WORD_MAX - 1 (leaving room for null terminator).
 */
static int normalize_word(const char *word, int len, char *out) {
    if (len > LEARNER_WORD_MAX - 1)
        len = LEARNER_WORD_MAX - 1;
    for (int i = 0; i < len; i++) {
        char c = word[i];
        if (c >= 'A' && c <= 'Z')
            out[i] = c + 32;
        else
            out[i] = c;
    }
    out[len] = '\0';
    return len;
}

/*
 * Find a slot for a word. Returns the slot index.
 *
 * If the word exists: returns its slot (word matches).
 * If the word is new: returns the first empty slot.
 *
 * Linear probing with a max of 128 probes. If the table is
 * so full that we can't find a slot in 128 probes, we return -1.
 * At 64K slots and typical vocabulary sizes, this never happens.
 */
static int vocab_find_slot(const Learner *lr, const char *norm, int len) {
    uint32_t h = fnv1a(norm, len);
    uint32_t mask = LEARNER_VOCAB_SLOTS - 1;

    for (int probe = 0; probe < 128; probe++) {
        uint32_t idx = (h + (uint32_t)probe) & mask;
        const WordEntry *e = &lr->vocab[idx];

        /* empty slot: word not found, this is where it would go */
        if (e->word[0] == '\0')
            return (int)idx;

        /* occupied slot: check if it's our word */
        if (strncmp(e->word, norm, (size_t)len) == 0 && e->word[len] == '\0')
            return (int)idx;
    }

    /* table too full — should never happen at normal load factors */
    return -1;
}

/*
 * Insert a word entry. Assumes the slot is empty (word[0] == '\0').
 */
static void vocab_insert(Learner *lr, int slot,
                          const char *norm, int len,
                          coord4 coord, coord4 born, uint32_t confidence) {
    WordEntry *e = &lr->vocab[slot];
    memcpy(e->word, norm, (size_t)len);
    e->word[len] = '\0';
    e->coord = coord;
    e->born_context = born;
    e->confidence = confidence;
    lr->vocab_count++;
}


/* ================================================================
 * Bond schema initialization (Layer 0)
 * ================================================================
 *
 * The 16 bond patterns are the innate cognitive schemas. Each one
 * maps to a default coordinate region — the dimensional signature
 * that words at this bond position tend to have.
 *
 * These biases are derived from the schema→prime mapping in
 * discovery-schema-bonds.md:
 *
 *   bond 0  (none)    = IDENTITY    → no bias (same coordinate)
 *   bond 1  (P)       = FORCE       → T-heavy (action)
 *   bond 2  (C)       = PATH        → D-heavy (spatial)
 *   bond 3  (P+C)     = TRAVERSAL   → T+D (verb of motion)
 *   bond 4  (R)       = CYCLE       → M-heavy (substance/loop)
 *   bond 5  (P+R)     = FULL-EMPTY  → T+M (temporal substance)
 *   bond 6  (C+R)     = ITERATION   → D+M (spatial substance)
 *   bond 7  (P+C+R)   = ACCUMULATE  → T+D+M (complex action)
 *   bond 8  (W)       = LINK/MATCH  → Q-heavy (evaluation)
 *   bond 9  (P+W)     = CONTACT     → T+Q (temporal quality)
 *   bond 10 (C+W)     = ENABLEMENT  → D+Q (spatial quality)
 *   bond 11 (P+C+W)   = PART-WHOLE  → T+D+Q (transform)
 *   bond 12 (R+W)     = SCALE       → M+Q (substance quality)
 *   bond 13 (P+R+W)   = NEAR-FAR    → T+M+Q (progressive)
 *   bond 14 (C+R+W)   = BALANCE     → D+M+Q (causation)
 *   bond 15 (P+C+R+W) = CONTAINER   → all dimensions (hylo)
 *
 * The default_region coord4 encodes which dimensions are "expected"
 * to be active (nonzero) at this bond position. +1 means "this
 * dimension is likely positive", -1 means "likely negative", 0
 * means "this dimension is not expected to dominate."
 */

static const char *schema_names[16] = {
    "identity",       /*  0: (none)   */
    "force",          /*  1: P        */
    "path",           /*  2: C        */
    "traversal",      /*  3: P+C      */
    "cycle",          /*  4: R        */
    "full-empty",     /*  5: P+R      */
    "iteration",      /*  6: C+R      */
    "accumulation",   /*  7: P+C+R    */
    "link",           /*  8: W        */
    "contact",        /*  9: P+W      */
    "enablement",     /* 10: C+W      */
    "part-whole",     /* 11: P+C+W    */
    "scale",          /* 12: R+W      */
    "near-far",       /* 13: P+R+W    */
    "balance",        /* 14: C+R+W    */
    "container"       /* 15: P+C+R+W  */
};

/*
 * Default regions for each bond pattern.
 *
 * These are dimensional biases, not exact coordinates.
 * When bond pattern B connects the previous word to the current
 * unknown word, the default region tells us which dimensions
 * of the current word should be active.
 *
 * Derivation: each active force means that dimension CHANGED
 * from previous to current. If prev was M-heavy and the bond
 * has P active, then current probably has T active (the T
 * dimension changed). The default_region encodes these expected
 * dimensional signatures.
 */
static const coord4 bond_default_regions[16] = {
    /*  0: identity    — no change expected, same region as previous   */
    { .t =  0, .d =  0, .m =  0, .q =  0 },
    /*  1: P (force)   — T dimension activated → verb/action           */
    { .t = +1, .d =  0, .m =  0, .q =  0 },
    /*  2: C (path)    — D dimension activated → spatial/preposition   */
    { .t =  0, .d = +1, .m =  0, .q =  0 },
    /*  3: P+C (trav)  — T+D both active → motion verb (run, go, come)*/
    { .t = +1, .d = +1, .m =  0, .q =  0 },
    /*  4: R (cycle)   — M dimension activated → noun/substance        */
    { .t =  0, .d =  0, .m = +1, .q =  0 },
    /*  5: P+R (full)  — T+M active → temporal substance (live, grow) */
    { .t = +1, .d =  0, .m = +1, .q =  0 },
    /*  6: C+R (iter)  — D+M active → spatial substance (place-noun)  */
    { .t =  0, .d = +1, .m = +1, .q =  0 },
    /*  7: P+C+R (acc) — T+D+M → complex (give, take, make)          */
    { .t = +1, .d = +1, .m = +1, .q =  0 },
    /*  8: W (link)    — Q dimension activated → adjective/evaluator  */
    { .t =  0, .d =  0, .m =  0, .q = +1 },
    /*  9: P+W (contact) — T+Q → perception/selection (see, hear)     */
    { .t = +1, .d =  0, .m =  0, .q = +1 },
    /* 10: C+W (enable)  — D+Q → determiner/conditional (this, if)    */
    { .t =  0, .d = +1, .m =  0, .q = +1 },
    /* 11: P+C+W (part)  — T+D+Q → transformation verb               */
    { .t = +1, .d = +1, .m =  0, .q = +1 },
    /* 12: R+W (scale)   — M+Q → pronoun/quantifier (some, all)       */
    { .t =  0, .d =  0, .m = +1, .q = +1 },
    /* 13: P+R+W (near)  — T+M+Q → progressive (mental verb)          */
    { .t = +1, .d =  0, .m = +1, .q = +1 },
    /* 14: C+R+W (bal)   — D+M+Q → causation (because, therefore)     */
    { .t =  0, .d = +1, .m = +1, .q = +1 },
    /* 15: P+C+R+W (con) — all active → hylo (think, full pipeline)   */
    { .t = +1, .d = +1, .m = +1, .q = +1 },
};

void learner_seed_bonds(Learner *lr) {
    for (int i = 0; i < LEARNER_BOND_COUNT; i++) {
        lr->bonds[i].type = (BondType)i;
        lr->bonds[i].default_region = bond_default_regions[i];
        lr->bonds[i].schema_name = schema_names[i];
    }
}


/* ================================================================
 * Prime seeding (Layer 1)
 * ================================================================
 *
 * The ~65 NSM primes are the first words. They have FIXED
 * coordinates (confidence = UINT32_MAX) that never update.
 *
 * These coordinates come from primes.md — each one is derived
 * from the dimensional semantics:
 *   T = temporal (when, sequence, change)
 *   D = spatial  (where, position, direction)
 *   M = substance (what, quantity, content)
 *   Q = quality  (how, evaluation, logic)
 *
 * The primes anchor the coordinate system. When the learner
 * encounters "the big cat sat", it knows "the", "big", "sat"
 * from the primes. It uses their coordinates + bond patterns
 * to estimate "cat" → M-heavy (noun).
 *
 * Without these anchors, the first few thousand words would
 * have no reference frame. The primes give the learner a
 * bootstrap: 65 fixed points in 4D space from which all other
 * coordinates are triangulated.
 */

/*
 * Helper: insert a prime into the vocabulary.
 * Primes have confidence = UINT32_MAX (immortal).
 */
static void seed_prime(Learner *lr, const char *word,
                       int8_t t, int8_t d, int8_t m, int8_t q) {
    int len = (int)strlen(word);
    char norm[LEARNER_WORD_MAX];
    len = normalize_word(word, len, norm);

    int slot = vocab_find_slot(lr, norm, len);
    if (slot < 0) return; /* table full, should never happen */

    /* if slot already occupied (duplicate prime), skip */
    if (lr->vocab[slot].word[0] != '\0') return;

    coord4 c = { .t = t, .d = d, .m = m, .q = q };
    vocab_insert(lr, slot, norm, len, c, c, UINT32_MAX);
}

void learner_seed_primes(Learner *lr) {

    /* ── 1. Substantives — WHO/WHAT (M axis) ──────────────────── */

    seed_prime(lr, "i",         0,  0, +1, +1);  /* self-substance           */
    seed_prime(lr, "you",       0,  0, +1, -1);  /* other-substance          */
    seed_prime(lr, "someone",   0,  0, +1,  0);  /* unspecified person       */
    seed_prime(lr, "something", 0,  0, +1,  0);  /* unspecified thing        */
    seed_prime(lr, "people",    0,  0, +2,  0);  /* plural substance         */
    seed_prime(lr, "body",      0, +1, +1,  0);  /* substance + spatial      */

    /* ── 2. Determiners — REFERENCE (Q axis) ──────────────────── */

    seed_prime(lr, "this",      0, +1,  0, +1);  /* spatial-near + signal    */
    seed_prime(lr, "same",      0,  0,  0, +1);  /* identity = +W test       */
    seed_prime(lr, "other",     0,  0,  0, -1);  /* inverse = -W guard       */

    /* ── 3. Quantifiers — HOW MUCH (M magnitude) ─────────────── */

    seed_prime(lr, "one",       0,  0, +1,  0);  /* unit substance           */
    seed_prime(lr, "two",       0,  0, +2,  0);  /* double substance         */
    seed_prime(lr, "some",      0,  0, +1, -1);  /* substance, uncertain     */
    seed_prime(lr, "all",       0,  0, +2, +1);  /* max substance, affirmed  */
    seed_prime(lr, "much",      0,  0, +2,  0);  /* high magnitude           */
    seed_prime(lr, "more",      0,  0, +1, +1);  /* increased + signal       */

    /* ── 4. Evaluators — GOOD/BAD (Q axis) ────────────────────── */

    seed_prime(lr, "good",      0,  0,  0, +1);  /* +W = positive quality    */
    seed_prime(lr, "bad",       0,  0,  0, -1);  /* -W = negative quality    */
    seed_prime(lr, "true",      0,  0,  0, +1);  /* pure positive = GOOD     */

    /* ── 5. Descriptors — SIZE (M magnitude) ──────────────────── */

    seed_prime(lr, "big",       0,  0, +2,  0);  /* high magnitude           */
    seed_prime(lr, "small",     0,  0, -2,  0);  /* low/inverse magnitude    */

    /* ── 6. Mental predicates — INNER STATES ──────────────────── */

    seed_prime(lr, "think",    +1,  0, +1, +1);  /* process + substance + Q  */
    seed_prime(lr, "know",      0,  0, +1, +1);  /* substance held + certain */
    seed_prime(lr, "want",     +1,  0, +1, +1);  /* drive toward substance   */
    seed_prime(lr, "feel",      0,  0, +1, +1);  /* substance + quality      */
    seed_prime(lr, "see",       0, +1,  0, +1);  /* spatial + signal = P+W   */
    seed_prime(lr, "hear",      0, +1,  0, +1);  /* same bond as SEE         */

    /* ── 7. Speech — COMMUNICATION ────────────────────────────── */

    seed_prime(lr, "say",      +1,  0,  0, +1);  /* temporal signal output   */

    /* ── 8. Actions — CHANGE (T axis) ─────────────────────────── */

    seed_prime(lr, "do",       +1,  0,  0,  0);  /* pure temporal = P bond   */
    seed_prime(lr, "happen",   +1,  0,  0,  0);  /* same as DO (involuntary) */
    seed_prime(lr, "move",     +1, +1,  0,  0);  /* velocity = D/T           */

    /* ── 9. Existence — BEING ─────────────────────────────────── */

    seed_prime(lr, "is",        0,  0, +1, +1);  /* substance + identity     */
    seed_prime(lr, "was",      +1,  0, +1, +1);  /* past being = +T          */
    seed_prime(lr, "have",      0,  0, +1, +1);  /* possession = M+Q         */

    /* ── 10. Life (T + M) ─────────────────────────────────────── */

    seed_prime(lr, "live",     +1,  0, +1,  0);  /* temporal substance       */
    seed_prime(lr, "die",      -1,  0, -1,  0);  /* inverse of LIVE          */

    /* ── 11. Motion / Transfer ────────────────────────────────── */

    seed_prime(lr, "go",       +1, +1,  0,  0);  /* same as MOVE             */
    seed_prime(lr, "come",     +1, -1,  0,  0);  /* motion toward (D inward) */
    seed_prime(lr, "make",     +1,  0, +1,  0);  /* temporal + substance     */
    seed_prime(lr, "take",     +1, +1, -1,  0);  /* motion + consume         */
    seed_prime(lr, "give",     +1, +1, +1,  0);  /* motion + produce         */

    /* ── 12. Time — WHEN (T axis) ─────────────────────────────── */

    seed_prime(lr, "now",      +1,  0,  0,  0);  /* pure temporal present    */
    seed_prime(lr, "before",   +2,  0,  0,  0);  /* higher temporal (past)   */
    seed_prime(lr, "after",    -2,  0,  0,  0);  /* inverse temporal (future)*/

    /* ── 13. Place — WHERE (D axis) ───────────────────────────── */

    seed_prime(lr, "where",     0, +1,  0, +1);  /* spatial + signal         */
    seed_prime(lr, "here",      0, +1,  0, +1);  /* spatial near + signal    */
    seed_prime(lr, "above",     0, +2,  0,  0);  /* high spatial magnitude   */
    seed_prime(lr, "below",     0, -2,  0,  0);  /* inverse spatial          */
    seed_prime(lr, "in",        0, -1,  0,  0);  /* inside = -D              */
    seed_prime(lr, "on",        0, +1,  0,  0);  /* surface = +D             */

    /* ── 14. Logic — STRUCTURE (Q axis) ───────────────────────── */

    seed_prime(lr, "not",       0,  0,  0, -1);  /* Q negation = NOT gate    */
    seed_prime(lr, "if",        0,  0,  0, +1);  /* conditional = W test     */
    seed_prime(lr, "because",  +1, +1,  0, +1);  /* T+D+Q = causation/map    */
    seed_prime(lr, "can",      +1,  0,  0, +1);  /* temporal possibility     */

    /* ── 15. Degree — MODIFIERS ───────────────────────────────── */

    seed_prime(lr, "very",      0,  0,  0, +2);  /* amplified quality        */

    /* ── 16. Connectives — LOGIC GATES ────────────────────────── */

    seed_prime(lr, "and",       0,  0,  0, +1);  /* conjunction = +W         */
    seed_prime(lr, "but",       0,  0,  0, -1);  /* adversative = -W         */
    seed_prime(lr, "or",        0,  0,  0, +1);  /* disjunction = +W         */

    /* ── 17. Articles — DETERMINERS (D+Q) ─────────────────────── */

    /*
     * Articles are spatial + quality: they point at something.
     * "the" = definite (stronger signal), "a" = indefinite (weaker).
     * Both activate D+Q, the determiner signature.
     */
    seed_prime(lr, "the",       0, +1,  0, +1);  /* definite: near + signal  */
    seed_prime(lr, "a",         0, +1,  0, -1);  /* indefinite: near + weak  */
}


/* ================================================================
 * Context estimation — the heart of memetic learning
 * ================================================================
 *
 * When an unknown word appears, we estimate its coordinate from:
 *
 *   1. CENTROID: average of surrounding known words' coordinates.
 *      This places the unknown word in the right neighborhood.
 *      If the context is "the big ___ sat quietly", the centroid
 *      of {the, big, sat, quietly} gives a rough location.
 *
 *   2. BOND CONSTRAINT: the bond between the previous word and
 *      this position tells us which dimensions should be active.
 *      If prev was M-heavy (noun) and bond shows P (T changed),
 *      then this position should be T-heavy (verb).
 *
 *   3. POSITION BIAS: based on the dominant dimension of the
 *      previous word, we bias toward the expected follow-on:
 *        after noun (M) → verb (T) or adjective (Q)
 *        after verb (T) → noun (M)
 *        after adj  (Q) → noun (M)
 *        after prep (D) → noun (M)
 *        after det  (D+Q) → adj (Q) or noun (M)
 *
 * These three signals are combined: the centroid provides the
 * base, the bond constraint adjusts the dimensional signature,
 * and the position bias tilts toward the grammatically expected
 * region. The result is a reasonable first estimate even for a
 * word never seen before.
 */

/*
 * Compute the centroid (average) of a context window.
 *
 * This is a simple componentwise average. No weighting by
 * position (yet — recency weighting could be added later).
 * The centroid represents "where in coordinate space are the
 * surrounding words?"
 */
static coord4 context_centroid(const coord4 *ctx, int len) {
    coord4 c = { .t = 0, .d = 0, .m = 0, .q = 0 };
    if (len <= 0) return c;

    int st = 0, sd = 0, sm = 0, sq = 0;
    for (int i = 0; i < len; i++) {
        st += ctx[i].t;
        sd += ctx[i].d;
        sm += ctx[i].m;
        sq += ctx[i].q;
    }

    /*
     * Integer division rounds toward zero, which is fine —
     * it biases the centroid toward the origin, where the
     * identity/silence coordinate lives. This is conservative:
     * uncertain → closer to neutral.
     */
    c.t = (int8_t)(st / len);
    c.d = (int8_t)(sd / len);
    c.m = (int8_t)(sm / len);
    c.q = (int8_t)(sq / len);
    return c;
}

/*
 * Determine the dominant dimension of a coordinate.
 *
 * Returns a bitmask indicating which dimension(s) have the
 * highest absolute value. Used to classify "what part of speech
 * does this coordinate represent?"
 *
 *   BOND_P set → T-dominant (verb)
 *   BOND_C set → D-dominant (preposition)
 *   BOND_R set → M-dominant (noun)
 *   BOND_W set → Q-dominant (adjective)
 *
 * Multiple bits can be set if dimensions are tied.
 */
static int dominant_dims(coord4 c) {
    int at = c.t < 0 ? -c.t : c.t;
    int ad = c.d < 0 ? -c.d : c.d;
    int am = c.m < 0 ? -c.m : c.m;
    int aq = c.q < 0 ? -c.q : c.q;

    int mx = at;
    if (ad > mx) mx = ad;
    if (am > mx) mx = am;
    if (aq > mx) mx = aq;

    if (mx == 0) return 0; /* origin — no dominant dimension */

    int mask = 0;
    if (at == mx) mask |= BOND_P;
    if (ad == mx) mask |= BOND_C;
    if (am == mx) mask |= BOND_R;
    if (aq == mx) mask |= BOND_W;
    return mask;
}

/*
 * Position-based bias: given the previous word's dimensional
 * signature, what coordinate region do we expect for the current
 * position?
 *
 * This encodes the grammar of English (and most languages) as
 * coordinate transitions. It's not a rule engine — it's a
 * dimensional bias derived from the observation that parts of
 * speech alternate in predictable ways.
 *
 * The bias is a coordinate whose dimensions indicate "activate
 * this dimension" (+1/-1) or "no opinion" (0).
 */
static coord4 position_bias(coord4 prev) {
    coord4 bias = { .t = 0, .d = 0, .m = 0, .q = 0 };
    int dom = dominant_dims(prev);

    if (dom == 0) {
        /*
         * Previous word is at origin (silence/unknown).
         * No grammatical expectation. Slight M-bias because
         * sentences often start with nouns/pronouns.
         */
        bias.m = +1;
        return bias;
    }

    /*
     * M-dominant (noun): after a noun, expect a verb (T-heavy)
     * or adjective (Q-heavy). The most common transition in
     * language is subject→verb, so T gets the strongest bias.
     */
    if (dom & BOND_R) {
        bias.t = +1;
        return bias;
    }

    /*
     * T-dominant (verb): after a verb, expect a noun/object
     * (M-heavy). "He ate ___" → the blank is almost certainly
     * a noun or determiner. M gets the bias.
     */
    if (dom & BOND_P) {
        bias.m = +1;
        return bias;
    }

    /*
     * Q-dominant (adjective/evaluator): after an adjective,
     * expect the noun it modifies (M-heavy). "The big ___" →
     * noun. Could also chain adjectives: "the big red ___" →
     * another adjective, but M is more likely overall.
     */
    if (dom & BOND_W) {
        bias.m = +1;
        return bias;
    }

    /*
     * D-dominant (preposition/spatial): after a preposition,
     * expect a noun complement (M-heavy). "On the ___" → noun.
     * Could also be a determiner, but the determiner is D+Q
     * which will itself point at a noun.
     */
    if (dom & BOND_C) {
        bias.m = +1;
        return bias;
    }

    return bias;
}

/*
 * Clamp a value to the int8_t range.
 */
static int8_t clamp8(int v) {
    if (v > 127) return 127;
    if (v < -128) return -128;
    return (int8_t)v;
}

/*
 * Estimate a coordinate for an unknown word.
 *
 * Combines three signals:
 *   1. Context centroid (where are the surrounding words?)
 *   2. Bond constraint (what dimensions should change?)
 *   3. Position bias (what part of speech is expected?)
 *
 * The bond constraint comes from classify_bond() between the
 * previous word and the expected position. The bond's default
 * region tells us which dimensions to activate.
 *
 * Weighting: centroid provides the base. The bond default region
 * and position bias are ADDED as adjustments. The result is
 * clamped to valid int8_t range.
 *
 * This is deliberately simple. The estimate doesn't need to be
 * perfect on the first encounter — it only needs to be in the
 * right REGION. Subsequent encounters will refine it via the
 * running weighted average.
 */
static coord4 estimate_coordinate(const coord4 *ctx, int ctx_len,
                                  coord4 prev_coord) {
    /* start with context centroid */
    coord4 centroid = context_centroid(ctx, ctx_len);

    /* determine bond between previous word and this position */
    BondType bond;
    if (ctx_len > 0) {
        /* use the last context word as "previous" */
        bond = classify_bond(ctx[ctx_len - 1], centroid);
    } else {
        /* no context — start of sentence, default to R (noun) */
        bond = (BondType)BOND_R;
    }

    /* get the bond's default region bias */
    coord4 bond_bias = bond_default_regions[bond & 0xF];

    /* get the position-based bias from previous word */
    coord4 pos_bias = position_bias(prev_coord);

    /*
     * Combine: centroid + bond_bias + position_bias.
     *
     * The centroid is the neighborhood. The bond_bias adjusts
     * which dimensions should be active. The pos_bias tilts
     * toward the grammatically expected part of speech.
     *
     * All three signals contribute equally. This is intentionally
     * simple — the running average across encounters will
     * converge toward the true coordinate regardless of the
     * initial estimate's precision.
     */
    coord4 est;
    est.t = clamp8((int)centroid.t + (int)bond_bias.t + (int)pos_bias.t);
    est.d = clamp8((int)centroid.d + (int)bond_bias.d + (int)pos_bias.d);
    est.m = clamp8((int)centroid.m + (int)bond_bias.m + (int)pos_bias.m);
    est.q = clamp8((int)centroid.q + (int)bond_bias.q + (int)pos_bias.q);

    return est;
}


/* ================================================================
 * Running weighted average — coordinate refinement
 * ================================================================
 *
 * On each subsequent encounter of a known word, we update its
 * coordinate as a running weighted average:
 *
 *   new = (old * confidence + context) / (confidence + 1)
 *   confidence++
 *
 * This has the property that:
 *   - Early encounters shift the coordinate significantly
 *     (confidence is low, each new sample has high weight)
 *   - Later encounters barely shift it
 *     (confidence is high, each new sample is a drop in the ocean)
 *   - The coordinate converges toward the "true" centroid of
 *     all contexts in which the word appears
 *
 * This is the memetic equivalent of learning through exposure.
 * A child first hears "cat" in "the big cat sat" and gets a
 * rough idea (noun, entity). After hearing "cats are furry",
 * "my cat sleeps", "the cat caught a mouse", the coordinate
 * converges to the stable average across all contexts.
 *
 * Primes never update: their confidence = UINT32_MAX, so the
 * context contribution is effectively zero.
 */

static void update_coordinate(WordEntry *e, coord4 context_coord) {
    /*
     * Primes are immortal. UINT32_MAX confidence means the
     * context contribution rounds to zero in the weighted average.
     * We skip them explicitly to avoid integer overflow.
     */
    if (e->confidence == UINT32_MAX)
        return;

    /*
     * Running weighted average per dimension:
     *   new_dim = (old_dim * confidence + ctx_dim) / (confidence + 1)
     *
     * We use integer arithmetic. The division truncates toward
     * zero, which introduces a slight bias toward the origin.
     * This is acceptable — it means rarely-seen words drift
     * slightly toward neutral, which is conservative and safe.
     *
     * Cap confidence at UINT32_MAX - 1 to prevent overflow
     * and to reserve UINT32_MAX for primes.
     */
    uint32_t c = e->confidence;
    uint32_t c1 = c + 1;

    e->coord.t = clamp8((int)((int32_t)e->coord.t * (int32_t)c +
                               (int32_t)context_coord.t) / (int32_t)c1);
    e->coord.d = clamp8((int)((int32_t)e->coord.d * (int32_t)c +
                               (int32_t)context_coord.d) / (int32_t)c1);
    e->coord.m = clamp8((int)((int32_t)e->coord.m * (int32_t)c +
                               (int32_t)context_coord.m) / (int32_t)c1);
    e->coord.q = clamp8((int)((int32_t)e->coord.q * (int32_t)c +
                               (int32_t)context_coord.q) / (int32_t)c1);

    if (c < UINT32_MAX - 1)
        e->confidence = c1;
}


/* ================================================================
 * Public API implementation
 * ================================================================ */

Learner *learner_create(void) {
    Learner *lr = calloc(1, sizeof(Learner));
    if (!lr) return NULL;

    lr->vocab = calloc(LEARNER_VOCAB_SLOTS, sizeof(WordEntry));
    if (!lr->vocab) {
        free(lr);
        return NULL;
    }

    /* seed Layer 0: the 16 innate bond schemas */
    learner_seed_bonds(lr);

    return lr;
}

void learner_free(Learner *lr) {
    if (!lr) return;
    free(lr->vocab);
    free(lr);
}

coord4 learner_process_word(Learner *lr,
                            const char *word, int len,
                            const coord4 *context_window, int ctx_len) {
    /* normalize the word (lowercase, clamp length) */
    char norm[LEARNER_WORD_MAX];
    len = normalize_word(word, len, norm);

    /* skip empty words */
    if (len == 0) {
        coord4 zero = { .t = 0, .d = 0, .m = 0, .q = 0 };
        return zero;
    }

    lr->total_words_seen++;

    int slot = vocab_find_slot(lr, norm, len);
    if (slot < 0) {
        /* table full — return origin. should never happen. */
        coord4 zero = { .t = 0, .d = 0, .m = 0, .q = 0 };
        return zero;
    }

    WordEntry *e = &lr->vocab[slot];

    if (e->word[0] != '\0') {
        /*
         * KNOWN WORD — we've seen this before.
         *
         * If it's a prime (confidence == UINT32_MAX), just return
         * the fixed coordinate. No update.
         *
         * Otherwise, compute context coordinate and update via
         * running weighted average. The coordinate drifts toward
         * the average of all contexts in which this word appears.
         */
        if (e->confidence == UINT32_MAX) {
            lr->total_prime_hits++;
            return e->coord;
        }

        /* compute context-derived coordinate for this encounter */
        coord4 prev = { .t = 0, .d = 0, .m = 0, .q = 0 };
        if (ctx_len > 0)
            prev = context_window[ctx_len - 1];

        coord4 ctx_coord = estimate_coordinate(context_window, ctx_len, prev);

        /* update via running weighted average */
        update_coordinate(e, ctx_coord);
        lr->total_updated++;

        return e->coord;
    }

    /*
     * UNKNOWN WORD — first encounter. Born-index it.
     *
     * 1. Determine the previous word's coordinate (last in context window).
     * 2. Estimate this word's coordinate from context + bond + position.
     * 3. Store immediately with confidence = 1.
     * 4. Store the born_context (first-encounter coordinate, never changes).
     *
     * This is the "born-indexed" property: the word gets a coordinate
     * the INSTANT it is first seen. No need to wait for more data.
     * The estimate may be rough, but subsequent encounters will refine it.
     */
    coord4 prev = { .t = 0, .d = 0, .m = 0, .q = 0 };
    if (ctx_len > 0)
        prev = context_window[ctx_len - 1];

    coord4 est = estimate_coordinate(context_window, ctx_len, prev);

    vocab_insert(lr, slot, norm, len, est, est, 1);
    lr->total_born++;

    return est;
}

WordEntry *learner_lookup(Learner *lr, const char *word, int len) {
    char norm[LEARNER_WORD_MAX];
    len = normalize_word(word, len, norm);

    int slot = vocab_find_slot(lr, norm, len);
    if (slot < 0) return NULL;

    WordEntry *e = &lr->vocab[slot];
    if (e->word[0] == '\0') return NULL;

    return e;
}


/* ================================================================
 * Statistics
 * ================================================================ */

void learner_stats(const Learner *lr) {
    printf("=== Learner Statistics ===\n");
    printf("  vocabulary slots:   %d / %d (%.1f%% load)\n",
           lr->vocab_count, LEARNER_VOCAB_SLOTS,
           100.0 * lr->vocab_count / LEARNER_VOCAB_SLOTS);
    printf("  total words seen:   %u\n", lr->total_words_seen);
    printf("  born-indexed:       %u\n", lr->total_born);
    printf("  updated:            %u\n", lr->total_updated);
    printf("  prime hits:         %u\n", lr->total_prime_hits);
    printf("\n");

    /* confidence distribution */
    uint32_t primes = 0;
    uint32_t conf_1 = 0;       /* seen once only */
    uint32_t conf_2_10 = 0;
    uint32_t conf_11_100 = 0;
    uint32_t conf_101_1000 = 0;
    uint32_t conf_1001_plus = 0;

    /* dimensional distribution: what does the vocabulary look like? */
    int t_dom = 0, d_dom = 0, m_dom = 0, q_dom = 0, balanced = 0;

    for (int i = 0; i < LEARNER_VOCAB_SLOTS; i++) {
        const WordEntry *e = &lr->vocab[i];
        if (e->word[0] == '\0') continue;

        /* confidence bins */
        if (e->confidence == UINT32_MAX) {
            primes++;
        } else if (e->confidence == 1) {
            conf_1++;
        } else if (e->confidence <= 10) {
            conf_2_10++;
        } else if (e->confidence <= 100) {
            conf_11_100++;
        } else if (e->confidence <= 1000) {
            conf_101_1000++;
        } else {
            conf_1001_plus++;
        }

        /* dimensional classification (skip primes — they're known) */
        if (e->confidence != UINT32_MAX) {
            int dom = dominant_dims(e->coord);
            if (dom == 0) {
                balanced++;
            } else if (dom & BOND_P) {
                t_dom++;
            } else if (dom & BOND_R) {
                m_dom++;
            } else if (dom & BOND_W) {
                q_dom++;
            } else if (dom & BOND_C) {
                d_dom++;
            } else {
                balanced++;
            }
        }
    }

    printf("  Confidence distribution:\n");
    printf("    primes (fixed):   %u\n", primes);
    printf("    seen 1x:          %u\n", conf_1);
    printf("    seen 2-10x:       %u\n", conf_2_10);
    printf("    seen 11-100x:     %u\n", conf_11_100);
    printf("    seen 101-1000x:   %u\n", conf_101_1000);
    printf("    seen 1001+x:      %u\n", conf_1001_plus);
    printf("\n");

    printf("  Dimensional distribution (learned words):\n");
    printf("    T-dominant (verb):     %d\n", t_dom);
    printf("    D-dominant (prep):     %d\n", d_dom);
    printf("    M-dominant (noun):     %d\n", m_dom);
    printf("    Q-dominant (adj):      %d\n", q_dom);
    printf("    balanced/origin:       %d\n", balanced);
}


/* ================================================================
 * Save / Load — binary serialization
 * ================================================================
 *
 * File format:
 *   [4 bytes] magic:   "LRNR"
 *   [4 bytes] version: 1
 *   [4 bytes] count:   number of entries
 *   [count entries]:
 *     [32 bytes] word (null-padded)
 *     [4 bytes]  coord (t, d, m, q as int8_t)
 *     [4 bytes]  born_context (t, d, m, q as int8_t)
 *     [4 bytes]  confidence (uint32_t, little-endian)
 *
 * Total per entry: 44 bytes.
 * A 50K-word vocabulary: 50000 * 44 = ~2.2 MB.
 *
 * The bond schemas are NOT saved — they're hardcoded and
 * reloaded on create. Only the vocabulary (Layers 1+2) is
 * persisted.
 */

#define LEARNER_MAGIC  0x524E524C  /* "LRNR" in little-endian */
#define LEARNER_VERSION 1

int learner_save(const Learner *lr, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    /* header */
    uint32_t magic = LEARNER_MAGIC;
    uint32_t version = LEARNER_VERSION;
    uint32_t count = lr->vocab_count;

    fwrite(&magic, 4, 1, f);
    fwrite(&version, 4, 1, f);
    fwrite(&count, 4, 1, f);

    /* entries: only occupied slots */
    for (int i = 0; i < LEARNER_VOCAB_SLOTS; i++) {
        const WordEntry *e = &lr->vocab[i];
        if (e->word[0] == '\0') continue;

        fwrite(e->word, LEARNER_WORD_MAX, 1, f);

        int8_t cbuf[4] = { e->coord.t, e->coord.d, e->coord.m, e->coord.q };
        fwrite(cbuf, 4, 1, f);

        int8_t bbuf[4] = { e->born_context.t, e->born_context.d,
                           e->born_context.m, e->born_context.q };
        fwrite(bbuf, 4, 1, f);

        fwrite(&e->confidence, 4, 1, f);
    }

    fclose(f);
    return 0;
}

Learner *learner_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    /* read header */
    uint32_t magic, version, count;
    if (fread(&magic, 4, 1, f) != 1) goto fail;
    if (fread(&version, 4, 1, f) != 1) goto fail;
    if (fread(&count, 4, 1, f) != 1) goto fail;

    if (magic != LEARNER_MAGIC || version != LEARNER_VERSION) goto fail;

    /* create a fresh learner (seeds bonds automatically) */
    Learner *lr = learner_create();
    if (!lr) goto fail;

    /* read entries */
    for (uint32_t i = 0; i < count; i++) {
        char word[LEARNER_WORD_MAX];
        int8_t cbuf[4], bbuf[4];
        uint32_t confidence;

        if (fread(word, LEARNER_WORD_MAX, 1, f) != 1) goto fail_lr;
        if (fread(cbuf, 4, 1, f) != 1) goto fail_lr;
        if (fread(bbuf, 4, 1, f) != 1) goto fail_lr;
        if (fread(&confidence, 4, 1, f) != 1) goto fail_lr;

        coord4 coord = { .t = cbuf[0], .d = cbuf[1], .m = cbuf[2], .q = cbuf[3] };
        coord4 born  = { .t = bbuf[0], .d = bbuf[1], .m = bbuf[2], .q = bbuf[3] };

        int wlen = (int)strlen(word);
        int slot = vocab_find_slot(lr, word, wlen);
        if (slot < 0) continue; /* table full — should never happen */

        if (lr->vocab[slot].word[0] == '\0') {
            vocab_insert(lr, slot, word, wlen, coord, born, confidence);
        }
    }

    fclose(f);
    return lr;

fail_lr:
    learner_free(lr);
fail:
    fclose(f);
    return NULL;
}
