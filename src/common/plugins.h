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

#ifndef COMMON_PLUGINS_H
#define COMMON_PLUGINS_H

#include <common/states.h>
#include <metrics/common/apis.h>

// Attributes
#define attr2(a1, a2)    __attribute__ ((a1, a2))
#define attr(a)          __attribute__ ((a))
#define attr_hidden      visibility("hidden")
#define attr_protected   visibility("protected")
#define attr_internal    visibility("internal")
#define attr_weak        weak
// Verbosity parameters
#define none	0
#define empty	NULL
#define no_ctx  NULL

typedef struct ctx_s {
	void *context;
	size_t size;
} ctx_t;

#define preturn(call, ...) \
	if (call == NULL) { \
		return_msg(EAR_UNDEFINED, Generr.api_undefined); \
	} \
	return call (__VA_ARGS__);

#define preturn_opt(call, ...) \
  if (call == NULL) { \
    return EAR_SUCCESS; \
  } \
  return call (__VA_ARGS__);


#endif //EAR_PRIVATE_PLUGINS_H
