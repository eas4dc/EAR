/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_SYSTEM_VERSION_H
#define COMMON_SYSTEM_VERSION_H

#include <common/types.h>

typedef struct version_s {
    char  str[32];
    uint  minor;
    uint  major;
    uint  hash;
} version_t;

#define VERSION_GT 0 //Greater-than
#define VERSION_GE 1 //Greater-or-equal
#define VERSION_LT 2 //Less-than
#define VERSION_LE 3 //Less-or-equal
#define VERSION_EQ 4 //Equal

// Initializes the version structure
void version_set(version_t *v, uint major, uint minor);

// Useful to compare between versions (ie: VERSION_GT means v > vtc)
int version_is(int symbol, version_t *v, version_t *vtc);

#endif //COMMON_SYSTEM_VERSION_H