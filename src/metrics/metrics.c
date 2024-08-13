/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/output/debug.h>
#include <common/utils/stress.h>
#include <common/utils/special.h>
#include <common/utils/strtable.h>
#include <common/utils/strscreen.h>
#include <common/utils/overhead.h>
#include <common/config/config_install.h>
#include <metrics/metrics.h>

#define nodepow(f, ...) energy_ ##f (__VA_ARGS__)
#define  cpupow(f, ...) energy_cpu_ ##f (__VA_ARGS__)

static ehandler_t      e_ctx;
static topology_t     *self_tp;
// Screen
static strtable_t      st __attribute__ ((unused));
static strscreen_t     ss __attribute__ ((unused));
static int             id_cpu;
static int             id_imc;
static int             id_ram;
static int             id_tmp;
static int             id_pow;
static int             id_gpu;
static int             id_nod;
static int             id_log;
static int             id_api;
static int             id_tpo;

void metrics_init(metrics_info_t *m, topology_t *tp, char *nodepow_path, ullong mflags)
{
    self_tp = tp;

    nodepow(load,nodepow_path);
    cpufreq_load(tp, MFLAG_UNZIP(mflags, MET_CPUFREQ));
    imcfreq_load(tp, MFLAG_UNZIP(mflags, MET_IMCFREQ));
     cpupow(load,tp, MFLAG_UNZIP(mflags, MET_CPUPOW));
     bwidth_load(tp, MFLAG_UNZIP(mflags, MET_BWIDTH));
      cache_load(tp, MFLAG_UNZIP(mflags, MET_CACHE));
      flops_load(tp, MFLAG_UNZIP(mflags, MET_FLOPS));
       temp_load(tp                              );
        cpi_load(tp, MFLAG_UNZIP(mflags, MET_CPI));
        gpu_load(    MFLAG_UNZIP(mflags, MET_GPU));

    nodepow(initialization, &e_ctx);
    cpufreq_init(no_ctx);
    imcfreq_init(no_ctx);
     cpupow(init,no_ctx);
     bwidth_init(no_ctx);
      cache_init(no_ctx);
      flops_init(no_ctx);
       temp_init(no_ctx);
        cpi_init(no_ctx);
        gpu_init(no_ctx);

    //
    metrics_info_get(m);
}

__attribute__((unused)) static void metrics_init_screen(metrics_info_t *m, topology_t *tp)
{
    static char buffer[16384];
    // Screen initialization
    scprintf_init(&ss, 50, 180, -1, '#'); // 0 to 49, 0 to 179
    scprintf_divide(&ss, (int[]) {  1,   2}, (int[]) { 34,  39}, &id_cpu, "cpufreq"  );
    scprintf_divide(&ss, (int[]) { 36,   2}, (int[]) { 37,  39}, &id_imc, "imcfreq"  );
    scprintf_divide(&ss, (int[]) { 39,   2}, (int[]) { 40,  39}, &id_ram, "bandwidth");
    scprintf_divide(&ss, (int[]) { 42,   2}, (int[]) { 43,  39}, &id_tmp, "temperature");
    scprintf_divide(&ss, (int[]) { 45,   2}, (int[]) { 46,  39}, &id_pow, "cpupow"   );
    scprintf_divide(&ss, (int[]) {  1,  41}, (int[]) {  9, 118}, &id_gpu, "gpu"      );
    scprintf_divide(&ss, (int[]) { 11,  41}, (int[]) { 13,  78}, &id_nod, "nodepow"  );
    scprintf_divide(&ss, (int[]) {  1, 154}, (int[]) {  6, 177}, &id_log, "ear-logo" );
    scprintf_divide(&ss, (int[]) {  8, 124}, (int[]) { 18, 177}, &id_api, "api-table");
    scprintf_divide(&ss, (int[]) { 20, 154}, (int[]) { 35, 177}, &id_tpo, "topology" );

    topology_tostr(tp, buffer, sizeof(buffer));
    scsprintf(&ss, id_tpo, 0, 0, buffer);

    scsprintf(&ss, id_log, 0, 0, special_ear_logo());
    scsprintf(&ss, id_api, 0, 0, metrics_info_tostr(m, buffer));
}

void metrics_info_get(metrics_info_t *m)
{
    bwidth_get_info(&m->ram);
     cache_get_info(&m->cch);
     flops_get_info(&m->flp);
       gpu_get_info(&m->gpu);

    m->cpu.layer = "CPUFREQ";
    m->imc.layer = "IMCFREQ";
    m->pow.layer = "CPUPOW";
    m->tmp.layer = "TEMPERATURE";
    m->cpi.layer = "CPI";

    cpufreq_get_api(&m->cpu.api);
    imcfreq_get_api(&m->imc.api);
     cpupow(get_api,&m->pow.api);
       temp_get_api(&m->tmp.api);
        cpi_get_api(&m->cpi.api);

    cpufreq_count_devices(no_ctx, &m->cpu.devs_count);
    imcfreq_count_devices(no_ctx, &m->imc.devs_count);
     cpupow(count_devices,no_ctx, &m->pow.devs_count);
       temp_count_devices(no_ctx, &m->tmp.devs_count);

    m->cpu.scope = SCOPE_NODE;
    m->imc.scope = SCOPE_NODE;
    m->pow.scope = SCOPE_NODE;
    m->tmp.scope = SCOPE_NODE;
    m->cpi.scope = SCOPE_PROCESS;

    m->cpu.granularity = GRANULARITY_THREAD;
    m->imc.granularity = GRANULARITY_SOCKET;
    m->pow.granularity = GRANULARITY_SOCKET;
    m->tmp.granularity = GRANULARITY_SOCKET;
    m->cpi.granularity = GRANULARITY_PROCESS;

    apinfo_tostr(&m->cpu);
    apinfo_tostr(&m->imc);
    apinfo_tostr(&m->pow);
    apinfo_tostr(&m->ram);
    apinfo_tostr(&m->cch);
    apinfo_tostr(&m->flp);
    apinfo_tostr(&m->tmp);
    apinfo_tostr(&m->cpi);
    apinfo_tostr(&m->gpu);
}

char *metrics_info_tostr(metrics_info_t *m, char *buffer)
{
    static strtable_t sti;
    tprintf_init2(&sti, fdout, STR_MODE_COL, "13 9 8 10 20");
    tsprintf(buffer, &sti, 0, "E.API||I.API||#Devs||Scope||Granularity");
    tsprintf(buffer, &sti, 1, "-----||-----||-----||-----||-----------");
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->cpu.layer, m->cpu.api_str, m->cpu.devs_count, m->cpu.scope_str, m->cpu.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->imc.layer, m->imc.api_str, m->imc.devs_count, m->imc.scope_str, m->imc.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->ram.layer, m->ram.api_str, m->ram.devs_count, m->ram.scope_str, m->ram.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->cch.layer, m->cch.api_str, m->cch.devs_count, m->cch.scope_str, m->cch.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->flp.layer, m->flp.api_str, m->flp.devs_count, m->flp.scope_str, m->flp.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->tmp.layer, m->tmp.api_str, m->tmp.devs_count, m->tmp.scope_str, m->tmp.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->pow.layer, m->pow.api_str, m->pow.devs_count, m->pow.scope_str, m->pow.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->cpi.layer, m->cpi.api_str, m->cpi.devs_count, m->cpi.scope_str, m->cpi.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->gpu.layer, m->gpu.api_str, m->gpu.devs_count, m->gpu.scope_str, m->gpu.granularity_str);
    return buffer;
}

void metrics_info_print(metrics_info_t *m, int fd)
{
    static char buffer[4096];
    dprintf(fd, "%s", metrics_info_tostr(m, buffer));
}

// Auxiliars
void cpupow_tellmemore(ullong *diffs, ullong *pack, ullong *dram, double secs)
{
    double dpack = 0.0;
    double ddram = 0.0;
    int i;

    *pack = 0;
    *dram = 0;
    for (i = 0; i < self_tp->socket_count; ++i) {
        ddram += (double) diffs[i];
        dpack += (double) diffs[self_tp->socket_count+i];
    }
    dpack = cpupow(compute_power, dpack, secs);
    ddram = cpupow(compute_power, ddram, secs);
    dpack = dpack * secs;
    ddram = ddram * secs;
    debug("*pack = %0.3lf J (%0.3lf secs)", dpack, secs);
    debug("*dram = %0.3lf J (%0.3lf secs)", ddram, secs);
    *pack = (ullong) dpack;
    *dram = (ullong) ddram;
}

static char *cpupow_data_tostr(ullong *diffs, double secs, char *buffer, size_t length)
{
    double power = 0.0;
    double mean = 0.0;
    int i, j;

    buffer[0] = '\0';
    for (i = j = 0; i < self_tp->socket_count; ++i) {
        power = cpupow(compute_power, diffs[self_tp->socket_count+i], secs);
        j += sprintf(&buffer[j], "%0.1lf ", power);
        mean += power;
    }
    sprintf(&buffer[j], "!%0.1lf\n", mean);
    return buffer;
}

void nodepow_read(void *nod)
{
    nodepow(dc_read, &e_ctx, nod);
}

void nodepow_data_diff(void *nod2, void *nod1, ulong *nodD)
{
    nodepow(accumulated, &e_ctx, nodD, nod1, nod2);
}

void nodepow_data_copy(void *nodD, void *nodS)
{
    memcpy(nodD, nodS, 8192);
}

ATTR_UNUSED static void nodepow_read_copy(void *nod2, void *nod1, ulong *nodD)
{
    nodepow_read(nod2);
    nodepow_data_diff(nod2, nod1, nodD);
    nodepow_data_copy(nod1, nod2);
}

static char *nodepow_data_tostr(ulong avrg, char *buffer, size_t length)
{
    sprintf(buffer, "!%lu", avrg);
    return buffer;
}

void metrics_read(metrics_read_t *mr)
{
    debug("read %lu", time(NULL));
    timestamp_getreal(&mr->time);
    mr->samples += 1;

    nodepow_read(         mr->nod);
    cpufreq_read(no_ctx,  mr->cpu);
    imcfreq_read(no_ctx,  mr->imc);
     cpupow(read,no_ctx,  mr->pow);
     bwidth_read(no_ctx,  mr->ram);
      cache_read(no_ctx, &mr->cch);
      flops_read(no_ctx, &mr->flp);
       temp_read(no_ctx,  mr->tmp, NULL);
        cpi_read(no_ctx, &mr->cpi);
        gpu_read(no_ctx,  mr->gpu);
}

void metrics_read_copy(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD)
{
    metrics_read(mr2);
    metrics_data_diff(mr2, mr1, mrD);
    metrics_data_copy(mr1, mr2);
}

void metrics_data_alloc(metrics_read_t *mr1, metrics_read_t *mr2, metrics_diff_t *mrD)
{
    if (mr1) {
        mr1->nod = calloc(8192, sizeof(char));
        cpufreq_data_alloc(&mr1->cpu, empty);
        imcfreq_data_alloc(&mr1->imc, empty);
         cpupow(data_alloc, no_ctx, &mr1->pow, NULL);
         bwidth_data_alloc(&mr1->ram);
           temp_data_alloc(&mr1->tmp);
            gpu_data_alloc(&mr1->gpu);
    }
    if (mr2) {
        mr2->nod = calloc(8192, sizeof(char));
        cpufreq_data_alloc(&mr2->cpu, empty);
        imcfreq_data_alloc(&mr2->imc, empty);
         cpupow(data_alloc, no_ctx, &mr2->pow, NULL);
         bwidth_data_alloc(&mr2->ram);
           temp_data_alloc(&mr2->tmp);
            gpu_data_alloc(&mr2->gpu);
    }
    if (mrD) {
        cpufreq_data_alloc(empty, &mrD->cpu_diff);
        imcfreq_data_alloc(empty, &mrD->imc_diff);
         cpupow(data_alloc, no_ctx, &mrD->pow_diff, NULL);
           temp_data_alloc(&mrD->tmp_diff);
            gpu_data_alloc(&mrD->gpu_diff);
    }
}

void metrics_data_diff(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD)
{
    mrD->time    = timestamp_fdiff(&mr2->time, &mr1->time, TIME_SECS, TIME_MSECS);
    mrD->samples = mr2->samples - mr1->samples; 
    debug("time    = %lf s", mrD->time);
    debug("samples = %llu", mrD->samples);

    nodepow_data_diff( mr2->nod,  mr1->nod,                       &mrD->nod_avrg);
    cpufreq_data_diff( mr2->cpu,  mr1->cpu,        mrD->cpu_diff, &mrD->cpu_avrg);
    imcfreq_data_diff( mr2->imc,  mr1->imc,        mrD->imc_diff, &mrD->imc_avrg);
     cpupow(data_diff, no_ctx, mr1->pow, mr2->pow, mrD->pow_diff);
     bwidth_data_diff( mr2->ram,  mr1->ram, NULL, &mrD->ram_diff, &mrD->ram_avrg);
      cache_data_diff(&mr2->cch, &mr1->cch,       &mrD->cch_diff, &mrD->cch_avrg);
      flops_data_diff(&mr2->flp, &mr1->flp,       &mrD->flp_diff, &mrD->flp_avrg);
       temp_data_diff( mr2->tmp,  mr1->tmp,        mrD->tmp_diff, &mrD->tmp_avrg);
        cpi_data_diff(&mr2->cpi, &mr1->cpi,       &mrD->cpi_diff, &mrD->cpi_avrg);
        gpu_data_diff( mr2->gpu,  mr1->gpu,        mrD->gpu_diff                );

    cpupow_tellmemore(mrD->pow_diff, &mrD->pow_pack, &mrD->pow_dram, mrD->time);
}

void metrics_data_copy(metrics_read_t *mrD, metrics_read_t *mrS)
{
    mrD->time    = mrS->time;
    mrD->samples = mrS->samples;

    nodepow_data_copy( mrD->nod,  mrS->nod);
    cpufreq_data_copy( mrD->cpu,  mrS->cpu);
    imcfreq_data_copy( mrD->imc,  mrS->imc);
     bwidth_data_copy( mrD->ram,  mrS->ram);
     cpupow(data_copy, no_ctx, mrD->pow, mrS->pow);
      cache_data_copy(&mrD->cch, &mrS->cch);
      flops_data_copy(&mrD->flp, &mrS->flp);
       temp_data_copy( mrD->tmp,  mrS->tmp);
        cpi_data_copy(&mrD->cpi, &mrS->cpi);
        gpu_data_copy( mrD->gpu,  mrS->gpu);
}

void metrics_data_null(metrics_diff_t *mrD)
{
    
}

void metrics_data_print(metrics_diff_t *mrD, int fd)
{
    char *string = metrics_data_tostr(mrD);
    dprintf(fd, "%s", string); 
}

char *metrics_data_tostr(metrics_diff_t *mrD)
{
    static char b[16384];
    scsprintf(&ss, id_cpu, 0, 1, cpufreq_data_tostr(mrD->cpu_diff,  mrD->cpu_avrg, b, sizeof(b)));
    scsprintf(&ss, id_imc, 0, 1, imcfreq_data_tostr(mrD->imc_diff, &mrD->imc_avrg, b, sizeof(b)));
    scsprintf(&ss, id_pow, 0, 1,  cpupow_data_tostr(mrD->pow_diff,  mrD->time    , b, sizeof(b)));
    scsprintf(&ss, id_ram, 0, 1,  bwidth_data_tostr(mrD->ram_diff,  mrD->ram_avrg, b, sizeof(b)));
    scsprintf(&ss, id_tmp, 0, 1,    temp_data_tostr(mrD->tmp_diff,  mrD->tmp_avrg, b, sizeof(b)));
    scsprintf(&ss, id_gpu, 0, 1,     gpu_data_tostr(mrD->gpu_diff,                 b, sizeof(b)));
    scsprintf(&ss, id_nod, 0, 1, nodepow_data_tostr(mrD->nod_avrg,                 b, sizeof(b)));
    return scprintf(&ss);
}

#if TEST
#include <stdio.h>
#include <stdlib.h>
#include <common/system/monitor.h>
#include <daemon/local_api/eard_api.h>

static topology_t     tp;
static metrics_info_t m;
static metrics_read_t mr1;
static metrics_read_t mr2;
static metrics_diff_t mrD;

static state_t metrics_apis_init(void *whatever)
{
    topology_init(&tp);
    metrics_init(&m, &tp, NULL, EARD);
    metrics_init_screen(&m, &tp);
    metrics_data_alloc(&mr1, &mr2, &mrD);
    metrics_read(&mr1);
    return EAR_SUCCESS;
}

static state_t metrics_apis_update(void *whatever)
{
    metrics_read_copy(&mr2, &mr1, &mrD);
    metrics_data_print(&mrD, 0);
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
