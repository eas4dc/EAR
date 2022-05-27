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
#include <management/management.h>
#include <common/output/verbose.h>

#define sa(f) \
        if (state_fail(f)) { \
                serror(#f " FAILED"); \
                exit(0); \
        }

static manages_apis_t m;

void manages_apis_init(topology_t *tp, uint eard, manages_apis_t **m_in)
{
	state_t s;
	// Load
	sa(mgt_cpufreq_load(tp, eard));
	sa(mgt_imcfreq_load(tp, eard, NULL));
	// Get API
	sa(mgt_cpufreq_get_api(&m.cpufreq.api));
	sa(mgt_imcfreq_get_api(&m.imcfreq.api));
	// Init
	sa(mgt_cpufreq_init(no_ctx));
	sa(mgt_imcfreq_init(no_ctx));
	// Count (cache does not need)
	sa(mgt_cpufreq_count_devices(no_ctx, &m.cpufreq.devs_count));
	sa(mgt_imcfreq_count_devices(no_ctx, &m.imcfreq.devs_count));
	// Availability
	sa(mgt_cpufreq_get_available_list(no_ctx, (const pstate_t **) &m.cpufreq.avail_list, &m.cpufreq.avail_count));
	sa(mgt_imcfreq_get_available_list(no_ctx, (const pstate_t **) &m.imcfreq.avail_list, &m.imcfreq.avail_count));
	// Current allocation
	sa(mgt_cpufreq_data_alloc((pstate_t **) &m.cpufreq.current_list, (uint **) &m.cpufreq.set_list));
	sa(mgt_cpufreq_data_alloc((pstate_t **) &m.imcfreq.current_list, (uint **) &m.imcfreq.set_list));
	
	if (m_in != NULL) {
		*m_in = &m;
	}
}

void manages_apis_dispose()
{
	sa(mgt_cpufreq_dispose(no_ctx));
	sa(mgt_imcfreq_dispose(no_ctx));
}

void manages_apis_print()
{
	apis_print(m.cpufreq.api, "MGT_CPUFREQ: ");
	apis_print(m.imcfreq.api, "MGT_IMCFREQ: ");
}

#define printline() \
    verbose(0, "------------------------------");

void manages_apis_print_available()
{
	mgt_cpufreq_data_print(m.cpufreq.avail_list, m.cpufreq.avail_count, verb_channel);
	mgt_imcfreq_data_print(m.imcfreq.avail_list, m.imcfreq.avail_count, verb_channel);
}

void manages_apis_print_current()
{
    sa(mgt_cpufreq_get_current_list(no_ctx, m.cpufreq.current_list));
    sa(mgt_imcfreq_get_current_list(no_ctx, m.imcfreq.current_list));
	
    mgt_cpufreq_data_print(m.cpufreq.current_list, m.cpufreq.devs_count, verb_channel);
    mgt_imcfreq_data_print(m.imcfreq.current_list, m.imcfreq.devs_count, verb_channel);
}

#if TEST
// gcc -I .. -DTEST=1 -o test management.c ../management/libmgt.a ../metrics/libmetrics.a ../daemon/local_api/libeard.a ../common/libcommon.a -lpthread -ldl

#include <daemon/local_api/eard_api_rpc.h>

static topology_t tp;

int is(char *buf, char *string)
{
    return strcmp(buf, string) == 0;
}

void read_command()
{
    char cmnd[SZ_PATH];
    char aux_string[SZ_PATH];
    uint aux_uint;

    scanf("%s", cmnd);
 
    if (is(cmnd, "mgt_cpufreq_set")) {
        scanf("%d", &aux_uint);
        sa(mgt_cpufreq_set_current(no_ctx, aux_uint, all_devs));
    } else if (is(cmnd, "mgt_cpufreq_get_governor")) {
        sa(mgt_cpufreq_get_governor(no_ctx, &aux_uint));
        sa(mgt_governor_tostr(aux_uint, aux_string));
        verbose(0, "Governor: %s", aux_string);
    } else if (is(cmnd, "mgt_cpufreq_set_governor")) {
        scanf("%s", aux_string);
        sa(mgt_governor_toint(aux_string, &aux_uint));
        sa(mgt_cpufreq_set_governor(no_ctx, aux_uint));
    } else if (is(cmnd, "mgt_imcfreq_set")) {
        scanf("%d", &aux_uint);
        sa(mgt_imcfreq_set_current(no_ctx, aux_uint, all_devs));
    } else if (is(cmnd, "print")) {
        manages_apis_print_current();
    } else if (is(cmnd, "disconnect")) {
        eards_disconnect();
    } else {
    }

    printline();
}

int main(int argc, char *argv[])
{
    sa(topology_init(&tp));

    // Connecting with EARD
    eards_connection();
    
    // General init
    manages_apis_init(&tp, EARD, NULL);
    manages_apis_print();
    manages_apis_print_available();
    printline();
  
    // Main loop 
    while(1) {
        read_command();
    }

    return 0;
}
#endif
