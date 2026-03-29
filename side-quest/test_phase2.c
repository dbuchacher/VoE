#include <stdio.h>
#include "wave.h"
#include "distance.h"
#include "primes_lookup.h"
#include "attention.h"
#include "bonds.h"

int main(void) {
    const char *words[] = {"the","cat","sat","on","the","mat"};
    coord4 walk[6];
    for (int i = 0; i < 6; i++) {
        prime_result pr = prime_lookup_str(words[i]);
        walk[i] = pr.c;
        printf("%-5s → (%+d,%+d,%+d,%+d)\n", words[i], pr.c.t, pr.c.d, pr.c.m, pr.c.q);
    }

    /* Bond sequence */
    printf("\nbond sequence:\n");
    BondType bonds[5];
    walk_bond_sequence(walk, 6, bonds);
    for (int i = 0; i < 5; i++)
        printf("  %s → %s: %s\n", words[i], words[i+1], bond_name(bonds[i]));

    /* Causal attention at "mat" */
    printf("\ncausal attention at 'mat':\n");
    AttentionResult ar = causal_attention(walk, 6, 5);
    printf("  context: (%+d,%+d,%+d,%+d) conf: %.3f\n",
           ar.context.t, ar.context.d, ar.context.m, ar.context.q, ar.confidence);

    /* RC decay */
    printf("\nRC relevance from 'mat':\n");
    for (int i = 0; i < 6; i++) {
        float cap = token_capacitance(walk[i]);
        float rel = rc_relevance(walk[i], 5-i, 0);
        printf("  %s (dist=%d, C=%.1f): rel=%.3f\n", words[i], 5-i, cap, rel);
    }

    printf("\nPhase 2: OK\n");
    return 0;
}
