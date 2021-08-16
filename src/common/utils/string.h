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

#ifndef COMMON_UTILS_STRING_H
#define COMMON_UTILS_STRING_H

#include <stdio.h>
#include <string.h>

#define unused(var) \
	(void)(var)

#define xsnprintf(buffer, size, ...) \
    snprintf(buffer, xsnsize(size), __VA_ARGS__);

#define xsprintf(buffer, ...) \
    snprintf(buffer, xsnsize(sizeof(buffer)), __VA_ARGS__);

#define xstrncpy(dst, src, size) \
    strncpy(dst, src, xsnsize(size));

#define xstrncat(dst, src, size) \
	strncat(dst, src, xsnsize(size));

size_t xsnsize(size_t size);

#endif
