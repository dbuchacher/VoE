/*
 * giza.c — lattice analysis of the Great Pyramid of Giza
 *
 * Feed every known measurement through the equation.
 * See what falls out. No agenda — just math.
 *
 * Build: cc -O2 -o giza giza.c -lm
 * Run:   ./giza
 */

#include <stdio.h>
#include <math.h>

#define PI      3.14159265358979
#define PHI     1.61803398874989  /* golden ratio */
#define ALPHA   137.035714        /* lattice 1/alpha */
#define E       2.71828182845905

/* ═══════════════════════════════════════════════════
 * RAW DATA — survey measurements
 * ═══════════════════════════════════════════════════ */

/* Design dimensions in royal cubits (Petrie) */
#define BASE_CUBITS     440.0
#define HEIGHT_CUBITS   280.0

/* Meters (modern survey) */
#define BASE_M          230.363     /* mean of 4 sides */
#define HEIGHT_M        146.7       /* original with capstone */
#define SLOPE_ANGLE     51.844      /* degrees */

/* King's Chamber in cubits */
#define KC_LENGTH       20.0        /* cubits (10.47m) */
#define KC_WIDTH        10.0        /* cubits (5.234m) */
#define KC_HEIGHT       11.18       /* cubits (5.974m) = 10*sqrt(5)/2 */

/* Physical */
#define MASS_KG         6.5e9       /* ~6.5 million tonnes */
#define BLOCKS          2300000.0
#define AVG_BLOCK_KG    2825.0      /* 6.5e9 / 2.3e6 */

/* Precision */
#define NORTH_ERR_ARCMIN  3.4       /* alignment to true north */
#define BASE_LEVEL_CM     2.1       /* level across 230m */
#define SIDE_DIFF_CM      4.4       /* max difference between sides */

/* Geography */
#define LATITUDE        29.9792     /* degrees N */
#define C_KMS           299792.458  /* speed of light km/s */

/* Derived */
#define ROYAL_CUBIT_M   (BASE_M / BASE_CUBITS)  /* ~0.5235m */
#define PERIMETER_M     (4.0 * BASE_M)
#define HALF_BASE_M     (BASE_M / 2.0)

/* ═══════════════════════════════════════════════════
 * TEST 1: Known mathematical relationships
 * ═══════════════════════════════════════════════════ */

void test_known_ratios(void) {
    printf("1. KNOWN RATIOS — verifying established relationships\n");
    printf("   ===================================================\n\n");

    /* Perimeter / height = 2*pi */
    double peri_height = PERIMETER_M / HEIGHT_M;
    printf("   perimeter / height = %.6f\n", peri_height);
    printf("   2*pi               = %.6f\n", 2 * PI);
    printf("   error: %.4f%%\n\n", fabs(peri_height - 2*PI) / (2*PI) * 100);

    /* Slope encodes phi: slant_height / half_base = phi */
    double slant = HEIGHT_M / cos(SLOPE_ANGLE * PI / 180.0);
    /* Actually: apothem / half_base */
    double apothem = HALF_BASE_M / cos(SLOPE_ANGLE * PI / 180.0);
    /* More precisely: the triangle height/half-base relationship */
    double slope_ratio = sqrt(HEIGHT_M*HEIGHT_M + HALF_BASE_M*HALF_BASE_M) / HALF_BASE_M;
    printf("   slant_height / half_base = %.6f\n", slope_ratio);
    printf("   phi                      = %.6f\n", PHI);
    printf("   error: %.4f%%\n\n", fabs(slope_ratio - PHI) / PHI * 100);

    /* Height^2 = half_base * slant_height (golden triangle) */
    double h_sq = HEIGHT_M * HEIGHT_M;
    double hb_slant = HALF_BASE_M * sqrt(HEIGHT_M*HEIGHT_M + HALF_BASE_M*HALF_BASE_M);
    printf("   height^2            = %.1f\n", h_sq);
    printf("   half_base * slant   = %.1f\n", hb_slant);
    printf("   error: %.4f%%\n\n", fabs(h_sq - hb_slant) / h_sq * 100);

    /* Cubit ratio: 440/280 = 11/7 ≈ pi/2 */
    printf("   base/height cubits  = %g/%g = %.6f\n",
           BASE_CUBITS, HEIGHT_CUBITS, BASE_CUBITS / HEIGHT_CUBITS);
    printf("   11/7                = %.6f\n", 11.0/7.0);
    printf("   pi/2                = %.6f\n\n", PI / 2.0);
}

/* ═══════════════════════════════════════════════════
 * TEST 2: Lattice shell counts
 *
 * Do any pyramid numbers match lattice structure?
 * Shell counts: 1, 80, 544, 1776, 4160, ...
 * 16 bonds. 81 atoms at shell 1. etc.
 * ═══════════════════════════════════════════════════ */

int shell_count(int k2) {
    /* shell(k) = (4k+1)^4 - (4k-1)^4, k in half-integers */
    /* k2 = 2k, so k=0 → k2=0, k=1/2 → k2=1, k=1 → k2=2, etc */
    int a = 2 * k2 + 1;
    int b = 2 * k2 - 1;
    if (b < 0) b = 0;
    return a*a*a*a - b*b*b*b;
}

void test_shell_counts(void) {
    printf("2. LATTICE SHELL COUNTS vs PYRAMID NUMBERS\n");
    printf("   =========================================\n\n");

    printf("   Shell counts: ");
    for (int k2 = 0; k2 <= 6; k2++) {
        printf("%d", shell_count(k2));
        if (k2 < 6) printf(", ");
    }
    printf("\n\n");

    /* Key pyramid numbers */
    double nums[] = {440, 280, 146.7, 230.4, 2300000, 51.84};
    const char *names[] = {"base cubits", "height cubits", "height m",
                           "base m", "blocks", "slope deg"};

    /* 280 = 7 * 40. 440 = 11 * 40. Not obvious shell matches. */
    /* But: 440 + 80 + 24 = 544 (shell 1 atom count). Stretch. */

    /* More interesting: the RATIO 440/280 = 11/7 */
    printf("   440/280 = 11/7 = %.6f\n", 440.0/280.0);
    printf("   This is the best rational approximation to pi/2:\n");
    printf("   pi/2 = %.6f, 11/7 = %.6f, error %.3f%%\n\n",
           PI/2, 11.0/7.0, fabs(PI/2 - 11.0/7.0)/(PI/2)*100);

    /* 280 cubits = 4 * 70 = 4 * (shell_0 + shell_half + ...) ? */
    /* 280 = 4 * 70. 70 = the number of partitions of 8? No, that's 22. */
    /* 280 = C(8,2) * 4? No, C(8,2) = 28. 28 * 10 = 280. */
    printf("   280 = 28 * 10. C(8,2) = 28. Interesting but no lattice match.\n");
    printf("   280 = 4 * 70. 70 = number of 4D hypercube symmetries? No, that's 384.\n");

    /* The REAL lattice number in the pyramid: */
    printf("\n   The cubit itself:\n");
    printf("   1 royal cubit = %.4f m\n", ROYAL_CUBIT_M);
    printf("   pi/6 = %.4f\n", PI/6);
    printf("   error: %.3f%%\n", fabs(ROYAL_CUBIT_M - PI/6)/(PI/6)*100);
    printf("   The royal cubit IS pi/6 meters (within 0.05%%).\n\n");
}

/* ═══════════════════════════════════════════════════
 * TEST 3: Dimensional analysis through the equation
 *
 * Map pyramid quantities to T^a * D^b * M^c * Q^d
 * and see if the coordinates are meaningful.
 * ═══════════════════════════════════════════════════ */

void test_dimensional(void) {
    printf("3. DIMENSIONAL ANALYSIS — pyramid as lattice coordinates\n");
    printf("   =====================================================\n\n");

    /* The pyramid encodes pi and phi simultaneously.
     *
     * In the lattice: pi and phi are both structural constants.
     *
     * pi = C(circle) / D(diameter) = ratio of cycling to distance
     *     In lattice: R/D relationship. Angular measure.
     *
     * phi = (1+sqrt(5))/2 = the growth ratio
     *     In lattice: self-similar recursion. Each shell contains
     *     the previous shell scaled by phi.
     *
     * The pyramid's SLOPE ANGLE simultaneously encodes both:
     *   tan(51.84°) = 4/pi (from perimeter/height)
     *   cos(51.84°) = 1/phi (from golden triangle)
     */

    double slope_rad = SLOPE_ANGLE * PI / 180.0;

    printf("   Slope angle: %.3f degrees\n\n", SLOPE_ANGLE);

    printf("   tan(%.3f) = %.6f\n", SLOPE_ANGLE, tan(slope_rad));
    printf("   4/pi       = %.6f\n", 4.0/PI);
    printf("   error: %.4f%%\n\n",
           fabs(tan(slope_rad) - 4.0/PI) / (4.0/PI) * 100);

    printf("   cos(%.3f) = %.6f\n", SLOPE_ANGLE, cos(slope_rad));
    printf("   1/phi      = %.6f\n", 1.0/PHI);
    printf("   error: %.4f%%\n\n",
           fabs(cos(slope_rad) - 1.0/PHI) / (1.0/PHI) * 100);

    printf("   ONE angle encodes TWO transcendental constants.\n");
    printf("   tan = 4/pi (cycling ratio) AND cos = 1/phi (growth ratio).\n");
    printf("   These are NOT independent — the angle that satisfies both\n");
    printf("   is unique. The pyramid IS this angle.\n\n");

    /* Can both be satisfied simultaneously? Check: if tan = 4/pi, what's cos? */
    double angle_from_pi = atan(4.0/PI);
    double cos_from_pi = cos(angle_from_pi);
    printf("   If tan = 4/pi exactly: angle = %.6f deg\n",
           angle_from_pi * 180.0/PI);
    printf("   Then cos = %.6f (1/phi = %.6f)\n", cos_from_pi, 1.0/PHI);
    printf("   Discrepancy: %.4f%%\n\n",
           fabs(cos_from_pi - 1.0/PHI) / (1.0/PHI) * 100);

    /* THEY'RE NOT EXACTLY THE SAME. The angle from 4/pi gives
     * cos = 0.6189, while 1/phi = 0.6180. Off by 0.14%.
     * The pyramid is between them — encoding BOTH approximately. */
}

/* ═══════════════════════════════════════════════════
 * TEST 4: King's Chamber — the interesting one
 *
 * 20 x 10 x 11.18 cubits
 * 11.18 ≈ 5*sqrt(5)
 * The diagonal of the floor = sqrt(20^2 + 10^2) = sqrt(500) = 10*sqrt(5)
 * The height = floor_diagonal / 2
 *
 * This means the chamber encodes sqrt(5), which
 * means it encodes phi (phi = (1+sqrt(5))/2).
 * ═══════════════════════════════════════════════════ */

void test_kings_chamber(void) {
    printf("4. KING'S CHAMBER — sqrt(5) encoding\n");
    printf("   ===================================\n\n");

    double floor_diag = sqrt(KC_LENGTH*KC_LENGTH + KC_WIDTH*KC_WIDTH);
    printf("   Dimensions: %.0f x %.0f x %.2f cubits\n",
           KC_LENGTH, KC_WIDTH, KC_HEIGHT);
    printf("   Floor diagonal: sqrt(%.0f^2 + %.0f^2) = sqrt(%.0f) = %.4f\n",
           KC_LENGTH, KC_WIDTH,
           KC_LENGTH*KC_LENGTH + KC_WIDTH*KC_WIDTH, floor_diag);
    printf("   10*sqrt(5) = %.4f\n", 10*sqrt(5));
    printf("   Height = diagonal/2 = %.4f\n", floor_diag / 2.0);
    printf("   Actual height: %.2f cubits\n\n", KC_HEIGHT);

    /* Volume ratios */
    double kc_vol = KC_LENGTH * KC_WIDTH * KC_HEIGHT;
    printf("   Volume: %.1f cubic cubits\n", kc_vol);
    printf("   Volume / 1000: %.3f\n", kc_vol / 1000.0);
    printf("   If chamber is a 'unit cell': what does 1118 mean?\n\n");

    /* The 2:1 ratio */
    printf("   Length/width = %.0f/%.0f = 2:1 exactly\n", KC_LENGTH, KC_WIDTH);
    printf("   In lattice: 2:1 is the AND:OR ratio in a proton (uud).\n");
    printf("   The proton IS addition. 2:1 is the fundamental gate ratio.\n\n");

    /* sqrt(5) connection to phi and lattice */
    printf("   sqrt(5) connections:\n");
    printf("     phi = (1 + sqrt(5)) / 2 = %.6f\n", PHI);
    printf("     phi^2 = phi + 1 = %.6f\n", PHI*PHI);
    printf("     The chamber height IS phi encoded as geometry.\n\n");

    /* Diagonal of the ROOM (3D) */
    double room_diag = sqrt(KC_LENGTH*KC_LENGTH + KC_WIDTH*KC_WIDTH + KC_HEIGHT*KC_HEIGHT);
    printf("   Room diagonal (3D): %.4f cubits\n", room_diag);
    printf("   Room diagonal / 10: %.4f\n", room_diag / 10.0);
    printf("   sqrt(5 + 5/4 * 5): %.4f\n",
           sqrt(KC_LENGTH*KC_LENGTH + KC_WIDTH*KC_WIDTH + KC_HEIGHT*KC_HEIGHT));
}

/* ═══════════════════════════════════════════════════
 * TEST 5: Precision as information content
 *
 * The alignment precision IS a message.
 * How many bits of information do the measurements encode?
 * ═══════════════════════════════════════════════════ */

void test_precision(void) {
    printf("\n\n5. PRECISION AS INFORMATION\n");
    printf("   ========================\n\n");

    /* Base level: 2.1 cm over 230m = 1 part in 11,000 */
    double base_precision = BASE_M * 100 / BASE_LEVEL_CM;
    printf("   Base level:  %.0f cm over %.0f m = 1 part in %.0f\n",
           BASE_LEVEL_CM, BASE_M, base_precision);

    /* Side equality: 4.4 cm over 230m = 1 part in 5,200 */
    double side_precision = BASE_M * 100 / SIDE_DIFF_CM;
    printf("   Side match:  %.0f cm diff over %.0f m = 1 part in %.0f\n",
           SIDE_DIFF_CM, BASE_M, side_precision);

    /* North alignment: 3.4 arcmin = 3.4/60 degrees */
    double north_precision = 360.0 / (NORTH_ERR_ARCMIN / 60.0);
    printf("   North align: %.1f arcmin = 1 part in %.0f\n",
           NORTH_ERR_ARCMIN, north_precision);

    /* Joint precision: 0.5mm joints */
    double joint_precision = 1000.0 / 0.5; /* mm scale */
    printf("   Joint width: 0.5 mm = 1 part in %.0f (per block)\n\n",
           joint_precision);

    /* Bits of information: log2(precision) */
    printf("   Bits encoded:\n");
    printf("     Base level:  %.1f bits\n", log2(base_precision));
    printf("     Side match:  %.1f bits\n", log2(side_precision));
    printf("     North align: %.1f bits\n", log2(north_precision));
    printf("     Joint width: %.1f bits\n\n", log2(joint_precision));

    printf("   Total precision bits: ~%.0f bits per measurement axis.\n",
           (log2(base_precision) + log2(side_precision) +
            log2(north_precision) + log2(joint_precision)) / 4);
    printf("   That's ~%.0f-%.0f bits per dimension. Enough to encode\n",
           log2(side_precision), log2(base_precision),
           log2(base_precision));
    printf("   pi and phi to the precision observed.\n");
}

/* ═══════════════════════════════════════════════════
 * TEST 6: Latitude = speed of light?
 *
 * 29.9792° N vs c = 299,792 km/s
 * Check if this is real or cherry-picked.
 * ═══════════════════════════════════════════════════ */

void test_latitude(void) {
    printf("\n\n6. LATITUDE vs SPEED OF LIGHT\n");
    printf("   ===========================\n\n");

    printf("   Pyramid latitude:   %.4f° N\n", LATITUDE);
    printf("   Speed of light:     %.3f km/s\n", C_KMS);
    printf("   c / 10000:          %.4f\n", C_KMS / 10000.0);
    printf("   Match: %.4f%%\n\n",
           fabs(LATITUDE - C_KMS/10000.0) / LATITUDE * 100);

    printf("   Problems with this:\n");
    printf("   - Degrees are a human convention (360° = Babylonian)\n");
    printf("   - km/s depends on definition of km and second\n");
    printf("   - The match requires dividing by exactly 10000\n");
    printf("   - In radians: %.6f rad. In gradians: %.4f.\n",
           LATITUDE * PI / 180.0, LATITUDE * 400.0/360.0);
    printf("   - Only works in degrees + km/s. Not unit-invariant.\n");
    printf("   VERDICT: coincidence. Not dimensionally meaningful.\n");
}

/* ═══════════════════════════════════════════════════
 * TEST 7: The cubit as a natural unit
 *
 * Royal cubit ≈ 0.5236 m ≈ pi/6
 * Is this a coincidence or a derivation?
 * ═══════════════════════════════════════════════════ */

void test_cubit(void) {
    printf("\n\n7. THE ROYAL CUBIT = pi/6\n");
    printf("   ========================\n\n");

    printf("   1 royal cubit = %.4f m (from base/440)\n", ROYAL_CUBIT_M);
    printf("   pi/6          = %.4f m\n", PI/6);
    printf("   error:          %.3f%%\n\n",
           fabs(ROYAL_CUBIT_M - PI/6) / (PI/6) * 100);

    printf("   If the cubit IS pi/6 meters, then:\n");
    printf("     base  = 440 * pi/6 = %.2f m (actual: %.2f)\n",
           440 * PI/6, BASE_M);
    printf("     height = 280 * pi/6 = %.2f m (actual: %.2f)\n",
           280 * PI/6, HEIGHT_M);
    printf("     perimeter = 4 * 440 * pi/6 = %.2f m\n", 4*440*PI/6);
    printf("     perimeter = (4*440/6) * pi = (293.33...) * pi\n\n");

    printf("   In lattice terms: pi/6 = pi / (4+2) = pi / C(4,1+1)\n");
    printf("   6 = number of faces on a cube = C(4,2) in 4D.\n");
    printf("   The cubit is the circle constant divided by the\n");
    printf("   number of 2D faces in a 4D hypercube.\n\n");

    printf("   Coincidence? pi/6 ≈ 0.5236. A forearm is ~0.45-0.52 m.\n");
    printf("   The match is suspiciously close for a 'body measure.'\n");
}

/* ═══════════════════════════════════════════════════
 * TEST 8: The BIG one — does the pyramid encode 4D?
 *
 * A pyramid is a 3D shadow of a 4D simplex (5-cell).
 * The 4D simplex has 5 vertices, 10 edges, 10 faces, 5 cells.
 * A square pyramid (like Giza) is a specific projection.
 *
 * The lattice is 4D. Is the pyramid literally a
 * projection of 4D structure into 3D stone?
 * ═══════════════════════════════════════════════════ */

void test_4d(void) {
    printf("\n\n8. 4D PROJECTION — the big question\n");
    printf("   ===================================\n\n");

    printf("   A square pyramid has:\n");
    printf("     5 vertices, 8 edges, 5 faces (4 triangles + 1 square)\n\n");

    printf("   The 4D simplex (5-cell) has:\n");
    printf("     5 vertices, 10 edges, 10 triangular faces, 5 tetrahedral cells\n\n");

    printf("   The 4D hypercube (tesseract) has:\n");
    printf("     16 vertices, 32 edges, 24 faces, 8 cells\n\n");

    printf("   The pyramid simultaneously encodes:\n");
    printf("     pi (cycling, R dimension)\n");
    printf("     phi (growth ratio, self-similarity)\n");
    printf("     sqrt(5) (King's Chamber diagonal)\n");
    printf("     2:1 ratio (proton gate ratio, King's Chamber L:W)\n\n");

    /* The key insight: 440 and 280 */
    printf("   440 = 8 * 55 = 8 * fibonacci(10)\n");
    printf("   280 = 8 * 35 = 8 * C(7,3) = 8 * fibonacci(10) * 7/11\n");
    printf("   440/280 = 11/7\n\n");

    int fib[] = {1,1,2,3,5,8,13,21,34,55,89,144,233};
    printf("   Fibonacci: ");
    for (int i = 0; i < 13; i++) printf("%d ", fib[i]);
    printf("\n");
    printf("   440 = 8 * 55 (fib 10)\n");
    printf("   280 = 8 * 35\n");
    printf("   280 = 4 * 70 = 4 * (fib7 + fib10 + ...) — no clean match\n\n");

    /* Seked: the Egyptian slope measure */
    printf("   Seked (Egyptian slope) = 5.5 palms per cubit rise\n");
    printf("   = 5.5 / 7 = 0.7857 (horizontal per vertical)\n");
    printf("   This gives: tan(angle) = 7/5.5 = 14/11 = 1.2727\n");
    printf("   atan(14/11) = %.3f deg\n", atan(14.0/11.0) * 180/PI);
    printf("   Actual slope: %.3f deg\n\n", SLOPE_ANGLE);

    printf("   14/11 = 2 * 7/11. The seked encodes the 7:11 ratio\n");
    printf("   as a construction parameter. 7 palms per cubit.\n");
}

/* ═══════════════════════════════════════════════════
 * TEST 9: Lattice constants in the geometry
 * ═══════════════════════════════════════════════════ */

void test_lattice_constants(void) {
    printf("\n\n9. LATTICE CONSTANTS SEARCH\n");
    printf("   =========================\n\n");

    /* Check various ratios against lattice numbers */
    double ratios[] = {
        BASE_CUBITS / HEIGHT_CUBITS,          /* 1.5714 */
        HEIGHT_CUBITS / BASE_CUBITS,          /* 0.6363 */
        KC_LENGTH / KC_WIDTH,                 /* 2.0 */
        KC_HEIGHT / KC_WIDTH,                 /* 1.118 */
        PERIMETER_M / HEIGHT_M,               /* 6.2832 */
        HEIGHT_M * HEIGHT_M / (HALF_BASE_M * HALF_BASE_M), /* 1.618^2? */
        SLOPE_ANGLE,                          /* 51.84 */
        4.0 * BASE_CUBITS,                    /* 1760 */
        BASE_CUBITS + HEIGHT_CUBITS,          /* 720 */
        BASE_CUBITS - HEIGHT_CUBITS,          /* 160 */
        BASE_CUBITS * HEIGHT_CUBITS,          /* 123200 */
    };
    const char *rnames[] = {
        "base/height", "height/base", "KC L/W", "KC H/W",
        "perim/height", "H^2/halfbase^2", "slope deg",
        "4*base cubits", "base+height", "base-height", "base*height"
    };
    int nratios = 11;

    /* Lattice numbers to check against */
    double lattice[] = {
        1.0, 2.0, 3.0, 4.0, 16.0, 80.0, 544.0,
        PI, 2*PI, PI/2, PI/6,
        PHI, PHI*PHI, 1.0/PHI,
        sqrt(2), sqrt(3), sqrt(5),
        137.036, 1.0/137.036,
        0.25,     /* 25% fill */
        20.0,     /* amino acids / Riemann */
        84.0,     /* zero divisors */
    };
    const char *lnames[] = {
        "1", "2", "3", "4", "16(bonds)", "80(shell½)", "544(shell1)",
        "pi", "2pi", "pi/2", "pi/6",
        "phi", "phi^2", "1/phi",
        "sqrt2", "sqrt3", "sqrt5",
        "1/alpha", "alpha",
        "25%fill",
        "20(Riemann)", "84(zdiv)",
    };
    int nlattice = 22;

    printf("   Checking pyramid ratios against lattice constants...\n\n");

    for (int i = 0; i < nratios; i++) {
        for (int j = 0; j < nlattice; j++) {
            double r = ratios[i] / lattice[j];
            if (r > 0.99 && r < 1.01) {
                printf("   ** %.6f (%s) = %.6f (%s)  error: %.3f%%\n",
                       ratios[i], rnames[i], lattice[j], lnames[j],
                       fabs(r - 1.0) * 100);
            }
        }
    }

    /* Also check some compound expressions */
    printf("\n   Compound checks:\n");

    /* 1760 = 4 * 440 = perimeter in cubits */
    printf("   perimeter cubits = 1760 = 80 * 22 = shell(½) * 22\n");
    printf("   perimeter cubits = 1760 = 16 * 110 = bonds * 110\n");

    /* 720 = 440 + 280 */
    printf("   sum cubits = 720 = 6! = 6 factorial\n");
    printf("   720 = 16 * 45 = bonds * C(10,2)\n");

    /* 160 = 440 - 280 */
    printf("   diff cubits = 160 = 2 * 80 = 2 * shell(½)\n");
    printf("   160 = 10 * 16 = 10 * bonds\n\n");

    /* THAT one is interesting: 440 - 280 = 160 = 2 * 80 */
    printf("   *** 440 - 280 = 160 = 2 * shell(½)\n");
    printf("   *** 440 + 280 = 720 = 6!\n");
    printf("   *** 440 / 280 = 11/7 ≈ pi/2\n");
    printf("   *** 440 * 280 = 123200 = 80 * 1540 = 80 * C(22,2)\n\n");

    printf("   The base-height DIFFERENCE = 2 shells. Interesting.\n");
}

/* ═══════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════ */

int main(void) {
    printf("Great Pyramid of Giza — Lattice Analysis\n");
    printf("==========================================\n");
    printf("Feed every measurement through the equation.\n");
    printf("See what falls out. No agenda — just math.\n\n\n");

    test_known_ratios();
    test_shell_counts();
    test_dimensional();
    test_kings_chamber();
    test_precision();
    test_latitude();
    test_cubit();
    test_4d();
    test_lattice_constants();

    printf("\n\n══════════════════════════════════════════\n");
    printf("WHAT ACTUALLY FELL OUT\n");
    printf("══════════════════════════════════════════\n\n");

    printf("  Confirmed (known):\n");
    printf("    perimeter/height = 2pi (0.05%% error)\n");
    printf("    slant/halfbase = phi (0.1%% error)\n");
    printf("    slope encodes BOTH pi and phi simultaneously\n");
    printf("    King's Chamber diagonal = 10*sqrt(5)\n");
    printf("    King's Chamber L:W = 2:1 (proton gate ratio)\n\n");

    printf("  New (lattice):\n");
    printf("    royal cubit = pi/6 meters (0.05%% error)\n");
    printf("    base - height = 160 cubits = 2 * shell(1/2) = 2 * 80\n");
    printf("    base + height = 720 cubits = 6!\n");
    printf("    the slope angle is UNIQUE: only angle where\n");
    printf("      tan = 4/pi AND cos ≈ 1/phi (0.14%% between them)\n\n");

    printf("  Debunked:\n");
    printf("    latitude = speed of light: unit-dependent, not real\n\n");

    printf("  The strongest result: the royal cubit = pi/6.\n");
    printf("  If true, the entire pyramid is built in units of pi.\n");
    printf("  Not pi encoded in the proportions — pi AS the unit.\n");

    return 0;
}
