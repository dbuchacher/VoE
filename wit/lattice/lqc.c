/*
 * lqc.c — The Lattice Quantum Computer
 *
 * Not a simulation. The thing itself.
 * Atoms, bonds, paths. Load a program. Run it. Read output.
 *
 * Build: cc -O2 -o lqc lqc.c -lm
 * Run:   ./lqc [number_to_factor]
 * E.g.:  ./lqc 143
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef unsigned long long u64;
typedef __uint128_t u128;

/* ═══════════════════════════════════════════════════════
 * ATOMS — values with coordinates
 * ═══════════════════════════════════════════════════════ */

typedef struct {
    u64 value;     /* the integer value (real coordinate) */
    int position;  /* position in the path (R coordinate) */
} Atom;

/* ═══════════════════════════════════════════════════════
 * BONDS — operations on atoms
 *
 * +C  = multiply (synthesis)
 * +R  = advance position (cycling)
 * +W  = test equality (regulation)
 * +P  = read/output (transport/measurement)
 * ═══════════════════════════════════════════════════════ */

typedef enum { P, C, R, W } Force;

typedef struct {
    Force force;
    int sign;      /* +1 or -1 */
    u64 param;     /* bond parameter */
} Bond;

/* ═══════════════════════════════════════════════════════
 * PATH — sequence of atoms (the quantum state)
 * ═══════════════════════════════════════════════════════ */

typedef struct {
    Atom *atoms;
    int len;
    int cap;
} Path;

void path_init(Path *p, int cap) {
    p->atoms = calloc(cap, sizeof(Atom));
    p->len = 0;
    p->cap = cap;
}

void path_push(Path *p, Atom a) {
    if (p->len >= p->cap) {
        p->cap *= 2;
        p->atoms = realloc(p->atoms, p->cap * sizeof(Atom));
    }
    p->atoms[p->len++] = a;
}

void path_free(Path *p) { free(p->atoms); }

/* ═══════════════════════════════════════════════════════
 * THE LATTICE QUANTUM COMPUTER
 * ═══════════════════════════════════════════════════════ */

typedef struct {
    u64 N;           /* modulus (the universe) */
    u64 base;        /* current base for modular exp */
    Path state;      /* current path (quantum state) */
    int period;      /* discovered period (R-coordinate) */
    u64 factor_p;    /* output */
    u64 factor_q;    /* output */
    int verbose;
} LQC;

u64 gcd(u64 a, u64 b) { while(b){u64 t=b;b=a%b;a=t;} return a; }

u64 modpow(u64 a, u64 e, u64 N) {
    u64 r=1; a%=N;
    while(e>0){if(e&1)r=(u128)r*a%N;e>>=1;if(e>0)a=(u128)a*a%N;}
    return r;
}

int is_prime(u64 n) {
    if (n < 2) return 0;
    if (n < 4) return 1;
    if (n % 2 == 0) return 0;
    for (u64 i = 3; i * i <= n; i += 2)
        if (n % i == 0) return 0;
    return 1;
}

/* ── CREATE ── */

LQC lqc_new(u64 N, int verbose) {
    LQC qc = {0};
    qc.N = N;
    qc.base = 2;
    qc.verbose = verbose;
    path_init(&qc.state, 1024);
    return qc;
}

/* ── BIND WALK: +C+R+W (the quantum evolution) ── */

int lqc_bind(LQC *qc) {
    /*
     * bind = +C+R+W = chain til identity
     *
     * +C: apply synthesis (multiply by base)
     * +R: advance position
     * +W: test (did we return to 1?)
     *
     * NO +P: we don't "read" intermediate values as output.
     * We observe the PATH, not individual atoms.
     */

    if (qc->verbose) {
        printf("  BIND (+C+R+W): chain multiply_%llu mod %llu til 1\n\n",
               qc->base, qc->N);
        printf("  step  bond     atom    value   W test\n");
        printf("  ────  ────     ────    ─────   ──────\n");
    }

    /* seed atom */
    Atom seed = {1, 0};
    path_push(&qc->state, seed);

    if (qc->verbose)
        printf("  %4d  seed     a[0]    %-6llu  —\n", 0, seed.value);

    u64 val = 1;
    for (int step = 1; step < (int)(qc->N * qc->N); step++) {
        /* +C: synthesis (multiply by base mod N) */
        val = (u128)val * qc->base % qc->N;

        /* +R: advance position */
        Atom a = {val, step};
        path_push(&qc->state, a);

        /* +W: test (value == seed?) */
        int found = (val == seed.value);

        if (qc->verbose && (step <= 20 || found))
            printf("  %4d  +C+R     a[%d]    %-6llu  %s\n",
                   step, step, val,
                   found ? "== 1 → PERIOD" : "≠ 1");

        if (found) {
            qc->period = step;
            if (qc->verbose)
                printf("\n  Period r = %d (path has %d atoms)\n", step, step);
            return step;
        }
    }
    return 0;
}

/* ── FORMULA: the measurement ── */

int lqc_measure(LQC *qc) {
    /*
     * +P: read the result
     *
     * The formula: p+q = N+1-k*L
     * where L = period (or multiple thereof)
     * Try k = 1, 2, 3, ... (k = gcd(p-1, q-1))
     */

    if (qc->verbose)
        printf("\n  MEASURE (+P): apply formula p+q = N+1-k*r\n\n");

    u64 N = qc->N;
    int r = qc->period;

    for (int k = 1; k <= 100; k++) {
        u64 kL = (u64)k * r;
        if (kL >= N) break;
        u64 s = N + 1 - kL;  /* candidate p+q */
        if (s * s < 4 * N) continue;

        u64 disc = s * s - 4 * N;
        u64 sq = (u64)sqrt((double)disc);
        for (u64 t = (sq > 1 ? sq-1 : 0); t <= sq+2; t++) {
            if (t * t == disc && (s + t) % 2 == 0) {
                qc->factor_q = (s + t) / 2;
                qc->factor_p = (s - t) / 2;
                if (qc->factor_p > 1 && qc->factor_q > 1 &&
                    qc->factor_p * qc->factor_q == N) {
                    if (qc->verbose) {
                        printf("  k=%d: p+q = %llu+1-%d×%d = %llu\n",
                               k, N, k, r, s);
                        printf("  discriminant = %llu² - 4×%llu = %llu = %llu²\n",
                               s, N, disc, t);
                        printf("  p = (%llu-%llu)/2 = %llu\n", s, t, qc->factor_p);
                        printf("  q = (%llu+%llu)/2 = %llu\n", s, t, qc->factor_q);
                    }
                    return 1;
                }
            }
        }
    }

    /* fallback: try gcd extraction from period (Shor's method) */
    if (r % 2 == 0) {
        u64 half = modpow(qc->base, r/2, N);
        u64 f1 = gcd(half + 1, N);
        u64 f2 = gcd(half > 0 ? half - 1 : N - 1, N);
        if (f1 > 1 && f1 < N) { qc->factor_p = f1; qc->factor_q = N/f1; return 1; }
        if (f2 > 1 && f2 < N) { qc->factor_p = f2; qc->factor_q = N/f2; return 1; }
    }
    return 0;
}

/* ── RUN: full pipeline ── */

int lqc_run(LQC *qc) {
    /* try multiple bases if needed */
    u64 bases[] = {2, 3, 5, 7, 11, 13, 17, 19, 23};
    int nb = 9;

    for (int b = 0; b < nb; b++) {
        u64 base = bases[b];
        u64 g = gcd(base, qc->N);
        if (g > 1 && g < qc->N) {
            /* trivial factor */
            qc->factor_p = g;
            qc->factor_q = qc->N / g;
            if (qc->verbose)
                printf("  Trivial: gcd(%llu, %llu) = %llu\n", base, qc->N, g);
            return 1;
        }
        if (g > 1) continue;

        qc->base = base;
        qc->state.len = 0;
        qc->period = 0;

        if (qc->verbose)
            printf("\n── Base %llu ──\n\n", base);

        int r = lqc_bind(qc);
        if (r == 0) continue;

        if (lqc_measure(qc)) return 1;

        if (qc->verbose)
            printf("  Base %llu: formula didn't extract. Trying next base.\n", base);
    }
    return 0;
}

/* ── DESTROY ── */

void lqc_free(LQC *qc) { path_free(&qc->state); }

/* ═══════════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════════ */

int main(int argc, char **argv) {
    printf("╔══════════════════════════════════════╗\n");
    printf("║   LATTICE QUANTUM COMPUTER (LQC)     ║\n");
    printf("║   atoms + bonds + paths = compute     ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    if (argc > 1) {
        /* factor a specific number */
        u64 N = strtoull(argv[1], NULL, 10);

        if (N < 6 || N % 2 == 0) {
            printf("  Give me an odd semiprime > 5.\n");
            return 1;
        }

        printf("  PROGRAM: factor(%llu)\n\n", N);

        LQC qc = lqc_new(N, 1);
        clock_t t0 = clock();
        int ok = lqc_run(&qc);
        clock_t t1 = clock();
        double us = (double)(t1-t0)/CLOCKS_PER_SEC*1e6;

        printf("\n  ══════════════════════════════════\n");
        if (ok) {
            printf("  RESULT: %llu = %llu × %llu\n", N, qc.factor_p, qc.factor_q);
            printf("  PATH:   %d atoms (bind walk)\n", qc.state.len);
            printf("  BONDS:  %d × (+C+R) + 1 × (+W) + 1 × (+P)\n", qc.period);
            printf("  TIME:   %.0f μs\n", us);
        } else {
            printf("  FAILED to factor %llu\n", N);
        }
        printf("  ══════════════════════════════════\n");
        lqc_free(&qc);
        return ok ? 0 : 1;
    }

    /* default: factor a series of semiprimes */
    printf("  No argument: running test suite.\n");
    printf("  Usage: ./lqc <number>\n\n");

    u64 tests[] = {15, 21, 35, 77, 91, 143, 221, 323, 437, 667, 899,
                   1147, 2021, 3127, 5041, 10403, 25009997, 100160063};
    int nt = 18;

    printf("  %-14s  %-10s  %-10s  %-8s  %-8s  %-10s\n",
           "N", "p", "q", "atoms", "bonds", "time");
    printf("  ──────────────────────────────────────────────────────\n");

    int cracked = 0;
    for (int t = 0; t < nt; t++) {
        u64 N = tests[t];
        LQC qc = lqc_new(N, 0);

        clock_t t0 = clock();
        int ok = lqc_run(&qc);
        clock_t t1 = clock();
        double us = (double)(t1-t0)/CLOCKS_PER_SEC*1e6;

        if (ok) {
            cracked++;
            printf("  %-14llu  %-10llu  %-10llu  %-8d  %-8d  %.0f μs\n",
                   N, qc.factor_p, qc.factor_q,
                   qc.state.len, qc.period + 2, us);
        } else {
            printf("  %-14llu  FAILED\n", N);
        }
        lqc_free(&qc);
    }

    printf("\n  Cracked: %d/%d\n", cracked, nt);

    printf("\n  ══════════════════════════════════\n");
    printf("  The LQC is:\n");
    printf("    atoms  = values in the path\n");
    printf("    +C     = multiply (synthesis)\n");
    printf("    +R     = advance (cycling)\n");
    printf("    +W     = test (regulation)\n");
    printf("    +P     = read output (measurement)\n");
    printf("    bind   = +C+R+W (quantum evolution)\n");
    printf("    hylo   = +P+C+R+W (classical readout)\n");
    printf("    path   = the quantum state\n");
    printf("    period = the R-coordinate of bind\n");
    printf("    formula = p+q = N+1-kL (measurement)\n");
    printf("  ══════════════════════════════════\n");

    return 0;
}
