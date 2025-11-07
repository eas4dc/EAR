/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <metrics/gpu/gpuprof.h>

#include <metrics/common/apis.h>
#include <stdio.h>
#include <stdlib.h>

#define EVENT_SET_CNT 2 // The number of sets you expect to test.

static apinfo_t info;

static gpuprof_t *m1;
static gpuprof_t *m2;
static gpuprof_t *mD;

static const gpuprof_evs_t *evs;
static uint evs_count;

int main(int argc, char *argv[])
{
    gpuprof_load(0);
    gpuprof_init();
    gpuprof_get_info(&info);
    apinfo_tostr(&info);
    printf("LOADED: %s %s %s %d\n", info.layer, info.api_str, info.granularity_str, info.devs_count);

    gpuprof_data_alloc(&m1);
    gpuprof_data_alloc(&m2);
    gpuprof_data_alloc(&mD);
    printf("ALLOCATED DATA\n");

    gpuprof_events_get(&evs, &evs_count);
    for (int i = 0; i < evs_count; ++i) {
        printf("EVENT_%d: id.%02u %s\n", i, evs[i].id, evs[i].name);
    }

    // char aux[8];
    // printf("Waiting before event set...\n");
    // scanf("%s", aux);

    char sets[EVENT_SET_CNT][512];

    for (int set_idx = 0; set_idx < EVENT_SET_CNT; set_idx++) {
        printf("Enter a comma-separated list of EVENTS set %d: ", set_idx);
        scanf("%s", sets[set_idx]);
        // Hardcoded if using gdb
        // snprintf(sets[set_idx], 512, "11,12,13");

        if (!set_idx) {
            gpuprof_events_set(all_devs, sets[set_idx]);
            printf("EVENTS SET %d: %s\n", set_idx, sets[set_idx]);
        }
    }

    // printf("Waiting after event set...\n");
    // scanf("%s", aux);

    gpuprof_read(m1);
    sleep(5);

    int curr_set = 0;
    int count    = 0;
    while (count < 100) {
        gpuprof_read_diff(m2, m1, mD);
        for (int d = 0; d < info.devs_count; ++d) {
            for (int m = 0; m < mD[d].values_count; ++m) {
                printf("METRIC_D%d_M%d: %lf\n", d, m, mD[d].values[m]);
            }
        }

        // printf("Waiting after reading set...\n");
        // scanf("%s", aux);

        count++;
        if (EVENT_SET_CNT != 1 && (count % EVENT_SET_CNT == 0)) {
            gpuprof_events_unset(0);

            // printf("Waiting after unsetting events...\n");
            // scanf("%s", aux);

            curr_set = (curr_set + 1) % EVENT_SET_CNT;
            gpuprof_events_set(0, sets[curr_set]);

            printf("EVENTS SET %d: %s\n", curr_set, sets[curr_set]);

            gpuprof_read(m1);
            // printf("Waiting after setting new events...\n");
            // scanf("%s", aux);
        } else {
            gpuprof_data_copy(m1, m2);
        }
        sleep(2);
    }

    return 0;
}
