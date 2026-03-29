/*
 * test_spread.c — Phase 0 gate: coordinate spread test
 *
 * Feeds common English words through derive() with accumulating context.
 * Measures bin entropy. Must pass before any large ingest.
 *
 * Pass: shell-1 entropy > 4.0, no shell-1 bin > 10%
 * Fail: either condition violated → derive.h is broken
 *
 * Build:
 *   cc -O2 -Wall -Wno-unused-function -o test_spread test_spread.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "wave.h"
#include "tokenize.h"
#include "distance.h"
#include "bonds.h"
#include "attention.h"
#include "derive.h"

#define FIELD_IMPLEMENTATION
#include "field.h"

/* 200 common English words covering nouns, verbs, adjectives,
 * prepositions, adverbs, conjunctions, determiners.
 * NOT primes — the test is about what happens to NON-prime words. */
static const char *test_words[] = {
    /* nouns */
    "cat", "dog", "house", "tree", "water", "fire", "book", "stone",
    "river", "mountain", "city", "road", "child", "hand", "heart",
    "night", "day", "world", "land", "king", "war", "food", "light",
    "music", "art", "gold", "iron", "rain", "snow", "wind",
    "fish", "bird", "horse", "wolf", "bear", "ship", "sword", "door",
    "wall", "bridge", "garden", "forest", "ocean", "desert", "island",
    "tower", "castle", "village", "market", "church",
    /* verbs */
    "run", "walk", "eat", "drink", "sleep", "fight", "build", "break",
    "sing", "dance", "read", "write", "speak", "listen", "watch",
    "fly", "swim", "jump", "fall", "rise", "grow", "cut", "burn",
    "open", "close", "push", "pull", "throw", "catch", "hold",
    "carry", "drop", "turn", "stand", "sit", "lie", "wait", "play",
    "win", "lose", "find", "hide", "kill", "save", "help", "lead",
    "follow", "begin", "end", "stay",
    /* adjectives */
    "old", "young", "new", "long", "short", "high", "low", "dark",
    "bright", "hot", "cold", "fast", "slow", "hard", "soft", "strong",
    "weak", "deep", "wide", "heavy", "empty", "full", "rich", "poor",
    "clean", "dirty", "sweet", "bitter", "sharp", "smooth",
    "loud", "quiet", "wild", "gentle", "brave", "wise", "proud",
    "angry", "happy", "sad",
    /* adverbs */
    "always", "never", "often", "sometimes", "quickly", "slowly",
    "quietly", "loudly", "carefully", "suddenly",
    /* prepositions */
    "into", "from", "with", "through", "between", "under", "over",
    "across", "along", "behind",
    /* conjunctions */
    "while", "until", "since", "though", "unless",
    /* derived forms (test morpheme decomposition) */
    "running", "walked", "broken", "singing", "darkness",
    "quickly", "beautiful", "unhappy", "rebuild", "misunderstood",
    "impossible", "discovery", "movement", "carefully", "stronger",
};

#define N_TEST_WORDS (sizeof(test_words) / sizeof(test_words[0]))

int main(void) {
    printf("Phase 0 spread test: %zu words\n\n", N_TEST_WORDS);

    /* Run words through derive with accumulating context */
    uint32_t bins_s1[81] = {0};
    uint32_t bins_s2[625] = {0};

    DeriveResult context[8];
    int ctx_len = 0;
    int sentence_pos = 0;

    /* Simulate sentences of ~8 words */
    for (size_t i = 0; i < N_TEST_WORDS; i++) {
        const char *word = test_words[i];
        int len = (int)strlen(word);

        DeriveResult dr = derive(word, len, context, ctx_len, sentence_pos);

        int b1 = coord4_to_bin_s1(dr.coord);
        int b2 = coord4_to_bin_s2(dr.coord);
        bins_s1[b1]++;
        bins_s2[b2]++;

        /* update context */
        if (ctx_len < 8) {
            context[ctx_len++] = dr;
        } else {
            for (int j = 0; j < 7; j++) context[j] = context[j+1];
            context[7] = dr;
        }

        sentence_pos++;
        /* reset sentence every ~8 words */
        if (sentence_pos >= 8) {
            sentence_pos = 0;
            ctx_len = 0;
        }
    }

    /* compute shell-1 entropy */
    uint32_t total_s1 = 0;
    for (int i = 0; i < 81; i++) total_s1 += bins_s1[i];

    double entropy_s1 = 0.0;
    double max_share_s1 = 0.0;
    int occupied_s1 = 0;
    int max_bin_s1 = -1;

    for (int i = 0; i < 81; i++) {
        if (bins_s1[i] == 0) continue;
        occupied_s1++;
        double p = (double)bins_s1[i] / total_s1;
        entropy_s1 -= p * log2(p);
        if (p > max_share_s1) {
            max_share_s1 = p;
            max_bin_s1 = i;
        }
    }

    /* compute shell-2 entropy */
    uint32_t total_s2 = 0;
    for (int i = 0; i < 625; i++) total_s2 += bins_s2[i];

    double entropy_s2 = 0.0;
    double max_share_s2 = 0.0;
    int occupied_s2 = 0;

    for (int i = 0; i < 625; i++) {
        if (bins_s2[i] == 0) continue;
        occupied_s2++;
        double p = (double)bins_s2[i] / total_s2;
        entropy_s2 -= p * log2(p);
        if (p > max_share_s2) max_share_s2 = p;
    }

    /* confidence distribution */
    int conf_counts[4] = {0};
    ctx_len = 0;
    sentence_pos = 0;
    for (size_t i = 0; i < N_TEST_WORDS; i++) {
        DeriveResult dr = derive(test_words[i], (int)strlen(test_words[i]),
                                 context, ctx_len, sentence_pos);
        conf_counts[dr.confidence < 4 ? dr.confidence : 3]++;
        if (ctx_len < 8) { context[ctx_len++] = dr; }
        else { for (int j=0;j<7;j++) context[j]=context[j+1]; context[7]=dr; }
        sentence_pos++;
        if (sentence_pos >= 8) { sentence_pos = 0; ctx_len = 0; }
    }

    /* report */
    printf("Shell 1 (81 bins):\n");
    printf("  occupied:    %d / 81 (%.0f%%)\n", occupied_s1, 100.0*occupied_s1/81);
    printf("  entropy:     %.2f bits (max 6.34, target > 4.0)\n", entropy_s1);
    printf("  max share:   %.1f%% at bin %d (target < 10%%)\n",
           max_share_s1 * 100, max_bin_s1);

    printf("\nShell 2 (625 bins):\n");
    printf("  occupied:    %d / 625 (%.0f%%)\n", occupied_s2, 100.0*occupied_s2/625);
    printf("  entropy:     %.2f bits (max 9.29)\n", entropy_s2);
    printf("  max share:   %.1f%%\n", max_share_s2 * 100);

    printf("\nConfidence distribution:\n");
    printf("  prime (3):     %d (%.0f%%)\n", conf_counts[3], 100.0*conf_counts[3]/N_TEST_WORDS);
    printf("  morpheme (2):  %d (%.0f%%)\n", conf_counts[2], 100.0*conf_counts[2]/N_TEST_WORDS);
    printf("  position (1):  %d (%.0f%%)\n", conf_counts[1], 100.0*conf_counts[1]/N_TEST_WORDS);
    printf("  char-level (0):%d (%.0f%%)\n", conf_counts[0], 100.0*conf_counts[0]/N_TEST_WORDS);

    /* top 10 shell-1 bins */
    printf("\nTop 10 shell-1 bins:\n");
    for (int rank = 0; rank < 10; rank++) {
        int best = -1;
        uint32_t best_count = 0;
        for (int i = 0; i < 81; i++) {
            if (bins_s1[i] > best_count) {
                best_count = bins_s1[i];
                best = i;
            }
        }
        if (best < 0) break;
        coord4 bc = bin_s1_to_coord4(best);
        printf("  bin %2d (%+d,%+d,%+d,%+d): %u words (%.1f%%)\n",
               best, bc.t, bc.x, bc.m, bc.q,
               best_count, 100.0*best_count/total_s1);
        bins_s1[best] = 0; /* zero out for next iteration */
    }

    /* pass/fail */
    printf("\n");
    int pass = 1;
    if (entropy_s1 < 4.0) {
        printf("FAIL: shell-1 entropy %.2f < 4.0\n", entropy_s1);
        pass = 0;
    }
    if (max_share_s1 > 0.10) {
        printf("FAIL: max shell-1 bin share %.1f%% > 10%%\n", max_share_s1 * 100);
        pass = 0;
    }
    if (pass) {
        printf("PASS: entropy %.2f > 4.0, max share %.1f%% < 10%%\n",
               entropy_s1, max_share_s1 * 100);
    }

    return pass ? 0 : 1;
}
