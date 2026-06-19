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
#include <stdlib.h>
#include <string.h>
#include <common/output/debug.h>
#include <common/system/random.h>
#include <metrics/common/offsets.h>
#include <metrics/cache/archs/demo.h>

static ofops_t ops[3];
static uint    d;
static uint    devs_count;
static uint    scope;
static uint    granularity;
static double  line_size;

#define O2(l, v) (offsetof(cache_t, l) + offsetof(cache_level_t, v))
#define O1(l)    (offsetof(cache_t, l))

static void load_everlasting()
{
    static int already_loaded = 0;
    size_t sz = sizeof(cache_t);

    if (already_loaded) {
        return;
    }
    ofops_add(&ops[0], ID_UINT64, 'r', sz, O2(l1d, accesses ),  40000000,   8000000,  0); //Idle
    ofops_add(&ops[0], ID_UINT64, 'r', sz, O2(l1d, misses   ),   8000000,   2000000, -1); //
    ofops_add(&ops[0], ID_UINT64, 'r', sz, O2(l2 , accesses ),   8000000,   2000000, -1); //
    ofops_add(&ops[0], ID_UINT64, 'r', sz, O2(l2 , misses   ),   2000000,    500000,  0); //
    ofops_add(&ops[0], ID_UINT64, 'r', sz, O2(l3 , accesses ),   2000000,    500000, -1); //
    ofops_add(&ops[0], ID_UINT64, 'r', sz, O2(l3 , misses   ),    500000,    100000,  0); //
    ofops_add(&ops[1], ID_UINT64, 'r', sz, O2(l1d, accesses ), 750000000, 600000000,  0); //Mem intensive
    ofops_add(&ops[1], ID_UINT64, 'r', sz, O2(l1d, misses   ),  75000000,  60000000, -1); //
    ofops_add(&ops[1], ID_UINT64, 'r', sz, O2(l2 , accesses ),  75000000,  50000000, -1); //
    ofops_add(&ops[1], ID_UINT64, 'r', sz, O2(l2 , misses   ),  20000000,  10000000,  0); //
    ofops_add(&ops[1], ID_UINT64, 'r', sz, O2(l3 , accesses ),  20000000,   5000000, -1); //
    ofops_add(&ops[1], ID_UINT64, 'r', sz, O2(l3 , misses   ),   2000000,   1000000,  0); //
    ofops_add(&ops[2], ID_UINT64, '-', sz, O2(l1d, hits     ), O2(l1d, accesses), O2(l1d, misses  ), 0);
    ofops_add(&ops[2], ID_UINT64, '-', sz, O2(l2 , hits     ), O2(l2 , accesses), O2(l2 , misses  ), 0);
    ofops_add(&ops[2], ID_UINT64, '-', sz, O2(l3 , hits     ), O2(l3 , accesses), O2(l3 , misses  ), 0);
    ofops_add(&ops[2], ID_DOUBLE, '/', sz, O2(l1d, miss_rate), O2(l1d, misses  ), O2(l1d, accesses), 0);
    ofops_add(&ops[2], ID_DOUBLE, '/', sz, O2(l2 , miss_rate), O2(l2 , misses  ), O2(l2 , accesses), 0);
    ofops_add(&ops[2], ID_DOUBLE, '/', sz, O2(l3 , miss_rate), O2(l3 , misses  ), O2(l3 , accesses), 0);
    ofops_add(&ops[2], ID_DOUBLE, 'a', sz, O2(l2 , lines_out), O2(l2 , misses  ),                 0, 0);
    ofops_add(&ops[2], ID_UINT32, 'i', sz, O1(pid),   2187, 0, 0);
    ofops_add(&ops[2], ID_NULL  , '&', sz, O1(ll ), O1(l3), 0, 0);
    ofops_add(&ops[2], ID_NULL  , '&', sz, O1(lbw), O1(l3), 0, 0);
    already_loaded = 1;
}

CACHE_F_LOAD(demo)
{
    if (!API_IS(options, API_DEMO)) {
        return;
    }
    // If other API is loaded before, DEMO won't be
    if (api_already_loaded(ops)) {
        return;
    }
    // Initializations
    load_everlasting();
    line_size = (double) tp->cache_line_size;
    if (SCOPE_IS(options, SCOPE_NODE)) {
        devs_count  = tp->cpu_count;
        scope       = SCOPE_NODE;
        granularity = GRANULARITY_CPU;
    } else if (SCOPE_IS(options, SCOPE_JOB)) {
        devs_count  = tp->cpu_count;
        scope       = SCOPE_JOB;
        granularity = GRANULARITY_PROCESS;
    } else {
        devs_count  = 1;
        scope       = SCOPE_PROCESS;
        granularity = GRANULARITY_PROCESS;
    }
    apis_put(ops->unload   , cache_demo_unload   );
    apis_put(ops->update   , cache_demo_update   );
    apis_put(ops->get_info , cache_demo_get_info );
    apis_put(ops->read     , cache_demo_read     );
    apis_put(ops->data_diff, cache_demo_data_diff);
    apis_put(ops->internals_tostr, cache_demo_internals_tostr);
}

CACHE_F_UNLOAD(demo)
{
}

CACHE_F_UPDATE(demo)
{
    if (option >= UPD_DEMO1_SET && option <= UPD_DEMO4_SET) {
        d = (option == UPD_DEMO3_SET)? 1: 0;
    }
    return_msg(EAR_ERROR, "option not available");
}

CACHE_F_GET_INFO(demo)
{
    info->api         = API_DEMO;
    info->scope       = scope;
    info->granularity = granularity;
    info->devs_count  = devs_count;
}

CACHE_F_READ(demo)
{
    memset(ca, 0, sizeof(cache_t)*devs_count);
    return EAR_SUCCESS;
}

CACHE_F_DATA_DIFF(demo)
{
    ofops_calc_array(&ops[d], (char *) caD, devs_count);
    ofops_calc_array(&ops[2], (void *) caD, devs_count);
    // Calculating the bandwidth
    if (gbs != NULL) {
        *gbs = (double) caD[0].l2.lines_out; //L2 like a victim cache
        *gbs = *gbs * ((double) line_size);
        *gbs = *gbs / ((double) 1E9);
    }
    caD[0].l3.lines_out = 0;
}

CACHE_F_INTERNALS(demo)
{
    sprintf(buffer, "CACHE L1/2/3: loaded RANDOM numbers\n");
}