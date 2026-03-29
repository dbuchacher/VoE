#ifndef LEARNER_H
#define LEARNER_H

#include <stdint.h>
#include "wave.h"
#include "bonds.h"

/*
 * learner.h — component 3.1: memetic context learner
 *
 * Learns word coordinates from context, not from a dictionary.
 * No training. No gradients. Born-indexed.
 *
 * ── Three layers ────────────────────────────────────────────────
 *
 *   Layer 0: 16 bond patterns (2^4 = 16, hardcoded, innate schemas)
 *            These are the image schemas from embodied cognition.
 *            They constrain what coordinates are valid at each
 *            walk position. The seed. Never changes.
 *
 *   Layer 1: ~65 semantic primes (NSM, Wierzbicka)
 *            First words learned, anchored to bond patterns.
 *            Fixed coordinates, confidence = UINT32_MAX.
 *            "I", "you", "good", "bad", "do", "move", etc.
 *
 *   Layer 2: All other words (everything encountered in text)
 *            Learned memetically from context. Born-indexed on
 *            first encounter. Updated as running weighted average
 *            on each subsequent encounter. Lifelong learning.
 *
 * ── How memetic learning works ──────────────────────────────────
 *
 *   1. Process text word by word.
 *   2. Look up each word in vocabulary.
 *   3. If known: return stored coordinate, confidence++.
 *   4. If unknown:
 *      a. Examine walk position (what comes before/after).
 *      b. Bond between previous word and this position tells us
 *         which dimensions CHANGED, constraining the region.
 *      c. Surrounding known words provide a centroid estimate.
 *      d. BORN-INDEX: store the coordinate immediately (confidence=1).
 *      e. Each subsequent encounter: running weighted average.
 *
 *   The bond pattern acts as a PRIOR. It says: "after a noun,
 *   the next word is likely T-heavy (verb) or Q-heavy (adjective)."
 *   The context centroid provides the ESTIMATE. Together they give
 *   a reasonable coordinate on the FIRST encounter. No training
 *   data needed.
 *
 * ── Position classification ─────────────────────────────────────
 *
 *   Parts of speech = dimensional signatures (from primes.md):
 *     Noun       = M-heavy (substance)
 *     Verb       = T-heavy (temporal change)
 *     Adjective  = Q-heavy (quality/evaluation)
 *     Adverb     = Q+T (quality of change)
 *     Preposition= D-heavy (spatial)
 *     Determiner = D+Q (spatial + signal)
 *     Pronoun    = M+Q (substance + signal)
 *     Conjunction= Q (logic gate)
 *
 *   Walk position constrains which signature is likely:
 *     After M-heavy (noun) → expect T-heavy (verb) or Q-heavy (adj)
 *     After T-heavy (verb) → expect M-heavy (noun/object)
 *     After Q-heavy (adj)  → expect M-heavy (noun it modifies)
 *     After D-heavy (prep) → expect M-heavy (noun complement)
 *     After D+Q (article)  → expect Q-heavy (adj) or M-heavy (noun)
 */


/* ================================================================
 * Constants
 * ================================================================ */

#define LEARNER_VOCAB_SLOTS  (1 << 16)   /* 64K slots, open addressing  */
#define LEARNER_WORD_MAX     32          /* max chars per word entry     */
#define LEARNER_BOND_COUNT   16          /* 2^4 bond patterns (Layer 0) */


/* ================================================================
 * WordEntry — one word in the learned vocabulary
 * ================================================================
 *
 * Each entry stores:
 *   - The text of the word (lowercased, null-terminated, max 31 chars)
 *   - Its learned coordinate in 4D space
 *   - How many times it's been seen (confidence)
 *   - The coordinate at first encounter (born_context, never changes)
 *
 * Primes have confidence = UINT32_MAX, meaning they never update
 * from context. They are the fixed anchors of the coordinate system.
 */

typedef struct {
    char     word[LEARNER_WORD_MAX]; /* lowercased text, null-terminated   */
    coord4   coord;                  /* current learned coordinate         */
    coord4   born_context;           /* coordinate at first encounter      */
    uint32_t confidence;             /* times seen (UINT32_MAX = prime)    */
} WordEntry;


/* ================================================================
 * BondSchema — one of the 16 innate bond patterns (Layer 0)
 * ================================================================
 *
 * Each bond pattern maps to a default coordinate REGION — the
 * dimensional signature that a word at this bond position is
 * likely to have.
 *
 * These are the image schemas from cognitive science:
 *   bond 0  = IDENTITY       (no force)
 *   bond 1  = FORCE          (P: temporal push)
 *   bond 2  = PATH           (C: spatial traverse)
 *   ...
 *   bond 15 = CONTAINER      (P+C+R+W: all forces)
 *
 * The default_region is not a specific coordinate — it's a
 * dimensional bias. d_t=+1 means "this position favors T-heavy
 * words" (i.e., verbs). The learner uses this bias to constrain
 * the initial coordinate estimate for unknown words.
 */

typedef struct {
    BondType type;                   /* the 4-bit bond pattern            */
    coord4   default_region;         /* dimensional bias for this position*/
    const char *schema_name;         /* human-readable schema name        */
} BondSchema;


/* ================================================================
 * Learner — the main structure
 * ================================================================ */

typedef struct {
    /* vocabulary: open-addressing hash table */
    WordEntry *vocab;                /* LEARNER_VOCAB_SLOTS entries       */
    uint32_t   vocab_count;          /* how many slots are occupied       */

    /* Layer 0: the 16 innate bond schemas */
    BondSchema bonds[LEARNER_BOND_COUNT];

    /* statistics */
    uint32_t   total_words_seen;     /* total word encounters             */
    uint32_t   total_born;           /* words born-indexed (first time)   */
    uint32_t   total_updated;        /* coordinates updated (subsequent)  */
    uint32_t   total_prime_hits;     /* lookups that hit a prime          */
} Learner;


/* ================================================================
 * API
 * ================================================================ */

/*
 * learner_create — allocate and initialize a learner.
 *
 * Allocates the 64K-slot vocabulary hash table and seeds the
 * bond schemas (Layer 0). Does NOT seed primes — call
 * learner_seed_primes() separately if you want Layer 1.
 *
 * Returns NULL on allocation failure.
 */
Learner *learner_create(void);

/*
 * learner_free — release all memory.
 */
void learner_free(Learner *lr);

/*
 * learner_seed_bonds — populate the 16 bond schemas (Layer 0).
 *
 * This is called automatically by learner_create(). The bond
 * patterns are hardcoded — they're the innate cognitive schemas
 * that constrain learning. They never change.
 */
void learner_seed_bonds(Learner *lr);

/*
 * learner_seed_primes — load the ~65 NSM primes (Layer 1).
 *
 * These are the "first words" — anchored to bond patterns with
 * fixed coordinates. confidence = UINT32_MAX ensures they never
 * update from context. The coordinate system needs these anchors
 * to bootstrap learning of all other words.
 *
 * Includes: I, you, someone, something, do, happen, move, good,
 * bad, big, small, know, think, want, feel, see, hear, say,
 * not, if, because, can, the, a, this, same, other, one, all,
 * some, before, after, now, where, here, above, below, etc.
 */
void learner_seed_primes(Learner *lr);

/*
 * learner_process_word — the core function: learn from context.
 *
 * Given a word (from the tokenizer) and a window of surrounding
 * context coordinates, returns the word's coordinate.
 *
 * If the word is known: returns stored coordinate, confidence++.
 * If the word is unknown: born-indexes it with an estimate
 *   derived from context and bond pattern, returns that estimate.
 *
 * Parameters:
 *   lr             — the learner
 *   word           — the word text (lowercased)
 *   len            — byte length of word (not null-terminated)
 *   context_window — array of coordinates for surrounding words
 *   ctx_len        — number of entries in context_window
 *                     (typically 2-8: the nearby known words)
 *
 * The context_window should contain coordinates of the words
 * that PRECEDE this word in the walk. The last entry is the
 * immediately preceding word. More entries = deeper context.
 *
 * Returns: the word's coordinate (known, or freshly estimated).
 */
coord4 learner_process_word(Learner *lr,
                            const char *word, int len,
                            const coord4 *context_window, int ctx_len);

/*
 * learner_lookup — find a word in the vocabulary.
 *
 * Returns pointer to the WordEntry if found, NULL if not.
 * The pointer is into the hash table — do not free it.
 */
WordEntry *learner_lookup(Learner *lr, const char *word, int len);

/*
 * learner_stats — print vocabulary statistics to stdout.
 *
 * Shows: total words, prime count, confidence distribution,
 * dimensional distribution of learned coordinates.
 */
void learner_stats(const Learner *lr);

/*
 * learner_save — write vocabulary to a binary file.
 *
 * Format: [magic][version][count][entries...]
 * Each entry: word (32 bytes) + coord + born_context + confidence.
 * Only occupied slots are written.
 */
int learner_save(const Learner *lr, const char *path);

/*
 * learner_load — read vocabulary from a binary file.
 *
 * Returns a fully initialized Learner with bonds seeded and
 * vocabulary loaded. Returns NULL on failure.
 */
Learner *learner_load(const char *path);


#endif /* LEARNER_H */
