/*
 * atlas.c — lattice analysis of ancient sites worldwide
 *
 * Same method as giza.c: feed measurements through the equation.
 * See what falls out. No agenda — just math.
 *
 * Build: cc -O2 -o atlas atlas.c -lm
 * Run:   ./atlas
 */

#include <stdio.h>
#include <math.h>

#define PI      3.14159265358979
#define PHI     1.61803398874989
#define SQRT5   2.23606797749979
#define CUBIT   (PI/6.0)            /* royal cubit in meters: 0.52360 m */

/* lattice constants */
#define GATES   3
#define DIMS    4
#define BONDS   16                  /* 2^DIMS */
#define FILL    25                  /* 25% optimal fill */

long shell(int k2) {
    long a = 2*k2 + 1;
    long b = (k2 > 0) ? 2*k2 - 1 : 0;
    return a*a*a*a - b*b*b*b;
}

long triangular(int n) { return (long)n * (n+1) / 2; }

int gcd(int a, int b) { while (b) { int t = b; b = a % b; a = t; } return a; }

/* ═══════════════════════════════════════════════════════
 * 1. PYTHAGOREAN IDENTITY — pure math, no site
 *
 * 3² + 4² = 5²  →  gates² + dims² = fill%
 *                →  gates² + bonds = fill%
 * ═══════════════════════════════════════════════════════ */

void test_pythagorean(void) {
    printf("1. THE PYTHAGOREAN IDENTITY\n");
    printf("   ========================\n\n");
    printf("   3^2 + 4^2 = 5^2\n");
    printf("   %d + %d = %d\n\n", GATES*GATES, DIMS*DIMS, FILL);
    printf("   gates^2 + bonds = fill%%\n");
    printf("   %d + %d = %d\n\n", GATES*GATES, BONDS, FILL);
    printf("   Why dims^2 = bonds: n^2 = 2^n has integer solutions n=2, n=4.\n");
    printf("   At n=4: 4^2 = 16 = 2^4. Specific to d=4.\n\n");
    printf("   The first Pythagorean triple IS the lattice identity.\n");
    printf("   3-4-5 is the ONLY triple from consecutive integers.\n");
}

/* ═══════════════════════════════════════════════════════
 * 2. KHAFRE PYRAMID — 3-4-5 in stone
 *
 * Petrie (1883): base = 215.25 m, height = 143.5 m
 * Slope: 53°10' ≈ atan(4/3) = 53.130°
 * In cubits: base ≈ 411 = 3×137, height ≈ 274 = 2×137
 * ═══════════════════════════════════════════════════════ */

void test_khafre(void) {
    printf("\n\n2. KHAFRE PYRAMID — 3-4-5 in stone\n");
    printf("   ==================================\n\n");

    double base_m = 215.25;
    double height_m = 143.5;
    double base_c = base_m / CUBIT;
    double height_c = height_m / CUBIT;
    double half_c = base_c / 2.0;

    printf("   Petrie (1883):\n");
    printf("   base   = %.2f m = %.1f cubits\n", base_m, base_c);
    printf("   height = %.1f m = %.1f cubits\n\n", height_m, height_c);

    double tan_s = height_c / half_c;
    printf("   tan(slope) = %.6f\n", tan_s);
    printf("   4/3         = %.6f\n", 4.0/3.0);
    printf("   error: %.4f%%\n\n", fabs(tan_s - 4.0/3.0) / (4.0/3.0) * 100);

    double angle_345 = atan(4.0/3.0) * 180.0 / PI;
    printf("   atan(4/3) = %.3f deg\n", angle_345);
    printf("   Petrie    = 53.167 deg (53 deg 10')\n\n");

    double slant = sqrt(height_c*height_c + half_c*half_c);
    printf("   half_base : height : slant = %.1f : %.1f : %.1f\n",
           half_c, height_c, slant);
    printf("   ratio: 3 : 4 : 5\n\n");

    int b = (int)round(base_c);
    int h = (int)round(height_c);
    int g = gcd(b, h);
    printf("   Integer cubits: base = %d, height = %d\n", b, h);
    printf("   GCD = %d\n", g);
    printf("   %d = %d x %d\n", b, b/g, g);
    printf("   %d = %d x %d\n\n", h, h/g, g);

    printf("   137 = 1/alpha (fine structure constant inverse)\n");
    printf("   lattice: 544/4 + 1 + 3/84 = 137.036\n\n");
    printf("   base   = gates x alpha^-1 = %d x %d = %d cubits\n", b/g, g, b);
    printf("   height = 2 x alpha^-1     = %d x %d = %d cubits\n\n", h/g, g, h);

    printf("   Proof: %d/(%d/2) = %d/%d = %d/%d = 4/3 exactly\n",
           h, b, 2*h, b, 2*h/g, b/g);
}

/* ═══════════════════════════════════════════════════════
 * 3. SEKED SEQUENCE — three pyramids, one pattern
 *
 * seked = horizontal per cubit rise, in palms (7 per cubit)
 * tan(slope) = 7 / seked
 *
 * Three pyramids: seked = 20/4, 21/4, 22/4
 * ═══════════════════════════════════════════════════════ */

void test_seked(void) {
    printf("\n\n3. THE SEKED SEQUENCE\n");
    printf("   ===================\n\n");
    printf("   seked = horizontal per cubit rise, in palms (7 per cubit)\n");
    printf("   tan(slope) = 7 / seked\n\n");

    const char *names[] = {"Bent Pyramid (lower)", "Khafre", "Khufu (Great)"};
    double measured[] = {54.46, 53.17, 51.844};
    int sn[] = {20, 21, 22};

    for (int i = 0; i < 3; i++) {
        double seked = sn[i] / 4.0;
        double tan_v = 7.0 / seked;
        double angle = atan(tan_v) * 180.0 / PI;

        printf("   %-22s  seked = %d/4 = %.2f\n", names[i], sn[i], seked);
        printf("     tan = 7/%.2f = %.6f", seked, tan_v);
        if (i == 0) printf(" = 7/5");
        if (i == 1) printf(" = 4/3");
        if (i == 2) printf(" = 14/11");
        printf("\n     predicted: %.3f deg  measured: ~%.3f deg  diff: %.3f deg\n\n",
               angle, measured[i], fabs(angle - measured[i]));
    }

    printf("   All tangents = 28/n for n = 20, 21, 22:\n");
    for (int n = 20; n <= 22; n++)
        printf("     28/%d = %.6f\n", n, 28.0/n);
    printf("   28 = T(7) = perfect number\n\n");

    printf("   14/11 vs 4/pi: %.6f vs %.6f  error: %.4f%%\n",
           14.0/11.0, 4.0/PI, fabs(14.0/11.0 - 4.0/PI)/(4.0/PI)*100);
    printf("\n   PROGRESSION:\n");
    printf("     seked 5.00 -> 7/5 = 1.400     integer ratio    ATOMS\n");
    printf("     seked 5.25 -> 4/3 = 1.333     Pythagorean      ATOMS\n");
    printf("     seked 5.50 -> 14/11 ~ 4/pi    transcendental   BONDS\n");
}

/* ═══════════════════════════════════════════════════════
 * 4. GIZA PLATEAU — three pyramids as a system
 * ═══════════════════════════════════════════════════════ */

void test_plateau(void) {
    printf("\n\n4. GIZA PLATEAU — three types of math\n");
    printf("   =====================================\n\n");

    double kh = 280, kfh = 274, mh = 125;
    double kb = 440, kfb = 411, mb = 207;

    printf("   Heights (cubits): Khufu=%.0f  Khafre=%.0f  Menkaure=%.0f\n",
           kh, kfh, mh);
    printf("   Bases (cubits):   Khufu=%.0f  Khafre=%.0f  Menkaure=%.0f\n\n",
           kb, kfb, mb);

    double r = kh / mh;
    printf("   Khufu/Menkaure height: %.0f/%.0f = %.4f\n", kh, mh, r);
    printf("   sqrt(5) = %.4f   error: %.3f%%\n\n", SQRT5, fabs(r-SQRT5)/SQRT5*100);

    printf("   Khufu - Khafre height: %.0f - %.0f = %.0f cubits\n", kh, kfh, kh-kfh);
    printf("   C(4,2) = 6 (4D face count, cubit denominator)\n\n");

    printf("   Menkaure height = %.0f = 5^3\n", mh);
    printf("   5 = hypotenuse of 3-4-5, sqrt(fill%%)\n\n");

    double diff_m = 146.7 - 143.5;
    printf("   Height diff in meters: 146.7 - 143.5 = %.1f m = %.1f cubits\n",
           diff_m, diff_m / CUBIT);
    printf("   (expect 6.0, measured %.1f)\n\n", diff_m / CUBIT);

    printf("   Khufu:    4/pi, 1/phi        TRANSCENDENTAL  (bonds)\n");
    printf("   Khafre:   4/3, 3-4-5, 137    RATIONAL        (atoms)\n");
    printf("   Menkaure: 5^3                INTEGER         (vacuum?)\n");
}

/* ═══════════════════════════════════════════════════════
 * 5. TEOTIHUACAN — Pyramid of the Sun
 * ═══════════════════════════════════════════════════════ */

void test_teotihuacan(void) {
    printf("\n\n5. TEOTIHUACAN — Pyramid of the Sun\n");
    printf("   ==================================\n\n");

    double base = 223.5;
    printf("   Base: %.1f m (Millon 1973)\n\n", base);

    double heights[] = {63.0, 65.5, 71.1, 75.0};
    const char *labels[] = {"current", "common est", "if = pi", "with temple"};
    for (int i = 0; i < 4; i++) {
        printf("   height %.1f m %-12s  base/height = %.4f",
               heights[i], labels[i], base/heights[i]);
        if (i == 2) printf("  = pi");
        printf("\n");
    }
    printf("\n   pi requires height = %.2f m. Within estimate range.\n", base/PI);
    printf("   Confidence: MODERATE. Height uncertain.\n");

    printf("\n   Harleston STU = 1.0594 m vs 2^(1/12) = %.4f (semitone)\n",
           pow(2, 1.0/12));
    printf("   error: %.4f%%. Controversial, not mainstream.\n",
           fabs(1.0594 - pow(2, 1.0/12))/pow(2,1.0/12)*100);
}

/* ═══════════════════════════════════════════════════════
 * 6. CHICHEN ITZA — El Castillo
 * ═══════════════════════════════════════════════════════ */

void test_chichen(void) {
    printf("\n\n6. CHICHEN ITZA — El Castillo\n");
    printf("   ============================\n\n");

    printf("   91 steps x 4 faces + 1 platform = %d (solar year)\n\n", 91*4+1);

    printf("   9 terraces = %d^2 = gates squared\n", GATES);
    printf("   same 9: Giza QC niche = 9, KC beams = 9\n");
    printf("   TWO CONTINENTS. SAME NUMBER.\n\n");

    printf("   Each face = 91 / 364 = 1/4 = 25%% = fill ratio\n");
    printf("   same 1/4: Giza KC_L/KC_floor = 20/80 = 1/4\n\n");

    printf("   91 = T(13) = %ld   (13 = fibonacci(7))\n", triangular(13));
    printf("   91 = 7 x 13 = palms x fib(palms)\n");
}

/* ═══════════════════════════════════════════════════════
 * 7. STONEHENGE
 * ═══════════════════════════════════════════════════════ */

void test_stonehenge(void) {
    printf("\n\n7. STONEHENGE\n");
    printf("   ===========\n\n");

    printf("   56 Aubrey holes. 30 sarsens. 5 trilithon pairs.\n\n");
    printf("   30 / 5 = 6 = C(4,2) = 4D face count\n");
    printf("   same 6: cubit = pi/6 = pi/C(4,2)\n\n");

    printf("   56 = C(8,3) = %ld\n", (long)(8*7*6)/(3*2*1));
    printf("   56 = 2 x T(7) = 2 x %ld  (T(7) = 28 = perfect number)\n",
           triangular(7));
    printf("   28 = numerator in all seked tangents (28/n)\n\n");

    printf("   Astronomical: 3 x 18.6-year lunar node cycle = 55.8\n");
    printf("   Eclipse prediction is the primary purpose.\n");
}

/* ═══════════════════════════════════════════════════════
 * 8. OTHER SITES
 * ═══════════════════════════════════════════════════════ */

void test_other(void) {
    printf("\n\n8. OTHER SITES\n");
    printf("   ============\n\n");

    printf("   BENT PYRAMID (Dahshur): lower slope = atan(7/5) = %.3f deg\n",
           atan(7.0/5.0)*180/PI);
    printf("     seked = 5.00 (integer). Step 0 of the seked sequence.\n\n");

    printf("   ANGKOR WAT: main axis possibly 1296 Khmer cubits\n");
    printf("     1296 = 6^4 = C(4,2)^dims = %d. UNVERIFIED.\n\n", 6*6*6*6);

    printf("   GOBEKLI TEPE (~9500 BCE): 12 outer + 2 inner pillars\n");
    printf("     12 = %d x %d (gates x dims). 12/2 = 6 = C(4,2).\n", GATES, DIMS);
    printf("     T-pillars. Too speculative without measurements.\n\n");

    printf("   PARTHENON: 8 front + 17 side = 25 = fill%%\n");
    printf("     Greek convention (n, 2n+1). Sum = 3n+1. Formula-driven.\n\n");

    printf("   NEWGRANGE: 97 kerbstones = 25th prime. Stretch.\n");
}

/* ═══════════════════════════════════════════════════════ */

int main(void) {
    printf("Atlas — Lattice Analysis of Ancient Sites\n");
    printf("==========================================\n\n\n");

    test_pythagorean();
    test_khafre();
    test_seked();
    test_plateau();
    test_teotihuacan();
    test_chichen();
    test_stonehenge();
    test_other();

    printf("\n\n==========================================\n");
    printf("CROSS-SITE CONSTANTS\n");
    printf("==========================================\n\n");
    printf("  3^2 = 9:    Egypt (Giza) + Mexico (Chichen Itza)\n");
    printf("  1/4 = 25%%:  Egypt (Giza) + Mexico (Chichen Itza)\n");
    printf("  C(4,2) = 6: Egypt (cubit) + England (Stonehenge)\n");
    printf("  3-4-5:      Egypt (Khafre) + lattice identity\n");
    printf("  T(7) = 28:  Egypt (sekeds) + England (56 = 2x28)\n");
    printf("  pi:          Egypt (Giza) + Mexico? (Teotihuacan)\n");
    printf("  sqrt(5):    Egypt (Giza interior + plateau ratio)\n");
    printf("  137:         Egypt (Khafre modules)\n\n");
    printf("  STRONGEST: 9 = gates^2 at Giza AND Chichen Itza.\n");

    return 0;
}
