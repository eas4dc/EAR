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

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdlib.h>
#include <common/sizes.h>
#include <common/utils/string.h>
#include <common/output/debug.h>

size_t xsnsize(size_t size)
{
	return size;
}

char *xsnbuffer(char *buffer)
{
    return buffer;
}

int appenv(const char *var, const char *string)
{
	char buffer[SZ_PATH];
	char *p = getenv(var);

	if (p != NULL) {
		sprintf(buffer, "%s:%s", string, p);
	} else {
		sprintf(buffer, "%s", string);
	}
	return setenv(var, buffer, 1);
}

char **strtoa(const char *string, char separator, char ***rows, uint *rows_count)
{
    uint str_length = 0;
    uint char_count = 0;
    char *pstring;
    char **prows;
    uint i;

    if (string == NULL) {
        return NULL;
    }
    if ((str_length = strlen(string)) <= 1) {
        return NULL;
    }
    // Allocating space for the new separated string (and copying)
    pstring = calloc(str_length+1, sizeof(char));
    strcpy(pstring, string);
    // Counting
    for (*rows_count = i = 0; i < strlen(pstring)+1; ++i) {
        debug("%c", pstring[i]);
        if (pstring[i] == separator || pstring[i] == '\0') {
            *rows_count += (char_count > 0);
            char_count = 0;
        } else {
            char_count += 1;
        }
    }
    debug("Rows detected %d", *rows_count);
    if (rows_count == 0) {
        return NULL;
    }
    // Allocating space for the rows
    prows = calloc(*rows_count, sizeof(char *));
    // Setting new references
    for (*rows_count = i = 0; i < str_length+1; ++i) {
        if (pstring[i] == separator || pstring[i] == '\0') {
            if (char_count > 0) {
                prows[*rows_count] = &pstring[i-char_count];
                debug("R%d: %s", *rows_count, prows[*rows_count]);
                // Passing to next row
                *rows_count += 1;
            }
            // Setting end char of the row
            pstring[i] = '\0';
            char_count = 0; 
        } else {
            char_count += 1;
        }
    }
    if (rows != NULL) {
        *rows = prows;
    }
    return prows;
}

void strtoa_free(char **list)
{
    free(list[0]);
    free(list);
}

void *strtoat(const char *string, char separator, void **list, uint *list_count, int id_type)
{
    char **alist = strtoa(string, separator, NULL, list_count);
    void *plist = NULL;
    int i;

    if (alist == NULL) {
        return NULL;
    }
    //
    if (id_type == ID_INT) { plist = calloc(*list_count, sizeof(int)); }
    if (id_type == ID_UINT) { plist = calloc(*list_count, sizeof(uint)); }
    if (id_type == ID_LONG) { plist = calloc(*list_count, sizeof(long)); }
    if (id_type == ID_ULONG) { plist = calloc(*list_count, sizeof(ulong)); }
    if (id_type == ID_LLONG) { plist = calloc(*list_count, sizeof(llong)); }
    if (id_type == ID_ULLONG) { plist = calloc(*list_count, sizeof(ullong)); }
    if (id_type == ID_FLOAT) { plist = calloc(*list_count, sizeof(float)); }
    if (id_type == ID_DOUBLE) { plist = calloc(*list_count, sizeof(double)); }
    //
    for (i = 0; i < *list_count; ++i) {
        if (id_type == ID_INT) { ((int *) plist)[i] = (int) atoi(alist[i]); }
        if (id_type == ID_UINT) { ((uint *) plist)[i] = (uint) atoi(alist[i]); }
        if (id_type == ID_LONG) { ((long *) plist)[i] = (long) strtol(alist[i], NULL, 10); }
        if (id_type == ID_ULONG) { ((ulong *) plist)[i] = (ulong) strtoul(alist[i], NULL, 10); }
        if (id_type == ID_LLONG) { ((llong *) plist)[i] = (llong) strtoll(alist[i], NULL, 10); }
        if (id_type == ID_ULLONG) { ((ullong *) plist)[i] = (ullong) strtoull(alist[i], NULL, 10); }
        if (id_type == ID_FLOAT) { ((float *) plist)[i] = (float) atof(alist[i]); }
        if (id_type == ID_DOUBLE) { ((double *) plist)[i] = (double) atof(alist[i]); }
    }
    // Freeing
    strtoa_free(alist);

    if (list != NULL) {
        list = plist;
    }

    return plist;
}

char **envtoa(const char *var, char ***list, uint *list_count)
{
    return strtoa(getenv(var), ',', list, list_count);
}

void envtoa_free(char **list)
{
    strtoa_free(list);
}

void *envtoat(const char *var, void **list, uint *list_count, int id_type)
{
    return strtoat(getenv(var), ',', list, list_count, id_type);
}
