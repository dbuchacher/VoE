#ifndef COORD_INDEX_H
#define COORD_INDEX_H

#include <stdint.h>
#include "wave.h"

/*
 * coord_index.h — born-index over word COORDINATES for next-word prediction
 *
 * Component 1.4 of the lattice-native intelligence system.
 *
 * Key insight: instead of indexing exact byte sequences (index.c does that),
 * we index by COORDINATE REGION. Words with similar coordinates share index
 * entries. This is the density map that replaces trained weights.
 *
 * The index maps a context fingerprint (compressed summary of recent word
 * coordinates) to a probability distribution over 81 coordinate bins.
 *
 * Bin scheme (shell 1):
 *   Each dimension quantized to {-1, 0, +1} = 3 values.
 *   Total bins = 3^4 = 81.
 *   Bin index = (t+1)*27 + (d+1)*9 + (m+1)*3 + (q+1)
 *
 * Context fingerprint:
 *   Weighted centroid of the last N word coordinates.
 *   Recent words weighted more. Clamped to [-3, +3].
 *   This is lossy but preserves the region of semantic space.
 *
 * No training. No gradient descent. One pass over data builds the index.
 * The geometry does what backprop would have to learn.
 */

/* number of words in the context window */
#define CIDX_CTX_N  8

/* number of coordinate bins: 3^4 = 81 */
#define CIDX_BINS   81

/* default hash table size: 64K slots */
#define CIDX_HASH_BITS  16
#define CIDX_HASH_SIZE  (1 << CIDX_HASH_BITS)
#define CIDX_HASH_MASK  (CIDX_HASH_SIZE - 1)

/* max linear probes before giving up on a slot */
#define CIDX_MAX_PROBE  64

/*
 * One slot in the hash table.
 *
 * key:      context fingerprint packed into 32 bits
 * occupied: 1 if this slot holds data
 * counts:   frequency of each bin as next-word destination
 *
 * Total per slot: 4 + 1 + 3(pad) + 81*4 = 332 bytes
 * 64K slots = ~21 MB. Small enough for testing.
 */
typedef struct {
    uint32_t key;              /* packed fingerprint: t|d|m|q bytes */
    uint8_t  occupied;
    uint8_t  _pad[3];
    uint32_t counts[CIDX_BINS]; /* how often each bin followed this context */
} CISlot;

/*
 * The full coordinate index.
 */
typedef struct {
    CISlot  *table;
    int      hash_bits;        /* log2 of table size */
    uint32_t hash_size;
    uint32_t hash_mask;
    long     unique_contexts;  /* occupied slots */
    long     total_adds;       /* total add() calls */
    long     collisions;       /* probe mismatches (diagnostic) */
} CoordIndex;


/* ── API ─────────────────────────────────────────────────────── */

/*
 * Create a new empty index with default table size.
 * Returns NULL on allocation failure.
 */
CoordIndex *coord_index_create(void);

/*
 * Record an observation: after context_fingerprint, the word next_word appeared.
 * Increments the count for next_word's bin in the fingerprint's entry.
 */
void coord_index_add(CoordIndex *ci, coord4 context_fingerprint, coord4 next_word);

/*
 * Query the index: given a context fingerprint, return the probability
 * distribution over 81 bins.
 *
 * probs_out must point to float[CIDX_BINS].
 * Returns 1 if the fingerprint was found, 0 if not (probs_out filled with uniform).
 */
int coord_index_query(CoordIndex *ci, coord4 context_fingerprint, float *probs_out);

/*
 * Save index to disk. Binary format, portable within same endianness.
 * Returns 0 on success, -1 on error.
 */
int coord_index_save(CoordIndex *ci, const char *path);

/*
 * Load index from disk file.
 * Returns new CoordIndex, or NULL on error.
 */
CoordIndex *coord_index_load(const char *path);

/* Print index statistics to stderr. */
void coord_index_stats(CoordIndex *ci);

/* Free all memory. */
void coord_index_free(CoordIndex *ci);


/* ── Coordinate utilities ────────────────────────────────────── */

/*
 * Compute a context fingerprint from a window of N word coordinates.
 *
 * Method: weighted sum with exponential recency bias.
 * Word at position n-1 (most recent) gets weight N, position 0 gets weight 1.
 * Each dimension summed independently, then clamped to [-3, +3].
 *
 * This is intentionally lossy. It preserves the centroid of the context region.
 * Words that are geometrically near contribute similar fingerprints,
 * which is exactly the generalization we want: similar contexts share density.
 */
coord4 coord4_fingerprint(const coord4 *context, int n);

/*
 * Map a coordinate to its shell-1 bin index (0-80).
 * Clamps each dimension to {-1, 0, +1}, then encodes as base-3.
 * Same formula as model.h coord_to_index but accepts any coord range.
 */
int coord_to_bin(coord4 c);

/*
 * Recover the bin centroid coordinate from a bin index.
 * Inverse of coord_to_bin for shell-1 values.
 */
coord4 bin_to_coord(int bin);

/*
 * Pack coord4 into a 32-bit key for hashing.
 * Each dimension stored as one byte. Works for values in [-128, +127].
 */
uint32_t coord4_pack(coord4 c);

/*
 * Unpack 32-bit key back to coord4.
 */
coord4 coord4_unpack(uint32_t key);

#endif /* COORD_INDEX_H */
