#ifndef LATTICE_H
#define LATTICE_H

#include <stdint.h>

/*
 * lattice.h — core types for the lattice model
 *
 * A wave byte encodes 4 coordinates in 1 byte:
 *   bits [7:6] = T    bits [5:4] = D    bits [3:2] = M    bits [1:0] = Q
 *   00 = 0    01 = +1    11 = -1    10 = extended (shell 2+)
 *
 * Shell 1: 3 values per dimension (-1, 0, +1) = 3^4 = 81 possible wave bytes.
 * These cover all fundamental bonds and basic characters.
 *
 * The 4D coordinate IS the embedding. No learned embedding layer.
 * Geometrically adjacent coordinates are semantically adjacent.
 */

/* 4D coordinate — the fundamental type */
typedef struct {
    int8_t t, d, m, q;
} coord4;

/* decode wave byte → 4D coordinate (shell 1 only, no extensions) */
static inline coord4 wave_decode(uint8_t wb) {
    coord4 c;
    /* each 2-bit field: 00=0, 01=+1, 11=-1, 10=extended */
    static const int8_t tbl[4] = {0, +1, 0, -1}; /* 10 treated as 0 here */
    c.t = tbl[(wb >> 6) & 3];
    c.d = tbl[(wb >> 4) & 3];
    c.m = tbl[(wb >> 2) & 3];
    c.q = tbl[(wb >> 0) & 3];
    return c;
}

/* encode 4D coordinate → wave byte (shell 1, values must be -1/0/+1) */
static inline uint8_t wave_encode(coord4 c) {
    /* +1 → 01, 0 → 00, -1 → 11 */
    static const uint8_t tbl[3] = {0x3, 0x0, 0x1}; /* [-1, 0, +1] */
    return (tbl[c.t + 1] << 6) |
           (tbl[c.d + 1] << 4) |
           (tbl[c.m + 1] << 2) |
           (tbl[c.q + 1] << 0);
}

/* coordinate as 4 floats for the model — the embedding */
static inline void coord_to_float(coord4 c, float out[4]) {
    out[0] = (float)c.t;
    out[1] = (float)c.d;
    out[2] = (float)c.m;
    out[3] = (float)c.q;
}

/*
 * Wave character encoding — phonetic coordinates
 *
 * T: case       (+1=lower, -1=upper)
 * D: place      (+1=front/labial, 0=central/alveolar, -1=back/velar)
 * M: openness   (+1=vowel, 0=fricative, -1=stop/nasal)
 * Q: voicing    (+1=voiced, -1=voiceless)
 *
 * 'a' = (+1,+1,+1,+1) = 0x55 = hylo
 * 'b' = (+1,+1,-1,+1) = 0x4D
 * silence = (0,0,0,0)  = 0x00 = wave/identity
 */

/* UTF-8 byte → wave byte (ASCII letters only, rest pass through as-is) */
static inline uint8_t utf8_to_wave(uint8_t ch) {
    /*
     * Phonetic coordinate table for a-z (lowercase)
     * Derived from articulatory phonetics — not designed, measured.
     * See wit/mind/coder/design WAVE CHARACTERS section.
     *
     * Collisions are intentional: k=c=q=x (same phonetic place),
     * b=m, d=n=j, l=r=z, a=w, u=y. Shell 2 disambiguates.
     */
    /*
     * Extracted from code/walks/input.asm scancode_table (the real implementation).
     * Indexed by PS/2 scancode there, re-indexed by ASCII letter here.
     * Collisions confirmed: k=q=x=c=0x7F, d=j=n=0x4D, l=r=z=0x41, a=w=0x55, etc.
     */
    static const uint8_t lower[26] = {
        /* a */ 0x55, /* (+1,+1,+1,+1) front open voiced = hylo */
        /* b */ 0x5D, /* (+1,+1,-1,+1) labial stop voiced */
        /* c */ 0x7F, /* (+1,-1,-1,-1) velar stop voiceless (=k=q=x) */
        /* d */ 0x4D, /* (+1,+1,-1,+1) alveolar stop voiced (=j=n) */
        /* e */ 0x45, /* (+1,0,+1,+1)  central vowel voiced */
        /* f */ 0x53, /* (+1,+1,0,-1)  labial fricative voiceless */
        /* g */ 0x7D, /* (+1,-1,-1,+1) velar stop voiced */
        /* h */ 0x73, /* (+1,-1,0,-1)  back fricative voiceless */
        /* i */ 0x57, /* (+1,+1,+1,-1) front vowel voiceless */
        /* j */ 0x4D, /* (+1,+1,-1,+1) (=d=n) */
        /* k */ 0x7F, /* (+1,-1,-1,-1) velar stop voiceless (=q=x=c) */
        /* l */ 0x41, /* (+1,0,0,+1)   alveolar fric voiced (=r=z) */
        /* m */ 0x5D, /* (+1,+1,-1,+1) labial closed voiced (=b) */
        /* n */ 0x4D, /* (+1,+1,-1,+1) alveolar closed voiced (=d=j) */
        /* o */ 0x77, /* (+1,-1,+1,-1) back vowel voiceless */
        /* p */ 0x5F, /* (+1,+1,-1,-1) labial stop voiceless */
        /* q */ 0x7F, /* (+1,-1,-1,-1) velar (=k=x=c) */
        /* r */ 0x41, /* (+1,0,0,+1)   alveolar fric voiced (=l=z) */
        /* s */ 0x43, /* (+1,0,0,-1)   alveolar fric voiceless */
        /* t */ 0x4F, /* (+1,+1,-1,-1) alveolar stop voiceless */
        /* u */ 0x75, /* (+1,-1,+1,+1) back vowel voiced (=y) */
        /* v */ 0x51, /* (+1,+1,0,+1)  labial fric voiced */
        /* w */ 0x55, /* (+1,+1,+1,+1) (=a) */
        /* x */ 0x7F, /* (+1,-1,-1,-1) (=k=c=q) */
        /* y */ 0x75, /* (+1,-1,+1,+1) palatal vowel voiced (=u) */
        /* z */ 0x41, /* (+1,0,0,+1)   alveolar fric voiced (=l=r) */
    };

    if (ch >= 'a' && ch <= 'z') return lower[ch - 'a'];
    if (ch >= 'A' && ch <= 'Z') return lower[ch - 'A'] ^ 0xC0; /* T flip */
    if (ch == ' ')  return 0x00; /* silence = wave = identity */
    if (ch == '\n') return 0x00; /* silence */
    if (ch == '0')  return 0x00; /* digits TBD — using silence for now */

    /* everything else: silence placeholder until proper encoding */
    return 0x00;
}

/*
 * The 16 bond patterns (odd coordinates = force active)
 *
 *  P=T odd, C=D odd, R=M odd, W=Q odd
 *  + = forward/consume, - = backward/produce
 */
#define BOND_P     1   /* read/write */
#define BOND_C     2   /* call/return */
#define BOND_R     4   /* loop/rewind */
#define BOND_W     8   /* test/guard */

/* classify wave byte as atom (all even) or bond (any odd) */
static inline int is_bond(coord4 c) {
    return (c.t & 1) | (c.d & 1) | (c.m & 1) | (c.q & 1);
}

/* bond force mask from coordinate */
static inline int bond_forces(coord4 c) {
    int f = 0;
    if (c.t & 1) f |= BOND_P;
    if (c.d & 1) f |= BOND_C;
    if (c.m & 1) f |= BOND_R;
    if (c.q & 1) f |= BOND_W;
    return f;
}

#endif /* LATTICE_H */
