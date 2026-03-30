/*
 * sky.c — lattice analysis of the sky
 *
 * Feed astronomical data through the equation.
 * Same method as giza.c and atlas.c.
 *
 * Build: cc -O2 -o sky sky.c -lm
 * Run:   ./sky
 */

#include <stdio.h>
#include <math.h>

#define PI    3.14159265358979
#define PHI   1.61803398874989
#define SQRT5 2.23606797749979
#define CUBIT (PI/6.0)

#define GATES 3
#define DIMS  4
#define BONDS 16
#define FILL  25

long shell(int k2) {
    long a = 2*k2 + 1;
    long b = (k2 > 0) ? 2*k2 - 1 : 0;
    return a*a*a*a - b*b*b*b;
}

long triangular(int n) { return (long)n*(n+1)/2; }

/* ═══════════════════════════════════════════════════════
 * 1. STRUCTURAL COUNTS — how many of X in the sky?
 * ═════��═════════════════════════════════════════════════ */

void test_counts(void) {
    printf("1. STRUCTURAL COUNTS\n");
    printf("   ==================\n\n");

    struct { int n; const char *what; const char *lattice; } counts[] = {
        {3,  "Orion's Belt stars",             "gates (AND, OR, NOT)"},
        {4,  "Southern Cross stars",           "dimensions (T, D, M, Q)"},
        {5,  "Platonic solids",                "sqrt(fill%) = sqrt(25)"},
        {7,  "MK spectral classes (OBAFGKM)",  "palms per cubit"},
        {7,  "Big Dipper stars",               "palms per cubit"},
        {7,  "Little Dipper stars",            "palms per cubit"},
        {7,  "Orion body stars",               "palms per cubit"},
        {7,  "Corona Borealis stars",          "palms per cubit"},
        {7,  "Bootes stars",                   "palms per cubit"},
        {7,  "classical celestial bodies",     "palms per cubit"},
        {8,  "solar system planets",           "gluons = 3^2 - 1"},
        {12, "zodiac constellations",          "gates x dims = 3 x 4"},
        {22, "first-magnitude stars",          "Khufu seked numerator (22/4)"},
        {88, "IAU constellations",             "gluons x 11 = 8 x 11"},
    };
    int n = sizeof(counts) / sizeof(counts[0]);

    for (int i = 0; i < n; i++) {
        printf("   %3d  %-35s = %s\n",
               counts[i].n, counts[i].what, counts[i].lattice);
    }

    printf("\n   the number 7 dominates the sky:\n");
    printf("     7 spectral classes. 7 classical wanderers (5 planets + sun + moon).\n");
    printf("     7 days/week (named for the 7 wanderers).\n");
    printf("     5+ constellations with exactly 7 stars.\n");
    printf("     7 = palms per cubit. the subdivision unit.\n");
}

/* ═══════════════════════════════════════════════════════
 * 2. ORION'S BELT — the 3
 * ══════════════════════��════════════════════════════��═══ */

void test_belt(void) {
    printf("\n\n2. ORION'S BELT\n");
    printf("   =============\n\n");

    /* angular separations (degrees) */
    double az_an = 1.37;   /* Alnitak - Alnilam */
    double an_mn = 1.38;   /* Alnilam - Mintaka */
    double total = 2.73;   /* Alnitak - Mintaka */

    printf("   Alnitak - Alnilam:  ~%.2f deg\n", az_an);
    printf("   Alnilam - Mintaka:  ~%.2f deg\n", an_mn);
    printf("   total span:         ~%.2f deg\n\n", total);

    printf("   1.37 deg x 100 = 137 = 1/alpha\n");
    printf("   BUT: degrees are a human convention (Babylonian 360).\n");
    printf("   same problem as latitude = c in giza.c. UNIT-DEPENDENT.\n");
    printf("   verdict: suggestive but not dimensionally meaningful.\n\n");

    /* magnitudes */
    double mag[] = {1.77, 1.69, 2.23};
    const char *names[] = {"Alnitak", "Alnilam", "Mintaka"};

    printf("   magnitudes: ");
    for (int i = 0; i < 3; i++)
        printf("%s=%.2f  ", names[i], mag[i]);
    printf("\n");

    /* brightness ratios (mag is log scale: 1 mag = 2.512x brightness) */
    double bright_ratio = pow(10, (mag[2]-mag[0])/2.5);
    printf("   Alnitak/Mintaka brightness: %.3f\n", bright_ratio);
    printf("   Alnilam/Mintaka brightness: %.3f\n",
           pow(10, (mag[2]-mag[1])/2.5));
    printf("   phi = %.3f. not a match.\n\n", PHI);

    printf("   the structural hit: 3 stars = %d = gates.\n", GATES);
    printf("   the angular separation (1.37 = 137/100) is unit-dependent.\n");
    printf("   the magnitudes don't encode obvious lattice constants.\n");
}

/* ══════════════════════════════���════════════════════════
 * 3. SUN AND MOON — the cubit in the sky?
 * ══���═══════════════���════════════════════════════════���═══ */

void test_sun_moon(void) {
    printf("\n\n3. SUN AND MOON APPARENT SIZE\n");
    printf("   ===========================\n\n");

    double sun_deg = 0.533;   /* mean apparent diameter */
    double moon_deg = 0.518;  /* mean apparent diameter */
    double cubit_val = PI/6;  /* 0.5236 — the royal cubit VALUE */

    printf("   Sun:  %.3f deg    Moon: %.3f deg\n", sun_deg, moon_deg);
    printf("   pi/6: %.4f (the cubit as a pure number)\n\n", cubit_val);

    printf("   Sun vs pi/6:  error %.2f%%\n",
           fabs(sun_deg - cubit_val)/cubit_val * 100);
    printf("   Moon vs pi/6: error %.2f%%\n\n",
           fabs(moon_deg - cubit_val)/cubit_val * 100);

    printf("   Sun/Moon ratio: %.4f (why eclipses work)\n\n",
           sun_deg/moon_deg);

    printf("   pi/6 = 0.5236 appears as:\n");
    printf("     royal cubit in METERS (the building unit)\n");
    printf("     sun/moon apparent size in DEGREES (the sky unit)\n");
    printf("   same VALUE, different dimensions.\n\n");

    printf("   CAUTION: degrees are conventional. but the COINCIDENCE\n");
    printf("   that builders chose pi/6 meters as their unit, and the\n");
    printf("   two brightest sky objects both subtend ~pi/6 degrees,\n");
    printf("   is at minimum notable.\n");
}

/* ════════════════════════════════════════════���══════════
 * 4. KEPLER — the lattice IS the orbit law
 * ════════════════���══════════════════════════════���═══════ */

void test_kepler(void) {
    printf("\n\n4. KEPLER'S THIRD LAW = CODEX ENTRY\n");
    printf("   ==================================\n\n");

    printf("   T^2 = a^3    (T in years, a in AU)\n\n");
    printf("   Dimensionally: T^2 / D^3 = GM = gravitational parameter\n");
    printf("   Rearranged: D^3 / T^2 = codex entry for grav_param\n\n");

    /* verify with actual data */
    struct { const char *name; double T; double a; } planet[] = {
        {"Mercury", 0.2408, 0.387},
        {"Venus",   0.6152, 0.723},
        {"Earth",   1.0000, 1.000},
        {"Mars",    1.8809, 1.524},
        {"Jupiter", 11.862, 5.203},
        {"Saturn",  29.457, 9.537},
        {"Uranus",  84.011, 19.19},
        {"Neptune", 164.8,  30.07},
    };

    printf("   planet      T(yr)    a(AU)    T^2       a^3       a^3/T^2\n");
    for (int i = 0; i < 8; i++) {
        double T2 = planet[i].T * planet[i].T;
        double a3 = planet[i].a * planet[i].a * planet[i].a;
        printf("   %-9s %7.3f  %6.3f  %9.3f %9.3f   %.4f\n",
               planet[i].name, planet[i].T, planet[i].a, T2, a3, a3/T2);
    }
    printf("\n   a^3/T^2 = 1.000 for all planets (= GM_sun in AU^3/yr^2)\n");
    printf("   this IS the lattice. D^3/T^2 is a codex atom.\n");
    printf("   Kepler didn't find a pattern. he found a coordinate.\n");
}

/* ═══════════��═════════════════════════════════════��═════
 * 5. RESONANCES — binary and musical
 * ═════════════���══════════════════════════════��══════════ */

void test_resonances(void) {
    printf("\n\n5. ORBITAL RESONANCES\n");
    printf("   ====================\n\n");

    printf("   Io : Europa : Ganymede = 1 : 2 : 4  (Laplace)\n");
    printf("   = 2^0 : 2^1 : 2^2 = binary doubling\n\n");

    printf("   Neptune : Pluto = 3 : 2  (exact resonance)\n");
    printf("   = Khafre base:height ratio = musical perfect fifth\n\n");

    printf("   Titius-Bode: a = 0.4 + 0.3 x 2^n\n");
    printf("   the 2^n term = binary scaling in D coordinate.\n");
    printf("   breaks at Neptune (predicted 38.8, actual 30.1).\n\n");

    printf("   binary doubling in orbital distances = the lattice's\n");
    printf("   binary force structure projected onto the D dimension.\n");
    printf("   4 forces, 2 states each = 2^4 bonds. orbits scale as 2^n.\n");
}

/* ════════════════════���══════════════════════════════════
 * 6. PRECESSION AND CYCLES
 * ═���═════════════════════════════════════════════════════ */

void test_precession(void) {
    printf("\n\n6. PRECESSION AND CYCLES\n");
    printf("   ======================\n\n");

    double prec_modern = 25772;
    double prec_trad = 25920;

    printf("   precession (modern): %.0f years\n", prec_modern);
    printf("   precession (traditional): %.0f years\n\n", prec_trad);

    printf("   traditional / 360 = %.1f (= SM correspondences?)\n",
           prec_trad / 360);
    printf("   72 = 8 x 9 = gluons x gates^2\n");
    printf("   BUT: 360 is Babylonian convention. unit-dependent.\n\n");

    printf("   Saros cycle: 223 synodic months = ~18.03 years\n");
    printf("   3 x Saros = Exeligmos = ~54.09 years\n");
    printf("   Stonehenge: 56 Aubrey holes ~ 3 x 18.6 lunar node cycles\n");
    printf("   56 = 2 x 28 = 2 x T(7)\n");
}

/* ═════���══════════════��══════════════════════════════════
 * 7. PLATONIC SOLIDS — geometry's periodic table
 * ═════════════��═════════════════════════════��═══════════ */

void test_platonic(void) {
    printf("\n\n7. PLATONIC SOLIDS\n");
    printf("   ================\n\n");

    struct { const char *name; int F; int E; int V; } solid[] = {
        {"tetrahedron",   4,  6,  4},
        {"cube",          6, 12,  8},
        {"octahedron",    8, 12,  6},
        {"dodecahedron", 12, 30, 20},
        {"icosahedron",  20, 30, 12},
    };

    int sumF = 0, sumE = 0, sumV = 0;
    printf("   solid          F    E    V    lattice meaning\n");
    for (int i = 0; i < 5; i++) {
        printf("   %-14s %2d  %3d  %3d", solid[i].name,
               solid[i].F, solid[i].E, solid[i].V);
        if (solid[i].F == 4) printf("    dims");
        if (solid[i].F == 6) printf("    C(4,2)");
        if (solid[i].F == 8) printf("    gluons");
        if (solid[i].F == 12) printf("    gates x dims");
        if (solid[i].F == 20) printf("    amino acids / Riemann");
        printf("\n");
        sumF += solid[i].F;
        sumE += solid[i].E;
        sumV += solid[i].V;
    }

    printf("\n   sum of faces:    %d = 2 x %d = 2 x fill%%\n", sumF, FILL);
    printf("   sum of vertices: %d = 2 x %d = 2 x fill%%\n", sumV, FILL);
    printf("   sum of edges:    %d = 10 x %d = 10 x gates^2\n\n", sumE, GATES*GATES);

    printf("   EVERY face count is a lattice number: 4, 6, 8, 12, 20.\n");
    printf("   5 solids exist because d=3 admits exactly 5.\n");
    printf("   (Euler formula + convexity constraint.)\n");
}

/* ═══════════════════════════════════════════════════════ */

int main(void) {
    printf("Sky — Lattice Analysis of Astronomical Data\n");
    printf("=============================================\n\n\n");

    test_counts();
    test_belt();
    test_sun_moon();
    test_kepler();
    test_resonances();
    test_precession();
    test_platonic();

    printf("\n\n=============================================\n");
    printf("WHAT FELL OUT\n");
    printf("=============================================\n\n");

    printf("  STRUCTURAL (the numbers are what they are):\n\n");
    printf("    3  belt stars           = gates\n");
    printf("    4  Cross stars          = dims\n");
    printf("    5  Platonic solids      = sqrt(fill)\n");
    printf("    7  spectral classes     = palms/cubit\n");
    printf("    7  classical wanderers  = palms/cubit\n");
    printf("    8  planets              = gluons\n");
    printf("    12 zodiac signs         = gates x dims\n");
    printf("    22 first-mag stars      = Khufu seked numerator\n\n");

    printf("  PHYSICAL LAWS THAT ARE CODEX ENTRIES:\n\n");
    printf("    Kepler T^2 = a^3        = D^3/T^2 = grav_param\n");
    printf("    Laplace 1:2:4           = binary doubling (2^n)\n");
    printf("    Neptune:Pluto 3:2       = Khafre base:height\n\n");

    printf("  NOTABLE BUT UNIT-DEPENDENT:\n\n");
    printf("    Sun/Moon ~ 0.53 deg     ~ pi/6 (the cubit VALUE)\n");
    printf("    Belt separation ~1.37 deg  ~ 137/100 (1/alpha)\n");
    printf("    precession/360 ~ 72     (SM correspondences)\n\n");

    printf("  PATTERN: the sky's structural numbers ARE the lattice set.\n");
    printf("  3, 4, 5, 7, 8, 12 — the same numbers in the codex,\n");
    printf("  in the pyramids, and in the sky. because they're\n");
    printf("  structural constants of d=4 geometry, and physics\n");
    printf("  runs on d=4 geometry.\n");

    return 0;
}
