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

#if defined(__aarch64__) || defined(_M_ARM64)
#define __ARCH_ARM   1
#define __ARCH_ARM64 1
#else
#define __ARCH_X86   1
#define __ARCH_X8664 1
#endif

#endif //HARDWARE_DEFINES_H
