/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/states.h>
#include <common/utils/string.h>

#include <metrics/metrics.h>
#include <management/management.h>
#include <daemon/local_api/eard_api.h>

char node_name[256];
topology_t tp;
char buffer[2048];

#define check(st, msg) \
    if (st != EAR_SUCCESS){\
        printf(msg);\
        eards_disconnect();\
        exit(1);\
    }

int usage(char *app)
{
    printf("Usage: %s [options]\n", app);
    printf("At least one option is required\n");
    printf("--cpu-info		shows information about cpu frequencies\n");
    printf("--mem-info		shows information about memory frequencies\n");
    printf("--gpu-info		shows information about gpu frequencies\n");
    printf("--all     		shows information about all the devices known by ear\n");
    printf("--help        shows this message\n");
    exit(0);
}

void show_gpu_info()
{

    printf("EAR GPU info in node %s\n",node_name);
    check(eards_connection(),"eards_connection");


    mgt_gpu_load(EARD);
    check(mgt_gpu_init(no_ctx), "mgt_gpu_init");
    uint gpu_api;
    mgt_gpu_get_api(&gpu_api);
    apis_tostr(gpu_api, buffer, sizeof(buffer));
    printf("EAR GPU info API: %s\n", buffer);

    uint gpu_dev;
    check(mgt_gpu_count_devices(no_ctx,&gpu_dev),"mgt_gpu_count_devices");
    printf("EAR GPU info num devices %u\n", gpu_dev);

    printf("EAR GPU info is supported %u\n", mgt_gpu_is_supported());

    ulong **gpu_freq_list;
    uint  *num_freqs;
    check(mgt_gpu_freq_get_available(no_ctx,(const ulong ***) &gpu_freq_list,(const uint **) &num_freqs),"mgt_gpu_freq_get_available");
    printf("EAR GPU info : Print list of freqs for GPU 0 assuming all are equal\n");
    ulong *curr_list = gpu_freq_list[0];
    for (uint gfreq = 0; gfreq < num_freqs[0]; gfreq ++){
        printf("EAR GPU info : GPU freq[0][%u] = %lu\n", gfreq, curr_list[gfreq]);
    }

    ulong *curr_gpuf;
    check(mgt_gpu_data_alloc(&curr_gpuf),"mgt_gpu_data_alloc");
    check(mgt_gpu_freq_limit_get_current(no_ctx, curr_gpuf),"mgt_gpu_freq_limit_get_current");
    for (uint cgpu = 0; cgpu < gpu_dev; cgpu++){
        printf("EAR GPU info : GPU freq[%u] = %lu\n", cgpu, curr_gpuf[cgpu]);
    }

    mgt_gpu_dispose(no_ctx);

    eards_disconnect();
}

void show_mem_info()
{
    printf("EAR MEM info in node %s\n",node_name);
    check(eards_connection(),"eards_connection");

    check(mgt_imcfreq_load(&tp, EARD, NULL),"mgt_imcfreq_load");
    check(mgt_imcfreq_init(no_ctx), "mgt_imcfreq_init");

    uint mem_api;
    check(mgt_imcfreq_get_api(&mem_api),"mgt_imcfreq_get_api");
    apis_tostr(mem_api, buffer, sizeof(buffer));
    printf("EAR MEM info API: %s\n", buffer);


    uint mem_devices;
    check(mgt_imcfreq_count_devices(no_ctx, &mem_devices),"mgt_imcfreq_count_devices");
    printf("EAR MEM info num devices %u\n", mem_devices);

    pstate_t *mem_pstate_list;
    uint      num_pstates;

    check(mgt_imcfreq_get_available_list(no_ctx, (const pstate_t **)&mem_pstate_list, &num_pstates),"mgt_imcfreq_get_available_list");
    pstate_tostr(mem_pstate_list, num_pstates, buffer, sizeof(buffer));
    printf("EAR MEM info list of MEM frequencies \n%s\n", buffer);

    pstate_t *current_mem_list;
    uint     *index;
    check(mgt_imcfreq_data_alloc(&current_mem_list, &index), "mgt_imcfreq_data_alloc");
    check(mgt_imcfreq_get_current_list(no_ctx, current_mem_list),"mgt_imcfreq_get_current_list");

    for (uint cdev = 0; cdev < mem_devices; cdev ++){
        printf("EAR MEM info curr freq DEV[%u] = %llu\n", cdev, current_mem_list[cdev].khz);
    }



    mgt_imcfreq_dispose(no_ctx);

    eards_disconnect();
}

void show_cpu_info()
{
    uint api;
    printf("EAR CPU info in node %s\n",node_name);
    check(eards_connection(),"eards_connection");

    topology_tostr(&tp, buffer, sizeof(buffer));
    printf("EAR CPU info Topology: %s\n", buffer);

    printf("EAR CPU info load\n");
    check(mgt_cpufreq_load(&tp, EARD),"mgt_cpufreq_load");
    check(mgt_cpufreq_init(no_ctx), "mgt_cpufreq_init");
    mgt_cpufreq_get_api(&api);
    apis_tostr(api, buffer, sizeof(buffer));
    printf("EAR CPU info API: %s\n", buffer);

    uint cpuf_num_dev;
    mgt_cpufreq_count_devices(no_ctx, &cpuf_num_dev);
    printf("EAR CPU info num devices:%u\n", cpuf_num_dev);

    pstate_t *cpu_pstate_list;
    uint 			num_pstates;
    check(mgt_cpufreq_get_available_list(no_ctx, (const pstate_t **)&cpu_pstate_list, &num_pstates), "mgt_cpufreq_get_available_list");

    pstate_tostr(cpu_pstate_list, num_pstates, buffer, sizeof(buffer));
    printf("EAR CPU info list of CPU frequencies \n%s\n", buffer);

    uint pstate_nominal;
    check(mgt_cpufreq_get_nominal(no_ctx, &pstate_nominal),"mgt_cpufreq_get_nominal");
    printf("EAR CPU info pstate nominal is %u, CPU freq = %llu KHz\n", pstate_nominal, cpu_pstate_list[pstate_nominal].khz);

    uint *governor_list;
    governor_list = (uint *)calloc(cpuf_num_dev, sizeof(uint));
    check(mgt_cpufreq_governor_get_list(no_ctx, governor_list),"mgt_cpufreq_governor_get_list");

    for (uint c = 0; c < cpuf_num_dev; c++){
        governor_tostr(governor_list[c],buffer);
        printf("EAR CPU info governor CPU[%u] = %s\n", c, buffer);
    }

    /* Asking for the current CPU freq requires data allocation */
    pstate_t *curr_cpu_pstate;
    uint     *index;
    mgt_cpufreq_data_alloc(&curr_cpu_pstate, &index);
    check(mgt_cpufreq_get_current_list(no_ctx, curr_cpu_pstate),"mgt_cpufreq_get_current_list");
    for (uint ccpu = 0; ccpu < cpuf_num_dev; ccpu++){
        printf("EAR CPU info curr CPUF[%u] = %llu\n", ccpu, curr_cpu_pstate[ccpu].khz);
    }

    /* CPU power API is not yet available at user level */

    mgt_cpufreq_dispose(no_ctx);
    eards_disconnect();
}


int main(int argc, char *argv[])
{

    if (getenv("EAR_TMP") == NULL) {
        printf("EAR_TMP must be set for the application to work\n Please load ear module\n");
        return 0;
    }

    char c;
    int option_idx = 0;
    static struct option long_options[] = {
        {"cpu-info",      no_argument, 0, 0},
        {"mem-info",      no_argument, 0, 1},
        {"gpu-info",    	no_argument, 0, 2},
        {"all",           no_argument, 0, 3},
        {"help",          no_argument, 0, 4},
        {NULL,          0, 0, 0}
    };

    /* Node name */

    /* Node name */
    gethostname(node_name, sizeof(node_name));
    strtok(node_name, ".");
    topology_init(&tp);

    while (1) {
        c = getopt_long(argc, argv, "c:m:g:a:h", long_options, &option_idx);
        if (c == -1) break;
        switch (c) {
            case 0:
                show_cpu_info(); break;
            case 1:
                show_mem_info(); break;
            case 2:
                show_gpu_info(); break;
            case 3:
                show_cpu_info();
                show_mem_info();
                show_gpu_info();
                break;
            case 4: usage(argv[0]);
            default:
                    break;
        }
    }


    return 0;
}

