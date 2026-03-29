#include "distance.h"
#include <stdio.h>
#include "wave.h"
#include "density.h"
#include "primes_lookup.h"

int main(void) {
    DensityMap *dm = density_load("lattice_density.bin");
    if (!dm) { fprintf(stderr, "can't load density\n"); return 1; }
    
    printf("=== EMPTY SHELL-1 BINS (coordinate regions with NO data) ===\n\n");
    int empty = 0;
    for (int i = 0; i < 81; i++) {
        if (dm->counts_s1[i] == 0) {
            coord4 c = bin_s1_to_coord(i);
            printf("  bin %2d  (%+d,%+d,%+d,%+d)  ", i, c.t, c.d, c.m, c.q);
            
            // describe what kind of text would live here
            char *tdesc = c.t > 0 ? "sustained/ongoing" : c.t < 0 ? "sudden/instant" : "neutral-time";
            char *ddesc = c.d > 0 ? "front/near/here" : c.d < 0 ? "back/far/there" : "neutral-space";
            char *mdesc = c.m > 0 ? "open/substance" : c.m < 0 ? "closed/absent" : "neutral-mass";
            char *qdesc = c.q > 0 ? "voiced/positive" : c.q < 0 ? "voiceless/negative" : "neutral-quality";
            
            printf("%s, %s, %s, %s\n", tdesc, ddesc, mdesc, qdesc);
            empty++;
        }
    }
    
    printf("\n=== OCCUPIED BINS (sorted by count) ===\n\n");
    // simple insertion sort of occupied bins
    int indices[81];
    int nocc = 0;
    for (int i = 0; i < 81; i++)
        if (dm->counts_s1[i] > 0)
            indices[nocc++] = i;
    
    for (int i = 0; i < nocc-1; i++)
        for (int j = i+1; j < nocc; j++)
            if (dm->counts_s1[indices[i]] < dm->counts_s1[indices[j]]) {
                int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
            }
    
    for (int i = 0; i < nocc; i++) {
        int idx = indices[i];
        coord4 c = bin_s1_to_coord(idx);
        printf("  bin %2d  (%+d,%+d,%+d,%+d)  count=%u  (%.2f%%)\n",
               idx, c.t, c.d, c.m, c.q, dm->counts_s1[idx],
               100.0f * dm->probs_s1[idx]);
    }
    
    printf("\n%d empty, %d occupied out of 81 bins\n", empty, nocc);
    density_free(dm);
    return 0;
}
