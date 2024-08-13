/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
// You can check it by "gcc -march=native -dM -E - < /dev/null"

#if defined(ARCH_ARM) || defined(ARCH_ARM64) || defined(__ARM_ARCH)
#define __ARCH_ARM 1
#else
#define __ARCH_X86 1
#endif

// Interesting features:
// __ARM_FEATURE_SVE
// __ARM_FEATURE_SVE_BITS
#if defined(__AVX512F__)
#define __X86_FEATURE_AVX512
#endif

#endif //HARDWARE_DEFINES_H
