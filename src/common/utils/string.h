/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_STRING_H
#define COMMON_UTILS_STRING_H

#include <stdio.h>
#include <string.h>
#include <common/types.h>

#define unused(var) \
	(void)(var)

#define xsnprintf(buffer, size, ...) \
    snprintf(xsnbuffer(buffer), xsnsize(size), __VA_ARGS__);

#define xsprintf(buffer, ...) \
    snprintf(xsnbuffer(buffer), xsnsize(sizeof(buffer)), __VA_ARGS__); \

#define xstrncpy(dst, src, size) \
    strncpy(dst, src, xsnsize(size));

#define xstrncat(dst, src, size) \
	strncat(dst, src, xsnsize(size));

// The xs functions and macros are used to avoid compile warnings.
size_t xsnsize(size_t size);

char *xsnbuffer(char *buffer);
// To return a string, it has a static buffer inside using TLS
char *rsprintf(char *format, ...);

// Given a string and a separator, returns a NULL terminated list of separated string elements.
char **strtoa(const char *string, char separator, char ***list, uint *list_count);
// Free values allocated by strtoa
void strtoa_free(char **list);
// Same strtoa behaviour but for lists of specific type. Check common/types.h and remember to free.
void *strtoat(const char *string, char separator, void **list, uint *list_count, int id_type);
// Appends a string to the environment variable 'var'.
int appenv(const char *var, const char *string);
// Given an envar in comma separated list format, allocates and returns a list of each element.
char **envtoa(const char *var, char ***list, uint *list_count);
// Free values allocated by envtoa
void envtoa_free(char **list);
// Same envtoa behaviour but for lists of specific type. Check common/types.h and remember to free.
void *envtoat(const char *var, void **list, uint *list_count, int id_type);

// Environment variable to ullong list example:
//    int list_count;
//    ullong *list;
//
//    if ((list = (ullong *) envtoat("PRUEBA", NULL, &list_count, ID_ULLONG)) == NULL) {
//        return 0;
//    }

// To parse a rank enumeration like CPUPOWER ('0', '0,1,2', '0-2,4', etc). Returns an array of 1s and 0s.
int rantoa(char *string, uint *array, uint array_length);
// Converts a string to ullong. Is base 10 or base 16 if starts with '0x'.
ullong atoull(const char *str);
//
int strisnum(char *string);

#endif
