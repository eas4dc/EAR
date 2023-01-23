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

#ifndef COMMON_HARDWARE_TOPOLOGY_TDP_H
#define COMMON_HARDWARE_TOPOLOGY_TDP_H

#include <common/hardware/topology.h>

// TDP base list
#define TDP_HASWELL                 VENDOR_INTEL, MODEL_HASWELL_X  , 100
#define TDP_BROADWELL               VENDOR_INTEL, MODEL_BROADWELL_X, 100
#define TDP_SKYLAKE                 VENDOR_INTEL, MODEL_SKYLAKE_X  , 150
#define TDP_ICELAKE                 VENDOR_INTEL, MODEL_ICELAKE_X  , 175
#define TDP_ZEN                     VENDOR_AMD  , FAMILY_ZEN       , 150
#define TDP_ZEN3                    VENDOR_AMD  , FAMILY_ZEN3      , 200

// TDP specific list
#define TDP_BROADWELL_E5_2686_V4    VENDOR_INTEL, MODEL_BROADWELL_X, "E5-2686 v4"    , 145
#define TDP_SKYLAKE_PLATINUM_8175M  VENDOR_INTEL, MODEL_SKYLAKE_X  , "Platinum 8175M", 125

#endif
