/*
 * char_solve.c — compute optimal wave byte character encoding
 *
 * Applies Compute First to the character encoding problem.
 * Searches classification schemes × coordinate assignments
 * to find the encoding that minimizes frequency-weighted cost
 * while preserving O(1) classification.
 *
 * cc -O2 -o char_solve char_solve.c -lm && ./char_solve
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── character definitions ──────────────────────────────────── */

enum { LETTER = 1, DIGIT = 2, SYMBOL = 3 };

typedef struct {
    char    ch;
    int     cat;        /* LETTER, DIGIT, SYMBOL */
    int     upper;      /* 1 if uppercase */
    char    pair;       /* case partner, 0 if none */
    double  freq;       /* relative frequency */
} Char;

/* english letter frequencies (per 1000 chars) */
static const double LF[26] = {
    8.2, 1.5, 2.8, 4.3, 12.7, 2.2, 2.0, 6.1, 7.0, 0.15,
    0.77, 4.0, 2.4, 6.7, 7.5, 1.9, 0.095, 6.0, 6.3, 9.1,
    2.8, 0.98, 2.4, 0.15, 2.0, 0.074
};

static Char chars[128];
static int nchars;

static void init_chars(void) {
    int n = 0;
    /* space */
    chars[n++] = (Char){' ', SYMBOL, 0, 0, 18.0};
    /* lowercase */
    for (int i = 0; i < 26; i++)
        chars[n++] = (Char){'a'+i, LETTER, 0, 'A'+i, LF[i]*0.70};
    /* uppercase */
    for (int i = 0; i < 26; i++)
        chars[n++] = (Char){'A'+i, LETTER, 1, 'a'+i, LF[i]*0.30};
    /* digits */
    for (int i = 0; i < 10; i++)
        chars[n++] = (Char){'0'+i, DIGIT, 0, 0, 0.5};
    /* symbols */
    const char *sym = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    const double sf[] = {
        0.3,0.3,0.1,0.05,0.1,0.1,0.6,0.4,0.4,0.1,
        0.2,1.2,0.8,0.2,0.4,0.3,0.2,0.1,0.2,0.1,
        0.15,0.3,0.1,0.3,0.1,0.2,0.1,0.3,0.1,0.3,
        0.15,0.1,0.05
    };
    for (int i = 0; sym[i]; i++)
        chars[n++] = (Char){sym[i], SYMBOL, 0, 0, sf[i]};
    nchars = n;
}

/* find char index by character */
static int find_char(char ch) {
    for (int i = 0; i < nchars; i++)
        if (chars[i].ch == ch) return i;
    return -1;
}

/* ── coordinates ────────────────────────────────────────────── */

typedef int Coord[4];

static int enc_bytes(const Coord c) {
    int cost = 1;
    for (int i = 0; i < 4; i++)
        if (c[i] < -1 || c[i] > 1) cost++;
    return cost;
}

static int coord_eq(const Coord a, const Coord b) {
    return a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3];
}

/* wave byte from coordinate */
static int wave_byte(const Coord c) {
    int wb = 0;
    for (int i = 0; i < 4; i++) {
        int v = c[i], bits;
        if (v == 0) bits = 0;
        else if (v == 1) bits = 1;
        else if (v == -1) bits = 3;
        else bits = 2; /* extended */
        wb |= bits << (6 - i*2);
    }
    return wb;
}

/* ── scheme definition ──────────────────────────────────────── */

/*
 * Classification scheme:
 *   case_dim:    which dim flips for case (0-3)
 *   cat_dim:     which dim classifies letter/digit/symbol (0-3)
 *   cat_rule:    how cat_dim values map:
 *     SPLIT3:  positive=letter, zero=digit, negative=symbol
 *     SPLIT2:  nonzero=letter, zero=non-letter (digit/symbol share,
 *              distinguished by a 3rd dim: digit_dim where val=0 → digit)
 *     SPLITV:  +1=consonant, -1=vowel, 0=non-letter (bonus: is_vowel)
 */

enum { SPLIT3, SPLIT2, SPLITV };

typedef struct {
    int case_dim;
    int cat_dim;
    int rule;
    int digit_dim;   /* for SPLIT2: which dim distinguishes digit from symbol */
    /* results */
    Coord map[128];  /* coordinate per char */
    int valid;
    double cost;
    int s1, s2;
} Scheme;

/* ── assignment engine ──────────────────────────────────────── */

/* pool of available coordinates, sorted by cost */
typedef struct { Coord c; int cost; } CCoord;

static int cmp_cc(const void *a, const void *b) {
    return ((CCoord*)a)->cost - ((CCoord*)b)->cost;
}

/*
 * Try to assign all characters under a given scheme.
 * Returns total frequency-weighted encoding cost, or 1e18 on failure.
 */
static double assign_scheme(Scheme *s) {
    /* build pool */
    CCoord pool[625];
    int npool = 0;
    for (int t = -2; t <= 2; t++)
    for (int d = -2; d <= 2; d++)
    for (int m = -2; m <= 2; m++)
    for (int q = -2; q <= 2; q++) {
        pool[npool].c[0] = t;
        pool[npool].c[1] = d;
        pool[npool].c[2] = m;
        pool[npool].c[3] = q;
        pool[npool].cost = enc_bytes(pool[npool].c);
        npool++;
    }
    qsort(pool, npool, sizeof(CCoord), cmp_cc);

    int used[625] = {0};
    memset(s->map, 0, sizeof(s->map));

    /* assign space = (0,0,0,0) */
    int sp = find_char(' ');
    if (sp >= 0) {
        Coord zero = {0,0,0,0};
        memcpy(s->map[sp], zero, sizeof(Coord));
        for (int i = 0; i < npool; i++)
            if (coord_eq(pool[i].c, zero)) { used[i] = 1; break; }
    }

    /* helper: check if coord satisfies category constraint */
    #define CAT_OK(coord, cat) cat_ok(s, coord, cat)

    /* letters have cat_dim constrained; case_dim must be nonzero for pairing */
    /* digits have cat_dim constrained */
    /* symbols get everything else */

    /* collect letter pairs, sorted by combined frequency (descending) */
    typedef struct { int lo, hi; double freq; } Pair;
    Pair pairs[26];
    int npairs = 0;
    for (int i = 0; i < nchars; i++) {
        if (chars[i].cat == LETTER && !chars[i].upper && chars[i].pair) {
            int j = find_char(chars[i].pair);
            if (j >= 0) {
                pairs[npairs++] = (Pair){i, j, chars[i].freq + chars[j].freq};
            }
        }
    }
    /* sort pairs by frequency descending */
    for (int i = 0; i < npairs-1; i++)
        for (int j = i+1; j < npairs; j++)
            if (pairs[j].freq > pairs[i].freq) {
                Pair tmp = pairs[i]; pairs[i] = pairs[j]; pairs[j] = tmp;
            }

    /* collect digits, sorted by frequency descending */
    int digits[10]; int ndigits = 0;
    for (int i = 0; i < nchars; i++)
        if (chars[i].cat == DIGIT) digits[ndigits++] = i;

    /* collect symbols (non-space), sorted by frequency descending */
    int symbols[64]; int nsymbols = 0;
    for (int i = 0; i < nchars; i++)
        if (chars[i].cat == SYMBOL && chars[i].ch != ' ') symbols[nsymbols++] = i;
    for (int i = 0; i < nsymbols-1; i++)
        for (int j = i+1; j < nsymbols; j++)
            if (chars[symbols[j]].freq > chars[symbols[i]].freq) {
                int tmp = symbols[i]; symbols[i] = symbols[j]; symbols[j] = tmp;
            }

    double total = (sp >= 0) ? chars[sp].freq * 1.0 : 0; /* space = 1 byte */
    int s1 = 1, s2 = 0; /* space counted */

    /* ── assign letter pairs ── */
    for (int p = 0; p < npairs; p++) {
        int lo = pairs[p].lo, hi = pairs[p].hi;
        int found = 0;
        for (int i = 0; i < npool && !found; i++) {
            if (used[i]) continue;
            Coord *c = &pool[i].c;

            /* cat_dim constraint */
            int cv = (*c)[s->cat_dim];
            if (s->rule == SPLIT3) {
                if (cv <= 0) continue; /* letters need positive cat_dim */
            } else if (s->rule == SPLIT2) {
                if (cv == 0) continue; /* letters need nonzero cat_dim */
            } else { /* SPLITV */
                if (cv == 0) continue; /* letters need nonzero cat_dim */
            }

            /* case_dim must be nonzero (need +/- pair) */
            if ((*c)[s->case_dim] == 0) continue;

            /* build partner: negate case_dim */
            Coord partner;
            memcpy(partner, *c, sizeof(Coord));
            partner[s->case_dim] = -partner[s->case_dim];

            /* find partner in pool */
            int pi = -1;
            for (int j = 0; j < npool; j++) {
                if (!used[j] && coord_eq(pool[j].c, partner)) {
                    pi = j; break;
                }
            }
            if (pi < 0) continue;

            /* convention: lowercase gets positive case_dim */
            Coord lo_c, hi_c;
            if ((*c)[s->case_dim] > 0) {
                memcpy(lo_c, *c, sizeof(Coord));
                memcpy(hi_c, partner, sizeof(Coord));
            } else {
                memcpy(lo_c, partner, sizeof(Coord));
                memcpy(hi_c, *c, sizeof(Coord));
            }

            memcpy(s->map[lo], lo_c, sizeof(Coord));
            memcpy(s->map[hi], hi_c, sizeof(Coord));
            used[i] = 1;
            used[pi] = 1;

            int lo_b = enc_bytes(lo_c), hi_b = enc_bytes(hi_c);
            total += lo_b * chars[lo].freq + hi_b * chars[hi].freq;
            if (lo_b == 1) s1++; else s2++;
            if (hi_b == 1) s1++; else s2++;
            found = 1;
        }
        if (!found) { s->valid = 0; return 1e18; }
    }

    /* ── assign digits ── */
    for (int di = 0; di < ndigits; di++) {
        int ci = digits[di];
        int found = 0;
        for (int i = 0; i < npool && !found; i++) {
            if (used[i]) continue;
            Coord *c = &pool[i].c;

            if (s->rule == SPLIT3) {
                if ((*c)[s->cat_dim] != 0) continue;
            } else if (s->rule == SPLIT2) {
                if ((*c)[s->cat_dim] != 0) continue;
                if ((*c)[s->digit_dim] != 0) continue; /* digit_dim=0 for digits */
            } else { /* SPLITV */
                if ((*c)[s->cat_dim] != 0) continue;
            }

            memcpy(s->map[ci], *c, sizeof(Coord));
            used[i] = 1;
            int b = enc_bytes(*c);
            total += b * chars[ci].freq;
            if (b == 1) s1++; else s2++;
            found = 1;
        }
        if (!found) { s->valid = 0; return 1e18; }
    }

    /* ── assign symbols ── */
    for (int si = 0; si < nsymbols; si++) {
        int ci = symbols[si];
        int found = 0;
        for (int i = 0; i < npool && !found; i++) {
            if (used[i]) continue;
            Coord *c = &pool[i].c;

            if (s->rule == SPLIT3) {
                if ((*c)[s->cat_dim] >= 0) continue; /* symbols need negative */
            } else if (s->rule == SPLIT2) {
                if ((*c)[s->cat_dim] != 0) continue; /* non-letter */
                if ((*c)[s->digit_dim] == 0) continue; /* not digit → symbol */
            } else { /* SPLITV */
                if ((*c)[s->cat_dim] != 0) continue;
                /* share with digits — just use what's left */
            }

            memcpy(s->map[ci], *c, sizeof(Coord));
            used[i] = 1;
            int b = enc_bytes(*c);
            total += b * chars[ci].freq;
            if (b == 1) s1++; else s2++;
            found = 1;
        }
        if (!found) { s->valid = 0; return 1e18; }
    }

    s->valid = 1;
    s->cost = total;
    s->s1 = s1;
    s->s2 = s2;
    return total;
}

/* ── output ─────────────────────────────────────────────────── */

static const char *DN[] = {"T", "D", "M", "Q"};

static void print_scheme(const Scheme *s) {
    double tf = 0;
    for (int i = 0; i < nchars; i++) tf += chars[i].freq;

    printf("  case dim:   %s (flip = XOR 0x%02X)\n",
           DN[s->case_dim],
           (int[]){0x80,0x20,0x08,0x02}[s->case_dim]);
    printf("  cat dim:    %s\n", DN[s->cat_dim]);

    if (s->rule == SPLIT3) {
        printf("  rule:       3-way (%s>0=letter, %s=0=digit, %s<0=symbol)\n",
               DN[s->cat_dim], DN[s->cat_dim], DN[s->cat_dim]);
        printf("  is_letter:  %s > 0  (one gate)\n", DN[s->cat_dim]);
        printf("  is_digit:   %s == 0 (one gate)\n", DN[s->cat_dim]);
        printf("  is_symbol:  %s < 0  (one gate)\n", DN[s->cat_dim]);
    } else if (s->rule == SPLIT2) {
        printf("  rule:       2-way (%s!=0=letter, %s=0=non-letter)\n",
               DN[s->cat_dim], DN[s->cat_dim]);
        printf("  is_letter:  %s != 0 (one gate)\n", DN[s->cat_dim]);
        printf("  is_digit:   %s == 0 AND %s == 0 (two gates)\n",
               DN[s->cat_dim], DN[s->digit_dim]);
        printf("  is_symbol:  %s == 0 AND %s != 0 (two gates)\n",
               DN[s->cat_dim], DN[s->digit_dim]);
    } else {
        printf("  rule:       vowel (%s=+1=cons, %s=-1=vowel, %s=0=non-letter)\n",
               DN[s->cat_dim], DN[s->cat_dim], DN[s->cat_dim]);
        printf("  is_letter:  %s != 0   (one gate)\n", DN[s->cat_dim]);
        printf("  is_vowel:   %s == -1  (one gate)\n", DN[s->cat_dim]);
        printf("  is_cons:    %s == +1  (one gate)\n", DN[s->cat_dim]);
    }
    printf("  case_flip:  negate %s\n\n", DN[s->case_dim]);

    printf("  shell 1:    %d chars (1 byte)\n", s->s1);
    printf("  shell 2:    %d chars (2 bytes)\n", s->s2);
    printf("  cost:       %.1f (freq×bytes)\n", s->cost);
    printf("  avg b/char: %.3f\n\n", s->cost / tf);

    /* letter shell breakdown */
    int ls1=0, ls2=0, ds1=0, ds2=0, ss1=0, ss2=0;
    for (int i = 0; i < nchars; i++) {
        int b = enc_bytes(s->map[i]);
        if (chars[i].cat == LETTER) { if (b==1) ls1++; else ls2++; }
        else if (chars[i].cat == DIGIT) { if (b==1) ds1++; else ds2++; }
        else { if (b==1) ss1++; else ss2++; }
    }
    printf("  letters:    %d s1, %d s2\n", ls1, ls2);
    printf("  digits:     %d s1, %d s2\n", ds1, ds2);
    printf("  symbols:    %d s1, %d s2\n\n", ss1, ss2);

    /* full map */
    printf("  ch   T   D   M   Q   wave  bytes  freq\n");
    printf("  ──  ──  ──  ──  ──  ────  ─────  ────\n");
    for (int c = 0x20; c <= 0x7E; c++) {
        for (int i = 0; i < nchars; i++) {
            if (chars[i].ch == c) {
                const int *co = s->map[i];
                char d = chars[i].ch;
                if (d == ' ') printf("  SP");
                else printf("   %c", d);
                printf("  %+d  %+d  %+d  %+d  0x%02X  %d      %.1f\n",
                       co[0], co[1], co[2], co[3],
                       wave_byte(co), enc_bytes(co), chars[i].freq);
                break;
            }
        }
    }
}

/* ── main ───────────────────────────────────────────────────── */

int main(void) {
    init_chars();
    printf("chars: %d\n\n", nchars);

    Scheme best = {0};
    best.cost = 1e18;

    int tried = 0, valid = 0;

    for (int cd = 0; cd < 4; cd++) {       /* case dim */
        for (int xd = 0; xd < 4; xd++) {   /* cat dim */
            if (xd == cd) continue;

            /* SPLIT3: positive=letter, zero=digit, negative=symbol */
            {
                Scheme s = {.case_dim=cd, .cat_dim=xd, .rule=SPLIT3};
                assign_scheme(&s);
                tried++;
                if (s.valid) {
                    valid++;
                    if (s.cost < best.cost) best = s;
                }
            }

            /* SPLIT2: nonzero=letter, zero=non-letter, split by digit_dim */
            for (int dd = 0; dd < 4; dd++) {
                if (dd == cd || dd == xd) continue;
                Scheme s = {.case_dim=cd, .cat_dim=xd, .rule=SPLIT2, .digit_dim=dd};
                assign_scheme(&s);
                tried++;
                if (s.valid) {
                    valid++;
                    if (s.cost < best.cost) best = s;
                }
            }

            /* SPLITV: +1=consonant, -1=vowel, 0=non-letter */
            {
                Scheme s = {.case_dim=cd, .cat_dim=xd, .rule=SPLITV};
                assign_scheme(&s);
                tried++;
                if (s.valid) {
                    valid++;
                    if (s.cost < best.cost) best = s;
                }
            }
        }
    }

    printf("tried %d schemes, %d valid\n\n", tried, valid);

    printf("═══════════════════════════════════════\n");
    printf("  BEST SCHEME\n");
    printf("═══════════════════════════════════════\n\n");
    print_scheme(&best);

    /* also show best SPLIT3 (clean 3-way) for comparison */
    Scheme best3 = {0};
    best3.cost = 1e18;
    for (int cd = 0; cd < 4; cd++)
        for (int xd = 0; xd < 4; xd++) {
            if (xd == cd) continue;
            Scheme s = {.case_dim=cd, .cat_dim=xd, .rule=SPLIT3};
            assign_scheme(&s);
            if (s.valid && s.cost < best3.cost) best3 = s;
        }

    if (best3.valid && best3.cost != best.cost) {
        printf("\n═══════════════════════════════════════\n");
        printf("  BEST 3-WAY (clean classification)\n");
        printf("═══════════════════════════════════════\n\n");
        print_scheme(&best3);
    }

    return 0;
}
