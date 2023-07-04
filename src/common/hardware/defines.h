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

#ifndef HARDWARE_DEFINES_H
#define HARDWARE_DEFINES_H

// Interesting compiler defines:
//  __x86_64
//  __AVX2__
//  __AVX512F__
//  ARCH_ARM
//  ARCH_ARM64
//  __ARM_ARCH
//  __ARM_ARCH_8A
//  __ARM_FEATURE_SVE
// You can check it by "gcc -march=native -dM -E - < /dev/null"

#if defined(ARCH_ARM) || defined(ARCH_ARM64) || defined(__ARM_ARCH)
#define __ARCH_ARM 1
#else
#define __ARCH_X86 1
#endif

#if defined(__AVX512F__)
#define __X86_FEATURE_AVX512
#endif

#endif //HARDWARE_DEFINES_H
