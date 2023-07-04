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

#include <stdlib.h>
#include <common/config/config_env.h>
#include <common/environment_common.h>
#include <common/hardware/topology_tdp.h>

#define tdpb_impl(comp, v, m, t) \
    if (topo->vendor == v && comp == m) { \
        topo->tdp = t; \
        return; \
    } 
#define tdpbi_impl(v, m, t) tdpb_impl(topo->model, v, m, t)
#define tdpba_impl(v, m, t) tdpb_impl(topo->family, v, m, t)

#define tdpei_impl(v, m, b, t) \
    if (topo->vendor == v && topo->model == m && !strcmp(topo->brand, b)) { \
        topo->tdp = t; \
        return; \
    } 

// b = base, e = specific
// i = intel, a = amd
#define unpack(macro, args) macro args
#define tdpbi(...) unpack(tdpbi_impl, (__VA_ARGS__))
#define tdpba(...) unpack(tdpba_impl, (__VA_ARGS__))
#define tdpei(...) unpack(tdpei_impl, (__VA_ARGS__))

void topology_tdp(topology_t *topo)
{
    topo->tdp = 200;

    // TDP custom
    if (ear_getenv(FLAG_CPU_TDP) != NULL) {
        topo->tdp = (uint) atoi(ear_getenv(FLAG_CPU_TDP));
        return;
    }
    // TDP specific
    tdpei(TDP_BROADWELL_E5_2686_V4);
    tdpei(TDP_SKYLAKE_PLATINUM_8175M);
    // TDP base
    tdpbi(TDP_HASWELL);
    tdpbi(TDP_BROADWELL);
    tdpbi(TDP_SKYLAKE);
    tdpbi(TDP_ICELAKE);
    tdpba(TDP_ZEN);
    tdpba(TDP_ZEN3);
}
