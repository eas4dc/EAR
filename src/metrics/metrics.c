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

#include <stdio.h>
#include <stdlib.h>
#include <common/utils/stress.h>
#include <common/utils/overhead.h>
#include <common/output/verbose.h>
#include <common/config/config_install.h>
#include <metrics/metrics.h>

void metrics_init(metrics_info_t *m, topology_t *tp, int eard)
{
    cpufreq_load(tp, eard);
    imcfreq_load(tp, eard);
     bwidth_load(tp, eard);
      cache_load(tp, eard);
      flops_load(tp, eard);
       temp_load(tp      );
        cpi_load(tp, eard);
        gpu_load(    eard);

    cpufreq_init(no_ctx);
    imcfreq_init(no_ctx);
     bwidth_init(no_ctx);
      cache_init(no_ctx);
      flops_init(no_ctx);
       temp_init(no_ctx);
        cpi_init(no_ctx);
        gpu_init(no_ctx);

    //
    cache_get_info(&m->cch);

    m->cpu.layer = "CPUFREQ";
    m->imc.layer = "IMCFREQ";
    m->ram.layer = "BANDWIDTH";
    m->flp.layer = "FLOPS";
    m->tmp.layer = "TEMPERATURE";
    m->cpi.layer = "CPI";
    m->gpu.layer = "GPU";

    cpufreq_get_api(&m->cpu.api);
    imcfreq_get_api(&m->imc.api);
     bwidth_get_api(&m->ram.api);
      flops_get_api(&m->flp.api);
       temp_get_api(&m->tmp.api);
        cpi_get_api(&m->cpi.api);
        gpu_get_api(&m->gpu.api);

    cpufreq_count_devices(no_ctx, &m->cpu.devs_count);
    imcfreq_count_devices(no_ctx, &m->imc.devs_count);
     bwidth_count_devices(no_ctx, &m->ram.devs_count);
       temp_count_devices(no_ctx, &m->tmp.devs_count);
        gpu_count_devices(no_ctx, &m->gpu.devs_count);

    m->cpu.scope = SCOPE_NODE;
    m->imc.scope = SCOPE_NODE;
    m->ram.scope = SCOPE_NODE;
    m->flp.scope = SCOPE_PROCESS;
    m->tmp.scope = SCOPE_NODE;
    m->cpi.scope = SCOPE_PROCESS;
    m->gpu.scope = SCOPE_NODE;

    m->cpu.granularity = GRANULARITY_THREAD;
    m->imc.granularity = GRANULARITY_SOCKET;
    m->ram.granularity = GRANULARITY_IMC;
    m->flp.granularity = GRANULARITY_PROCESS;
    m->tmp.granularity = GRANULARITY_SOCKET;
    m->cpi.granularity = GRANULARITY_PROCESS;
    m->gpu.granularity = GRANULARITY_PERIPHERAL;

    apinfo_tostr(&m->cpu);
    apinfo_tostr(&m->imc);
    apinfo_tostr(&m->ram);
    apinfo_tostr(&m->cch);
    apinfo_tostr(&m->flp);
    apinfo_tostr(&m->tmp);
    apinfo_tostr(&m->cpi);
    apinfo_tostr(&m->gpu);
}

void metrics_read(metrics_read_t *mr)
{
    timestamp_get(&mr->time);

    cpufreq_read(no_ctx,  mr->cpu);
    imcfreq_read(no_ctx,  mr->imc);
     bwidth_read(no_ctx,  mr->ram);
      cache_read(no_ctx, &mr->cch);
      flops_read(no_ctx, &mr->flp);
        cpi_read(no_ctx, &mr->cpi);
        gpu_read(no_ctx,  mr->gpu);
}

void metrics_read_copy(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD)
{
	mrD->time = timestamp_diffnow(&mr1->time, TIME_SECS);

    cpufreq_read_copy(no_ctx,  mr2->cpu,  mr1->cpu,        mrD->cpu_diff, &mrD->cpu_avrg);
    imcfreq_read_copy(no_ctx,  mr2->imc,  mr1->imc,        mrD->imc_diff, &mrD->imc_avrg);
     bwidth_read_copy(no_ctx,  mr2->ram,  mr1->ram, NULL, &mrD->ram_diff, &mrD->ram_avrg);
      cache_read_copy(no_ctx, &mr2->cch, &mr1->cch,       &mrD->cch_diff, &mrD->cch_avrg);
      flops_read_copy(no_ctx, &mr2->flp, &mr1->flp,       &mrD->flp_diff, &mrD->flp_avrg);
       temp_read     (no_ctx,                              mrD->tmp_diff, &mrD->tmp_avrg);
        cpi_read_copy(no_ctx, &mr2->cpi, &mr1->cpi,       &mrD->cpi_diff, &mrD->cpi_avrg);
        gpu_read_copy(no_ctx,  mr2->gpu,  mr1->gpu,        mrD->gpu_diff                );
}

void metrics_data_alloc(metrics_read_t *mr1, metrics_read_t *mr2, metrics_diff_t *mrD)
{
    // Read 1
    cpufreq_data_alloc(&mr1->cpu, empty);
    imcfreq_data_alloc(&mr1->imc, empty);
     bwidth_data_alloc(&mr1->ram);
        gpu_data_alloc(&mr1->gpu);
    // Read 2
    cpufreq_data_alloc(&mr2->cpu, empty);
    imcfreq_data_alloc(&mr2->imc, empty);
     bwidth_data_alloc(&mr2->ram);
        gpu_data_alloc(&mr2->gpu);
    // Diff
    cpufreq_data_alloc(empty, &mrD->cpu_diff);
    imcfreq_data_alloc(empty, &mrD->imc_diff);
       temp_data_alloc(&mrD->tmp_diff);
        gpu_data_alloc(&mrD->gpu_diff);
}

#if TEST
#include <stdio.h>
#include <stdlib.h>
#include <common/utils/stress.h>
#include <common/utils/special.h>
#include <common/utils/overhead.h>
#include <common/utils/strtable.h>
#include <common/utils/strscreen.h>
#include <common/output/verbose.h>
#include <common/system/monitor.h>
#include <daemon/local_api/eard_api.h>
#include <metrics/metrics.h>

static topology_t     tp;
static strtable_t     st;
static strscreen_t    ss;
static metrics_info_t m;
static metrics_read_t mr1;
static metrics_read_t mr2;
static metrics_diff_t mrD;
static int            idc;
static int            idi;
static int            idb;
static int            idt;
static int            idl;
static int            ida;
static int            idh;

static state_t metrics_apis_init(void *whatever)
{
    char buffer[16384];

    topology_init(&tp);

    metrics_init(&m, &tp, EARD);
    metrics_data_alloc(&mr1, &mr2, &mrD);
    metrics_read(&mr1);

    wprintf_init(&ss, 50, 180, 0, '#'); // 0 to 49, 0 to 179
    wprintf_divide(&ss, (int[]) {  1,   2}, (int[]) { 34,  39}, &idc, "cpufreq"  );
    wprintf_divide(&ss, (int[]) { 36,   2}, (int[]) { 37,  39}, &idi, "imcfreq");
    wprintf_divide(&ss, (int[]) { 39,   2}, (int[]) { 40,  39}, &idb, "bandwidth");
    wprintf_divide(&ss, (int[]) { 42,   2}, (int[]) { 43,  39}, &idt, "temperature");
    wprintf_divide(&ss, (int[]) {  1, 154}, (int[]) {  6, 177}, &idl, "ear-logo" );
    wprintf_divide(&ss, (int[]) {  8, 124}, (int[]) { 17, 177}, &ida, "api-table");
    wprintf_divide(&ss, (int[]) { 19, 154}, (int[]) { 34, 177}, &idh, "topology" );

    topology_tostr(&tp, buffer, sizeof(buffer));
    wsprintf(&ss, idh, 0, 0, buffer);

    tprintf_init2(&st, fdout, STR_MODE_COL, "13 9 8 10 20");
    tsprintf(buffer, &st, 0, "E.API||I.API||#Devs||Scope||Granularity");
    tsprintf(buffer, &st, 1, "-----||-----||-----||-----||-----------");
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.cpu.layer, m.cpu.api_str, m.cpu.devs_count, m.cpu.scope_str, m.cpu.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.imc.layer, m.imc.api_str, m.imc.devs_count, m.imc.scope_str, m.imc.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.ram.layer, m.ram.api_str, m.ram.devs_count, m.ram.scope_str, m.ram.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.cch.layer, m.cch.api_str, m.cch.devs_count, m.cch.scope_str, m.cch.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.flp.layer, m.flp.api_str, m.flp.devs_count, m.flp.scope_str, m.flp.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.tmp.layer, m.tmp.api_str, m.tmp.devs_count, m.tmp.scope_str, m.tmp.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.cpi.layer, m.cpi.api_str, m.cpi.devs_count, m.cpi.scope_str, m.cpi.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.gpu.layer, m.gpu.api_str, m.gpu.devs_count, m.gpu.scope_str, m.gpu.granularity_str);

    wsprintf(&ss, idl, 0, 0, special_ear_logo());
    wsprintf(&ss, ida, 0, 0, buffer);

    wprintf(&ss);

    return EAR_SUCCESS;
}

static state_t metrics_apis_update(void *whatever)
{
    char b[16384];

    metrics_read_copy(&mr2, &mr1, &mrD);

    wsprintf(&ss, idc, 0, 1, cpufreq_data_tostr(mrD.cpu_diff,  mrD.cpu_avrg, b, sizeof(b)));
    wsprintf(&ss, idi, 0, 1, imcfreq_data_tostr(mrD.imc_diff, &mrD.imc_avrg, b, sizeof(b)));
    wsprintf(&ss, idb, 0, 1,  bwidth_data_tostr(mrD.ram_diff,  mrD.ram_avrg, b, sizeof(b)));
    wsprintf(&ss, idt, 0, 1,    temp_data_tostr(mrD.tmp_diff,  mrD.tmp_avrg, b, sizeof(b)));
    wprintf(&ss);

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
	monitor_init();
	suscription_t *sus = suscription();
	sus->call_init     = metrics_apis_init;
	sus->call_main     = metrics_apis_update;
	sus->time_relax    = 2000;
	sus->time_burst    = 2000;
	sus->suscribe(sus);
    sleep(100);

    return 0;
}
#endif