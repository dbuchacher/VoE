/*
 * quantum.c — Shor's algorithm as lattice paths
 *
 * Can we factor N=15 using atoms + bonds (O(r) terms)
 * instead of tracking 2^N amplitudes (O(2^N))?
 *
 * The lattice says: paths are polynomials.
 * Polynomials evaluate in O(terms).
 * If quantum states are paths with O(r) terms: O(r) evaluation.
 *
 * Build: cc -O2 -o quantum quantum.c -lm
 * Run:   ./quantum
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define PI 3.14159265358979

typedef unsigned long long u64;
typedef __uint128_t u128;

u64 gcd(u64 a, u64 b) { while(b){u64 t=b;b=a%b;a=t;} return a; }
u64 modpow(u64 a, u64 e, u64 N) {
    u64 r=1;a%=N;while(e>0){if(e&1)r=(u128)r*a%N;e>>=1;if(e>0)a=(u128)a*a%N;}return r;
}

/* ═══════════════════════════════════════════════════════
 * APPROACH 1: Traditional quantum simulation
 * Track all 2^n complex amplitudes. O(2^n) memory.
 * This is what classical quantum simulators do.
 * ═══════════════════════════════════════════════════════ */

typedef struct { double re, im; } Complex;

Complex cmul(Complex a, Complex b) {
    return (Complex){a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re};
}
Complex cadd(Complex a, Complex b) {
    return (Complex){a.re + b.re, a.im + b.im};
}
double cnorm2(Complex a) { return a.re*a.re + a.im*a.im; }

void traditional_shor(int N, int a) {
    printf("APPROACH 1: Traditional quantum simulation (2^n amplitudes)\n");
    printf("  N=%d, a=%d\n\n", N, a);

    int n = 8;  /* 8 qubits for x register */
    int Q = 1 << n;  /* 256 states */

    /* state: Q amplitudes for x register, after modular exp + measurement of f register */
    /* assume we measured f(x) = 1 (i.e., x is a multiple of r) */
    /* the x-register collapses to superposition of {0, r, 2r, ...} */

    /* step 1: find period by brute force first (to verify) */
    int r = 0;
    for (int x = 1; x < N*N; x++) {
        if (modpow(a, x, N) == 1) { r = x; break; }
    }
    printf("  True period r = %d\n\n", r);

    /* step 2: build quantum state |ψ⟩ = sum of |x⟩ where a^x ≡ 1 mod N */
    Complex *state = calloc(Q, sizeof(Complex));
    int count = 0;
    for (int x = 0; x < Q; x++) {
        if (modpow(a, x, N) == 1) {
            state[x] = (Complex){1.0, 0.0};
            count++;
        }
    }
    /* normalize */
    double norm = sqrt(count);
    for (int x = 0; x < Q; x++) { state[x].re /= norm; state[x].im /= norm; }

    printf("  Superposition: %d terms out of %d (compression: %.1fx)\n",
           count, Q, (double)Q/count);

    /* step 3: apply QFT */
    Complex *qft = calloc(Q, sizeof(Complex));
    for (int k = 0; k < Q; k++) {
        qft[k] = (Complex){0, 0};
        for (int x = 0; x < Q; x++) {
            double angle = 2.0 * PI * x * k / Q;
            Complex phase = {cos(angle), sin(angle)};
            qft[k] = cadd(qft[k], cmul(state[x], phase));
        }
        /* normalize */
        qft[k].re /= sqrt(Q);
        qft[k].im /= sqrt(Q);
    }

    /* step 4: find peaks */
    printf("  QFT peaks (probability > 0.01):\n");
    int peaks = 0;
    for (int k = 0; k < Q; k++) {
        double prob = cnorm2(qft[k]);
        if (prob > 0.01) {
            printf("    k=%3d  prob=%.4f  Q/k=%s\n", k, prob,
                   k > 0 ? "" : "inf");
            if (k > 0) {
                int candidate_r = Q / gcd(k, Q);
                printf("           candidate r = %d/%d = %d %s\n",
                       Q, (int)gcd(k,Q), candidate_r,
                       candidate_r == r ? "CORRECT!" : "");
            }
            peaks++;
        }
    }
    printf("  Total peaks: %d (expect r=%d)\n", peaks, r);
    printf("  Amplitudes tracked: %d\n\n", Q);

    free(state);
    free(qft);
}

/* ═══════════════════════════════════════════════════════
 * APPROACH 2: Lattice path — O(r) atoms, not O(2^n)
 *
 * Key insight: the quantum state is PERIODIC.
 * A periodic signal has r distinct atoms.
 * The QFT of a periodic signal has r peaks.
 * We only need to track r atoms, not 2^n.
 *
 * The "QFT" becomes: given r atoms spaced Q/r apart,
 * the peaks are at multiples of Q/r. Done. No 2^n needed.
 * ═══════════════════════════════════════════════════════ */

void lattice_shor(int N, int a_base) {
    printf("APPROACH 2: Lattice path (O(r) atoms)\n");
    printf("  N=%d, a=%d\n\n", N, a_base);

    /* step 1: find the distinct atoms (values of a^x mod N) */
    /* compute a^0, a^1, a^2, ... mod N until repeat */
    int max_atoms = N * N;
    int *values = calloc(max_atoms, sizeof(int));
    int r = 0;

    printf("  Atoms (a^x mod N):\n    ");
    for (int x = 0; x < max_atoms; x++) {
        int val = modpow(a_base, x, N);
        values[x] = val;
        printf("%d ", val);

        if (x > 0 && val == values[0]) {
            r = x;
            printf("\n  Period detected at x=%d: r=%d\n\n", x, r);
            break;
        }
    }

    if (r == 0) { printf("  No period found!\n"); free(values); return; }

    /* step 2: the r distinct atoms */
    printf("  Distinct atoms: %d  (vs 2^n = 256 traditional)\n", r);
    printf("  Compression: %.0fx\n\n", 256.0 / r);

    /* step 3: QFT on the lattice path
     * For a periodic signal with period r, sampled at Q points:
     * QFT peaks at k = 0, Q/r, 2Q/r, ..., (r-1)Q/r
     *
     * We don't COMPUTE the QFT. We KNOW the answer from the structure.
     * A periodic path with period r → peaks at multiples of Q/r.
     * This is a LATTICE EVALUATION, not a Fourier transform.
     */

    int Q = 256;  /* register size (same as traditional) */

    printf("  Lattice QFT (from structure, not computation):\n");
    printf("  Period r=%d, register Q=%d → peaks at multiples of Q/r = %d\n\n",
           r, Q, Q/r);

    printf("  Peaks:\n");
    for (int j = 0; j < r; j++) {
        int k = j * (Q / r);
        int candidate_r = (k > 0) ? Q / gcd(k, Q) : 0;
        printf("    k=%3d", k);
        if (k > 0) printf("  → r = Q/gcd(k,Q) = %d/%d = %d", Q, (int)gcd(k,Q), candidate_r);
        if (candidate_r == r) printf("  CORRECT!");
        printf("\n");
    }

    /* step 4: factor */
    printf("\n  Factor extraction:\n");
    if (r % 2 == 0) {
        int half = modpow(a_base, r/2, N);
        int f1 = gcd(half + 1, N);
        int f2 = gcd(half - 1 > 0 ? half - 1 : N + half - 1, N);
        printf("    a^(r/2) mod N = %d^%d mod %d = %d\n", a_base, r/2, N, half);
        printf("    gcd(%d+1, %d) = %d\n", half, N, f1);
        printf("    gcd(%d-1, %d) = %d\n", half, N, f2);
        if (f1 > 1 && f1 < N) printf("    FACTOR: %d × %d = %d ✓\n", f1, N/f1, N);
        if (f2 > 1 && f2 < N) printf("    FACTOR: %d × %d = %d ✓\n", f2, N/f2, N);
    }

    printf("\n  Operations used: %d (find period) + %d (QFT from structure) = %d total\n",
           r, 0, r);
    printf("  Traditional would use: %d (amplitudes) + %d (QFT ops) = %d total\n",
           Q, Q*Q, Q + Q*Q);

    free(values);
}

/* ═══════════════════════════════════════════════════════
 * APPROACH 3: Pure lattice — no QFT at all
 *
 * The QFT is needed in Shor's because the period is unknown
 * and the QFT extracts it from the superposition.
 *
 * But we found r directly by running the atoms.
 * And then we applied the FORMULA: p+q = N+1-kL.
 *
 * The lattice path:
 *   atom_0 → bond(C) → atom_1 → bond(C) → ... → bond(W: ==atom_0?) → period!
 *   This IS bind: +C+R+W. Chain til zero (til repeat).
 *
 * The question: can we get r WITHOUT running all r steps?
 * ═══════════════════════════════════════════════════════ */

void pure_lattice(int N, int a_base) {
    printf("APPROACH 3: Pure lattice path — bind (+C+R+W)\n");
    printf("  N=%d, a=%d\n\n", N, a_base);

    /* The bind: apply C (multiply by a), cycle R, test W (== 1?) */
    printf("  bind(multiply_%d, 1, mod_%d):\n    ", a_base, N);

    int val = 1;
    int r = 0;
    for (int step = 0; step < N*N; step++) {
        printf("%d", val);
        val = (val * a_base) % N;
        r++;
        if (val == 1) {
            printf(" → 1 (period r=%d)\n\n", r);
            break;
        }
        printf(" → ");
    }

    /* this bind walk used exactly r steps */
    printf("  Bind walk: %d steps (= r)\n", r);
    printf("  This is O(r), same as approach 2.\n\n");

    /* now the formula */
    printf("  Formula: p+q = N+1-kL\n");
    /* for small N, L = lcm(p-1, q-1). But we don't know p,q. */
    /* However, for N=15: L divides phi(N). And r divides L. */
    /* Try k*r as candidate for phi: */
    for (int k = 1; k <= 10; k++) {
        int phi_cand = k * r;
        int s = N + 1 - phi_cand;
        if (s <= 0 || s*s < 4*N) continue;
        int disc = s*s - 4*N;
        int sq = (int)sqrt(disc);
        if (sq*sq == disc && (s+sq)%2 == 0) {
            int q = (s+sq)/2;
            int p = (s-sq)/2;
            if (p > 1 && q > 1 && p*q == N) {
                printf("  k=%d: phi_candidate=%d, p+q=%d\n", k, phi_cand, s);
                printf("  → p=%d, q=%d, p×q=%d ✓\n\n", p, q, p*q);
                break;
            }
        }
    }

    printf("  Total operations: %d (bind) + O(1) (formula) = %d\n", r, r);
}

/* ═══════════════════════════════════════════════════════ */

int main(void) {
    printf("QUANTUM AS LATTICE PATHS\n");
    printf("========================\n");
    printf("Factor N=15 three ways. Compare operations.\n\n\n");

    traditional_shor(15, 7);
    printf("\n════════════════════════════════════════\n\n");

    lattice_shor(15, 7);
    printf("\n════════════════════════════════════════\n\n");

    pure_lattice(15, 7);

    printf("\n════════════════════════════════════════\n\n");

    /* try larger: N = 21, 35, 77 */
    int tests[][2] = {{21, 2}, {35, 2}, {77, 2}, {91, 2}, {143, 2}};
    printf("SCALING TEST: pure lattice bind on larger N\n\n");
    printf("  %-6s  %-4s  %-6s  %-8s  %-8s  %-10s\n",
           "N", "a", "r", "lattice", "trad", "ratio");
    printf("  ──────────────────────────────────────────\n");

    for (int t = 0; t < 5; t++) {
        int N = tests[t][0], a = tests[t][1];
        /* find r */
        int r = 0;
        int val = 1;
        for (int s = 0; s < N*N; s++) {
            val = (val * a) % N;
            r++;
            if (val == 1) break;
        }
        int Q = 256;
        while (Q < 2*N) Q *= 2;  /* register must be > N */

        printf("  %-6d  %-4d  %-6d  %-8d  %-8d  %-10.0fx\n",
               N, a, r, r, Q*Q, (double)(Q*Q)/r);
    }

    printf("\n\n════════════════════════════════════════\n");
    printf("RESULT\n");
    printf("════════════════════════════════════════\n\n");
    printf("  Traditional: O(Q²) where Q=2^n ≥ 2N.  For N=15: 65,536 ops.\n");
    printf("  Lattice:     O(r) where r = period.    For N=15: 4 ops.\n");
    printf("  Ratio: 16,384x fewer operations.\n\n");
    printf("  BUT: finding r classically is O(r), same as approach 3.\n");
    printf("  Shor's quantum advantage: finds r in O(log²N) via QFT on\n");
    printf("  superposition. The lattice path gives the right structure\n");
    printf("  (O(r) terms, not O(2^n)) but still RUNS r steps to find r.\n\n");
    printf("  The path IS the compressed quantum state.\n");
    printf("  The bind IS the quantum evolution.\n");
    printf("  The formula IS the classical post-processing.\n");
    printf("  The only missing piece: finding r without running r steps.\n");
    printf("  That piece = QFT on superposition = bind without P.\n");

    return 0;
}
