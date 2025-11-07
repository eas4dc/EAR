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

#include <common/hardware/defines.h>
#include <common/hardware/mrs.h>
#include <common/output/debug.h>
#include <stdlib.h>

#if __ARCH_ARM
#define mrs_asm(reg) __asm__ volatile("mrs %0, " #reg : "=r"(var));
#else
#define mrs_asm(reg)
#endif

static ullong mrsf_zero()
{
    return 0LLU;
}

#define mrs_file(name, path)                                                                                           \
    static ullong mrsf_##name()                                                                                        \
    {                                                                                                                  \
        char buffer[128];                                                                                              \
        FILE *file;                                                                                                    \
        ullong var = 0;                                                                                                \
        if ((file = fopen(path, "r")) == NULL) {                                                                       \
            return 0;                                                                                                  \
        }                                                                                                              \
        fscanf(file, "%s", buffer);                                                                                    \
        var = (ullong) strtoll(&buffer[2], NULL, 16);                                                                  \
        debug("read '%s': %llu", path, var);                                                                           \
        fclose(file);                                                                                                  \
        return var;                                                                                                    \
    }

#define mrs_function(name, reg, fcall)                                                                                 \
    ullong mrs_##name()                                                                                                \
    {                                                                                                                  \
        ullong var;                                                                                                    \
        if ((var = fcall)) {                                                                                           \
            return var;                                                                                                \
        }                                                                                                              \
        mrs_asm(reg) debug("read '%s': %llu", #reg, var);                                                              \
        return var;                                                                                                    \
    }

// File functions are hidden
mrs_file(midr, "/sys/devices/system/cpu/cpu0/regs/identification/midr_el1");
// mrs_file(revidr, "/sys/devices/system/cpu/cpu0/regs/identification/revidr_el1");
//  Introduce new register reading functions as required
mrs_function(midr, MIDR_EL1, mrsf_midr());
mrs_function(cntfrq, CNTFRQ_EL0, mrsf_zero());
mrs_function(dfr0, ID_AA64DFR0_EL1, mrsf_zero());
mrs_function(pmccntr, PMCCNTR_EL0, mrsf_zero());
mrs_function(pmccfiltr, PMCCFILTR_EL0, mrsf_zero());