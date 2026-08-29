#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parse a line of the form "key = value". Whitespace around '=' is optional.
   Recognized keys: refresh, device, warn, crit. Returns 1 if a known key was
   matched and stored, else 0 (blank lines, comments, and unknown keys). */
int config_parse_line(const char *line, gpu_top_config *c) {
    if (!line || !c) return 0;

    /* skip leading whitespace */
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '#') return 0;

    /* find '=' */
    const char *eq = strchr(p, '=');
    if (!eq) return 0;

    /* key = p .. eq, trimmed on the right */
    size_t kl = (size_t)(eq - p);
    while (kl > 0 && (p[kl - 1] == ' ' || p[kl - 1] == '\t')) kl--;
    if (kl == 0 || kl >= 64) return 0;
    char key[64];
    memcpy(key, p, kl);
    key[kl] = '\0';

    /* value = after '=', trimmed on both sides and trailing newline */
    const char *val = eq + 1;
    while (*val == ' ' || *val == '\t') val++;
    char value[64];
    size_t i = 0, j = 0;
    while (val[j] && val[j] != '\n' && val[j] != '\r' && i < sizeof value - 1)
        value[i++] = val[j++];
    value[i] = '\0';
    while (i > 0) {
        char ch = value[i - 1];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
            value[--i] = '\0';
        else
            break;
    }

    /* empty value (e.g. "device =") is malformed: ignore rather than clobber */
    if (value[0] == '\0') return 0;

    if (strcmp(key, "refresh") == 0) { c->refresh_s = atoi(value); return 1; }
    if (strcmp(key, "device")   == 0) { c->device    = atoi(value); return 1; }
    if (strcmp(key, "warn")     == 0) { c->warn      = atoi(value); return 1; }
    if (strcmp(key, "crit")     == 0) { c->crit      = atoi(value); return 1; }
    return 0;
}

/* Open path; if NULL return -1. For each line call config_parse_line.
   Returns 0 on successful open regardless of how many keys matched, -1 if the
   file could not be opened. */
int config_load(const char *path, gpu_top_config *c) {
    if (!path) return -1;
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char line[256];
    while (fgets(line, sizeof line, f))
        config_parse_line(line, c);
    fclose(f);
    return 0;
}
