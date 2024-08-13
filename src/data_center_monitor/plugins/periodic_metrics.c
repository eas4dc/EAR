/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <string.h>
#include <report/report.h>
#include <common/output/debug.h>
#include <common/system/plugin_manager.h>
#include <data_center_monitor/plugins/conf.h>
#include <data_center_monitor/plugins/metrics.h>

#if SHOW_DEBUGS
static cchar             *self_tag = "periodic_metrics";
#endif
// Metrics
static periodic_metric_t  cs; // Current sample
static metrics_read_t     mr1;
static metrics_read_t     mr2;
static metrics_diff_t     mrD;
// Reporting
static conf_t            *conf;
static report_id_t        rid;
static uint               can_report;
static ulong              mpi;
static ullong             nominal_khz = 5000000;

declr_up_get_tag()
{
    *tag = "periodic_metrics";
    *tags_deps = "!conf+!metrics";
}

declr_up_action_init(_conf)
{
    conf = (conf_t *) data;
    // Loading report manager (is using the same report plugins than EARDBD, by now)
    if (state_ok(report_load(conf->cluster.install.dir_plug, conf->cluster.db_manager.plugins))){
        report_create_id(&rid, 0, 0, 0);
        // Initializing report manager
        if (state_ok(report_init(&rid, &conf->cluster))) {
            can_report = 1;
        }
    }
    return NULL;
}

static void metrics_read_static(metrics_read_t *mr)
{
    metrics_data_copy(mr, &((mets_t *) plugin_manager_action("metrics"))->mr);
}

declr_up_action_init(_periodic_metrics)
{
    *data_alloc = &cs;
    init_periodic_metric(&cs); //eard_power_monitoring()
    metrics_data_alloc(&mr1, &mr2, &mrD);
    metrics_read_static(&mr1);
    return rsprintf("Periodic Metrics plugin initialized correctly %s", !(can_report) ? "(missing conf)": "");
}

declr_up_action_periodic(_periodic_metrics)
{
    static char buffer[512];

    metrics_read_static(&mr2);
    metrics_data_diff(&mr2, &mr1, &mrD);
    // Fill the current sample
    cs.start_time  = mr1.time.tv_sec;
    cs.end_time    = mr2.time.tv_sec;
    cs.avg_f       = mrD.cpu_avrg;
    cs.temp        = mrD.tmp_avrg;
    cs.DRAM_energy = mrD.pow_dram;
    cs.PCK_energy  = mrD.pow_pack;
    cs.DC_energy   = mrD.nod_avrg;
    debug("%s.%lu.%lu.%llu: %lu J (%lu %lu), %lu KHz",
        cs.node_id, cs.job_id, cs.step_id, mrD.samples, cs.DC_energy, cs.PCK_energy, cs.DRAM_energy, cs.avg_f);
    // Corrections SNMP+PDU (power monitor thing)
    if (conf != NULL && conf->node != NULL && cs.DC_energy > conf->node->max_error_power){
        // cs.DC_energy = conf->node->max_sig_power;
    }
    // Reporting and alert-system
    if (can_report) {
        if (conf->node != NULL && cs.temp > conf->node->max_temp) {
            // log_report_eard_rt_error(&rid, cs.job_id, cs.step_id, TEMP_ERROR, (llong) cs.temp);
        }
        if ((cs.DRAM_energy + cs.PCK_energy) == 0) {
            // log_report_eard_rt_error(&rid, cs.job_id, cs.step_id, RA<PL_ERROR, 0);
        }
        if ((cs.avg_f == 0) || ((cs.avg_f > nominal_khz) && (mpi))) {
            // log_report_eard_rt_error(&rid, cs.job_id, cs.step_id, FREQ_ERROR, (llong) cs.avg_f);
        }
        report_periodic_metrics(&rid, &cs, 1); // Un plugin de report que haga un print para probar
    }
    metrics_data_copy(&mr1, &mr2);
    sprintf(buffer, "%s.%lu.%lu.%llu: %lu J (%lu %lu), %lu KHz",
        cs.node_id, cs.job_id, cs.step_id, mrD.samples, cs.DC_energy, cs.PCK_energy, cs.DRAM_energy, cs.avg_f);
    return buffer;
}

declr_up_post_data()
{
    if (is_msg("new_job") == 0) {
        new_job_for_period(&cs, ((ulong *) data)[0], ((ulong *) data)[1]);
        debug("'%s': posted with message '%s' jid '%lu.%lu'", self_tag, msg, cs.job_id, cs.step_id);
    }
    if (is_msg("end_job") == 0) {
        end_job_for_period(&cs);
        debug("'%s': posted with message '%s' jid '%lu.%lu'", self_tag, msg, cs.job_id, cs.step_id);
    }
    return NULL;
}