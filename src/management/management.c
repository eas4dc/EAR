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
/* clang-format off */
#include <stdlib.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/system/monitor.h>
#include <common/utils/special.h>
#include <common/utils/strscreen.h>
#include <common/utils/strtable.h>
#include <common/config/config_install.h>
#include <daemon/local_api/eard_api.h>
#include <management/management.h>

void management_load(manages_info_t *m, topology_t *tp, uint *options)
{
    uint default_options[10] = {0};

    if (options == NULL) {
        options = default_options;
    }
    mgt_cpufreq_load(tp, options[MGT_OPT_CPUFREQ]);
    mgt_imcfreq_load(tp, options[MGT_OPT_IMCFREQ], NULL);
    mgt_cpuprio_load(tp, API_FREE);
     mgt_cpupow_load(tp, options[MGT_OPT_CPUPOW ]);
        mgt_gpu_load(    options[MGT_OPT_GPU    ]);

    mgt_cpufreq_init(no_ctx);
    mgt_cpuprio_init();
    mgt_imcfreq_init(no_ctx);
     mgt_cpupow_init();
        mgt_gpu_init(no_ctx);

    management_info_get(m);
}

__attribute__((unused)) static void management_init_screen(manages_info_t *m, topology_t *tp)
{
    static char buffer[16384];
    static strscreen_t ss;
    static int idcpufreq;
    static int idcpuprio;
    static int idcpupow;
    static int idimcfreq;
    static int idgpu;
    static int idl;
    static int ida;
    static int idh;

    // Screen initialization
    scprintf_init(&ss, 50, 180, 0, '#'); // 0 to 49, 0 to 179
    scprintf_divide(&ss, (int[]) { 1,   2}, (int[]) {14,  39}, &idcpufreq, "cpufreq"  );
    scprintf_divide(&ss, (int[]) {16,   2}, (int[]) {29,  39}, &idcpuprio, "cpuprio"  );
    scprintf_divide(&ss, (int[]) {31,   2}, (int[]) {44,  39}, &idcpupow , "cpupow"   );
    scprintf_divide(&ss, (int[]) { 1,  41}, (int[]) {44,  77}, &idimcfreq, "imcfreq"  );
    scprintf_divide(&ss, (int[]) { 1,  79}, (int[]) {44, 115}, &idgpu    , "gpu"      );
    scprintf_divide(&ss, (int[]) { 1, 154}, (int[]) { 6, 177}, &idl      , "ear-logo" );
    scprintf_divide(&ss, (int[]) { 8, 124}, (int[]) {14, 177}, &ida      , "api-table");
    scprintf_divide(&ss, (int[]) {16, 154}, (int[]) {31, 177}, &idh      , "topology" );

    topology_tostr(tp, buffer, sizeof(buffer));
    scsprintf(&ss, idh, 0, 0, buffer);

    scsprintf(&ss, idl, 0, 0, special_ear_logo());
    scsprintf(&ss, ida, 0, 0, management_info_tostr(m, buffer));
    mgt_cpufreq_data_tostr(m->cpufreq.list1, m->cpufreq.list1_count, buffer, sizeof(buffer));
    scsprintf(&ss, idcpufreq, 0, 1, buffer);
    mgt_cpuprio_data_tostr(m->cpuprio.list1, m->cpuprio.list2, buffer, sizeof(buffer));
    scsprintf(&ss, idcpuprio, 0, 1, buffer);
    mgt_cpupow_tdp_tostr(CPUPOW_SOCKET, m->cpupow.list1, buffer, sizeof(buffer));
    scsprintf(&ss, idcpupow, 0, 1, buffer);
    mgt_imcfreq_data_tostr(m->imcfreq.list1, m->imcfreq.list1_count, buffer, sizeof(buffer));
    scsprintf(&ss, idimcfreq, 0, 1, buffer);
    scsprintf(&ss, idgpu, 0, 1, mgt_gpu_features_tostr(buffer, sizeof(buffer)));

    scprintf(&ss);
}

void management_info_get(manages_info_t *m)
{
    // New and powerful get_info()
    mgt_cpupow_get_info(&m->cpupow);

    // The next 5 paragraphs are obsolete
    m->cpufreq.layer = "MGT_CPUFREQ";
    m->cpuprio.layer = "MGT_CPUPRIO";
    m->imcfreq.layer = "MGT_IMCFREQ";
    m->gpu.layer     = "MGT_GPU";

    mgt_cpufreq_get_api(&m->cpufreq.api);
    mgt_cpuprio_get_api(&m->cpuprio.api);
    mgt_imcfreq_get_api(&m->imcfreq.api);
    mgt_gpu_get_api(&m->gpu.api);

    m->cpufreq.granularity = GRANULARITY_THREAD;
    m->cpuprio.granularity = GRANULARITY_THREAD;
    m->imcfreq.granularity = GRANULARITY_SOCKET;
    m->gpu.granularity     = GRANULARITY_PERIPHERAL;

    m->cpufreq.scope = SCOPE_NODE;
    m->cpuprio.scope = SCOPE_NODE;
    m->imcfreq.scope = SCOPE_NODE;
    m->gpu.scope     = SCOPE_NODE;

    mgt_cpufreq_count_devices(no_ctx, &m->cpufreq.devs_count);
    mgt_cpuprio_count_devices(no_ctx, &m->cpuprio.devs_count);
    mgt_imcfreq_count_devices(no_ctx, &m->imcfreq.devs_count);
    mgt_gpu_count_devices(no_ctx, &m->gpu.devs_count);

    apinfo_tostr(&m->cpufreq);
    apinfo_tostr(&m->cpuprio);
    apinfo_tostr(&m->cpupow );
    apinfo_tostr(&m->imcfreq);
    apinfo_tostr(&m->gpu);

    // Allocations for system services
    mgt_cpufreq_data_alloc((pstate_t **)  &m->cpufreq.list2, (uint **) &m->cpufreq.list3); // Current PSs and set indexes
    mgt_imcfreq_data_alloc((pstate_t **)  &m->imcfreq.list2, (uint **) &m->imcfreq.list3); // Current PSs and set indexes
    mgt_cpuprio_data_alloc((cpuprio_t **) &m->cpuprio.list1, (uint **) &m->cpuprio.list2); // Avail CLOSes and set assocs
    mgt_gpu_freq_data_alloc((ulong **) &m->gpu.list2); // Current KHz/Watts
    mgt_gpu_freq_data_alloc((ulong **) &m->gpu.list3); // CUrrent KHz/Watts

    // Get available frequencies (constants)
    mgt_cpufreq_get_available_list(no_ctx, (const pstate_t **) &m->cpufreq.list1, &m->cpufreq.list1_count);
    mgt_imcfreq_get_available_list(no_ctx, (const pstate_t **) &m->imcfreq.list1, &m->imcfreq.list1_count);

    // Get available priorities (allocated)
    mgt_cpuprio_data_count(&m->cpuprio.list1_count, &m->cpuprio.list2_count); //
    mgt_cpuprio_get_available_list(m->cpuprio.list1);
    mgt_cpuprio_get_current_list((uint *) m->cpuprio.list2);

    // Get current socket TDPs
    mgt_cpupow_data_alloc(CPUPOW_SOCKET, (uint32_t **) &m->cpupow.list1);
    mgt_cpupow_tdp_get(CPUPOW_SOCKET, m->cpupow.list1);

    // Get GPUs information
    mgt_gpu_get_devices(no_ctx, (gpu_devs_t **) &m->gpu.list1, &m->gpu.list1_count);
}

char *management_info_tostr(manages_info_t *m, char *buffer)
{
    static strtable_t st;

    double cpu0ps0 = ((double) (((const pstate_t *) m->cpufreq.list1)[0].khz)) / 1000000.0;
    double cpu0psM = ((double) (((const pstate_t *) m->cpufreq.list1)[m->cpufreq.list1_count - 1].khz)) / 1000000.0;
    double imc0ps0 = ((double) (((const pstate_t *) m->imcfreq.list1)[0].khz)) / 1000000.0;
    double imc0psM = ((double) (((const pstate_t *) m->imcfreq.list1)[m->cpufreq.list1_count - 1].khz)) / 1000000.0;
    uint cpu0tdp   = ((uint *) m->cpupow.list1)[0];

    tprintf_init2(&st, fdout, STR_MODE_COL, "13 9 8 10 13 20");
    tsprintf(buffer, &st, 0, "E.API||I.API||#Devs||Scope||Granularity||Details");
    tsprintf(buffer, &st, 1, "-----||-----||-----||-----||-----------||-------");
    if (m->cpufreq.api  > API_DUMMY) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||%0.2lf to %0.2lf GHz", m->cpufreq.layer, m->cpufreq.api_str, m->cpufreq.devs_count, m->cpufreq.scope_str, m->cpufreq.granularity_str, cpu0ps0, cpu0psM);
    if (m->cpufreq.api <= API_DUMMY) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||                    ", m->cpufreq.layer, m->cpufreq.api_str, m->cpufreq.devs_count, m->cpufreq.scope_str, m->cpufreq.granularity_str);
    if (m->cpuprio.api == API_NONE ) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||                    ", m->cpuprio.layer, m->cpuprio.api_str, m->cpuprio.devs_count, m->cpuprio.scope_str, m->cpuprio.granularity_str);
    if (m->cpupow.api   > API_DUMMY) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||%u W (TDP)          ", m->cpupow.layer , m->cpupow.api_str , m->cpupow.devs_count , m->cpupow.scope_str , m->cpupow.granularity_str, cpu0tdp);
    if (m->cpupow.api  <= API_DUMMY) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||                    ", m->cpupow.layer , m->cpupow.api_str , m->cpupow.devs_count , m->cpupow.scope_str , m->cpupow.granularity_str);
    if (m->imcfreq.api  > API_DUMMY) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||%0.2lf to %0.2lf GHz", m->imcfreq.layer, m->imcfreq.api_str, m->imcfreq.devs_count, m->imcfreq.scope_str, m->imcfreq.granularity_str, imc0ps0, imc0psM);
    if (m->imcfreq.api <= API_DUMMY) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||                    ", m->imcfreq.layer, m->imcfreq.api_str, m->imcfreq.devs_count, m->imcfreq.scope_str, m->imcfreq.granularity_str);
    if (m->gpu.api      > API_DUMMY) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||                    ", m->gpu.layer    , m->gpu.api_str    , m->gpu.devs_count    , m->gpu.scope_str    , m->gpu.granularity_str);
    // if (m->gpu.api  == API_NONE ) tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s||                    ", m->gpu.layer    , m->gpu.api_str    , m->gpu.devs_count    , m->gpu.scope_str    , m->gpu.granularity_str);
    return buffer;
}

char *management_data_tostr()
{
    return NULL;
}

#if TEST
static manages_info_t man;
static topology_t tp;

static state_t monitor_apis_init(void *whatever)
{
    unused(whatever);
    topology_init(&tp);
    management_load(&man, &tp, metrics_envtoops("OPTS", 4));
    management_init_screen(&man, &tp);
    return EAR_SUCCESS;
}

static state_t monitor_apis_update(void *whatever)
{
    unused(whatever);
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
    sus->call_init     = monitor_apis_init;
    sus->call_main     = monitor_apis_update;
    sus->time_relax    = 2000;
    sus->time_burst    = 2000;
    sus->suscribe(sus);

    sleep(100);

    return 0;
}
#endif
