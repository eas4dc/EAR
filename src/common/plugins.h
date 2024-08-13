/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
//#define none	0
#define empty	NULL

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
