/*
 * pore.c — lattice derivation: pore diameter → chain length
 *
 * The formula (from dimensional analysis):
 *   D_pore = W_0 + beta * sqrt(N) * l_bond
 *   D_pore = 350 + 63 * sqrt(N)     (picometers)
 *
 * Integer form (no floats, lattice-native):
 *   (D_pore - 350)^2 = 3969 * N
 *   D_pore = 350 + isqrt(3969 * N)
 *
 * Lattice coordinates used:
 *   diffusion   D^2/T  (-2,+4,0,0)   chain fills sqrt(N) space
 *   stiffness   M/T^2  (-4,0,+2,0)   C-C bond = 346 kJ/mol
 *   energy      D^2M/T^2             thermal drive
 *   density     M/D^3                catalyst pore density
 *
 * Usage:
 *   ./pore              show all fuel targets
 *   ./pore N            pore diameter for chain length N
 *   ./pore --reverse D  chain length for pore diameter D (pm)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* integer square root via Newton's method — lattice bind bond */
static long isqrt(long n) {
    if (n <= 0) return 0;
    long x = n;
    long y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

/* constants in picometers (integer, no floats) */
#define W0      350     /* van der Waals backbone width (pm) */
#define BL      63      /* beta * l_bond = 0.5 * 126 pm */
#define BL2     3969    /* BL^2 = 63^2 */

/* C-C bond energy in kJ/mol */
#define BOND_E  346

/* pore diameter for chain length N (picometers) */
static long pore_for_chain(int n) {
    return W0 + isqrt((long)BL2 * n);
}

/* max chain length for pore diameter D (picometers) */
static int chain_for_pore(long d_pm) {
    long diff = d_pm - W0;
    if (diff <= 0) return 0;
    return (int)((diff * diff) / BL2);
}

/* thermal stability: bond_energy / kT at temperature (Kelvin) */
static int stability_ratio(int temp_k) {
    /* kT in J/mol = 8.314 * T (gas constant * temp) */
    /* bond = 346000 J/mol */
    /* ratio = 346000 / (8.314 * T) */
    /* integer: ratio = 346000 * 1000 / (8314 * T) */
    return (int)(346000000L / (8314L * temp_k));
}

typedef struct {
    const char *name;
    int n;              /* carbon count */
    const char *use;
} fuel_t;

static fuel_t fuels[] = {
    {"methane",     1,  "natural gas, heating"},
    {"ethane",      2,  "petrochemical feedstock"},
    {"propane",     3,  "LPG, cooking"},
    {"butane",      4,  "lighter fuel, LPG blend"},
    {"pentane",     5,  "gasoline component"},
    {"hexane",      6,  "solvent, gasoline"},
    {"heptane",     7,  "gasoline standard"},
    {"octane",      8,  "gasoline (target)"},
    {"nonane",      9,  "gasoline/jet overlap"},
    {"decane",     10,  "jet fuel component"},
    {"undecane",   11,  "jet fuel / kerosene"},
    {"dodecane",   12,  "jet fuel (Jet-A target)"},
    {"tridecane",  13,  "diesel / kerosene"},
    {"tetradecane",14,  "diesel component"},
    {"hexadecane", 16,  "diesel (cetane target)"},
    {"eicosane",   20,  "heavy diesel / lubricant"},
    {"triacontane",30,  "paraffin wax"},
    {"pentacontane",50, "heavy wax"},
};

static const int n_fuels = sizeof(fuels) / sizeof(fuels[0]);

typedef struct {
    const char *name;
    long pore_pm;       /* pore diameter in picometers */
    const char *type;
} zeolite_t;

static zeolite_t zeolites[] = {
    {"SAPO-34",     380,  "silicoaluminophosphate"},
    {"ZSM-22",      460,  "zeolite (TON)"},
    {"ZSM-5",       550,  "zeolite (MFI)"},
    {"Beta",        670,  "zeolite (BEA)"},
    {"mordenite",   700,  "zeolite (MOR)"},
    {"MCM-41",     2000,  "mesoporous silica"},
    {"SBA-15",     6000,  "mesoporous silica"},
};

static const int n_zeolites = sizeof(zeolites) / sizeof(zeolites[0]);

static void print_header(void) {
    printf("  N   name            D_pore(pm)  D_pore(nm)  use\n");
    printf("  ──  ──────────────  ──────────  ──────────  ───────────────────\n");
}

static void print_fuel(fuel_t *f) {
    long d = pore_for_chain(f->n);
    printf("  %-3d %-15s %6ld pm    %ld.%02ld nm   %s\n",
           f->n, f->name, d, d / 1000, (d % 1000) / 10, f->use);
}

static void print_zeolites(void) {
    printf("\n  known zeolites (verification):\n");
    printf("  name        pore(pm)  predicted N_max  actual product\n");
    printf("  ────────    ────────  ──────────────   ──────────────\n");
    for (int i = 0; i < n_zeolites; i++) {
        zeolite_t *z = &zeolites[i];
        int n = chain_for_pore(z->pore_pm);
        printf("  %-10s  %5ld pm   C%-3d             ", z->name, z->pore_pm, n);
        if (n <= 3) printf("light olefins (C2-C3)\n");
        else if (n <= 10) printf("gasoline range\n");
        else if (n <= 20) printf("diesel range\n");
        else printf("no constraint (broad F-T)\n");
    }
}

static void print_stability(void) {
    printf("\n  thermal stability (C-C bond = %d kJ/mol):\n", BOND_E);
    printf("  temp(K)  temp(C)  bond/kT  status\n");
    printf("  ──────   ──────   ───────  ──────\n");
    int temps[] = {300, 400, 500, 600, 800, 1000, 2000, 4000};
    for (int i = 0; i < 8; i++) {
        int t = temps[i];
        int r = stability_ratio(t);
        printf("  %5d    %5d    %5dx   %s\n", t, t - 273, r,
               r > 50 ? "stable" : r > 10 ? "reactive" :
               r > 2 ? "cracking" : "decomposing");
    }
}

static void print_gap(void) {
    printf("\n  THE GAP — no commercial catalyst tuned for:\n");
    printf("  ────────────────────────────────────────────\n");
    printf("  jet fuel (C12):  need %ld pm = %ld.%02ld nm pore\n",
           pore_for_chain(12), pore_for_chain(12)/1000,
           (pore_for_chain(12)%1000)/10);
    printf("  diesel (C16):    need %ld pm = %ld.%02ld nm pore\n",
           pore_for_chain(16), pore_for_chain(16)/1000,
           (pore_for_chain(16)%1000)/10);
    printf("  ZSM-5 = 550 pm (gasoline), Beta = 670 pm (too wide)\n");
    printf("  solution: MOF with tunable linker at target diameter\n");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        /* show everything */
        printf("pore diameter for hydrocarbon chain length\n");
        printf("D = %d + %d * sqrt(N)  (picometers)\n\n", W0, BL);
        print_header();
        for (int i = 0; i < n_fuels; i++)
            print_fuel(&fuels[i]);
        print_zeolites();
        print_stability();
        print_gap();
        return 0;
    }

    if (strcmp(argv[1], "--reverse") == 0 && argc == 3) {
        long d = atol(argv[2]);
        int n = chain_for_pore(d);
        printf("pore %ld pm → max chain C%d\n", d, n);
        return 0;
    }

    /* single chain length */
    int n = atoi(argv[1]);
    if (n > 0) {
        long d = pore_for_chain(n);
        printf("C%d → %ld pm (%ld.%02ld nm)\n", n, d, d/1000, (d%1000)/10);
    }

    return 0;
}
