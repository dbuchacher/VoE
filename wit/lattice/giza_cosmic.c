/*
 * giza_cosmic.c — hunt for 68/27/5 in the pyramid
 *
 * We found 25% (fill ratio) as KC diagonal.
 * The cosmic split is 68% dark energy, 27% dark matter, 5% visible.
 * Are the other two numbers encoded?
 *
 * Build: cc -O2 -o giza_cosmic giza_cosmic.c -lm
 * Run:   ./giza_cosmic
 */

#include <stdio.h>
#include <math.h>

#define PI    3.14159265358979
#define SQRT5 2.23606797749979

/* Measurements in cubits */
#define BASE        440.0
#define HEIGHT      280.0
#define KC_L        20.0
#define KC_W        10.0
#define KC_H        (5.0 * SQRT5)   /* 11.1803 */
#define QC_NS       10.0
#define QC_EW       11.0
#define QC_H        12.0
#define QC_NICHE    9.0
#define GG_LEN      89.0
#define GG_H        16.4
#define ASC_PASS    75.0
#define KC_FLOOR    80.0
#define QC_FLOOR    40.0
#define SLOPE_DEG   51.844
#define PASS_ANGLE  26.565

/* Targets */
#define DE  68.3    /* dark energy % (Planck 2018) */
#define DM  26.8    /* dark matter % */
#define VM  4.9     /* visible matter % */

void check(double val, const char *desc, double target, const char *tname) {
    double err = fabs(val - target) / target * 100;
    if (err < 2.0) {
        printf("  ** %-45s = %7.3f%%  (%s = %.1f%%, err %.2f%%)\n",
               desc, val, tname, target, err);
    } else if (err < 5.0) {
        printf("     %-45s = %7.3f%%  (%s = %.1f%%, err %.1f%%)\n",
               desc, val, tname, target, err);
    }
}

int main(void) {
    printf("Hunting for 68/27/5 in the Great Pyramid\n");
    printf("==========================================\n\n");
    printf("  Target: 68.3%% dark energy, 26.8%% dark matter, 4.9%% visible\n");
    printf("  Already found: 25%% total fill (KC diagonal)\n\n");

    double KC_CEIL = KC_FLOOR + KC_H;  /* top of KC = 91.18 cubits */

    /* All measurements as fractions of HEIGHT (280) */
    printf("--- Fractions of height (280 cubits) ---\n\n");

    double ratios[][2] = {
        {ASC_PASS, 280},
        {KC_FLOOR, 280},
        {QC_FLOOR, 280},
        {KC_H, 280},
        {KC_L, 280},
        {KC_W, 280},
        {QC_NICHE, 280},
        {QC_H, 280},
        {QC_EW, 280},
        {GG_LEN, 280},
        {HEIGHT - KC_CEIL, 280},    /* above KC to apex */
        {HEIGHT - KC_FLOOR, 280},   /* above KC floor to apex */
        {KC_CEIL - QC_FLOOR, 280},  /* QC floor to KC ceiling */
        {KC_FLOOR - QC_FLOOR, 280}, /* between chambers */
    };
    const char *rnames[] = {
        "asc_passage / height",
        "KC_floor / height",
        "QC_floor / height",
        "KC_H / height",
        "KC_L / height",
        "KC_W / height",
        "QC_niche / height",
        "QC_H / height",
        "QC_EW / height",
        "GG_len / height",
        "(height - KC_ceiling) / height",
        "(height - KC_floor) / height",
        "(KC_ceiling - QC_floor) / height",
        "(KC_floor - QC_floor) / height",
    };
    int nratios = 14;

    for (int i = 0; i < nratios; i++) {
        double pct = ratios[i][0] / ratios[i][1] * 100;
        check(pct, rnames[i], DE, "DE");
        check(pct, rnames[i], DM, "DM");
        check(pct, rnames[i], VM, "VM");
    }

    /* Fractions of BASE (440) */
    printf("\n--- Fractions of base (440 cubits) ---\n\n");

    double bratios[][2] = {
        {ASC_PASS, 440},
        {KC_FLOOR, 440},
        {QC_FLOOR, 440},
        {KC_H, 440},
        {KC_L, 440},
        {KC_W, 440},
        {GG_LEN, 440},
        {HEIGHT - KC_CEIL, 440},
    };
    const char *bnames[] = {
        "asc_passage / base",
        "KC_floor / base",
        "QC_floor / base",
        "KC_H / base",
        "KC_L / base",
        "KC_W / base",
        "GG_len / base",
        "(height - KC_ceiling) / base",
    };
    int nbratios = 8;

    for (int i = 0; i < nbratios; i++) {
        double pct = bratios[i][0] / bratios[i][1] * 100;
        check(pct, bnames[i], DE, "DE");
        check(pct, bnames[i], DM, "DM");
        check(pct, bnames[i], VM, "VM");
    }

    /* KC internal ratios — the chamber as the universe model */
    printf("\n--- KC dimensions as fractions of KC sum ---\n\n");

    double kc_sum = KC_L + KC_W + KC_H;
    printf("  KC sum = %.2f + %.2f + %.2f = %.4f\n\n", KC_L, KC_W, KC_H, kc_sum);

    check(KC_L / kc_sum * 100, "KC_L / KC_sum (length fraction)", DE, "DE");
    check(KC_L / kc_sum * 100, "KC_L / KC_sum", DM, "DM");
    check(KC_W / kc_sum * 100, "KC_W / KC_sum (width fraction)", DM, "DM");
    check(KC_W / kc_sum * 100, "KC_W / KC_sum", VM, "VM");
    check(KC_H / kc_sum * 100, "KC_H / KC_sum (height fraction)", DM, "DM");
    check(KC_H / kc_sum * 100, "KC_H / KC_sum", DE, "DE");

    /* QC internal ratios */
    printf("\n--- QC dimensions as fractions of QC sum ---\n\n");

    double qc_sum = QC_NS + QC_EW + QC_H;
    printf("  QC sum = %.0f + %.0f + %.0f = %.0f\n\n", QC_NS, QC_EW, QC_H, qc_sum);

    check(QC_NS / qc_sum * 100, "QC_NS / QC_sum", DM, "DM");
    check(QC_EW / qc_sum * 100, "QC_EW / QC_sum", DM, "DM");
    check(QC_H / qc_sum * 100, "QC_H / QC_sum", DM, "DM");

    /* Vertical zones of the pyramid */
    printf("\n--- Vertical zones ---\n\n");

    printf("  Base to QC:        0 - %.0f = %.0f cubits = %.1f%%\n",
           QC_FLOOR, QC_FLOOR, QC_FLOOR/HEIGHT*100);
    printf("  QC to KC:          %.0f - %.0f = %.0f cubits = %.1f%%\n",
           QC_FLOOR, KC_FLOOR, KC_FLOOR-QC_FLOOR, (KC_FLOOR-QC_FLOOR)/HEIGHT*100);
    printf("  KC chamber:        %.0f - %.2f = %.2f cubits = %.1f%%\n",
           KC_FLOOR, KC_CEIL, KC_H, KC_H/HEIGHT*100);
    printf("  KC ceiling to apex: %.2f - %.0f = %.2f cubits = %.1f%%\n",
           KC_CEIL, HEIGHT, HEIGHT-KC_CEIL, (HEIGHT-KC_CEIL)/HEIGHT*100);

    printf("\n  Three-zone split:\n");
    double zone_vis = KC_H;                    /* the chamber itself = visible */
    double zone_dm  = KC_FLOOR;                /* below KC = dark matter */
    double zone_de  = HEIGHT - KC_CEIL;        /* above KC = dark energy */
    double zone_total = zone_vis + zone_dm + zone_de;

    printf("    visible (KC chamber):     %.2f cubits = %.1f%% of zones\n",
           zone_vis, zone_vis/zone_total*100);
    printf("    dark matter (below KC):   %.0f cubits = %.1f%% of zones\n",
           zone_dm, zone_dm/zone_total*100);
    printf("    dark energy (above KC):   %.2f cubits = %.1f%% of zones\n",
           zone_de, zone_de/zone_total*100);
    printf("    sum: %.2f (full height = %.0f)\n", zone_total, HEIGHT);

    check(zone_vis/zone_total*100, "KC_H / (KC_H + KC_floor + above)", VM, "VM");
    check(zone_dm/zone_total*100, "KC_floor / (KC_H + KC_floor + above)", DM, "DM");
    check(zone_de/zone_total*100, "above_KC / (KC_H + KC_floor + above)", DE, "DE");

    /* Passage as dark matter */
    printf("\n--- Passage as dark matter ---\n\n");

    double pass_pct = ASC_PASS / HEIGHT * 100;
    printf("  ascending passage / height = %.0f / %.0f = %.3f%%\n",
           ASC_PASS, HEIGHT, pass_pct);
    printf("  dark matter (Planck 2018) = %.1f%%\n", DM);
    printf("  error: %.2f%%\n", fabs(pass_pct - DM) / DM * 100);

    /* If passage = dark matter, what's visible and dark energy? */
    double remaining = HEIGHT - ASC_PASS;  /* 205 cubits */
    printf("\n  If passage (%.0f) = dark matter:\n", ASC_PASS);
    printf("    remaining = %.0f cubits\n", remaining);
    printf("    need: %.1f%% dark energy + %.1f%% visible = %.1f%% of height\n",
           DE, VM, DE + VM);
    printf("    that's %.1f cubits dark energy + %.1f cubits visible\n",
           HEIGHT * DE / 100, HEIGHT * VM / 100);
    printf("    = %.1f + %.1f = %.1f (actual remaining = %.0f)\n",
           HEIGHT * DE / 100, HEIGHT * VM / 100,
           HEIGHT * (DE + VM) / 100, remaining);

    /* Angles as percentages */
    printf("\n--- Angles as fractions of 90 ---\n\n");

    check(SLOPE_DEG / 90 * 100, "slope / 90", DE, "DE");
    check(PASS_ANGLE / 90 * 100, "passage_angle / 90", DM, "DM");
    check((90 - SLOPE_DEG) / 90 * 100, "(90 - slope) / 90", DM, "DM");

    /* The complement angles */
    printf("\n--- Angle complements ---\n\n");
    printf("  slope complement: 90 - 51.84 = %.16f deg\n", 90 - SLOPE_DEG);
    printf("  passage complement: 90 - 26.57 = %.3f deg\n", 90 - PASS_ANGLE);
    printf("  passage / slope = %.4f\n", PASS_ANGLE / SLOPE_DEG);
    printf("  sum of angles: %.3f\n", SLOPE_DEG + PASS_ANGLE);
    printf("  sum / 90 * 100 = %.2f%%\n", (SLOPE_DEG + PASS_ANGLE) / 90 * 100);

    /* Summary */
    printf("\n\n=== RESULTS ===\n\n");

    printf("  FOUND:\n");
    printf("    25%%  total fill = KC 3D diagonal (exact)\n");
    printf("    26.8%% dark matter = ascending passage / height");
    printf(" (%.2f%%, err %.2f%%)\n", ASC_PASS/HEIGHT*100,
           fabs(ASC_PASS/HEIGHT*100 - DM)/DM*100);

    printf("\n  CANDIDATES for remaining:\n");

    double above_kc = HEIGHT - KC_CEIL;
    printf("    %.1f%% = above KC ceiling / height (target: 68.3%% DE, err %.1f%%)\n",
           above_kc/HEIGHT*100, fabs(above_kc/HEIGHT*100 - DE)/DE*100);

    printf("    %.1f%% = KC_H / height (target: 4.9%% VM, err %.1f%%)\n",
           KC_H/HEIGHT*100, fabs(KC_H/HEIGHT*100 - VM)/VM*100);

    printf("    %.1f%% = slope / 90 (target: 68.3%% DE, err %.1f%%)\n",
           SLOPE_DEG/90*100, fabs(SLOPE_DEG/90*100 - DE)/DE*100);

    printf("    %.1f%% = passage / 90 (target: 26.8%% DM, err %.1f%%)\n",
           PASS_ANGLE/90*100, fabs(PASS_ANGLE/90*100 - DM)/DM*100);

    return 0;
}
