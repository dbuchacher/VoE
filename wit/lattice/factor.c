/*
 * factor.c — can the lattice find factoring coordinates?
 *
 * Shor's proved the coordinates exist (the period).
 * Compute First says: if coordinates exist, find them.
 * Feed semiprimes through the equation. See what falls out.
 *
 * Build: cc -O2 -o factor factor.c -lm
 * Run:   ./factor
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

long gcd(long a, long b) {
    while (b) { long t = b; b = a % b; a = t; }
    return a;
}

long lcm(long a, long b) { return a / gcd(a, b) * b; }

/* multiplicative order of a mod N (period of a^x mod N) */
long period(long a, long N) {
    if (gcd(a, N) != 1) return -1;
    long val = a % N;
    for (long r = 1; r < N; r++) {
        if (val == 1) return r;
        val = (val * a) % N;
    }
    return -1;
}

/* modular exponentiation: a^e mod N */
long modpow(long a, long e, long N) {
    long result = 1;
    a %= N;
    while (e > 0) {
        if (e & 1) result = (result * a) % N;
        e >>= 1;
        a = (a * a) % N;
    }
    return result;
}

/* try to extract factor from period */
long extract_factor(long a, long r, long N) {
    if (r % 2 != 0) return 0;
    long half = modpow(a, r / 2, N);
    long f1 = gcd(half + 1, N);
    long f2 = gcd(half - 1 + N, N);  /* avoid negative */
    if (f1 > 1 && f1 < N) return f1;
    if (f2 > 1 && f2 < N) return f2;
    return 0;
}

int main(void) {
    printf("Factoring via Lattice — Exploratory\n");
    printf("====================================\n\n");

    int primes[] = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73};
    int np = 20;

    /* === TABLE 1: periods and known relationships === */
    printf("TABLE 1: periods for base a=2\n");
    printf("  N=p*q      p    q   phi(N)   r    lcm(p-1,q-1)  r==lcm  r|phi  factor\n");
    printf("  ─────────────────────────────────────────────────────────────────────\n");

    int hits = 0, total = 0;
    for (int i = 0; i < 12; i++) {
        for (int j = i + 1; j < 12; j++) {
            long p = primes[i], q = primes[j];
            long N = p * q;
            long phi = (p - 1) * (q - 1);
            long r = period(2, N);
            long l = lcm(p - 1, q - 1);
            long f = extract_factor(2, r, N);
            total++;
            if (f > 0) hits++;

            printf("  %5ld  %4ld %4ld  %5ld  %4ld   %5ld         %s     %s    %ld\n",
                   N, p, q, phi, r, l,
                   r == l ? "yes" : " no",
                   phi % r == 0 ? "yes" : " no",
                   f);
        }
    }
    printf("\n  Factor extraction rate: %d/%d (%.0f%%)\n\n", hits, total, 100.0*hits/total);

    /* === TABLE 2: does r/N or r/sqrt(N) converge? === */
    printf("TABLE 2: period ratios — looking for convergence\n");
    printf("  N        r     r/N      r/sqrtN  r/cbrtN  r/ln(N)  phi/N\n");
    printf("  ────────────────────────────────────────────────────────\n");

    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            long p = primes[i], q = primes[j];
            long N = p * q;
            long phi = (p - 1) * (q - 1);
            long r = period(2, N);
            printf("  %5ld  %4ld  %.4f  %7.3f  %7.3f  %7.3f  %.4f\n",
                   N, r, (double)r / N, r / sqrt(N), r / cbrt(N),
                   r / log(N), (double)phi / N);
        }
    }

    /* === TABLE 3: what does r look like in lattice terms? === */
    printf("\nTABLE 3: period factorization — lattice numbers?\n");
    printf("  N=p*q      r    r factors             lattice?\n");
    printf("  ────────────────────────────────────────────────\n");

    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            long p = primes[i], q = primes[j];
            long N = p * q;
            long r = period(2, N);

            /* factor r */
            printf("  %5ld  %4ld    ", N, r);
            long rr = r;
            int first = 1;
            for (long f = 2; f * f <= rr; f++) {
                while (rr % f == 0) {
                    if (!first) printf("*");
                    printf("%ld", f);
                    rr /= f;
                    first = 0;
                }
            }
            if (rr > 1) {
                if (!first) printf("*");
                printf("%ld", rr);
            }
            if (r == 1) printf("1");

            /* check lattice numbers */
            printf("  ");
            if (r % 3 == 0) printf(" 3|r");
            if (r % 4 == 0) printf(" 4|r");
            if (r % 6 == 0) printf(" 6|r");
            if (r % 7 == 0) printf(" 7|r");
            if (r % 8 == 0) printf(" 8|r");
            if (r % 12 == 0) printf(" 12|r");
            if (r % 16 == 0) printf(" 16|r");
            printf("\n");
        }
    }

    /* === TABLE 4: multiple bases — does the GCD of periods help? === */
    printf("\nTABLE 4: GCD of periods from multiple bases\n");
    printf("  N=p*q      r(2)  r(3)  r(5)  gcd(r2,r3)  gcd(all)  phi  match?\n");
    printf("  ────────────────────────────────────────────────────────────────\n");

    for (int i = 0; i < 8; i++) {
        for (int j = i + 1; j < 8; j++) {
            long p = primes[i], q = primes[j];
            long N = p * q;
            long phi = (p - 1) * (q - 1);
            long r2 = period(2, N);
            long r3 = period(3, N);
            long r5 = period(5, N);
            if (r3 < 0) r3 = 0;
            if (r5 < 0) r5 = 0;
            long g23 = (r3 > 0) ? gcd(r2, r3) : r2;
            long g = g23;
            if (r5 > 0) g = gcd(g, r5);

            /* try LCM instead */
            long l23 = (r3 > 0) ? lcm(r2, r3) : r2;
            long l = l23;
            if (r5 > 0) l = lcm(l, r5);

            printf("  %5ld   %4ld  %4ld  %4ld    %5ld     %5ld   %5ld  %s\n",
                   N, r2, r3, r5, g23, g, phi,
                   l == phi ? "lcm=phi!" : (g23 == phi ? "gcd=phi!" : ""));
        }
    }

    /* === TABLE 5: the key test — can we predict r from N alone? === */
    printf("\nTABLE 5: r vs expressions of N (looking for the formula)\n");
    printf("  N        r     N-r    N%%r   sqrt(N-r)  is_square?  N/r\n");
    printf("  ────────────────────────────────────────────────────────\n");

    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            long p = primes[i], q = primes[j];
            long N = p * q;
            long r = period(2, N);
            long diff = N - r;
            long sq = (long)sqrt(diff);
            int is_sq = (sq * sq == diff) || ((sq+1)*(sq+1) == diff);

            printf("  %5ld  %4ld  %4ld   %3ld    %6.2f      %s     %.2f\n",
                   N, r, diff, N % r, sqrt((double)diff),
                   is_sq ? "YES" : " no", (double)N / r);
        }
    }

    printf("\n\n====================================\n");
    printf("LOOKING FOR THE FORMULA\n");
    printf("====================================\n\n");
    printf("If Shor's finds r with complex exponents,\n");
    printf("and Compute First says coordinates exist,\n");
    printf("the formula should be here. In the data.\n");
    printf("What relates r to N without knowing p,q?\n");

    return 0;
}
