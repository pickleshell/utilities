#ifndef UI_H
#define UI_H
#include "amd.h"
#include "ollama.h"

typedef struct {
    gpu_metrics g;
    proc_row  procs[256];
    size_t    nproc;
    ollama_row ollamas[256];
    size_t    nollama;
    int  refresh_s;
    char rocm_ver[32];
    int  paused;
    int  dev_idx;
    int  dev_count;
    int  focus;
    int  sel_proc;
    int  scroll_ollama;
    int  scroll_proc;
    int  sel_ollama;  // selected ollama row index (0-based)
    int  uptime_s;    // seconds since startup, set by main each frame
    int  warn;        // yellow threshold percent
    int  crit;        // red threshold percent
} view_t;

void make_bar(double pct, int width, char *buf, size_t buflen);
void ui_render(const view_t *v);
#endif
