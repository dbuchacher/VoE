/*
 * wasm — walk assembler
 *
 *   physics:    compiles quantum state descriptions into wave packets
 *   math:       maps symbolic notation to byte representation
 *   programmer: assembles walk files into raw binary blobs
 *   english:    turns bond symbols into the bytes the walker reads
 *
 * Usage: hodos input.w [-o output.bin]
 *
 * Input format:
 *   π̄δ  θ¹¹ρ  0xE9  'V'        bond  flags  arg0  arg1
 *   : label_name                label definition
 *   skip_z label_name           conditional skip
 *   # comment                   ignored
 *
 * Output: raw bytes. Same bytes NASM would produce.
 * This tool dissolves on a trit processor — hardware reads walks natively.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_LABELS  256
#define MAX_FIXUPS  256
#define MAX_OUTPUT  65536
#define MAX_LINE    1024
#define MAX_TOKEN   128

/* ── output buffer ─────────────────────────────────────────── */

static uint8_t  out[MAX_OUTPUT];
static int      pos = 0;

static void emit8(uint8_t v)  { out[pos++] = v; }
static void emit32(uint32_t v) {
    out[pos++] = v & 0xFF;
    out[pos++] = (v >> 8) & 0xFF;
    out[pos++] = (v >> 16) & 0xFF;
    out[pos++] = (v >> 24) & 0xFF;
}
static void emit64(uint64_t v) {
    emit32(v & 0xFFFFFFFF);
    emit32(v >> 32);
}

/* ── labels ────────────────────────────────────────────────── */

static struct { char name[MAX_TOKEN]; int pos; } labels[MAX_LABELS];
static int nlabels = 0;

static struct { char name[MAX_TOKEN]; int pos; uint8_t ctrl; } fixups[MAX_FIXUPS];
static int nfixups = 0;

static int find_label(const char *name) {
    for (int i = 0; i < nlabels; i++)
        if (strcmp(labels[i].name, name) == 0)
            return labels[i].pos;
    return -1;
}

/* ── bond symbol → wave byte(s) ────────────────────────────── */

struct bond { const char *sym; uint8_t bytes[4]; int len; };

static const struct bond bonds[] = {
    /* pattern 1: π (read/write) */
    {"π",      {0x40},       1},
    {"π̄",      {0xC0},       1},
    {"π₃",     {0x80, 0x03}, 2},     /* byte read (T=+3 extended) */
    {"π̄₃",     {0x80, 0xFD}, 2},       /* byte write (T=-3 extended) */
    {"π₅",     {0x80, 0x05}, 2},     /* word read (T=+5 extended) */
    {"π̄₅",     {0x80, 0xFB}, 2},     /* word write (T=-5 extended) */
    {"π₇",     {0x80, 0x07}, 2},     /* dword read (T=+7 extended) */
    {"π̄₇",     {0x80, 0xF9}, 2},     /* dword write (T=-7 extended) */

    /* pattern 2: ∘ (call/return) */
    {"∘",      {0x10},       1},
    {"∘̄",      {0x30},       1},

    /* pattern 4: ∮ (loop/increment) */
    {"∮̄",      {0x0C},       1},     /* increment (-R, M=-1) */

    /* pattern 5: π∮ (copy/fill) */
    {"π̄∮̄₃",    {0xC8, 0xFD}, 2},    /* dword fill (T=-1, M=-3 ext) */

    /* pattern 8: δ (test/guard) + magnitudes */
    {"δ",      {0x01},       1},     /* test (==) */
    {"δ̄",      {0x03},       1},     /* guard */
    {"δ₃",     {0x02, 0x03}, 2},     /* AND (Q=+3 ext) */
    {"δ̄₃",     {0x02, 0xFD}, 2},    /* OR  (Q=-3 ext) */
    {"δ₅",     {0x02, 0x05}, 2},     /* XOR */
    {"δ̄₅",     {0x02, 0xFB}, 2},    /* NOT */
    {"δ₇",     {0x02, 0x07}, 2},     /* SHL */
    {"δ̄₇",     {0x02, 0xF9}, 2},    /* SHR */
    {"δ₁₁",    {0x02, 0x0B}, 2},    /* less-than */
    {"δ̄₁₁",    {0x02, 0xF5}, 2},    /* greater-than */
    {"δ₁₃",    {0x02, 0x0D}, 2},    /* negate */
    {"δ̄₁₃",    {0x02, 0xF3}, 2},    /* abs */

    /* pattern 9: πδ (filter/port/add) */
    {"πδ̄",     {0x43},       1},     /* port_read byte (+P-W) */
    {"π₇δ̄",    {0x83, 0x07}, 2},    /* port_read dword (+P-W, T=+7 ext) */
    {"πδ̄₃",    {0x42, 0xFD}, 2},    /* MSR read (+P, Q=-3 ext) */
    {"πδ̄₅",    {0x42, 0xFB}, 2},    /* CPUID read (+P, Q=-5 ext) */
    {"π̄δ",     {0xC1},       1},     /* port_write byte (-P+W) */
    {"π̄₇δ",    {0x81, 0xF9}, 2},    /* port_write dword (-P+W, T=-7 ext) */
    {"π̄δ₃",    {0xC2, 0x03}, 2},    /* MSR write (-P, Q=+3 ext) */
    {"π̄δ̄",     {0xC3},       1},     /* add (-P-W) */
    {"π̄δ̄₃",    {0xC2, 0xFD}, 2},   /* subtract (-P, Q=-3 ext) */
    {"πδ",     {0x41},       1},     /* filter/min (+P+W) */
    {"πδ₃",    {0x42, 0x03}, 2},     /* max (+P, Q=+3 ext) */

    /* pattern 14: ∘∮δ (bind) */
    {"∘∮δ",    {0x15},       1},

    /* pattern 15: π∘∮δ (hylo) */
    {"π∘∮δ",   {0x55},       1},

    /* pattern 3: π∘ (fold) */
    {"π∘",     {0x50},       1},

    /* pattern 6: ∘∮ (fix) */
    {"∘∮",     {0x14},       1},

    /* pattern 7: π∘∮ (foldl) */
    {"π∘∮",    {0x54},       1},

    /* pattern 10: ∘δ (maybe) */
    {"∘δ",     {0x11},       1},

    /* pattern 11: π∘δ (map) */
    {"π∘δ",    {0x51},       1},

    /* pattern 12: ∮δ (until/take_while) */
    {"∮δ",     {0x05},       1},

    /* pattern 13: π∮δ (scan) / π∮δ̄ (port bulk read) */
    {"π∮δ",    {0x45},       1},     /* scan: accumulate */
    {"π∮δ̄",    {0x47},       1},     /* port bulk read: rep insd */

    /* atom: τχ (multiply) */
    {"τχ",     {0x80|0x20, 0x02, 0x02}, 3},  /* T=+2 ext, D=+2 ext */

    {NULL, {0}, 0}
};

/* ── θ constants ──────────────────────────────────────────── */

struct theta { const char *sym; uint8_t val; int shells[3]; };

static const struct theta thetas[] = {
    {"θρρρ",   0x00, {0,0,0}},
    {"θ¹ρρ",   0x01, {1,0,0}},
    {"θ¹¹ρ",   0x05, {1,1,0}},
    {"θ¹¹¹",   0x15, {1,1,1}},
    {"θ²ρρ",   0x02, {2,0,0}},
    {"θ²¹ρ",   0x06, {2,1,0}},
    {"θ²²ρ",   0x0A, {2,2,0}},
    {"θ²²²",   0x2A, {2,2,2}},
    {"θ²³ρ",   0x0E, {2,3,0}},
    {"θ³ρρ",   0x03, {3,0,0}},
    {"θ³²ρ",   0x0B, {3,2,0}},
    {"θ³³ρ",   0x0F, {3,3,0}},
    {"θρ¹ρ",   0x04, {0,1,0}},
    {"θρ²ρ",   0x08, {0,2,0}},
    {"θρ²²",   0x28, {0,2,2}},
    {"θρ³ρ",   0x0C, {0,3,0}},
    {"θ²ρρ·",  0x82, {2,0,0}},  /* deref arg0 */
    {"θρ²ρ·",  0x48, {0,2,0}},  /* deref arg1 */
    {"θ³ρρ·",  0x83, {3,0,0}},  /* deref arg0 (u64) */
    {NULL, 0, {0,0,0}}
};

/* shell → byte count: 0=0, 1=1, 2=4, 3=8 */
static const int shell_bytes[] = {0, 1, 4, 8};

/* ── tokenizer ─────────────────────────────────────────────── */

static char *tokens[64];
static int  ntokens;

static void tokenize(char *line) {
    ntokens = 0;
    char *p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == ';' || *p == '\n') break;
        tokens[ntokens] = p;
        if (*p == '\'') {
            /* character literal */
            p++;
            tokens[ntokens] = p; /* point past quote */
            while (*p && *p != '\'') p++;
            if (*p) *p++ = 0;
        } else {
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
            if (*p) *p++ = 0;
        }
        ntokens++;
    }
}

/* ── parse a value (hex, decimal, char) ──────────────────── */

static uint64_t parse_val(const char *s) {
    if (s[0] == '0' && s[1] == 'x')
        return strtoull(s, NULL, 16);
    if (s[0] == '-')
        return (uint64_t)(int64_t)strtoll(s, NULL, 10);
    return strtoull(s, NULL, 10);
}

/* ── assemble one line ─────────────────────────────────────── */

static void assemble_line(void) {
    if (ntokens == 0) return;
    int t = 0;

    /* label definition */
    if (strcmp(tokens[t], ":") == 0) {
        t++;
        if (t < ntokens) {
            strcpy(labels[nlabels].name, tokens[t]);
            labels[nlabels].pos = pos;
            nlabels++;
        }
        return;
    }

    /* walker control */
    if (strcmp(tokens[t], "skip_z") == 0 || strcmp(tokens[t], "skip_nz") == 0 ||
        strcmp(tokens[t], "loop_back") == 0) {
        uint8_t ctrl = 0xFE;
        if (strcmp(tokens[t], "skip_nz") == 0) ctrl = 0xFD;
        if (strcmp(tokens[t], "loop_back") == 0) ctrl = 0xFC;
        t++;
        emit8(ctrl);
        /* save fixup — resolve label on pass 2 */
        strcpy(fixups[nfixups].name, tokens[t]);
        fixups[nfixups].pos = pos;
        fixups[nfixups].ctrl = ctrl;
        nfixups++;
        emit32(0); /* placeholder */
        return;
    }

    /* bond symbol */
    for (const struct bond *b = bonds; b->sym; b++) {
        if (strcmp(tokens[t], b->sym) == 0) {
            for (int i = 0; i < b->len; i++)
                emit8(b->bytes[i]);
            t++;
            break;
        }
    }

    /* θ constant + args */
    if (t < ntokens) {
        for (const struct theta *th = thetas; th->sym; th++) {
            if (strcmp(tokens[t], th->sym) == 0) {
                emit8(th->val);
                t++;
                /* emit args based on shell levels */
                for (int a = 0; a < 3 && t < ntokens; a++) {
                    int sz = shell_bytes[th->shells[a]];
                    if (sz == 0) continue; /* pipeline — no bytes */
                    uint64_t v = parse_val(tokens[t]);
                    t++;
                    if (sz == 1) emit8(v);
                    else if (sz == 4) emit32(v);
                    else emit64(v);
                }
                break;
            }
        }
    }
}

/* ── resolve fixups ────────────────────────────────────────── */

static int resolve_fixups(void) {
    for (int i = 0; i < nfixups; i++) {
        int target = find_label(fixups[i].name);
        if (target < 0) {
            fprintf(stderr, "hodos: undefined label '%s'\n", fixups[i].name);
            return 1;
        }
        /* offset = target - (fixup_pos + 4) for skip_z/skip_nz
           offset = (fixup_pos + 4) - target for loop_back */
        int ref = fixups[i].pos + 4; /* past the u32 */
        int offset;
        if (fixups[i].ctrl == 0xFC) /* loop_back */
            offset = ref - target;
        else /* skip_z, skip_nz */
            offset = target - ref;
        uint32_t u = (uint32_t)offset;
        out[fixups[i].pos]     = u & 0xFF;
        out[fixups[i].pos + 1] = (u >> 8) & 0xFF;
        out[fixups[i].pos + 2] = (u >> 16) & 0xFF;
        out[fixups[i].pos + 3] = (u >> 24) & 0xFF;
    }
    return 0;
}

/* ── process file (recursive for @include) ────────────────── */

static int include_depth = 0;

static int process_file(const char *path) {
    if (include_depth > 8) {
        fprintf(stderr, "hodos: include depth > 8: %s\n", path);
        return 1;
    }

    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 1; }

    /* resolve directory of this file for relative includes */
    char dir[MAX_LINE] = ".";
    const char *slash = strrchr(path, '/');
    if (slash) {
        int dlen = (int)(slash - path);
        if (dlen >= MAX_LINE) dlen = MAX_LINE - 1;
        memcpy(dir, path, dlen);
        dir[dlen] = '\0';
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        tokenize(line);
        if (ntokens >= 2 && strcmp(tokens[0], "@include") == 0) {
            char inc_path[MAX_LINE];
            if (tokens[1][0] == '/')
                snprintf(inc_path, sizeof(inc_path), "%s", tokens[1]);
            else
                snprintf(inc_path, sizeof(inc_path), "%s/%s", dir, tokens[1]);
            include_depth++;
            int err = process_file(inc_path);
            include_depth--;
            if (err) { fclose(f); return err; }
            continue;
        }
        assemble_line();
    }
    fclose(f);
    return 0;
}

/* ── main ──────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: hodos input.w [-o output.bin]\n");
        return 1;
    }

    const char *infile = argv[1];
    const char *outfile = "a.out.w";
    for (int i = 2; i < argc - 1; i++)
        if (strcmp(argv[i], "-o") == 0)
            outfile = argv[i + 1];

    if (process_file(infile)) return 1;

    if (resolve_fixups()) return 1;

    FILE *out_f = fopen(outfile, "wb");
    if (!out_f) { perror(outfile); return 1; }
    fwrite(out, 1, pos, out_f);
    fclose(out_f);

    fprintf(stderr, "hodos: %d bytes\n", pos);
    return 0;
}
