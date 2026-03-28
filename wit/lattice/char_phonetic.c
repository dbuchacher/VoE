/*
 * char_phonetic.c — find where characters live in the lattice
 *
 * Instead of ASSIGNING coordinates to characters, DERIVE them
 * from measurable phonetic properties. If the lattice is everything,
 * then 'a' already has a coordinate — we just need to find it.
 *
 * Articulatory phonetics gives 3 features per sound:
 *   Place of articulation (WHERE in the vocal tract)
 *   Manner of articulation (HOW open/constricted)
 *   Voicing (vocal cord vibration)
 *
 * These map to D, M, Q. T is case (visual property, not phonetic).
 *
 * cc -O2 -o char_phonetic char_phonetic.c && ./char_phonetic
 */

#include <stdio.h>
#include <string.h>

/* ── phonetic features ──────────────────────────────────────── */

/*
 * Place (D): where the constriction occurs, front to back
 *   +1 = labial/front     (lips: b,p,f,v,m,w — or front vowel: i,e)
 *    0 = coronal/central   (tongue tip/blade: t,d,s,z,n,l,r — or central: a)
 *   -1 = dorsal/back       (tongue body: k,g,ŋ,j,h — or back vowel: o,u)
 *
 * Manner (M): degree of constriction, open to closed
 *   +1 = open/vowel        (no constriction: a,e,i,o,u)
 *    0 = partial/fricative  (turbulent airflow: f,v,s,z,h,θ,ð)
 *   -1 = closed/stop        (complete closure: b,p,d,t,g,k,m,n)
 *
 * Voice (Q): vocal cord state
 *   +1 = voiced/natural     (cords vibrate: b,d,g,v,z,m,n,l,r + all vowels)
 *   -1 = voiceless/modified (cords still: p,t,k,f,s,h)
 *    0 = neutral             (ambiguous or doesn't apply)
 *
 * Case (T): visual form (not phonetic — 'a' and 'A' sound identical)
 *   +1 = lowercase
 *   -1 = uppercase
 */

typedef struct {
    char    ch;
    char    ipa[8];     /* primary IPA sound */
    int     place;      /* D: +1 front, 0 central, -1 back */
    int     manner;     /* M: +1 vowel, 0 fricative, -1 stop/nasal */
    int     voice;      /* Q: +1 voiced, -1 voiceless, 0 neutral */
    char    pair;       /* voicing pair if any (t↔d, s↔z, etc.) */
} Phone;

static Phone letters[] = {
    /* ── vowels (M = +1, always voiced Q = +1) ── */
    {'a', "/æ/",   +1, +1, +1, 0},    /* front open unrounded */
    {'e', "/ɛ/",    0, +1, +1, 0},    /* central-mid */
    {'i', "/ɪ/",   +1, +1, -1, 0},    /* front close — Q=-1 for close */
    {'o', "/ɒ/",   -1, +1, -1, 0},    /* back rounded — Q=-1 for rounded */
    {'u', "/ʊ/",   -1, +1, +1, 0},    /* back open-ish */

    /* ── stops (M = -1, come in voiced/voiceless pairs) ── */
    {'b', "/b/",   +1, -1, +1, 'p'},  /* labial voiced */
    {'p', "/p/",   +1, -1, -1, 'b'},  /* labial voiceless */
    {'d', "/d/",    0, -1, +1, 't'},  /* alveolar voiced */
    {'t', "/t/",    0, -1, -1, 'd'},  /* alveolar voiceless */
    {'g', "/g/",   -1, -1, +1, 'k'},  /* velar voiced */
    {'k', "/k/",   -1, -1, -1, 'g'},  /* velar voiceless */

    /* ── fricatives (M = 0, come in voiced/voiceless pairs) ── */
    {'v', "/v/",   +1,  0, +1, 'f'},  /* labial voiced */
    {'f', "/f/",   +1,  0, -1, 'v'},  /* labial voiceless */
    {'z', "/z/",    0,  0, +1, 's'},  /* alveolar voiced */
    {'s', "/s/",    0,  0, -1, 'z'},  /* alveolar voiceless */
    {'h', "/h/",   -1,  0, -1,  0},   /* glottal voiceless (no pair) */

    /* ── nasals (M = -1 closed, but sonorant — voiced) ── */
    {'m', "/m/",   +1, -1, +1,  0},   /* labial nasal */
    {'n', "/n/",    0, -1, +1,  0},   /* alveolar nasal */

    /* ── liquids/approximants (between vowel and fricative) ── */
    {'l', "/l/",    0,  0, +1,  0},   /* alveolar lateral — M=0 */
    {'r', "/ɹ/",    0,  0, +1,  0},   /* alveolar approximant — M=0 */

    /* ── glides (vowel-like consonants) ── */
    {'w', "/w/",   +1, +1, +1,  0},   /* labial glide — nearly a vowel */
    {'y', "/j/",   -1, +1, +1,  0},   /* palatal glide — nearly a vowel */

    /* ── oddballs (English spelling ≠ phonetics) ── */
    {'c', "/k/",   -1, -1, -1,  0},   /* = k (or s before e,i) */
    {'j', "/dʒ/",   0, -1, +1,  0},   /* affricate, similar to d */
    {'q', "/k/",   -1, -1, -1,  0},   /* = k always */
    {'x', "/ks/",  -1, -1, -1,  0},   /* = k+s cluster */
};

#define NLETTERS (sizeof(letters)/sizeof(letters[0]))

/* ── coordinate computation ─────────────────────────────────── */

typedef struct { int t, d, m, q; } Coord;

static Coord phonetic_coord(const Phone *p, int lowercase) {
    return (Coord){
        .t = lowercase ? +1 : -1,  /* case */
        .d = p->place,              /* where */
        .m = p->manner,             /* how open */
        .q = p->voice,              /* voiced? */
    };
}

static int wave_byte(Coord c) {
    int enc(int v) {
        if (v == 0) return 0;
        if (v == 1) return 1;
        if (v == -1) return 3;
        return 2;
    }
    return (enc(c.t) << 6) | (enc(c.d) << 4) | (enc(c.m) << 2) | enc(c.q);
}

static const char *bond_name(Coord c) {
    /* look up what bond this coordinate corresponds to */
    int mask = 0;
    if (c.d & 1) mask |= 1;  /* wait, bonds have odd coords... */

    /* for shell-1 coords in {-1,0,+1}, odd means ±1 */
    int p = (c.t == +1 || c.t == -1) ? 1 : 0;
    int cc = (c.d == +1 || c.d == -1) ? 1 : 0;
    int r = (c.m == +1 || c.m == -1) ? 1 : 0;
    int w = (c.q == +1 || c.q == -1) ? 1 : 0;

    /* all nonzero at shell 1 → odd → bond */
    int bmask = p | (cc << 1) | (r << 2) | (w << 3);

    static const char *names[] = {
        "wave",       /* 0 */
        "read/write", /* 1  P */
        "apply",      /* 2  C */
        "fold",       /* 3  P+C */
        "drain",      /* 4  R */
        "slurp/fill", /* 5  P+R */
        "fix",        /* 6  C+R */
        "foldl",      /* 7  P+C+R */
        "test/guard",  /* 8  W */
        "filter/io",  /* 9  P+W */
        "maybe",      /* 10 C+W */
        "map",        /* 11 P+C+W */
        "take_while", /* 12 R+W */
        "scan",       /* 13 P+R+W */
        "bind",       /* 14 C+R+W */
        "hylo",       /* 15 P+C+R+W */
    };
    return names[bmask];
}

/* ── main ───────────────────────────────────────────────────── */

int main(void) {
    printf("CHARACTERS FROM PHONETICS\n");
    printf("═════════════════════════\n\n");
    printf("Dimensions derived from articulatory phonetics:\n");
    printf("  T = case        (+1=lower, -1=upper)\n");
    printf("  D = place       (+1=front/labial, 0=central/alveolar, -1=back/velar)\n");
    printf("  M = openness    (+1=vowel, 0=fricative, -1=stop/nasal)\n");
    printf("  Q = voicing     (+1=voiced, -1=voiceless)\n\n");

    printf("  ch  IPA     T   D   M   Q   wave   bond pattern\n");
    printf("  ──  ────   ──  ──  ──  ──   ────   ───────────\n");

    /* track collisions */
    typedef struct { char ch; Coord c; } Seen;
    Seen seen[64];
    int nseen = 0;

    for (int i = 0; i < (int)NLETTERS; i++) {
        Coord c = phonetic_coord(&letters[i], 1);
        int wb = wave_byte(c);

        /* check collision */
        char collision = 0;
        for (int j = 0; j < nseen; j++) {
            if (seen[j].c.t == c.t && seen[j].c.d == c.d &&
                seen[j].c.m == c.m && seen[j].c.q == c.q) {
                collision = seen[j].ch;
                break;
            }
        }

        printf("   %c   %-6s %+d  %+d  %+d  %+d   0x%02X   %-12s",
               letters[i].ch, letters[i].ipa,
               c.t, c.d, c.m, c.q, wb, bond_name(c));

        if (collision)
            printf("  ← COLLISION with '%c'", collision);
        if (letters[i].pair)
            printf("  [voice pair: %c↔%c = Q flip]", letters[i].ch, letters[i].pair);

        printf("\n");

        seen[nseen++] = (Seen){letters[i].ch, c};
    }

    /* ── voicing pairs ── */
    printf("\n\nVOICING PAIRS (differ only in Q, like case pairs differ only in T)\n");
    printf("══════════════════════════════════════════════════════════════════\n\n");

    for (int i = 0; i < (int)NLETTERS; i++) {
        if (!letters[i].pair) continue;
        if (letters[i].voice != +1) continue; /* only print voiced member */
        Coord v = phonetic_coord(&letters[i], 1);
        /* find voiceless partner */
        for (int j = 0; j < (int)NLETTERS; j++) {
            if (letters[j].ch == letters[i].pair) {
                Coord u = phonetic_coord(&letters[j], 1);
                printf("  %c (%+d,%+d,%+d,%+d) ↔ %c (%+d,%+d,%+d,%+d)   Q flip: %+d → %+d\n",
                       letters[i].ch, v.t, v.d, v.m, v.q,
                       letters[j].ch, u.t, u.d, u.m, u.q,
                       v.q, u.q);
                break;
            }
        }
    }

    /* ── analysis ── */
    printf("\n\nANALYSIS\n");
    printf("════════\n\n");

    int collisions = 0;
    for (int i = 0; i < (int)NLETTERS; i++) {
        Coord ci = phonetic_coord(&letters[i], 1);
        for (int j = i+1; j < (int)NLETTERS; j++) {
            Coord cj = phonetic_coord(&letters[j], 1);
            if (ci.t==cj.t && ci.d==cj.d && ci.m==cj.m && ci.q==cj.q)
                collisions++;
        }
    }

    int unique = 0;
    for (int i = 0; i < (int)NLETTERS; i++) {
        Coord ci = phonetic_coord(&letters[i], 1);
        int dup = 0;
        for (int j = 0; j < i; j++) {
            Coord cj = phonetic_coord(&letters[j], 1);
            if (ci.t==cj.t && ci.d==cj.d && ci.m==cj.m && ci.q==cj.q) {
                dup = 1; break;
            }
        }
        if (!dup) unique++;
    }

    printf("  total letters:      26\n");
    printf("  unique coordinates: %d\n", unique);
    printf("  collisions:         %d\n", collisions);
    printf("  need shell 2:       %d letters (to disambiguate collisions)\n",
           26 - unique + collisions);

    printf("\n  Collisions occur because 3 values per dimension can't\n");
    printf("  distinguish all sounds. These letters need shell 2\n");
    printf("  (one extended dimension) to separate:\n\n");

    for (int i = 0; i < (int)NLETTERS; i++) {
        Coord ci = phonetic_coord(&letters[i], 1);
        for (int j = i+1; j < (int)NLETTERS; j++) {
            Coord cj = phonetic_coord(&letters[j], 1);
            if (ci.t==cj.t && ci.d==cj.d && ci.m==cj.m && ci.q==cj.q) {
                printf("  '%c' = '%c' at (%+d,%+d,%+d,%+d)\n",
                       letters[i].ch, letters[j].ch,
                       ci.t, ci.d, ci.m, ci.q);
            }
        }
    }

    /* highlight 'a' */
    printf("\n\n'a' IN THE LATTICE\n");
    printf("══════════════════\n\n");
    Coord a = phonetic_coord(&letters[0], 1);
    printf("  'a' = (%+d, %+d, %+d, %+d) = 0x%02X\n",
           a.t, a.d, a.m, a.q, wave_byte(a));
    printf("  bond pattern: %s\n", bond_name(a));
    printf("\n  front(+D) + vowel(+M) + voiced(+Q) + lowercase(+T)\n");
    printf("  = the simplest full-positive excitation above silence\n");
    printf("  = all four forces active, all positive\n");
    printf("  = %s\n", bond_name(a));

    return 0;
}
