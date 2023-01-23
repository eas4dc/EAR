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
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/system/monitor.h>
#include <common/config/config_install.h>
#include <daemon/local_api/eard_api.h>
#include <management/test.h>

static manages_apis_t m;

#define sa(f) \
	if (state_fail(f)) { \
		serror(#f " FAILED"); \
		exit(0); \
	}
#define sn(f) \
    f;

#define frun(f) f
#if USE_GPUS
#define fgpu(f) frun(f)
#else
#define fgpu(f)
#endif

void management_init(topology_t *tp, int eard, manages_apis_t **man)
{
    frun(mgt_cpufreq_load(tp, eard      ));
    frun(mgt_cpuprio_load(tp, eard      ));
    frun(mgt_imcfreq_load(tp, eard, NULL));
    fgpu(    mgt_gpu_load(    eard      ));

    frun(mgt_cpufreq_get_api(&m.cpu.api));
    frun(mgt_cpuprio_get_api(&m.pri.api));
    frun(mgt_imcfreq_get_api(&m.imc.api));
    fgpu(    mgt_gpu_get_api(&m.gpu.api));

    frun(sa(mgt_cpufreq_init(no_ctx)));
    frun(sa(mgt_cpuprio_init(no_ctx)));
    frun(sa(mgt_imcfreq_init(no_ctx)));
    fgpu(sn(    mgt_gpu_init(no_ctx)));

    frun(sa(mgt_cpufreq_count_devices(no_ctx, &m.cpu.devs_count)));
    frun(sn(mgt_cpuprio_count_devices(no_ctx, &m.pri.devs_count)));
    frun(sa(mgt_imcfreq_count_devices(no_ctx, &m.imc.devs_count)));
    fgpu(sa(    mgt_gpu_count_devices(no_ctx, &m.gpu.devs_count)));

    //          | list1         | list2             | list3
    // -------- | ------------- | ----------------- | -----------------
    // cpufreq  | avail. ps.    | current ps.       | set index
    // cpuprio  | avail. clos   | current/set assoc | -
    // imcfreq  | avail. ps.    | current ps.       | set index
    // gpu      | devices       | current khz/watts | current khz/watts

    fgpu(sa(mgt_gpu_get_devices(no_ctx, (gpu_devs_t **) &m.gpu.list1, &m.gpu.list1_count)));
    // These current lists are not allocated
	frun(sa(mgt_cpufreq_get_available_list(no_ctx, (const pstate_t **) &m.cpu.list1, &m.cpu.list1_count)));
    frun(sa(mgt_imcfreq_get_available_list(no_ctx, (const pstate_t **) &m.imc.list1, &m.imc.list1_count)));
    // Current data allocation
    frun(sa(mgt_cpufreq_data_alloc((pstate_t  **) &m.cpu.list2, (uint **) &m.cpu.list3)));
    frun(sn(mgt_cpuprio_data_alloc((cpuprio_t **) &m.pri.list1, (uint **) &m.pri.list2)));
    frun(sa(mgt_imcfreq_data_alloc((pstate_t  **) &m.imc.list2, (uint **) &m.imc.list3)));
    fgpu(sa(    mgt_gpu_data_alloc((ulong     **) &m.gpu.list2                        )));
    fgpu(sa(    mgt_gpu_data_alloc((ulong     **) &m.gpu.list3                        )));
    // Counting data
    //frun(sn(mgt_cpufreq_data_count(&m.cpu.list1_count, &m.cpu.list2_count)));
    frun(sn(mgt_cpuprio_data_count(&m.pri.list1_count, &m.pri.list2_count)));
    //frun(sn(mgt_imcfreq_data_count(&m.imc.list1_count, &m.imc.list2_count)));
    // Apis to string
    frun(apis_tostr(m.cpu.api, m.cpu.api_str, 32));
    frun(apis_tostr(m.pri.api, m.pri.api_str, 32));
    frun(apis_tostr(m.imc.api, m.imc.api_str, 32));
    fgpu(apis_tostr(m.gpu.api, m.gpu.api_str, 32));
    // Granularities
    frun(sprintf(m.cpu.gran_str, "threads"));
    frun(sprintf(m.pri.gran_str, "threads"));
    frun(sprintf(m.imc.gran_str, "sockets"));
    fgpu(sprintf(m.gpu.gran_str, "gpus"));

    if (man != NULL) {    
        *man = &m;
    }
}

#if TEST
static topology_t tp;

static state_t monitor_apis_init(void *whatever)
{
    topology_init(&tp);

    management_init(&tp, EARD, NULL);

    return EAR_SUCCESS;
}

static state_t monitor_apis_update(void *whatever)
{
    // Printf clear
    system("clear");

    tprintf_init(fdout, STR_MODE_COL, "11 11 6 20");
    tprintf("API||Internal||#Devs||Granularity");
    tprintf("---||--------||-----||-----------");

    frun(tprintf("CPUFREQ||%s||%u||%s", m.cpu.api_str, m.cpu.devs_count, m.cpu.gran_str));
    frun(tprintf("CPUPRIO||%s||%u||%s", m.pri.api_str, m.pri.devs_count, m.pri.gran_str));
    frun(tprintf("IMCFREQ||%s||%u||%s", m.imc.api_str, m.imc.devs_count, m.imc.gran_str));
    fgpu(tprintf("GPU    ||%s||%u||%s", m.gpu.api_str, m.gpu.devs_count, m.gpu.gran_str));
    
    frun(printf("—— cpufreq ————————————————————————————————————————————————————————————————————\n"));
    frun(mgt_cpufreq_data_print(m.cpu.list1, m.cpu.list1_count, verb_channel));
    frun(printf("—— cpuprio ————————————————————————————————————————————————————————————————————\n"));
    frun(mgt_cpuprio_data_print(m.pri.list1, m.pri.list2, verb_channel));
    frun(printf("—— imcfreq ————————————————————————————————————————————————————————————————————\n"));
    frun(mgt_imcfreq_data_print(m.imc.list1, m.imc.list1_count, verb_channel));
    fgpu(printf("—— gpu ————————————————————————————————————————————————————————————————————————\n"));
    
    return EAR_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (atoi(argv[1])) {
            eards_connection();
        }
    }

	// Monitoring
	sa(monitor_init());
	suscription_t *sus = suscription();
	sus->call_init     = monitor_apis_init;
	sus->call_main     = monitor_apis_update;
	sus->time_relax    = 2000;
	sus->time_burst    = 2000;
	sa(sus->suscribe(sus));

    sleep(100);

    return 0;
}
#endif
