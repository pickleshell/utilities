#pragma once
typedef struct {
    int refresh_s;  // poll interval in seconds (>=1)
    int device;     // default GPU index
    int warn;       // yellow threshold percent
    int crit;       // red threshold percent
} gpu_top_config;
int config_parse_line(const char *line, gpu_top_config *c); // returns 1 if a known key was set, else 0
int config_load(const char *path, gpu_top_config *c);       // returns 0 if file opened (even if some lines ignored), -1 if file missing
