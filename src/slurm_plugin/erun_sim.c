/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
#include <slurm_plugin/slurm_plugin.h>
#include <slurm_plugin/slurm_plugin_environment.h>
#include <slurm_plugin/slurm_plugin_serialization.h>

//
extern plug_serialization_t sd;

// Buffers
extern char buffer[SZ_PATH];

//
extern char **_argv;
extern int _argc;

//
extern int _sp;

spank_err_t spank_getenv(spank_t spank, const char *var, char *buf, int len)
{
    char *c;
    if (var == NULL || buf == NULL) {
        return ESPANK_ERROR;
    }
    if ((c = ear_getenv(var)) == NULL) {
        return ESPANK_ERROR;
    }
    snprintf(buf, len, "%s", c);
    return ESPANK_SUCCESS;
}

spank_err_t spank_setenv(spank_t spank, const char *var, const char *val, int overwrite)
{
    if (var == NULL || val == NULL) {
        return ESPANK_ERROR;
    }
    if (setenv(var, val, overwrite) == -1) {
        return ESPANK_ERROR;
    }
    return ESPANK_SUCCESS;
}

spank_err_t spank_unsetenv(spank_t spank, const char *var)
{
    if (var == NULL) {
        return ESPANK_ERROR;
    }
    return (spank_err_t) (unsetenv(var) == 0);
}

spank_context_t spank_context(void)
{
    return (spank_context_t) _sp;
}

spank_err_t spank_get_item(spank_t spank, spank_item_t item, int *p)
{
    if (item == S_JOB_ID) {
        *p = atoi(getenv(Var.job_id.slurm));
    } else if (item == S_JOB_STEPID) {
        *p = atoi(getenv(Var.step_id.slurm));
    } else if (item == S_TASK_EXIT_STATUS) {
        *p = sd.subject.exit_status;
    } else if (item == S_TASK_ID) {
        *p = atoi(getenv(Var.local_id.slurm));
    } else if (item == S_STEP_CPUS_PER_TASK) {
        *p = 1;
    } else {
        *p = 0;
    }
    return ESPANK_SUCCESS;
}

hostlist_t slurm_hostlist_create(char *node_list)
{
    // Simulation of the slurm.h function: create a new hostlist from a string
    // representation, e.g. "cmp[0-5,12,20-25]". But for ERUN, the list is
    // coming like "cmp1,cmp2,cmp3,etc". Then, we return a copy.
    char *copy = calloc(strlen(node_list) + 8, sizeof(char));
    return (hostlist_t) strcpy(copy, node_list);
}

char *slurm_hostlist_shift(hostlist_t host_list)
{
    static char buffer[128] = {0};
    static char *copy = NULL;
    char *p = NULL;
    // Returning a node one by one
    if (copy == NULL) {
        copy = (char *) slurm_hostlist_create(host_list);
    }
    if ((p = strchr(copy, ',')) != NULL) {
        *p = '\0';
        strncpy(buffer, copy, 127);
        copy = &p[1];
    } else if (strlen(copy) > 1) {
        strncpy(buffer, copy, 127);
        copy = &copy[strlen(copy)];
    } else {
        copy = NULL;
        return NULL;
    }
    // No need to free, because ERUN does it only once
    return buffer;
}

spank_err_t spank_option_register_print(spank_t sp, struct spank_option *opt)
{
    int n = strlen(opt->usage);
    int i = 0;
    int o = 0;
    char c;

    printf("\t--%s\t\t", opt->name);
    if (strlen(opt->name) < 5) {
        printf("\t");
    }
    if (n < 64) {
        printf("%s\n", opt->usage);
        return ESPANK_SUCCESS;
    }
    while (i < n) {
        c = opt->usage[i];
        printf("%c", c);
        o = (o == 1) | (i != 0 && (i % 64) == 0);
        if (o && (c == ' ' || c == ',')) {
            printf("\n\t\t\t\t");
            o = 0;
        }
        i += 1;
    }
    printf("\n");

    return ESPANK_SUCCESS;
}

spank_err_t spank_option_register_call(int argc, char *argv[], spank_t sp, struct spank_option *opt)
{
    char *p;

    sprintf(buffer, "--%s=", opt->name);
    p = plug_acav_get(argc, argv, buffer);

    if (p == NULL) {
        return ESPANK_SUCCESS;
    }
    if (strlen(p) == 0) {
        return ESPANK_SUCCESS;
    }

    opt->cb(0, p, 0);

    return ESPANK_SUCCESS;
}

spank_err_t spank_option_register(spank_t sp, struct spank_option *opt)
{
    if (plug_context_is(sp, Context.error)) {
        return spank_option_register_print(sp, opt);
    } else if (plug_context_is(sp, Context.srun)) {
        return spank_option_register_call(_argc, _argv, sp, opt);
    }
    return ESPANK_SUCCESS;
}
