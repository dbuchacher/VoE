/*
 * moon.c — lattice analysis of the Moon
 *
 * The Moon has "impossible" coincidences.
 * Feed them through the equation.
 *
 * Build: cc -O2 -o moon moon.c -lm
 * Run:   ./moon
 */

#include <stdio.h>
#include <math.h>

#define PI    3.14159265358979
#define PHI   1.61803398874989
#define SQRT5 2.23606797749979

#define GATES 3
#define DIMS  4
#define BONDS 16
#define FILL  25

/* ═══════════════════════════════════════════════════════
 * RAW DATA — all from standard references
 * ═══════════════════════════════════════════════════════ */

/* diameters (km) */
#define MOON_D    3474.8
#define EARTH_D   12742.0
#define SUN_D     1392700.0

/* masses (kg) */
#define MOON_M    7.342e22
#define EARTH_M   5.972e24

/* distances (km, mean) */
#define MOON_DIST 384400.0
#define SUN_DIST  149597870.0

/* surface gravity (m/s^2) */
#define MOON_G    1.625
#define EARTH_G   9.807

/* orbital periods (days) */
#define SIDEREAL  27.322    /* sidereal month */
#define SYNODIC   29.530    /* synodic month */

/* apparent angular diameters (degrees, mean) */
#define SUN_ANG   0.533
#define MOON_ANG  0.518

/* ═══════════════════════════════════════════════════════
 * 1. DIAMETER RATIO: Moon/Earth = 3/11
 * ═══════════════════════════════════════════════════════ */

void test_diameter(void) {
    printf("1. DIAMETER RATIO: Moon / Earth = 3/11\n");
    printf("   =====================================\n\n");

    double r = MOON_D / EARTH_D;
    printf("   Moon diameter:  %.1f km\n", MOON_D);
    printf("   Earth diameter: %.1f km\n\n", EARTH_D);

    printf("   ratio: %.6f\n", r);
    printf("   3/11:  %.6f\n", 3.0/11.0);
    printf("   error: %.4f%%\n\n", fabs(r - 3.0/11.0) / (3.0/11.0) * 100);

    printf("   3 = gates (AND, OR, NOT)\n");
    printf("   11 = Khufu base/layer (440/40)\n");
    printf("   11/7 = pi/2 (pyramid base:height ratio)\n\n");

    printf("   Moon/Earth = gates / pyramid_number.\n");
    printf("   0.02%% error. this is TIGHT.\n");
}

/* ═══════════════════════════════════════════════════════
 * 2. MASS RATIO: Moon/Earth = 1/81 = 1/3^4
 * ═══════════════════════════════════════════════════════ */

void test_mass(void) {
    printf("\n\n2. MASS RATIO: Moon / Earth = 1/3^4\n");
    printf("   ====================================\n\n");

    double r = MOON_M / EARTH_M;
    double expect = 1.0 / (GATES * GATES * GATES * GATES);
    int g4 = GATES*GATES*GATES*GATES;

    printf("   Moon mass:  %.3e kg\n", MOON_M);
    printf("   Earth mass: %.3e kg\n\n", EARTH_M);

    printf("   ratio:  %.6f = 1 / %.2f\n", r, 1.0/r);
    printf("   1/%d:   %.6f = 1 / %d.00\n", g4, expect, g4);
    printf("   error:  %.2f%%\n\n", fabs(1.0/r - g4) / g4 * 100);

    printf("   %d = %d^4 = gates^4\n", g4, GATES);
    printf("   the Moon is 1/gates^4 of Earth's mass.\n");
}

/* ═══════════════════════════════════════════════════════
 * 3. GRAVITY RATIO: Moon/Earth = 1/6 = 1/C(4,2)
 * ═══════════════════════════════════════════════════════ */

void test_gravity(void) {
    printf("\n\n3. GRAVITY RATIO: Moon / Earth = 1/C(4,2)\n");
    printf("   =========================================\n\n");

    double r = MOON_G / EARTH_G;
    int c42 = 6;  /* C(4,2) = 6 */

    printf("   Moon gravity:  %.3f m/s^2\n", MOON_G);
    printf("   Earth gravity: %.3f m/s^2\n\n", EARTH_G);

    printf("   ratio: %.6f\n", r);
    printf("   1/6:   %.6f\n", 1.0/c42);
    printf("   error: %.2f%%\n\n", fabs(r - 1.0/c42) / (1.0/c42) * 100);

    printf("   C(4,2) = 6 = 4D face count = cubit denominator (pi/6)\n");
    printf("   Moon gravity = Earth gravity / C(4,2).\n");
}

/* ═══════════════════════════════════════════════════════
 * 4. DISTANCE: Moon = 30 Earth diameters away
 * ═══════════════════════════════════════════════════════ */

void test_distance(void) {
    printf("\n\n4. DISTANCE: Moon = 30 Earth diameters\n");
    printf("   ======================================\n\n");

    double ed = MOON_DIST / EARTH_D;

    printf("   Moon distance:   %.0f km\n", MOON_DIST);
    printf("   Earth diameter:  %.0f km\n\n", EARTH_D);

    printf("   ratio: %.2f Earth diameters\n", ed);
    printf("   30:    expected\n");
    printf("   error: %.2f%%\n\n", fabs(ed - 30) / 30 * 100);

    printf("   30 = 5 x 6 = sqrt(fill) x C(4,2)\n");
    printf("   30 = Stonehenge sarsens\n");
    printf("   30 = icosahedron edges = dodecahedron edges\n");
}

/* ═══════════════════════════════════════════════════════
 * 5. APPARENT SIZE: Sun ≈ Moon ≈ pi/6 degrees
 * ═══════════════════════════════════════════════════════ */

void test_apparent(void) {
    printf("\n\n5. APPARENT SIZE: Sun ~ Moon ~ pi/6 degrees\n");
    printf("   ==========================================\n\n");

    double cubit = PI / 6.0;

    printf("   Sun apparent:  %.3f deg\n", SUN_ANG);
    printf("   Moon apparent: %.3f deg\n", MOON_ANG);
    printf("   pi/6:          %.4f\n\n", cubit);

    printf("   Sun/Moon:  %.4f (why eclipses work)\n", SUN_ANG/MOON_ANG);
    printf("   Sun/pi6:   error %.2f%%\n", fabs(SUN_ANG-cubit)/cubit*100);
    printf("   Moon/pi6:  error %.2f%%\n\n", fabs(MOON_ANG-cubit)/cubit*100);

    printf("   CAUTION: degrees are conventional (unit-dependent).\n");
    printf("   but pi/6 is also the cubit in meters.\n");
    printf("   same VALUE as both the building unit and the sky unit.\n");
}

/* ═══════════════════════════════════════════════════════
 * 6. ORBITAL PERIOD: sidereal month ≈ 3^3 days
 * ═══════════════════════════════════════════════════════ */

void test_period(void) {
    printf("\n\n6. ORBITAL PERIOD: sidereal ~ 3^3 days\n");
    printf("   ======================================\n\n");

    int g3 = GATES*GATES*GATES;

    printf("   sidereal month: %.3f days\n", SIDEREAL);
    printf("   3^3 = %d\n", g3);
    printf("   error: %.2f%%\n\n", fabs(SIDEREAL - g3) / g3 * 100);

    printf("   synodic month:  %.3f days\n", SYNODIC);
    printf("   synodic / sidereal = %.4f\n\n", SYNODIC / SIDEREAL);

    printf("   12 synodic months = %.1f days (lunar year)\n", 12 * SYNODIC);
    printf("   365.25 - 354.4 = %.1f day gap (solar - lunar)\n",
           365.25 - 12 * SYNODIC);
    printf("   the gap ~ 11 days. 11 = base/layer (440/40).\n");
}

/* ═══════════════════════════════════════════════════════
 * 7. ECLIPSE GEOMETRY: distance/diameter ratios
 * ═══════════════════════════════════════════════════════ */

void test_eclipse(void) {
    printf("\n\n7. ECLIPSE GEOMETRY\n");
    printf("   ==================\n\n");

    double moon_dd = MOON_DIST / MOON_D;
    double sun_dd = SUN_DIST / SUN_D;

    printf("   Moon distance/diameter: %.1f\n", moon_dd);
    printf("   Sun distance/diameter:  %.1f\n\n", sun_dd);

    printf("   ratio of ratios: %.4f (1.000 = perfect eclipse)\n\n",
           moon_dd / sun_dd);

    double avg = (moon_dd + sun_dd) / 2;
    printf("   average: %.1f\n", avg);
    printf("   108 = 4 x 27 = dims x gates^3\n");
    printf("   108 = 12 x 9 = (gates x dims) x gates^2\n");
    printf("   108 = interior angle of regular pentagon\n");
    printf("   error from 108: %.1f%%\n\n", fabs(avg-108)/108*100);

    printf("   the eclipse works because both distance/diameter\n");
    printf("   ratios are ~108 = dims x gates^3.\n");
}

/* ═══════════════════════════════════════════════════════
 * 8. ALL RATIOS TOGETHER — the pattern
 * ═══════════════════════════════════════════════════════ */

void test_all(void) {
    printf("\n\n8. ALL MOON RATIOS\n");
    printf("   =================\n\n");

    printf("   diameter  Moon/Earth  = 3/11      (error 0.02%%)\n");
    printf("   mass      Moon/Earth  = 1/81      (error 0.4%%)\n");
    printf("   gravity   Moon/Earth  = 1/6       (error 0.6%%)\n");
    printf("   distance  Moon/Earth  = 30 d      (error 0.6%%)\n");
    printf("   apparent  Sun/Moon    ~ 1.000     (error 2.9%%)\n");
    printf("   period    sidereal    ~ 27 d      (error 1.2%%)\n");
    printf("   eclipse   dist/diam   ~ 108       (error 1%%)\n\n");

    printf("   lattice numbers found:\n");
    printf("     3   = gates\n");
    printf("     6   = C(4,2)\n");
    printf("     11  = pyramid base/layer\n");
    printf("     27  = 3^3 = gates cubed\n");
    printf("     30  = C(4,2) x 5 = sarsens\n");
    printf("     81  = 3^4 = gates^4\n");
    printf("     108 = 4 x 27 = dims x gates^3\n\n");

    printf("   EVERYTHING is powers of 3 (gates) combined with\n");
    printf("   the other lattice constants (dims, C(4,2), 11).\n\n");

    printf("   3/11  = diameter    (3^1 / 11)\n");
    printf("   1/81  = mass        (3^0 / 3^4)\n");
    printf("   1/6   = gravity     (3^0 / C(4,2))\n");
    printf("   27    = period      (3^3)\n");
    printf("   108   = eclipse     (4 x 3^3)\n\n");

    printf("   the Moon is a 3-machine. GATES everywhere.\n");
    printf("   it doesn't encode the lattice. it runs on it.\n");
    printf("   the 'impossible coincidences' are powers of 3\n");
    printf("   because 3 = gates = functional completeness.\n");
}

int main(void) {
    printf("Moon — Lattice Analysis\n");
    printf("========================\n");
    printf("The 'impossible coincidences.' Are they lattice?\n\n\n");

    test_diameter();
    test_mass();
    test_gravity();
    test_distance();
    test_apparent();
    test_period();
    test_eclipse();
    test_all();

    return 0;
}
