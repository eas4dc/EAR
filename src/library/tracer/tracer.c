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

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/config.h>
#include <common/config/config_env.h>
// #define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <library/tracer/tracer.h>
#include <library/common/utils.h>
#include <common/system/symplug.h>
#include <common/types/signature.h>

#ifdef EAR_GUI

/** \struct Defines the tracer API symbols. Your tracer plugins should implement some of the below functions to work. */ 
typedef struct traces_symbols {

    /** Executed at application start.
     * @param app The Job name.
     * @param global_rank The global MPI rank of the calling process.
     * @param local_rank The node local MPI rank of the calling process.
     * @param nodes The number of nodes of the current job.
     * @param mpis The number of MPI processes on the current node.
     * @param ppn The number of MPI processes on the current node. */
    void (*traces_init) (char *app, int global_rank, int local_rank, int nodes, int mpis, int ppn);

    void (*traces_end)(int global_rank,int local_rank, unsigned long int total_ener);
    void (*traces_start)();
    void (*traces_stop)();
    void (*traces_frequency)(int global_rank, int local_rank, unsigned long f);
    void (*traces_new_signature)(int global_rank, int local_rank, signature_t *sig);
    void (*traces_PP)(int global_rank, int local_rank, double seconds, double power); 
    void (*traces_new_n_iter)(int global_rank, int local_rank, ullong period_id, int loop_size, int iterations);
    void (*traces_new_period)(int global_rank, int local_rank, ullong period_id);
    void (*traces_end_period)(int global_rank, int local_rank); 
    void (*traces_policy_state)(int global_rank, int local_rank, int state);
    void (*traces_dynais)(int global_rank, int local_rank, int state);
    void (*traces_earl_mode_dynais)(int global_rank, int local_rank);
    void (*traces_earl_mode_periodic)(int global_rank, int local_rank);
    void (*traces_reconfiguration)(int global_rank, int local_rank);
    int (*traces_are_on)();
    void (*traces_mpi_init)();
    void (*traces_mpi_call)(int global_rank, int local_rank, ulong ev, ulong a1, ulong a2, ulong a3);
    void (*traces_mpi_end)();
    void (*traces_generic_event)(int global_rank, int local_rank, int event, int value);
} trace_sym_t;

static trace_sym_t trace_syms_fun;
static int trace_plugin=0;
const int       trace_syms_n = 20;

const char     *trace_syms_nam[] = {
    "traces_init",
    "traces_end",
    "traces_start",
    "traces_stop",
    "traces_frequency",
    "traces_new_signature",
    "traces_PP",
    "traces_new_n_iter",
    "traces_new_loop",
    "traces_end_loop",
    "traces_policy_state",
    "traces_dynais",
    "traces_earl_mode_dynais",
    "traces_earl_mode_periodic",
    "traces_reconfiguration",
    "traces_are_on",
    "traces_mpi_init",
    "traces_mpi_call",
    "traces_mpi_end",
    "traces_generic_event"
};

/*
 *
 *
 * ear_api.c
 *
 *
 */


void traces_init(settings_conf_t *conf, char *app, int global_rank, int local_rank, int nodes, int mpis, int ppn)
{
    char *traces_plugin;
    traces_plugin=ear_getenv(FLAG_TRACE_PLUGIN);

    if (traces_plugin==NULL) {
        trace_plugin=0;
        debug("traces OFF");
        return;
    }
    trace_plugin=1;

    debug("traces library");

    if (conf == NULL) {
        debug("NULL configuration in traces_init");
        return;
    }

    char my_plug_path[SZ_PATH];
    char tracer_path[SZ_PATH];
    state_t st;

    /* my_plug_path := [EAR_INSTALL_PATH/lib/plugins | HACK_EARL_INSTALL_PATH/plugins] */
    st = utils_create_plugin_path(my_plug_path, conf->installation.dir_plug,
            ear_getenv(HACK_EARL_INSTALL_PATH), conf->user_type);

    // Using current directory if there's no plugins path
    if (state_fail(st)) {
        sprintf(my_plug_path, ".");
    }

    // Loading install/lib/plugins/tracer/tracer.so OR ./tracer/tracer.so
    xsprintf(tracer_path, "%s/tracer/%s", my_plug_path, traces_plugin);
    verbose(2, "Trying to load '%s' plugin", tracer_path);

    if (state_fail(st = symplug_open(tracer_path, (void **) &trace_syms_fun, trace_syms_nam, trace_syms_n))) {
        // Loading ./tracer.so
        sprintf(tracer_path, "./%s", traces_plugin);
        verbose(2, "Trying to load '%s' plugin", tracer_path);

        if (state_fail(st = symplug_open(tracer_path, (void **) &trace_syms_fun,
                        trace_syms_nam, trace_syms_n))) {
            verbose(2, "Trying to load '%s' plugin", traces_plugin);

            if (state_fail(st = symplug_open(traces_plugin, (void **) &trace_syms_fun,
                            trace_syms_nam, trace_syms_n))) {
                verbose(2, "%sERROR%s Unable to load tracer plugin %s (%d)", COL_RED, COL_CLR, state_msg, st);
                trace_plugin = 0;
                return;
            }
        }
    }
    verbose(1, "Tracer plugin '%s%s%s' has been loaded with EAR.", COL_GRE, traces_plugin, COL_CLR);

    /*
       int ret = symplug_open(traces_plugin, (void **) &trace_syms_fun, trace_syms_nam, trace_syms_n);
       if (ret!=EAR_SUCCESS){ 
       debug("symplug_open() in library/traces returned %d (%s)", ret, intern_error_str);
       trace_plugin=0;
       return;
       }
       */

    if (trace_syms_fun.traces_init!=NULL){
        debug("trace_syms_fun.traces_init");
        trace_syms_fun.traces_init(app,global_rank,local_rank,nodes,mpis,ppn);
    }
    return;
}

// ear_api.c
void traces_end(int global_rank, int local_rank, unsigned long total_energy)
{
    debug("trace_end");
    if (trace_plugin && trace_syms_fun.traces_end!=NULL){
        trace_syms_fun.traces_end(global_rank,local_rank,total_energy);	
    }
    return;
}

// ear_states.c
void traces_new_period(int global_rank, int local_rank, ullong loop_id)
{
    debug("traces_new_period");
    if (trace_plugin && trace_syms_fun.traces_new_period!=NULL){
        trace_syms_fun.traces_new_period(global_rank,local_rank,loop_id);
    }
    return;
}

// ear_api.c
void traces_new_n_iter(int global_rank, int local_rank, ullong loop_id, int loop_size, int iterations)
{
    debug("traces_new_n_iter");
    if (trace_plugin && trace_syms_fun.traces_new_n_iter!=NULL){
        trace_syms_fun.traces_new_n_iter(global_rank,local_rank,loop_id,loop_size,iterations);
    }
    return;
}

// ear_api.c
void traces_end_period(int global_rank, int local_rank)
{
    debug("traces_end_period");
    if (trace_plugin && trace_syms_fun.traces_end_period!=NULL){
        trace_syms_fun.traces_end_period(global_rank,local_rank);
    }
    return;
}

// ear_states.c
void traces_new_signature(int global_rank, int local_rank, signature_t *sig)
{
    debug("traces_new_signature");
    if (trace_plugin && trace_syms_fun.traces_new_signature!=NULL){
        trace_syms_fun.traces_new_signature(global_rank,local_rank,sig);
    }
    return;
}

// ear_states.c
void traces_PP(int global_rank, int local_rank, double seconds, double power)
{
    debug("traces_PP");
    if (trace_plugin && trace_syms_fun.traces_PP!=NULL){
        trace_syms_fun.traces_PP(global_rank,local_rank,seconds,power);
    }
    return;

}

// ear_api.c, ear_states.c
void traces_frequency(int global_rank, int local_rank, unsigned long f)
{
    debug("traces_frequency");
    if (trace_plugin && trace_syms_fun.traces_frequency!=NULL){
        trace_syms_fun.traces_frequency(global_rank,local_rank,f);
    }
    return;
}


void traces_policy_state(int global_rank, int local_rank, int state)
{
    debug("traces_policy_state");
    if (trace_plugin && trace_syms_fun.traces_policy_state!=NULL){
        trace_syms_fun.traces_policy_state(global_rank,local_rank,state);
    }
    return;
}

void traces_dynais(int global_rank, int local_rank, int state)
{
    debug("traces_dynais");
    if (trace_plugin && trace_syms_fun.traces_dynais!=NULL){
        trace_syms_fun.traces_dynais(global_rank,local_rank,state);
    }
    return;
}
void traces_earl_mode_dynais(int global_rank, int local_rank)
{
    debug("traces_earl_mode_dynais");
    if (trace_plugin && trace_syms_fun.traces_earl_mode_dynais!=NULL){
        trace_syms_fun.traces_earl_mode_dynais(global_rank,local_rank);
    }
    return;
}
void traces_earl_mode_periodic(int global_rank, int local_rank)
{
    debug("traces_earl_mode_periodic");
    if (trace_plugin && trace_syms_fun.traces_earl_mode_periodic!=NULL){
        trace_syms_fun.traces_earl_mode_periodic(global_rank,local_rank);
    }
    return;
}

void traces_reconfiguration(int global_rank, int local_rank)
{
    debug("traces_reconfiguration");
    if (trace_plugin && trace_syms_fun.traces_reconfiguration!=NULL){
        trace_syms_fun.traces_reconfiguration(global_rank,local_rank);
    }
    return;
}

int traces_are_on()
{
    debug("traces_are_on");
    if (trace_plugin && trace_syms_fun.traces_are_on!=NULL){
        return trace_syms_fun.traces_are_on();
    }
    return 0;	
}
void traces_start()
{
    debug("traces_start");
    if (trace_plugin && trace_syms_fun.traces_start!=NULL){
        trace_syms_fun.traces_start();
    }
    return;
}

void traces_stop()
{
    debug("traces_stop");
    if (trace_plugin && trace_syms_fun.traces_stop!=NULL){
        trace_syms_fun.traces_stop();
    }
    return;
}


void traces_mpi_init()
{
    debug("traces_mpi_init");
    if (trace_plugin && trace_syms_fun.traces_mpi_init!=NULL){
        trace_syms_fun.traces_mpi_init();
    }
    return;
}
void traces_mpi_call(int global_rank, int local_rank, ulong ev, ulong a1, ulong a2, ulong a3)
{
    if (trace_plugin && trace_syms_fun.traces_mpi_call!=NULL){
        trace_syms_fun.traces_mpi_call(global_rank,local_rank,ev,a1,a2,a3);
    }
    return;
}

void traces_mpi_end()
{
    debug("traces_mpi_end");
    if (trace_plugin && trace_syms_fun.traces_mpi_end!=NULL){
        trace_syms_fun.traces_mpi_end();
    }
    return;
}

void traces_generic_event(int global_rank, int local_rank, int event, int value)
{
  debug("traces_generic_event");
  if (trace_plugin && trace_syms_fun.traces_generic_event != NULL){
    trace_syms_fun.traces_generic_event(global_rank, local_rank, event, value);
  }
  return;
}

#endif
