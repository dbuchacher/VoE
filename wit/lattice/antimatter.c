/*
 * antimatter.c — lattice derivation: optimal positron production
 *
 * Pair production: photon (≥1.022 MeV) + nucleus → e⁻ + e⁺
 * The nucleus is the W bond (catalyst). Material geometry is the template.
 *
 * Cascade: electron beam → bremsstrahlung → pair production → repeat
 * Each generation doubles particles until energy < 1.022 MeV (critical energy)
 * N_pairs ≈ E_beam / E_critical — the R bond (iteration count)
 *
 * The "zeolite" for antimatter: target material + thickness + beam energy.
 * Pre-computed. O(1) per positron after that.
 *
 * Lattice coordinates:
 *   energy     D²M/T²  (-4,+4,+2,0)   beam energy, pair threshold
 *   stiffness  M/T²    (-4,0,+2,0)     critical energy (cascade stop)
 *   density    M/D³    (0,-6,+2,0)     target density
 *   bandwidth  M/T     (-2,0,+2,0)     production rate (mass per time)
 *   power      D²M/T³  (-6,+4,+2,0)   beam power
 *
 * Usage:
 *   ./antimatter                all targets + production analysis
 *   ./antimatter --beam E_MeV   yield for specific beam energy
 *   ./antimatter --goal mass_ug time to produce mass_ug micrograms
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* positron mass in kg */
#define M_E         9.109e-31
/* particles per gram of positrons */
#define PER_GRAM    1.098e27
/* pair production threshold MeV */
#define THRESHOLD   1.022
/* fine structure constant */
#define ALPHA_INV   137
/* classical electron radius (fm) */
#define R_E_FM      2.818

typedef struct {
    const char *name;
    const char *symbol;
    int Z;              /* atomic number */
    int A;              /* mass number */
    double rho;         /* density g/cm³ */
    double E_crit;      /* critical energy MeV */
    double X0;          /* radiation length g/cm² */
} target_t;

static target_t targets[] = {
    {"tungsten",    "W",  74, 184, 19.25,  7.97,  6.76},
    {"iridium",     "Ir", 77, 192, 22.56,  7.36,  6.46},
    {"osmium",      "Os", 76, 190, 22.59,  7.49,  6.54},
    {"gold",        "Au", 79, 197, 19.32,  7.11,  6.46},
    {"lead",        "Pb", 82, 207, 11.34,  7.43,  6.37},
    {"uranium",     "U",  92, 238, 19.05,  6.65,  6.00},
    {"platinum",    "Pt", 78, 195, 21.45,  7.23,  6.54},
    {"rhenium",     "Re", 75, 186, 21.02,  7.67,  6.65},
    {"bismuth",     "Bi", 83, 209, 9.78,   7.38,  6.32},
    {"tantalum",    "Ta", 73, 181, 16.65,  8.10,  6.82},
};
static const int n_targets = sizeof(targets) / sizeof(targets[0]);

/* figure of merit: Z² × ρ / A — pair production rate per volume */
static double fom(target_t *t) {
    return (double)(t->Z * t->Z) * t->rho / t->A;
}

/* optimal target thickness in mm (1 radiation length) */
static double thickness_mm(target_t *t) {
    return 10.0 * t->X0 / t->rho;  /* X0/ρ in cm, ×10 for mm */
}

/* positron yield per beam electron at given beam energy */
static double yield(target_t *t, double E_beam_MeV) {
    if (E_beam_MeV < THRESHOLD) return 0;
    /* cascade: N ≈ E / E_crit, times conversion efficiency ~0.5 */
    return 0.5 * E_beam_MeV / t->E_crit;
}

/* production rate: positrons per second at given beam power */
static double rate(target_t *t, double E_beam_MeV, double beam_power_W) {
    /* electrons per second = power / (energy per electron) */
    double E_joules = E_beam_MeV * 1.602e-13;
    double electrons_per_sec = beam_power_W / E_joules;
    return electrons_per_sec * yield(t, E_beam_MeV);
}

/* time to produce mass_grams at given rate (seconds) */
static double time_for_mass(double rate_per_sec, double mass_grams) {
    double n_needed = mass_grams * PER_GRAM;
    return n_needed / rate_per_sec;
}

static void print_targets(void) {
    printf("  target material ranking (pair production figure of merit):\n\n");
    printf("  material    Z    ρ(g/cm³)  FOM(Z²ρ/A)  thickness  E_crit\n");
    printf("  ────────    ──   ────────  ──────────  ─────────  ──────\n");

    /* sort by FOM (simple bubble) */
    int idx[20];
    for (int i = 0; i < n_targets; i++) idx[i] = i;
    for (int i = 0; i < n_targets-1; i++)
        for (int j = i+1; j < n_targets; j++)
            if (fom(&targets[idx[i]]) < fom(&targets[idx[j]])) {
                int tmp = idx[i]; idx[i] = idx[j]; idx[j] = tmp;
            }

    for (int i = 0; i < n_targets; i++) {
        target_t *t = &targets[idx[i]];
        printf("  %-10s  %2d   %6.2f    %7.1f     %4.1f mm    %.2f MeV\n",
               t->name, t->Z, t->rho, fom(t),
               thickness_mm(t), t->E_crit);
    }
}

static void print_yield(target_t *t) {
    printf("\n  yield curve for %s (Z=%d):\n\n", t->name, t->Z);
    printf("  beam(MeV)  e⁺/electron  cascade generations\n");
    printf("  ─────────  ───────────  ───────────────────\n");
    double energies[] = {1, 2, 5, 10, 25, 50, 100, 250, 500, 1000};
    for (int i = 0; i < 10; i++) {
        double e = energies[i];
        double y = yield(t, e);
        int gens = (int)(e > t->E_crit ? e / t->E_crit : 0);
        if (y > 0)
            printf("  %7.0f    %8.1f      %d R-iterations\n", e, y, gens);
        else
            printf("  %7.0f    %8s      below threshold\n", e, "—");
    }
}

static void print_production(target_t *t) {
    printf("\n  production rate vs beam power (%s, 100 MeV beam):\n\n", t->name);
    printf("  beam power     e⁺/sec         g/year          time for 1μg\n");
    printf("  ──────────     ──────         ──────          ───────────\n");

    double powers[] = {1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9};
    const char *labels[] = {"1 kW", "10 kW", "100 kW", "1 MW", "10 MW", "100 MW", "1 GW"};

    for (int i = 0; i < 7; i++) {
        double r = rate(t, 100.0, powers[i]);
        double grams_per_year = r * 3.156e7 / PER_GRAM;
        double time_1ug = time_for_mass(r, 1e-6);

        if (time_1ug < 60)
            printf("  %-10s   %10.2e     %10.2e g    %.1f sec\n",
                   labels[i], r, grams_per_year, time_1ug);
        else if (time_1ug < 3600)
            printf("  %-10s   %10.2e     %10.2e g    %.1f min\n",
                   labels[i], r, grams_per_year, time_1ug/60);
        else if (time_1ug < 86400)
            printf("  %-10s   %10.2e     %10.2e g    %.1f hrs\n",
                   labels[i], r, grams_per_year, time_1ug/3600);
        else if (time_1ug < 3.156e7)
            printf("  %-10s   %10.2e     %10.2e g    %.1f days\n",
                   labels[i], r, grams_per_year, time_1ug/86400);
        else
            printf("  %-10s   %10.2e     %10.2e g    %.1f years\n",
                   labels[i], r, grams_per_year, time_1ug/3.156e7);
    }
}

static void print_energy_comparison(void) {
    printf("\n  energy density comparison (per kg of fuel):\n\n");
    printf("  source              MJ/kg           kg for a 777 flight\n");
    printf("  ──────              ─────           ───────────────────\n");
    /* 777 long haul ~ 100 tons jet fuel × 43 MJ/kg = 4.3 TJ */
    double flight_J = 4.3e12;
    struct { const char *name; double mj_per_kg; } sources[] = {
        {"Li-ion battery",     0.9},
        {"jet fuel (C12)",     43},
        {"hydrogen gas",       142},
        {"fission (U-235)",    82e6},
        {"fusion (D-T)",       340e6},
        {"antimatter (e⁺e⁻)",  9e10},
    };
    for (int i = 0; i < 6; i++) {
        double kg = flight_J / (sources[i].mj_per_kg * 1e6);
        if (kg >= 1.0)
            printf("  %-22s %12.1f    %12.1f kg\n",
                   sources[i].name, sources[i].mj_per_kg, kg);
        else if (kg >= 1e-3)
            printf("  %-22s %12.1f    %12.3f g\n",
                   sources[i].name, sources[i].mj_per_kg, kg * 1000);
        else
            printf("  %-22s %12.1f    %12.3f mg\n",
                   sources[i].name, sources[i].mj_per_kg, kg * 1e6);
    }
}

static void print_template(void) {
    /* find best target */
    int best = 0;
    for (int i = 1; i < n_targets; i++)
        if (fom(&targets[i]) > fom(&targets[best])) best = i;
    target_t *t = &targets[best];

    printf("\n  ═══════════════════════════════════════════════════\n");
    printf("  THE TEMPLATE — pre-computed optimal configuration\n");
    printf("  ═══════════════════════════════════════════════════\n\n");
    printf("  target:     %s (Z=%d, FOM=%.1f)\n", t->name, t->Z, fom(t));
    printf("  thickness:  %.1f mm (1 radiation length)\n", thickness_mm(t));
    printf("  beam:       100 MeV electrons\n");
    printf("  yield:      %.1f positrons per beam electron\n", yield(t, 100));
    printf("\n");
    printf("  for 1 μg antimatter (enough for ~90 MJ = 2 liters jet fuel):\n");
    double r_1mw = rate(t, 100, 1e6);
    double t_1ug = time_for_mass(r_1mw, 1e-6);
    printf("    at 1 MW beam:  %.1f days\n", t_1ug / 86400);
    double r_1gw = rate(t, 100, 1e9);
    double t_1ug_gw = time_for_mass(r_1gw, 1e-6);
    printf("    at 1 GW beam:  %.0f seconds\n", t_1ug_gw);
    printf("\n");
    printf("  for 1 g antimatter (enough for ~90 TJ = 2M liters jet fuel):\n");
    double t_1g_gw = time_for_mass(r_1gw, 1.0);
    printf("    at 1 GW beam:  %.1f years\n", t_1g_gw / 3.156e7);
    printf("\n");
    printf("  lattice bonds:\n");
    printf("    R bond:   cascade iterations = E_beam / E_crit = %.0f\n",
           100.0 / t->E_crit);
    printf("    W bond:   critical energy = %.2f MeV (cascade stop)\n", t->E_crit);
    printf("    P bond:   magnetic collection (transport positrons out)\n");
    printf("    C bond:   Penning trap accumulation (synthesis/storage)\n");
    printf("\n");
    printf("  the target geometry IS the pre-computed template.\n");
    printf("  %s, %.1f mm thick, 100 MeV beam = O(1) per positron.\n",
           t->name, thickness_mm(t));
    printf("  scaling is pure beam power. 1 GW = 1 μg per %.0f sec.\n",
           t_1ug_gw);
}

int main(int argc, char **argv) {
    if (argc >= 3 && strcmp(argv[1], "--beam") == 0) {
        double e = atof(argv[2]);
        target_t *t = &targets[0]; /* tungsten default */
        printf("beam %.0f MeV on %s: %.1f e⁺/electron\n",
               e, t->name, yield(t, e));
        return 0;
    }

    if (argc >= 3 && strcmp(argv[1], "--goal") == 0) {
        double ug = atof(argv[2]);
        target_t *t = &targets[0];
        double r = rate(t, 100, 1e9); /* 1 GW beam */
        double secs = time_for_mass(r, ug * 1e-6);
        printf("%.1f μg on %s at 1 GW: %.1f seconds\n",
               ug, t->name, secs);
        return 0;
    }

    printf("antimatter production — lattice derivation\n");
    printf("pair production: photon + nucleus → e⁻ + e⁺\n");
    printf("cascade: beam → bremsstrahlung → pairs → repeat (R bond)\n");
    printf("threshold: %.3f MeV (2 × electron mass × c²)\n\n", THRESHOLD);

    print_targets();

    /* use best target for detailed analysis */
    int best = 0;
    for (int i = 1; i < n_targets; i++)
        if (fom(&targets[i]) > fom(&targets[best])) best = i;

    print_yield(&targets[best]);
    print_production(&targets[best]);
    print_energy_comparison();
    print_template();

    return 0;
}
