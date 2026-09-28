/* Noninteractive client for the real EARDBD TCP protocol. */

#include <database_cache/eardbd_api.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failed(const char *operation, state_t result)
{
    if (state_ok(result)) {
        return 0;
    }
    fprintf(stderr, "%s failed (%d): %s\n", operation, result, state_msg ? state_msg : "no detail");
    return 1;
}

static int parse_number(const char *value, ulong maximum, ulong *result)
{
    char *end;
    errno   = 0;
    *result = strtoul(value, &end, 10);
    return errno || value[0] == '-' || end == value || *end != '\0' || *result == 0 || *result > maximum;
}

int main(int argc, char **argv)
{
    ulong port, job;
    if (argc < 4 || parse_number(argv[3], 65535, &port)) {
        fprintf(stderr, "Usage: %s status HOST PORT | send HOST PORT JOB_ID\n", argv[0]);
        return 2;
    }
    if (argc == 4 && strcmp(argv[1], "status") == 0) {
        eardbd_status_t status = {0};
        return failed("eardbd_status", eardbd_status(argv[2], port, &status));
    }
    if (argc != 5 || strcmp(argv[1], "send") != 0 || parse_number(argv[4], ULONG_MAX, &job)) {
        return 2;
    }

    cluster_conf_t conf = {0};
    my_node_conf_t node = {0};
    if (strlen(argv[2]) >= sizeof(node.db_ip)) {
        return 2;
    }
    strcpy(node.db_ip, argv[2]);
    conf.db_manager.tcp_port = port;
    if (failed("eardbd_connect", eardbd_connect(&conf, &node))) {
        return 1;
    }
    int result = 0;
    for (uint i = 0; i < 3 && !result; i++) {
        application_t app = {0};
        app.job.id        = job;
        app.job.step_id   = i + 1;
        strcpy(app.node_id, "report-test");
        strcpy(app.job.app_id, "report-regression");
        app.is_mpi          = i != 1;
        app.is_learning     = i == 2;
        app.signature.avg_f = 112 + i;
        result              = failed("eardbd_send_application", eardbd_send_application(&app));
    }
    for (uint i = 0; i < 2 && !result; i++) {
        periodic_metric_t metric = {0};
        metric.job_id            = job;
        metric.step_id           = 10 + i;
        strcpy(metric.node_id, "report-test");
        metric.start_time = 1700000000 + i * 10;
        metric.end_time   = metric.start_time + 10;
        metric.DC_energy  = 100 + i;
        result            = failed("eardbd_send_periodic_metric", eardbd_send_periodic_metric(&metric));
    }
    if (failed("eardbd_disconnect", eardbd_disconnect())) {
        result = 1;
    }
    if (!result) {
        printf("Sent 3 applications and 2 periodic metrics for job %lu; verify delivery in the plugin output.\n", job);
    }
    return result;
}
