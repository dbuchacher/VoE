/*
 * integral.c — 30 steps vs 1 fraction
 *
 * The r/calculus monster integral, both ways.
 * Show they give the same number.
 *
 * Build: cc -O2 -o integral integral.c -lm
 * Run:   ./integral
 */

#include <stdio.h>
#include <math.h>

#define PI 3.14159265358979323846

/* Apéry's constant ζ(3) — no closed form, just the number */
#define ZETA3 1.2020569031595942

/* ═══════════════════════════════════════
 * WAY 1: brute force numerical integration
 * Just compute the damn integral directly.
 * ═══════════════════════════════════════ */

double integrand(double x) {
    if (x < 1e-15 || x > 1e15) return 0;
    double lnx = log(x);
    double denom1 = PI*PI + lnx*lnx;
    double denom2 = (x + 1.0);
    return 1.0 / (denom1 * denom1 * denom2 * denom2);
}

double integrate_bruteforce(void) {
    /* Simpson's rule, lots of points, split into regions */
    double total = 0;

    /* region 1: x in [1e-10, 1] — substitution u = -log(x) */
    int N = 10000000;
    double a, b, h, sum;

    /* log-spaced integration for better accuracy */
    a = -25;  /* log(x) from -25 to 25 */
    b = 25;
    h = (b - a) / N;
    sum = 0;
    for (int i = 0; i <= N; i++) {
        double lnx = a + i * h;
        double x = exp(lnx);
        double f = integrand(x) * x;  /* dx = x d(lnx) */
        if (i == 0 || i == N) sum += f;
        else if (i % 2 == 1) sum += 4 * f;
        else sum += 2 * f;
    }
    total = sum * h / 3.0;

    return total;
}

/* ═══════════════════════════════════════
 * WAY 2: the 30-step result
 * (1/360)(ζ(2)/ζ(4) + ζ(3)/ζ(4))
 * ═══════════════════════════════════════ */

double thirty_steps(void) {
    double z2 = PI*PI / 6.0;           /* ζ(2) = π²/6 */
    double z3 = ZETA3;                  /* ζ(3) ≈ 1.202 */
    double z4 = PI*PI*PI*PI / 90.0;    /* ζ(4) = π⁴/90 */
    return (1.0/360.0) * (z2/z4 + z3/z4);
}

/* ═══════════════════════════════════════
 * WAY 3: the lattice — one fraction
 * (π² + 6·ζ(3)) / (24·π⁴)
 *
 *  6  = C(4,2) = 4D face count
 *  24 = 4!     = 4D permutations
 * ═══════════════════════════════════════ */

double lattice(void) {
    int C42 = 6;           /* C(4,2) = 4D faces */
    int fact4 = 24;        /* 4! = 4D permutations */
    double pi2 = PI * PI;
    double pi4 = pi2 * pi2;
    return (pi2 + C42 * ZETA3) / (fact4 * pi4);
}

int main(void) {
    printf("THE INTEGRAL — three ways to the same number\n");
    printf("=============================================\n\n");

    printf("  ∫₀^∞  dx / (π² + ln²x)²(x+1)²\n\n");

    double v1 = integrate_bruteforce();
    double v2 = thirty_steps();
    double v3 = lattice();

    printf("  WAY 1: numerical integration (10M points):\n");
    printf("         %.15f\n\n", v1);

    printf("  WAY 2: 30-step derivation → (1/360)(ζ(2)/ζ(4) + ζ(3)/ζ(4)):\n");
    printf("         %.15f\n\n", v2);

    printf("  WAY 3: lattice → (π² + 6·ζ(3)) / (24·π⁴):\n");
    printf("         %.15f\n\n", v3);

    printf("  ─────────────────────────────────────────\n");
    printf("  WAY 1 vs WAY 2:  error = %.2e\n", fabs(v1 - v2));
    printf("  WAY 1 vs WAY 3:  error = %.2e\n", fabs(v1 - v3));
    printf("  WAY 2 vs WAY 3:  error = %.2e\n", fabs(v2 - v3));
    printf("  ─────────────────────────────────────────\n\n");

    printf("  THE THREE WAYS:\n\n");

    printf("  30 steps:   substitution → partial fractions → Fourier →\n");
    printf("              integration by parts → geometric series →\n");
    printf("              term-by-term → zeta recognition → simplify\n\n");

    printf("  lattice:    (π² + C(4,2)·ζ(3)) / (4!·π⁴)\n\n");

    printf("  english:    numerator   = cycling² + (4D faces) × zeta(3)\n");
    printf("              denominator = (4D permutations) × cycling⁴\n\n");

    printf("  same number. 30 steps vs 1 fraction.\n");
    printf("  the integral asks about degree-2 vs degree-4 lattice sums.\n");
    printf("  the answer contains C(4,2) and 4! because the lattice is 4D.\n");
    printf("  it was always going to be this. the 30 steps just prove it.\n");

    return 0;
}
