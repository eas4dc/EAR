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
#include <common/utils/special.h>
#include <common/utils/strtable.h>
#include <common/utils/strscreen.h>
#include <common/config/config_install.h>
#include <daemon/local_api/eard_api.h>
#include <management/management.h>

static manages_apis_t m;

void management_init(topology_t *tp, int eard, manages_apis_t **man)
{
    mgt_cpufreq_load(tp, eard      );
    mgt_cpuprio_load(tp, eard      );
    mgt_imcfreq_load(tp, eard, NULL);
     mgt_cpupow_load(tp, eard      );
        mgt_gpu_load(    eard      );

    mgt_cpufreq_init(no_ctx);
    mgt_cpuprio_init(no_ctx);
    mgt_imcfreq_init(no_ctx);
     mgt_cpupow_init(      );
        mgt_gpu_init(no_ctx);

    // The next 5 paragraphs are obsolete
    m.cpu.layer = "MGT_CPUFREQ";
    m.pri.layer = "MGT_CPUPRIO";
    m.imc.layer = "MGT_IMCFREQ";
    m.gpu.layer = "MGT_GPU";

    mgt_cpufreq_get_api(&m.cpu.api);
    mgt_cpuprio_get_api(&m.pri.api);
    mgt_imcfreq_get_api(&m.imc.api);
        mgt_gpu_get_api(&m.gpu.api);

    m.cpu.granularity = GRANULARITY_THREAD;
    m.pri.granularity = GRANULARITY_THREAD;
    m.imc.granularity = GRANULARITY_SOCKET;
    m.gpu.granularity = GRANULARITY_PERIPHERAL;

    m.cpu.scope = SCOPE_NODE;
    m.pri.scope = SCOPE_NODE;
    m.imc.scope = SCOPE_NODE;
    m.gpu.scope = SCOPE_NODE;

    mgt_cpufreq_count_devices(no_ctx, &m.cpu.devs_count);
    mgt_cpuprio_count_devices(no_ctx, &m.pri.devs_count);
    mgt_imcfreq_count_devices(no_ctx, &m.imc.devs_count);
        mgt_gpu_count_devices(no_ctx, &m.gpu.devs_count);

    // New and powerful get_info()
    mgt_cpupow_get_info(&m.pow);

    apinfo_tostr(&m.cpu);
    apinfo_tostr(&m.pri);
    apinfo_tostr(&m.pow);
    apinfo_tostr(&m.imc);
    apinfo_tostr(&m.gpu);
    
    // Allocations for system services
    mgt_cpufreq_data_alloc((pstate_t  **) &m.cpu.list2, (uint **) &m.cpu.list3); // Current PSs and set indexes
    mgt_imcfreq_data_alloc((pstate_t  **) &m.imc.list2, (uint **) &m.imc.list3); // Current PSs and set indexes
    mgt_cpuprio_data_alloc((cpuprio_t **) &m.pri.list1, (uint **) &m.pri.list2); // Avail CLOSes and set assocs
        mgt_gpu_data_alloc((ulong     **) &m.gpu.list2); // Current KHz/Watts
        mgt_gpu_data_alloc((ulong     **) &m.gpu.list3); // CUrrent KHz/Watts

    // Get available frequencies (constants)
    mgt_cpufreq_get_available_list(no_ctx, (const pstate_t **) &m.cpu.list1, &m.cpu.list1_count);
    mgt_imcfreq_get_available_list(no_ctx, (const pstate_t **) &m.imc.list1, &m.imc.list1_count);

    // Get available priorities (allocated)
    mgt_cpuprio_data_count(&m.pri.list1_count, &m.pri.list2_count); //
    mgt_cpuprio_get_available_list(m.pri.list1);

    // Get current socket TDPs
    mgt_cpupow_data_alloc(CPUPOW_SOCKET, (uint **) &m.pow.list1);
    mgt_cpupow_tdp_get(CPUPOW_SOCKET, m.pow.list1);

    // Get GPUs information
    mgt_gpu_get_devices(no_ctx, (gpu_devs_t **) &m.gpu.list1, &m.gpu.list1_count);

    if (man != NULL) {
        *man = &m;
    }
}

#if TEST
static topology_t  tp;
static strscreen_t ss;
static strtable_t  st;
static int         idc;
static int         idr;
static int         idp;
static int         idi;
static int         idl;
static int         ida;
static int         idh;

static state_t monitor_apis_init(void *whatever)
{
    char buffer[16384];

    topology_init(&tp);

    management_init(&tp, EARD, NULL);

    wprintf_init(&ss, 50, 180, 0, '#'); // 0 to 49, 0 to 179
    wprintf_divide(&ss, (int[]) {  1,   2}, (int[]) { 14,  39}, &idc, "cpufreq"  );
    wprintf_divide(&ss, (int[]) { 16,   2}, (int[]) { 29,  39}, &idr, "cpuprio"  );
    wprintf_divide(&ss, (int[]) { 31,   2}, (int[]) { 44,  39}, &idp, "cpupow"   );
    wprintf_divide(&ss, (int[]) {  1,  41}, (int[]) { 44,  77}, &idi, "imcfreq"  );
    wprintf_divide(&ss, (int[]) {  1, 154}, (int[]) {  6, 177}, &idl, "ear-logo" );
    wprintf_divide(&ss, (int[]) {  8, 124}, (int[]) { 14, 177}, &ida, "api-table");
    wprintf_divide(&ss, (int[]) { 16, 154}, (int[]) { 31, 177}, &idh, "topology" );

    topology_tostr(&tp, buffer, sizeof(buffer));
    wsprintf(&ss, idh, 0, 0, buffer);

    tprintf_init2(&st, fdout, STR_MODE_COL, "13 9 8 10 20");
    tsprintf(buffer, &st, 0, "E.API||I.API||#Devs||Scope||Granularity");
    tsprintf(buffer, &st, 1, "-----||-----||-----||-----||-----------");
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.cpu.layer, m.cpu.api_str, m.cpu.devs_count, m.cpu.scope_str, m.cpu.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.pri.layer, m.pri.api_str, m.pri.devs_count, m.pri.scope_str, m.pri.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.pow.layer, m.pow.api_str, m.pow.devs_count, m.pow.scope_str, m.pow.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.imc.layer, m.imc.api_str, m.imc.devs_count, m.imc.scope_str, m.imc.granularity_str);
    tsprintf(buffer, &st, 1, "%s||%s||%u||%s||%s", m.gpu.layer, m.gpu.api_str, m.gpu.devs_count, m.gpu.scope_str, m.gpu.granularity_str);

    wsprintf(&ss, idl, 0, 0, special_ear_logo());
    wsprintf(&ss, ida, 0, 0, buffer    );

    mgt_cpufreq_data_tostr(m.cpu.list1, m.cpu.list1_count, buffer, sizeof(buffer));
    wsprintf(&ss, idc, 0, 1, buffer);

    mgt_cpuprio_data_tostr(m.pri.list1, m.pri.list2, buffer, sizeof(buffer));
    wsprintf(&ss, idr, 0, 1, buffer);

    mgt_cpupow_tdp_tostr(CPUPOW_SOCKET, m.pow.list1, buffer, sizeof(buffer));
    wsprintf(&ss, idp, 0, 1, buffer);

    mgt_imcfreq_data_tostr(m.imc.list1, m.imc.list1_count, buffer, sizeof(buffer));
    wsprintf(&ss, idi, 0, 1, buffer);

    wprintf(&ss);

    return EAR_SUCCESS;
}

static state_t monitor_apis_update(void *whatever)
{
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
