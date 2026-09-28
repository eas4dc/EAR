/* Synthetic report plugins for a user-managed EARDBD test instance. */

#include <errno.h>
#include <report/report.h>
#include <stdio.h>
#include <string.h>

#ifndef REPORT_ID
#define REPORT_ID "UNSET"
#endif

static FILE *output;

static state_t flush_output(void)
{
    if (output == NULL) {
        fprintf(stderr, "record_%s: output is not initialized\n", REPORT_ID);
        return EAR_ERROR;
    }
    if (ferror(output) || fflush(output) != 0) {
        fprintf(stderr, "record_%s: failed to write recording: %s\n", REPORT_ID, strerror(errno));
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}

state_t report_init(report_id_t *id, cluster_conf_t *conf)
{
    char path[SZ_PATH];
    int length = snprintf(path, sizeof(path), "%s/%s.tsv", conf->install.dir_temp, REPORT_ID);
    if (length < 0 || (size_t) length >= sizeof(path)) {
        fprintf(stderr, "record_%s: recording path is too long\n", REPORT_ID);
        return EAR_ERROR;
    }
    output = fopen(path, "a");
    if (output == NULL) {
        fprintf(stderr, "record_%s: cannot open %s: %s\n", REPORT_ID, path, strerror(errno));
        return EAR_ERROR;
    }
    fprintf(output, "init\t%s\t%ld\n", REPORT_ID, (long) id->pid);
    state_t result = flush_output();
    if (state_ok(result)) {
        fprintf(stderr, "record_%s: initialized, recording to %s\n", REPORT_ID, path);
    }
    return result;
}

state_t report_applications(report_id_t *id, application_t *apps, uint count)
{
    if (output == NULL) {
        return EAR_ERROR;
    }
    for (uint i = 0; i < count; i++) {
        application_t *a = &apps[i];
        fprintf(output, "app\t%s\t%lu\t%lu\t%s\t%s\t%u\t%u\t%lu\n", REPORT_ID, (ulong) a->job.id,
                (ulong) a->job.step_id, a->node_id, a->job.app_id, a->is_mpi, a->is_learning, a->signature.avg_f);
    }
    return flush_output();
}

state_t report_periodic_metrics(report_id_t *id, periodic_metric_t *metrics, uint count)
{
    if (output == NULL) {
        return EAR_ERROR;
    }
    for (uint i = 0; i < count; i++) {
        periodic_metric_t *m = &metrics[i];
        fprintf(output, "metric\t%s\t%lu\t%lu\t%s\t%lld\t%lld\t%lu\n", REPORT_ID, m->job_id, m->step_id, m->node_id,
                (long long) m->start_time, (long long) m->end_time, m->DC_energy);
    }
    return flush_output();
}

static state_t record_callback(const char *name, uint count)
{
    if (output == NULL) {
        return EAR_ERROR;
    }
    fprintf(output, "%s\t%s\t%u\n", name, REPORT_ID, count);
    return flush_output();
}

state_t report_loops(report_id_t *id, loop_t *loops, uint count)
{
    return record_callback("loops", count);
}

state_t report_events(report_id_t *id, ear_event_t *events, uint count)
{
    return record_callback("events", count);
}

state_t report_misc(report_id_t *id, uint type, const char *data, uint count)
{
    return record_callback("misc", count);
}

state_t report_dispose(report_id_t *id)
{
    state_t result = record_callback("dispose", 0);
    if (output != NULL && fclose(output) != 0) {
        result = EAR_ERROR;
    }
    output = NULL;
    return result;
}
