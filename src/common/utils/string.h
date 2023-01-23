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
// Given a string and a separator, allocates the space and returns the number of elements
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

#endif