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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#include <stdlib.h>
#include <metrics/metrics.h>
#include <common/output/verbose.h>
#include <common/utils/overhead.h>

#define sa(f) \
	if (state_fail(f)) { \
		serror(#f " FAILED"); \
		exit(0); \
	}

#define printline() \
    dprintf(verb_channel, "-------------------------------------------------------------------\n");

static metrics_apis_t m;

state_t metrics_apis_init(topology_t *tp, uint eard, metrics_apis_t **m_in)
{
	// Load
	cpufreq_load(tp, eard);
	imcfreq_load(tp, eard);
     bwidth_load(tp, eard);
      cache_load(tp, eard);
      flops_load(tp, eard);
        cpi_load(tp, eard);
#if NOT_READY
	    gpu_load(    eard);
#endif
	// Get API
	cpufreq_get_api(&m.cpu.api);
	imcfreq_get_api(&m.imc.api);
     bwidth_get_api(&m.ram.api);
      cache_get_api(&m.cch.api);
      flops_get_api(&m.flp.api);
        cpi_get_api(&m.cpi.api);
#if NOT_READY
	    gpu_get_api(&m.gpu.api);
#endif
	// Working
	m.cpu.working = (m.cpu.api > API_DUMMY);
	m.imc.working = (m.imc.api > API_DUMMY);
    m.ram.working = (m.ram.api > API_DUMMY);
    m.cch.working = (m.cch.api > API_DUMMY);
    m.flp.working = (m.flp.api > API_DUMMY);
    m.cpi.working = (m.cpi.api > API_DUMMY);
#if NOT_READY
	m.gpu.working = (m.gpu.api > API_DUMMY);
#endif
	// Init
	sa(cpufreq_init(&m.cpu.c));
	sa(imcfreq_init(&m.imc.c));
    sa( bwidth_init(&m.ram.c));
    sa(  cache_init(&m.cch.c));
    sa(  flops_init(&m.flp.c));
    sa(    cpi_init(&m.cpi.c));
#if NOT_READY
	sa(    gpu_init(&m.gpu.c));
#endif
	// Count (cache, flops and cpi does not have yet)
	sa(cpufreq_count_devices(&m.cpu.c, &m.cpu.devs_count));
	sa(imcfreq_count_devices(&m.imc.c, &m.imc.devs_count));
    sa( bwidth_count_devices(&m.ram.c, &m.ram.devs_count));
    m.cch.devs_count = 1;
    m.flp.devs_count = 1;
    m.cpi.devs_count = 1;
#if NOT_READY
	sa(    gpu_count_devices(&m.gpu.c, &m.gpu.devs_count));
#endif

	if (m_in != NULL) {
		*m_in = &m;
	}

	return EAR_SUCCESS;
}

state_t metrics_apis_dispose()
{
	sa(cpufreq_dispose(&m.cpu.c));
	sa(imcfreq_dispose(&m.imc.c));
    sa( bwidth_dispose(&m.ram.c));
    sa(  cache_dispose(&m.cch.c));
    sa(  flops_dispose(&m.flp.c));
    sa(    cpi_dispose(&m.cpi.c));
#if NOT_READY
	sa(    gpu_dispose(&m.gpu.c));
#endif
	
	return EAR_SUCCESS;
}

state_t metrics_apis_print()
{
	apis_print(m.cpu.api, "MET_CPUFREQ: ");
	apis_print(m.imc.api, "MET_IMCFREQ: ");
    apis_print(m.ram.api, "MET_BWIDTH : ");
    apis_print(m.cch.api, "MET_CACHE  : ");
    apis_print(m.flp.api, "MET_FLOPS  : ");
    apis_print(m.cpi.api, "MET_CPI    : ");
#if NOT_READY
	apis_print(m.gpu.api, "MET_GPU    : ");
#endif
    printline();
	
	return EAR_SUCCESS;
}

state_t metrics_read_alloc(metrics_read_t *mr)
{
	sa(cpufreq_data_alloc(&mr->cpu, empty));
	sa(imcfreq_data_alloc(&mr->imc, empty));
    sa( bwidth_data_alloc(&mr->ram));
#if NOT_READY
	sa(    gpu_data_alloc(&mr->gpu));
#endif

	return EAR_SUCCESS;
}

state_t metrics_read_free(metrics_read_t *mr)
{
	return EAR_SUCCESS;
}

state_t metrics_read(metrics_read_t *mr)
{
	timestamp_get(&mr->time);
	
	sa(cpufreq_read(&m.cpu.c,  mr->cpu));
	sa(imcfreq_read(&m.imc.c,  mr->imc));
    sa( bwidth_read(&m.ram.c,  mr->ram));
    sa(  cache_read(&m.cch.c, &mr->cch));
    sa(  flops_read(&m.flp.c, &mr->flp));
    sa(    cpi_read(&m.cpi.c, &mr->cpi));
#if NOT_READY
	sa(    gpu_read(&m.gpu.c,  mr->gpu));
#endif

	return EAR_SUCCESS;
}

state_t metrics_read_diff(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD)
{
	mrD->time = timestamp_diffnow(&mr1->time, TIME_SECS);

	sa(cpufreq_read_diff(&m.cpu.c,  mr2->cpu,  mr1->cpu,        mrD->cpu_diff, &mrD->cpu_avrg));
	sa(imcfreq_read_diff(&m.imc.c,  mr2->imc,  mr1->imc,        mrD->imc_diff, &mrD->imc_avrg));
    sa( bwidth_read_diff(&m.ram.c,  mr2->ram,  mr1->ram, NULL, &mrD->ram_diff, &mrD->ram_avrg));
    sa(  cache_read_diff(&m.cch.c, &mr2->cch, &mr1->cch, &mrD->cch_diff, &mrD->cch_avrg));
    sa(  flops_read_diff(&m.flp.c, &mr2->flp, &mr1->flp, &mrD->flp_diff, &mrD->flp_avrg));
    sa(    cpi_read_diff(&m.cpi.c, &mr2->cpi, &mr1->cpi, &mrD->cpi_diff, &mrD->cpi_avrg));
#if NOT_READY
	sa(    gpu_read_diff(&m.gpu.c,  mr2->gpu,  mr1->gpu,  mrD->gpu_diff                ));
#endif

	return EAR_SUCCESS;
}

state_t metrics_read_copy(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD)
{
	mrD->time = timestamp_diffnow(&mr1->time, TIME_SECS);
	
	sa(cpufreq_read_copy(&m.cpu.c,  mr2->cpu,  mr1->cpu,        mrD->cpu_diff, &mrD->cpu_avrg));
	sa(imcfreq_read_copy(&m.imc.c,  mr2->imc,  mr1->imc,        mrD->imc_diff, &mrD->imc_avrg));
    sa( bwidth_read_copy(&m.ram.c,  mr2->ram,  mr1->ram, NULL, &mrD->ram_diff, &mrD->ram_avrg));
    sa(  cache_read_copy(&m.cch.c, &mr2->cch, &mr1->cch, &mrD->cch_diff, &mrD->cch_avrg));
    sa(  flops_read_copy(&m.flp.c, &mr2->flp, &mr1->flp, &mrD->flp_diff, &mrD->flp_avrg));
    sa(    cpi_read_copy(&m.cpi.c, &mr2->cpi, &mr1->cpi, &mrD->cpi_diff, &mrD->cpi_avrg));
#if NOT_READY
	sa(    gpu_read_copy(&m.gpu.c,  mr2->gpu,  mr1->gpu,  mrD->gpu_diff                ));
#endif

	return EAR_SUCCESS;
}


state_t metrics_diff_alloc(metrics_diff_t *md)
{
	sa(cpufreq_data_alloc(empty, &md->cpu_diff));
	sa(imcfreq_data_alloc(empty, &md->imc_diff));
#if NOT_READY
	sa(    gpu_data_alloc(&md->gpu_diff));
#endif

	return EAR_SUCCESS;
}

state_t metrics_diff_free(metrics_read_t *md)
{
	return EAR_SUCCESS;
}

state_t metrics_diff_print(metrics_diff_t *mrD)
{
	cpufreq_data_print( mrD->cpu_diff, mrD->cpu_avrg, verb_channel);
	imcfreq_data_print( mrD->imc_diff,&mrD->imc_avrg, verb_channel);
     bwidth_data_print( mrD->ram_diff, mrD->ram_avrg, verb_channel);
      cache_data_print(&mrD->cch_diff, mrD->cch_avrg, verb_channel);
      flops_data_print(&mrD->flp_diff, mrD->flp_avrg, verb_channel);
        cpi_data_print(&mrD->cpi_diff, mrD->cpi_avrg, verb_channel);
#if NOT_READY
	    gpu_data_print( mrD->gpu_diff,                verb_channel);
#endif

	return EAR_SUCCESS;
}

#if TEST
#include <stdio.h>
#include <unistd.h>
#include <common/system/monitor.h>
#include <management/management.h>
#include <daemon/local_api/eard_api_rpc.h>

static metrics_apis_t *mets;
static metrics_read_t mr1;
static metrics_read_t mr2;
static metrics_diff_t mrD;
static topology_t tp;
static int print;

state_t monitor_apis_init(void *whatever)
{
	sa(topology_init(&tp));

	//eards_connection();
	
	// Metrics
	sa(metrics_apis_init(&tp, EARD, &mets));
	sa(metrics_apis_print());
	
	sa(metrics_read_alloc(&mr1));
	sa(metrics_read_alloc(&mr2));
	sa(metrics_diff_alloc(&mrD));
	sa(metrics_read(&mr1));
	
	return EAR_SUCCESS;
}

void work()
{
    #define N 262144
    static ullong array1[N];
    static ullong array2[N];
    static ullong array3[N];

	int i;

	for (i = 2; i < N; ++i) {
		array1[i] = array2[i-1] + array3[i-2];
	}
}

state_t monitor_apis_update(void *whatever)
{
    work();

	if (print) {
		sa(metrics_read_copy(&mr2, &mr1, &mrD));
		sa(metrics_diff_print(&mrD));
        printline();
	}

	return EAR_SUCCESS;
} 

int init()
{
    print = 1;

	// Monitoring   
	sa(monitor_init());
	suscription_t *sus = suscription();
	sus->call_init     = monitor_apis_init; 
	sus->call_main     = monitor_apis_update;
	sus->time_relax    = 2000;
	sus->time_burst    = 2000;
	sa(sus->suscribe(sus));

	return 0;
}

int is(char *buf, char *string)
{
    return strcmp(buf, string) == 0;
}

int main(int argc, char *argv[])
{
	char cmnd[SZ_PATH];

	while(1)
    {
    printline();
    scanf("%s", cmnd);

    if (is(cmnd, "connect")) {
        eards_connection();
    } else if (is(cmnd, "disconnect")) {
        eards_disconnect();
    } else if (is(cmnd, "init")) {
        init();
    } else if (is(cmnd, "dispose")) {
        metrics_apis_dispose();
    } else if (is(cmnd, "oh")) {
       overhead_report(); 
    }
	outif:
	;
	}

	return 0;
}
#endif
