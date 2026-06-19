/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <daemon/remote_api/eard_rapi.h>
#include <global_manager/cluster_powercap.h>
#include <global_manager/plugins/powercap_plugin.h>

static cluster_conf_t my_cluster_conf = {0};

state_t plugin_init(cluster_conf_t conf[static 1])
{
    my_cluster_conf = *conf;
    return EAR_SUCCESS;
}

static state_t _read_non_computational_power(uint64_t *power)
{
    *power = 0;
    return EAR_SUCCESS;
}

/* This function is executed when there is not enough power for new running nodes */
void reduce_allocation(cluster_powercap_status_t *cs, powercap_opt_t *cluster_options, uint min_reduction,
                       cluster_powercap_ctx_t ctx[static 1])
{
    int i = 0;
    uint red1, red, red_node;
    int new_extra;
    if (ctx->num_extra == 0) {
        error("We need to reallocated power and there is no extra power ");
        return;
    }
    red_node = min_reduction / ctx->num_extra;
    verbose(VGM_PC + 1, "SEQ reduce_allocation implementation avg red=%uW", red_node);
    debug("ROUND 1- reducing avg power ");
    while ((min_reduction > 0) && (i < cs->num_greedy)) {
        if (cs->greedy_data[i].extra_power) {
            red1      = ear_min(red_node, cs->greedy_data[i].extra_power);
            red       = ear_min(red1, min_reduction);
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
            red       = ear_min(red1, min_reduction);
            new_extra = -red;
            verbose(VGM_PC + 1, "%sreducing %d W to node %d %s", COL_RED, new_extra, i, COL_CLR);
            cluster_options->extra_power[i] += new_extra;
            min_reduction -= red;
        }
        i++;
    }
}

void allocate_free_power_to_greedy_nodes(cluster_powercap_status_t *cluster_status, powercap_opt_t *cluster_options,
                                         uint *total_free, bool must_send_pc_options[static 1],
                                         cluster_powercap_ctx_t ctx[static 1])
{
    int i, more_power;
    uint pending = *total_free;
    if (ctx->num_greedy == 0) {
        debug("NO greedy nodes, returning");
        return;
    }
    if (pending == 0) {
        debug("NO free power, returning");
        return;
    }
    *must_send_pc_options = true;
    debug("allocate_free_power_to_greedy_nodes----------------");
    verbose(VGM_PC + 1, "Total extra power for nodes with NO current extra power %u W, %u nodes", ctx->req_no_extra,
            ctx->num_no_extra);
    verbose(VGM_PC + 1, "Nodes with extra power already allocated %u", ctx->num_greedy);
    ctx->greedy_allocated = 0;
    if (ctx->num_no_extra > 0) {
        if (ctx->req_no_extra < pending)
            more_power = -1;
        else
            more_power = pending / ctx->num_no_extra;
        verbose(VGM_PC + 1, "STAGE_1-Allocating %d watts to new greedy nodes first (-1 => alloc=req)", more_power);
        for (i = 0; i < cluster_status->num_greedy; i++) {
            if ((cluster_status->greedy_data[i].requested) && (!cluster_status->greedy_data[i].extra_power)) {
                if (more_power < 0)
                    cluster_options->extra_power[i] = cluster_status->greedy_data[i].requested;
                else
                    cluster_options->extra_power[i] = ear_min(more_power, cluster_status->greedy_data[i].requested);
                pending -= cluster_options->extra_power[i];
                ctx->greedy_allocated += cluster_options->extra_power[i];
            }
        }
    }
    /* If there is pending power to allocate */
    verbose(VGM_PC + 1, "STAGE-2 allocating %uW to the rest of greedy nodes", pending);
    if (pending) {
        verbose(VGM_PC + 1, "Allocating %u watts to greedy nodes ", pending);
        more_power = pending / ctx->num_greedy;
        for (i = 0; i < cluster_status->num_greedy; i++) {
            if ((cluster_status->greedy_data[i].requested) && (!cluster_options->extra_power[i])) {
                cluster_options->extra_power[i] = ear_min(cluster_status->greedy_data[i].requested, more_power);
                pending -= cluster_options->extra_power[i];
                ctx->greedy_allocated += cluster_options->extra_power[i];
            }
        }
    }
    *total_free = pending;
}

uint powercap_reallocation(cluster_powercap_status_t *cluster_status, powercap_opt_t *cluster_options,
                           bool must_send_pc_options[static 1], cluster_powercap_ctx_t ctx[static 1])
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
    cluster_options->cluster_perc_power = (cluster_status->total_powercap * 100) / ctx->current_cluster_powercap;
    memcpy(cluster_options->greedy_nodes, cluster_status->greedy_nodes, cluster_status->num_greedy * sizeof(int));

    ctx->total_free                  = ctx->current_cluster_powercap - cluster_status->total_powercap;
    uint64_t non_computational_power = 0;
    if (!state_ok(_read_non_computational_power(&non_computational_power))) {
        warning("Non-computational power could not be read");
    } else {
        ctx->total_free -= non_computational_power;
    }

    verbose(VGM_PC + 1, "Total power %u , requested for new %u (potential) released %u extra_req %u extra_used %u",
            ctx->current_cluster_powercap, cluster_status->requested, cluster_status->released, ctx->total_req_greedy,
            ctx->total_extra_power);
    verbose(VGM_PC + 1, "Free power before reallocation %d", ctx->total_free);

    if (cluster_status->requested)
        *must_send_pc_options = true;

    /* Allocated power + default requested must be less that maximum: ASK_DEF */
    if ((cluster_status->total_powercap + cluster_status->requested) <= ctx->current_cluster_powercap) {
        if (cluster_status->requested) {
            verbose(VGM_PC + 1, "There is enough power for new running jobs");
            ctx->total_free =
                ctx->current_cluster_powercap - (cluster_status->total_powercap + cluster_status->requested);
            verbose(VGM_PC + 1, "Free power after allocating power to new jobs %d", ctx->total_free);
        }
        if (ctx->total_req_greedy == 0) {
            return 1;
        }
        /* At this point we know there is greedy power requested: GREEDY */
        if (ctx->total_req_greedy <= ctx->total_free) {
            verbose(VGM_PC + 1,
                    "There is more free power than requested by greedy nodes=> allocating all the req power");
            ctx->total_free -= ctx->total_req_greedy;
            for (i = 0; i < cluster_status->num_greedy; i++)
                cluster_options->extra_power[i] = cluster_status->greedy_data[i].requested;
            *must_send_pc_options = true;
        } else {
            verbose(
                VGM_PC + 1, "There is not enough power for all the greedy nodes (free %d req %u)(used %u allocated %u)",
                ctx->total_free, ctx->total_req_greedy, cluster_status->current_power, cluster_status->total_powercap);
            if (cluster_status->released == 0) {
                verbose(VGM_PC + 1, "Anyway there is not enough power ");
                cluster_options->num_greedy = cluster_status->num_greedy;
                allocate_free_power_to_greedy_nodes(cluster_status, cluster_options, (uint *) &ctx->total_free,
                                                    must_send_pc_options, ctx);
            } else if (cluster_status->released) {
                return 0; // We try to release power to have power for greedy nodes
            }
        }
    } else {
        /* There is not enough power for new jobs (ASK_DEF), we must reduce the extra allocation */
        if (cluster_status->released == 0) {
            verbose(VGM_PC + 1, "We must reduce the extra allocation (used %u allocated %u)",
                    cluster_status->current_power, cluster_status->total_powercap);
            min_reduction =
                (cluster_status->total_powercap + cluster_status->requested) - ctx->current_cluster_powercap;
            ctx->total_free = 0;
            reduce_allocation(cluster_status, cluster_options, min_reduction, ctx);
            *must_send_pc_options = true;
        } else if (cluster_status->released)
            return 0; // We try to release power to have power for greedy nodes
    }
    return 1;
}

state_t plugin_soft_powercap_decision(power_check_t power[static 1], uint64_t used_power)
{

    return EAR_SUCCESS;
}

state_t plugin_hard_powercap_decision(powercap_status_t status[static 1], powercap_opt_t opt[static 1],
                                      bool send_options[static 1], cluster_powercap_ctx_t ctx[static 1])
{
    if (powercap_reallocation(status, opt, send_options, ctx) == 0) {
        return EAR_WARNING;
    }

    return EAR_SUCCESS;
}
