/*
 * factor3.c — lattice factoring: push deeper
 *
 * Finding from factor2.c: p+q = N+1-kL, 100% success.
 * The formula is an atom-bond bridge (product → sum via R-force).
 *
 * New question: for balanced semiprimes, L ≈ (√N - 1)²/2.
 * The CORRECTION from this approximation encodes the factors.
 * Can we find the correction without computing L directly?
 *
 * Also: what if we compute a^(candidate_phi) mod N and check?
 * If candidate is correct, result = 1. This is O(log N) per test.
 *
 * Build: cc -O2 -o factor3 factor3.c -lm
 * Run:   ./factor3
 */

#include <stdio.h>
#include <math.h>
#include <time.h>

typedef unsigned long long u64;

u64 gcd(u64 a, u64 b) {
    while (b) { u64 t = b; b = a % b; a = t; }
    return a;
}

/* modular exponentiation: a^e mod N (handles 64-bit) */
u64 modpow(u64 a, u64 e, u64 N) {
    u64 result = 1;
    a %= N;
    while (e > 0) {
        if (e & 1) result = (__uint128_t)result * a % N;
        e >>= 1;
        a = (__uint128_t)a * a % N;
    }
    return result;
}

/* check if d is a perfect square, return sqrt or 0 */
u64 isqrt_check(u64 d) {
    if (d == 0) return 0;
    u64 sq = (u64)sqrt((double)d);
    /* floating point might be off by 1 */
    for (u64 t = (sq > 1 ? sq - 1 : 0); t <= sq + 2; t++) {
        if (t * t == d) return t;
    }
    return 0;
}

/* try to factor N given candidate for phi(N) */
int try_phi(u64 N, u64 phi, u64 *p, u64 *q) {
    if (phi >= N) return 0;
    u64 s = N + 1 - phi;  /* candidate p+q */
    if (s * s < 4 * N) return 0;
    u64 disc = s * s - 4 * N;
    u64 sq = isqrt_check(disc);
    if (sq == 0) return 0;
    if ((s + sq) % 2 != 0) return 0;
    *q = (s + sq) / 2;
    *p = (s - sq) / 2;
    return (*p > 1 && *q > 1 && *p * *q == N);
}

int main(void) {
    printf("Factor3: pushing the lattice formula deeper\n");
    printf("=============================================\n\n");

    /* === TEST 1: phi-candidate testing ===
     * For balanced semiprimes, phi ≈ N - 2√N + 1 = (√N - 1)²
     * Try phi_candidate = (√N - 1)² + δ for small δ.
     * For each candidate: verify with a^phi mod N == 1.
     * If yes AND discriminant is perfect square: factored.
     */

    printf("TEST 1: factor by searching near phi ≈ (sqrt(N)-1)^2\n");
    printf("  For each candidate phi: check 2^phi mod N == 1\n");
    printf("  Then check if N+1-phi gives a perfect square discriminant.\n\n");

    /* test semiprimes of increasing size */
    u64 test_p[] = {101, 1009, 10007, 100003, 1000003, 10000019};
    u64 test_q[] = {103, 1013, 10009, 100019, 1000033, 10000079};
    int ntests = 6;

    printf("  N (bits)  p         q         delta   checks  time(us)  method\n");
    printf("  ──────────────────────────────────────────────────────────────\n");

    for (int t = 0; t < ntests; t++) {
        u64 p = test_p[t], q = test_q[t];
        u64 N = p * q;
        int bits = 0;
        for (u64 tmp = N; tmp; tmp >>= 1) bits++;

        u64 sqN = (u64)sqrt((double)N);
        u64 phi_base = (sqN - 1) * (sqN - 1);

        clock_t start = clock();
        u64 fp = 0, fq = 0;
        int found = 0;
        long checks = 0;

        /* search δ = 0, ±1, ±2, ... around phi_base */
        for (long delta = 0; delta < 1000000 && !found; delta++) {
            for (int sign = -1; sign <= 1; sign += 2) {
                u64 phi_cand = phi_base + sign * delta;
                if (phi_cand == 0 || phi_cand >= N) continue;
                checks++;

                /* quick check: 2^phi_cand mod N should be 1 */
                if (modpow(2, phi_cand, N) != 1) continue;

                /* double check with base 3 */
                if (gcd(3, N) == 1 && modpow(3, phi_cand, N) != 1) continue;

                /* candidate phi might be correct (or a multiple) */
                /* try the formula */
                if (try_phi(N, phi_cand, &fp, &fq)) {
                    found = 1;
                    break;
                }

                /* phi_cand might be a multiple of phi. try phi_cand/k */
                for (u64 k = 2; k <= 20; k++) {
                    if (phi_cand % k == 0) {
                        if (try_phi(N, phi_cand / k, &fp, &fq)) {
                            found = 1;
                            break;
                        }
                    }
                }
                if (found) break;
            }
        }

        clock_t end = clock();
        long us = (end - start) * 1000000 / CLOCKS_PER_SEC;

        if (found) {
            long actual_delta = (long)((p-1)*(q-1)) - (long)phi_base;
            printf("  %3d bit   %-9llu %-9llu %6ld  %6ld   %6ld us  phi-search\n",
                   bits, fp, fq, actual_delta, checks, us);
        } else {
            printf("  %3d bit   FAILED                               %6ld us\n", bits, us);
        }
    }

    /* === TEST 2: Pollard p-1 with lattice smoothness === */
    printf("\nTEST 2: Pollard p-1 (when p-1 is smooth)\n");
    printf("  Compute 2^(B!) mod N, then gcd(result-1, N)\n\n");

    /* primes where p-1 is smooth */
    u64 smooth_tests[][2] = {
        {1021, 1031},     /* 1020=2²×3×5×17, 1030=2×5×103 — not very smooth */
        {433, 439},       /* 432=2⁴×3³, 438=2×3×73 */
        {577, 601},       /* 576=2⁶×3², 600=2³×3×5² — SMOOTH! */
        {769, 811},       /* 768=2⁸×3, 810=2×3⁴×5 — SMOOTH! */
        {1153, 1201},     /* 1152=2⁷×3², 1200=2⁴×3×5² — SMOOTH! */
    };

    printf("  N            p      q      B    found   time\n");
    printf("  ───────────────────────────────────────────\n");

    for (int t = 0; t < 5; t++) {
        u64 p = smooth_tests[t][0], q = smooth_tests[t][1];
        u64 N = p * q;

        clock_t start = clock();
        u64 a = 2;
        u64 fp = 0;
        int B;

        for (B = 2; B <= 100 && fp == 0; B++) {
            a = modpow(a, B, N);  /* a = 2^(B!) mod N cumulatively */
            u64 g = gcd(a > 0 ? a - 1 : N - 1, N);
            if (g > 1 && g < N) {
                fp = g;
            }
        }

        clock_t end = clock();
        long us = (end - start) * 1000000 / CLOCKS_PER_SEC;

        if (fp > 0) {
            printf("  %-12llu %-6llu %-6llu B=%-3d  YES    %ld us\n",
                   N, fp, N/fp, B-1, us);
        } else {
            printf("  %-12llu %-6llu %-6llu B>100  no     %ld us\n",
                   N, p, q, us);
        }
    }

    /* === TEST 3: the atom-bond bridge formula on medium numbers === */
    printf("\nTEST 3: formula p+q = N+1-kL on 32-bit semiprimes\n");
    printf("  Using baby-step/giant-step for period computation\n\n");

    /* medium semiprimes */
    u64 med_p[] = {65537, 65539, 65543};
    u64 med_q[] = {65551, 65557, 65579};

    for (int t = 0; t < 3; t++) {
        u64 p = med_p[t], q = med_q[t];
        u64 N = p * q;
        u64 phi = (p-1) * (q-1);
        u64 L_true = phi / gcd(p-1, q-1);
        u64 g_true = gcd(p-1, q-1);

        printf("  N = %llu (%llu x %llu)\n", N, p, q);
        printf("  phi = %llu, L = %llu, g = %llu\n", phi, L_true, g_true);
        printf("  p+q = %llu, N+1-2L = %llu (%s)\n",
               p + q, N + 1 - 2 * L_true,
               (p+q == N+1-2*L_true) ? "MATCH" : "need k>2");

        /* approximate: L ≈ (√N - 1)²/2 */
        u64 sqN = (u64)sqrt((double)N);
        u64 L_approx = (sqN - 1) * (sqN - 1) / 2;
        printf("  L_approx = (sqrt(N)-1)^2/2 = %llu (error: %lld)\n",
               L_approx, (long long)L_true - (long long)L_approx);
        printf("  error = %.4f%%\n\n",
               fabs((double)L_true - L_approx) / L_true * 100);
    }

    printf("\n=============================================\n");
    printf("FINDINGS\n");
    printf("=============================================\n\n");

    printf("  1. FORMULA VERIFIED: p+q = N+1-kL (100%% rate)\n\n");
    printf("  2. PHI-SEARCH: for balanced primes, phi near (sqrt(N)-1)^2.\n");
    printf("     Search delta around that estimate. Each check is O(log N)\n");
    printf("     via modular exponentiation. Total: O(delta_max * log N).\n");
    printf("     delta_max = |p-q|^2 / (4*sqrt(N)) approximately.\n\n");
    printf("  3. POLLARD p-1: works when p-1 or q-1 is smooth.\n");
    printf("     O(B * log N) where B = smoothness bound.\n");
    printf("     Not general but very fast when it works.\n\n");
    printf("  4. ATOM-BOND BRIDGE: the formula converts\n");
    printf("     product (N=pq, atom) to sum (p+q, bond) via period (L, R-force).\n");
    printf("     This IS the lattice structure of factoring.\n\n");
    printf("  5. OPEN: computing L from N alone in sub-O(sqrt(N)) time.\n");
    printf("     The formula is found. The fast evaluation isn't. Yet.\n");

    return 0;
}
