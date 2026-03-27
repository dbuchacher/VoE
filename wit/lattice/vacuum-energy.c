/*
 * vacuum-energy.c — lattice derivation: vacuum pair production
 *
 * The vacuum isn't empty. 95% of the universe's energy is unobserved:
 *   dark energy (68%) = empty buffer slots with negative pressure
 *   dark matter (27%) = written but unread entries
 *   visible     (5%)  = read entries
 *
 * Pair production from vacuum = reading an unread entry.
 * The Schwinger mechanism does this: intense field → pairs from vacuum.
 * The Casimir effect proves vacuum energy does real work.
 *
 * Question: what geometry makes vacuum → pairs efficient?
 *
 * Lattice coordinates:
 *   vacuum density    M/D³     (0,-6,+2,0)    ~6e-10 J/m³
 *   pair threshold    D²M/T²   (-4,+4,+2,0)   1.022 MeV
 *   Schwinger field   DM/(T²Q) (-4,+2,+2,-2)  1.3e18 V/m
 *   Casimir force     M/(DT²)  (-4,-2,+2,0)   scales as 1/D⁴
 *   pressure          M/(DT²)  (-4,+2,+2,0)   field pressure
 */

#include <stdio.h>
#include <math.h>

/* constants */
#define CC          2.998e8     /* speed of light m/s */
#define C2          8.988e16    /* c² */
#define HBAR        1.055e-34   /* reduced Planck */
#define E_CHARGE    1.602e-19   /* elementary charge */
#define ME          9.109e-31   /* electron mass kg */
#define EPS0        8.854e-12   /* vacuum permittivity */
#define LAMBDA_C    3.861e-13   /* Compton wavelength ℏ/(m_e c) meters */
#define PI          3.14159265
#define EV_TO_J     1.602e-19
#define MEV_TO_J    1.602e-13

/* pair production threshold */
#define E_PAIR_J    (2.0 * ME * C2)        /* 1.637e-13 J = 1.022 MeV */
#define E_PAIR_MEV  1.022

/* Schwinger critical field: E_s = m²c³ / (eℏ) */
#define E_SCHWINGER (ME * ME * CC * C2 / (E_CHARGE * HBAR))
/* ~1.32e18 V/m */

/* vacuum energy density (cosmological, measured) */
#define RHO_VAC     5.96e-10    /* J/m³ */

/* Casimir energy density between plates at separation d (meters) */
/* E/V = -π²ℏc / (720 d⁴) per unit area × 1/d for volume */
static double casimir_energy_density(double d) {
    return PI * PI * HBAR * CC / (720.0 * d * d * d * d);
}

/* Casimir pressure between plates at separation d */
static double casimir_pressure(double d) {
    return PI * PI * HBAR * CC / (240.0 * d * d * d * d);
}

/* electric field at distance r from a nucleus of charge Z */
static double coulomb_field(int Z, double r) {
    return Z * E_CHARGE / (4.0 * PI * EPS0 * r * r);
}

/* nuclear radius ≈ 1.2 × A^(1/3) fm, A ≈ 2.5Z for heavy nuclei */
static double nuclear_radius(int Z) {
    double A = 2.5 * Z;
    return 1.2e-15 * pow(A, 1.0/3.0);
}

/* supercritical Z: field at COMPTON WAVELENGTH distance must exceed Schwinger.
 * pair production needs the field supercritical over λ_c = 386 fm,
 * not just at the nuclear surface. this is why Z~137 (= 1/α), not Z~1 */
static double field_at_compton(int Z) {
    return coulomb_field(Z, LAMBDA_C);
}

static double field_at_surface(int Z) {
    return coulomb_field(Z, nuclear_radius(Z));
}

typedef struct {
    const char *name;
    double spacing_m;       /* structure feature size */
    double energy_density;  /* vacuum energy at that scale J/m³ */
    double pairs_per_m3_s;  /* estimated pair production rate */
    double power_per_m3;    /* power output W/m³ */
} geometry_t;

int main(void) {
    printf("vacuum energy — lattice derivation\n");
    printf("can geometry extract pair-production energy from vacuum?\n\n");

    /* ─── fundamental thresholds ─── */
    printf("=== THRESHOLDS ===\n\n");
    printf("  pair production:    %.3f MeV (%.3e J)\n", E_PAIR_MEV, E_PAIR_J);
    printf("  Schwinger field:    %.3e V/m\n", E_SCHWINGER);
    printf("  vacuum energy:      %.3e J/m³ (cosmological)\n", RHO_VAC);
    printf("  volume for 1 pair:  %.3e m³ (vacuum energy alone)\n",
           E_PAIR_J / RHO_VAC);
    printf("  cube side:          %.1f km\n",
           pow(E_PAIR_J / RHO_VAC, 1.0/3.0) / 1000);

    printf("\n  cosmological vacuum: 1 pair per (%.0f km)³ — useless at human scale\n",
           pow(E_PAIR_J / RHO_VAC, 1.0/3.0) / 1000);

    /* ─── Casimir at various scales ─── */
    printf("\n=== CASIMIR ENERGY DENSITY vs SCALE ===\n\n");
    printf("  gap          energy density    ratio to vacuum    pairs possible?\n");
    printf("  ───          ──────────────    ───────────────    ──────────────\n");

    double gaps[] = {1e-3, 1e-6, 1e-9, 1e-10, 1e-12, 1e-15};
    const char *labels[] = {"1 mm", "1 μm", "1 nm", "1 Å", "1 pm", "1 fm"};

    for (int i = 0; i < 6; i++) {
        double e = casimir_energy_density(gaps[i]);
        double ratio = e / RHO_VAC;
        int pairs = (e > E_PAIR_J / (gaps[i] * gaps[i] * gaps[i]));
        printf("  %-6s       %12.3e J/m³  %12.3e×      %s\n",
               labels[i], e, ratio,
               pairs ? "YES — exceeds threshold" : "no");
    }

    printf("\n  at 1 fm (nuclear scale), Casimir energy density exceeds pair threshold\n");
    printf("  this is where the vacuum becomes a usable reservoir\n");

    /* ─── Casimir power extraction ─── */
    printf("\n=== CASIMIR POWER (oscillating plates) ===\n\n");
    printf("  concept: plates oscillate between d_min and d_max\n");
    printf("  work per cycle = Casimir pressure × displacement × area\n\n");

    double d_min = 10e-9;   /* 10 nm minimum gap */
    double d_max = 100e-9;  /* 100 nm maximum gap */
    double area = 1.0;      /* 1 m² plate area */

    double p_min = casimir_pressure(d_min);
    double p_max = casimir_pressure(d_max);
    double work_per_cycle = (p_min - p_max) * (d_max - d_min) * area;

    printf("  plates: 1 m² area, oscillating 10-100 nm gap\n");
    printf("  pressure at 10 nm:   %12.3e Pa\n", p_min);
    printf("  pressure at 100 nm:  %12.3e Pa\n", p_max);
    printf("  work per cycle:      %12.3e J\n", work_per_cycle);
    printf("  at 1 GHz oscillation: %.3e W = %.3f μW\n",
           work_per_cycle * 1e9, work_per_cycle * 1e9 * 1e6);
    printf("\n  verdict: microwatts per m². not useful for power.\n");

    /* ─── supercritical nuclei ─── */
    printf("\n=== SUPERCRITICAL FIELD — nuclear Z scan ===\n\n");
    printf("  Schwinger: spontaneous pairs when E > %.3e V/m\n", E_SCHWINGER);
    printf("  question: at what Z does the nuclear surface field exceed this?\n\n");
    printf("  Z     element      field at λ_c (V/m)     ratio to Schwinger\n");
    printf("  ──    ───────      ──────────────────     ──────────────────\n");

    int zvals[] = {26, 50, 79, 92, 100, 118, 137, 150, 173, 200};
    const char *elems[] = {"Fe", "Sn", "Au", "U", "Fm", "Og", "?", "?", "?", "?"};

    int supercrit_z = 0;
    for (int i = 0; i < 10; i++) {
        int z = zvals[i];
        double e_field = field_at_compton(z);
        double ratio = e_field / E_SCHWINGER;
        printf("  %-4d  %-8s     %8.2e V/m      %.4f%s\n",
               z, elems[i], e_field, ratio,
               ratio >= 1.0 ? "  ← SUPERCRITICAL" : "");
        if (ratio >= 1.0 && supercrit_z == 0) supercrit_z = z;
    }

    /* ─── binary search for exact supercritical Z ─── */
    int z_lo = 100, z_hi = 250;
    while (z_hi - z_lo > 1) {
        int z_mid = (z_lo + z_hi) / 2;
        double ratio = field_at_compton(z_mid) / E_SCHWINGER;
        if (ratio >= 1.0) z_hi = z_mid;
        else z_lo = z_mid;
    }

    printf("\n  supercritical threshold: Z = %d\n", z_hi);
    printf("  at Z=%d, nuclear surface field exceeds Schwinger limit\n", z_hi);
    printf("  spontaneous pair production from vacuum begins\n");
    printf("  no external energy needed — field IS the energy source\n");

    /* ─── effective Z from crystal arrangement ─── */
    printf("\n=== EFFECTIVE SUPERCRITICAL Z FROM GEOMETRY ===\n\n");
    printf("  can't make element %d. but can ARRANGE existing nuclei.\n\n", z_hi);
    printf("  two uranium nuclei (Z=92) in collision:\n");
    printf("    combined Z = 184 (above threshold %d)\n", z_hi);
    printf("    this is EXACTLY what heavy-ion colliders do\n");
    printf("    GSI Darmstadt: U+U collisions → spontaneous pair production\n");
    printf("    observed: positron peaks at predicted energies\n\n");

    printf("  static approach (no collision):\n");
    printf("    uranium crystal lattice, spacing ~0.3 nm\n");
    printf("    at nuclear separation (~10 fm), fields superpose\n");
    printf("    need nuclei within ~20 fm to get effective Z > %d\n", z_hi);
    printf("    this means nuclear contact — which is fusion/fission territory\n\n");

    double combined_field = field_at_compton(184); /* Z_eff = 184 at λ_c */
    printf("    U+U combined: Z_eff=184, field at λ_c = %.3e V/m\n", combined_field);
    printf("    ratio to Schwinger: %.2f ← %s\n",
           combined_field / E_SCHWINGER,
           combined_field / E_SCHWINGER >= 1.0 ? "SUPERCRITICAL" : "subcritical");

    /* ─── the template ─── */
    printf("\n  ═══════════════════════════════════════════════════\n");
    printf("  THE TEMPLATE — vacuum pair production\n");
    printf("  ═══════════════════════════════════════════════════\n\n");

    printf("  the vacuum IS the energy reservoir (95%% of cosmic energy)\n");
    printf("  accessing it requires supercritical field: E > %.2e V/m\n\n", E_SCHWINGER);

    printf("  three paths to supercritical:\n\n");

    printf("  1. heavy-ion collision (Z₁+Z₂ > %d)\n", z_hi);
    printf("     U+U → Z_eff=184 → spontaneous pairs\n");
    printf("     demonstrated at GSI. pairs observed.\n");
    printf("     problem: instantaneous, not sustained\n\n");

    printf("  2. laser (Schwinger mechanism)\n");
    printf("     need: %.2e V/m focal field\n", E_SCHWINGER);
    printf("     current best: ~1e15 V/m (factor 1000 short)\n");
    printf("     laser intensity doubling: ~10x per decade\n");
    printf("     estimated: ~2050-2060 for Schwinger threshold\n\n");

    printf("  3. metamaterial cavity (the real template)\n");
    printf("     nanostructured high-Z material creates\n");
    printf("     standing-wave field enhancement in cavity\n");
    printf("     effective field = cavity_Q × input_field\n");
    printf("     need Q ~ 1000 with heavy-nucleus cavity walls\n");
    printf("     candidate: uranium or tungsten photonic crystal\n");
    printf("     cavity spacing: ~%.1f nm (half-wavelength of 1 MeV photon)\n",
           0.5 * 1.24e-12 / 1e-9);
    printf("     THIS is the pore-diameter equivalent for vacuum energy\n\n");

    printf("  lattice bonds:\n");
    printf("    W bond: cavity geometry = field enhancement (passive)\n");
    printf("    R bond: standing wave = sustained oscillation\n");
    printf("    P bond: pair extraction from cavity\n");
    printf("    C bond: annihilation target (energy harvest)\n\n");

    double cavity_q = 1000;
    double input_field = 1e15;  /* achievable with current lasers */
    double enhanced = cavity_q * input_field;
    printf("  numbers:\n");
    printf("    current laser field:     %.0e V/m\n", input_field);
    printf("    cavity Q needed:         %.0f\n", E_SCHWINGER / input_field);
    printf("    cavity Q demonstrated:   ~10⁶ (optical cavities)\n");
    printf("    enhanced field:          %.0e V/m\n", enhanced);
    printf("    Schwinger threshold:     %.2e V/m\n", E_SCHWINGER);
    printf("    ratio:                   %.1f — %s\n",
           enhanced / E_SCHWINGER,
           enhanced >= E_SCHWINGER ? "ABOVE THRESHOLD" : "below");
    printf("\n");
    printf("  if Q=%.0f cavity works: laser energy in → pairs from vacuum → annihilation out\n",
           E_SCHWINGER / input_field);
    printf("  the cavity provides field enhancement (passive W bond)\n");
    printf("  the vacuum provides the mass-energy (reservoir)\n");
    printf("  net energy = annihilation energy - laser energy\n");
    printf("  positive if cavity Q > Schwinger_field / laser_field = %.0f\n",
           E_SCHWINGER / input_field);

    return 0;
}
