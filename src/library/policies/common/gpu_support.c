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

#define _GNU_SOURCE
#include <sched.h>
//#define SHOW_DEBUGS 1
#include <common/includes.h>
#include <common/config.h>
#include <common/system/symplug.h>
#include <library/policies/policy_ctx.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/metrics/metrics.h>

extern const int     polsyms_n;
extern const char     *polsyms_nam[];
state_t policy_gpu_load(settings_conf_t *app_settings,polsym_t *psyms)
{
    char *obj_path;
    char basic_gpu_path[SZ_PATH_INCOMPLETE];
    char pgpu_name[64];
    conf_install_t *data=&app_settings->installation;

    char *ins_path = ear_getenv(HACK_EARL_INSTALL_PATH);


    if (masters_info.my_master_rank >= 0){
#if USE_GPUS 
#ifdef GPU_OPT 
        verbose_master(2,"GPU optimization ON. Using policy base on %s", app_settings->policy_name);
#endif
        obj_path = ear_getenv(HACK_GPU_POWER_POLICY);
        if (obj_path != NULL) verbose_master(2,"%s defined %s",HACK_GPU_POWER_POLICY,obj_path);
#ifdef GPU_OPT
        xsnprintf(pgpu_name,sizeof(pgpu_name),"gpu_%s.so",app_settings->policy_name);
#else
        xsnprintf(pgpu_name,sizeof(pgpu_name),"gpu_monitoring.so");
#endif
        if (((obj_path==NULL) && (ins_path == NULL)) || (app_settings->user_type != AUTHORIZED)){
            xsnprintf(basic_gpu_path,sizeof(basic_gpu_path),"%s/policies/%s",data->dir_plug,pgpu_name);
            obj_path=basic_gpu_path;
        }else{
            if (obj_path == NULL){
                xsnprintf(basic_gpu_path,sizeof(basic_gpu_path),"%s/plugins/policies/%s",ins_path,pgpu_name);
                obj_path=basic_gpu_path;
            }
        }
        verbose_master(2,"Loading GPU policy %s",obj_path);
        if (symplug_open(obj_path, (void **)psyms, polsyms_nam, polsyms_n) != EAR_SUCCESS){
            verbose_master(2,"Error loading file %s",obj_path);
            return EAR_ERROR;
        }
#endif
    }
    return EAR_SUCCESS;
}

state_t policy_gpu_init_ctx(polctx_t *c)
{
#if 0
    uint policy_gpu_model;
    uint gpun;
#endif
    c->num_gpus = 0;

#if USE_GPUS
#if 0
    if (gpu_lib_load(no_ctx) != EAR_SUCCESS) {
        error("gpu_lib_load in init_power_policy");
        return EAR_ERROR;
    } else {
        if (gpu_lib_init(no_ctx) == EAR_SUCCESS){
            verbose_master(2,"gpu_lib_init success");
            gpu_lib_model(no_ctx, &policy_gpu_model);
            if (policy_gpu_model == API_DUMMY){
                debug("Setting policy num gpus to 0 because DUMMY");
                c->num_gpus = 0;
            }else{
                gpu_lib_count(&gpun);
                c->num_gpus= gpun;
            }
            debug("Num gpus detected in policy_load %u ",c->num_gpus);	

        } else {
            error("gpu_lib_init");
            return EAR_ERROR;
        }
    }
#else
    if (metrics_gpus_get(MET_GPU)->ok) {
        c->num_gpus = metrics_gpus_get(MET_GPU)->devs_count;
    }
#endif // NEW_GPU_API
#endif // USE_GPUS
    return EAR_SUCCESS;
}
