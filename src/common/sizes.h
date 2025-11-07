/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_SIZES_H
#define EAR_SIZES_H

#include <linux/limits.h>

#define SZ_FILENAME        NAME_MAX + 1
#define SZ_PATH_KERNEL     64
#define SZ_PATH            PATH_MAX
#define SZ_PATH_SHORT      1024
#define SZ_PATH_INCOMPLETE SZ_PATH - (SZ_PATH_SHORT * 2)
// Buffer and name are abstract concepts, use it with caution
#define SZ_BUFFER       PATH_MAX
#define SZ_BUFFER_EXTRA PATH_MAX * 2
#define SZ_NAME_SHORT   128
#define SZ_NAME_MEDIUM  SZ_FILENAME
#define SZ_NAME_LARGE   512

#endif // EAR_SIZES_H