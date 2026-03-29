#ifndef DISTANCE_H
#define DISTANCE_H

#include <math.h>
#include "wave.h"

/*
 * distance.h -- coordinate distance + RC relevance decay
 *
 * Components 2.1 (distance) and 4.2 (attention weighting) of the
 * lattice-native intelligence system.
 *
 * These are tightly coupled because decay IS the attention function.
 * There are no learned weights here.  No backpropagation.  The physics
 * of the lattice determines the attention pattern:
 *
 *   relevance = exp(-walk_distance / (R * C))
 *
 * where R = connection resistance (base impedance + clause penalty)
 * and   C = token capacitance   (|mu| — substance dimension weight).
 *
 * This REPLACES learned attention weights.  The 1/sqrt(d_k) scaling in
 * standard transformers is a constant damper.  RC decay is a per-token
 * damper: each word gets its own decay rate based on its content.
 *
 * High-C tokens (nouns, proper nouns, numbers — high |mu|) persist
 * across long distances.  Low-C tokens (articles, prepositions — low
 * |mu|) become irrelevant quickly.  High-R connections (across clause
 * boundaries) decay faster than low-R connections (within a phrase).
 *
 * The constant scaling was the first approximation.
 * Per-token RC is the full solution.
 *
 * See primes.md, Prediction 13: "Half-Life = RC Relevance Decay".
 * See mind: C = 2mc^2/h, proportional to MASS (mu dimension).
 *
 * Dependencies: wave.h (coord4 type), math.h (sqrtf, expf)
 */


/* ================================================================
 * 1. coord4_chebyshev -- L-infinity distance in 4D (PRIMARY)
 * ================================================================
 *
 * L-infinity (Chebyshev) norm of the difference vector.
 * max(|dt|, |dx|, |dm|, |dq|).
 *
 * This is the NATIVE metric of the lattice.  The guide says:
 *   "Degree is the maximum absolute exponent in a tuple."
 *   "degree N  max |coord| = 2N"
 *
 * Shells are defined by L-infinity, not L2.  A point at (3,0,0,0)
 * and a point at (3,3,3,3) are on the SAME shell (degree = 3).
 * L2 would place them at distance 3 vs 6 — wrong for the lattice.
 * L-infinity correctly says both are shell 3.
 *
 * Chebyshev distance counts the minimum number of king-moves on
 * a chessboard.  On a discrete integer lattice, this is the
 * natural metric: how many shells apart are two points?
 */

static inline int coord4_chebyshev(coord4 a, coord4 b) {
    int dt = a.t - b.t; if (dt < 0) dt = -dt;
    int dx = a.x - b.x; if (dx < 0) dx = -dx;
    int dm = a.m - b.m; if (dm < 0) dm = -dm;
    int dq = a.q - b.q; if (dq < 0) dq = -dq;
    int max = dt;
    if (dx > max) max = dx;
    if (dm > max) max = dm;
    if (dq > max) max = dq;
    return max;
}


/* ================================================================
 * 2. coord4_distance -- primary distance (Chebyshev)
 * ================================================================
 *
 * The primary distance function uses L-infinity (Chebyshev)
 * because the lattice defines shells by max|exponent|.
 *
 * Returns float for compatibility with callers expecting float.
 */

static inline float coord4_distance(coord4 a, coord4 b) {
    return (float)coord4_chebyshev(a, b);
}


/* ================================================================
 * 3. coord4_l2 -- euclidean distance in 4D (alternative)
 * ================================================================
 *
 * L2 norm of the difference vector.  Exact geometric distance
 * in continuous 4D space.  Useful where smooth gradients matter
 * but NOT the native lattice metric.
 *
 * NOTE: This function also exists in primes_lookup.h (identical).
 * Both are static inline so there is no linker conflict.
 */

static inline float coord4_l2(coord4 a, coord4 b) {
    int dt = a.t - b.t;
    int dx = a.x - b.x;
    int dm = a.m - b.m;
    int dq = a.q - b.q;
    return sqrtf((float)(dt*dt + dx*dx + dm*dm + dq*dq));
}


/* ================================================================
 * 4. coord4_manhattan -- L1 distance (alternative)
 * ================================================================
 *
 * Sum of absolute differences.  No square root.  Good enough for
 * binning, neighbor checks, and anywhere you need a fast distance
 * that preserves ordering.
 *
 * NOTE: Also exists in primes_lookup.h (identical implementation).
 * Same static-inline story as coord4_l2 above.
 */

static inline int coord4_manhattan(coord4 a, coord4 b) {
    int dt = a.t - b.t; if (dt < 0) dt = -dt;
    int dx = a.x - b.x; if (dx < 0) dx = -dx;
    int dm = a.m - b.m; if (dm < 0) dm = -dm;
    int dq = a.q - b.q; if (dq < 0) dq = -dq;
    return dt + dx + dm + dq;
}


/* ================================================================
 * 5. coord4_dim_distance -- distance along ONE dimension
 * ================================================================
 *
 * Returns |a[dim] - b[dim]| for a single axis.
 *
 *   dim 0 = T (temporal / when / duration)
 *   dim 1 = X (spatial  / where / place)
 *   dim 2 = M (mass     / what  / manner)
 *   dim 3 = Q (charge   / how   / voicing)
 *
 * Used by dimensional attention heads: a T-head attends only to
 * temporal distance, an M-head only to substance distance, etc.
 * Four dimensions, four heads.  No hyperparameter search.
 */

static inline int coord4_dim_distance(coord4 a, coord4 b, int dim) {
    int d;
    switch (dim) {
    case 0:  d = a.t - b.t; break;
    case 1:  d = a.x - b.x; break;
    case 2:  d = a.m - b.m; break;
    case 3:  d = a.q - b.q; break;
    default: d = 0;         break;
    }
    return d < 0 ? -d : d;
}


/* ================================================================
 * 6. token_capacitance -- how much information this word carries
 * ================================================================
 *
 * Capacitance = |mu| (absolute value of the mass dimension).
 *
 * The lattice says C = 2mc^2/h, proportional to MASS.  In the
 * coordinate system, mass is the mu dimension.  A word's persistence
 * in context is determined by how much SUBSTANCE it carries:
 *
 *   High |mu| (content words):
 *     - Nouns: "Einstein", "theory", "electron" — high substance
 *     - These have large |mu| coordinates
 *     -> High capacitance -> persists long in context
 *
 *   Low |mu| (function words):
 *     - Articles/prepositions: "the", "a", "of", "is"
 *     - These have small |mu| (often 0)
 *     -> Low capacitance -> decays fast
 *
 * This is NOT a heuristic.  The mu dimension IS the mass/substance
 * axis by construction (wave.h: M = manner/openness = how much
 * substance flows).  The lattice derivation (mind: Half-Life = RC,
 * C = 2mc^2/h) specifically says capacitance is proportional to mass.
 *
 * Using all 4 dimensions (L2 magnitude) was wrong: it conflated
 * temporal, spatial, and charge properties with substance.  A word
 * can be temporally specific (high |T|) but carry little substance
 * (low |M|) — "now" vs "electron".  Only mu determines persistence.
 *
 * Floor at 0.5 to prevent degenerate decay.  A token with C near 0
 * would vanish instantly — the floor ensures even function words
 * have a minimum half-life of a few tokens.
 */

static inline float token_capacitance(coord4 c) {
    float C = (float)(c.m < 0 ? -c.m : c.m);  /* abs(mu) */
    return C > 0.5f ? C : 0.5f;  /* minimum capacitance */
}


/* ================================================================
 * 7. connection_resistance -- how hard it is to reach a token
 * ================================================================
 *
 * R = 1 + clause_depth * 2
 *
 * CRITICAL FIX: R is a property of the CONNECTION (impedance),
 * NOT proportional to walk_distance.
 *
 * The old formula R = walk_distance * (1 + clause_depth) had a
 * fatal degeneracy: at clause_depth=0, R = walk_distance, so
 *   relevance = exp(-walk_distance / (walk_distance * C))
 *             = exp(-1/C)
 * This is a CONSTANT regardless of distance!  A word 5 tokens
 * away had the same relevance as a word 500 tokens away.  The
 * distance canceled out completely.
 *
 * The fix: R is base resistance (1.0) plus a penalty per clause
 * boundary crossed (2.0 per level).  Walk_distance enters ONLY
 * through the numerator of the decay formula:
 *
 *   relevance = exp(-walk_distance / (R * C))
 *
 * Now walk_distance matters:
 *   - Same clause (depth=0): R=1, tau=C.
 *     "Einstein" (C~1.0) at distance 5: exp(-5/1.0) = 0.0067
 *     "Einstein" (C~1.0) at distance 1: exp(-1/1.0) = 0.37
 *     Distance discriminates!
 *
 *   - Across 1 clause boundary (depth=1): R=3, tau=3*C.
 *     Same word at distance 5: exp(-5/3.0) = 0.19
 *     Clause boundary adds resistance, slowing the decay.
 *
 *   - Across 2 clause boundaries (depth=2): R=5, tau=5*C.
 *     Same word at distance 5: exp(-5/5.0) = 0.37
 *     More embedding = more resistance = slower decay.
 *
 * The base resistance of 1.0 means within a clause, decay is
 * governed purely by capacitance (C).  Each clause boundary
 * adds 2.0 to the resistance, softening the decay curve to
 * allow cross-clause references for high-C words.
 */

static inline float connection_resistance(int walk_distance, int clause_depth) {
    (void)walk_distance;  /* R is connection impedance, not distance */
    float R = 1.0f + (float)clause_depth * 2.0f;
    return R;
}


/* ================================================================
 * 8. rc_relevance -- the attention weight for one token
 * ================================================================
 *
 * relevance = exp(-walk_distance / (R * C))
 *
 * This is the RC time constant from electrical circuit theory
 * applied to language.  The token IS a capacitor.  The syntactic
 * path IS a resistor.  The relevance IS the voltage decay.
 *
 * R * C = time constant (in tokens).  A token remains significantly
 * relevant for roughly R*C tokens, then exponentially fades.
 *
 * With the corrected formulas (C = |mu|, R = 1 + 2*clause_depth):
 *
 *   "electron" at distance 10, same clause:
 *     C = 1.0 (|mu|=1, manner=closed for stops)
 *     R = 1.0 (no clause boundary)
 *     tau = 1.0, relevance = exp(-10/1.0) = 0.000045 (very faded)
 *
 *   "electron" at distance 2, same clause:
 *     tau = 1.0, relevance = exp(-2/1.0) = 0.14 (still relevant)
 *
 *   A high-mu word (mu=3) at distance 10, same clause:
 *     C = 3.0, R = 1.0, tau = 3.0
 *     relevance = exp(-10/3.0) = 0.036 (persists longer)
 *
 *   Same word across 1 clause boundary:
 *     C = 3.0, R = 3.0, tau = 9.0
 *     relevance = exp(-10/9.0) = 0.33 (clause adds resistance)
 *
 * The key: high-C words (high |mu|) persist longer.  Low-C words
 * (low |mu|) fade fast.  Clause boundaries add resistance that
 * stretches the decay curve.  Walk distance always discriminates.
 */

static inline float rc_relevance(coord4 word, int walk_distance, int clause_depth) {
    float C = token_capacitance(word);
    float R = connection_resistance(walk_distance, clause_depth);
    float tau = R * C;  /* RC time constant */
    return expf(-(float)walk_distance / tau);
}


/* ================================================================
 * 9. rc_attention_weights -- full attention vector, no training
 * ================================================================
 *
 * Computes RC-decay attention weights for every position in the
 * context relative to query_pos.  This REPLACES the Q*K^T/sqrt(d)
 * computation in standard transformers.
 *
 * For each position i:
 *   raw_weight[i] = rc_relevance(context[i], |query_pos - i|, 0)
 *
 * Then normalize so weights sum to 1 (softmax-like, but the
 * distribution comes from physics, not learned projections).
 *
 * clause_depth is 0 for all positions in this version.  A future
 * version will take a clause_depth array from the parser (component
 * 3.x) so that subordination boundaries increase resistance.
 *
 * The query position attends to itself with weight determined by
 * its own capacitance at distance 0 (R floors to 1, so
 * self-relevance = exp(0) = 1.0 before normalization).
 *
 * This function is O(n) per query position.  For a full n*n
 * attention matrix, call it n times = O(n^2), same as standard
 * attention.  But the weights are COMPUTED, not LEARNED.  No
 * parameter matrix.  No gradient.  No training.
 */

static inline void rc_attention_weights(const coord4 *context, int n,
                                        int query_pos, float *weights_out)
{
    float sum = 0.0f;
    int i;

    for (i = 0; i < n; i++) {
        int walk = query_pos - i;
        if (walk < 0) walk = -walk;

        if (walk == 0) {
            /* self-attention: distance 0, R floors to 1, exp(0) = 1 */
            weights_out[i] = 1.0f;
        } else {
            weights_out[i] = rc_relevance(context[i], walk, 0);
        }
        sum += weights_out[i];
    }

    /* normalize to sum to 1 */
    if (sum > 0.0f) {
        float inv = 1.0f / sum;
        for (i = 0; i < n; i++) {
            weights_out[i] *= inv;
        }
    }
}


#endif /* DISTANCE_H */
