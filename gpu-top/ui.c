#include "ui.h"
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

void make_bar(double pct, int width, char *buf, size_t buflen) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    if (width < 0) width = 0;
    int filled = (int)(pct / 100.0 * width + 0.5);
    if (filled > width) filled = width;
    int i = 0;
    for (; i < filled && (size_t)i < buflen - 1; i++) buf[i] = '#';
    for (; i < width  && (size_t)i < buflen - 1; i++) buf[i] = '-';
    buf[i] = '\0';
}

/* Severity from usage thresholds: 0 normal, 1 warn, 2 crit. No color used. */
static int sev_for(double pct, int warn, int crit) {
    if (pct >= (double)crit) return 2;
    if (pct >= (double)warn) return 1;
    return 0;
}

/* Monochrome fill glyph per severity: solid -> dark hatch -> coarse hatch. */
static const char *filled_glyph(int sev) {
    switch (sev) {
        case 2: return "\xe2\x96\x92"; /* ▒ medium shade (crit) */
        case 1: return "\xe2\x96\x93"; /* ▓ dark shade  (warn) */
        default: return "\xe2\x96\x88"; /* █ full block (normal) */
    }
}

/* Leading label marker per severity: none / ! / !! */
static const char *sev_marker(int sev) {
    return sev == 2 ? "!!" : sev == 1 ? "!" : "";
}

static void draw_bar_row(int y, const char *label, double pct, const char *valstr, int warn, int crit) {
    int sev = sev_for(pct, warn, crit);
    const char *fg = filled_glyph(sev);
    char lbl[16];
    snprintf(lbl, sizeof lbl, "%s%s", label, sev_marker(sev));
    mvprintw(y, 1, "%-6s", lbl);   /* bar always starts at column 7 */
    int x = 7;
    char pat[128];
    make_bar(pct, 30, pat, sizeof pat);
    char bar[192];
    bar[0] = '\0';
    for (int i = 0; pat[i] && strlen(bar) < sizeof bar - 4; i++)
        strcat(bar, (pat[i] == '#') ? fg : "\xe2\x96\x91"); /* ░ empty */
    mvprintw(y, x, "%s", bar);
    mvprintw(y, x + 31, " %s", valstr);
}

void ui_render(const view_t *v) {
    erase();
    int h, w; getmaxyx(stdscr, h, w); (void)w;
    mvprintw(0, 1, "gpu-top - %s (ROCm %s) - %ds - up %dm - %d GPU(s)%s",
             v->g.name[0] ? v->g.name : "?", v->rocm_ver, v->refresh_s,
             v->uptime_s / 60, v->dev_count,
             v->paused ? " [PAUSED]" : "");
    char s[64];
    snprintf(s, sizeof s, "%u%%", v->g.util_pct);
    draw_bar_row(2, "GPU", (double)v->g.util_pct, s, v->warn, v->crit);
    snprintf(s, sizeof s, "%.1f/%lu GB", v->g.vram_used_mb/1024.0, v->g.vram_total_mb/1024);
    draw_bar_row(3, "VRAM", v->g.vram_total_mb ? (double)v->g.vram_used_mb/v->g.vram_total_mb*100.0 : 0, s, v->warn, v->crit);
    snprintf(s, sizeof s, "%uW", v->g.power_mw/1000);
    draw_bar_row(4, "PWR", (double)v->g.power_mw/100000.0, s, v->warn, v->crit);
    snprintf(s, sizeof s, "Temp %dC  Clock %u MHz", v->g.temp_c, v->g.clock_mhz);
    mvprintw(5, 1, "%s", s);

    int oy = 7;
    mvprintw(oy, 1, "Ollama models");
    mvprintw(oy+1, 1, "%-22s %-10s %-10s %-10s %-10s %s",
             "NAME","ID","SIZE","PROC","CTX","UNTIL");
    if (v->nollama == 0)
        mvprintw(oy+2, 1, "ollama not running / not installed");
    else {
        int maxr = h - (oy + 3);
        if (maxr > 0) for (int i = 0; i < maxr; i++) {
            size_t idx = (size_t)v->scroll_ollama + (size_t)i;
            if (idx >= v->nollama) break;
            const ollama_row *r = &v->ollamas[idx];
            int ry = oy + 2 + i;
            if (v->focus == 0 && (int)idx == v->sel_ollama) attron(A_REVERSE);
            mvprintw(ry, 1, "%-22s %-10s %-10s %-10s %-10s %s",
                     r->name, r->id, r->size, r->processor, r->context, r->until);
            if (v->focus == 0 && (int)idx == v->sel_ollama) attroff(A_REVERSE);
        }
    }

    int py = oy + 3 + (v->nollama ? (int)v->nollama : 1) + 1;
    if (py >= h-2) py = (h-2 > oy+3) ? h-2 : oy+3;
    mvprintw(py, 1, "GPU processes");
    mvprintw(py+1, 1, "%-8s %-24s %-12s %s", "PID","NAME","VRAM","CU%");
    int pmax = h - (py + 2) - 1;
    if (pmax < 0) pmax = 0;
    size_t start = v->scroll_proc;
    for (int i = 0; (size_t)(start+i) < v->nproc && i < pmax; i++) {
        size_t idx = start + i;
        const proc_row *r = &v->procs[idx];
        int ry = py+2+i;
        if (v->focus == 1 && (int)idx == v->sel_proc) attron(A_REVERSE);
        mvprintw(ry, 1, "%-8u %-24s %-12lu %u%%", r->pid, r->name, r->vram_mb, r->cu_pct);
        if (v->focus == 1 && (int)idx == v->sel_proc) attroff(A_REVERSE);
    }
    mvprintw(h-1, 1, "q quit | sp pause | d dev | s sort | up/dn scroll | tab focus | k kill");
    refresh();
}
