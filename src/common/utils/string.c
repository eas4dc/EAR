/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <common/sizes.h>
#include <common/utils/string.h>
#include <common/output/debug.h>
#include <common/environment_common.h>

size_t xsnsize(size_t size)
{
	return size;
}

char *xsnbuffer(char *buffer)
{
    return buffer;
}

char *rsprintf(char *format, ...)
{
    static __thread char buffer[8192];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    return buffer;
}

int appenv(const char *var, const char *string)
{
	char buffer[SZ_PATH];
	char *p = ear_getenv(var);

	if (p != NULL) {
		sprintf(buffer, "%s:%s", string, p);
	} else {
		sprintf(buffer, "%s", string);
	}
	return setenv(var, buffer, 1);
}

char **strtoa(const char *string, char separator, char ***rows, uint *rows_count)
{
    uint   str_length = 0;
    uint   char_count = 0;
    char  *pstring;
    uint   prows_count;
    char **prows;
    uint   i;

    debug("parsing string %s", string);
    if (rows_count != NULL) {
        *rows_count = 0;
    }
    if (string == NULL) {
        debug("string is NULL");
        return NULL;
    }
    if ((str_length = strlen(string)) < 1) {
        debug("string is too short");
        return NULL;
    }
    if (strlen(string) == 1 && string[0] == separator) {
        debug("string is just a separator");
        return NULL;
    }
    // Allocating space for the new separated string (and copying)
    pstring = calloc(str_length+1, sizeof(char));
    strcpy(pstring, string);
    // Counting
    for (prows_count = i = 0; i < strlen(pstring)+1; ++i) {
        debug("%c", pstring[i]);
        if (pstring[i] == separator || pstring[i] == '\0') {
            prows_count += (char_count > 0);
            char_count = 0;
        } else {
            char_count += 1;
        }
    }
    debug("Rows detected %d", prows_count);
    if (prows_count == 0) {
        free(pstring);
        return NULL;
    }
    // Allocating space for the rows (plus additional NULL terminated and pstring original address)
    prows    = calloc(prows_count+2, sizeof(char *));
    prows[0] = pstring;
    prows    = prows+1;
    // Setting new references
    for (prows_count = char_count = i = 0; i < str_length+1; ++i) {
        if (pstring[i] == separator || pstring[i] == '\0') {
            if (char_count > 0) {
                prows[prows_count] = &pstring[i-char_count];
                debug("R%d: %s", prows_count, prows[prows_count]);
                // Passing to next row
                prows_count += 1;
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
    if (rows_count != NULL) {
        debug("Returning rows %u", prows_count);
        *rows_count = prows_count;
    }
    return prows;
}

void strtoa_free(char **list)
{
    free(list[-1]);
    free(list-1);
}

void *strtoat(const char *string, char separator, void **list, uint *list_count, int id_type)
{
    char **alist = NULL;
    void *plist = NULL;
    uint plist_count;
    int i;

    if ((alist = strtoa(string, separator, NULL, &plist_count)) == NULL) {
        return NULL;
    }
    // We are adding
    if (id_type == ID_INT)    { plist = calloc(plist_count, sizeof(int));    }
    if (id_type == ID_UINT)   { plist = calloc(plist_count, sizeof(uint));   }
    if (id_type == ID_LONG)   { plist = calloc(plist_count, sizeof(long));   }
    if (id_type == ID_ULONG)  { plist = calloc(plist_count, sizeof(ulong));  }
    if (id_type == ID_LLONG)  { plist = calloc(plist_count, sizeof(llong));  }
    if (id_type == ID_ULLONG) { plist = calloc(plist_count, sizeof(ullong)); }
    if (id_type == ID_FLOAT)  { plist = calloc(plist_count, sizeof(float));  }
    if (id_type == ID_DOUBLE) { plist = calloc(plist_count, sizeof(double)); }
    // Its a NULL terminated list
    for (i = 0; alist[i] != NULL; ++i) {
        if (id_type == ID_INT)    { ((int *)    plist)[i] = (int) atoi(alist[i]);                  }
        if (id_type == ID_UINT)   { ((uint *)   plist)[i] = (uint) atoi(alist[i]);                 }
        if (id_type == ID_LONG)   { ((long *)   plist)[i] = (long) strtol(alist[i], NULL, 10);     }
        if (id_type == ID_ULONG)  { ((ulong *)  plist)[i] = (ulong) strtoul(alist[i], NULL, 10);   }
        if (id_type == ID_LLONG)  { ((llong *)  plist)[i] = (llong) strtoll(alist[i], NULL, 10);   }
        if (id_type == ID_ULLONG) { ((ullong *) plist)[i] = (ullong) strtoull(alist[i], NULL, 10); }
        if (id_type == ID_FLOAT)  { ((float *)  plist)[i] = (float) atof(alist[i]);                }
        if (id_type == ID_DOUBLE) { ((double *) plist)[i] = (double) atof(alist[i]);               }
    }
    // Freeing
    strtoa_free(alist);

    if (list != NULL) {
        list = plist;
    }
    if (list_count != NULL) {
        *list_count = plist_count;
    }

    return plist;
}

char **envtoa(const char *var, char ***list, uint *list_count)
{
    return strtoa(ear_getenv(var), ',', list, list_count);
}

void envtoa_free(char **list)
{
    strtoa_free(list);
}

void *envtoat(const char *var, void **list, uint *list_count, int id_type)
{
    return strtoat(ear_getenv(var), ',', list, list_count, id_type);
}


int rantoa(char *string, uint *array, uint array_length)
{
    uint list_count;
    int a, b, i;
    char **list;
    char *c;

    if (string == NULL) {
        return 0;
    }
    if (strtoa(string, ',', &list, &list_count) == NULL) {
        if ((a = atoi(string)) >= array_length) {
            return 0;
        }
        array[a] = 1;
        return 0;
    }
    for (i = 0; i < list_count; ++i) {
        if ((c = strchr(list[i], '-')) != NULL) {
            c[0] = '\0';
            a = atoi(list[i]);
            b = atoi(&c[1]);
            while(a <= b) {
                array[a] = 1;
                ++a;
            }
            c[0] = '-';
        } else {
            array[atoi(list[i])] = 1;
        }
    }
    strtoa_free(list);
    return 1;
}

ullong atoull(const char *str)
{
    int base = 10;
    if (str != NULL) {
        if (strchr(str, 'X') != NULL || strchr(str, 'x') != NULL) {
            base = 16;
        }
        return strtoull(str, NULL, base);
    }
    return 0LLU;
}

int strisnum(char *string)
{
    while (*string != '\0') {
             if (*string == '0') {}
        else if (*string == '1') {}
        else if (*string == '2') {}
        else if (*string == '3') {}
        else if (*string == '4') {}
        else if (*string == '5') {}
        else if (*string == '6') {}
        else if (*string == '7') {}
        else if (*string == '8') {}
        else if (*string == '9') {}
        else return 0;
        ++string;
    }
    return 1;
}
