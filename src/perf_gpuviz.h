#ifndef PERF_GPUVIZ_H
#define PERF_GPUVIZ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*perf_gpuviz_event_callback_t)(void* context,
        int64_t timestamp,
        int pid,
        int cpu,
        const char* comm,
        const char* symbol,
        const char* dso);

int perf_gpuviz_load(const char* path, void* context,
        perf_gpuviz_event_callback_t event_callback);

#ifdef __cplusplus
}
#endif

#endif
