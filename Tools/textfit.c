// One-off tool: does every card description still fit inside its card?
//
// Not part of the game. Like fontbake, this reads main.c rather than being told
// what to check - a hand-kept list of strings is a list that drifts, and the
// way this problem shows up is one card in one language spilling past its box
// in a build nobody looked at closely.
//
//   textfit.exe <main.c> [neodgm16.h is compiled in]
//
// It pulls the card table and the phrase table out of the source, folds each
// Korean description the same way DrawCardDesc does, and reports anything that
// still will not fit - either too wide for the box or too tall for the rows the
// screen has to spare.
//
// The fold here MUST match DrawCardDesc. If that function changes, change this
// one in the same commit, or the tool starts lying in the reassuring direction.
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "neodgm16.h"

#define MAX_CARDS 128
#define MAX_LINES 4
#define MAX_PHRASE 512
#define STRBUF 256

typedef struct { char name[STRBUF]; char d[MAX_LINES][STRBUF]; int n; } CardRow;
typedef struct { char en[STRBUF]; char ko[STRBUF]; } PhraseRow;

static CardRow   cards[MAX_CARDS];   static int cardN;
static PhraseRow phrases[MAX_PHRASE]; static int phraseN;

static Font F16, F32;
static Font FontFor(int size) { return (size <= 22) ? F16 : F32; }
static int Measure(const char* s, int fs) {
    return (int)MeasureTextEx(FontFor(fs), s, (float)fs, (float)fs / 10.0f).x;
}

// Every screen that draws a card, with the width it actually has to draw in and
// the number of rows before it runs into whatever is underneath. These come
// from the DrawCardDesc call sites; keep them in step by hand, because the
// alternative is parsing the layout out of C and that is a worse bargain.
typedef struct { const char* screen; int fs; int maxW; int rows; } Screen;
static const Screen screens[] = {
    { "draft",   16, 248, 5 },
    { "shop",    16, 248, 4 },
    { "codex",   15, 410, 4 },
    { "result",  15, 410, 4 },
    { "pending", 14, 280, 4 },
};
#define SCREEN_N ((int)(sizeof screens / sizeof screens[0]))

static char* Slurp(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char* b = (char*)malloc((size_t)n + 1);
    if (!b) { fclose(f); return NULL; }
    size_t got = fread(b, 1, (size_t)n, f);
    b[got] = '\0';
    fclose(f);
    return b;
}

// Copy the C string literal starting at *p (which must point at the opening
// quote) into out. Returns the position just past the closing quote.
static const char* ReadLiteral(const char* p, char* out, size_t cap) {
    size_t k = 0;
    p++;                                    // opening quote
    while (*p && *p != '"') {
        if (*p == '\\' && p[1]) p++;        // no escapes in this table, but be safe
        if (k + 1 < cap) out[k++] = *p;
        p++;
    }
    out[k] = '\0';
    return *p ? p + 1 : p;
}

static void ParsePhrases(const char* src) {
    const char* p = strstr(src, "static const Phrase phrases[] = {");
    if (!p) return;
    const char* end = strstr(p, "\n};");
    for (; p && p < end; ) {
        const char* open = strchr(p, '{');
        if (!open || open >= end) break;
        const char* q = strchr(open, '"');
        if (!q || q >= end) break;
        if (phraseN >= MAX_PHRASE) break;
        q = ReadLiteral(q, phrases[phraseN].en, STRBUF);
        const char* q2 = strchr(q, '"');
        if (!q2 || q2 >= end) break;
        ReadLiteral(q2, phrases[phraseN].ko, STRBUF);
        phraseN++;
        p = strchr(q2 + 1, '\n');
        if (!p) break;
    }
}

static const char* T(const char* en) {
    for (int i = 0; i < phraseN; i++)
        if (strcmp(phrases[i].en, en) == 0) return phrases[i].ko;
    return en;
}

static void ParseCards(const char* src) {
    const char* p = strstr(src, "static const Card cards[] = {");
    if (!p) return;
    const char* end = strstr(p, "\n};");
    p = strchr(p, '\n');
    while (p && p < end) {
        const char* nl = strchr(p + 1, '\n');
        // A row starts with `{ "` after leading space; rows may wrap onto the
        // next line, so scan from the row's `{` up to its terminating `},`.
        const char* brace = strstr(p, "{ \"");
        if (!brace || brace >= end) break;
        const char* stop = strstr(brace, "},");
        if (!stop || stop > end) stop = end;
        if (cardN >= MAX_CARDS) break;

        CardRow* c = &cards[cardN];
        memset(c, 0, sizeof *c);
        const char* q = brace;
        int slot = -1;                       // -1 = name, then 0..3
        while ((q = strchr(q, '"')) != NULL && q < stop) {
            char buf[STRBUF];
            q = ReadLiteral(q, buf, STRBUF);
            if (slot < 0) { strncpy(c->name, buf, STRBUF - 1); slot = 0; }
            else if (slot < MAX_LINES) { strncpy(c->d[slot], buf, STRBUF - 1); slot++; }
        }
        c->n = (slot > 0) ? slot : 0;
        cardN++;
        p = (stop < end) ? stop + 2 : end;
        (void)nl;
    }
}

// The same fold DrawCardDesc performs. Returns the number of rows used and, via
// `worst`, the widest fragment it produced.
static int Fold(const char* s, int fs, int maxW, int* worst) {
    int rows = 0;
    *worst = 0;
    while (*s) {
        while (*s == ' ') s++;
        if (!*s) break;
        char buf[STRBUF];
        int cut = 0, depth = 0;
        for (const char* q = s; ; q++) {
            if (*q == '(') { depth++; continue; }
            if (*q == ')') { if (depth > 0) depth--; continue; }
            if (*q != ' ' && *q != '\0') continue;
            if (*q == ' ' && depth > 0) continue;
            int len = (int)(q - s);
            if (len <= 0 || len >= STRBUF) break;
            memcpy(buf, s, (size_t)len); buf[len] = '\0';
            if (Measure(buf, fs) > maxW) break;
            cut = len;
            if (!*q) break;
        }
        if (cut <= 0) cut = (int)strlen(s);
        memcpy(buf, s, (size_t)cut); buf[cut] = '\0';
        int w = Measure(buf, fs);
        if (w > *worst) *worst = w;
        rows++;
        s += cut;
    }
    return rows;
}

int main(int argc, char** argv) {
    // raylib tears the process down hard enough that a buffered stdout can be
    // lost on exit. Unbuffered from the start, so a report always reaches the
    // caller even when the exit code is ugly.
    setvbuf(stdout, NULL, _IONBF, 0);

    const char* path = (argc > 1) ? argv[1] : "main.c";

    // Optional width override, applied to every screen. Two uses: proving this
    // tool still reports anything at all (a checker that has never failed is a
    // checker nobody has tested), and asking what a narrower card would cost.
    int forceW = (argc > 2) ? atoi(argv[2]) : 0;
    char* src = Slurp(path);
    if (!src) { printf("cannot read %s\n", path); return 1; }

    ParsePhrases(src);
    ParseCards(src);

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(64, 64, "textfit");
    F16 = LoadFont_Neodgm16();
    F32 = F16;                    // only the small face is used by card screens

    int bad = 0;
    for (int s = 0; s < SCREEN_N; s++) {
        Screen sc2 = screens[s];
        if (forceW > 0) sc2.maxW = forceW;
        const Screen* sc = &sc2;
        for (int i = 0; i < cardN; i++) {
            CardRow* c = &cards[i];
            int rows = 0, over = 0, worstW = 0;
            for (int L = 0; L < c->n; L++) {
                int w;
                rows += Fold(T(c->d[L]), sc->fs, sc->maxW, &w);
                if (w > sc->maxW) { over = 1; if (w > worstW) worstW = w; }
            }
            if (over) {
                printf("[%-7s] %-16s TOO WIDE  %dpx > %dpx  (한 낱말이 상자보다 넓다)\n",
                       sc->screen, c->name, worstW, sc->maxW);
                bad++;
            }
            if (rows > sc->rows) {
                printf("[%-7s] %-16s TOO TALL  %d행 > %d행\n",
                       sc->screen, c->name, rows, sc->rows);
                bad++;
            }
        }
    }

    printf("\ncards: %d   phrases: %d   problems: %d\n", cardN, phraseN, bad);
    if (!bad) printf("every card description fits every screen.\n");

    UnloadFont(F16);
    CloseWindow();
    free(src);
    return bad ? 1 : 0;
}
