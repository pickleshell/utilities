#define _GNU_SOURCE
#include "amd.h"
#include <amd_smi/amdsmi.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

static amdsmi_socket_handle  socks[16];
static amdsmi_processor_handle procs[16];
static int ndev = 0;

static void read_file_ull(const char *path, uint64_t *out) {
    FILE *f = fopen(path, "r"); if (!f) return;
    if (fscanf(f, "%lu", out) != 1) { /* ignore */ } fclose(f);
}
static void read_file_ll(const char *path, int64_t *out) {
    FILE *f = fopen(path, "r"); if (!f) return;
    long long v = 0;
    if (fscanf(f, "%lld", &v) == 1) *out = (int64_t)v;
    fclose(f);
}
static int find_amdgpu_hwmon(char *path, size_t len) {
    for (int i = 0; i < 16; i++) {
        char n[64]; snprintf(n, sizeof n, "/sys/class/hwmon/hwmon%d/name", i);
        FILE *f = fopen(n, "r"); if (!f) continue;
        char nm[32]; nm[0]=0;
        if (fgets(nm, sizeof nm, f)) { fclose(f);
            size_t l=strlen(nm); if (l && nm[l-1]=='\n') nm[l-1]=0;
            if (strcmp(nm,"amdgpu")==0){ snprintf(path,len,"/sys/class/hwmon/hwmon%d",i); return 1; }
        } else fclose(f);
    }
    return 0;
}

int amd_init(void) {
    if (amdsmi_init(AMDSMI_INIT_AMD_GPUS) != AMDSMI_STATUS_SUCCESS) return -1;
    uint32_t sc = 16;
    if (amdsmi_get_socket_handles(&sc, socks) != AMDSMI_STATUS_SUCCESS) { amdsmi_shut_down(); return -1; }
    ndev = 0;
    for (uint32_t i = 0; i < sc && ndev < 16; i++) {
        uint32_t pc = (uint32_t)(16 - ndev);
        if (amdsmi_get_processor_handles(socks[i], &pc, procs + ndev) != AMDSMI_STATUS_SUCCESS) continue;
        ndev += (int)pc;
    }
    return ndev > 0 ? ndev : -1;
}
void amd_shutdown(void) { if (ndev) amdsmi_shut_down(); ndev = 0; }
int  amd_device_count(void) { return ndev; }

void amd_device_name(int di, char *buf, size_t len) {
    buf[0] = '\0';
    if (di < 0 || di >= ndev) return;
    char path[64]; snprintf(path, sizeof path, "/sys/class/drm/card%d/device/product_name", di);
    FILE *f = fopen(path, "r");
    if (f) { if (fgets(buf, (int)len, f)) { size_t l=strlen(buf); if (l && buf[l-1]=='\n') buf[l-1]=0; } fclose(f); }
    if (buf[0]) return;
    snprintf(buf, len, "AMD GPU %d", di);
}

int amd_refresh(int di, gpu_metrics *m) {
    if (di < 0 || di >= ndev) return -1;
    memset(m, 0, sizeof *m);
    amdsmi_processor_handle h = procs[di];
    amd_device_name(di, m->name, sizeof m->name);
    amdsmi_engine_usage_t eng;
    if (amdsmi_get_gpu_activity(h, &eng) == AMDSMI_STATUS_SUCCESS) m->util_pct = eng.gfx_activity;
    uint64_t total = 0, used = 0;
    amdsmi_get_gpu_memory_total(h, AMDSMI_MEM_TYPE_VRAM, &total);
    amdsmi_get_gpu_memory_usage(h, AMDSMI_MEM_TYPE_VRAM, &used);
    m->vram_total_mb = total / 1048576UL;
    m->vram_used_mb = used / 1048576UL;
    amdsmi_power_info_t pwr;
    if (amdsmi_get_power_info(h, &pwr) == AMDSMI_STATUS_SUCCESS) m->power_mw = (uint32_t)(pwr.socket_power / 1000);
    int64_t t = 0;
    if (amdsmi_get_temp_metric(h, AMDSMI_TEMPERATURE_TYPE_EDGE, AMDSMI_TEMP_CURRENT, &t) == AMDSMI_STATUS_SUCCESS) m->temp_c = (int32_t)(t / 1000);
    amdsmi_clk_info_t clk;
    if (amdsmi_get_clock_info(h, AMDSMI_CLK_TYPE_GFX, &clk) == AMDSMI_STATUS_SUCCESS) m->clock_mhz = clk.clk;

    char sp[128];
    if (m->util_pct == 0) {
        uint64_t u=0; snprintf(sp,sizeof sp,"/sys/class/drm/card%d/device/gpu_busy_percent",di);
        read_file_ull(sp,&u); if (u<=100) m->util_pct=(uint32_t)u;
    }
    if (m->power_mw == 0) {
        char hw[64]; if (find_amdgpu_hwmon(hw,sizeof hw)) {
            uint64_t uw=0; snprintf(sp,sizeof sp,"%s/power1_input",hw); read_file_ull(sp,&uw);
            if (uw) m->power_mw=(uint32_t)(uw/1000);
        }
    }
    if (m->temp_c == 0) {
        char hw[64]; if (find_amdgpu_hwmon(hw,sizeof hw)) {
            int64_t td=0; snprintf(sp,sizeof sp,"%s/temp1_input",hw); read_file_ll(sp,&td);
            if (td) m->temp_c=(int32_t)(td/1000);
        }
    }
    if (m->clock_mhz == 0) {
        snprintf(sp,sizeof sp,"/sys/class/drm/card%d/device/pp_dpm_sclk",di);
        FILE *cf=fopen(sp,"r");
        if (cf) { char line[128];
            while (fgets(line,sizeof line,cf)) { if (strchr(line,'*')) { uint32_t mhz=0; if (sscanf(line,"%*u: %u",&mhz)==1) m->clock_mhz=mhz; break; } }
            fclose(cf);
        }
    }
    return 0;
}

int amd_get_processes(int di, proc_row *out, size_t cap, size_t *n) {
    (void)di;
    *n = 0;
    amdsmi_process_info_t list[256];
    uint32_t cnt = 256;
    if (amdsmi_get_gpu_compute_process_info(list, &cnt) != AMDSMI_STATUS_SUCCESS) return -1;
    for (uint32_t i = 0; i < cnt && *n < cap; i++) {
        proc_row *r = &out[*n];
        r->pid = list[i].process_id;
        r->vram_mb = list[i].vram_usage;
        r->cu_pct = list[i].cu_occupancy;
        char path[64]; snprintf(path, sizeof path, "/proc/%u/comm", r->pid);
        FILE *f = fopen(path, "r");
        if (f) { if (!fgets(r->name, sizeof r->name, f)) r->name[0] = '\0'; fclose(f);
                 size_t l = strlen(r->name); if (l && r->name[l-1]=='\n') r->name[l-1] = '\0'; }
        else snprintf(r->name, sizeof r->name, "%u", r->pid);
        (*n)++;
    }
    return 0;
}

void amd_rocm_version(char *buf, size_t len) {
    const char *r = getenv("ROCm_ROOT");
    if (!r) r = "/opt/rocm-6.4.2";
    const char *p = strrchr(r, '-');
    if (p && isdigit((unsigned char)p[1])) snprintf(buf, len, "%s", p + 1);
    else snprintf(buf, len, "?");
}
