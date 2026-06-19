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
// #define SHOW_DEBUGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/output/debug.h>
#include <common/utils/stress.h>
#include <common/utils/special.h>
#include <common/utils/overhead.h>
#include <common/utils/strtable.h>
#include <common/utils/strscreen.h>
#include <common/config/config_install.h>
#include <metrics/metrics.h>

#define nodepow(f, ...) energy_##f(__VA_ARGS__)
#define cpupow(f, ...)  energy_cpu_##f(__VA_ARGS__)

static ehandler_t e_ctx;
static topology_t *self_tp;
// Screen
static strtable_t st __attribute__((unused));
static strscreen_t ss __attribute__((unused));
static int id_bwidth;
static int id_cache;
static int id_cpi;
static int id_cpufreq;
static int id_cpupow;
static int id_flops;
static int id_gpu;
static int id_imcfreq;
static int id_io;
static int id_nodepow;
static int id_proc;
static int id_temp;
static int id_log;
static int id_api;
static int id_tpo;

void metrics_load(metrics_info_t *m, topology_t *tp, char *nodepow_path, uint *options)
{
    uint default_options[MET_OPT_MAX] = {0};
    self_tp = tp;

    if (options == NULL) {
        options = default_options;
    }
    nodepow(load, nodepow_path);
    cpufreq_load(tp, options[MET_OPT_CPUFREQ]);
    imcfreq_load(tp, options[MET_OPT_IMCFREQ]);
     cpupow(load,tp, options[MET_OPT_CPUPOW] );
     bwidth_load(tp, options[MET_OPT_BWIDTH] );
      cache_load(tp, options[MET_OPT_CACHE]  );
      flops_load(tp, options[MET_OPT_FLOPS]  );
       temp_load(tp, options[MET_OPT_TEMP]   );
        cpi_load(tp, options[MET_OPT_CPI]    );
        gpu_load(    options[MET_OPT_GPU]    );
         io_load(tp, options[MET_OPT_IO]     );
       proc_load(tp, options[MET_OPT_PROC]   );

    nodepow(init, &e_ctx);
     cpupow(init, no_ctx);

    metrics_info_get(m);
}

void metrics_update(uint option, void *value)
{
    cache_update(option, value);
      cpi_update(option, value);
    flops_update(option, value);
      gpu_update(option, value);
       io_update(option, value);
     proc_update(option, value);
}

__attribute__((unused)) static void metrics_init_screen(metrics_info_t *m, topology_t *tp)
{
    static char buffer[16384];
    // Screen initialization
    scprintf_init(&ss, 50, 180, -1, '#'); // 0 to 49, 0 to 179
    scprintf_divide(&ss, (int[]) {39,   2}, (int[]) {40,  39}, &id_bwidth , "bandwidth"  );
    scprintf_divide(&ss, (int[]) {29,  41}, (int[]) {34, 118}, &id_cache  , "cache"      );
    scprintf_divide(&ss, (int[]) {22,  41}, (int[]) {27, 118}, &id_cpi    , "cpi"        );
    scprintf_divide(&ss, (int[]) { 1,   2}, (int[]) {34,  39}, &id_cpufreq, "cpufreq"    );
    scprintf_divide(&ss, (int[]) {45,   2}, (int[]) {46,  39}, &id_cpupow , "cpupow"     );
    scprintf_divide(&ss, (int[]) {36,  41}, (int[]) {41, 118}, &id_flops  , "flops"      );
    scprintf_divide(&ss, (int[]) { 1,  41}, (int[]) { 9, 118}, &id_gpu    , "gpu"        );
    scprintf_divide(&ss, (int[]) {36,   2}, (int[]) {37,  39}, &id_imcfreq, "imcfreq"    );
    scprintf_divide(&ss, (int[]) {15,  41}, (int[]) {20, 118}, &id_io     , "io"         );
    scprintf_divide(&ss, (int[]) {11,  41}, (int[]) {13, 118}, &id_nodepow, "nodepow"    );
    scprintf_divide(&ss, (int[]) {43,  41}, (int[]) {48, 118}, &id_proc   , "proc"       );
    scprintf_divide(&ss, (int[]) {42,   2}, (int[]) {43,  39}, &id_temp   , "temperature");
    scprintf_divide(&ss, (int[]) { 1, 154}, (int[]) { 6, 177}, &id_log    , "ear-logo"   );
    scprintf_divide(&ss, (int[]) { 8, 124}, (int[]) {20, 177}, &id_api    , "api-table"  );
    scprintf_divide(&ss, (int[]) {22, 154}, (int[]) {36, 177}, &id_tpo    , "topology"   );
    topology_tostr(tp, buffer, sizeof(buffer));
    scsprintf(&ss, id_tpo, 0, 0, buffer);
    scsprintf(&ss, id_log, 0, 0, special_ear_logo());
    scsprintf(&ss, id_api, 0, 0, metrics_info_tostr(m, buffer));
}

void metrics_info_get(metrics_info_t *m)
{
    cpufreq_get_info(&m->cpufreq);
    imcfreq_get_info(&m->imcfreq);
     bwidth_get_info(&m->bwidth );
      cache_get_info(&m->cache  );
      flops_get_info(&m->flops  );
       temp_get_info(&m->temp   );
        cpi_get_info(&m->cpi    );
        gpu_get_info(&m->gpu    );
         io_get_info(&m->io     );
       proc_get_info(&m->proc   );

    m->cpupow.layer = "CPUPOW";
    cpupow(get_api, &m->cpupow.api);
    cpupow(count_devices, no_ctx, &m->cpupow.devs_count);
    m->cpupow.scope       = SCOPE_NODE;
    m->cpupow.granularity = GRANULARITY_SOCKET;

    apinfo_tostr(&m->cpufreq);
    apinfo_tostr(&m->imcfreq);
    apinfo_tostr(&m->cpupow );
    apinfo_tostr(&m->bwidth );
    apinfo_tostr(&m->cache  );
    apinfo_tostr(&m->flops  );
    apinfo_tostr(&m->temp   );
    apinfo_tostr(&m->cpi    );
    apinfo_tostr(&m->gpu    );
    apinfo_tostr(&m->io     );
    apinfo_tostr(&m->proc   );
}

char *metrics_info_tostr(metrics_info_t *m, char *buffer)
{
    static strtable_t sti;
    tprintf_init2(&sti, fdout, STR_MODE_COL, "13 9 8 10 20");

    tsprintf(buffer, &sti, 0, "E.API||I.API||#Devs||Scope||Granularity");
    tsprintf(buffer, &sti, 1, "-----||-----||-----||-----||-----------");
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->bwidth.layer , m->bwidth.api_str , m->bwidth.devs_count , m->bwidth.scope_str , m->bwidth.granularity_str );
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->cache.layer  , m->cache.api_str  , m->cache.devs_count  , m->cache.scope_str  , m->cache.granularity_str  );
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->cpi.layer    , m->cpi.api_str    , m->cpi.devs_count    , m->cpi.scope_str    , m->cpi.granularity_str    );
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->cpufreq.layer, m->cpufreq.api_str, m->cpufreq.devs_count, m->cpufreq.scope_str, m->cpufreq.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->cpupow.layer , m->cpupow.api_str , m->cpupow.devs_count , m->cpupow.scope_str , m->cpupow.granularity_str );
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->flops.layer  , m->flops.api_str  , m->flops.devs_count  , m->flops.scope_str  , m->flops.granularity_str  );
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->gpu.layer    , m->gpu.api_str    , m->gpu.devs_count    , m->gpu.scope_str    , m->gpu.granularity_str    );
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->imcfreq.layer, m->imcfreq.api_str, m->imcfreq.devs_count, m->imcfreq.scope_str, m->imcfreq.granularity_str);
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->io.layer     , m->io.api_str     , m->io.devs_count     , m->io.scope_str     , m->io.granularity_str     );
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->proc.layer   , m->proc.api_str   , m->proc.devs_count   , m->proc.scope_str   , m->proc.granularity_str   );
    tsprintf(buffer, &sti, 1, "%s||%s||%u||%s||%s", m->temp.layer   , m->temp.api_str   , m->temp.devs_count   , m->temp.scope_str   , m->temp.granularity_str   );
    return buffer;
}

void metrics_info_print(metrics_info_t *m, int fd)
{
    static char buffer[4096];
    dprintf(fd, "%s", metrics_info_tostr(m, buffer));
}

// Auxiliars
void cpupow_tellmemore(ullong *diffs, ullong *pack, ullong *dram, double secs, ullong *tot_pack, ullong *tot_dram)
{
    if (!diffs || !pack || !dram || !tot_pack || !tot_dram) {
        return;
    }
    *tot_pack = 0.0;
    *tot_dram = 0.0;

    for (int i = 0; i < self_tp->socket_count; ++i) {
        double dram_pow = cpupow(compute_power, (double) diffs[i], secs);
        double pack_pow = cpupow(compute_power, (double) diffs[self_tp->socket_count + i], secs);
        *tot_dram += (ullong) dram_pow;
        *tot_pack += (ullong) pack_pow;
        debug("dram[%d] = %0.3lf W (%0.3lf J / %0.3lf secs)", i, dram_pow, (double) diffs[i], secs);
        debug("pack[%d] = %0.3lf W (%0.3lf J / %0.3lf secs)", i, pack_pow, (double) diffs[self_tp->socket_count + i], secs);
        pack[i] = (ullong) pack_pow;
        dram[i] = (ullong) dram_pow;
    }
    debug("tot_pack = %llu W", *tot_pack);
    debug("tot_dram = %llu W", *tot_dram);
}

static char *cpupow_data_tostr(ullong *diffs, double secs, char *buffer, size_t length)
{
    double power = 0.0;
    double mean  = 0.0;
    int i, j;

    buffer[0] = '\0';
    for (i = j = 0; i < self_tp->socket_count && (size_t) j < length; ++i) {
        power = cpupow(compute_power, diffs[self_tp->socket_count + i], secs);
        j    += sprintf(&buffer[j], "%0.1lf ", power);
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
    snprintf(buffer, length, "!%lu", avrg);
    return buffer;
}

void metrics_read(metrics_read_t *mr)
{
    debug("read %lu", time(NULL));
    timestamp_getreal(&mr->time);
    mr->samples += 1;

    nodepow_read(mr->nodepow );
    cpufreq_read(mr->cpufreq );
    imcfreq_read(mr->imcfreq );
     cpupow(read,no_ctx, mr->cpupow);
     bwidth_read(mr->bwidth  );
      cache_read(mr->cache   );
      flops_read(mr->flops   );
       temp_read(mr->temp, NULL);
        cpi_read(mr->cpi     );
        gpu_read(mr->gpu     );
         io_read(mr->io      );
       proc_read(mr->proc    );
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
        mr1->nodepow = calloc(8192, sizeof(char));
        cpufreq_data_alloc(&mr1->cpufreq, empty);
        imcfreq_data_alloc(&mr1->imcfreq, empty);
         cpupow(data_alloc, no_ctx, &mr1->cpupow, NULL);
         bwidth_data_alloc(&mr1->bwidth );
          cache_data_alloc(&mr1->cache  );
          flops_data_alloc(&mr1->flops  );
           temp_data_alloc(&mr1->temp   );
            cpi_data_alloc(&mr1->cpi    );
            gpu_data_alloc(&mr1->gpu    );
             io_data_alloc(&mr1->io     );
           proc_data_alloc(&mr1->proc   );
    }
    if (mr2) {
        mr2->nodepow = calloc(8192, sizeof(char));
        cpufreq_data_alloc(&mr2->cpufreq, empty);
        imcfreq_data_alloc(&mr2->imcfreq, empty);
         cpupow(data_alloc, no_ctx, &mr2->cpupow, NULL);
         bwidth_data_alloc(&mr2->bwidth );
          cache_data_alloc(&mr2->cache  );
          flops_data_alloc(&mr2->flops  );
           temp_data_alloc(&mr2->temp   );
            cpi_data_alloc(&mr2->cpi    );
            gpu_data_alloc(&mr2->gpu    );
             io_data_alloc(&mr2->io     );
           proc_data_alloc(&mr2->proc   );
    }
    if (mrD) {
        cpufreq_data_alloc(empty, &mrD->cpufreq_diff);
        imcfreq_data_alloc(empty, &mrD->imcfreq_diff);
         cpupow(data_alloc, no_ctx, &mrD->cpupow_diff, NULL);
         cpupow(data_alloc, no_ctx, &mrD->cpupow_dram, NULL);
         cpupow(data_alloc, no_ctx, &mrD->cpupow_pack, NULL);
          cache_data_alloc(&mrD->cache_diff);
          flops_data_alloc(&mrD->flops_diff);
           temp_data_alloc(&mrD->temp_diff );
            cpi_data_alloc(&mrD->cpi_diff  );
            gpu_data_alloc(&mrD->gpu_diff  );
             io_data_alloc(&mrD->io_diff   );
           proc_data_alloc(&mrD->proc_diff );
    }
}

void metrics_data_diff(metrics_read_t *mr2, metrics_read_t *mr1, metrics_diff_t *mrD)
{
    mrD->time    = timestamp_fdiff(&mr2->time, &mr1->time, TIME_SECS, TIME_MSECS);
    mrD->samples = mr2->samples - mr1->samples;
    debug("time    = %lf s", mrD->time);
    debug("samples = %llu", mrD->samples);

     bwidth_data_diff(mr2->bwidth , mr1->bwidth , NULL, &mrD->bwidth_diff, &mrD->bwidth_avrg);
      cache_data_diff(mr2->cache  , mr1->cache  , mrD->cache_diff  , &mrD->cache_avrg  );
        cpi_data_diff(mr2->cpi    , mr1->cpi    , mrD->cpi_diff    , &mrD->cpi_avrg    );
    cpufreq_data_diff(mr2->cpufreq, mr1->cpufreq, mrD->cpufreq_diff, &mrD->cpufreq_avrg);
     cpupow(data_diff, no_ctx, mr1->cpupow, mr2->cpupow, mrD->cpupow_diff);
      flops_data_diff(mr2->flops  , mr1->flops  , mrD->flops_diff  , &mrD->flops_avrg  );
        gpu_data_diff(mr2->gpu    , mr1->gpu    , mrD->gpu_diff                        );
    imcfreq_data_diff(mr2->imcfreq, mr1->imcfreq, mrD->imcfreq_diff, &mrD->imcfreq_avrg);
         io_data_diff(mr2->io     , mr1->io     , mrD->io_diff     , &mrD->io_avrg     );
    nodepow_data_diff(mr2->nodepow, mr1->nodepow, &mrD->nodepow_avrg);
       proc_data_diff(mr2->proc   , mr1->proc   , mrD->proc_diff                       );
       temp_data_diff(mr2->temp   , mr1->temp   , mrD->temp_diff   , &mrD->temp_avrg   );

    cpupow_tellmemore(mrD->cpupow_diff, mrD->cpupow_pack, mrD->cpupow_dram, mrD->time, &mrD->cpupow_tot_dram, &mrD->cpupow_tot_pack);
}

void metrics_data_copy(metrics_read_t *mrD, metrics_read_t *mrS)
{
    mrD->time    = mrS->time;
    mrD->samples = mrS->samples;

     bwidth_data_copy(mrD->bwidth , mrS->bwidth );
      cache_data_copy(mrD->cache  , mrS->cache  );
        cpi_data_copy(mrD->cpi    , mrS->cpi    );
    cpufreq_data_copy(mrD->cpufreq, mrS->cpufreq);
     cpupow(data_copy, no_ctx, mrD->cpupow, mrS->cpupow);
      flops_data_copy(mrD->flops  , mrS->flops  );
        gpu_data_copy(mrD->gpu    , mrS->gpu    );
    imcfreq_data_copy(mrD->imcfreq, mrS->imcfreq);
         io_data_copy(mrD->io     , mrS->io     );
    nodepow_data_copy(mrD->nodepow, mrS->nodepow);
       proc_data_copy(mrD->proc   , mrS->proc   );
       temp_data_copy(mrD->temp   , mrS->temp   );
}

void metrics_data_print(metrics_diff_t *mrD, int fd)
{
    char *string = metrics_data_tostr(mrD);
    dprintf(fd, "%s", string);
}

char *metrics_data_tostr(metrics_diff_t *mrD)
{
    static char b[16384];
    // New order, alphabetic:
    //   bandwidth, cache, cpi, cpufreq, cpupow, flops, gpu, imcfreq, io, nodepow, proc, temp
    scsprintf(&ss, id_bwidth , 0, 1,  bwidth_data_tostr(mrD->bwidth_diff ,  mrD->bwidth_avrg , b, sizeof(b)));
    scsprintf(&ss, id_cache  , 0, 1,   cache_data_tostr(mrD->cache_diff  ,  mrD->cache_avrg  , b, sizeof(b)));
    scsprintf(&ss, id_cpi    , 0, 1,     cpi_data_tostr(mrD->cpi_diff    ,  mrD->cpi_avrg    , b, sizeof(b)));
    scsprintf(&ss, id_cpufreq, 0, 1, cpufreq_data_tostr(mrD->cpufreq_diff,  mrD->cpufreq_avrg, b, sizeof(b)));
    scsprintf(&ss, id_cpupow , 0, 1,  cpupow_data_tostr(mrD->cpupow_diff ,  mrD->time        , b, sizeof(b)));
    scsprintf(&ss, id_flops  , 0, 1,   flops_data_tostr(mrD->flops_diff  ,  mrD->flops_avrg  , b, sizeof(b)));
    scsprintf(&ss, id_gpu    , 0, 1,     gpu_data_tostr(mrD->gpu_diff    ,                     b, sizeof(b)));
    scsprintf(&ss, id_imcfreq, 0, 1, imcfreq_data_tostr(mrD->imcfreq_diff, &mrD->imcfreq_avrg, b, sizeof(b)));
    scsprintf(&ss, id_io     , 0, 1,      io_data_tostr(mrD->io_diff     ,  mrD->io_avrg     , b, sizeof(b)));
    scsprintf(&ss, id_nodepow, 0, 1, nodepow_data_tostr(mrD->nodepow_avrg,                     b, sizeof(b)));
    scsprintf(&ss, id_proc   , 0, 1,    proc_data_tostr(mrD->proc_diff   ,                     b, sizeof(b)));
    scsprintf(&ss, id_temp   , 0, 1,    temp_data_tostr(mrD->temp_diff   ,  mrD->temp_avrg   , b, sizeof(b)));
    return scprintf(&ss);
}

uint *metrics_envtoops(char *var_name, uint options_expected)
{
    static uint *options = NULL;
    uint options_count   = 0;

    if (getenv(var_name) != NULL) {
        strtoat(getenv(var_name), ',', (void **) &options, &options_count, ID_UINT);
        #if SHOW_DEBUGS
        for (int i = 0; i < options_count; i++) {
            debug("OPT %d: %u", i, options[i]);
        }
        #endif
        if (options_count < options_expected) {
            fprintf(stderr, "Number of options in the list is less than %d.\n", options_expected);
            exit(EXIT_FAILURE);
        }
    }
    return options;
}

#if TEST
#include <common/system/monitor.h>
#include <daemon/local_api/eard_api.h>

static topology_t     tp;
static metrics_info_t m;
static metrics_read_t mr1;
static metrics_read_t mr2;
static metrics_diff_t mrD;

static state_t metrics_apis_init(void *whatever)
{
    unused(whatever);
    topology_init(&tp);
    metrics_load(&m, &tp, NULL, metrics_envtoops("OPTS", 10));
    metrics_init_screen(&m, &tp);
    metrics_data_alloc(&mr1, &mr2, &mrD);
    metrics_read(&mr1);
    return EAR_SUCCESS;
}

static state_t metrics_apis_update(void *whatever)
{
    unused(whatever);
    metrics_read_copy(&mr2, &mr1, &mrD);
    metrics_data_print(&mrD, 0);
    return EAR_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (atoi(argv[1])) {
            if (state_fail(eards_connection())) {
                printf("Connection error: %s\n", state_msg);
            }
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
