#ifndef WAVE_H
#define WAVE_H

#include <stdint.h>

/*
 * wave.h — UTF-8 to wave byte converter, v2 encoding
 *
 * Replaces lattice.h.  Component 1.1 of the lattice-native intelligence
 * system.  Every ASCII letter maps to a single wave byte whose 4D
 * coordinate encodes the articulatory phonetics of the sound.
 *
 * ── The four dimensions ──────────────────────────────────────────
 *
 *   T = temporal dynamics (duration)
 *       +1 sustained   — vowels, fricatives, nasals (you can hold them)
 *        0 transitional — glides/approximants: w, j, l, r
 *       -1 transient    — stops, affricates (burst and gone)
 *
 *   D = place of articulation (where in the vocal tract)
 *       +1 front  — labial/dental: p, b, m, f, v, w
 *        0 central — alveolar/palatal: t, d, n, s, z, l, r
 *       -1 back   — velar/glottal: k, g, ng, h
 *
 *   M = manner / openness (how much substance flows through)
 *       +1 open   — vowels (maximum airflow, most energy)
 *        0 partial — fricatives (restricted airflow)
 *       -1 closed  — stops, nasals (blocked airflow)
 *
 *   Q = voicing (laryngeal vibration quality)
 *       +1 voiced    — b, d, g, v, z, m, n, l, r, all vowels
 *       -1 voiceless — p, t, k, f, s, h
 *
 * ── Wave byte encoding ───────────────────────────────────────────
 *
 *   bits [7:6] = T    [5:4] = D    [3:2] = M    [1:0] = Q
 *   00 = 0      01 = +1      11 = -1      10 = extended (shell 2+)
 *
 *   Shell 1: 1 byte.  3^4 = 81 possible coordinates.
 *   The 4D coordinate IS the embedding.  No learned embedding layer.
 *   Geometrically adjacent coordinates are phonetically adjacent.
 *
 * ── Lattice constraint ───────────────────────────────────────────
 *
 *   These dimensions are locked across all three layers:
 *     Physics:   T=time   D=distance   M=mass     Q=charge
 *     Word:      T=when   D=where      M=what     Q=how
 *     Character: T=dur    D=place      M=manner   Q=voicing
 *
 *   There is only ONE phonetic assignment per dimension that satisfies
 *   this.  The IPA organizes consonants by exactly these features.
 *
 * ── Silence ──────────────────────────────────────────────────────
 *
 *   (0,0,0,0) = 0x00 = identity = silence = space = newline.
 *   Digits and symbols map to 0x00 at shell 1 (TBD at shell 2).
 *   No case distinction: 'A' = 'a'.  Case is text, not sound.
 */


/* ================================================================
 * coord4 — the fundamental type
 * ================================================================ */

typedef struct {
    int8_t t, x, m, q;
} coord4;

/*
 * float4 — continuous coordinate for walk evaluation.
 *
 * coord4 is for storage (bins, field, reverse index).
 * float4 is for live computation (walks, attention, equation eval).
 *
 * "The path IS the function." — mind/mind
 * Polynomials are continuous, not discrete. The walk must carry
 * float precision so the equation can distinguish nearby words
 * and the trajectory drifts smoothly instead of snapping to grid.
 */
typedef struct {
    float t, x, m, q;
} float4;

static inline float4 coord4_to_float4(coord4 c) {
    return (float4){ (float)c.t, (float)c.x, (float)c.m, (float)c.q };
}

static inline coord4 float4_to_coord4(float4 f) {
    coord4 c;
    c.t = (int8_t)(f.t > 127 ? 127 : f.t < -128 ? -128 : (f.t >= 0 ? (int)(f.t + 0.5f) : (int)(f.t - 0.5f)));
    c.x = (int8_t)(f.x > 127 ? 127 : f.x < -128 ? -128 : (f.x >= 0 ? (int)(f.x + 0.5f) : (int)(f.x - 0.5f)));
    c.m = (int8_t)(f.m > 127 ? 127 : f.m < -128 ? -128 : (f.m >= 0 ? (int)(f.m + 0.5f) : (int)(f.m - 0.5f)));
    c.q = (int8_t)(f.q > 127 ? 127 : f.q < -128 ? -128 : (f.q >= 0 ? (int)(f.q + 0.5f) : (int)(f.q - 0.5f)));
    return c;
}


/* ================================================================
 * wave_decode — wave byte to 4D coordinate
 * ================================================================
 *
 * Each 2-bit field: 00=0, 01=+1, 10=extended(treated as 0), 11=-1
 */

static inline coord4 wave_decode(uint8_t wb) {
    static const int8_t tbl[4] = {0, +1, 0, -1}; /* 10=extended → 0 at shell 1 */
    coord4 c;
    c.t = tbl[(wb >> 6) & 3];
    c.x = tbl[(wb >> 4) & 3];
    c.m = tbl[(wb >> 2) & 3];
    c.q = tbl[(wb >> 0) & 3];
    return c;
}


/* ================================================================
 * wave_encode — 4D coordinate to wave byte
 * ================================================================
 *
 * Values must be -1, 0, or +1.  Anything else is undefined.
 *   +1 → 01,  0 → 00,  -1 → 11
 */

static inline uint8_t wave_encode(coord4 c) {
    static const uint8_t tbl[3] = {
        0x3,  /* -1 → 11 */
        0x0,  /*  0 → 00 */
        0x1   /* +1 → 01 */
    };
    return (tbl[c.t + 1] << 6) |
           (tbl[c.x + 1] << 4) |
           (tbl[c.m + 1] << 2) |
           (tbl[c.q + 1] << 0);
}


/* ================================================================
 * utf8_to_wave — ASCII letter to wave byte (v2 phonetic encoding)
 * ================================================================
 *
 * Every letter maps to its articulatory phonetic coordinate.
 * No case distinction at shell 1: 'A' = 'a'.
 *
 * Collisions are intentional and phonetically correct:
 *   l = r  (0x01)  — both alveolar approximant voiced; lateral vs
 *                     rhotic is a manner subtlety for shell 2
 *   a = i  (0x55)  — both front sustained open voiced; height is
 *                     an M-precision extension for shell 2
 *   o = u  (0x75)  — both back sustained open voiced; same reason
 *   k = c = q = x  (0xFF) — English has 4 letters for /k/
 *   g = j  (0xFD)  — /g/ and /dʒ/ are both back voiced stops at
 *                     shell 1 resolution; affricate detail is shell 2
 *   sh = h (0x73)  — both back voiceless fricatives; glottal depth
 *                     is a D-precision extension
 *   f = th (0x53)  — both front voiceless fricatives; dental vs
 *                     labiodental is D-precision
 *   v = dh (0x51)  — same, voiced
 *
 * Every remaining collision is between sounds that are genuinely
 * phonetically similar.  Shell 2 extends the relevant dimension.
 */

static inline uint8_t utf8_to_wave(uint8_t ch) {

    /*
     * v2 phonetic table for a-z.  Hex values from char-encoding-v2.md.
     *
     * Format per entry:  (T, D, M, Q) = hex
     *
     * Consonants derive from the IPA chart:
     *   place (D):   labial=+1   alveolar=0   velar/glottal=-1
     *   manner (M):  stop/nasal=-1   fricative=0   (vowel=+1)
     *   duration (T): stop=-1   nasal/fric=+1   glide=0
     *   voicing (Q): voiced=+1   voiceless=-1
     *
     * Vowels are all sustained (+1), open (+1), voiced (+1).
     * Only D discriminates at shell 1: front (+1) vs back (-1).
     */
    static const uint8_t wave[26] = {

        /* ── vowels ────────────────────────────────────────── */
        /*  a  (+1,+1,+1,+1) = 0x55   front open voiced
         *     /æ/ — tongue forward, jaw open, cords vibrate.
         *     The maximally "bright" vowel: all dimensions positive.
         *     Collides with 'i' — both front; height separates at shell 2. */
        0x55,

        /* ── labial consonants (D=+1, near the lips) ──────── */
        /*  b  (-1,+1,-1,+1) = 0xDD   voiced labial stop
         *     /b/ — lips seal shut, burst open. Transient (-1),
         *     front (+1), closed (-1), vocal cords active (+1).
         *     v1 had b=m collision.  v2 separates: b is T=-1 (stop),
         *     m is T=+1 (nasal can be held). */
        0xDD,

        /*  c  (-1,-1,-1,-1) = 0xFF   voiceless velar stop (= k)
         *     In English, 'c' is almost always /k/ (cat, cup, arc).
         *     Same phoneme, same coordinate.  Four letters, one sound. */
        0xFF,

        /*  d  (-1, 0,-1,+1) = 0xCD   voiced alveolar stop
         *     /d/ — tongue tip hits alveolar ridge, burst open.
         *     v1 had d=n collision.  v2 separates: d is T=-1,
         *     n is T=+1 (nasal sustains). */
        0xCD,

        /*  e  (+1, 0,+1,+1) = 0x45   central vowel voiced
         *     /ɛ/ — tongue at neutral position, mid-open.
         *     D=0 (central) distinguishes from a (front) and o (back). */
        0x45,

        /*  f  (+1,+1, 0,-1) = 0x53   voiceless labial fricative
         *     /f/ — lower lip against upper teeth, air turbulence.
         *     Sustained (+1) because you can hold the hiss.
         *     Collides with 'th' /θ/ — both front voiceless fricatives.
         *     Dental vs labiodental is a D-precision matter for shell 2. */
        0x53,

        /*  g  (-1,-1,-1,+1) = 0xFD   voiced velar stop
         *     /g/ — tongue back hits soft palate, burst.
         *     Collides with 'j' /dʒ/ — affricate = stop + fricative,
         *     a manner subtlety beyond shell 1 resolution. */
        0xFD,

        /*  h  (+1,-1, 0,-1) = 0x73   voiceless glottal fricative
         *     /h/ — the deepest sound: air from the glottis.
         *     Back (-1), partial airflow (0), no voicing (-1).
         *     Collides with 'sh' — both back voiceless fricatives.
         *     Shell 2 separates: h is glottal (D=-2), sh is postalveolar. */
        0x73,

        /*  i  (+1,+1,+1,+1) = 0x55   front vowel voiced (= a at shell 1)
         *     /ɪ/ — tongue high and forward.  At shell 1 resolution,
         *     both 'a' and 'i' are front-open-voiced.  Vowel height
         *     (the close/open distinction) needs M-precision at shell 2:
         *     i gets M=+2 (close), a stays M=+1 (open). */
        0x55,

        /*  j  (-1,-1,-1,+1) = 0xFD   voiced postalveolar affricate (= g)
         *     /dʒ/ — "judge".  Back voiced stop at shell 1.
         *     The affricate nature (stop + fricative) is a shell 2 feature. */
        0xFD,

        /* ── velar consonants (D=-1, deep in the throat) ──── */
        /*  k  (-1,-1,-1,-1) = 0xFF   voiceless velar stop
         *     /k/ — tongue back seals against soft palate, burst.
         *     The canonical back voiceless stop.  c, q, x are aliases. */
        0xFF,

        /* ── alveolar consonants (D=0, middle of mouth) ───── */
        /*  l  ( 0, 0, 0,+1) = 0x01   alveolar lateral approximant
         *     /l/ — tongue tip up, air flows around sides.
         *     T=0 (transitional) — not sustained like a fricative,
         *     not transient like a stop.  This was the key v2 insight:
         *     putting glides at T=0 resolved the l=z collision from v1.
         *     Collides with 'r' — both alveolar approximants.
         *     Lateral vs rhotic is a manner subtlety for shell 2. */
        0x01,

        /*  m  (+1,+1,-1,+1) = 0x5D   voiced labial nasal
         *     /m/ — lips sealed, air exits through nose.
         *     T=+1 (sustained) — you can hum /m/ indefinitely.
         *     This is what separates m from b: same place, same
         *     closure, same voicing, but m sustains, b bursts. */
        0x5D,

        /*  n  (+1, 0,-1,+1) = 0x4D   voiced alveolar nasal
         *     /n/ — tongue tip seals alveolar ridge, air through nose.
         *     Same T separation from d as m has from b. */
        0x4D,

        /*  o  (+1,-1,+1,+1) = 0x75   back vowel voiced
         *     /ɒ/ — tongue pulled back, lips rounded.
         *     D=-1 (back) distinguishes from a/i (front) and e (central).
         *     Collides with 'u' — both back open voiced.  Height is shell 2. */
        0x75,

        /*  p  (-1,+1,-1,-1) = 0xDF   voiceless labial stop
         *     /p/ — lips seal, burst, no voicing.
         *     The voiceless partner of b.  Flip Q bit to voice it. */
        0xDF,

        /*  q  (-1,-1,-1,-1) = 0xFF   voiceless velar stop (= k)
         *     English 'q' is always /k/ (queen = /kwiːn/).
         *     Redundant letter, same coordinate as k, c, x. */
        0xFF,

        /*  r  ( 0, 0, 0,+1) = 0x01   alveolar approximant (= l)
         *     /ɹ/ — tongue tip curled back, air flows freely.
         *     Same shell 1 coordinate as l.  Both are transitional,
         *     central, partially open, voiced.  Shell 2 distinguishes
         *     lateral (l) from rhotic (r) via manner extension. */
        0x01,

        /*  s  (+1, 0, 0,-1) = 0x43   voiceless alveolar fricative
         *     /s/ — tongue near alveolar ridge, turbulent airstream.
         *     Sustained (+1), central (0), partial (0), voiceless (-1).
         *     The voiceless partner of z.  Flip Q to get z. */
        0x43,

        /*  t  (-1, 0,-1,-1) = 0xCF   voiceless alveolar stop
         *     /t/ — tongue tip hits alveolar ridge, burst, no voicing.
         *     The voiceless partner of d.  The most common consonant
         *     in English sits at D=0 (central = default position). */
        0xCF,

        /*  u  (+1,-1,+1,+1) = 0x75   back vowel voiced (= o at shell 1)
         *     /ʊ/ — tongue high and back, lips rounded.
         *     Same D=-1 back position as o.  Height separates at shell 2. */
        0x75,

        /*  v  (+1,+1, 0,+1) = 0x51   voiced labial fricative
         *     /v/ — lower lip against upper teeth, turbulence + voicing.
         *     The voiced partner of f.  Collides with 'dh' /ð/ — both
         *     are front voiced fricatives. */
        0x51,

        /*  w  ( 0,+1,+1,+1) = 0x15   labial glide
         *     /w/ — lips round and release into the following vowel.
         *     T=0 (transitional) — this is what separates w from 'a'.
         *     v1 had a=w collision.  v2 resolves it: a is sustained
         *     (you can hold /aaaa/), w is transitional (it glides). */
        0x15,

        /*  x  (-1,-1,-1,-1) = 0xFF   voiceless velar stop (= k)
         *     English 'x' = /ks/.  At shell 1, maps to the /k/ component.
         *     The /s/ component would need a digraph encoding. */
        0xFF,

        /*  y  ( 0,-1,+1,+1) = 0x35   palatal glide
         *     /j/ — tongue body rises toward palate, glides into vowel.
         *     T=0 (transitional, like w), D=-1 (back/palatal), M=+1
         *     (open, vowel-like), Q=+1 (voiced).
         *     Not a vowel in onset position ("yes"), vowel-like in coda
         *     ("happy").  The T=0 marks it as a glide either way. */
        0x35,

        /*  z  (+1, 0, 0,+1) = 0x41   voiced alveolar fricative
         *     /z/ — like s but with vocal cord vibration.
         *     v1 had l=z collision.  v2 resolves it: z is T=+1
         *     (sustained: you can hold /zzzz/), l is T=0 (transitional). */
        0x41,
    };

    /* case-fold: 'A'-'Z' → 'a'-'z'.  No case in the wave byte. */
    if (ch >= 'a' && ch <= 'z') return wave[ch - 'a'];
    if (ch >= 'A' && ch <= 'Z') return wave[ch - 'A'];

    /* silence: space, newline, digits, symbols all map to identity */
    return 0x00;
}


/* ================================================================
 * wave_to_utf8 — wave byte to ASCII letter (best effort)
 * ================================================================
 *
 * Returns the "canonical" letter for a wave byte.  Where multiple
 * letters share a coordinate (collisions), we pick the most common:
 *   0x55 → 'a' (not 'i')    0x75 → 'o' (not 'u')
 *   0xFF → 'k' (not c/q/x)  0xFD → 'g' (not 'j')
 *   0x01 → 'l' (not 'r')
 *
 * Returns '?' for wave bytes that don't correspond to any letter.
 * Returns ' ' for 0x00 (silence).
 */

static inline char wave_to_utf8(uint8_t wb) {
    /*
     * Reverse lookup table.  We check against each known wave byte.
     * There are only 15 distinct wave bytes for 26 letters, so a
     * switch is clean and fast.
     *
     * Ordered by wave byte value for easy scanning:
     *   0x00 silence
     *   0x01 l/r        → l
     *   0x15 w
     *   0x35 y
     *   0x41 z
     *   0x43 s
     *   0x45 e
     *   0x4D n
     *   0x51 v
     *   0x53 f
     *   0x55 a/i        → a
     *   0x5D m
     *   0x73 h  (also sh)
     *   0x75 o/u        → o
     *   0xCD d
     *   0xCF t
     *   0xDD b
     *   0xDF p
     *   0xFD g  (also j)
     *   0xFF k  (also c,q,x)
     */
    switch (wb) {
    case 0x00: return ' ';

    /* glides (T=0) */
    case 0x01: return 'l';  /* l/r collision — l is more common */
    case 0x15: return 'w';
    case 0x35: return 'y';

    /* fricatives (T=+1, M=0) */
    case 0x41: return 'z';
    case 0x43: return 's';
    case 0x51: return 'v';
    case 0x53: return 'f';
    case 0x73: return 'h';

    /* vowels (T=+1, M=+1) */
    case 0x45: return 'e';
    case 0x55: return 'a';  /* a/i collision — 'a' is the canonical front vowel */
    case 0x75: return 'o';  /* o/u collision — 'o' is the canonical back vowel */

    /* nasals (T=+1, M=-1) */
    case 0x4D: return 'n';
    case 0x5D: return 'm';

    /* stops (T=-1, M=-1) */
    case 0xCD: return 'd';
    case 0xCF: return 't';
    case 0xDD: return 'b';
    case 0xDF: return 'p';
    case 0xFD: return 'g';  /* g/j collision — 'g' is the simple stop */
    case 0xFF: return 'k';  /* k/c/q/x — 'k' is the canonical spelling */

    default:   return '?';
    }
}


/* ================================================================
 * coord_to_float — 4D coordinate to float array for model input
 * ================================================================
 *
 * The coordinate IS the embedding.  No lookup table, no learned
 * parameters.  A vowel at (+1,+1,+1,+1) is geometrically adjacent
 * to a glide at (0,+1,+1,+1) — and phonetically they are.
 */

static inline void coord_to_float(coord4 c, float out[4]) {
    out[0] = (float)c.t;
    out[1] = (float)c.x;
    out[2] = (float)c.m;
    out[3] = (float)c.q;
}


/* ================================================================
 * Bond classification
 * ================================================================
 *
 * In the lattice, coordinates with odd values in any dimension are
 * "bonds" (forces between atoms).  Coordinates with all even values
 * are "atoms" (stable states).
 *
 * At shell 1, -1 and +1 are odd, 0 is even.  So:
 *   - Silence (0,0,0,0) is an atom (identity, no force).
 *   - Every letter is a bond (at least one dimension is +/-1).
 *   - The bond forces tell you WHICH dimensions are active.
 *
 * Bond force bits:
 *   P (bit 0) = T is odd → temporal force (read/write)
 *   C (bit 1) = D is odd → spatial force (call/return)
 *   R (bit 2) = M is odd → substance force (loop/rewind)
 *   W (bit 3) = Q is odd → quality force (test/guard)
 */

#define BOND_P     1   /* T odd: temporal — read/write */
#define BOND_C     2   /* D odd: spatial  — call/return */
#define BOND_R     4   /* M odd: substance — loop/rewind */
#define BOND_W     8   /* Q odd: quality  — test/guard */

/*
 * is_bond — does this coordinate carry any force?
 *
 * Returns nonzero if any dimension is odd (i.e., +/-1 at shell 1).
 * At shell 1 this means: every letter is a bond, only silence is an atom.
 */
static inline int is_bond(coord4 c) {
    return (c.t & 1) | (c.x & 1) | (c.m & 1) | (c.q & 1);
}

/*
 * bond_forces — which forces are active in this coordinate?
 *
 * Returns a bitmask of BOND_P/C/R/W.
 *
 * Examples:
 *   'a' = (+1,+1,+1,+1) → P|C|R|W = 0xF  (all forces active)
 *   'e' = (+1, 0,+1,+1) → P|R|W   = 0xD  (no spatial force)
 *   'l' = ( 0, 0, 0,+1) → W       = 0x8  (only quality force)
 *   ' ' = ( 0, 0, 0, 0) → 0       = 0x0  (no force, atom)
 */
static inline int bond_forces(coord4 c) {
    int f = 0;
    if (c.t & 1) f |= BOND_P;
    if (c.x & 1) f |= BOND_C;
    if (c.m & 1) f |= BOND_R;
    if (c.q & 1) f |= BOND_W;
    return f;
}


/* ================================================================
 * Phonetic classification helpers
 * ================================================================
 *
 * These derive directly from the dimensional semantics.
 * No lookup tables — the coordinate carries the information.
 */

/* Any letter (vowel, consonant, or glide).  Anything that's not silence.
 * At shell 1, every letter has at least one nonzero dimension.
 * Silence (0,0,0,0) is the only non-letter. */
static inline int is_letter(coord4 c) {
    return c.t != 0 || c.x != 0 || c.m != 0 || c.q != 0;
}

/* Vowel: sustained, open, voiced.  M=+1. */
static inline int is_vowel(coord4 c) {
    return c.m == 1;
}

/* Consonant: not a vowel, but still a letter. */
static inline int is_consonant(coord4 c) {
    return is_letter(c) && !is_vowel(c);
}

/* Stop: transient and closed.  T=-1, M=-1. */
static inline int is_stop(coord4 c) {
    return c.t == -1 && c.m == -1;
}

/* Nasal: sustained and closed.  T=+1, M=-1.
 * The T dimension is what distinguishes nasals from stops:
 * you can hold /mmm/ but not /p/. */
static inline int is_nasal(coord4 c) {
    return c.t == 1 && c.m == -1;
}

/* Fricative: sustained and partial.  T=+1, M=0. */
static inline int is_fricative(coord4 c) {
    return c.t == 1 && c.m == 0;
}

/* Glide/approximant: transitional.  T=0 (and is a letter). */
static inline int is_glide(coord4 c) {
    return c.t == 0 && is_letter(c);
}

/* Voiced: vocal cords vibrate.  Q=+1. */
static inline int is_voiced(coord4 c) {
    return c.q == 1;
}

/* Front sound: near the lips.  D=+1. */
static inline int is_front(coord4 c) {
    return c.x == 1;
}

/* Back sound: deep in throat.  D=-1. */
static inline int is_back(coord4 c) {
    return c.x == -1;
}


/* ================================================================
 * Coordinate transforms
 * ================================================================
 *
 * Because dimensions are phonetically meaningful, bit flips
 * correspond to phonetic transforms:
 *
 *   voice_flip:  XOR 0x02 — toggles Q between +1 and -1
 *                p ↔ b,  t ↔ d,  k ↔ g,  f ↔ v,  s ↔ z
 *
 *   stop_nasal:  XOR 0x80 — toggles T between +1 and -1
 *                b ↔ m,  d ↔ n,  g ↔ ng
 *
 * These are not arbitrary bit tricks.  They reflect real phonological
 * processes: voicing assimilation and nasal assimilation.
 */

#define VOICE_FLIP  0x02  /* XOR: toggle voicing (Q dimension) */
#define STOP_NASAL  0x80  /* XOR: toggle sustained/transient (T dimension) */


#endif /* WAVE_H */
