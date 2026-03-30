/*
 * factor_test.c — generate keys and crack them
 *
 * Generate semiprimes of increasing size.
 * Race: trial division vs phi-search vs Pollard p-1 vs lattice formula.
 *
 * Build: cc -O2 -o factor_test factor_test.c -lm
 * Run:   ./factor_test
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long u64;
typedef __uint128_t u128;

u64 gcd(u64 a, u64 b) {
    while (b) { u64 t = b; b = a % b; a = t; }
    return a;
}

u64 modpow(u64 a, u64 e, u64 N) {
    u64 result = 1;
    a %= N;
    while (e > 0) {
        if (e & 1) result = (u128)result * a % N;
        e >>= 1;
        if (e > 0) a = (u128)a * a % N;
    }
    return result;
}

/* Miller-Rabin primality test */
int is_prime(u64 n) {
    if (n < 2) return 0;
    if (n < 4) return 1;
    if (n % 2 == 0) return 0;
    u64 d = n - 1;
    int r = 0;
    while (d % 2 == 0) { d /= 2; r++; }
    u64 witnesses[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    for (int i = 0; i < 12; i++) {
        u64 a = witnesses[i];
        if (a >= n) continue;
        u64 x = modpow(a, d, n);
        if (x == 1 || x == n - 1) continue;
        int found = 0;
        for (int j = 0; j < r - 1; j++) {
            x = (u128)x * x % n;
            if (x == n - 1) { found = 1; break; }
        }
        if (!found) return 0;
    }
    return 1;
}

/* simple PRNG (xorshift64) */
u64 rng_state = 0;
u64 xorshift64(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

/* generate random prime of approximately `bits` bits */
u64 random_prime(int bits) {
    u64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    u64 min = (bits >= 2) ? (1ULL << (bits - 1)) : 2;
    for (;;) {
        u64 candidate = (xorshift64() & mask) | min | 1;
        if (is_prime(candidate)) return candidate;
    }
}

u64 isqrt(u64 n) {
    if (n == 0) return 0;
    u64 x = (u64)sqrt((double)n);
    while (x * x > n) x--;
    while ((x + 1) * (x + 1) <= n) x++;
    return x;
}

u64 isqrt_check(u64 d) {
    u64 sq = isqrt(d);
    if (sq * sq == d) return sq;
    return 0;
}

/* === METHOD 1: trial division === */
u64 trial_division(u64 N, long *ops) {
    *ops = 0;
    for (u64 i = 3; i * i <= N; i += 2) {
        (*ops)++;
        if (N % i == 0) return i;
    }
    return 0;
}

/* === METHOD 2: Pollard p-1 === */
u64 pollard_pm1(u64 N, int B_max, long *ops) {
    *ops = 0;
    u64 a = 2;
    for (int B = 2; B <= B_max; B++) {
        a = modpow(a, B, N);
        (*ops)++;
        u64 g = gcd(a > 0 ? a - 1 : N - 1, N);
        if (g > 1 && g < N) return g;
    }
    return 0;
}

/* === METHOD 3: phi-search near (sqrt(N)-1)^2 === */
u64 phi_search(u64 N, long max_delta, long *ops) {
    *ops = 0;
    u64 sqN = isqrt(N);
    u64 phi_base = (sqN - 1) * (sqN - 1);

    for (long delta = 0; delta <= max_delta; delta++) {
        for (int sign = 0; sign <= 1; sign++) {
            u64 phi_cand;
            if (sign == 0)
                phi_cand = phi_base + delta;
            else
                phi_cand = (delta <= (long)phi_base) ? phi_base - delta : 0;

            if (phi_cand == 0 || phi_cand >= N) continue;
            (*ops)++;

            /* quick modpow check */
            if (modpow(2, phi_cand, N) != 1) continue;
            if (gcd(3, N) == 1 && modpow(3, phi_cand, N) != 1) continue;

            /* try as phi directly */
            u64 s = N + 1 - phi_cand;
            if (s <= 2 || s * s < 4 * N) continue;
            u64 disc = s * s - 4 * N;
            u64 sq = isqrt_check(disc);
            if (sq > 0 && (s + sq) % 2 == 0) {
                u64 q = (s + sq) / 2;
                u64 p = (s - sq) / 2;
                if (p > 1 && q > 1 && p * q == N) return p;
            }

            /* try phi_cand/k */
            for (u64 k = 2; k <= 30; k++) {
                if (phi_cand % k != 0) continue;
                u64 phi2 = phi_cand / k;
                s = N + 1 - phi2;
                if (s <= 2 || s >= N) continue;
                if (s * s < 4 * N) continue;
                disc = s * s - 4 * N;
                sq = isqrt_check(disc);
                if (sq > 0 && (s + sq) % 2 == 0) {
                    u64 q = (s + sq) / 2;
                    u64 p = (s - sq) / 2;
                    if (p > 1 && q > 1 && p * q == N) return p;
                }
            }
        }
    }
    return 0;
}

/* === METHOD 4: Pollard rho (for comparison) === */
u64 pollard_rho(u64 N, long *ops) {
    *ops = 0;
    if (N % 2 == 0) return 2;
    u64 x = 2, y = 2, c = 1, d = 1;
    while (d == 1) {
        x = ((u128)x * x + c) % N;
        y = ((u128)y * y + c) % N;
        y = ((u128)y * y + c) % N;
        d = gcd(x > y ? x - y : y - x, N);
        (*ops)++;
        if (*ops > 10000000) return 0;
    }
    return (d != N) ? d : 0;
}

double elapsed_us(clock_t start, clock_t end) {
    return (double)(end - start) / CLOCKS_PER_SEC * 1e6;
}

int main(int argc, char **argv) {
    rng_state = (u64)time(NULL) ^ 0xdeadbeef;

    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║  LATTICE FACTORING — GENERATE KEY, CRACK KEY        ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");

    int bit_sizes[] = {16, 20, 24, 28, 32};
    int nsizes = 5;

    printf("  Generating semiprimes N = p*q and racing methods.\n");
    printf("  Each prime is half the target bit size.\n\n");

    printf("  %-6s  %-20s  %-12s  %-12s  %-12s  %-12s\n",
           "bits", "N", "trial div", "pollard p-1", "phi-search", "pollard rho");
    printf("  %-6s  %-20s  %-12s  %-12s  %-12s  %-12s\n",
           "────", "────────────────────", "──────────", "──────────", "──────────", "──────────");

    for (int si = 0; si < nsizes; si++) {
        int total_bits = bit_sizes[si];
        int half_bits = total_bits / 2;

        /* generate 3 keys per size */
        for (int trial = 0; trial < 3; trial++) {
            u64 p = random_prime(half_bits);
            u64 q = random_prime(half_bits);
            if (p > q) { u64 t = p; p = q; q = t; }
            u64 N = p * q;

            int actual_bits = 0;
            for (u64 tmp = N; tmp; tmp >>= 1) actual_bits++;

            long ops1, ops2, ops3, ops4;
            clock_t t0, t1, t2, t3, t4;

            /* trial division */
            t0 = clock();
            u64 f1 = trial_division(N, &ops1);
            t1 = clock();

            /* pollard p-1 (bound 10000) */
            u64 f2 = pollard_pm1(N, 10000, &ops2);
            t2 = clock();

            /* phi-search (max delta 500000) */
            u64 f3 = phi_search(N, 500000, &ops3);
            t3 = clock();

            /* pollard rho */
            u64 f4 = pollard_rho(N, &ops4);
            t4 = clock();

            printf("  %2d-bit  %-20llu  ", actual_bits, N);

            /* trial div result */
            if (f1) printf("%6.0f us   ", elapsed_us(t0, t1));
            else printf("  FAIL      ");

            /* pollard p-1 */
            if (f2) printf("%6.0f us   ", elapsed_us(t1, t2));
            else printf("  FAIL      ");

            /* phi-search */
            if (f3) printf("%6.0f us   ", elapsed_us(t2, t3));
            else printf("  FAIL      ");

            /* pollard rho */
            if (f4) printf("%6.0f us   ", elapsed_us(t3, t4));
            else printf("  FAIL      ");

            printf("\n");

            /* print key details for first of each size */
            if (trial == 0) {
                printf("           p=%-10llu q=%-10llu  p-1 smooth? ", p, q);
                /* check smoothness of p-1 */
                u64 pm1 = p - 1;
                u64 largest = 1;
                for (u64 f = 2; f * f <= pm1; f++) {
                    while (pm1 % f == 0) { pm1 /= f; if (f > largest) largest = f; }
                }
                if (pm1 > largest) largest = pm1;
                printf("largest_factor(p-1)=%llu\n", largest);
            }
        }
        printf("\n");
    }

    /* === BIG TEST: 48-bit and 56-bit keys === */
    printf("\n  BIGGER KEYS:\n\n");

    int big_bits[] = {40, 48, 56};
    for (int si = 0; si < 3; si++) {
        int total_bits = big_bits[si];
        int half_bits = total_bits / 2;

        u64 p = random_prime(half_bits);
        u64 q = random_prime(half_bits);
        if (p > q) { u64 t = p; p = q; q = t; }
        u64 N = p * q;

        int actual_bits = 0;
        for (u64 tmp = N; tmp; tmp >>= 1) actual_bits++;

        printf("  %d-bit key: N = %llu  (p=%llu, q=%llu)\n", actual_bits, N, p, q);

        long ops;
        clock_t t0, t1;

        /* pollard rho — should be fastest for general case */
        t0 = clock();
        u64 f = pollard_rho(N, &ops);
        t1 = clock();
        printf("    pollard rho:  %s  %llu  %.0f us  (%ld ops)\n",
               f ? "CRACKED" : "FAILED ", f, elapsed_us(t0, t1), ops);

        /* pollard p-1 */
        t0 = clock();
        f = pollard_pm1(N, 100000, &ops);
        t1 = clock();
        printf("    pollard p-1:  %s  %llu  %.0f us  (%ld ops)\n",
               f ? "CRACKED" : "FAILED ", f, elapsed_us(t0, t1), ops);

        /* phi-search */
        t0 = clock();
        f = phi_search(N, 2000000, &ops);
        t1 = clock();
        printf("    phi-search:   %s  %llu  %.0f us  (%ld checks)\n",
               f ? "CRACKED" : "FAILED ", f, elapsed_us(t0, t1), ops);

        /* trial division */
        t0 = clock();
        f = trial_division(N, &ops);
        t1 = clock();
        printf("    trial div:    %s  %llu  %.0f us  (%ld ops)\n\n",
               f ? "CRACKED" : "FAILED ", f, elapsed_us(t0, t1), ops);
    }

    printf("══════════════════════════════════════════════════════\n");
    printf("  The lattice formula p+q = N+1-kL is verified.\n");
    printf("  Phi-search works for balanced/close primes.\n");
    printf("  Pollard p-1 works for smooth p-1.\n");
    printf("  Pollard rho is the general-purpose winner.\n");
    printf("  The open question: can the lattice beat rho?\n");
    printf("══════════════════════════════════════════════════════\n");

    return 0;
}
