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
 * where R = connection resistance (how far, how many clause boundaries)
 * and   C = token capacitance   (how much information the word carries).
 *
 * This REPLACES learned attention weights.  The 1/sqrt(d_k) scaling in
 * standard transformers is a constant damper.  RC decay is a per-token
 * damper: each word gets its own decay rate based on its content.
 *
 * High-C tokens (nouns, proper nouns, numbers) persist across long
 * distances.  Low-C tokens (articles, prepositions) become irrelevant
 * quickly.  High-R connections (across clause boundaries) decay faster
 * than low-R connections (within a phrase).
 *
 * The constant scaling was the first approximation.
 * Per-token RC is the full solution.
 *
 * See primes.md, Prediction 13: "Half-Life = RC Relevance Decay".
 *
 * Dependencies: wave.h (coord4 type), math.h (sqrtf, expf)
 */


/* ================================================================
 * 1. coord4_distance -- euclidean distance in 4D
 * ================================================================
 *
 * L2 norm of the difference vector.  Exact geometric distance
 * in the lattice coordinate space.
 *
 * NOTE: This function also exists in primes_lookup.h (identical).
 * Both are static inline so there is no linker conflict.  We
 * provide it here so distance.h is self-contained (only needs
 * wave.h), but if you already include primes_lookup.h, either
 * definition works.
 */

static inline float coord4_distance(coord4 a, coord4 b) {
    int dt = a.t - b.t;
    int dd = a.d - b.d;
    int dm = a.m - b.m;
    int dq = a.q - b.q;
    return sqrtf((float)(dt*dt + dd*dd + dm*dm + dq*dq));
}


/* ================================================================
 * 2. coord4_manhattan -- L1 distance (cheaper)
 * ================================================================
 *
 * Sum of absolute differences.  No square root.  Good enough for
 * binning, neighbor checks, and anywhere you need a fast distance
 * that preserves ordering.
 *
 * NOTE: Also exists in primes_lookup.h (identical implementation).
 * Same static-inline story as coord4_distance above.
 */

static inline int coord4_manhattan(coord4 a, coord4 b) {
    int dt = a.t - b.t; if (dt < 0) dt = -dt;
    int dd = a.d - b.d; if (dd < 0) dd = -dd;
    int dm = a.m - b.m; if (dm < 0) dm = -dm;
    int dq = a.q - b.q; if (dq < 0) dq = -dq;
    return dt + dd + dm + dq;
}


/* ================================================================
 * 3. coord4_dim_distance -- distance along ONE dimension
 * ================================================================
 *
 * Returns |a[dim] - b[dim]| for a single axis.
 *
 *   dim 0 = T (temporal / when / duration)
 *   dim 1 = D (spatial  / where / place)
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
    case 1:  d = a.d - b.d; break;
    case 2:  d = a.m - b.m; break;
    case 3:  d = a.q - b.q; break;
    default: d = 0;         break;
    }
    return d < 0 ? -d : d;
}


/* ================================================================
 * 4. token_capacitance -- how much information this word carries
 * ================================================================
 *
 * Capacitance = magnitude of the coordinate vector = sqrt(t^2+d^2+m^2+q^2).
 *
 * The insight: a word's information content is encoded in where it
 * sits in the lattice.  Words far from the origin are specific,
 * high-content, semantically rich.  Words near the origin are
 * vague, functional, grammatical scaffolding.
 *
 *   High magnitude (far from origin):
 *     - Content nouns (M-heavy): "Einstein", "theory", "electron"
 *     - Proper nouns: specific referents, high information
 *     - Numbers: maximally specific
 *     -> High capacitance -> persists long in context
 *
 *   Low magnitude (near origin):
 *     - Function words: "the", "a", "of", "is"
 *     - These have small coordinates (close to zero in most dims)
 *     -> Low capacitance -> decays fast
 *
 * This is NOT a heuristic.  The coordinate magnitude IS the
 * information content because the lattice was designed so that
 * coordinates reflect semantic weight.  The physics determines it.
 *
 * Floor at 0.1 to prevent division by zero in RC calculations.
 * A token with C=0 would be pure silence -- and silence already
 * maps to (0,0,0,0) which gets special handling elsewhere.
 */

static inline float token_capacitance(coord4 c) {
    float mag = sqrtf((float)(c.t*c.t + c.d*c.d + c.m*c.m + c.q*c.q));
    return mag > 0.1f ? mag : 0.1f;
}


/* ================================================================
 * 5. connection_resistance -- how hard it is to reach a token
 * ================================================================
 *
 * R = walk_distance * (1 + clause_depth)
 *
 * walk_distance: number of tokens between source and target.
 *   1 = adjacent words (low resistance, strong connection).
 *   100 = far apart (high resistance, weak connection).
 *
 * clause_depth: number of subordination boundaries crossed.
 *   0 = same clause (no extra resistance).
 *   1 = one clause boundary (e.g., relative clause).
 *   2 = doubly embedded (e.g., "He said that she knew that...")
 *
 * The multiplication means clause boundaries act as resistors
 * in series: each boundary multiplies the effective distance.
 * A word 5 tokens away across 2 clause boundaries is as hard
 * to reach as a word 15 tokens away in the same clause.
 *
 * Floor at 1.0 to prevent degenerate zero-distance cases.
 */

static inline float connection_resistance(int walk_distance, int clause_depth) {
    float r = (float)walk_distance * (1.0f + (float)clause_depth);
    return r > 1.0f ? r : 1.0f;
}


/* ================================================================
 * 6. rc_relevance -- the attention weight for one token
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
 * Examples (at clause_depth=0, R=walk_distance):
 *
 *   "Einstein" at distance 50:
 *     C ~ 4.0 (high-magnitude noun coordinate)
 *     R = 50, RC = 200
 *     relevance = exp(-50/200) = exp(-0.25) = 0.78  (still very relevant)
 *
 *   "the" at distance 50:
 *     C ~ 1.0 (low-magnitude function word)
 *     R = 50, RC = 50
 *     relevance = exp(-50/50) = exp(-1) = 0.37  (fading)
 *
 *   "the" at distance 5:
 *     C ~ 1.0, R = 5, RC = 5
 *     relevance = exp(-5/5) = exp(-1) = 0.37  (same decay ratio)
 *
 *   "of" at distance 20:
 *     C ~ 0.5 (very low magnitude preposition)
 *     R = 20, RC = 10
 *     relevance = exp(-20/10) = exp(-2) = 0.14  (nearly irrelevant)
 *
 * The key: high-C words (nouns) stay relevant across hundreds of
 * tokens.  Low-C words (articles) become irrelevant in tens.
 * No fixed context window.  No arbitrary cutoff.  The decay IS
 * the attention mask, computed from the token's coordinates.
 */

static inline float rc_relevance(coord4 word, int walk_distance, int clause_depth) {
    float C = token_capacitance(word);
    float R = connection_resistance(walk_distance, clause_depth);
    float tau = R * C;  /* RC time constant */
    return expf(-(float)walk_distance / tau);
}


/* ================================================================
 * 7. rc_attention_weights -- full attention vector, no training
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
