#ifndef TOKENIZE_H
#define TOKENIZE_H

#include <stdint.h>

/*
 * tokenize.h — component 1.2: word-boundary tokenizer
 *
 * Sits between raw UTF-8 text and the wave byte encoder.
 * Splits English text into words, numbers, and punctuation tokens.
 * All output is lowercased — case has no meaning in the coordinate system
 * (the T dimension encodes phonetic case, not orthographic case).
 *
 * Design constraints:
 *   - Zero allocations. Caller provides all buffers.
 *   - Tokens are (pointer, length) pairs into the original text.
 *   - Two interfaces: batch (tokenize_line) and streaming (tokenizer_*).
 *
 * Tokenization rules:
 *   1. Split on whitespace (space, tab, newline, CR).
 *   2. Contractions stay together: "don't" = 1 token. The apostrophe
 *      between two letters is a contraction, not a break.
 *   3. Hyphenated compounds stay together: "self-referential" = 1 token.
 *      A hyphen between two letters is a join, not a break.
 *   4. Numbers are tokens: "42", "3.14". A dot between digits is decimal.
 *   5. Punctuation is emitted as its own single-char token.
 *   6. Possessives split: "cat's" → "cat" + "'s". This is the one case
 *      where an apostrophe DOES split. We detect it by checking if the
 *      apostrophe-s appears at word end (not followed by a letter).
 *   7. Everything is lowercased in-place... well, not in-place because
 *      we point into the original buffer. The is_lower flag on Token
 *      marks whether lowering was needed, but the caller should lowercase
 *      when consuming. (We don't modify the source buffer.)
 *
 * Why word tokens and not wave bytes directly?
 *   The wave byte encoder maps individual characters to phonetic coordinates.
 *   But the model needs to know word boundaries — where one concept ends and
 *   another begins. A space in wave bytes is silence (0x00), but we also need
 *   structural tokens for punctuation, and we need to handle the messy edge
 *   cases of English orthography (contractions, possessives, hyphens) before
 *   the text hits the coordinate system.
 */

/* ── Token ──────────────────────────────────────────────────────── */

typedef struct {
    const uint8_t *start;  /* pointer into source buffer */
    int len;               /* byte length of this token */
    int is_punct;          /* 1 if this token is punctuation, 0 if word/number */
} Token;

/* ── Character classification ───────────────────────────────────── */

/*
 * We roll our own instead of pulling in <ctype.h> — we want exact control
 * and no locale surprises. These only handle ASCII, which is fine: UTF-8
 * multibyte sequences have high bits set and fall through to "letter" handling.
 */

static inline int tok_is_space(uint8_t c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static inline int tok_is_letter(uint8_t c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline int tok_is_digit(uint8_t c) {
    return c >= '0' && c <= '9';
}

static inline int tok_is_alnum(uint8_t c) {
    return tok_is_letter(c) || tok_is_digit(c);
}

/*
 * Punctuation that gets its own token.
 * Apostrophes and hyphens are NOT here — they are handled specially
 * (they can join word parts or split them depending on context).
 */
static inline int tok_is_punct(uint8_t c) {
    return c == '.' || c == '!' || c == '?' || c == ',' ||
           c == ';' || c == ':' || c == '"' || c == '(' ||
           c == ')' || c == '[' || c == ']' || c == '{' ||
           c == '}' || c == '/' || c == '\\' || c == '@' ||
           c == '#' || c == '$' || c == '%' || c == '^' ||
           c == '&' || c == '*' || c == '+' || c == '=' ||
           c == '<' || c == '>' || c == '~' || c == '`' ||
           c == '|';
}

static inline uint8_t tok_lower(uint8_t c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

/* ── Batch tokenizer ────────────────────────────────────────────── */

/*
 * tokenize_line — split a text buffer into tokens.
 *
 * Parameters:
 *   text       — input UTF-8 text (does not need to be null-terminated)
 *   text_len   — number of bytes in text
 *   out        — caller-provided array of Token structs
 *   max_tokens — capacity of out[]
 *
 * Returns: number of tokens written to out[] (always <= max_tokens).
 *
 * The returned Token pointers point into the original text buffer.
 * The caller must not free or modify text while tokens are in use.
 *
 * Lowercasing: tokens point at the original bytes (mixed case). The caller
 * is responsible for lowercasing when consuming — use tok_lower() per byte.
 * We don't modify the source buffer.
 */
static int tokenize_line(const char *text, int text_len,
                         Token *out, int max_tokens) {
    const uint8_t *buf = (const uint8_t *)text;
    int pos = 0;       /* current position in buf */
    int ntok = 0;      /* tokens emitted so far */

    while (pos < text_len && ntok < max_tokens) {

        /* skip whitespace */
        while (pos < text_len && tok_is_space(buf[pos]))
            pos++;
        if (pos >= text_len)
            break;

        /*
         * Case 1: standalone punctuation.
         *
         * A dot between two digits is NOT punctuation — it's a decimal point
         * and belongs to the number token. So we only emit '.' as punctuation
         * if it's not digit-dot-digit.
         */
        if (tok_is_punct(buf[pos])) {
            /*
             * Special case: dot as decimal point.
             * If we see digit(s) before dot and digit(s) after, this dot
             * belongs to a number. But since we scan left-to-right, the
             * digits before the dot would already be a token. So we handle
             * decimal dots in the number-scanning path below, not here.
             *
             * Here we just emit it as punctuation.
             */
            out[ntok].start = &buf[pos];
            out[ntok].len = 1;
            out[ntok].is_punct = 1;
            ntok++;
            pos++;
            continue;
        }

        /*
         * Case 2: number token.
         *
         * Starts with a digit. Absorb more digits and at most one decimal
         * point (if followed by another digit).
         */
        if (tok_is_digit(buf[pos])) {
            int start = pos;
            while (pos < text_len && tok_is_digit(buf[pos]))
                pos++;

            /* decimal point? only if digit follows */
            if (pos < text_len - 1 && buf[pos] == '.' && tok_is_digit(buf[pos + 1])) {
                pos++; /* consume the dot */
                while (pos < text_len && tok_is_digit(buf[pos]))
                    pos++;
            }

            out[ntok].start = &buf[start];
            out[ntok].len = pos - start;
            out[ntok].is_punct = 0;
            ntok++;
            continue;
        }

        /*
         * Case 3: word token (starts with a letter or high byte for UTF-8).
         *
         * A "word" absorbs:
         *   - letters and digits (alphanumeric)
         *   - high bytes (>= 0x80, UTF-8 continuation/start bytes)
         *   - apostrophes between letters (contractions)
         *   - hyphens between letters (compounds)
         *
         * Possessive detection:
         *   If the word ends with 's or 'S (apostrophe + s at the boundary),
         *   and the next char is NOT a letter, we split:
         *     "cat's" → "cat" (word) + "'s" (word)
         *   But "don't" does NOT split because after the 't' comes... well,
         *   the 't' is not 's'. Only apostrophe-s at word-end splits.
         *
         *   More precisely: we scan the whole word greedily, then check if
         *   it ends with 's where 's is a possessive (not a contraction).
         *   Contraction patterns: n't, 're, 've, 'll, 'd, 'm.
         *   If it's apostrophe + s and not one of these, it's possessive.
         */
        if (tok_is_letter(buf[pos]) || buf[pos] >= 0x80) {
            int start = pos;

            while (pos < text_len) {
                uint8_t c = buf[pos];

                /* regular word character */
                if (tok_is_alnum(c) || c >= 0x80) {
                    pos++;
                    continue;
                }

                /*
                 * Apostrophe: join if between letters (contraction).
                 * "don't" — the ' is between n and t, both letters → join.
                 * "dogs'" — the ' is after s, but nothing follows → don't join.
                 * "it's"  — the ' is between t and s, both letters → join
                 *           (we'll split possessives in a second pass below).
                 */
                if (c == '\'' && pos > start &&
                    pos + 1 < text_len && tok_is_letter(buf[pos + 1])) {
                    pos++;
                    continue;
                }

                /*
                 * Hyphen: join if between letters (compound word).
                 * "self-referential" → joined.
                 * "end-"             → don't join trailing hyphen.
                 */
                if (c == '-' && pos > start &&
                    pos + 1 < text_len && tok_is_letter(buf[pos + 1])) {
                    pos++;
                    continue;
                }

                /* anything else breaks the word */
                break;
            }

            int word_len = pos - start;

            /*
             * Possessive split.
             *
             * Check if the word ends with apostrophe + 's'/'S'.
             * If so, and it's NOT a known contraction pattern, split it.
             *
             * Known contractions that end in 's':
             *   - "let's" (let + 's = let us)  — this IS a contraction.
             *   - "he's"  (he + 's = he is)    — ambiguous, but we split.
             *   - "it's"  (it + 's = it is)    — contraction, but we split.
             *
             * Decision: we always split 's at word end. This loses "let's"
             * as a contraction but gains correct possessives. The model can
             * learn that "'s" after a noun is possessive and after a pronoun
             * is a contraction. One token either way.
             *
             * We do NOT split:
             *   - n't (don't, can't, won't, isn't...)
             *   - 're (you're, they're, we're...)
             *   - 've (I've, you've, could've...)
             *   - 'll (I'll, you'll, she'll...)
             *   - 'd  (I'd, you'd, he'd...)
             *   - 'm  (I'm)
             * These are true contractions where splitting would lose meaning.
             */
            if (word_len >= 3) {
                uint8_t second_last = tok_lower(buf[start + word_len - 2]);
                uint8_t last = tok_lower(buf[start + word_len - 1]);

                if (second_last == '\'' && last == 's') {
                    /*
                     * Found 's at end. Emit the stem as one token,
                     * then 's as a second token.
                     */
                    int stem_len = word_len - 2;

                    /* emit stem */
                    out[ntok].start = &buf[start];
                    out[ntok].len = stem_len;
                    out[ntok].is_punct = 0;
                    ntok++;

                    /* emit 's as a separate token (it's a word, not punct) */
                    if (ntok < max_tokens) {
                        out[ntok].start = &buf[start + stem_len];
                        out[ntok].len = 2; /* apostrophe + s */
                        out[ntok].is_punct = 0;
                        ntok++;
                    }
                    continue;
                }
            }

            /* no possessive — emit the whole word */
            out[ntok].start = &buf[start];
            out[ntok].len = word_len;
            out[ntok].is_punct = 0;
            ntok++;
            continue;
        }

        /*
         * Case 4: unrecognized byte.
         *
         * Could be a bare apostrophe, bare hyphen, or some other character
         * we didn't categorize. Emit as a single-byte punctuation token
         * and move on. Don't get stuck.
         */
        out[ntok].start = &buf[pos];
        out[ntok].len = 1;
        out[ntok].is_punct = 1;
        ntok++;
        pos++;
    }

    return ntok;
}

/* ── Streaming tokenizer ────────────────────────────────────────── */

/*
 * For processing large files byte-by-byte without loading a full line.
 *
 * Usage:
 *   Tokenizer t;
 *   tokenizer_init(&t, buf, sizeof(buf));
 *   while (read_byte(&byte)) {
 *       int n = tokenizer_feed(&t, byte);
 *       while (n-- > 0) {
 *           Token tok = tokenizer_get(&t);
 *           // use tok
 *       }
 *   }
 *   // flush any remaining token at end of input
 *   int n = tokenizer_finish(&t);
 *   while (n-- > 0) {
 *       Token tok = tokenizer_get(&t);
 *       // use tok
 *   }
 *
 * Internal buffer: the caller provides a scratch buffer at init time.
 * The tokenizer accumulates word bytes there, then emits tokens that
 * point into this scratch buffer. Tokens are only valid until the next
 * call to tokenizer_feed() or tokenizer_finish().
 *
 * The ready[] ring holds tokens waiting to be consumed by tokenizer_get().
 * We need at most 2 tokens ready at once (possessive split: stem + 's).
 */

#define TOK_READY_MAX  4   /* max tokens buffered (possessive = 2, plus margin) */

typedef struct {
    /* scratch buffer for accumulating the current word/number */
    uint8_t *buf;          /* caller-provided scratch space */
    int buf_cap;           /* capacity of buf */
    int buf_len;           /* bytes accumulated so far (relative to buf_base) */
    int buf_base;          /* offset into buf where current accumulation starts */

    /*
     * Why buf_base?
     *
     * When we flush a word, emitted tokens hold pointers into buf[].
     * If the same feed() call then starts accumulating a new word at buf[0],
     * it would overwrite the old token data. So after a flush, we advance
     * buf_base past the old data: new accumulation starts at buf[buf_base].
     * The base resets to 0 at the start of each feed() call (old tokens
     * have been consumed by then).
     */

    /*
     * Separate stash for punctuation bytes emitted after a word flush.
     *
     * Problem: when we flush a word, emitted tokens point into buf[].
     * If we then emit a punctuation byte by writing to buf[0], we'd
     * corrupt the word token data. So punctuation bytes go here instead.
     * We need at most 2 (the NUM_DOT case can emit a dot then another punct).
     */
    uint8_t punct_stash[4];
    int punct_stash_len;

    /* state machine */
    enum {
        TOK_STATE_IDLE,    /* between tokens, eating whitespace */
        TOK_STATE_WORD,    /* inside a word (letters, digits, hyphens, apostrophes) */
        TOK_STATE_NUMBER,  /* inside a number */
        TOK_STATE_NUM_DOT  /* saw a dot after digits, waiting to see if digit follows */
    } state;

    /* output ring: tokens ready for the caller to consume */
    Token ready[TOK_READY_MAX];
    int ready_head;        /* next slot to write */
    int ready_count;       /* tokens available */
} Tokenizer;

/*
 * tokenizer_init — prepare a streaming tokenizer.
 *
 * buf/buf_cap: scratch space for accumulating the current token's bytes.
 * 256 bytes is plenty for any English word. 1024 if you want safety margin.
 */
static inline void tokenizer_init(Tokenizer *t, uint8_t *buf, int buf_cap) {
    t->buf = buf;
    t->buf_cap = buf_cap;
    t->buf_len = 0;
    t->buf_base = 0;
    t->punct_stash_len = 0;
    t->state = TOK_STATE_IDLE;
    t->ready_head = 0;
    t->ready_count = 0;
}

/*
 * Push a token into the ready ring. Internal helper.
 */
static inline void tok_emit(Tokenizer *t, const uint8_t *start, int len, int is_punct) {
    if (t->ready_count >= TOK_READY_MAX)
        return; /* overflow — drop token. caller's buffer was too small. */

    int slot = (t->ready_head + t->ready_count) % TOK_READY_MAX;
    t->ready[slot].start = start;
    t->ready[slot].len = len;
    t->ready[slot].is_punct = is_punct;
    t->ready_count++;
}

/*
 * Emit a single punctuation byte using the stash (not the main buf).
 * This is safe to call after flushing a word because it doesn't touch buf[].
 */
static inline void tok_emit_punct(Tokenizer *t, uint8_t c) {
    if (t->punct_stash_len < (int)sizeof(t->punct_stash)) {
        int idx = t->punct_stash_len++;
        t->punct_stash[idx] = c;
        tok_emit(t, &t->punct_stash[idx], 1, 1);
    }
}

/*
 * Flush the current accumulated word/number in buf as a token (or tokens,
 * if possessive split applies). Internal helper.
 *
 * After flushing, advances buf_base past the emitted data so that new
 * accumulation won't overwrite token pointers from this flush.
 */
static inline void tok_flush_word(Tokenizer *t) {
    if (t->buf_len == 0)
        return;

    uint8_t *word_start = t->buf + t->buf_base;
    int orig_len = t->buf_len;

    /*
     * Strip trailing hyphens and apostrophes.
     *
     * In streaming mode we absorb hyphens/apostrophes optimistically
     * (can't look ahead to see if a letter follows). If the word ends
     * with one, it was a false join — strip it and emit as punctuation.
     * The stripped chars still live in the buffer so we can emit them.
     */
    int word_len = orig_len;
    while (word_len > 0) {
        uint8_t last = word_start[word_len - 1];
        if (last == '-' || last == '\'') {
            word_len--;
        } else {
            break;
        }
    }

    /*
     * Check for possessive 's at end.
     * Same logic as the batch tokenizer: if word ends with 's, split.
     */
    if (word_len >= 3) {
        uint8_t second_last = tok_lower(word_start[word_len - 2]);
        uint8_t last = tok_lower(word_start[word_len - 1]);

        if (second_last == '\'' && last == 's') {
            int stem_len = word_len - 2;
            tok_emit(t, word_start, stem_len, 0);
            tok_emit(t, word_start + stem_len, 2, 0);
            /* emit any stripped trailing chars as punct */
            for (int i = word_len; i < orig_len; i++)
                tok_emit_punct(t, word_start[i]);
            t->buf_base += orig_len;
            t->buf_len = 0;
            return;
        }
    }

    if (word_len > 0) {
        tok_emit(t, word_start, word_len, 0);
    }

    /* emit stripped trailing chars as individual punct tokens */
    for (int i = word_len; i < orig_len; i++)
        tok_emit_punct(t, word_start[i]);

    t->buf_base += orig_len;
    t->buf_len = 0;
}

/*
 * Append a byte to the accumulation buffer. Internal helper.
 * Writes at buf[buf_base + buf_len].
 */
static inline void tok_accum(Tokenizer *t, uint8_t c) {
    int pos = t->buf_base + t->buf_len;
    if (pos < t->buf_cap)
        t->buf[pos] = c;
    t->buf_len++;
    /* silently drop if buffer full — word is absurdly long */
}

/*
 * tokenizer_feed — feed one byte into the streaming tokenizer.
 *
 * Returns: number of tokens now available via tokenizer_get().
 *          Can be 0 (byte absorbed), 1 (token emitted), or 2 (possessive split).
 *
 * The caller should call tokenizer_get() that many times to drain the tokens.
 */
static int tokenizer_feed(Tokenizer *t, uint8_t c) {
    t->ready_count = 0;   /* clear previous output */
    t->ready_head = 0;
    t->punct_stash_len = 0; /* reset punct stash for this feed cycle */

    /*
     * Reset buf_base: any tokens from the previous feed() have been
     * consumed by the caller, so we can reuse the whole buffer.
     * But if we're mid-word, we need to compact the live data first.
     */
    if (t->buf_base > 0 && t->buf_len > 0) {
        /* mid-accumulation: shift live bytes to start of buffer */
        for (int i = 0; i < t->buf_len; i++)
            t->buf[i] = t->buf[t->buf_base + i];
    }
    t->buf_base = 0;

    switch (t->state) {

    case TOK_STATE_IDLE:
        /* between tokens — classify the incoming byte */
        if (tok_is_space(c)) {
            /* still idle, eat whitespace */
            return 0;
        }
        if (tok_is_punct(c)) {
            /*
             * Punctuation: emit immediately as a single-char token.
             * Use the punct stash so we don't touch the main buf.
             */
            tok_emit_punct(t, c);
            return t->ready_count;
        }
        if (tok_is_digit(c)) {
            t->state = TOK_STATE_NUMBER;
            t->buf_len = 0;
            tok_accum(t, c);
            return 0;
        }
        if (tok_is_letter(c) || c >= 0x80) {
            t->state = TOK_STATE_WORD;
            t->buf_len = 0;
            tok_accum(t, c);
            return 0;
        }
        /*
         * Unrecognized byte (bare apostrophe, bare hyphen, control char...).
         * Emit as punctuation.
         */
        tok_emit_punct(t, c);
        return t->ready_count;

    case TOK_STATE_WORD:
        /*
         * Inside a word. Continue if alnum, high byte, or joining punct.
         */
        if (tok_is_alnum(c) || c >= 0x80) {
            tok_accum(t, c);
            return 0;
        }

        /*
         * Apostrophe inside a word: could be contraction ("don't") or
         * possessive boundary ("cat's"). We greedily absorb it — the
         * possessive check happens when we flush.
         *
         * But we only absorb if the PREVIOUS byte was a letter (not at
         * word start) — a leading apostrophe is punctuation.
         */
        if (c == '\'' && t->buf_len > 0 && tok_is_letter(t->buf[t->buf_base + t->buf_len - 1])) {
            tok_accum(t, c);
            return 0;
        }

        /*
         * Hyphen inside a word: absorb if previous byte was a letter.
         * We can't look ahead in the streaming case, so we absorb the
         * hyphen and hope a letter follows. If not, it'll dangle at
         * the word end — harmless, the model sees it.
         */
        if (c == '-' && t->buf_len > 0 && tok_is_letter(t->buf[t->buf_base + t->buf_len - 1])) {
            tok_accum(t, c);
            return 0;
        }

        /*
         * Anything else: the word is done. Flush it, then handle this
         * byte as if we were idle.
         */
        tok_flush_word(t);
        t->state = TOK_STATE_IDLE;

        /*
         * Now process the current byte `c` in idle context.
         * We could recurse, but let's just inline the logic to keep
         * the call stack flat (this runs on every byte of Wikipedia).
         */
        if (tok_is_space(c)) {
            /* nothing to do */
        } else if (tok_is_punct(c)) {
            tok_emit_punct(t, c);
        } else if (tok_is_digit(c)) {
            t->state = TOK_STATE_NUMBER;
            t->buf_len = 0;
            tok_accum(t, c);
        } else if (tok_is_letter(c) || c >= 0x80) {
            t->state = TOK_STATE_WORD;
            t->buf_len = 0;
            tok_accum(t, c);
        } else {
            /* unrecognized — emit as punct */
            tok_emit_punct(t, c);
        }
        return t->ready_count;

    case TOK_STATE_NUMBER:
        if (tok_is_digit(c)) {
            tok_accum(t, c);
            return 0;
        }
        if (c == '.' && t->buf_len > 0) {
            /*
             * Could be decimal point. We need to look ahead to see if a
             * digit follows. In streaming mode we can't, so we transition
             * to a tentative state.
             */
            tok_accum(t, c); /* stash the dot */
            t->state = TOK_STATE_NUM_DOT;
            return 0;
        }
        if (tok_is_letter(c) || c >= 0x80) {
            /*
             * Letter after number: "42nd", "3D". Could absorb into a
             * word-like token, but for simplicity we break here.
             * The number becomes its own token, the letter starts a word.
             */
            tok_flush_word(t); /* flush number */
            t->state = TOK_STATE_WORD;
            t->buf_len = 0;
            tok_accum(t, c);
            return t->ready_count;
        }

        /* end of number — flush and handle c as idle */
        tok_flush_word(t);
        t->state = TOK_STATE_IDLE;

        if (tok_is_space(c)) {
            /* nothing */
        } else if (tok_is_punct(c)) {
            tok_emit_punct(t, c);
        } else {
            tok_emit_punct(t, c);
        }
        return t->ready_count;

    case TOK_STATE_NUM_DOT:
        /*
         * We just saw digits followed by a dot. If a digit comes now,
         * the dot is decimal and we continue the number. Otherwise,
         * the dot is punctuation — we need to back up.
         */
        if (tok_is_digit(c)) {
            /* decimal point confirmed — continue number */
            tok_accum(t, c);
            t->state = TOK_STATE_NUMBER;
            return 0;
        }

        /*
         * Not a decimal point. The dot we already accumulated was actually
         * end-of-sentence punctuation (or similar). We need to:
         *   1. Remove the dot from buf (it's the last byte).
         *   2. Flush the number without the dot.
         *   3. Emit the dot as punctuation.
         *   4. Handle the current byte c.
         */
        t->buf_len--; /* remove the dot */
        tok_flush_word(t); /* emit number */

        /* emit the dot as punctuation via stash (safe after flush) */
        tok_emit_punct(t, '.');

        t->state = TOK_STATE_IDLE;

        /* now handle c */
        if (tok_is_space(c)) {
            /* nothing */
        } else if (tok_is_digit(c)) {
            t->state = TOK_STATE_NUMBER;
            t->buf_len = 0;
            tok_accum(t, c);
        } else if (tok_is_letter(c) || c >= 0x80) {
            t->state = TOK_STATE_WORD;
            t->buf_len = 0;
            tok_accum(t, c);
        } else if (tok_is_punct(c)) {
            tok_emit_punct(t, c);
        } else {
            tok_emit_punct(t, c);
        }
        return t->ready_count;
    }

    /* unreachable, but silence the compiler */
    return 0;
}

/*
 * tokenizer_get — pop the next ready token.
 *
 * Call this after tokenizer_feed() returns > 0, once per token.
 * Returns a Token whose .start points into the tokenizer's scratch buffer.
 * The token data is only valid until the next tokenizer_feed() call.
 *
 * If no tokens are available, returns a zero-length token.
 */
static inline Token tokenizer_get(Tokenizer *t) {
    if (t->ready_count <= 0) {
        Token empty = {0, 0, 0};
        return empty;
    }
    Token tok = t->ready[t->ready_head];
    t->ready_head = (t->ready_head + 1) % TOK_READY_MAX;
    t->ready_count--;
    return tok;
}

/*
 * tokenizer_finish — flush any remaining token at end of input.
 *
 * Must be called when the input stream ends to emit the last token
 * (which hasn't been terminated by whitespace/punct yet).
 *
 * Returns: number of tokens available via tokenizer_get().
 */
static int tokenizer_finish(Tokenizer *t) {
    t->ready_count = 0;
    t->ready_head = 0;
    t->punct_stash_len = 0;
    /* no buf_base reset here — tokens from finish must stay valid */

    if (t->state == TOK_STATE_WORD || t->state == TOK_STATE_NUMBER) {
        tok_flush_word(t);
    } else if (t->state == TOK_STATE_NUM_DOT) {
        /*
         * Input ended right after "123." — the dot is ambiguous.
         * Emit the digits as a number and the dot as punctuation.
         */
        t->buf_len--; /* remove dot */
        tok_flush_word(t);
        tok_emit_punct(t, '.');
    }

    t->state = TOK_STATE_IDLE;
    return t->ready_count;
}

/* ── Utility: lowercase copy ────────────────────────────────────── */

/*
 * tok_lower_copy — copy a token's bytes to a destination buffer, lowercased.
 *
 * dst must have room for at least tok.len bytes (no null terminator added).
 * Returns tok.len for convenience.
 *
 * This is the standard way to consume a token in the lattice pipeline:
 *   uint8_t word[256];
 *   int n = tok_lower_copy(&tok, word);
 *   for (int i = 0; i < n; i++)
 *       wave_buf[i] = utf8_to_wave(word[i]);
 */
static inline int tok_lower_copy(const Token *tok, uint8_t *dst) {
    for (int i = 0; i < tok->len; i++)
        dst[i] = tok_lower(tok->start[i]);
    return tok->len;
}

#endif /* TOKENIZE_H */
