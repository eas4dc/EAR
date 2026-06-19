/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_ARGS_H
#define COMMON_UTILS_ARGS_H
/* clang-format off */

// Allowed syntax
//	-a
//	-bval
//	-b val
//	--something
//	--something=val
//	--something val
// Suffixes:
//	: required value
// Examples:
//	args_get(argc, argv, "a:", buffer);
//	args_get(argc, argv, "something", buffer);

// Returns NULL if not found, and a pointer to the value of the argument. In
// case is just a flag without value, it returns the pointer to the flag.
char *args_get(int argc, char *argv[], const char *flag_name, char *buffer);

/* clang-format on */
#endif // COMMON_UTILS_ARGS_H