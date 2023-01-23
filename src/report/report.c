/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <pthread.h>
#include <common/config.h>
#include <report/report.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/system/symplug.h>
#include <common/types/configuration/cluster_conf.h>

#define P_NUM   8
#define S_NUM   7
#define S_FLAGS RTLD_LOCAL | RTLD_NOW

static const char *names[] =
{
    "report_init",
    "report_dispose",
    "report_applications",
    "report_loops",
    "report_events",
    "report_periodic_metrics",
    "report_misc"
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static report_ops_t ops[P_NUM];
static uint plugins_count;
static uint init;

static void append(report_ops_t *aux)
{
    int f, s;
    ullong *aux1 = (ullong *) aux;
    for (s = 0; s < S_NUM; ++s) {
        if (aux1[s] == 0LLU) {
            continue;
        }
        for (f = 0; f < P_NUM; ++f) {
            ullong *aux2 = (ullong *) &ops[f];
            ullong *aux3 = &aux2[s];
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
        debug("Loading '%s' plugin", path);
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
    while (pthread_mutex_trylock(&lock));
    if (init != 0) {
        goto leave;
    }
    static_load(install_path, libs);
    static_load(install_path, getenv(FLAG_REPORT_ADD));
    init = 1;
leave:
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t report_create_id(report_id_t *id, int local_rank, int global_rank, int master_rank){
    id->local_rank = local_rank;
    id->global_rank = global_rank;
    id->master_rank = master_rank;
		id->pid         = getpid();
    debug("created new id - lrank %d - grank %d - master %d", id->local_rank, id->global_rank, id->master_rank);
    return EAR_SUCCESS;
}


#define call_0(op, void) \
    ops[p]. op ()
#define call_1(op, type1, var1) \
    ops[p]. op (var1)
#define call_2(op, type1, var1, type2, var2) \
    ops[p]. op (var1, var2)
#define call_3(op, type1, var1, type2, var2, type3, var3) \
    ops[p]. op (var1, var2, var3)
#define call_4(op, type1, var1, type2, var2, type3, var3, type4, var4) \
    ops[p]. op (var1, var2, var3, var4)


#define func_0(op, void) \
    state_t op ()
#define func_1(op, type1, var1) \
    state_t op (type1 var1)
#define func_2(op, type1, var1, type2, var2) \
    state_t op (type1 var1, type2 var2)
#define func_3(op, type1, var1, type2, var2, type3, var3) \
    state_t op (type1 var1, type2 var2, type3 var3)
#define func_4(op, type1, var1, type2, var2, type3, var3, type4, var4) \
    state_t op (type1 var1, type2 var2, type3 var3, type4 var4)


#define define(NARGS, op, ...) \
    func_ ## NARGS (op, __VA_ARGS__) { \
        int p; \
        for (p = 0; p < plugins_count; ++p) { \
            if (ops[p]. op != NULL) { \
                call_ ## NARGS(op, __VA_ARGS__); \
            } \
        } \
        return EAR_SUCCESS; \
    }

define(2, report_init, report_id_t *, id, cluster_conf_t *, cconf);
define(1, report_dispose, report_id_t *, id);
define(3, report_applications, report_id_t *, id, application_t *, apps, uint, count);
define(3, report_loops, report_id_t *, id, loop_t *, loops, uint, count);
define(3, report_events, report_id_t *, id, ear_event_t *, eves, uint, count);
define(3, report_periodic_metrics,report_id_t *, id, periodic_metric_t *, mets, uint, count);
define(4, report_misc, report_id_t *, id, uint, type, const char *, data, uint, count);

#if TEST
int main(int argc, char *argv[])
{
		cluster_conf_t cconf;
		report_id_t id;
    report_load(NULL, "plug_dummy.so:plug_dummy2.so");
    report_init(&id,&cconf);
    report_applications(&id,NULL, 0);
    report_loops(&id,NULL, 0);
    report_events(&id,NULL, 0);
    report_periodic_metrics(&id,NULL, 0);
    report_dispose(&id);
    return 0;
}
#endif
