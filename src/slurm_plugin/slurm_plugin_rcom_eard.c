/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <slurm_plugin/slurm_plugin_rcom.h>

static int plug_rcom_eard_connect(spank_t sp, plug_serialization_t *sd, char *by)
{
    int port   = sd->pack.eard.servs.eard.port;
    char *node = sd->subject.host;

    if (remote_connect(node, port) < 0) {
        plug_verbose(sp, 2, "connecting to EARD: '%s:%d': FAILED (by %s)", node, port, by);
        return ESPANK_ERROR;
    }
    plug_verbose(sp, 2, "connected to EARD: '%s:%d': OK (by %s)", node, port, by);
    return ESPANK_SUCCESS;
}

static int plug_rcom_eard_disconnect(spank_t sp, plug_serialization_t *sd, char *by)
{
    remote_disconnect();
    return ESPANK_SUCCESS;
}

static int plug_rcom_eard_job_sbatch(spank_t sp, plug_serialization_t *sd, int new_job)
{
    plug_verbose(sp, 2, "function plug_rcom_eard_job_sbatch");
    char *net_ext = sd->pack.eard.servs.net_ext;
    int port      = sd->pack.eard.servs.eard.port;
    int i         = 0;

    if (sd->job.nodes_count == 0) {
        plug_error(sp, "nodes counter is 0");
        return ESPANK_ERROR;
    }
    while (i < sd->job.nodes_count) {
        plug_verbose(sp, 2, "connecting to EARD %d/%d: '%s:%d' (by SBATCH/SALLOC)", i, sd->job.nodes_count - 1,
                     sd->job.nodes_list[i], port);
        ++i;
    }
    if (new_job) {
        ear_nodelist_new_job(&sd->job.app, port, net_ext, sd->job.nodes_list, sd->job.nodes_count);
        // To know if EARD was contacted previosly by SBATCH
        setenv_agnostic(sp, Var.con_eard_sbatch.mod, "1", 1);
    } else {
        ear_nodelist_end_job(sd->job.app.job.id, sd->job.app.job.step_id, port, net_ext, sd->job.nodes_list,
                             sd->job.nodes_count);
    }
    return ESPANK_SUCCESS;
}

static int plug_rcom_eard_job_srun(spank_t sp, plug_serialization_t *sd, int new_job)
{
    plug_verbose(sp, 2, "function plug_rcom_eard_job_srun");
#if ERUN
    if (sd->erun.avoid_connecting) {
        return ESPANK_SUCCESS;
    }
#endif
    // Hostlist get
    if (plug_rcom_eard_connect(sp, sd, "SRUN") == ESPANK_ERROR) {
        return ESPANK_ERROR;
    }
    if (new_job) {
        if (ear_node_new_job(&sd->job.app)) {
            plug_print_application(sp, &sd->job.app);
            // To know if EARD was contacted previously by SRUN
            setenv_agnostic(sp, Var.con_eard_srun.mod, "1", 1);
        } else {
            plug_verbose(sp, 2, "EARD communication error on REMOTE SRUN: %d", sd->job.task.pid);
        }
    } else {
        ear_node_end_job(sd->job.app.job.id, sd->job.app.job.step_id);
    }
    plug_rcom_eard_disconnect(sp, sd, "SRUN");
    return ESPANK_SUCCESS;
}

int plug_rcom_eard_job_start(spank_t sp, plug_serialization_t *sd)
{
    plug_verbose(sp, 2, "function plug_rcom_eard_job_start");
    if (plug_context_was(sd, Context.srun)) {
        return plug_rcom_eard_job_srun(sp, sd, 1);
    } else {
        return plug_rcom_eard_job_sbatch(sp, sd, 1);
    }
}

int plug_rcom_eard_job_finish(spank_t sp, plug_serialization_t *sd)
{
    plug_verbose(sp, 2, "function plug_rcom_eard_job_finish");
    if (plug_context_was(sd, Context.srun)) {
        return plug_rcom_eard_job_srun(sp, sd, 0);
    } else {
        return plug_rcom_eard_job_sbatch(sp, sd, 0);
    }
}

int plug_rcom_eard_task_start(spank_t sp, plug_serialization_t *sd)
{
    plug_verbose(sp, 2, "function plug_rcom_eard_task_start");
#if ERUN
    if (sd->erun.avoid_connecting) {
        return ESPANK_SUCCESS;
    }
#endif
    if (plug_rcom_eard_connect(sp, sd, "TASK") == ESPANK_ERROR) {
        return ESPANK_ERROR;
    }
    // 1: OK, 0: KO
    if (ear_node_new_task(&sd->job.task)) {
        // To know if EARD was contacted previously by SRUN
        setenv_agnostic(sp, Var.con_eard_task.mod, "1", 1);
    } else {
        plug_verbose(sp, 2, "EARD communication error on REMOTE SRUN/TASK: %d", sd->job.task.pid);
    }
    plug_rcom_eard_disconnect(sp, sd, "TASK");
    return ESPANK_SUCCESS;
}

int plug_rcom_eard_task_finish(spank_t sp, plug_serialization_t *sd)
{
    plug_verbose(sp, 2, "function plug_rcom_eard_task_finish");
    return ESPANK_SUCCESS;
}