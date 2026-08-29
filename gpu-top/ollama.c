#include "ollama.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static void trim(char *s) {
    if (!s) return;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

int parse_ollama_ps(const char *text, ollama_row *out, size_t cap, size_t *n) {
    *n = 0;
    if (!text) return -1;
    const char *line = text;
    const char *eol = strchr(line, '\n');
    size_t hdr_len = eol ? (size_t)(eol - line) : strlen(line);
    const char *kws[6] = {"NAME","ID","SIZE","PROCESSOR","CONTEXT","UNTIL"};
    int cols[6];
    for (int i = 0; i < 6; i++) {
        const char *k = strstr(line, kws[i]);
        if (!k) return -1;
        cols[i] = (int)(k - line);
    }
    const char *p = eol ? eol + 1 : line + hdr_len;
    while (*p) {
        while (*p == '\n') p++;
        if (!*p) break;
        const char *le = strchr(p, '\n');
        size_t llen = le ? (size_t)(le - p) : strlen(p);
        if (llen == 0) { p = le ? le + 1 : p + llen; continue; }
        if (*n >= cap) break;
        ollama_row *r = &out[*n];
        for (int i = 0; i < 6; i++) {
            int start = cols[i];
            int end = (i < 5) ? cols[i + 1] : (int)llen;
            if (start < 0) start = 0;
            if (start > (int)llen) start = (int)llen;
            if (end > (int)llen) end = (int)llen;
            if (end < start) end = start;
            char buf[160];
            int blen = end - start;
            if (blen >= (int)sizeof(buf)) blen = sizeof(buf) - 1;
            memcpy(buf, p + start, blen);
            buf[blen] = '\0';
            trim(buf);
            switch (i) {
                case 0: snprintf(r->name,      sizeof(r->name),      "%.*s", (int)sizeof(r->name)-1,      buf); break;
                case 1: snprintf(r->id,        sizeof(r->id),        "%.*s", (int)sizeof(r->id)-1,        buf); break;
                case 2: snprintf(r->size,      sizeof(r->size),      "%.*s", (int)sizeof(r->size)-1,      buf); break;
                case 3: snprintf(r->processor, sizeof(r->processor), "%.*s", (int)sizeof(r->processor)-1, buf); break;
                case 4: snprintf(r->context,   sizeof(r->context),   "%.*s", (int)sizeof(r->context)-1,   buf); break;
                case 5: snprintf(r->until,     sizeof(r->until),     "%.*s", (int)sizeof(r->until)-1,     buf); break;
            }
        }
        (*n)++;
        p = le ? le + 1 : p + llen;
    }
    return 0;
}
