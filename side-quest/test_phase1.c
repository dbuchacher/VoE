/* Phase 1 integration test — does everything compile and link? */
#include <stdio.h>
#include "wave.h"
#include "tokenize.h"
#include "primes_lookup.h"
#include "density.h"
/* coord_index needs special handling — it has a .c file */

int main(void) {
    /* 1. Wave converter v2 */
    uint8_t wb = utf8_to_wave('a');
    coord4 c = wave_decode(wb);
    printf("wave: 'a' = 0x%02X = (%d,%d,%d,%d)\n", wb, c.t, c.d, c.m, c.q);

    char back = wave_to_utf8(wb);
    printf("reverse: 0x%02X = '%c'\n", wb, back);

    /* 2. Tokenizer */
    const char *text = "the cat sat on the mat";
    Token tokens[32];
    int n = tokenize_line(text, strlen(text), tokens, 32);
    printf("tokens(%d):", n);
    for (int i = 0; i < n; i++)
        printf(" '%.*s'", tokens[i].len, tokens[i].start);
    printf("\n");

    /* 3. Prime decomposer */
    for (int i = 0; i < n; i++) {
        char word[64];
        int wlen = tokens[i].len < 63 ? tokens[i].len : 63;
        memcpy(word, tokens[i].start, wlen);
        word[wlen] = '\0';
        prime_result pr = prime_lookup_str(word);
        printf("  '%s' → (%+d,%+d,%+d,%+d) %s\n", word,
               pr.c.t, pr.c.d, pr.c.m, pr.c.q,
               pr.status == PRIME_FOUND ? "FOUND" : "UNKNOWN");
    }

    /* 4. Density map */
    DensityMap *dm = density_create();
    coord4 test_coords[] = {
        {0,0,0,+1}, {0,0,0,+1}, {0,0,0,+1},  /* good good good */
        {0,0,0,-1},                             /* bad */
        {+1,+1,0,0}, {+1,+1,0,0},              /* run run */
    };
    for (int i = 0; i < 6; i++)
        density_add(dm, test_coords[i]);
    density_normalize(dm);
    printf("density at GOOD: %.3f\n", density_probability(dm, (coord4){0,0,0,+1}));
    printf("density at BAD:  %.3f\n", density_probability(dm, (coord4){0,0,0,-1}));
    printf("density at RUN:  %.3f\n", density_probability(dm, (coord4){+1,+1,0,0}));
    density_free(dm);

    printf("\nPhase 1: all components compile and run.\n");
    return 0;
}
