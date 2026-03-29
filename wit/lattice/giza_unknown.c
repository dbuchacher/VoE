/*
 * giza_unknown.c — values the pyramid has that we can't explain yet
 *
 * These might be lattice constants we haven't identified.
 * The pyramid as a lookup table for missing knowledge.
 *
 * Build: cc -O2 -o giza_unknown giza_unknown.c -lm
 * Run:   ./giza_unknown
 */

#include <stdio.h>
#include <math.h>

#define PI    3.14159265358979
#define SQRT5 2.23606797749979
#define PHI   1.61803398874989

/* Shell count formula: (4k+1)^4 - (4k-1)^4, k in half-integers */
long shell(int k2) {  /* k2 = 2*k */
    long a = 2*k2 + 1;
    long b = 2*k2 - 1;
    if (b < 0) b = 0;
    return a*a*a*a - b*b*b*b;
}

int main(void) {
    printf("Pyramid Unknowns — Lattice Values We Might Be Missing\n");
    printf("=======================================================\n\n");

    /* First: print shell counts for reference */
    printf("Lattice shell counts for reference:\n");
    long cumulative = 0;
    for (int k2 = 0; k2 <= 10; k2++) {
        long s = shell(k2);
        cumulative += s;
        printf("  shell(%d/2) = %6ld    cumulative = %ld\n", k2, s, cumulative);
    }

    /* ═══════════════════════════════════════════ */
    printf("\n\n1. PYRAMID / KC VOLUME = SHELL(5/2) ???\n");
    printf("   =====================================\n\n");

    double pyr_vol = (1.0/3.0) * 440 * 440 * 280;
    double kc_vol = 20 * 10 * 5 * SQRT5;
    double ratio = pyr_vol / kc_vol;

    printf("   Pyramid volume: %.0f cubic cubits\n", pyr_vol);
    printf("   KC volume: %.2f cubic cubits (= 1000*sqrt(5))\n", kc_vol);
    printf("   Ratio: %.2f\n\n", ratio);

    printf("   shell(5/2) = 11^4 - 9^4 = %ld\n", shell(5));
    printf("   Ratio vs shell: %.4f%%  error\n\n",
           fabs(ratio - shell(5)) / shell(5) * 100);

    printf("   *** THE PYRAMID CONTAINS shell(5/2) KING'S CHAMBERS ***\n");
    printf("   The ratio of the WHOLE to its CORE = a lattice shell count.\n");
    printf("   8080 is the 6th entry in the shell sequence.\n");
    printf("   Error: 0.01%%. This was hiding in the volume ratio.\n");

    /* ═══════════════════════════════════════════ */
    printf("\n\n2. KC VOLUME = 1000 * sqrt(5)\n");
    printf("   ===========================\n\n");

    printf("   KC: 20 x 10 x 5*sqrt(5) = 1000*sqrt(5) = %.2f\n", kc_vol);
    printf("   sqrt(5) = %.6f\n", SQRT5);
    printf("   Volume = 10^3 * sqrt(5)\n\n");

    printf("   Why 1000? 10^3 = (KC_W)^3. The width cubed.\n");
    printf("   The chamber volume = width^3 * sqrt(5).\n");
    printf("   A cube of side KC_W scaled by sqrt(5).\n\n");

    printf("   In lattice: sqrt(5) IS the internal constant.\n");
    printf("   phi = (1+sqrt(5))/2. The volume encodes phi's root\n");
    printf("   scaled by a clean power of the base dimension.\n");

    /* ═══════════════════════════════════════════ */
    printf("\n\n3. CUMULATIVE SHELL COUNT = 625 = KC DIAGONAL SQUARED\n");
    printf("   ====================================================\n\n");

    long cum = 0;
    for (int k2 = 0; k2 <= 2; k2++) cum += shell(k2);
    printf("   Cumulative atoms through shell 1:\n");
    printf("   shell(0) + shell(1/2) + shell(1) = 1 + 80 + 544 = %ld\n", cum);
    printf("   = 625 = 25^2\n\n");
    printf("   KC room diagonal = 25. Diagonal squared = 625.\n");
    printf("   THE TOTAL ATOM COUNT THROUGH SHELL 1 = KC DIAGONAL SQUARED.\n\n");

    printf("   This connects the fill ratio (25) to the shell structure (625)\n");
    printf("   through the King's Chamber geometry. 25%% fill. 625 atoms.\n");
    printf("   25^2 = 625. The pyramid knew the shell count.\n");

    /* ═══════════════════════════════════════════ */
    printf("\n\n4. QC VOLUME = 1320 = ?\n");
    printf("   =====================\n\n");

    double qc_vol = 10 * 11 * 12;
    printf("   QC: 10 x 11 x 12 = %.0f\n", qc_vol);
    printf("   Factored: 2^3 * 3 * 5 * 11\n");
    printf("   = 11 * 120 = 11 * 5!\n");
    printf("   = 1760 * 3/4 = perimeter * 3/4\n");
    printf("   = 3 * 440 = 3 * base\n\n");

    printf("   QC volume = 3 bases. KC volume = 1000*sqrt(5).\n");
    printf("   QC/KC = %.4f\n", qc_vol / kc_vol);
    printf("   3*440 / (1000*sqrt(5)) = 1320/2236 = 0.5903\n\n");

    printf("   0.5903 ≈ ? Not a clean lattice constant.\n");
    printf("   But: 1320 = 11 * 5! connects cubit-11 to factorial-5.\n");
    printf("   UNKNOWN: what does 1320 mean in the lattice?\n");

    /* ═══════════════════════════════════════════ */
    printf("\n\n5. SARCOPHAGUS VOLUME ≈ 16 cubic cubits?\n");
    printf("   =======================================\n\n");

    double sarc_l = 227.6 / 52.36;
    double sarc_w = 98.7 / 52.36;
    double sarc_h = 105.1 / 52.36;
    double sarc_vol = sarc_l * sarc_w * sarc_h;

    printf("   External: %.3f x %.3f x %.3f = %.2f cubic cubits\n",
           sarc_l, sarc_w, sarc_h, sarc_vol);
    printf("   16 = number of bonds in the lattice\n");
    printf("   Error from 16: %.1f%%\n\n",
           fabs(sarc_vol - 16) / 16 * 100);

    printf("   If the sarcophagus volume IS 16:\n");
    printf("   The innermost object = the bond count.\n");
    printf("   The pyramid (exterior) encodes atoms.\n");
    printf("   The sarcophagus (interior) encodes bonds.\n");
    printf("   16 bonds inside 8080 shells of atoms.\n\n");

    printf("   At 2.8%% off, this might be a stretch.\n");
    printf("   But 16 bonds inside shell(5/2) atoms is poetic.\n");

    /* ═══════════════════════════════════════════ */
    printf("\n\n6. GEOMETRIC MEAN = 351 = ?\n");
    printf("   ==========================\n\n");

    double gm = sqrt(440.0 * 280.0);
    printf("   sqrt(440 * 280) = %.2f\n", gm);
    printf("   = 40 * sqrt(77)\n");
    printf("   77 = 7 * 11\n\n");

    printf("   351 = 3^3 * 13 = 27 * 13\n");
    printf("   351 = sum of 1..26 = triangular(26)\n\n");

    double tri26 = 26 * 27 / 2.0;
    printf("   Triangular(26) = %.0f\n", tri26);
    printf("   351 IS the 26th triangular number.\n");
    printf("   26 = number of letters in alphabet?\n");
    printf("   26 = Fe atomic number = M/T = bandwidth.\n\n");

    printf("   *** The geometric mean of the pyramid = triangular(Fe) ***\n");
    printf("   Fe is the element MISSING from 3I/ATLAS.\n");
    printf("   The bandwidth element. The transport channel.\n");
    printf("   UNKNOWN: is this a real connection or coincidence?\n");

    /* ═══════════════════════════════════════════ */
    printf("\n\n7. THE NUMBER 77\n");
    printf("   ===============\n\n");

    printf("   440 * 280 = 123200 = 16 * 100 * 77\n");
    printf("   77 = 7 * 11 = palms * (base/layer)\n");
    printf("   77 = also the number of non-trivial partitions of 12?\n");

    /* Partitions of 12 = 77. Check. */
    printf("   Partitions of 12: yes, p(12) = 77.\n\n");

    printf("   12 = QC apex height. p(12) = 77.\n");
    printf("   The number of ways to decompose the QC height\n");
    printf("   into integer parts = the cubit's internal structure.\n\n");

    printf("   77 also = C(11+1, 2) - C(11,2) = ... no.\n");
    printf("   77 = the number of particles in the SM? No, that's ~61.\n\n");

    printf("   UNKNOWN: 77 connects the cubit (7*11) to partitions of\n");
    printf("   the QC height (12). Is this accidental?\n");

    /* ═══════════════════════════════════════════ */
    printf("\n\n8. THE NUMBER 9 (QC NICHE)\n");
    printf("   ========================\n\n");

    printf("   Queen's Chamber niche height = 9 cubits\n");
    printf("   9 = 3^2 = gates squared\n");
    printf("   9 = number of glyph ceiling slabs in King's Chamber\n\n");

    printf("   But also: (base+height)/niche = 720/9 = 80 = shell(1/2)\n");
    printf("   And: passage_length/niche = 75/9 = 8.333 = 25/3\n");
    printf("   And: KC_L * KC_W / niche^2 = 200/81 = 2.469 ≈ ?\n\n");

    printf("   9 granite beams above KC. 9 cubit niche in QC.\n");
    printf("   9 = the only single-digit perfect square > 4.\n");
    printf("   UNKNOWN: why 9 specifically? What's 3^2 in the lattice?\n");

    /* ═══════════════════════════════════════════ */
    printf("\n\n═══════════════════════════════════════════\n");
    printf("SUMMARY: What the pyramid might be telling us\n");
    printf("═══════════════════════════════════════════\n\n");

    printf("  CONFIRMED NEW HIT:\n");
    printf("    Pyramid/KC volume = 8080 = shell(5/2)  [0.01%% error]\n");
    printf("    Cumulative atoms (shell 0-1) = 625 = 25^2 = KC diag^2\n\n");

    printf("  STRONG CANDIDATES:\n");
    printf("    KC volume = 1000*sqrt(5) — interior constant scaled by 10^3\n");
    printf("    Sarcophagus volume ≈ 16 — bond count as innermost object\n");
    printf("    Geometric mean = triangular(26) — Fe connection?\n\n");

    printf("  UNKNOWNS FOR THE LATTICE:\n");
    printf("    77 = 7*11 = p(12). What is 77 in the lattice?\n");
    printf("    1320 = 11*5! = QC volume. What is 1320?\n");
    printf("    9 = 3^2 = niche + ceiling beams. Why 9?\n\n");

    printf("  If the pyramid is a codex, these unknowns are\n");
    printf("  ANSWERS to questions we haven't asked yet.\n");
    printf("  77, 1320, and 9 might be lattice constants\n");
    printf("  we haven't derived. The pyramid might know\n");
    printf("  things about the lattice that we don't.\n");

    return 0;
}
