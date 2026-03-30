/*
 * check_bins.c — analyze which bins a text file populates
 *
 * Usage: check_bins [FILE ...]
 *   With no args, reads stdin.
 *   Prints: bin distribution, empty bins, gap descriptions.
 *
 * Build: cc -O2 -Wall -Wno-unused-function -o check_bins check_bins.c -lm
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

static const char *bin_description(coord4 c) {
    /* describe what kind of text fills this coordinate region */
    static char buf[256];
    char *p = buf;

    if (c.t > 0) p += sprintf(p, "τ+(active/verb) ");
    if (c.t < 0) p += sprintf(p, "τ-(past/passive) ");
    if (c.x > 0) p += sprintf(p, "χ+(near/spatial) ");
    if (c.x < 0) p += sprintf(p, "χ-(inside/deep) ");
    if (c.m > 0) p += sprintf(p, "μ+(substance/noun) ");
    if (c.m < 0) p += sprintf(p, "μ-(absence/void) ");
    if (c.q > 0) p += sprintf(p, "φ+(positive/test) ");
    if (c.q < 0) p += sprintf(p, "φ-(negative/guard) ");
    if (c.t == 0 && c.x == 0 && c.m == 0 && c.q == 0)
        p += sprintf(p, "(origin/silence)");

    if (p > buf && *(p-1) == ' ') *(p-1) = '\0';
    return buf;
}

static void process_file(FILE *f, uint32_t bins[81], int conf[4],
                         int *total_words) {
    char line[4096];
    Token toks[512];
    DeriveResult context[8];
    int ctx_len = 0, sentence_pos = 0;

    while (fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        int ntok = tokenize_line(line, len, toks, 512);

        for (int i = 0; i < ntok; i++) {
            if (toks[i].is_punct && toks[i].len == 1) {
                char ch = toks[i].start[0];
                if (ch == '.' || ch == '!' || ch == '?') {
                    sentence_pos = 0;
                    ctx_len = 0;
                    continue;
                }
            }
            if (toks[i].is_punct) continue;

            char word[64];
            int wl = toks[i].len < 63 ? toks[i].len : 63;
            for (int j = 0; j < wl; j++)
                word[j] = (char)tok_lower(toks[i].start[j]);
            word[wl] = '\0';

            DeriveResult dr = derive(word, wl, context, ctx_len, sentence_pos);
            bins[coord4_to_bin_s1(dr.coord)]++;
            conf[dr.confidence < 4 ? dr.confidence : 3]++;
            (*total_words)++;

            if (ctx_len < 8) context[ctx_len++] = dr;
            else { for (int j=0;j<7;j++) context[j]=context[j+1]; context[7]=dr; }
            sentence_pos++;
        }
    }
}

int main(int argc, char **argv) {
    uint32_t bins[81] = {0};
    int conf[4] = {0};
    int total = 0;

    if (argc <= 1) {
        process_file(stdin, bins, conf, &total);
    } else {
        for (int i = 1; i < argc; i++) {
            FILE *f = fopen(argv[i], "r");
            if (!f) { perror(argv[i]); continue; }
            process_file(f, bins, conf, &total);
            fclose(f);
        }
    }

    /* entropy + stats */
    double h = 0, max_share = 0;
    int occupied = 0, max_bin = -1;
    for (int i = 0; i < 81; i++) {
        if (!bins[i]) continue;
        occupied++;
        double p = (double)bins[i] / total;
        h -= p * log2(p);
        if (p > max_share) { max_share = p; max_bin = i; }
    }

    printf("=== Bin Analysis: %d words ===\n\n", total);
    printf("Confidence: prime=%d(%.0f%%) morph=%d(%.0f%%) pos=%d(%.0f%%) char=%d(%.0f%%)\n",
           conf[3], 100.0*conf[3]/total, conf[2], 100.0*conf[2]/total,
           conf[1], 100.0*conf[1]/total, conf[0], 100.0*conf[0]/total);
    printf("Shell-1: %d/81 occupied (%.0f%%), entropy=%.2f bits, max=%.1f%% at bin %d\n\n",
           occupied, 100.0*occupied/81, h, max_share*100, max_bin);

    /* occupied bins sorted by count */
    printf("--- Occupied bins ---\n");
    int order[81];
    for (int i=0;i<81;i++) order[i]=i;
    for (int i=0;i<80;i++) for(int j=i+1;j<81;j++)
        if(bins[order[j]]>bins[order[i]]) {int t=order[i];order[i]=order[j];order[j]=t;}

    for (int r=0;r<81 && bins[order[r]]>0;r++) {
        int b = order[r];
        coord4 c = bin_s1_to_coord4(b);
        printf("  bin %2d (%+d,%+d,%+d,%+d) %4d words %5.1f%%  %s\n",
               b, c.t, c.x, c.m, c.q, bins[b], 100.0*bins[b]/total,
               bin_description(c));
    }

    /* empty bins grouped by what they need */
    int n_empty = 0;
    for (int i=0;i<81;i++) if(!bins[i]) n_empty++;

    printf("\n--- Empty bins: %d ---\n", n_empty);
    printf("Generate text targeting these coordinate regions:\n\n");

    /* group by dominant missing dimension */
    const char *groups[] = {"τ- (past/passive/ended)", "χ- (inside/deep/hidden)",
                            "μ- (absence/void/nothing)", "φ- (negative/wrong/bad)",
                            "compound negative", "compound positive", "other"};
    for (int g = 0; g < 7; g++) {
        int first = 1;
        for (int i = 0; i < 81; i++) {
            if (bins[i]) continue;
            coord4 c = bin_s1_to_coord4(i);
            int group;
            int neg_count = (c.t<0)+(c.x<0)+(c.m<0)+(c.q<0);
            int pos_count = (c.t>0)+(c.x>0)+(c.m>0)+(c.q>0);
            if (neg_count >= 2) group = 4;
            else if (c.t < 0 && neg_count == 1) group = 0;
            else if (c.x < 0 && neg_count == 1) group = 1;
            else if (c.m < 0 && neg_count == 1) group = 2;
            else if (c.q < 0 && neg_count == 1) group = 3;
            else if (pos_count >= 2) group = 5;
            else group = 6;

            if (group != g) continue;
            if (first) { printf("  [%s]\n", groups[g]); first = 0; }
            printf("    bin %2d (%+d,%+d,%+d,%+d)  %s\n",
                   i, c.t, c.x, c.m, c.q, bin_description(c));
        }
    }

    printf("\n%s\n", (h > 4.0 && max_share < 0.10) ? "PASS" : "FAIL");
    return (h > 4.0 && max_share < 0.10) ? 0 : 1;
}
