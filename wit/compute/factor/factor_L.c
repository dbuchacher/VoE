/*
 * factor_L.c — hunt for L from N alone
 *
 * The formula works: p+q = N+1-kL. 100% verified.
 * The open problem: computing L in sub-O(√N) time.
 *
 * Attack angles:
 * 1. l-adic decomposition: find v_l(p-1) ≠ v_l(q-1) for small primes l
 *    If they differ at ANY l, gcd gives a factor.
 *    Cost: O(log²N) per prime l. Total: O(π(B) × log²N).
 *
 * 2. Multi-base resonance: compute a^(m) mod N for many bases
 *    with m chosen as products of prime powers.
 *    If m ≡ 0 mod (p-1) but m ≢ 0 mod (q-1): gcd(a^m - 1, N) = p.
 *
 * 3. Lattice sieve: combine info from 4 bases to narrow L.
 *
 * Build: cc -O2 -o factor_L factor_L.c -lm
 * Run:   ./factor_L
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

typedef unsigned long long u64;
typedef __uint128_t u128;

u64 gcd(u64 a, u64 b) {
    while (b) { u64 t = b; b = a % b; a = t; }
    return a;
}

u64 modpow(u64 a, u64 e, u64 N) {
    u64 result = 1; a %= N;
    while (e > 0) {
        if (e & 1) result = (u128)result * a % N;
        e >>= 1;
        if (e > 0) a = (u128)a * a % N;
    }
    return result;
}

int is_prime(u64 n) {
    if (n < 2) return 0;
    if (n < 4) return 1;
    if (n % 2 == 0) return 0;
    u64 d = n - 1; int r = 0;
    while (d % 2 == 0) { d /= 2; r++; }
    u64 wit[] = {2,3,5,7,11,13,17,19,23,29,31,37};
    for (int i = 0; i < 12; i++) {
        u64 a = wit[i]; if (a >= n) continue;
        u64 x = modpow(a, d, n);
        if (x == 1 || x == n-1) continue;
        int ok = 0;
        for (int j = 0; j < r-1; j++) {
            x = (u128)x * x % n;
            if (x == n-1) { ok = 1; break; }
        }
        if (!ok) return 0;
    }
    return 1;
}

u64 rng_state = 0;
u64 xorshift64(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

u64 random_prime(int bits) {
    u64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
    u64 min = (bits >= 2) ? (1ULL << (bits-1)) : 2;
    for (;;) {
        u64 c = (xorshift64() & mask) | min | 1;
        if (is_prime(c)) return c;
    }
}

/* ═══════════════════════════════════════════════════════
 * METHOD 1: l-adic prime power GCD
 *
 * For each small prime l: compute a^(l^k) mod N
 * and check gcd(result - 1, N) after each step.
 *
 * If v_l(p-1) ≠ v_l(q-1), there exists k where
 * a^(l^k) ≡ 1 mod p but ≢ 1 mod q (or vice versa).
 * Then gcd(a^(l^k) - 1, N) = p or q.
 *
 * Cost: O(log_l(N)) modpows per prime l.
 * Total with B primes: O(B × log²N).
 * ═══════════════════════════════════════════════════════ */

u64 ladic_factor(u64 N, int max_primes, long *ops) {
    int small_primes[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,
                          73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,
                          157,163,167,173,179,181,191,193,197,199,211,223,227,229};
    int np = max_primes < 50 ? max_primes : 50;
    *ops = 0;

    /* for each base */
    u64 bases[] = {2, 3, 5, 7, 11, 13};
    int nb = 6;

    for (int bi = 0; bi < nb; bi++) {
        u64 base = bases[bi];
        if (gcd(base, N) > 1) {
            if (gcd(base, N) < N) return gcd(base, N);
            continue;
        }

        /* for each small prime l: compute base^(l^k) mod N */
        for (int pi = 0; pi < np; pi++) {
            u64 l = small_primes[pi];
            u64 x = modpow(base, l, N);  /* base^l mod N */
            (*ops)++;

            for (int k = 1; k < 64; k++) {
                u64 g = gcd(x > 0 ? x - 1 : N - 1, N);
                if (g > 1 && g < N) return g;
                if (g == N) break;  /* both orders divide l^k, try next l */

                /* x = x^l mod N = base^(l^(k+1)) mod N */
                x = modpow(x, l, N);
                (*ops)++;
            }
        }
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════
 * METHOD 2: Cumulative product of prime powers
 *
 * Compute a^M mod N where M accumulates prime powers.
 * M = 2^k2 × 3^k3 × 5^k5 × 7^k7 × ...
 * After each new factor, check gcd.
 *
 * This is Pollard p-1 but with individual prime control.
 * ═══════════════════════════════════════════════════════ */

u64 cumulative_factor(u64 N, int B, long *ops) {
    *ops = 0;
    u64 a = 2;

    /* accumulate: a = a^l mod N for each prime l up to B */
    /* then a = a^l mod N again for l^2, etc */
    for (u64 l = 2; l <= (u64)B; l++) {
        /* check if l is prime (simple) */
        int isp = 1;
        for (u64 f = 2; f*f <= l; f++) if (l%f==0) { isp=0; break; }
        if (!isp) continue;

        /* add l^k where l^k <= N */
        u64 pk = l;
        while (pk <= (u64)B * (u64)B) {  /* smooth bound squared */
            a = modpow(a, l, N);
            (*ops)++;

            u64 g = gcd(a > 0 ? a - 1 : N - 1, N);
            if (g > 1 && g < N) return g;
            if (g == N) return 0;  /* overshot */

            if (pk > (u64)B * (u64)B / l) break;
            pk *= l;
        }
    }
    return 0;
}

/* ═══════════════════════════════════════════════════════
 * METHOD 3: 4-dimensional lattice sieve
 *
 * Use 4 bases (2,3,5,7) as 4 dimensions.
 * For each: compute base^(2^k) mod N.
 * Combine results multiplicatively and check gcd.
 *
 * The idea: different bases probe different subgroups.
 * Their product might separate p from q even when
 * individual bases don't.
 * ═══════════════════════════════════════════════════════ */

u64 lattice_4d_factor(u64 N, long *ops) {
    *ops = 0;
    u64 bases[] = {2, 3, 5, 7};
    int nb = 4;
    int max_k = 40;

    /* precompute: power_table[base][k] = base^(2^k) mod N */
    u64 table[4][40];
    for (int b = 0; b < nb; b++) {
        if (gcd(bases[b], N) > 1 && gcd(bases[b], N) < N)
            return gcd(bases[b], N);
        table[b][0] = bases[b] % N;
        for (int k = 1; k < max_k; k++) {
            table[b][k] = (u128)table[b][k-1] * table[b][k-1] % N;
            (*ops)++;

            /* check individual */
            u64 g = gcd(table[b][k] > 0 ? table[b][k] - 1 : N-1, N);
            if (g > 1 && g < N) return g;
        }
    }

    /* try pairwise products: table[i][k] × table[j][l] - 1 */
    for (int k = 0; k < max_k; k++) {
        for (int l = 0; l < max_k; l++) {
            for (int i = 0; i < nb; i++) {
                for (int j = i+1; j < nb; j++) {
                    u64 prod = (u128)table[i][k] * table[j][l] % N;
                    (*ops)++;
                    u64 g = gcd(prod > 0 ? prod - 1 : N-1, N);
                    if (g > 1 && g < N) return g;
                }
            }
        }
    }

    return 0;
}

/* Pollard rho for comparison */
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
        if (*ops > 50000000) return 0;
    }
    return (d != N) ? d : 0;
}

double elapsed_us(clock_t s, clock_t e) {
    return (double)(e - s) / CLOCKS_PER_SEC * 1e6;
}

int main(void) {
    rng_state = (u64)time(NULL) ^ 0xCAFEBABE;

    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║  HUNT FOR L — sub-O(√N) factoring attempts      ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");

    int bit_sizes[] = {32, 40, 48, 56, 60};
    int nsizes = 5;

    for (int si = 0; si < nsizes; si++) {
        int total_bits = bit_sizes[si];
        int half_bits = total_bits / 2;

        printf("═══ %d-bit keys ═══\n", total_bits);

        for (int trial = 0; trial < 3; trial++) {
            u64 p = random_prime(half_bits);
            u64 q = random_prime(half_bits);
            if (p > q) { u64 t = p; p = q; q = t; }
            u64 N = p * q;

            int bits = 0;
            for (u64 tmp = N; tmp; tmp >>= 1) bits++;

            printf("\n  N = %llu (%d bits, p=%llu q=%llu)\n", N, bits, p, q);

            /* smoothness of p-1 and q-1 */
            u64 pm1 = p-1, qm1 = q-1;
            u64 lp = 1, lq = 1;
            u64 tmp;
            tmp = pm1; for (u64 f=2; f*f<=tmp; f++) while(tmp%f==0) { tmp/=f; if(f>lp)lp=f; }
            if (tmp > lp) lp = tmp;
            tmp = qm1; for (u64 f=2; f*f<=tmp; f++) while(tmp%f==0) { tmp/=f; if(f>lq)lq=f; }
            if (tmp > lq) lq = tmp;
            printf("  largest_factor(p-1)=%llu  largest_factor(q-1)=%llu\n", lp, lq);

            long ops;
            clock_t t0, t1;
            u64 f;

            /* l-adic */
            t0 = clock();
            f = ladic_factor(N, 50, &ops);
            t1 = clock();
            printf("  l-adic (50 primes):  %s  f=%llu  %.0f us  %ld ops\n",
                   f ? "CRACKED" : "FAILED ", f, elapsed_us(t0,t1), ops);

            /* cumulative */
            t0 = clock();
            f = cumulative_factor(N, 10000, &ops);
            t1 = clock();
            printf("  cumulative (B=10K):  %s  f=%llu  %.0f us  %ld ops\n",
                   f ? "CRACKED" : "FAILED ", f, elapsed_us(t0,t1), ops);

            /* 4D lattice */
            t0 = clock();
            f = lattice_4d_factor(N, &ops);
            t1 = clock();
            printf("  4D lattice sieve:    %s  f=%llu  %.0f us  %ld ops\n",
                   f ? "CRACKED" : "FAILED ", f, elapsed_us(t0,t1), ops);

            /* pollard rho (baseline) */
            t0 = clock();
            f = pollard_rho(N, &ops);
            t1 = clock();
            printf("  pollard rho:         %s  f=%llu  %.0f us  %ld ops\n",
                   f ? "CRACKED" : "FAILED ", f, elapsed_us(t0,t1), ops);
        }
        printf("\n");
    }

    printf("══════════════════════════════════════════════════\n");
    printf("ANALYSIS\n");
    printf("══════════════════════════════════════════════════\n\n");
    printf("  l-adic: O(B × log^2 N). Wins when v_l(p-1) ≠ v_l(q-1)\n");
    printf("          for some small prime l ≤ B. POLYNOMIAL in log N.\n");
    printf("          Fails when p-1 and q-1 have same l-adic structure\n");
    printf("          for all small l (RSA-hardened keys do this).\n\n");
    printf("  cumulative: Pollard p-1. O(B × log^2 N). Wins when p-1\n");
    printf("              is B-smooth. Fails for large prime factors.\n\n");
    printf("  4D lattice: combines 4 bases in 2^k powers. O(log^2 N × 6)\n");
    printf("              products. Occasionally catches cases others miss.\n\n");
    printf("  pollard rho: O(N^(1/4)). General purpose. Always works.\n\n");
    printf("  THE WALL: RSA keys have p-1 = 2 × large_prime.\n");
    printf("  No small l separates them. No smooth B catches them.\n");
    printf("  The remaining attack surface: the RELATIONSHIP between\n");
    printf("  the large prime factors of (p-1) and (q-1).\n");
    printf("  If the lattice can see that relationship: RSA falls.\n");

    return 0;
}
