#include "ollama.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* The header keyword offsets this sample is aligned to (the parser derives
   these dynamically via strstr on the header line; we build the data rows to
   match so the fixed column-offset slicer parses every field correctly,
   including a multi-token PROCESSOR column). */
static const int COL_OFF[6] = {0, 18, 26, 34, 52, 64}; /* NAME ID SIZE PROCESSOR CONTEXT UNTIL */

/* Build a column-aligned ollama ps sample. Each field is left-justified into
   its column width so offsets line up with the header keywords. */
static void build_sample(char *buf, size_t sz) {
    snprintf(buf, sz,
        "%-*s%-*s%-*s%-*s%-*s%s\n",
        COL_OFF[1], "NAME",
        COL_OFF[2] - COL_OFF[1], "ID",
        COL_OFF[3] - COL_OFF[2], "SIZE",
        COL_OFF[4] - COL_OFF[3], "PROCESSOR",
        COL_OFF[5] - COL_OFF[4], "CONTEXT",
        "UNTIL");

    char *p = buf + strlen(buf);
    snprintf(p, sz - (p - buf),
        "%-*s%-*s%-*s%-*s%-*s%s\n",
        COL_OFF[1], "llama3:8b",
        COL_OFF[2] - COL_OFF[1], "abc123",
        COL_OFF[3] - COL_OFF[2], "1.8 GB",
        COL_OFF[4] - COL_OFF[3], "100% GPU",
        COL_OFF[5] - COL_OFF[4], "8192 MB",
        "4m");

    p = buf + strlen(buf);
    snprintf(p, sz - (p - buf),
        "%-*s%-*s%-*s%-*s%-*s%s\n",
        COL_OFF[1], "nomic-embed-text",
        COL_OFF[2] - COL_OFF[1], "def456",
        COL_OFF[3] - COL_OFF[2], "0.7 GB",
        COL_OFF[4] - COL_OFF[3], "GPU",
        COL_OFF[5] - COL_OFF[4], "2048 MB",
        "Less than 5s");

    p = buf + strlen(buf);
    snprintf(p, sz - (p - buf),
        "%-*s%-*s%-*s%-*s%-*s%s\n",
        COL_OFF[1], "mixed-model",
        COL_OFF[2] - COL_OFF[1], "ghi789",
        COL_OFF[3] - COL_OFF[2], "3.2 GB",
        COL_OFF[4] - COL_OFF[3], "42% GPU, 58% CPU",
        COL_OFF[5] - COL_OFF[4], "4096 MB",
        "12m");
}

int main(void) {
    char sample[512];
    build_sample(sample, sizeof(sample));

    ollama_row rows[8];
    size_t n = 0;
    int rc = parse_ollama_ps(sample, rows, 8, &n);
    assert(rc == 0);
    assert(n == 3);
    assert(strcmp(rows[0].name, "llama3:8b") == 0);
    assert(strcmp(rows[0].id, "abc123") == 0);
    assert(strcmp(rows[0].size, "1.8 GB") == 0);
    assert(strcmp(rows[0].processor, "100% GPU") == 0);
    assert(strcmp(rows[0].context, "8192 MB") == 0);
    assert(strcmp(rows[0].until, "4m") == 0);
    assert(strcmp(rows[1].name, "nomic-embed-text") == 0);
    assert(strcmp(rows[1].id, "def456") == 0);
    assert(strcmp(rows[1].size, "0.7 GB") == 0);
    assert(strcmp(rows[1].processor, "GPU") == 0);
    assert(strcmp(rows[1].context, "2048 MB") == 0);
    assert(strcmp(rows[1].until, "Less than 5s") == 0);
    assert(strcmp(rows[2].name, "mixed-model") == 0);
    assert(strcmp(rows[2].id, "ghi789") == 0);
    assert(strcmp(rows[2].size, "3.2 GB") == 0);
    assert(strcmp(rows[2].processor, "42% GPU, 58% CPU") == 0);
    assert(strcmp(rows[2].context, "4096 MB") == 0);
    assert(strcmp(rows[2].until, "12m") == 0);

    /* malformed: no "NAME" header -> -1 */
    size_t n2 = 99;
    assert(parse_ollama_ps("garbage no header\n", rows, 8, &n2) == -1);

    printf("test_ollama PASS\n");
    return 0;
}
