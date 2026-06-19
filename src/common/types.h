/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H
/* clang-format off */

#include <common/types/generic.h>

// Types enumeration
enum id_types {
    ID_NULL,
    ID_INT,
    ID_UINT,
    ID_INT32 = ID_INT,
    ID_UINT32 = ID_UINT,
    ID_LONG,
    ID_ULONG,
    ID_LLONG,
    ID_ULLONG,
    ID_INT64 = ID_LLONG,
    ID_UINT64 = ID_ULLONG,
    ID_FLOAT,
    ID_DOUBLE,
};

/* clang-format on */
#endif // COMMON_TYPES_H
