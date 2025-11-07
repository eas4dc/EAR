/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_TYPES_GENERIC
#define COMMON_TYPES_GENERIC

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

//
typedef const char cchar;
typedef unsigned char uchar;
typedef signed char byte;
typedef unsigned char ubyte;
typedef unsigned long long ull;
typedef unsigned long long ullong;
typedef signed long long llong;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long long addr_t;

// Attributes
#define ATTR_UNUSED __attribute__((unused))

// Not generic (in process of cleaning)
typedef uint8_t job_type;
typedef ulong job_id;

#define GENERIC_NAME       256
#define SHORT_GENERIC_NAME 16
#define POLICY_NAME        32
#define ENERGY_TAG_SIZE    32
#define MAX_PATH_SIZE      256
#define NODE_SIZE          256
#define ID_SIZE            64
#define USER               64

#define ear_min(X, Y)      (((X) < (Y)) ? (X) : (Y))
#define ear_max(X, Y)      (((X) > (Y)) ? (X) : (Y))

#endif // COMMON_TYPES_GENERIC
