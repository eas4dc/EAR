/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/config.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/system/symplug.h>
#include <common/types/configuration/cluster_conf.h>
#include <pthread.h>
#include <report/report.h>
#include <stdlib.h>

#define P_NUM   8
#define S_NUM   7
#define S_FLAGS RTLD_LOCAL | RTLD_NOW

static const char *names[] = {"report_init",   "report_dispose",          "report_applications", "report_loops",
                              "report_events", "report_periodic_metrics", "report_misc"};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static report_ops_t ops[P_NUM];
static uint plugins_count;
static uint init;

static void append(report_ops_t *aux)
{
    int f, s;

    // Sets the function 'F' from 'aux' OPs, to the first free position of 'ops'
    // array.
    ullong *aux1 = (ullong *) aux;
    // From function 0 (init) to function 6 (misc)
    for (s = 0; s < S_NUM; ++s) {
        if (aux1[s] == 0LLU) {
            continue;
        }
        // From plugin 0 to plugin 8 (P_NUM)
        for (f = 0; f < P_NUM; ++f) {
            // Getting plugin F
            ullong *aux2 = (ullong *) &ops[f];
            // Getting function S
            ullong *aux3 = &aux2[s];
            // If function S from plugin F is free, set the OP function here
            if (*aux3 == 0LLU) {
                *aux3 = aux1[s];
                break;
            }
        }
    }
    plugins_count++;
}

static void static_load(const char *install_path, const char *_libs)
{
    report_ops_t ops_aux;
    char path[SZ_PATH];
    char libs[SZ_PATH];
    char *lib;
    state_t s;

    // If no library, no sense
    if (_libs == NULL) {
        return;
    }
    // Using current folder if install path is NULL
    if (install_path == NULL) {
        install_path = ".";
    }
    // Cleaning plugin auxiliar symbols
    memset((void *) &ops_aux, 0, sizeof(report_ops_t));
    // Copying original list to prevent any change
    strcpy(libs, _libs);
    // List iterations
    lib = strtok(libs, ":");
    while (lib != NULL) {
        if (plugins_count == P_NUM) {
            error("Unable to load report plugin %s: reached maximum of loaded plugins", lib);
        }
        // Loading /install/lib/plugins/report/lib.so
        sprintf(path, "%s/reports/%s", install_path, lib);
        if (state_fail(s = plug_open(path, (void **) &ops_aux, (cchar **) names, S_NUM, S_FLAGS))) {
            // Loading ./lib.so
            sprintf(path, "./%s", lib);
            debug("Loading '%s' plugin", path);
            if (state_fail(s = plug_open(path, (void **) &ops_aux, (cchar **) names, S_NUM, S_FLAGS))) {
                // Loading /???/lib.so
                debug("Loading '%s' plugin", lib);
                if (state_fail(s = plug_open(lib, (void **) &ops_aux, (cchar **) names, S_NUM, S_FLAGS))) {
                    error("Unable to load report plugin %s (%d)", state_msg, s);
                    goto next;
                }
            }
        }
        debug("Loaded report plugin: %s", lib);
        append(&ops_aux);
    next:
        lib = strtok(NULL, ":");
    }
}

state_t report_load(const char *install_path, const char *libs)
{
    state_t s = EAR_SUCCESS;
    while (pthread_mutex_trylock(&lock))
        ;
    if (init != 0) {
        goto leave;
    }
    static_load(install_path, libs);
    static_load(install_path, ear_getenv(FLAG_REPORT_ADD));
    if (plugins_count > 0) {
        init = 1;
    } else {
        state_msg = "No plugins loaded";
        s         = EAR_ERROR;
    }
leave:
    pthread_mutex_unlock(&lock);
    return_msg(s, state_msg);
}

state_t report_create_id(report_id_t *id, int local_rank, int global_rank, int master_rank)
{
    id->local_rank  = local_rank;
    id->global_rank = global_rank;
    id->master_rank = master_rank;
    id->pid         = getpid();
    debug("created new id - lrank %d - grank %d - master %d", id->local_rank, id->global_rank, id->master_rank);
    return EAR_SUCCESS;
}

#define call_0(op, void)                                               ops[p].op()
#define call_1(op, type1, var1)                                        ops[p].op(var1)
#define call_2(op, type1, var1, type2, var2)                           ops[p].op(var1, var2)
#define call_3(op, type1, var1, type2, var2, type3, var3)              ops[p].op(var1, var2, var3)
#define call_4(op, type1, var1, type2, var2, type3, var3, type4, var4) ops[p].op(var1, var2, var3, var4)

#define func_0(op, void)                                               state_t op()
#define func_1(op, type1, var1)                                        state_t op(type1 var1)
#define func_2(op, type1, var1, type2, var2)                           state_t op(type1 var1, type2 var2)
#define func_3(op, type1, var1, type2, var2, type3, var3)              state_t op(type1 var1, type2 var2, type3 var3)
#define func_4(op, type1, var1, type2, var2, type3, var3, type4, var4)                                                 \
    state_t op(type1 var1, type2 var2, type3 var3, type4 var4)

#define define(NARGS, st, op, ...)                                                                                     \
    func_##NARGS(op, __VA_ARGS__)                                                                                      \
    {                                                                                                                  \
        state_t sret = st;                                                                                             \
        int p;                                                                                                         \
        for (p = 0; p < plugins_count; ++p) {                                                                          \
            if (ops[p].op != NULL) {                                                                                   \
                if (state_ok(call_##NARGS(op, __VA_ARGS__))) {                                                         \
                    sret = EAR_SUCCESS;                                                                                \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
        return sret;                                                                                                   \
    }

define(2, EAR_ERROR, report_init, report_id_t *, id, cluster_conf_t *, cconf);
define(1, EAR_SUCCESS, report_dispose, report_id_t *, id);
define(3, EAR_SUCCESS, report_applications, report_id_t *, id, application_t *, apps, uint, count);
define(3, EAR_SUCCESS, report_loops, report_id_t *, id, loop_t *, loops, uint, count);
define(3, EAR_SUCCESS, report_events, report_id_t *, id, ear_event_t *, eves, uint, count);
define(3, EAR_SUCCESS, report_periodic_metrics, report_id_t *, id, periodic_metric_t *, mets, uint, count);
define(4, EAR_SUCCESS, report_misc, report_id_t *, id, uint, type, const char *, data, uint, count);

#if TEST
int main(int argc, char *argv[])
{
    cluster_conf_t cconf;
    report_id_t id;
    report_load(NULL, "plug_dummy.so:plug_dummy2.so");
    report_init(&id, &cconf);
    report_applications(&id, NULL, 0);
    report_loops(&id, NULL, 0);
    report_events(&id, NULL, 0);
    report_periodic_metrics(&id, NULL, 0);
    report_dispose(&id);
    return 0;
}
#endif
