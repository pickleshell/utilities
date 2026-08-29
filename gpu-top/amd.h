#ifndef AMD_H
#define AMD_H
#include <stdint.h>
#include <stddef.h>

typedef struct {
    char     name[64];
    uint32_t util_pct;
    uint64_t vram_used_mb;
    uint64_t vram_total_mb;
    uint32_t power_mw;   // socket_power (uW) / 1000
    int32_t  temp_c;
    uint32_t clock_mhz;
} gpu_metrics;

typedef struct {
    uint32_t pid;
    char     name[64];
    uint64_t vram_mb;
    uint32_t cu_pct;
} proc_row;

int  amd_init(void);
void amd_shutdown(void);
int  amd_device_count(void);
void amd_device_name(int di, char *buf, size_t len);
int  amd_refresh(int di, gpu_metrics *m);
int  amd_get_processes(int di, proc_row *out, size_t cap, size_t *n);
void amd_rocm_version(char *buf, size_t len);
#endif
