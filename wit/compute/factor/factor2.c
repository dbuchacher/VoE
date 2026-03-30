/*
 * factor2.c — push deeper on the N mod r = p finding
 *
 * From factor.c: N mod r = p for p=3 cases.
 * WHY? Because N mod (q-1) = p always (when p < q-1).
 * Proof: pq mod (q-1) = p*(q mod (q-1)) = p*1 = p.
 *
 * And: phi(N) = (p-1)(q-1) = N - (p+q) + 1
 * So:  p+q = N + 1 - phi(N)
 * And: phi(N) = lcm(p-1,q-1) * gcd(p-1,q-1)
 *
 * If gcd(p-1,q-1) = 2 (true ~60% of the time for random primes):
 *   phi(N) = 2 * lcm(p-1,q-1) = 2 * L
 *   p + q = N + 1 - 2L
 *   then solve: x^2 - (p+q)x + N = 0
 *
 * THIS IS THE FORMULA. Let's verify it.
 *
 * Build: cc -O2 -o factor2 factor2.c -lm
 * Run:   ./factor2
 */

#include <stdio.h>
#include <math.h>

long gcd(long a, long b) {
    while (b) { long t = b; b = a % b; a = t; }
    return a;
}

long lcm_fn(long a, long b) { return a / gcd(a, b) * b; }

long period(long a, long N) {
    if (gcd(a, N) != 1) return -1;
    long val = a % N;
    for (long r = 1; r < N; r++) {
        if (val == 1) return r;
        val = (val * a) % N;
    }
    return -1;
}

/* try to factor using the formula: p+q = N+1 - k*L */
int try_formula(long N, long L, long *out_p, long *out_q) {
    for (int k = 1; k <= 20; k++) {
        long s = N + 1 - (long)k * L;  /* candidate p+q */
        if (s <= 0) break;
        if (s * s < 4 * N) continue;   /* discriminant negative */
        long disc = s * s - 4 * N;
        long sq = (long)sqrt((double)disc);
        /* check sq and sq+1 (floating point) */
        for (long t = sq - 1; t <= sq + 1; t++) {
            if (t >= 0 && t * t == disc) {
                if ((s + t) % 2 == 0) {
                    *out_p = (s - t) / 2;
                    *out_q = (s + t) / 2;
                    if (*out_p > 1 && *out_q > 1 && *out_p * *out_q == N)
                        return k;
                }
            }
        }
    }
    return 0;
}

int main(void) {
    printf("Factor Formula: p+q = N+1-2L where L = lcm(p-1,q-1)\n");
    printf("=====================================================\n\n");

    int primes[] = {3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97};
    int np = 24;

    /* === TEST 1: verify p+q = N+1-g*L with known factorization === */
    printf("TEST 1: verify the formula (using known p,q)\n");
    printf("  N=p*q      p    q   p+q   L=lcm   g=gcd  N+1-2L  match?  k\n");
    printf("  ───────────────────────────────────────────────────────────\n");

    int match2 = 0, total = 0;
    for (int i = 0; i < 15; i++) {
        for (int j = i + 1; j < 15; j++) {
            long p = primes[i], q = primes[j];
            long N = p * q;
            long L = lcm_fn(p - 1, q - 1);
            long g = gcd(p - 1, q - 1);
            long s_formula = N + 1 - 2 * L;
            long s_actual = p + q;
            total++;
            int m = (s_formula == s_actual) ? 1 : 0;
            if (m) match2++;

            if (i < 6 || m == 0) {  /* print first few + all mismatches */
                printf("  %5ld  %4ld %4ld  %4ld  %5ld    %ld   %5ld    %s    %ld\n",
                       N, p, q, s_actual, L, g, s_formula,
                       m ? "YES" : " no",
                       (long)g);
            }
        }
    }
    printf("  ... (showing subset)\n");
    printf("  k=2 match rate: %d/%d (%.0f%%)\n\n", match2, total, 100.0*match2/total);

    /* === TEST 2: factor using COMPUTED periods (not known p,q) === */
    printf("TEST 2: factor using computed periods (simulating Shor)\n");
    printf("  N        bases_used  L_computed  k   p    q    correct?\n");
    printf("  ─────────────────────────────────────────────────────\n");

    int success = 0;
    total = 0;
    int bases[] = {2, 3, 5, 7, 11, 13};
    int nb = 6;

    for (int i = 0; i < 15; i++) {
        for (int j = i + 1; j < 15; j++) {
            long p = primes[i], q = primes[j];
            long N = p * q;
            total++;

            /* compute periods for multiple bases, take LCM */
            long L = 1;
            int used = 0;
            for (int b = 0; b < nb; b++) {
                long r = period(bases[b], N);
                if (r > 0) {
                    L = lcm_fn(L, r);
                    used++;
                }
            }

            /* try formula */
            long fp = 0, fq = 0;
            int k = try_formula(N, L, &fp, &fq);

            if (k > 0 && fp * fq == N) {
                success++;
                printf("  %5ld    %d bases    %5ld      k=%d  %3ld  %3ld   YES\n",
                       N, used, L, k, fp, fq);
            } else {
                printf("  %5ld    %d bases    %5ld      ---  FAILED (true L=%ld)\n",
                       N, used, L, lcm_fn(p-1, q-1));
            }
        }
    }
    printf("\n  Success rate: %d/%d (%.0f%%)\n\n", success, total, 100.0*success/total);

    /* === TEST 3: the lattice formula === */
    printf("TEST 3: THE FORMULA\n");
    printf("====================\n\n");
    printf("  Given: N = p*q (semiprime to factor)\n");
    printf("  Find:  L = lcm of periods from multiple bases a^x mod N\n");
    printf("  Then:  for k = 1, 2, 3, ...:\n");
    printf("           s = N + 1 - k*L\n");
    printf("           d = s*s - 4*N\n");
    printf("           if d is a perfect square:\n");
    printf("             p = (s - sqrt(d)) / 2\n");
    printf("             q = (s + sqrt(d)) / 2\n");
    printf("             DONE.\n\n");

    printf("  In lattice terms:\n");
    printf("    N   = atom in product form (T^a * D^b)\n");
    printf("    L   = R-coordinate (cycling period)\n");
    printf("    p+q = atom in sum form (superposition = -P-W = add bond)\n");
    printf("    The formula converts product (atoms) to sum (bonds)\n");
    printf("    through cycling (R-force). ATOM-BOND BRIDGE.\n\n");

    printf("  The ONLY hard part: computing L classically is O(L).\n");
    printf("  Shor's computes L in O(log^2 N) using complex exponents.\n");
    printf("  Question: can the lattice compute L faster than O(L)?\n\n");

    /* === TEST 4: what IS L as a function of N? === */
    printf("TEST 4: L vs N — is there a pattern?\n");
    printf("  N        L      L/N     L/phi   N/L   (N+1-2L)/sqrt(4N)\n");
    printf("  ──────────────────────────────────────────────────────\n");

    for (int i = 0; i < 12; i++) {
        for (int j = i + 1; j < 12; j++) {
            long p = primes[i], q = primes[j];
            long N = p * q;
            long L = lcm_fn(p - 1, q - 1);
            long phi = (p - 1) * (q - 1);
            long s = p + q;
            printf("  %5ld  %5ld   %.4f  %.4f  %5.2f   %.4f\n",
                   N, L, (double)L/N, (double)L/phi, (double)N/L,
                   (double)s / (2*sqrt(N)));
        }
    }

    printf("\n  Note: (p+q)/(2*sqrt(N)) measures how 'balanced' the factors are.\n");
    printf("  For p=q: this = 1.000. For p<<q: this >> 1.\n");
    printf("  L/N doesn't converge. L/phi = 1/gcd(p-1,q-1) — varies.\n");

    return 0;
}
