#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <locale.h>
#include <time.h>
#include <ncurses.h>
#include "amd.h"
#include "ollama.h"
#include "ui.h"
#include "config.h"

static view_t V;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static volatile int stopping = 0;
static volatile int paused = 0;
static int g_sort = 0; /* 0 vram desc, 1 cu desc, 2 pid asc, 3 name asc */

static int proc_cmp(const void *pa, const void *pb) {
    const proc_row *a = pa, *b = pb;
    switch (g_sort) {
        case 1: return (b->cu_pct > a->cu_pct) - (b->cu_pct < a->cu_pct);
        case 2: return (a->pid > b->pid) - (a->pid < b->pid);
        case 3: return strcmp(a->name, b->name);
        default: return (b->vram_mb > a->vram_mb) - (b->vram_mb < a->vram_mb);
    }
}

static void *refresh_thread(void *arg) {
    (void)arg;
    while (!stopping) {
        if (!paused) {
            char otext[8192]; otext[0] = '\0';
            FILE *o = popen("ollama ps 2>/dev/null", "r");
            if (o) { size_t k = 0; int c; while ((c = fgetc(o)) != EOF && k < sizeof otext - 1) otext[k++] = (char)c; otext[k]='\0'; pclose(o); }
            pthread_mutex_lock(&mtx);
            int period = V.refresh_s * 10;
            int di = V.dev_idx;
            amd_refresh(di, &V.g);
            amd_get_processes(di, V.procs, 256, &V.nproc);
            qsort(V.procs, V.nproc, sizeof(proc_row), proc_cmp);
            parse_ollama_ps(otext, V.ollamas, 256, &V.nollama);
            pthread_mutex_unlock(&mtx);
            for (int i = 0; i < period && !stopping; i++) usleep(100000);
        } else {
            for (int i = 0; i < 10 && !stopping; i++) usleep(100000);
        }
    }
    return NULL;
}

static void draw(void) { pthread_mutex_lock(&mtx); ui_render(&V); pthread_mutex_unlock(&mtx); }

int main(int argc, char **argv) {
    int check = (argc > 1 && strcmp(argv[1], "--check") == 0);
    time_t start_time = time(NULL);
    int ndev = amd_init();
    if (ndev < 0) { fprintf(stderr, "gpu-top: no AMD GPU / amdsmi init failed\n"); return 1; }
    amd_rocm_version(V.rocm_ver, sizeof V.rocm_ver);
    V.dev_count = ndev; V.focus = 1; V.sel_ollama = 0; V.uptime_s = 0;

    /* config defaults */
    gpu_top_config cfg = {1, 0, 60, 85};
    const char *home = getenv("HOME");
    char cfgpath[512]; cfgpath[0] = '\0';
    if (home) {
        snprintf(cfgpath, sizeof cfgpath, "%s/.gpu-toprc", home);
        config_load(cfgpath, &cfg);
    }
    if (cfg.warn <= 0 || cfg.warn > 100) cfg.warn = 60;
    if (cfg.crit <= 0 || cfg.crit > 100) cfg.crit = 85;
    if (cfg.warn >= cfg.crit) { cfg.warn = 60; cfg.crit = 85; }
    V.warn = cfg.warn; V.crit = cfg.crit;
    V.refresh_s = cfg.refresh_s > 0 ? cfg.refresh_s : 1;
    V.dev_idx = (cfg.device >= 0 && cfg.device < V.dev_count) ? cfg.device : 0;

    if (check) {
        gpu_metrics m; char nm[64]; amd_device_name(0, nm, sizeof nm); amd_refresh(0, &m);
        printf("[0] %s util=%u%% vram=%lu/%lu MB power=%u mW temp=%dC clk=%u MHz\n",
               nm, m.util_pct, m.vram_used_mb, m.vram_total_mb, m.power_mw, m.temp_c, m.clock_mhz);
        proc_row pr[256]; size_t np = 0; amd_get_processes(0, pr, 256, &np);
        for (size_t i = 0; i < np; i++) printf("    pid=%u %s vram=%lu MB cu=%u%%\n", pr[i].pid, pr[i].name, pr[i].vram_mb, pr[i].cu_pct);
        amd_shutdown(); return 0;
    }

    setlocale(LC_ALL, "");
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE);
    curs_set(0); /* no color: bars use monochrome patterns, not COLOR_PAIR */
    timeout(200);

    pthread_t tid; pthread_create(&tid, NULL, refresh_thread, NULL);

    while (1) {
        pthread_mutex_lock(&mtx);
        V.uptime_s = (int)(time(NULL) - start_time);
        pthread_mutex_unlock(&mtx);
        draw();
        int ch = getch();
        if (ch == 'q') break;
        else if (ch == ' ') { paused = !paused; V.paused = paused; }
        else if (ch == 'd') { pthread_mutex_lock(&mtx); V.dev_idx = (V.dev_idx + 1) % V.dev_count; pthread_mutex_unlock(&mtx); }
        else if (ch == 's') {
            pthread_mutex_lock(&mtx);
            g_sort = (g_sort + 1) % 4;
            qsort(V.procs, V.nproc, sizeof(proc_row), proc_cmp);
            pthread_mutex_unlock(&mtx);
        }
        else if (ch == KEY_UP || ch == KEY_DOWN) {
            pthread_mutex_lock(&mtx);
            int nollama = (int)V.nollama;
            if (V.focus == 1) {
                if (ch == KEY_DOWN && (size_t)(V.sel_proc + 1) < V.nproc) V.sel_proc++;
                if (ch == KEY_UP && V.sel_proc > 0) V.sel_proc--;
            } else {
                if (nollama > 0) {
                    if (ch == KEY_DOWN && V.sel_ollama < nollama - 1) V.sel_ollama++;
                    if (ch == KEY_UP && V.sel_ollama > 0) V.sel_ollama--;
                    if (V.sel_ollama > nollama - 1) V.sel_ollama = nollama - 1;
                }
            }
            pthread_mutex_unlock(&mtx);
        }
        else if (ch == KEY_PPAGE) {
            pthread_mutex_lock(&mtx);
            if (V.focus == 1) V.scroll_proc = (V.scroll_proc > 5) ? V.scroll_proc - 5 : 0;
            else V.scroll_ollama = (V.scroll_ollama > 5) ? V.scroll_ollama - 5 : 0;
            pthread_mutex_unlock(&mtx);
        }
        else if (ch == KEY_NPAGE) {
            pthread_mutex_lock(&mtx);
            if (V.focus == 1) V.scroll_proc += 5;
            else V.scroll_ollama += 5;
            pthread_mutex_unlock(&mtx);
        }
        else if (ch == '\t') { V.focus = V.focus ? 0 : 1; }
        else if (ch == 'k') {
            if (V.focus != 1) continue; /* kill only acts on the GPU-process panel */
            pthread_mutex_lock(&mtx);
            int sel = V.sel_proc; uint32_t pid = (sel < (int)V.nproc) ? V.procs[sel].pid : 0;
            char pname[64]; snprintf(pname, sizeof pname, "%s", (sel < (int)V.nproc) ? V.procs[sel].name : "");
            pthread_mutex_unlock(&mtx);
            if (pid) {
                timeout(-1);
                mvprintw(LINES-2, 1, "Kill PID %u (%s)? y/N ", pid, pname);
                refresh();
                int c = getch();
                timeout(200);
                if (c == 'y' || c == 'Y') {
                    if (kill((pid_t)pid, 0) != 0) {
                        mvprintw(LINES-2, 1, "process %u gone", pid);
                    } else {
                        kill((pid_t)pid, SIGTERM);
                        for (int i = 0; i < 30; i++) { usleep(100000); if (kill((pid_t)pid, 0) != 0) break; }
                        if (kill((pid_t)pid, 0) == 0) kill((pid_t)pid, SIGKILL);
                    }
                    pthread_mutex_lock(&mtx); V.sel_proc = 0; V.scroll_proc = 0; pthread_mutex_unlock(&mtx);
                }
                /* declined: do NOT reset selection */
            }
        }
    }

    stopping = 1;
    pthread_join(tid, NULL);
    endwin();
    amd_shutdown();
    return 0;
}
