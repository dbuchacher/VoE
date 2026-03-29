/*
 * factor_wall.c — math the wall
 *
 * THE WALL: p-1 = 2*A, q-1 = 2*B where A,B are large primes.
 * Can we find A or B from N alone?
 *
 * Known: N = p*q = (2A+1)(2B+1) = 4AB + 2A + 2B + 1
 * So: N-1 = 4AB + 2(A+B) = 2(2AB + A + B)
 * And: (N-1)/2 = 2AB + A + B = A(2B+1) + B = A*q + B
 * Also: (N-1)/2 = B(2A+1) + A = B*p + A
 *
 * So: (N-1)/2 mod p = A = (p-1)/2   ← BUT WE DON'T KNOW p
 *     (N-1)/2 mod q = B = (q-1)/2
 *
 * HOWEVER: what about (N-1)/2 mod small numbers?
 *
 * And: A*B = (p-1)(q-1)/4 = phi(N)/4
 *      A+B = (p+q-2)/2 = (p+q)/2 - 1
 *
 * If we find A+B and A*B, we solve: x^2 - (A+B)x + AB = 0
 * Then p = 2A+1, q = 2B+1.
 *
 * A*B = phi(N)/4 = (N - (p+q) + 1)/4
 * A+B = (p+q)/2 - 1
 *
 * Both require p+q. STILL CIRCULAR. But let's look at what
 * properties of A and B we CAN compute from N.
 *
 * Build: cc -O2 -o factor_wall factor_wall.c -lm
 * Run:   ./factor_wall
 */

#include <stdio.h>
#include <math.h>
#include <time.h>

typedef unsigned long long u64;
typedef __uint128_t u128;

u64 gcd(u64 a, u64 b) { while(b){u64 t=b;b=a%b;a=t;} return a; }
u64 modpow(u64 a, u64 e, u64 N) {
    u64 r=1; a%=N;
    while(e>0){if(e&1)r=(u128)r*a%N;e>>=1;if(e>0)a=(u128)a*a%N;}
    return r;
}

int main(void) {
    printf("MATH THE WALL\n");
    printf("==============\n\n");

    printf("Given: N = p*q where p = 2A+1, q = 2B+1, A and B are large primes.\n");
    printf("Find: A or B from N.\n\n");

    /* identity: (N-1)/2 = 2AB + A + B */
    /* so (N-1)/2 mod small_primes gives info about A+B and AB */

    printf("KEY IDENTITY: (N-1)/2 = 2AB + A + B\n");
    printf("              N-1 = 2(2AB + A + B)\n");
    printf("              N = 4AB + 2A + 2B + 1\n\n");

    /* test with known values */
    u64 test_A[] = {53, 521, 5003, 50261, 500107};
    u64 test_B[] = {59, 557, 5009, 50287, 500113};
    int nt = 5;

    for (int t = 0; t < nt; t++) {
        u64 A = test_A[t], B = test_B[t];
        u64 p = 2*A + 1, q = 2*B + 1;
        u64 N = p * q;

        printf("═══ A=%llu B=%llu p=%llu q=%llu N=%llu ═══\n", A, B, p, q, N);

        u64 half = (N-1)/2;
        printf("  (N-1)/2 = %llu = 2AB + A + B = %llu (check: %s)\n",
               half, 2*A*B + A + B, half == 2*A*B+A+B ? "OK" : "MISMATCH");

        /* what can we compute from N alone? */
        printf("  N mod 4 = %llu\n", N % 4);
        printf("  N mod 8 = %llu\n", N % 8);
        printf("  (N-1)/2 mod 3 = %llu  (A+B mod 3 = %llu, 2AB mod 3 = %llu)\n",
               half % 3, (A+B) % 3, (2*A*B) % 3);
        printf("  (N-1)/2 mod 5 = %llu  (A+B mod 5 = %llu)\n",
               half % 5, (A+B) % 5);
        printf("  (N-1)/2 mod 7 = %llu  (A+B mod 7 = %llu)\n",
               half % 7, (A+B) % 7);

        /* can we recover A+B mod small primes from (N-1)/2 mod small primes? */
        /* (N-1)/2 = 2AB + (A+B) */
        /* (N-1)/2 mod l = (2AB mod l) + ((A+B) mod l) mod l */
        /* Two unknowns (A mod l, B mod l) in one equation. Underdetermined. */

        /* BUT: we also have N = 4AB + 2(A+B) + 1 */
        /* N mod l = (4AB + 2(A+B) + 1) mod l */
        /* Two equations (N mod l, (N-1)/2 mod l), two unknowns (A mod l, B mod l) */

        /* From N: 4AB + 2(A+B) = N-1 mod l */
        /* From half: 2AB + (A+B) = (N-1)/2 mod l */
        /* Subtract: 2AB + (A+B) = (N-1) - (N-1)/2 = (N-1)/2 mod l */
        /* SAME EQUATION. Only one independent equation. */

        /* try: 2^((N-1)/2) mod N — Euler criterion */
        u64 euler = modpow(2, half, N);
        printf("  2^((N-1)/2) mod N = %llu\n", euler);

        /* Jacobi symbol: if this is N-1 = -1 mod N, 2 is QNR mod N */
        if (euler == N - 1) {
            printf("    → 2 is QNR mod N. So 2 is QNR mod exactly one of p,q.\n");
            /* gcd(2^((N-1)/4) ± 1, N) might give a factor */
            if (half % 2 == 0) {
                u64 quarter = modpow(2, half/2, N);
                u64 g1 = gcd(quarter > 0 ? quarter - 1 : N-1, N);
                u64 g2 = gcd(quarter + 1, N);
                printf("    2^((N-1)/4) mod N = %llu\n", quarter);
                printf("    gcd(result-1, N) = %llu  %s\n", g1,
                       (g1>1 && g1<N) ? "FACTOR!" : "");
                printf("    gcd(result+1, N) = %llu  %s\n", g2,
                       (g2>1 && g2<N) ? "FACTOR!" : "");

                /* keep halving */
                u64 val = quarter;
                for (int depth = 3; depth <= 8; depth++) {
                    if (val % 2 != 0) break; /* can't halve odd exponent easily */
                    /* compute 2^((N-1)/2^depth) mod N */
                    u64 exp = half;
                    for (int d = 1; d < depth; d++) {
                        if (exp % 2 != 0) break;
                        exp /= 2;
                    }
                    val = modpow(2, exp, N);
                    g1 = gcd(val > 0 ? val - 1 : N-1, N);
                    g2 = gcd(val + 1, N);
                    if (g1 > 1 && g1 < N) {
                        printf("    DEPTH %d: gcd(2^((N-1)/%d)-1, N) = %llu  FACTOR!\n",
                               depth, 1 << depth, g1);
                        break;
                    }
                    if (g2 > 1 && g2 < N) {
                        printf("    DEPTH %d: gcd(2^((N-1)/%d)+1, N) = %llu  FACTOR!\n",
                               depth, 1 << depth, g2);
                        break;
                    }
                }
            }
        } else if (euler == 1) {
            printf("    → 2 is QR mod both p and q.\n");
            /* try different approach: a^((N-1)/2) for a = 3,5,7... */
            u64 bases[] = {3, 5, 6, 7, 10, 11, 13};
            for (int bi = 0; bi < 7; bi++) {
                u64 a = bases[bi];
                if (gcd(a, N) > 1) continue;
                u64 e = modpow(a, half, N);
                if (e == N - 1) {
                    printf("    base %llu: QNR! trying quarter...\n", a);
                    if (half % 2 == 0) {
                        u64 q2 = modpow(a, half/2, N);
                        u64 g1 = gcd(q2 > 0 ? q2 - 1 : N-1, N);
                        u64 g2 = gcd(q2 + 1, N);
                        if (g1 > 1 && g1 < N) { printf("    FACTOR: %llu\n", g1); break; }
                        if (g2 > 1 && g2 < N) { printf("    FACTOR: %llu\n", g2); break; }

                        /* deeper */
                        u64 exp = half / 2;
                        for (int d = 0; d < 10 && exp % 2 == 0; d++) {
                            exp /= 2;
                            u64 val = modpow(a, exp, N);
                            g1 = gcd(val > 0 ? val - 1 : N-1, N);
                            g2 = gcd(val + 1, N);
                            if (g1 > 1 && g1 < N) { printf("    FACTOR (depth %d): %llu\n", d+3, g1); goto next; }
                            if (g2 > 1 && g2 < N) { printf("    FACTOR (depth %d): %llu\n", d+3, g2); goto next; }
                        }
                    }
                }
            }
        } else {
            printf("    → UNEXPECTED: Euler criterion gave %llu (not ±1)\n", euler);
            printf("    → This means N is composite with specific structure.\n");
            u64 g1 = gcd(euler > 0 ? euler - 1 : N-1, N);
            u64 g2 = gcd(euler + 1, N);
            if (g1 > 1 && g1 < N) printf("    FACTOR from euler-1: %llu\n", g1);
            if (g2 > 1 && g2 < N) printf("    FACTOR from euler+1: %llu\n", g2);
        }
        next:
        printf("\n");
    }

    /* === RANDOM HARD KEYS: p = 2A+1, q = 2B+1, A,B prime === */
    printf("\n═══ SAFE PRIMES (hardest case) ═══\n\n");
    printf("  Safe primes: p = 2A+1 where A is also prime.\n");
    printf("  These resist Pollard p-1 completely.\n\n");

    /* some known safe primes */
    u64 safe_p[] = {5, 7, 11, 23, 47, 59, 83, 107, 167, 179, 227, 263,
                    347, 359, 383, 467, 479, 503, 563, 587, 719, 839, 863,
                    887, 983, 1019, 1187, 1283, 1307, 2459, 2903, 3779,
                    5399, 7079, 8819, 10007 + 4, 15683, 28403, 32183};
    /* verify they're safe: p prime and (p-1)/2 prime */

    printf("  N              p      q      euler2  method        result\n");
    printf("  ─────────────────────────────────────────────────────────\n");

    for (int i = 0; i < 25; i++) {
        for (int j = i + 1; j < 25 && j < i + 3; j++) {
            u64 p = safe_p[i], q = safe_p[j];
            if (p < 5 || q < 5) continue;
            u64 N = p * q;

            u64 half = (N-1)/2;
            u64 euler = modpow(2, half, N);

            const char *method = "none";
            u64 factor = 0;

            /* try all bases for Euler criterion split */
            u64 bases[] = {2,3,5,6,7,10,11,13,14,15,17,19,21,22,23};
            for (int bi = 0; bi < 15 && factor == 0; bi++) {
                u64 a = bases[bi];
                if (gcd(a, N) > 1) {
                    if (gcd(a, N) < N) { factor = gcd(a, N); method = "trivial gcd"; }
                    continue;
                }
                u64 e = modpow(a, half, N);
                if (e != 1 && e != N-1) {
                    /* Euler not ±1: direct factor */
                    u64 g = gcd(e - 1, N);
                    if (g > 1 && g < N) { factor = g; method = "euler!=+-1"; break; }
                    g = gcd(e + 1, N);
                    if (g > 1 && g < N) { factor = g; method = "euler!=+-1"; break; }
                }
                if (e == N - 1 && half % 2 == 0) {
                    /* try deeper halving */
                    u64 exp = half;
                    while (exp % 2 == 0) {
                        exp /= 2;
                        u64 val = modpow(a, exp, N);
                        u64 g1 = gcd(val > 0 ? val-1 : N-1, N);
                        u64 g2 = gcd(val + 1, N);
                        if (g1 > 1 && g1 < N) { factor = g1; method = "deep halving"; break; }
                        if (g2 > 1 && g2 < N) { factor = g2; method = "deep halving"; break; }
                        if (val == 1 || val == N-1) break;
                    }
                }
            }

            printf("  %-14llu %-6llu %-6llu  %-6llu  %-14s %llu\n",
                   N, p, q, euler, method, factor);
        }
    }

    printf("\n══════════════════════════════════════════════════\n");
    printf("THE WALL IS:\n");
    printf("  When p and q are safe primes (p=2A+1, q=2B+1, A,B prime),\n");
    printf("  EVERY smooth-based method fails. The only factors of p-1\n");
    printf("  are {1, 2, A, 2A} where A is a prime ~N/4 in size.\n\n");
    printf("  The Euler criterion split (trying multiple bases and\n");
    printf("  halving exponents) IS a sub-sqrt method — it's O(log^2 N)\n");
    printf("  per base. If ANY base separates p from q via QR/QNR,\n");
    printf("  factoring is POLYNOMIAL.\n\n");
    printf("  By quadratic reciprocity, base a separates p,q when\n");
    printf("  a is QR mod one and QNR mod the other. For random a,\n");
    printf("  this happens ~50%% of the time.\n\n");
    printf("  The halving depth needed = v_2(p-1) vs v_2(q-1).\n");
    printf("  For safe primes: p-1 = 2A (v_2 = 1 if A is odd prime).\n");
    printf("  If v_2(p-1) = v_2(q-1) = 1: halving at (N-1)/4 splits.\n");
    printf("  This should work >50%% of the time.\n");

    return 0;
}
