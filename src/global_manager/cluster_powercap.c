/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// #define SHOW_DEBUGS 1
#include <common/colors.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/system/execute.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/generic.h>
#include <common/types/log.h>

#include <daemon/remote_api/eard_rapi.h>
#include <daemon/remote_api/eard_rapi_internals.h>

#include <global_manager/cluster_powercap.h>
#include <global_manager/log_eargmd.h>
#include <global_manager/plugins/powercap_plugin.h>

#include <report/report.h>

extern cluster_conf_t my_cluster_conf;
extern int64_t default_cluster_powercap;
extern uint cluster_powercap_period;
extern eargm_def_t *e_def;

extern uint process_created;
extern pthread_mutex_t plocks;
void check_pending_processes();

uint current_cluster_power;
uint min_cluster_powercap;
uint powercap_unlimited = 0;
uint risk               = 0;
uint max_num_nodes      = 0;
/* Shared data with eargm_ext thread to sent to meta_eargms */
shared_powercap_data_t ext_powercap_data;
pthread_mutex_t ext_mutex = PTHREAD_MUTEX_INITIALIZER;

uint num_nodes;
uint actions_executed = 0;

cluster_powercap_ctx_t cpc_ctx = {0};

cluster_powercap_status_t *my_cluster_power_status;
powercap_opt_t cluster_options;
extern uint policy;
extern char **nodes;
extern int num_eargm_nodes;
extern char host[GENERIC_NAME];
static bool must_send_pc_options;
static ulong current_extra_power;
#define min(a, b) (a < b ? a : b)

void check_powercap_actions(uint cluster_powercap);
void write_shared_data(ulong current_power, ulong freeable_power, ulong requested);

static uint synthetic_total_nodes(void)
{
    if (num_eargm_nodes > 0)
        return (uint) num_eargm_nodes;
    return max_num_nodes;
}

static ulong synthetic_default_powercap(void)
{
    if (default_cluster_powercap < 0)
        return min_cluster_powercap;
    return (ulong) default_cluster_powercap;
}

static state_t synthetic_powercap_read_sample(synthetic_powercap_sample_t *sample)
{
    return synthetic_powercap_sample(cpc_ctx.current_cluster_powercap, synthetic_default_powercap(),
                                     synthetic_total_nodes(), sample);
}

static void synthetic_powercap_apply_status(const synthetic_powercap_sample_t *sample)
{
    my_cluster_power_status->total_nodes      = (uint32_t) sample->total_nodes;
    my_cluster_power_status->idle_nodes       = (uint32_t) sample->idle_nodes;
    my_cluster_power_status->released         = 0;
    my_cluster_power_status->requested        = (uint32_t) sample->requested;
    my_cluster_power_status->total_idle_power = (uint32_t) sample->freeable_power;
    my_cluster_power_status->current_power    = (uint32_t) sample->current_power;
    my_cluster_power_status->total_powercap   = cpc_ctx.current_cluster_powercap;
}

void eargm_report_event(uint event_type, ulong value)
{
    ear_event_t event = {0};

    event.timestamp = time(NULL);
    event.jid       = 0;
    event.step_id   = 0;

    char *gm_name = ear_getenv("EARGMNAME");
    int ret;
    if (gm_name)
        ret = snprintf(event.node_id, sizeof(event.node_id), "eargm-%s-%s", gm_name, host);
    else
        ret = snprintf(event.node_id, sizeof(event.node_id), "eargm-%s", host);

    if (ret < 0) {
        verbose(VGM_PC, "%sERROR%s Writing the hostname to the event.", COL_RED, COL_CLR);
    } else if (ret >= sizeof(event.node_id)) {
        verbose(VGM_PC, "%sWARNING%s The node id field of the event was truncated. Required size: %d", COL_YLW, COL_CLR,
                ret);
    }

    event.event = event_type;
    event.value = value;
    report_events(NULL, &event, 1);
    // db_insert_ear_event(&event);
}

void aggregate_data(powercap_status_t *cs, cluster_powercap_ctx_t *ctx)
{
    int i;
    ctx->total_req_greedy  = 0;
    ctx->total_extra_power = 0;
    ctx->req_no_extra      = 0;
    ctx->num_no_extra      = 0;
    ctx->num_greedy        = 0;
    ctx->num_extra         = 0;
    ctx->extra_power_alloc = 0;
    for (i = 0; i < cs->num_greedy; i++) {
        ctx->total_req_greedy += cs->greedy_data[i].requested;
        ctx->total_extra_power += cs->greedy_data[i].extra_power;
        if (cs->greedy_data[i].extra_power) {
            ctx->num_extra++;
            ctx->extra_power_alloc += cs->greedy_data[i].extra_power;
        }
        if (cs->greedy_data[i].requested)
            ctx->num_greedy++;
        if ((cs->greedy_data[i].requested) && (!cs->greedy_data[i].extra_power)) {
            ctx->req_no_extra += cs->greedy_data[i].requested;
            ctx->num_no_extra++;
        }
    }
}

state_t _read_non_computational_power(uint64_t *power)
{
    *power = 0;
    return EAR_SUCCESS;
}

void print_powercap_opt(powercap_opt_t *opt)
{
    int i;
    verbose(VGM_PC, "%s", COL_GRE);
    verbose(VGM_PC, "num_greedy %u", opt->num_greedy);
    for (i = 0; i < opt->num_greedy; i++) {
        verbose(VGM_PC, "greedy_node %d extra power %d", opt->greedy_nodes[i], opt->extra_power[i]);
    }
    verbose(VGM_PC, "Cluster power allocation percentage %u", opt->cluster_perc_power);
    verbose(VGM_PC, "%s", COL_CLR);
}

#if 0
void allocate_free_power_to_greedy_nodes(cluster_powercap_status_t *cluster_status, powercap_opt_t *cluster_options,
                                         uint *total_free)
{
    int i, more_power;
    uint pending = *total_free;
    if (num_greedy == 0) {
        debug("NO greedy nodes, returning");
        return;
    }
    if (pending == 0) {
        debug("NO free power, returning");
        return;
    }
    must_send_pc_options = 1;
    debug("allocate_free_power_to_greedy_nodes----------------");
    verbose(VGM_PC + 1, "Total extra power for nodes with NO current extra power %u W, %u nodes", req_no_extra,
            num_no_extra);
    verbose(VGM_PC + 1, "Nodes with extra power already allocated %u", num_greedy);
    greedy_allocated = 0;
    if (num_no_extra > 0) {
        if (req_no_extra < pending)
            more_power = -1;
        else
            more_power = pending / num_no_extra;
        verbose(VGM_PC + 1, "STAGE_1-Allocating %d watts to new greedy nodes first (-1 => alloc=req)", more_power);
        for (i = 0; i < cluster_status->num_greedy; i++) {
            if ((cluster_status->greedy_data[i].requested) && (!cluster_status->greedy_data[i].extra_power)) {
                if (more_power < 0)
                    cluster_options->extra_power[i] = cluster_status->greedy_data[i].requested;
                else
                    cluster_options->extra_power[i] = min(more_power, cluster_status->greedy_data[i].requested);
                pending -= cluster_options->extra_power[i];
                greedy_allocated += cluster_options->extra_power[i];
            }
        }
    }
    /* If there is pending power to allocate */
    verbose(VGM_PC + 1, "STAGE-2 allocating %uW to the rest of greedy nodes", pending);
    if (pending) {
        verbose(VGM_PC + 1, "Allocating %u watts to greedy nodes ", pending);
        more_power = pending / num_greedy;
        for (i = 0; i < cluster_status->num_greedy; i++) {
            if ((cluster_status->greedy_data[i].requested) && (!cluster_options->extra_power[i])) {
                cluster_options->extra_power[i] = min(cluster_status->greedy_data[i].requested, more_power);
                pending -= cluster_options->extra_power[i];
                greedy_allocated += cluster_options->extra_power[i];
            }
        }
    }
    *total_free = pending;
}

/* This function is executed when there is not enough power for new running nodes */
void reduce_allocation(cluster_powercap_status_t *cs, powercap_opt_t *cluster_options, uint min_reduction)
{
    int i = 0;
    uint red1, red, red_node;
    int new_extra;
    if (num_extra == 0) {
        error("We need to reallocated power and there is no extra power ");
        return;
    }
    red_node = min_reduction / num_extra;
    verbose(VGM_PC + 1, "SEQ reduce_allocation implementation avg red=%uW", red_node);
    debug("ROUND 1- reducing avg power ");
    while ((min_reduction > 0) && (i < cs->num_greedy)) {
        if (cs->greedy_data[i].extra_power) {
            red1      = min(red_node, cs->greedy_data[i].extra_power);
            red       = min(red1, min_reduction);
            new_extra = -red;
            verbose(VGM_PC + 1, "%sreducing %d W to node %d %s", COL_RED, new_extra, i, COL_CLR);
            cluster_options->extra_power[i] = new_extra;
            min_reduction -= red;
        }
        i++;
    }
    debug("ROUND 2- Reducing remaining power ");
    i = 0;
    while ((min_reduction > 0) && (i < cs->num_greedy)) {
        /* cluster_options->extra_power[i] is a negative value */
        if ((cs->greedy_data[i].extra_power + cluster_options->extra_power[i]) > 0) {
            red1      = cs->greedy_data[i].extra_power + cluster_options->extra_power[i];
            red       = min(red1, min_reduction);
            new_extra = -red;
            verbose(VGM_PC + 1, "%sreducing %d W to node %d %s", COL_RED, new_extra, i, COL_CLR);
            cluster_options->extra_power[i] += new_extra;
            min_reduction -= red;
        }
        i++;
    }
}
#endif

#if 0
/* This function is executed when there is enough power for new running nodes but not for all the greedy nodes */
uint powercap_reallocation(cluster_powercap_status_t *cluster_status, powercap_opt_t *cluster_options)
{
    int i;
    uint num_nodes = cluster_status->total_nodes;
    uint min_reduction;
    verbose(VGM_PC + 1, "There are %u nodes  %u idle nodes %u greedy nodes ", num_nodes, cluster_status->idle_nodes,
            cluster_status->num_greedy);
    if (cluster_options->greedy_nodes == NULL) {
        cluster_options->greedy_nodes = calloc(cluster_status->num_greedy, sizeof(int));
        cluster_options->extra_power  = calloc(cluster_status->num_greedy, sizeof(uint));
    }
    cluster_options->num_greedy         = cluster_status->num_greedy;
    cluster_options->cluster_perc_power = (cluster_status->total_powercap * 100) / cpc_ctx.current_cluster_powercap;
    memcpy(cluster_options->greedy_nodes, cluster_status->greedy_nodes, cluster_status->num_greedy * sizeof(int));

    total_free                       = cpc_ctx.current_cluster_powercap - cluster_status->total_powercap;
    uint64_t non_computational_power = 0;
    if (!state_ok(_read_non_computational_power(&non_computational_power))) {
        warning("Non-computational power could not be read");
    } else {
        total_free -= non_computational_power;
    }

    verbose(VGM_PC + 1, "Total power %u , requested for new %u (potential) released %u extra_req %u extra_used %u",
            cpc_ctx.current_cluster_powercap, cluster_status->requested, cluster_status->released, total_req_greedy,
            total_extra_power);
    verbose(VGM_PC + 1, "Free power before reallocation %d", total_free);

    if (cluster_status->requested)
        must_send_pc_options = 1;

    /* Allocated power + default requested must be less that maximum: ASK_DEF */
    if ((cluster_status->total_powercap + cluster_status->requested) <= cpc_ctx.current_cluster_powercap) {
        if (cluster_status->requested) {
            verbose(VGM_PC + 1, "There is enough power for new running jobs");
            total_free = cpc_ctx.current_cluster_powercap - (cluster_status->total_powercap + cluster_status->requested);
            verbose(VGM_PC + 1, "Free power after allocating power to new jobs %d", total_free);
        }
        if (total_req_greedy == 0) {
            return 1;
        }
        /* At this point we know there is greedy power requested: GREEDY */
        if (total_req_greedy <= total_free) {
            verbose(VGM_PC + 1,
                    "There is more free power than requested by greedy nodes=> allocating all the req power");
            total_free -= total_req_greedy;
            for (i = 0; i < cluster_status->num_greedy; i++)
                cluster_options->extra_power[i] = cluster_status->greedy_data[i].requested;
            must_send_pc_options = 1;
        } else {
            verbose(VGM_PC + 1,
                    "There is not enough power for all the greedy nodes (free %d req %u)(used %u allocated %u)",
                    total_free, total_req_greedy, cluster_status->current_power, cluster_status->total_powercap);
            if (cluster_status->released == 0) {
                verbose(VGM_PC + 1, "Anyway there is not enough power ");
                cluster_options->num_greedy = cluster_status->num_greedy;
                allocate_free_power_to_greedy_nodes(cluster_status, cluster_options, (uint *) &total_free);
            } else if (cluster_status->released) {
                return 0; // We try to release power to have power for greedy nodes
            }
        }
    } else {
        /* There is not enough power for new jobs (ASK_DEF), we must reduce the extra allocation */
        if (cluster_status->released == 0) {
            verbose(VGM_PC + 1, "We must reduce the extra allocation (used %u allocated %u)",
                    cluster_status->current_power, cluster_status->total_powercap);
            min_reduction = (cluster_status->total_powercap + cluster_status->requested) - cpc_ctx.current_cluster_powercap;
            total_free    = 0;
            reduce_allocation(cluster_status, cluster_options, min_reduction);
            must_send_pc_options = 1;
        } else if (cluster_status->released)
            return 0; // We try to release power to have power for greedy nodes
    }
    return 1;
}
#endif

void send_powercap_options_to_cluster(powercap_opt_t *cluster_options)
{
    if (cluster_powercap_is_synthetic_mode())
        return;
    /* only this EARGM's nodes: a node that is not in the greedy list takes the options as
     * "your default power is granted", which would serve another EARGM's ASK_DEF nodes */
    ear_set_powercap_opt(&my_cluster_conf, cluster_options, nodes, num_eargm_nodes);
}

void print_cluster_power_status(powercap_status_t *my_cluster_power_status)
{
    int i;
    powercap_status_t *cs = my_cluster_power_status;
    if (cs == NULL) {
        debug("cluster status is NULL");
        return;
    }
    verbose(VGM_PC,
            "%sTotal %u Idle  %u power (released %u requested %u consumed %u allocated %u allocated_to_idle %u) "
            "total_greedy_nodes %u%s",
            COL_CLR, cs->total_nodes, cs->idle_nodes, cs->released, cs->requested, cs->current_power,
            cs->total_powercap, cs->num_greedy, cs->total_idle_power, COL_CLR);
    for (i = 0; i < cs->num_greedy; i++) {
        verbose(VGM_PC, "%s\t[%d] ip %d greedy_req %u extra_power %u%s", COL_CLR, i, cs->greedy_nodes[i],
                cs->greedy_data[i].requested, cs->greedy_data[i].extra_power, COL_CLR);
    }
}

pthread_t cluster_powercap_th;

/* Cluster Soft powercap */
void cluster_soft_powercap()
{

    power_check_t power;
    synthetic_powercap_sample_t synthetic_sample = {0};
    bool synthetic_mode                          = cluster_powercap_is_synthetic_mode();

    my_cluster_power_status = calloc(1, sizeof(cluster_powercap_status_t));
    if (my_cluster_power_status == NULL) {
        error("Allocating memory for power status");
        write_shared_data(0, 0, 0);
        return;
    }

    memset(&power, 0, sizeof(power_check_t));
    if (synthetic_mode) {
        if (state_fail(synthetic_powercap_read_sample(&synthetic_sample))) {
            error("Could not read synthetic powercap sample");
            free(my_cluster_power_status);
            write_shared_data(0, 0, 0);
            return;
        }
        power.power     = synthetic_sample.current_power;
        power.num_nodes = (int64_t) (synthetic_sample.total_nodes - synthetic_sample.idle_nodes);
    } else {
        ear_get_power(&my_cluster_conf, &power, nodes, num_eargm_nodes);
    }

    eargm_report_event(CLUSTER_POWER, (ulong) power.power);

    float upper_limit = ((float) (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_limit) / 100.0);
    float lower_limit = ((float) (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_lower) / 100.0);
    uint64_t used_power              = power.power;
    uint64_t non_computational_power = 0;
    if (!synthetic_mode) {
        if (!state_ok(_read_non_computational_power(&non_computational_power))) {
            warning("Non-computational power could not be read");
        } else {
            used_power += non_computational_power;
        }
    }

    verbose(VGM_PC, "cluster_soft_powercap, total power %ld, computational power %ld and nodes %ld", used_power,
            power.power, power.num_nodes);
    if (cpc_ctx.current_cluster_powercap == POWER_CAP_UNLIMITED) {
        verbose(VGM_PC, "cluster_powercap is unlimited, nothing to do");
        write_shared_data(used_power, 0, 0);
        free(my_cluster_power_status);
        return;
    }

    uint64_t freeable_power    = 0;
    uint64_t requested         = 0;
    uint64_t new_limit         = 0;
    uint32_t num_soft_pc_nodes = max_num_nodes;

    if (synthetic_mode) {
        synthetic_powercap_apply_status(&synthetic_sample);
        used_power     = synthetic_sample.current_power;
        freeable_power = synthetic_sample.freeable_power;
        requested      = synthetic_sample.requested;
    }

    if (used_power > upper_limit) {
        /* If we have a nodelist we only want to divide by that list*/
        if (num_eargm_nodes != 0) {
            num_soft_pc_nodes = num_eargm_nodes;
        }
        /* For heterogeneous clusters we set the max powercap for each node, which should be set by the admin in
         * ear.conf For homogeneous clusters we can simply divide the current power between the number of active nodes.
         */
        if (num_soft_pc_nodes > 0) {
            if (my_cluster_conf.eargm.soft_pc_strategy == HOMOGENEOUS) {
                // Available power is homegeneously distributed
                new_limit = cpc_ctx.current_cluster_powercap / num_soft_pc_nodes;
            } else {
                // UINT_MAX means each node will select the max powercap set in ear.conf
                new_limit = UINT_MAX;
            }
            if (!synthetic_mode)
                ear_set_powerlimit(&my_cluster_conf, new_limit, nodes, num_eargm_nodes);

            verbose(VGM_PC, "cluster_only_powercap: sending new limit %lu", (ulong) new_limit);
            eargm_report_event(NODE_POWERCAP, (ulong) new_limit);
        } else {
            warning("No nodes detected for this EARGM");
        }
        if (!synthetic_mode) {
            requested = used_power - upper_limit;
        }
        risk = 1;
    } else {
        if (risk == 1 && used_power <= lower_limit) {
            verbose(VGM_PC, "cluster_only_powercap: sending POWER_CAP_UNLIMTED to nodes");
            if (!synthetic_mode)
                ear_set_powerlimit(&my_cluster_conf, 1, nodes, num_eargm_nodes); // set powercap unlimited
            risk = 0;
            eargm_report_event(POWER_UNLIMITED, 1);
        }
        if (!synthetic_mode) {
            freeable_power =
                (uint64_t) (upper_limit - used_power) / 2; // for now, we will free half of whatever extra power
        }
    }
    write_shared_data(used_power, freeable_power, requested);

    /* These values in my_cluster_power_status are used automatically by the actions */
    if (!synthetic_mode) {
        my_cluster_power_status->current_power    = used_power;
        my_cluster_power_status->idle_nodes       = 0;
        my_cluster_power_status->total_idle_power = 0;
    }

    check_powercap_actions(used_power);

    free(my_cluster_power_status);
}

void *eargm_powercap_th(void *noarg)
{
    if (pthread_setname_np(pthread_self(), "cluster_powercap") != 0)
        error("Setting name forcluster_powercap thread %s", strerror(errno));

    if (!cluster_powercap_is_synthetic_mode() && my_cluster_conf.eargm.powercap_mode == EARGM_SOFT_POWERCAP)
        ear_set_powerlimit(&my_cluster_conf, 1, nodes, num_eargm_nodes); // set powercap to unlimited at the start
    while (1) {
        sleep(cluster_powercap_period);
        verbose(VGM_PC, "new powercap period");
        /* we check if there are zombie processes */
        if (process_created) {
            check_pending_processes();
        }
        if (cluster_power_limited()) {
            switch (my_cluster_conf.eargm.powercap_mode) {
                case EARGM_SOFT_POWERCAP:
                    cluster_soft_powercap();
                    break;
                case EARGM_HARD_POWERCAP:
                    cluster_hard_powercap();
                    break;
                case EARGM_MONITOR:
                default:
                    cluster_power_monitor();
                    break;
            }
        }
    }
}

void cluster_powercap_reset_pc()
{
    ulong effective_default_powercap = synthetic_default_powercap();

    verbose(VGM_PC, "Resetting cluster powercap to %lu", effective_default_powercap);
    cpc_ctx.current_cluster_powercap = effective_default_powercap;
    current_extra_power              = 0;
}

void cluster_powercap_set_pc(uint limit)
{
    ulong effective_default_powercap = synthetic_default_powercap();

    verbose(VGM_PC, "Setting cluster powercap to %u", limit);

    cpc_ctx.current_cluster_powercap = limit;

    if (effective_default_powercap < cpc_ctx.current_cluster_powercap)
        current_extra_power = cpc_ctx.current_cluster_powercap - effective_default_powercap;
    else
        current_extra_power = 0;
#if 0
    pthread_mutex_lock(&ext_mutex);
    ext_powercap_data.current_powercap = cpc_ctx.current_cluster_powercap;
    pthread_mutex_unlock(&ext_mutex);
#endif
}

void cluster_powercap_red_pc(uint red)
{
    ulong effective_default_powercap = synthetic_default_powercap();

    verbose(VGM_PC, "Reducing cluster powercap by %u (from %u to %u)", red, cpc_ctx.current_cluster_powercap,
            cpc_ctx.current_cluster_powercap - red);
    if (red > cpc_ctx.current_cluster_powercap) {
        verbose(VGM_PC, "Cannot reduce powercap by %u (would overflow)", red);
    } else {
        cpc_ctx.current_cluster_powercap -= red;
    }

    if (effective_default_powercap < cpc_ctx.current_cluster_powercap)
        current_extra_power = cpc_ctx.current_cluster_powercap - effective_default_powercap;
    else
        current_extra_power = 0;
}

void cluster_powercap_inc_pc(uint inc)
{
    ulong effective_default_powercap = synthetic_default_powercap();

    verbose(VGM_PC, "Increasing cluster powercap by %u (from %u to %u)", inc, cpc_ctx.current_cluster_powercap,
            cpc_ctx.current_cluster_powercap + inc);
    cpc_ctx.current_cluster_powercap += inc;
    if (effective_default_powercap < cpc_ctx.current_cluster_powercap)
        current_extra_power = cpc_ctx.current_cluster_powercap - effective_default_powercap;
    else
        current_extra_power = 0;
}

uint cluster_get_min_power(cluster_conf_t *conf, char type)
{
    uint total = 0;
    int i, j, k;
    int tag_id, def_tag_id;
    def_tag_id = get_default_tag_id(conf);

    for (i = 0; i < conf->num_islands; i++) {
        for (j = 0; j < conf->islands[i].num_ranges; j++) {
            node_range_t *current_range = &conf->islands[i].ranges[j];
            tag_id                      = -1;
            for (k = 0; k < current_range->num_tags; k++) {
                tag_id = tag_id_exists(conf, conf->islands[i].specific_tags[current_range->specific_tags[k]]);
                if (tag_id >= 0) {
                    break;
                }
            }
            if (tag_id < 0) {
                tag_id = def_tag_id;
            }
            if (type == ENERGY_TYPE) {
                total += conf->tags[tag_id].max_power * node_range_get_num_nodes(current_range);
            } else {
                total += conf->tags[tag_id].powercap * node_range_get_num_nodes(current_range);
            }
        }
    }

    return total;
}

/* Each node can always reclaim its default powercap (ASK_DEF), and never goes under its
 * MIN_WORKING_POWER: both sums must fit in this EARGM's budget, or the guarantee breaks */
static void check_node_defaults(cluster_conf_t *cc, uint budget)
{
    uint64_t sum_def = 0, sum_min = 0;
    int counted = 0;
    if (num_eargm_nodes <= 0 || nodes == NULL) {
        verbose(VGM_PC, "no explicit node list: node defaults are not checked against the budget");
        return;
    }
    for (int i = 0; i < num_eargm_nodes; i++) {
        my_node_conf_t *n = get_my_node_conf(cc, nodes[i]);
        if (n == NULL)
            continue;
        if (n->powercap > POWER_CAP_UNLIMITED)
            sum_def += n->powercap;
        sum_min += n->min_working_power;
        counted++;
    }
    if (sum_def > budget)
        warning("the default powercaps of %d nodes add up to %lu W, above the %u W budget: nodes asking for "
                "their default may not get it",
                counted, sum_def, budget);
    if (sum_min > budget)
        warning("MIN_WORKING_POWER of %d nodes adds up to %lu W, above the %u W budget: the budget can't be met "
                "with every node working",
                counted, sum_min, budget);
}

void cluster_powercap_init(cluster_conf_t *cc)
{
    int ret;
    max_num_nodes                    = get_num_nodes(cc);
    cpc_ctx.current_cluster_powercap = default_cluster_powercap;
    min_cluster_powercap             = cluster_get_min_power(cc, POWER_TYPE);
    if (cpc_ctx.current_cluster_powercap == -1)
        cpc_ctx.current_cluster_powercap = min_cluster_powercap;

    if (cpc_ctx.current_cluster_powercap > 0) {
        verbose(VGM_PC, "Power cap limit set to %u", cpc_ctx.current_cluster_powercap);
    } else {
        verbose(VGM_PC, "Power cap unlimited");
    }

    if (cpc_ctx.current_cluster_powercap == 0)
        return;
    if (cpc_ctx.current_cluster_powercap > POWER_CAP_UNLIMITED)
        check_node_defaults(cc, cpc_ctx.current_cluster_powercap);

    verbose(VGM_PC, "Loading plugins %s from path %s", e_def->plugins, cc->install.dir_plug);
    if (!state_ok(eargm_plugin_load(cc->install.dir_plug, e_def->plugins))) {
        error("Could not load EARGM plugins");
    } else {
        verbose(VGM_PC, "Loaded plugin! (%s)", e_def->plugins);
        if (!state_ok(eargm_plugin_init(cc))) {
            error("Could not init EARGM plugins");
        } else {
            verbose(VGM_PC, "Plugin init done!");
        }
    }

    /* This thread accepts external commands */
    if ((ret = pthread_create(&cluster_powercap_th, NULL, eargm_powercap_th, NULL))) {
        errno = ret;
        error("error creating eargm_server for external api %s", strerror(errno));
    }
    powercap_unlimited = tags_are_unlimited(cc);
}

int cluster_power_limited()
{
    return (cpc_ctx.current_cluster_powercap);
}

/* This function is executed when we are in a good position after executing the higher limit action*/
void execute_powercap_lower_action()
{
    char cmd[300];
    /* If action is different than no_action we run the command */
    if (strcmp(my_cluster_conf.eargm.powercap_lower_action, "no_action")) {
        /* Format is: command current_power current_limit total_idle_nodes allocated_idle_power*/
        sprintf(cmd, "%s %u %u %u %u", my_cluster_conf.eargm.powercap_lower_action,
                my_cluster_power_status->current_power, cpc_ctx.current_cluster_powercap,
                my_cluster_power_status->idle_nodes, my_cluster_power_status->total_idle_power);
        verbose(VGM_PC, "%sExecuting powercap resume action: %s %s", COL_GRE, cmd, COL_CLR);
        execute_with_fork(cmd);
        pthread_mutex_lock(&plocks);
        process_created++;
        pthread_mutex_unlock(&plocks);
        actions_executed = 0;
    } else {
        verbose(VGM_PC, "No action for powercap lower limit");
    }
}

/* This function is executed when we are close to the limit , consumed or allocated, depending of powercap_mode*/
void execute_powercap_limit_action()
{
    char cmd[300];
    /* If action is different than no_action we run the command */
    if (strcmp(my_cluster_conf.eargm.powercap_limit_action, "no_action")) {
        /* Format is: command current_power current_limit total_idle_nodes allocated_idle_power*/
        sprintf(cmd, "%s %u %u %u %u", my_cluster_conf.eargm.powercap_limit_action,
                my_cluster_power_status->current_power, cpc_ctx.current_cluster_powercap,
                my_cluster_power_status->idle_nodes, my_cluster_power_status->total_idle_power);
        verbose(VGM_PC, "%sExecuting powercap suspend action: %s%s", COL_RED, cmd, COL_CLR);
        execute_with_fork(cmd);
        pthread_mutex_lock(&plocks);
        process_created++;
        pthread_mutex_unlock(&plocks);
        actions_executed = 1;
    } else {
        verbose(VGM_PC, "No action for powercap limit");
    }
}

void check_powercap_actions(uint cluster_powercap)
{
    if (cluster_powercap >=
            ((float) (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_limit) / 100.0) &&
        !actions_executed) {
        execute_powercap_limit_action();
    } else if (cluster_powercap <=
                   ((float) (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_lower) / 100.0) &&
               actions_executed) {
        execute_powercap_lower_action();
    }
}

void write_shared_data(ulong current_power, ulong freeable_power, ulong requested)
{
    ulong effective_default_powercap = synthetic_default_powercap();

    pthread_mutex_lock(&ext_mutex);
    memset(&ext_powercap_data, 0, sizeof(shared_powercap_data_t));
    ext_powercap_data.status = PC_STATUS_OK;
    if (requested > 0 && effective_default_powercap > cpc_ctx.current_cluster_powercap) {
        if (effective_default_powercap < (cpc_ctx.current_cluster_powercap + requested)) {
            requested = effective_default_powercap - cpc_ctx.current_cluster_powercap;
        }
        freeable_power           = 0; // if pc_status_ask_def and requesting we cannot free
        ext_powercap_data.status = PC_STATUS_ASK_DEF;
    } else if (requested) {
        freeable_power           = 0; // if greedy and requesting, we cannot free more than our extra_power
        ext_powercap_data.status = PC_STATUS_GREEDY;
    }

    ext_powercap_data.requested        = requested;
    ext_powercap_data.current_power    = current_power;
    ext_powercap_data.extra_power      = current_extra_power;
    ext_powercap_data.available_power  = freeable_power;
    ext_powercap_data.current_powercap = cpc_ctx.current_cluster_powercap;
    ext_powercap_data.def_power        = effective_default_powercap;
    debug("Written shared data with current power %lu and default power %lu", cpc_ctx.current_cluster_powercap,
          effective_default_powercap);
    pthread_mutex_unlock(&ext_mutex);
}

void cluster_power_monitor()
{
    debug("%sGlobal POWER monitoring INIT----%s", COL_BLU, COL_CLR);
    if (cluster_powercap_is_synthetic_mode()) {
        synthetic_powercap_sample_t synthetic_sample = {0};
        ulong used_power                             = 0;

        my_cluster_power_status = calloc(1, sizeof(cluster_powercap_status_t));
        if (my_cluster_power_status == NULL) {
            error("Allocating memory for synthetic power status");
            write_shared_data(0, 0, 0);
            return;
        }
        if (state_fail(synthetic_powercap_read_sample(&synthetic_sample))) {
            error("Could not read synthetic powercap sample");
            free(my_cluster_power_status);
            write_shared_data(0, 0, 0);
            return;
        }
        synthetic_powercap_apply_status(&synthetic_sample);
        used_power = synthetic_sample.current_power;
        verbose(VGM_PC,
                "ITER %lu:Synthetic data: used power %lu freeable power %lu requested %lu total_nodes %u "
                "active nodes %u "
                "req_power_per_node "
                "%lu total_demand %lu",
                synthetic_sample.iteration, used_power, synthetic_sample.freeable_power, synthetic_sample.requested,
                synthetic_sample.total_nodes, synthetic_sample.total_nodes - synthetic_sample.idle_nodes,
                synthetic_sample.requested_power_per_node, synthetic_sample.demand_total);
        write_shared_data(used_power, synthetic_sample.freeable_power, synthetic_sample.requested);
        eargm_report_event(CLUSTER_POWER, (ulong) used_power);
        if (used_power >=
                ((float) (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_limit) / 100.0) &&
            !actions_executed) {
            execute_powercap_limit_action();
        } else if (used_power <=
                       ((float) (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_lower) /
                        100.0) &&
                   actions_executed) {
            execute_powercap_lower_action();
        } else {
            verbose(VGM_PC + 1, "Total power %u limit for action %lu", (uint) used_power,
                    (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_limit) / 100);
        }
        free(my_cluster_power_status);
        debug("%sGlobal POWER monitoring END-----%s", COL_BLU, COL_CLR);
        return;
    }
    /* We use this function for now, we must use a new light one */
    if (state_fail(ear_get_powercap_status(&my_cluster_conf, &my_cluster_power_status, 1, nodes, num_eargm_nodes))) {
        verbose(VGM_PC + 1, "ear_get_powercap_status in cluster_power_monitor returns EAR_ERROR");
        write_shared_data(0, 0, 0);
        return;
    }
    write_shared_data(my_cluster_power_status->current_power, 0, 0);
    eargm_report_event(CLUSTER_POWER, (ulong) my_cluster_power_status->current_power);
    if (my_cluster_power_status->current_power >=
            ((float) (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_limit) / 100.0) &&
        !actions_executed) {
        execute_powercap_limit_action();
    } else if (my_cluster_power_status->current_power <=
                   ((float) (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_lower) / 100.0) &&
               actions_executed) {
        execute_powercap_lower_action();
    } else {
        verbose(VGM_PC + 1, "Total power %u limit for action %lu", my_cluster_power_status->current_power,
                (cpc_ctx.current_cluster_powercap * my_cluster_conf.eargm.defcon_power_limit) / 100);
    }
    free(my_cluster_power_status);
    debug("%sGlobal POWER monitoring END-----%s", COL_BLU, COL_CLR);
}

#define CLUSTER_PC_TO_SHARE 0.5

void cluster_hard_powercap()
{
    debug("%sSTART cluster_check_powercap---------%s", COL_BLU, COL_CLR);
    if (cluster_powercap_is_synthetic_mode()) {
        synthetic_powercap_sample_t synthetic_sample = {0};
        ulong used_power                             = 0;

        my_cluster_power_status = calloc(1, sizeof(cluster_powercap_status_t));
        if (my_cluster_power_status == NULL) {
            error("Allocating memory for synthetic power status");
            write_shared_data(0, 0, 0);
            return;
        }
        if (state_fail(synthetic_powercap_read_sample(&synthetic_sample))) {
            error("Could not read synthetic powercap sample");
            free(my_cluster_power_status);
            write_shared_data(0, 0, 0);
            return;
        }
        synthetic_powercap_apply_status(&synthetic_sample);
        used_power = synthetic_sample.current_power;
        eargm_report_event(CLUSTER_POWER, (ulong) used_power);
        verbose(VGM_PC,
                "ITER %lu: Synthetic data: used power %lu freeable power %lu requested %lu total_nodes %u "
                "active nodes %u "
                "req_power_per_node "
                "%lu total_demand %lu",
                synthetic_sample.iteration, used_power, synthetic_sample.freeable_power, synthetic_sample.requested,
                synthetic_sample.total_nodes, synthetic_sample.total_nodes - synthetic_sample.idle_nodes,
                synthetic_sample.requested_power_per_node, synthetic_sample.demand_total);
        write_shared_data(used_power, synthetic_sample.freeable_power, synthetic_sample.requested);
        check_powercap_actions((uint) used_power);
        free(my_cluster_power_status);
        debug("%sEND cluster_check_powercap----------%s", COL_BLU, COL_CLR);
        return;
    }
    if (state_fail(ear_get_powercap_status(&my_cluster_conf, &my_cluster_power_status, 1, nodes, num_eargm_nodes))) {
        verbose(VGM_PC + 1, "ear_get_powercap_status in cluster_hard_powercap returns EAR_ERROR");
        write_shared_data(0, 0, 0);
        return;
    }
    memset(&cluster_options, 0, sizeof(powercap_opt_t));
    must_send_pc_options = 0;
    eargm_report_event(CLUSTER_POWER, (ulong) my_cluster_power_status->current_power);
    print_cluster_power_status(my_cluster_power_status);
    aggregate_data(my_cluster_power_status, &cpc_ctx);

    // returning EAR_WARNING is sloppy, but for now it works
    // EAR_WARNING indicates that it needs more power, and that wants power freed
    if (eargm_plugin_hard_powercap_decision(my_cluster_power_status, &cluster_options, &must_send_pc_options,
                                            &cpc_ctx) == EAR_WARNING) {
        /* only this EARGM's nodes: with several EARGMs a cluster-wide call takes other EARGMs' power */
        pc_release_data_t rel_power = {0};
        if (state_fail(ear_release_idle_power(&my_cluster_conf, &rel_power, nodes, num_eargm_nodes))) {
            error("Error in cluster_release_idle_power");
            rel_power.released = 0;
        } else {
            verbose(VGM_PC, "%s%u Watts from idle nodes released%s", COL_GRE, rel_power.released, COL_CLR);
        }
        my_cluster_power_status->released = 0;
        my_cluster_power_status->total_powercap -= ear_min(rel_power.released, my_cluster_power_status->total_powercap);
        eargm_plugin_hard_powercap_decision(my_cluster_power_status, &cluster_options, &must_send_pc_options, &cpc_ctx);
    }

    uint cap_margin = (cpc_ctx.current_cluster_powercap > min_cluster_powercap)
                          ? cpc_ctx.current_cluster_powercap - min_cluster_powercap
                          : 0;
    uint power_to_free =
        ear_min(cap_margin, cpc_ctx.total_free + my_cluster_power_status->released * CLUSTER_PC_TO_SHARE);
    uint power_to_request = 0;
    if (cpc_ctx.total_req_greedy > cpc_ctx.greedy_allocated) // to prevent overflow
        power_to_request = cpc_ctx.total_req_greedy - cpc_ctx.greedy_allocated;

    verbose(VGM_PC, "current powercap %u total_req_greedy %u and greedy_allocated %u", cpc_ctx.current_cluster_powercap,
            cpc_ctx.total_req_greedy, cpc_ctx.greedy_allocated);
    write_shared_data(my_cluster_power_status->current_power, power_to_free,
                      power_to_request); // TODO: fill the extra power and freeable_power

    if (must_send_pc_options) {
        print_powercap_opt(&cluster_options);
        send_powercap_options_to_cluster(&cluster_options);
    } else {
        verbose(VGM_PC + 1, "There is no need to send the pc_options");
    }

    check_powercap_actions(my_cluster_power_status->total_powercap);

    free(my_cluster_power_status);
    free(cluster_options.greedy_nodes);
    free(cluster_options.extra_power);
    debug("%sEND cluster_check_powercap----------%s", COL_BLU, COL_CLR);
}

#if 0
/* Cluster HARD powercap */
void old_cluster_hard_powercap()
{
    debug("%sSTART cluster_check_powercap---------%s", COL_BLU, COL_CLR);
    if (state_fail(ear_get_powercap_status(&my_cluster_conf, &my_cluster_power_status, 1, nodes, num_eargm_nodes))) {
        verbose(VGM_PC + 1, "ear_get_powercap_status in cluster_hard_powercap returns EAR_ERROR");
        write_shared_data(0, 0, 0);
        return;
    }
    memset(&cluster_options, 0, sizeof(powercap_opt_t));
    must_send_pc_options = 0;
    eargm_report_event(CLUSTER_POWER, (ulong) my_cluster_power_status->current_power);
    print_cluster_power_status(my_cluster_power_status);
    aggregate_data(my_cluster_power_status);
    if (powercap_reallocation(my_cluster_power_status, &cluster_options) == 0) {
        /* only this EARGM's nodes: with several EARGMs a cluster-wide call takes other EARGMs' power */
        pc_release_data_t rel_power = {0};
        if (state_fail(ear_release_idle_power(&my_cluster_conf, &rel_power, nodes, num_eargm_nodes))) {
            error("Error in cluster_release_idle_power");
            rel_power.released = 0;
        } else {
            verbose(VGM_PC, "%s%u Watts from idle nodes released%s", COL_GRE, rel_power.released, COL_CLR);
        }
        my_cluster_power_status->released = 0;
        my_cluster_power_status->total_powercap -=
            ear_min(rel_power.released, my_cluster_power_status->total_powercap);
        powercap_reallocation(my_cluster_power_status, &cluster_options);
    }
    uint cap_margin =
        (cpc_ctx.current_cluster_powercap > min_cluster_powercap) ? cpc_ctx.current_cluster_powercap - min_cluster_powercap : 0;
    uint power_to_free    = ear_min(cap_margin,
                                    total_free + my_cluster_power_status->released * CLUSTER_PC_TO_SHARE);
    uint power_to_request = 0;
    if (total_req_greedy > greedy_allocated) // to prevent overflow
        power_to_request = total_req_greedy - greedy_allocated;

    verbose(VGM_PC, "current powercap %u total_req_greedy %u and greedy_allocated %u", cpc_ctx.current_cluster_powercap,
            total_req_greedy, greedy_allocated);
    write_shared_data(my_cluster_power_status->current_power, power_to_free,
                      power_to_request); // TODO: fill the extra power and freeable_power

    if (must_send_pc_options) {
        print_powercap_opt(&cluster_options);
        send_powercap_options_to_cluster(&cluster_options);
    } else {
        verbose(VGM_PC + 1, "There is no need to send the pc_options");
    }

    check_powercap_actions(my_cluster_power_status->total_powercap);

    debug("%sSTART cluster_check_powercap---------%s", COL_BLU, COL_CLR);
    if (state_fail(ear_get_powercap_status(&my_cluster_conf, &my_cluster_power_status, 1, nodes, num_eargm_nodes))) {
        verbose(VGM_PC + 1, "ear_get_powercap_status in cluster_hard_powercap returns EAR_ERROR");
        write_shared_data(0, 0, 0);
        return;
    }
    memset(&cluster_options, 0, sizeof(powercap_opt_t));
    must_send_pc_options = 0;
    eargm_report_event(CLUSTER_POWER, (ulong) my_cluster_power_status->current_power);
    print_cluster_power_status(my_cluster_power_status);
    aggregate_data(my_cluster_power_status);
    if (powercap_reallocation(my_cluster_power_status, &cluster_options) == 0) {
        /* only this EARGM's nodes: with several EARGMs a cluster-wide call takes other EARGMs' power */
        pc_release_data_t rel_power = {0};
        if (state_fail(ear_release_idle_power(&my_cluster_conf, &rel_power, nodes, num_eargm_nodes))) {
            error("Error in cluster_release_idle_power");
            rel_power.released = 0;
        } else {
            verbose(VGM_PC, "%s%u Watts from idle nodes released%s", COL_GRE, rel_power.released, COL_CLR);
        }
        my_cluster_power_status->released = 0;
        my_cluster_power_status->total_powercap -=
            ear_min(rel_power.released, my_cluster_power_status->total_powercap);
        powercap_reallocation(my_cluster_power_status, &cluster_options);
    }
    uint cap_margin =
        (cpc_ctx.current_cluster_powercap > min_cluster_powercap) ? cpc_ctx.current_cluster_powercap - min_cluster_powercap : 0;
    uint power_to_free    = ear_min(cap_margin,
                                    total_free + my_cluster_power_status->released * CLUSTER_PC_TO_SHARE);
    uint power_to_request = 0;
    if (total_req_greedy > greedy_allocated) // to prevent overflow
        power_to_request = total_req_greedy - greedy_allocated;

    verbose(VGM_PC, "current powercap %u total_req_greedy %u and greedy_allocated %u", cpc_ctx.current_cluster_powercap,
            total_req_greedy, greedy_allocated);
    write_shared_data(my_cluster_power_status->current_power, power_to_free,
                      power_to_request); // TODO: fill the extra power and freeable_power

    if (must_send_pc_options) {
        print_powercap_opt(&cluster_options);
        send_powercap_options_to_cluster(&cluster_options);
    } else {
        verbose(VGM_PC + 1, "There is no need to send the pc_options");
    }

    check_powercap_actions(my_cluster_power_status->total_powercap);

    free(my_cluster_power_status);
    free(cluster_options.greedy_nodes);
    free(cluster_options.extra_power);
    debug("%sEND cluster_check_powercap----------%s", COL_BLU, COL_CLR);
    free(my_cluster_power_status);
    free(cluster_options.greedy_nodes);
    free(cluster_options.extra_power);
    debug("%sEND cluster_check_powercap----------%s", COL_BLU, COL_CLR);
}
#endif
