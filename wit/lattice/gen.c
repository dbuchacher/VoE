/*
 * gen.c — the lattice as math
 *
 * Outputs every atom AND bond as a formula.
 * Atoms = even coordinates = multiplication (T^a · D^b · M^c · Q^d)
 * Bonds = odd coordinates = control flow (forces: P, C, R, W)
 *
 * Shell count formula: shell(k) = (4k+1)^4 - (4k-1)^4
 *   k=0: 1    k=½: 80    k=1: 544    k=3/2: 1,776    k=2: 4,160
 * Use to verify output completeness. Counts explode fast — that's
 * why --all uses 2-dim only above degree 4.
 *
 * Usage:
 *   ./gen --degree N [k]    # atoms at degree N, optional k-dim filter
 *   ./gen --bonds N         # bonds at half-degree N (magnitude N coords)
 *   ./gen --all             # everything: atoms + bonds
 *   ./gen T D M Q           # single atom (integer exponents)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *dim_name[] = {"T", "D", "M", "Q"};

/* force names: P=transport, C=synthesis, R=cycling, W=regulation */
static const char *force_name[] = {"P", "C", "R", "W"};

/* bond pattern names indexed by bitmask (P=1, C=2, R=4, W=8) */

/* gate names for 2-dim combinations */

/* count active (nonzero) dimensions */
int active_dims(int a, int b, int c, int d) {
    return (a != 0) + (b != 0) + (c != 0) + (d != 0);
}

/* ══════════════════════════════════════════════════════════
 * ATOMS — even coordinates, multiplication/division
 * ══════════════════════════════════════════════════════════ */

void print_atom(int t, int d, int m, int q) {
    int exp[4] = {t, d, m, q};
    int i;

    printf("  (%+d,%+d,%+d,%+d)  ", t*2, d*2, m*2, q*2);

    int has_pos = 0, has_neg = 0;
    for (i = 0; i < 4; i++) {
        if (exp[i] > 0) has_pos = 1;
        if (exp[i] < 0) has_neg = 1;
    }

    if (!has_pos && !has_neg) {
        printf("1                    wave (identity)");
    } else {
        int first = 1;
        for (i = 0; i < 4; i++) {
            if (exp[i] > 0) {
                if (!first) printf("·");
                printf("%s", dim_name[i]);
                if (exp[i] > 1) printf("^%d", exp[i]);
                first = 0;
            }
        }
        if (!has_pos) printf("1");

        if (has_neg) {
            printf("/");
            first = 1;
            for (i = 0; i < 4; i++) {
                if (exp[i] < 0) {
                    if (!first) printf("·");
                    printf("%s", dim_name[i]);
                    if (exp[i] < -1) printf("^%d", -exp[i]);
                    first = 0;
                }
            }
        }

        /* pure formula only — domain labels are in codex.txt */
    }

    printf("\n");
}

void gen_atoms(int degree, int max_k) {
    int n = degree;
    int t, d, m, q;
    int count = 0;

    printf("# ATOMS — Degree %d (multiply/divide)\n", n);
    if (max_k > 0) printf("# (%d-dim only)\n", max_k);
    printf("#\n");

    for (t = -n; t <= n; t++)
    for (d = -n; d <= n; d++)
    for (m = -n; m <= n; m++)
    for (q = -n; q <= n; q++) {
        int maxabs = abs(t);
        if (abs(d) > maxabs) maxabs = abs(d);
        if (abs(m) > maxabs) maxabs = abs(m);
        if (abs(q) > maxabs) maxabs = abs(q);
        if (maxabs != n) continue;
        if (n > 0 && t == 0 && d == 0 && m == 0 && q == 0) continue;
        if (max_k > 0 && active_dims(t, d, m, q) > max_k) continue;

        print_atom(t, d, m, q);
        count++;
    }

    printf("# %d atoms\n\n", count);
}

/* ══════════════════════════════════════════════════════════
 * BONDS — odd coordinates, control flow
 * ══════════════════════════════════════════════════════════ */

void print_bond(int t, int d, int m, int q) {
    int coords[4] = {t, d, m, q};
    int i;

    /* coordinates */
    printf("  (%+d,%+d,%+d,%+d)  ", t, d, m, q);

    /* forces with sign and magnitude */
    for (i = 0; i < 4; i++) {
        if (coords[i] > 0) {
            printf("+%s", force_name[i]);
            if (coords[i] > 1) printf("%d", coords[i]);
        } else if (coords[i] < 0) {
            printf("-%s", force_name[i]);
            if (coords[i] < -1) printf("%d", -coords[i]);
        }
    }

    printf("\n");
}

void gen_bonds(int magnitude) {
    int t, d, m, q;
    int count = 0;

    printf("# BONDS — Magnitude %d (control flow)\n", magnitude);
    printf("# Forces: P=transport C=synthesis R=cycling W=regulation\n");
    printf("# +dir=consume/forward  -dir=produce/backward\n");
    printf("#\n");

    /* enumerate all odd-coordinate tuples at this magnitude */
    for (t = -magnitude; t <= magnitude; t++)
    for (d = -magnitude; d <= magnitude; d++)
    for (m = -magnitude; m <= magnitude; m++)
    for (q = -magnitude; q <= magnitude; q++) {
        /* at least one coord must be odd */
        if ((t & 1) == 0 && (d & 1) == 0 && (m & 1) == 0 && (q & 1) == 0)
            continue;

        /* must be at this magnitude (not a lower one) */
        int maxabs = abs(t);
        if (abs(d) > maxabs) maxabs = abs(d);
        if (abs(m) > maxabs) maxabs = abs(m);
        if (abs(q) > maxabs) maxabs = abs(q);
        if (maxabs != magnitude) continue;

        print_bond(t, d, m, q);
        count++;
    }

    printf("# %d bonds\n\n", count);
}

/* ══════════════════════════════════════════════════════════
 * MAIN
 * ══════════════════════════════════════════════════════════ */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s --degree N [k]    # atoms at degree N\n", argv[0]);
        fprintf(stderr, "  %s --bonds N         # bonds at magnitude N\n", argv[0]);
        fprintf(stderr, "  %s --all             # atoms (0-4 full, 5-21 2dim) + bonds (1,3)\n", argv[0]);
        fprintf(stderr, "  %s T D M Q           # single atom\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--degree") == 0 && argc >= 3) {
        int deg = atoi(argv[2]);
        int max_k = (argc >= 4) ? atoi(argv[3]) : 0;
        gen_atoms(deg, max_k);
    }
    else if (strcmp(argv[1], "--bonds") == 0 && argc >= 3) {
        int mag = atoi(argv[2]);
        gen_bonds(mag);
    }
    else if (strcmp(argv[1], "--all") == 0) {
        /* atoms: degrees 0-4 full, 5-40 2-dim only */
        int deg;
        for (deg = 0; deg <= 4; deg++)
            gen_atoms(deg, 0);
        for (deg = 5; deg <= 21; deg++)
            gen_atoms(deg, 2);

        /* bonds: magnitude 1 (fundamental) and 3 (compound) */
        gen_bonds(1);
        gen_bonds(3);
    }
    else if (argc >= 5) {
        int t = atoi(argv[1]);
        int d = atoi(argv[2]);
        int m = atoi(argv[3]);
        int q = atoi(argv[4]);
        print_atom(t, d, m, q);
    }
    else {
        fprintf(stderr, "Bad args\n");
        return 1;
    }

    return 0;
}
