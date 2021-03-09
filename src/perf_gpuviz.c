#include "perf_gpuviz.h"

#include <unistd.h>

// must be included first to work around include bugs
#include "linux/compiler.h"

// the rest are in arbitrary order
#include "linux/err.h"
#include "util/auxtrace.h"
#include "util/event.h"
#include "util/header.h"
#include "util/symbol.h"
#include "util/thread.h"
#include "util/evsel.h"
#include "util/map.h"
#include "util/dso.h"
#include "util/session.h"
#include "util/tool.h"

static void* s_context;
static perf_gpuviz_event_callback_t s_event_callback;

static int process_sample_event(struct perf_tool *tool,
                union perf_event *event,
                struct perf_sample *sample,
                struct evsel *evsel,
                struct machine *machine)
{
    //uint64_t period = evsel->core.attr.sample_freq;
    //printf("$$ %lu %f\n", period,1./period);

    struct addr_location al;
    if (machine__resolve(machine, &al, sample) < 0) {
        //printf("failed to resolve sample\n");
        return 0;//-1;
    }

    if (al.sym == NULL) {
        //printf("symbol is null at %f\n", (double)sample->time/1.e9);
        return 0;//-1;
    }

    const char* symbol_name = al.sym->name;
    const char* dso_name = al.map->dso->short_name;
    int cpu = al.thread->cpu;

    // cpu doesn't appear to be working...
    if (cpu < 0)
        cpu = 0;

    s_event_callback(s_context,
            sample->time,
            al.thread->pid_,
            cpu,
            thread__comm_str(al.thread),
            symbol_name,
            dso_name);

    //printf("got sample %s() in %s at %f\n", symbol_name, dso_name, sample_time);
    return 0;
}

static int process_read_event(struct perf_tool *tool,
                  union perf_event *event,
                  struct perf_sample *sample __maybe_unused,
                  struct evsel *evsel,
                  struct machine *machine __maybe_unused)
{
    //printf("got event\n");
    return 0;
}

static int process_attr(struct perf_tool *tool __maybe_unused,
            union perf_event *event,
            struct evlist **pevlist)
{
    //printf("got attr\n");
    return 0;
}

static int process_feature_event(struct perf_session *session,
                 union perf_event *event)
{
    //printf("got feature event\n");
    return 0;
}

int perf_gpuviz_load(const char* path, void* context,
        perf_gpuviz_event_callback_t event_callback)
{
    s_context = context;
    s_event_callback = event_callback;

    // hack. page_size needs to be initialized first.
    extern unsigned int page_size;
    if (page_size == 0) {
        page_size = sysconf(_SC_PAGE_SIZE);
    }

    struct perf_tool tool = {
        .sample         = process_sample_event,
        .mmap         = perf_event__process_mmap,
        .mmap2         = perf_event__process_mmap2,
        .comm         = perf_event__process_comm,
        .namespaces     = perf_event__process_namespaces,
        .cgroup         = perf_event__process_cgroup,
        .exit         = perf_event__process_exit,
        .fork         = perf_event__process_fork,
        .lost         = perf_event__process_lost,
        .read         = process_read_event,
        .attr         = process_attr,
        .tracing_data     = perf_event__process_tracing_data,
        .build_id     = perf_event__process_build_id,
        .id_index     = perf_event__process_id_index,
        .auxtrace_info     = perf_event__process_auxtrace_info,
        .auxtrace     = perf_event__process_auxtrace,
        .event_update     = perf_event__process_event_update,
        .feature     = process_feature_event,
        .ordered_events     = true,
        .ordering_requires_timestamps = true,
    };
    (void)tool;

    struct perf_data data = {
        .mode = PERF_DATA_MODE_READ,
        .path = path,
    };

    struct perf_session *session = perf_session__new(&data, false, &tool);
    if (IS_ERR(session)) {
        printf("error creating perf session!\n");
        return -1;
    }

    if (symbol__init(&session->header.env) < 0) {
        printf("symbol init error!\n");
        return -1;
    }

    //perf_session__fprintf_info(session, stdout, true);
    perf_session__process_events(session);

    printf("success\n");
    return 0;
}
