/*
 * factor_hunt.c — brute-force formula search
 *
 * The period r = f(a, N) exists. Find f.
 * Compute EVERY O(log N) quantity from N.
 * Check which ones equal p+q, phi(N), or r.
 *
 * If ANY computable quantity matches: that's the formula.
 *
 * Build: cc -O2 -o factor_hunt factor_hunt.c -lm
 * Run:   ./factor_hunt
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

typedef unsigned long long u64;
typedef __uint128_t u128;

u64 gcd(u64 a, u64 b) { while(b){u64 t=b;b=a%b;a=t;} return a; }
u64 modpow(u64 a, u64 e, u64 N) {
    u64 r=1; a%=N;
    while(e>0){if(e&1)r=(u128)r*a%N;e>>=1;if(e>0)a=(u128)a*a%N;}
    return r;
}
u64 isqrt(u64 n) {
    u64 x = (u64)sqrt((double)n);
    while (x*x > n) x--;
    while ((x+1)*(x+1) <= n) x++;
    return x;
}

int main(void) {
    printf("FORMULA HUNT: compute everything from N, match to targets\n");
    printf("==========================================================\n\n");

    /* test cases: known factorizations */
    u64 test_p[] = {101, 1009, 10007, 100003, 307, 809, 4999, 7919};
    u64 test_q[] = {103, 1013, 10009, 100019, 311, 811, 5003, 7927};
    int nt = 8;

    for (int t = 0; t < nt; t++) {
        u64 p = test_p[t], q = test_q[t];
        u64 N = p * q;
        u64 phi = (p-1)*(q-1);
        u64 s = p + q;       /* target: p+q */
        u64 sqN = isqrt(N);

        printf("N = %llu = %llu x %llu\n", N, p, q);
        printf("TARGETS: p+q = %llu,  phi = %llu,  phi/2 = %llu\n\n", s, phi, phi/2);

        /* compute lots of quantities from N alone */
        printf("  %-45s  value          match?\n", "quantity");
        printf("  %-45s  ─────          ──────\n", "────────");

        struct { const char *label; u64 value; } features[200];
        int nf = 0;

#define F(n, v) do { features[nf].label = (n); features[nf].value = (v); nf++; } while(0)

        nf = 0;
        F("sqrt(N)", sqN);
        F("sqrt(N)+1", sqN+1);
        F("2*sqrt(N)", 2*sqN);
        F("2*sqrt(N)+1", 2*sqN+1);
        F("2*sqrt(N)+2", 2*sqN+2);
        F("N - 2*sqrt(N) + 1  [(sqrt-1)^2]", N - 2*sqN + 1);
        F("(N-1)/2", (N-1)/2);
        F("N mod sqrt(N)", N % sqN);
        F("N mod (sqrt(N)-1)", sqN > 1 ? N % (sqN-1) : 0);
        F("N mod (sqrt(N)+1)", N % (sqN+1));
        F("N mod (2*sqrt(N))", N % (2*sqN));

        /* Euler criterion values */
        for (int a = 2; a <= 13; a++) {
            if (gcd(a, N) > 1) continue;
            u64 e = modpow(a, (N-1)/2, N);
            char buf[80];
            snprintf(buf, sizeof(buf), "%d^((N-1)/2) mod N", a);
            F(buf, e);

            /* try gcd of euler value with N */
            u64 g = gcd(e > 0 ? e - 1 : 0, N);
            snprintf(buf, sizeof(buf), "gcd(%d^((N-1)/2)-1, N)", a);
            if (g > 1 && g < N) F(buf, g);

            g = gcd(e + 1, N);
            snprintf(buf, sizeof(buf), "gcd(%d^((N-1)/2)+1, N)", a);
            if (g > 1 && g < N) F(buf, g);

            /* quarter */
            if ((N-1) % 4 == 0) {
                u64 e4 = modpow(a, (N-1)/4, N);
                snprintf(buf, sizeof(buf), "%d^((N-1)/4) mod N", a);
                F(buf, e4);

                g = gcd(e4 > 0 ? e4 - 1 : 0, N);
                snprintf(buf, sizeof(buf), "gcd(%d^((N-1)/4)-1, N)", a);
                if (g > 1 && g < N) F(buf, g);

                g = gcd(e4 + 1, N);
                snprintf(buf, sizeof(buf), "gcd(%d^((N-1)/4)+1, N)", a);
                if (g > 1 && g < N) F(buf, g);
            }

            /* a^sqrt(N) mod N */
            u64 asq = modpow(a, sqN, N);
            snprintf(buf, sizeof(buf), "%d^sqrt(N) mod N", a);
            F(buf, asq);

            g = gcd(asq > 0 ? asq - 1 : 0, N);
            snprintf(buf, sizeof(buf), "gcd(%d^sqrt(N)-1, N)", a);
            if (g > 1 && g < N) F(buf, g);

            /* a^(2*sqrt(N)) mod N */
            u64 a2sq = modpow(a, 2*sqN, N);
            snprintf(buf, sizeof(buf), "%d^(2*sqrt(N)) mod N", a);
            F(buf, a2sq);

            g = gcd(a2sq > 0 ? a2sq - 1 : 0, N);
            snprintf(buf, sizeof(buf), "gcd(%d^(2*sqrtN)-1, N)", a);
            if (g > 1 && g < N) F(buf, g);

            /* a^(N-sqrt(N)) mod N */
            u64 anms = modpow(a, N - sqN, N);
            snprintf(buf, sizeof(buf), "%d^(N-sqrt(N)) mod N", a);
            F(buf, anms);

            /* NEW: a^(N+1) mod N — if N=pq, a^(N+1) = a^(pq+1) */
            u64 anp1 = modpow(a, N+1, N);
            snprintf(buf, sizeof(buf), "%d^(N+1) mod N", a);
            F(buf, anp1);

            /* a^(N+1) = a^(pq+1) = a × (a^p)^q.
               By Fermat: a^p ≡ a mod p. So a^(pq+1) ≡ a^(q+1) mod p.
               Similarly: a^(pq+1) ≡ a^(p+1) mod q.
               So: a^(N+1) mod N = CRT(a^(q+1) mod p, a^(p+1) mod q)
               This MIXES p and q! The value encodes BOTH factors. */

            /* check: does a^(N+1) - a^2 reveal something? */
            u64 diff = (anp1 >= modpow(a, 2, N)) ?
                       anp1 - modpow(a, 2, N) :
                       N - modpow(a, 2, N) + anp1;
            snprintf(buf, sizeof(buf), "(%d^(N+1) - %d^2) mod N", a, a);
            F(buf, diff);

            g = gcd(diff, N);
            snprintf(buf, sizeof(buf), "gcd(%d^(N+1)-%d^2, N)", a, a);
            if (g > 1 && g < N) F(buf, g);

            /* a^(N+1) - a^(s_approx) where s_approx = 2*sqrtN */
            u64 as = modpow(a, 2*sqN, N);
            u64 diff2 = (anp1 >= as) ? anp1 - as : N - as + anp1;
            snprintf(buf, sizeof(buf), "(%d^(N+1) - %d^(2sqrtN)) mod N", a, a);
            F(buf, diff2);

            g = gcd(diff2, N);
            snprintf(buf, sizeof(buf), "gcd(%d^(N+1)-%d^(2sqrtN), N)", a, a);
            if (g > 1 && g < N) F(buf, g);
        }

        /* Jacobi-related */
        F("N mod 3", N % 3);
        F("N mod 4", N % 4);
        F("N mod 7", N % 7);
        F("N mod 8", N % 8);
        F("N mod 12", N % 12);
        F("N mod 16", N % 16);
        F("N mod 24", N % 24);

        /* print all, highlighting matches */
        int found_any = 0;
        for (int i = 0; i < nf; i++) {
            int is_s = (features[i].value == s);
            int is_p = (features[i].value == p);
            int is_q = (features[i].value == q);
            int is_phi = (features[i].value == phi);
            int is_phi2 = (features[i].value == phi/2);

            if (is_s || is_p || is_q || is_phi || is_phi2) {
                printf("  *** %-43s  %-14llu", features[i].label, features[i].value);
                if (is_s) printf(" = p+q!");
                if (is_p) printf(" = p!");
                if (is_q) printf(" = q!");
                if (is_phi) printf(" = phi!");
                if (is_phi2) printf(" = phi/2!");
                printf("\n");
                found_any = 1;
            }
        }

        if (!found_any)
            printf("  (no direct match found among %d features)\n", nf);

        printf("\n  Key Euler values:\n");
        for (int a = 2; a <= 7; a++) {
            if (gcd(a, N) > 1) continue;
            u64 e = modpow(a, (N-1)/2, N);
            u64 anp1 = modpow(a, N+1, N);
            u64 a2 = modpow(a, 2, N);
            printf("    %d^((N-1)/2) mod N = %llu  (%s)\n", a, e,
                   e == 1 ? "QR" : e == N-1 ? "QNR" : "SPLIT!");
            printf("    %d^(N+1) mod N = %llu,  %d^2 = %llu,  diff = %llu\n",
                   a, anp1, a, a2, (anp1 >= a2 ? anp1-a2 : N-a2+anp1));
            /* a^(N+1) = CRT(a^(q+1) mod p, a^(p+1) mod q)
               if we knew this decomposition, we'd have p and q */
        }

        printf("\n────────────────────────────────────────────────────────\n\n");
    }

    return 0;
}
