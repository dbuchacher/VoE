#ifndef BONDS_H
#define BONDS_H

#include "wave.h"
#include <stddef.h>  /* NULL */

/*
 * bonds.h -- component 2.3: bond-typed relationship classification
 *
 * THIS IS NOT ATTENTION.  This is something better.
 *
 * ── What generic attention does ─────────────────────────────────
 *
 *   Standard transformer attention computes:
 *     score(i, j) = softmax(Q_i . K_j / sqrt(d))
 *
 *   This tells you HOW MUCH position j matters to position i.
 *   A single scalar.  All relationships are the same type -- just
 *   stronger or weaker.  The model must learn to route different
 *   relationship types through different heads, and even then it
 *   can only approximate "this head handles coreference" or "this
 *   head handles subject-verb."  Nothing enforces it.
 *
 * ── What bond-typed attention does ──────────────────────────────
 *
 *   Bond classification computes:
 *     type(i, j) = which_dimensions_changed(coord[i], coord[j])
 *
 *   This tells you WHAT KIND of relationship connects i to j.
 *   Not a scalar -- a 4-bit tag from a taxonomy of 16 relationship
 *   types, derived from the four fundamental forces of the lattice:
 *
 *     P (bit 0) = temporal force   (T dimension changed)
 *     C (bit 1) = spatial force    (D dimension changed)
 *     R (bit 2) = substance force  (M dimension changed)
 *     W (bit 3) = quality force    (Q dimension changed)
 *
 *   The bitmask of which forces are active IS the relationship type.
 *   16 combinations = 16 types.  Not learned.  Built in.
 *
 * ── Why 16 is the right number ──────────────────────────────────
 *
 *   4 binary dimensions → 2^4 = 16 possible change patterns.
 *   This is the same 16 that appears everywhere in the lattice:
 *   16 bond patterns, 16 force combinations, 16 relationship types.
 *
 *   Transformers typically use 8-16 attention heads per layer.
 *   Not a coincidence.  They're trying to learn what the lattice
 *   provides for free: a taxonomy of ~16 relationship types.
 *   But they learn it from data, imperfectly, without guarantees.
 *   We get it from geometry, exactly, by construction.
 *
 * ── The 16 bond types ───────────────────────────────────────────
 *
 *   Mask  Forces   Name            Relationship
 *   ────  ───────  ──────────────  ────────────────────────────
 *    0    (none)   identity        same word / self-loop
 *    1    P        reference       one word points to another
 *    2    C        invocation      verb calls its arguments
 *    4    R        continuation    sequential / list linkage
 *    8    W        evaluation      adjective evaluates noun
 *    3    P+C      traversal       subject traverses to object
 *    5    P+R      accumulation    list items building toward total
 *    6    C+R      iteration       repeated action
 *    9    P+W      selection       filtering / choosing
 *   10    C+W      conditional     if-then relationship
 *   12    R+W      search          looking / scanning
 *    7    P+C+R    reduction       summary / totaling
 *   11    P+C+W    transformation  change of type
 *   13    P+R+W    progressive     gradual change
 *   14    C+R+W    causation       because / therefore
 *   15    P+C+R+W  full pipeline   complex multi-step reasoning
 *
 * ── Usage ───────────────────────────────────────────────────────
 *
 *   Given word coordinates (from the born-index or from tokenize +
 *   wave encoding), classify_bond tells you the relationship type
 *   between any two words.  walk_bond_sequence gives you the full
 *   grammar of a walk -- the sequence of relationship types that
 *   connects each adjacent pair.
 *
 *   This replaces the attention matrix with a bond matrix:
 *   instead of N^2 learned scalars, you get N^2 typed labels
 *   from the geometry alone.  The model still needs to learn
 *   weights, but now it has STRUCTURE to hang them on.
 */


/* ================================================================
 * BondType — the 16 relationship types
 * ================================================================
 *
 * Each value IS its bitmask.  Built from BOND_P, BOND_C, BOND_R,
 * BOND_W which are already defined as macros in wave.h (they are
 * the fundamental force bits: 1, 2, 4, 8).
 *
 * We reuse those macros and define an enum for the compound types.
 * BOND_TRAVERSAL = 3 = BOND_P|BOND_C = bits 0+1.
 * You can bitwise-OR individual forces to get compound types, and
 * the result is always a valid BondType.
 *
 * The lattice names (fold, slurp, fix, etc.) come from the functional
 * programming operations that each force combination corresponds to.
 * The relationship names come from linguistics.  Same structure,
 * different vocabulary.
 */

typedef enum {
    BOND_NONE           =  0,  /* (none)  — identity / same position         */

    /* single forces — values match BOND_P/C/R/W macros from wave.h:
     *   BOND_P = 1   P       — reference (temporal link)
     *   BOND_C = 2   C       — invocation (spatial call)
     *   BOND_R = 4   R       — continuation (substance flow)
     *   BOND_W = 8   W       — evaluation (quality test)
     */

    /* two-force compounds */
    BOND_PC             =  3,  /* P+C     — traversal (fold)                  */
    BOND_PR             =  5,  /* P+R     — accumulation (slurp)              */
    BOND_CR             =  6,  /* C+R     — iteration (fix)                   */
    BOND_PW             =  9,  /* P+W     — selection (filter)                */
    BOND_CW             = 10,  /* C+W     — conditional (maybe)               */
    BOND_RW             = 12,  /* R+W     — search (take_while)               */

    /* three-force compounds */
    BOND_PCR            =  7,  /* P+C+R   — reduction (foldl)                 */
    BOND_PCW            = 11,  /* P+C+W   — transformation (map)              */
    BOND_PRW            = 13,  /* P+R+W   — progressive (scan)                */
    BOND_CRW            = 14,  /* C+R+W   — causation (bind)                  */

    /* all four forces */
    BOND_PCRW           = 15   /* P+C+R+W — full pipeline (hylo)              */
} BondType;


/* ================================================================
 * bond_name — human-readable name for each bond type
 * ================================================================
 *
 * Returns the linguistic name.  The lattice name (fold, slurp, etc.)
 * is in the enum comments above.
 *
 * Index by BondType value directly — they're 0-15.
 */

static inline const char *bond_name(BondType b) {
    static const char *names[16] = {
        "identity",        /*  0: (none)   */
        "reference",       /*  1: P        */
        "invocation",      /*  2: C        */
        "traversal",       /*  3: P+C      */
        "continuation",    /*  4: R        */
        "accumulation",    /*  5: P+R      */
        "iteration",       /*  6: C+R      */
        "reduction",       /*  7: P+C+R    */
        "evaluation",      /*  8: W        */
        "selection",       /*  9: P+W      */
        "conditional",     /* 10: C+W      */
        "transformation",  /* 11: P+C+W    */
        "search",          /* 12: R+W      */
        "progressive",     /* 13: P+R+W    */
        "causation",       /* 14: C+R+W    */
        "full pipeline"    /* 15: P+C+R+W  */
    };
    return names[b & 0xF];
}


/* ================================================================
 * classify_bond — what type of relationship connects two words?
 * ================================================================
 *
 * Given two word coordinates, determine which dimensions changed.
 * Each changed dimension activates its corresponding force.
 * The bitmask of active forces IS the bond type.
 *
 * This is the core insight: the GEOMETRY of the coordinate space
 * tells you the GRAMMAR of the relationship.
 *
 *   - T changed → temporal relationship (P force: reference)
 *   - D changed → spatial relationship  (C force: invocation)
 *   - M changed → substance relationship (R force: continuation)
 *   - Q changed → quality relationship  (W force: evaluation)
 *
 * Multiple dimensions changing → compound relationship.
 * "John hit the ball" has a traversal bond (P+C) between
 * "John" and "ball" because both time and place shift.
 *
 * The sign of the change tells you direction (see classify_bond_directed)
 * but the TYPE is determined only by WHICH dimensions changed, not
 * by how much or in which direction.
 *
 * Contrast with dot-product attention:
 *   dot(a, b) = a.t*b.t + a.x*b.x + a.m*b.m + a.q*b.q
 *   → one scalar, all dimensions mixed, no type information
 *
 *   classify_bond(a, b) = P*(t!=t') | C*(d!=d') | R*(m!=m') | W*(q!=q')
 *   → 4-bit tag, each dimension tracked independently, typed
 */

static inline BondType classify_bond(coord4 a, coord4 b) {
    int mask = 0;
    if (a.t != b.t) mask |= BOND_P;
    if (a.x != b.x) mask |= BOND_C;
    if (a.m != b.m) mask |= BOND_R;
    if (a.q != b.q) mask |= BOND_W;
    return (BondType)mask;
}


/* ================================================================
 * classify_bond_directed — bond type + direction of each force
 * ================================================================
 *
 * Same classification as classify_bond, but also encodes the
 * DIRECTION of each force.  This matters because:
 *
 *   Forward (+) = consuming / reading / pulling
 *   Backward (-) = producing / writing / pushing
 *
 * A reference bond from "it" to "cat" is forward-P (reading the
 * referent).  A reference from "cat" to "it" would be backward-P
 * (producing the referent that "it" will consume).
 *
 * Encoding (8 bits total):
 *
 *   Bits [3:0] = bond type mask (which forces are active)
 *                Same as classify_bond.
 *
 *   Bits [7:4] = direction flags (1 = forward/positive change)
 *                Bit 4: P direction (1 if b.t > a.t, 0 if b.t < a.t)
 *                Bit 5: C direction (1 if b.x > a.x, 0 if b.x < a.x)
 *                Bit 6: R direction (1 if b.m > a.m, 0 if b.m < a.m)
 *                Bit 7: W direction (1 if b.q > a.q, 0 if b.q < a.q)
 *
 *   Direction bits are only meaningful for active forces.
 *   If a force is not active (type bit = 0), its direction bit is 0.
 *
 * To extract:
 *   BondType type = result & 0x0F;
 *   int dirs      = (result >> 4) & 0x0F;
 *   int p_fwd     = dirs & 1;     // P direction: 1=forward, 0=backward
 *   int c_fwd     = (dirs>>1)&1;  // C direction
 *   int r_fwd     = (dirs>>2)&1;  // R direction
 *   int w_fwd     = (dirs>>3)&1;  // W direction
 */

static inline uint8_t classify_bond_directed(coord4 from, coord4 to) {
    int mask = 0;
    int dirs = 0;

    if (from.t != to.t) {
        mask |= BOND_P;
        if (to.t > from.t) dirs |= 1;  /* +P: forward temporal */
    }
    if (from.x != to.x) {
        mask |= BOND_C;
        if (to.x > from.x) dirs |= 2;  /* +C: forward spatial */
    }
    if (from.m != to.m) {
        mask |= BOND_R;
        if (to.m > from.m) dirs |= 4;  /* +R: forward substance */
    }
    if (from.q != to.q) {
        mask |= BOND_W;
        if (to.q > from.q) dirs |= 8;  /* +W: forward quality */
    }

    return (uint8_t)((dirs << 4) | mask);
}


/* helpers to unpack a directed bond result */

static inline BondType directed_bond_type(uint8_t db) {
    return (BondType)(db & 0x0F);
}

static inline int directed_bond_dirs(uint8_t db) {
    return (db >> 4) & 0x0F;
}

/* is force f (BOND_P/C/R/W) going forward in this directed bond? */
static inline int directed_is_forward(uint8_t db, int f) {
    /* f is a single-bit mask: 1, 2, 4, or 8 */
    /* the direction bit for force f sits at bit position (4 + log2(f)) */
    /* since f is already a power of 2, (db >> 4) & f works directly */
    return ((db >> 4) & f) != 0;
}


/* ================================================================
 * BondFull — bond type + direction + magnitude per dimension
 * ================================================================
 *
 * The guide (lines 133-153) shows that magnitude within a bond
 * pattern selects the specific operation.  E.g. delta at |phi|=1
 * is "test", at |phi|=3 is "AND", at |phi|=5 is "XOR".
 *
 * classify_bond gives you the 4-bit pattern (WHAT kind of bond).
 * classify_bond_directed adds direction (WHICH way).
 * classify_bond_full adds magnitude (HOW MUCH), which selects the
 * specific operation variant within a bond pattern.
 *
 * The existing functions remain for backward compatibility.
 */

typedef struct {
    BondType type;          /* the 4-bit pattern (0-15) */
    uint8_t  direction;     /* per-dimension direction bits (same as
                               upper nibble of classify_bond_directed) */
    int8_t   magnitudes[4]; /* per-dimension magnitude of change:
                               [0]=T(P), [1]=X(C), [2]=M(R), [3]=Q(W) */
} BondFull;

static inline BondFull classify_bond_full(coord4 a, coord4 b) {
    BondFull bf;
    bf.type = BOND_NONE;
    bf.direction = 0;

    int dt = (int)b.t - (int)a.t;
    int dx = (int)b.x - (int)a.x;
    int dm = (int)b.m - (int)a.m;
    int dq = (int)b.q - (int)a.q;

    int mask = 0;
    int dirs = 0;

    if (dt != 0) { mask |= BOND_P; if (dt > 0) dirs |= 1; }
    if (dx != 0) { mask |= BOND_C; if (dx > 0) dirs |= 2; }
    if (dm != 0) { mask |= BOND_R; if (dm > 0) dirs |= 4; }
    if (dq != 0) { mask |= BOND_W; if (dq > 0) dirs |= 8; }

    bf.type = (BondType)mask;
    bf.direction = (uint8_t)dirs;

    /* magnitudes: absolute value of per-dimension delta, clamped to int8 */
    bf.magnitudes[0] = (int8_t)(dt < 0 ? -dt : dt);
    bf.magnitudes[1] = (int8_t)(dx < 0 ? -dx : dx);
    bf.magnitudes[2] = (int8_t)(dm < 0 ? -dm : dm);
    bf.magnitudes[3] = (int8_t)(dq < 0 ? -dq : dq);

    return bf;
}


/* ================================================================
 * bond_relationship_str — human-readable description of relationship
 * ================================================================
 *
 * Given two word coordinates, returns a verb/phrase describing
 * the relationship.  These are the active-voice descriptions
 * suitable for printing: "a REFERS TO b", "a CALLS b", etc.
 *
 * Uses the directed bond to pick appropriate verb form.
 */

static inline const char *bond_relationship_str(coord4 a, coord4 b) {
    BondType type = classify_bond(a, b);

    /*
     * Each relationship type gets a human-readable verb.
     * These are indexed by the 4-bit bond mask directly.
     *
     * The linguistic labels come from the primes.md derivation:
     *   P = reference ("refers to")
     *   C = invocation ("calls")
     *   R = continuation ("continues from")
     *   W = evaluation ("evaluates")
     *   ...and compound combinations.
     */
    static const char *verbs[16] = {
        "equals",              /*  0: identity — same coordinate           */
        "refers to",           /*  1: P — temporal reference               */
        "calls",               /*  2: C — spatial invocation               */
        "traverses to",        /*  3: P+C — subject→object traversal      */
        "continues from",      /*  4: R — sequential continuation          */
        "accumulates with",    /*  5: P+R — list building                  */
        "iterates over",       /*  6: C+R — repeated action                */
        "reduces",             /*  7: P+C+R — total/summary                */
        "evaluates",           /*  8: W — quality judgment                 */
        "selects from",        /*  9: P+W — filtering                     */
        "conditions on",       /* 10: C+W — if-then                       */
        "transforms",          /* 11: P+C+W — type change                 */
        "searches",            /* 12: R+W — scanning                      */
        "progresses toward",   /* 13: P+R+W — gradual change              */
        "causes",              /* 14: C+R+W — causal link                 */
        "pipelines through"    /* 15: P+C+R+W — full reasoning chain      */
    };

    return verbs[type & 0xF];
}


/* ================================================================
 * bond_directed_name — force name with direction
 * ================================================================
 *
 * Returns the proper directed name for a single-force bond:
 *
 *   P forward = "read"      P backward = "write"
 *   C forward = "call"      C backward = "return"
 *   R forward = "advance"   R backward = "rewind"
 *   W forward = "test"      W backward = "guard"
 *
 * For compound bonds (multiple forces active), returns NULL.
 * Use bond_lattice_name() for compound bond naming.
 *
 * The forward/backward duality is the matter/antimatter distinction:
 * write IS the antimatter of read (pi-bar), return IS the antimatter
 * of call (comp-bar), etc.
 */

static inline const char *bond_directed_name(uint8_t directed_bond) {
    BondType type = directed_bond_type(directed_bond);
    int dirs = directed_bond_dirs(directed_bond);

    /* forward names (positive direction on the active dimension) */
    static const char *fwd[16] = {
        NULL,       /*  0: identity — no direction */
        "read",     /*  1: P  +T */
        "call",     /*  2: C  +X */
        NULL,       /*  3: P+C    */
        "advance",  /*  4: R  +M */
        NULL,       /*  5: P+R    */
        NULL,       /*  6: C+R    */
        NULL,       /*  7: P+C+R  */
        "test",     /*  8: W  +Q */
        NULL,       /*  9: P+W    */
        NULL,       /* 10: C+W    */
        NULL,       /* 11: P+C+W  */
        NULL,       /* 12: R+W    */
        NULL,       /* 13: P+R+W  */
        NULL,       /* 14: C+R+W  */
        NULL        /* 15: P+C+R+W */
    };

    /* backward names (negative direction = antimatter) */
    static const char *bwd[16] = {
        NULL,       /*  0: identity */
        "write",    /*  1: P  -T */
        "return",   /*  2: C  -X */
        NULL,       /*  3: P+C    */
        "rewind",   /*  4: R  -M */
        NULL,       /*  5: P+R    */
        NULL,       /*  6: C+R    */
        NULL,       /*  7: P+C+R  */
        "guard",    /*  8: W  -Q */
        NULL,       /*  9: P+W    */
        NULL,       /* 10: C+W    */
        NULL,       /* 11: P+C+W  */
        NULL,       /* 12: R+W    */
        NULL,       /* 13: P+R+W  */
        NULL,       /* 14: C+R+W  */
        NULL        /* 15: P+C+R+W */
    };

    /* only defined for single-force bonds */
    if (type != BOND_P && type != (BondType)BOND_C &&
        type != (BondType)BOND_R && type != (BondType)BOND_W)
        return NULL;

    /* direction bit for this force matches the force bit position */
    int is_fwd = (dirs & type) != 0;
    return is_fwd ? fwd[type] : bwd[type];
}


/* ================================================================
 * walk_bond_sequence — extract the grammar of a walk
 * ================================================================
 *
 * A walk is a sequence of word coordinates.  The bond sequence is
 * the series of relationship types connecting each adjacent pair.
 *
 *   walk:  [w0, w1, w2, w3, w4]
 *   bonds: [b01, b12, b23, b34]    (len-1 entries)
 *
 * The bond sequence IS the grammar of the sentence.  Consider:
 *
 *   "The big cat sat on the mat"
 *    ─W──R──PC──P──C──W──PC─
 *    eval cont trav ref inv eval trav
 *
 * This tells you: "the" evaluates "big" (adjective modifying),
 * "big" continues to "cat" (descriptor sequence), "cat" traverses
 * to "sat" (subject→verb), "sat" refers back (temporal anchor),
 * "on" invokes (prepositional call), "the" evaluates "mat", and
 * "mat" completes the traversal.
 *
 * Traditional parsing gives you a tree with labeled edges.
 * Bond sequences give you the same information as a flat array
 * of 4-bit tags, with no parser, no grammar rules, no training.
 * The geometry does the parsing.
 *
 * Caller must ensure bonds_out has space for at least (len - 1)
 * entries.  If len <= 1, no bonds are written.
 */

static inline void walk_bond_sequence(const coord4 *walk, int len,
                                       BondType *bonds_out) {
    for (int i = 0; i < len - 1; i++) {
        bonds_out[i] = classify_bond(walk[i], walk[i + 1]);
    }
}


/* ================================================================
 * walk_bond_sequence_directed — directed grammar of a walk
 * ================================================================
 *
 * Same as walk_bond_sequence but stores the full directed bond
 * (type + direction in a uint8_t) for each adjacent pair.
 * Use directed_bond_type() and directed_bond_dirs() to unpack.
 */

static inline void walk_bond_sequence_directed(const coord4 *walk, int len,
                                                uint8_t *dbonds_out) {
    for (int i = 0; i < len - 1; i++) {
        dbonds_out[i] = classify_bond_directed(walk[i], walk[i + 1]);
    }
}


/* ================================================================
 * bond_force_count — how many forces are active in this bond?
 * ================================================================
 *
 * Population count of the 4-bit mask.  Range: 0 to 4.
 *
 * This is the "complexity" of the relationship:
 *   0 = identity (no change)
 *   1 = simple (one force)
 *   2 = compound (two forces)
 *   3 = complex (three forces)
 *   4 = full pipeline (all four forces)
 *
 * Empirical prediction: most adjacent-word bonds in natural
 * language should be 1 or 2 forces.  Bonds of 3+ should be
 * rarer and mark structurally important transitions (clause
 * boundaries, topic shifts, causal chains).
 */

static inline int bond_force_count(BondType b) {
    int n = b & 0xF;
    /* popcount of 4 bits */
    n = (n & 0x5) + ((n >> 1) & 0x5);
    n = (n & 0x3) + ((n >> 2) & 0x3);
    return n;
}


/* ================================================================
 * bond_has_force — does this bond include a specific force?
 * ================================================================
 *
 * Example: bond_has_force(BOND_PCR, BOND_C) → 1 (yes, C is active)
 *          bond_has_force(BOND_PCR, BOND_W) → 0 (no, W is not)
 */

static inline int bond_has_force(BondType b, int force) {
    return (b & force) != 0;
}


/* ================================================================
 * bond_lattice_name — the functional-programming name
 * ================================================================
 *
 * The lattice operations map 1:1 to functional programming
 * combinators.  This returns those names for anyone who thinks
 * in Haskell rather than linguistics:
 *
 *   P     = read     C     = call     R     = loop     W     = test
 *   P+C   = fold     P+R   = slurp    C+R   = fix
 *   P+W   = filter   C+W   = maybe    R+W   = take_while
 *   P+C+R = foldl    P+C+W = map      P+R+W = scan
 *   C+R+W = bind     P+C+R+W = hylo
 */

static inline const char *bond_lattice_name(BondType b) {
    static const char *names[16] = {
        "id",         /*  0 */
        "read",       /*  1: P         */
        "call",       /*  2: C         */
        "fold",       /*  3: P+C       */
        "loop",       /*  4: R         */
        "slurp",      /*  5: P+R       */
        "fix",        /*  6: C+R       */
        "foldl",      /*  7: P+C+R     */
        "test",       /*  8: W         */
        "filter",     /*  9: P+W       */
        "maybe",      /* 10: C+W       */
        "map",        /* 11: P+C+W     */
        "take_while", /* 12: R+W       */
        "scan",       /* 13: P+R+W     */
        "bind",       /* 14: C+R+W     */
        "hylo"        /* 15: P+C+R+W   */
    };
    return names[b & 0xF];
}


#endif /* BONDS_H */
