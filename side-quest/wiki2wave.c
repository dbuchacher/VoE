/*
 * wiki2wave.c — extract Wikipedia article text and convert to wave bytes
 *
 * Reads bz2-compressed Wikipedia XML dump from stdin (via bzcat pipe).
 * Extracts plain text from <text> elements, strips mediawiki markup.
 * Outputs wave byte sequences — one article per line, wave bytes as raw binary.
 *
 * Usage:
 *   bzcat data/enwiki-latest-pages-articles.xml.bz2 | ./wiki2wave > data/wiki.wave
 *
 * Or for text preview:
 *   bzcat data/enwiki-latest-pages-articles.xml.bz2 | ./wiki2wave -t | head
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lattice.h"

#define MAX_LINE (1024 * 1024)  /* 1MB line buffer */
#define MAX_TEXT (16 * 1024 * 1024) /* 16MB article buffer */

static int text_mode = 0;  /* -t flag: output decoded text instead of raw bytes */
static long articles = 0;
static long total_bytes = 0;

/*
 * Strip mediawiki markup from article text.
 * Not perfect — just enough to get clean prose.
 */
static int strip_markup(const char *src, char *dst, int maxlen) {
    int si = 0, di = 0;
    int in_template = 0;  /* {{ ... }} depth */
    int in_ref = 0;       /* <ref ... /> or <ref>...</ref> */
    int in_comment = 0;   /* <!-- ... --> */
    int in_tag = 0;       /* generic < ... > */

    while (src[si] && di < maxlen - 1) {
        /* skip comments */
        if (!in_comment && src[si] == '<' && src[si+1] == '!' &&
            src[si+2] == '-' && src[si+3] == '-') {
            in_comment = 1; si += 4; continue;
        }
        if (in_comment) {
            if (src[si] == '-' && src[si+1] == '-' && src[si+2] == '>') {
                in_comment = 0; si += 3;
            } else {
                si++;
            }
            continue;
        }

        /* skip templates {{ ... }} (nested) */
        if (src[si] == '{' && src[si+1] == '{') {
            in_template++; si += 2; continue;
        }
        if (in_template && src[si] == '}' && src[si+1] == '}') {
            in_template--; si += 2; continue;
        }
        if (in_template) { si++; continue; }

        /* skip ref tags */
        if (src[si] == '<' && (src[si+1] == 'r' || src[si+1] == 'R') &&
            src[si+2] == 'e' && src[si+3] == 'f') {
            /* find closing > or /> */
            while (src[si] && src[si] != '>') si++;
            if (src[si] == '>') si++;
            /* if not self-closing, skip to </ref> */
            if (si >= 2 && src[si-2] != '/') {
                const char *end = strstr(&src[si], "</ref>");
                if (end) si = (int)(end - src) + 6;
            }
            continue;
        }

        /* skip generic HTML tags */
        if (src[si] == '<') {
            while (src[si] && src[si] != '>') si++;
            if (src[si] == '>') si++;
            continue;
        }

        /* wiki links [[target|display]] → keep display (or target if no pipe) */
        if (src[si] == '[' && src[si+1] == '[') {
            si += 2;
            const char *end = strstr(&src[si], "]]");
            if (end) {
                /* find pipe */
                const char *pipe = NULL;
                for (const char *p = &src[si]; p < end; p++) {
                    if (*p == '|') { pipe = p; break; }
                }
                const char *text_start = pipe ? pipe + 1 : &src[si];
                int text_len = (int)(end - text_start);
                if (di + text_len < maxlen - 1) {
                    memcpy(&dst[di], text_start, text_len);
                    di += text_len;
                }
                si = (int)(end - src) + 2;
            }
            continue;
        }

        /* external links [url text] → keep text */
        if (src[si] == '[' && src[si+1] != '[') {
            si++;
            /* skip URL */
            while (src[si] && src[si] != ' ' && src[si] != ']') si++;
            if (src[si] == ' ') si++; /* skip space after URL */
            /* copy display text */
            while (src[si] && src[si] != ']') {
                if (di < maxlen - 1) dst[di++] = src[si];
                si++;
            }
            if (src[si] == ']') si++;
            continue;
        }

        /* bold/italic: '''bold''' ''italic'' → strip quotes */
        if (src[si] == '\'' && src[si+1] == '\'') {
            while (src[si] == '\'') si++;
            continue;
        }

        /* headings: == Heading == → keep text, add newline */
        if (src[si] == '=' && src[si+1] == '=') {
            while (src[si] == '=') si++;
            while (src[si] == ' ') si++;
            if (di > 0 && dst[di-1] != '\n') dst[di++] = '\n';
            continue;
        }

        /* normal character — keep it */
        dst[di++] = src[si++];
    }

    dst[di] = '\0';
    return di;
}

/*
 * Check if article should be skipped (redirects, disambiguation, etc.)
 */
static int should_skip(const char *text) {
    if (strncasecmp(text, "#REDIRECT", 9) == 0) return 1;
    if (strncasecmp(text, "#redirect", 9) == 0) return 1;
    return 0;
}

/*
 * Convert clean text to wave bytes and output.
 * Each article becomes a sequence terminated by 0xFF (separator token).
 */
static void emit_article(const char *clean, int len) {
    if (len < 50) return; /* skip very short articles */

    for (int i = 0; i < len; i++) {
        uint8_t wb = utf8_to_wave((uint8_t)clean[i]);

        if (text_mode) {
            /* text mode: show the wave byte as hex + original char */
            if (isprint(clean[i]))
                printf("%c", clean[i]);
            else if (clean[i] == '\n')
                printf("\n");
        } else {
            /* binary mode: raw wave bytes */
            fputc(wb, stdout);
        }
        total_bytes++;
    }

    /* article separator */
    if (text_mode)
        printf("\n---\n");
    else
        fputc(0xFF, stdout); /* 0xFF = separator (not a valid shell-1 wave byte) */

    articles++;
    if (articles % 10000 == 0)
        fprintf(stderr, "\r%ld articles, %ld MB wave bytes",
                articles, total_bytes / (1024*1024));
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-t") == 0)
        text_mode = 1;

    char *line = malloc(MAX_LINE);
    char *article = malloc(MAX_TEXT);
    char *clean = malloc(MAX_TEXT);
    if (!line || !article || !clean) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    int in_text = 0;
    int article_len = 0;

    while (fgets(line, MAX_LINE, stdin)) {
        /* find <text> opening tag */
        if (!in_text) {
            char *start = strstr(line, "<text");
            if (start) {
                /* find end of opening tag */
                char *gt = strchr(start, '>');
                if (gt) {
                    in_text = 1;
                    article_len = 0;
                    /* copy content after > */
                    gt++;
                    /* check for </text> on same line */
                    char *end = strstr(gt, "</text>");
                    if (end) {
                        int len = (int)(end - gt);
                        if (len > 0 && article_len + len < MAX_TEXT) {
                            memcpy(article + article_len, gt, len);
                            article_len += len;
                        }
                        article[article_len] = '\0';
                        in_text = 0;
                        /* process article */
                        if (!should_skip(article)) {
                            int clen = strip_markup(article, clean, MAX_TEXT);
                            emit_article(clean, clen);
                        }
                    } else {
                        int len = strlen(gt);
                        if (article_len + len < MAX_TEXT) {
                            memcpy(article + article_len, gt, len);
                            article_len += len;
                        }
                    }
                }
            }
            continue;
        }

        /* inside <text> — accumulate or find </text> */
        char *end = strstr(line, "</text>");
        if (end) {
            int len = (int)(end - line);
            if (article_len + len < MAX_TEXT) {
                memcpy(article + article_len, line, len);
                article_len += len;
            }
            article[article_len] = '\0';
            in_text = 0;
            if (!should_skip(article)) {
                int clen = strip_markup(article, clean, MAX_TEXT);
                emit_article(clean, clen);
            }
        } else {
            int len = strlen(line);
            if (article_len + len < MAX_TEXT) {
                memcpy(article + article_len, line, len);
                article_len += len;
            }
        }
    }

    fprintf(stderr, "\rdone: %ld articles, %ld MB wave bytes\n",
            articles, total_bytes / (1024*1024));

    free(line);
    free(article);
    free(clean);
    return 0;
}
