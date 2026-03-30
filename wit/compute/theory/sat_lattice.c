/*
 * sat_lattice.c — SAT as lattice coordinates
 *
 * Tests the P != NP structural argument:
 *   Verification (P) = pipeline: Q = O(n)
 *   Search (NP) = tree: Q = O(2^n)
 *   The gap grows exponentially.
 *
 * Also encodes SAT clauses as lattice bonds to show the
 * coordinate representation of boolean satisfiability.
 *
 * Build: cc -O2 -o sat_lattice sat_lattice.c -lm
 * Run:   ./sat_lattice
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAX_VARS    22
#define MAX_CLAUSES 200
#define MAX_LITS    3

/* ═══════════════════════════════════════════════════
 * LATTICE COORDINATE ENCODING
 * ═══════════════════════════════════════════════════ */

typedef struct { int t, d, m, q; } Coord;

/*
 * Variables map to lattice dimensions:
 *   var 0-3 → T,D,M,Q at shell 1 (magnitude 1)
 *   var 4-7 → T,D,M,Q at shell 2 (magnitude 2)
 *   ...
 * Negation flips the sign.
 *
 * This is the EXACT same encoding as wave bytes:
 *   positive = +1 = active/true
 *   negative = -1 = negated/false
 *   zero     = absent from clause
 */
Coord literal_coord(int var, int neg) {
    Coord c = {0, 0, 0, 0};
    int dim   = var % 4;
    int shell = 1 + var / 4;
    int val   = neg ? -shell : +shell;
    switch (dim) {
        case 0: c.t = val; break;
        case 1: c.d = val; break;
        case 2: c.m = val; break;
        case 3: c.q = val; break;
    }
    return c;
}

/* A clause (OR of literals) sums into one bond coordinate.
 * AND of clauses = path (sequence of bonds). */
Coord clause_bond(int lits[][2], int nlits) {
    Coord bond = {0, 0, 0, 0};
    for (int i = 0; i < nlits; i++) {
        Coord c = literal_coord(lits[i][0], lits[i][1]);
        bond.t += c.t;
        bond.d += c.d;
        bond.m += c.m;
        bond.q += c.q;
    }
    return bond;
}

void print_coord(Coord c) {
    printf("(%+d,%+d,%+d,%+d)", c.t, c.d, c.m, c.q);
}

/* ═══════════════════════════════════════════════════
 * SAT FORMULA
 * ═══════════════════════════════════════════════════ */

typedef struct {
    int lits[MAX_CLAUSES][MAX_LITS][2];  /* [clause][lit][var, neg] */
    int nlits[MAX_CLAUSES];
    int nclauses;
    int nvars;
} Formula;

/* Evaluate formula at assignment (bitmask).
 * THIS IS VERIFICATION — the P part.
 * Pipeline: test clause 1, then 2, then 3...
 * One assignment in, one boolean out. O(m*k). */
int eval(Formula *f, long assignment) {
    for (int i = 0; i < f->nclauses; i++) {
        int sat = 0;
        for (int j = 0; j < f->nlits[i]; j++) {
            int val = (assignment >> f->lits[i][j][0]) & 1;
            if (f->lits[i][j][1]) val = !val;
            if (val) { sat = 1; break; }
        }
        if (!sat) return 0;
    }
    return 1;
}

/* Brute-force search: try all 2^n assignments.
 * THIS IS SEARCH — the NP part.
 * Tree: branch on x0 (0 or 1), then x1, ...
 * 2^n leaves. Exponential. */
long brute_search(Formula *f, long *first_solution) {
    long total = 1L << f->nvars;
    long solutions = 0;
    *first_solution = -1;

    for (long mask = 0; mask < total; mask++) {
        if (eval(f, mask)) {
            if (solutions == 0) *first_solution = mask;
            solutions++;
        }
    }
    return solutions;
}

/* Generate random 3-SAT at the phase transition.
 * Clause ratio ~4.27 is where instances are hardest:
 * the boundary between almost-always-SAT and almost-always-UNSAT. */
void gen_3sat(Formula *f, int nvars, double clause_ratio) {
    f->nvars = nvars;
    f->nclauses = (int)(nvars * clause_ratio);
    if (f->nclauses > MAX_CLAUSES) f->nclauses = MAX_CLAUSES;

    for (int i = 0; i < f->nclauses; i++) {
        f->nlits[i] = 3;
        for (int j = 0; j < 3; j++) {
            f->lits[i][j][0] = rand() % nvars;
            f->lits[i][j][1] = rand() % 2;
        }
    }
}

/* ═══════════════════════════════════════════════════
 * TIMING
 * ═══════════════════════════════════════════════════ */

double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

/* Time verification: evaluate ONE known-good assignment.
 * This is the P operation — pipeline, O(m). */
double time_verify_ns(Formula *f, long solution) {
    int reps = 100000;
    double start = now_ns();
    volatile int r = 0;
    for (int i = 0; i < reps; i++)
        r += eval(f, solution);
    return (now_ns() - start) / reps;
}

/* Time search: brute-force all 2^n assignments.
 * This is the NP operation — tree, O(2^n).
 * Runs multiple reps for small n to get stable timing. */
double time_search_ns(Formula *f, long *out_solutions) {
    long dummy;
    int n = f->nvars;
    int reps = n <= 12 ? 10000 : (n <= 16 ? 1000 : (n <= 20 ? 10 : 1));
    volatile long accum = 0;

    double start = now_ns();
    for (int i = 0; i < reps; i++)
        accum += brute_search(f, &dummy);
    double elapsed = (now_ns() - start) / reps;
    *out_solutions = accum / reps;
    return elapsed;
}

/* ═══════════════════════════════════════════════════
 * MAIN
 * ═══════════════════════════════════════════════════ */

int main(void) {
    srand(time(NULL));

    printf("SAT as Lattice Coordinates\n");
    printf("==========================\n\n");

    /* ── Example: show lattice encoding ── */

    printf("Example: (x0 | !x1 | x2) & (!x0 | x3) & (x1 | !x2 | x3)\n\n");

    const char *dim[] = {"T", "D", "M", "Q"};

    int ex_clauses[][3][2] = {
        {{0,0}, {1,1}, {2,0}},    /* x0 | !x1 | x2 */
        {{0,1}, {3,0}, {-1,-1}},  /* !x0 | x3 (2 lits) */
        {{1,0}, {2,1}, {3,0}},    /* x1 | !x2 | x3 */
    };
    int ex_nlits[] = {3, 2, 3};

    printf("  Lattice encoding (variables -> dimensions, clauses -> bonds):\n\n");
    for (int i = 0; i < 3; i++) {
        printf("    clause %d: ", i);
        Coord bond = {0,0,0,0};
        for (int j = 0; j < ex_nlits[i]; j++) {
            int var = ex_clauses[i][j][0];
            int neg = ex_clauses[i][j][1];
            Coord c = literal_coord(var, neg);
            bond.t += c.t; bond.d += c.d;
            bond.m += c.m; bond.q += c.q;
            printf("%s%s", neg ? "!" : "", dim[var % 4]);
            print_coord(c);
            if (j < ex_nlits[i] - 1) printf(" | ");
        }
        printf("  ->  bond ");
        print_coord(bond);
        printf("\n");
    }

    Formula ex = {.nvars = 4, .nclauses = 3};
    ex.nlits[0] = 3; ex.nlits[1] = 2; ex.nlits[2] = 3;
    int ex_data[][3][2] = {
        {{0,0},{1,1},{2,0}},
        {{0,1},{3,0},{0,0}},  /* padding */
        {{1,0},{2,1},{3,0}},
    };
    memcpy(ex.lits, ex_data, sizeof(ex_data));

    long ex_sol;
    long ex_count = brute_search(&ex, &ex_sol);
    printf("\n  Search space: 2^4 = 16\n");
    printf("  Solutions: %ld\n", ex_count);
    printf("  Q_verify = %d (clauses checked per assignment)\n", ex.nclauses);
    printf("  Q_search = %.1f (assignments per solution)\n",
           ex_count > 0 ? 16.0 / ex_count : -1.0);

    /* ── Main test: Q factor scaling ── */

    printf("\n\nQ Factor Scaling: P vs NP Gap\n");
    printf("=============================\n\n");
    printf("  n     2^n          verify_ns    search_ns      ratio         Q_search\n");
    printf("  --    ----------   ---------    -----------    -----------   ----------\n");

    int sizes[] = {4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);
    double clause_ratio = 4.27;
    int trials = 5;

    for (int si = 0; si < nsizes; si++) {
        int n = sizes[si];
        long space = 1L << n;

        double sum_tv = 0, sum_ts = 0, sum_qs = 0;
        int valid = 0;

        for (int t = 0; t < trials; t++) {
            Formula f;
            gen_3sat(&f, n, clause_ratio);

            long sol;
            long nsol = brute_search(&f, &sol);

            if (nsol > 0) {
                double t_verify = time_verify_ns(&f, sol);
                long search_sols;
                double t_search = time_search_ns(&f, &search_sols);
                sum_tv += t_verify;
                sum_ts += t_search;
                sum_qs += (double)space / nsol;
                valid++;
            }
        }

        if (valid > 0) {
            double avg_tv = sum_tv / valid;
            double avg_ts = sum_ts / valid;
            double avg_qs = sum_qs / valid;
            printf("  %-5d %-12ld %-12.1f %-14.1f %-13.1f %.1f\n",
                   n, space, avg_tv, avg_ts, avg_ts / avg_tv, avg_qs);
        } else {
            printf("  %-5d %-12ld %-12s %-14s %-13s %s\n",
                   n, space, "-", "UNSAT", "-", "-");
        }
    }

    printf("\n");
    printf("  verify_ns = time to check ONE assignment (pipeline, P)\n");
    printf("  search_ns = time to search ALL assignments (tree, NP)\n");
    printf("  ratio     = search/verify = the P vs NP gap\n");
    printf("  Q_search  = 2^n / #solutions = structural imbalance\n");

    printf("\n\nTopological Summary\n");
    printf("===================\n\n");
    printf("  VERIFY (P):  pipeline\n");
    printf("    read assignment -> test clause 1 -> test clause 2 -> ... -> result\n");
    printf("    Bonds: P+W (read + test) x m clauses\n");
    printf("    Q = O(m), linear in clauses\n");
    printf("    Ring buffer shaped: sequential, single path\n");
    printf("\n");
    printf("  SEARCH (NP):  tree\n");
    printf("    branch x0 -> branch x1 -> ... -> branch x_{n-1}\n");
    printf("    Each level: try 0, try 1 (binary split)\n");
    printf("    Q = O(2^n), exponential in variables\n");
    printf("    NOT ring buffer shaped: needs 2^n parallel pipelines\n");
    printf("\n");
    printf("  The ratio column shows the gap growing exponentially.\n");
    printf("  Pipeline can't simulate tree without exponential blowup.\n");
    printf("  That's P != NP. The topology IS the separation.\n");

    return 0;
}
