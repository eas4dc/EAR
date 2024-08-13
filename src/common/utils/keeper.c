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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/system/file.h>
#include <common/output/debug.h>
#include <common/utils/keeper.h>
#include <common/config/config_env.h>

#define BUFFER_SIZE 2048

static char  ids[128][128];
static char  values[128][BUFFER_SIZE];
static int   lines_count;
static int   fd = -1;

static int keeper_read_line(int fd, char *id, char *value)
{
    ssize_t size;
    int i = 0;

    while(1) {
        size = read(fd, &id[i], 1);
        debug("%c", id[i]);
        if (size  <=  0)   return -1;
        if (id[i] == '\n') return  0;
        if (id[i] == EOF ) return -1;
        if (id[i] == '=' ) break;
        ++i;
    }
    id[i] = '\0';
    debug("going to value")
    i = 0;
    while(1) {
        size = read(fd, &value[i], 1);
        debug("%c", value[i]);
        if (size     ==  0)   return  2;
        if (size     <= -1)   return -1;
        if (value[i] == '\n') break;
        if (value[i] == EOF)  return  2;
        if (value[i] == '=')  return  0;
        ++i;
    }
    value[i] = '\0';

    return 1;
}

static void keeper_open()
{
    char path[256];
    char *tmp;

    // Open file if not previously opened
    if ((tmp = ear_getenv(ENV_PATH_TMP)) == NULL) {
        tmp = "/tmp";
    }
    if (fd >= 0) {
        return;
    }
    sprintf(path, "%s/ear-dyn.conf", tmp);
    // If we are root, we change the user and group of the keeper file.
    if (getuid() == 0) {
        chown(path, getuid(), getgid());
    }
    debug("Opening %s", path);
    if ((fd = open(path, O_RDWR | O_CREAT, PERMS(111,100,100))) < 0) {
        debug("Failed: %s", strerror(errno));
        return;
    }
    while(1) {
        int res = keeper_read_line(fd, ids[lines_count], values[lines_count]);
        debug("Returned %d", res);
        lines_count += (res > 0);
        if (res ==  2) break;
        if (res == -1) break;
    }
#if SHOW_DEBUGS
    int i = 0;
    while(i < lines_count) {
        debug("Read %s = '%s'", ids[i], values[i]);
        ++i;
    }
#endif
}

static void super_write(int fd, char *buffer)
{
    size_t wpend = strlen(buffer);
    size_t wtotal = 0;
    size_t wstep = 0;

    while ((wpend > 0) && ((wstep = write(fd, &buffer[wtotal], wpend)) > 0)) {
        wpend = wpend - wstep;
        wtotal += wstep;
    }
}

static void keeper_save()
{
    char buffer[512];
    int i = 0;

    if (fd < 0) {
        return;
    }
    // Position 0 of the file
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);

    while(i < lines_count) {
        xsprintf(buffer, "%s=%s\n", ids[i], values[i]);
        super_write(fd, buffer);
        ++i;
    }
}

static int keeper_is_saved(const char *id, int *index)
{
    int i;
    for (i = 0; i < lines_count; ++i) {
        debug("Comparing ids %s vs %s", id, ids[i]);
        if (strcmp(ids[i], id) == 0) {
            if (index != NULL) {
                *index = i;
            }
            debug("%s id is found in position %d (%s)", id, *index, ids[*index]);
            return 1;
        }
    }
    debug("%s id is not found", id);
    return 0;
}

static int keeper_is_different(const char *id, char *value)
{
    int i;

    if (!keeper_is_saved(id, &i)) {
        return 0;
    }
    if (strcmp(values[i], value) == 0) {
        debug("'%s' and '%s' are equal");
        return 0;
    }
    debug("'%s' and '%s' are different");
    return 1;
}

void keeper_save_string(const char *id, char *value)
{
    int i;

    debug("Saving %s = %s", id, value);
    if (keeper_is_saved(id, &i)) {
        debug("%s is previously saved", id);
        if (keeper_is_different(id, value)) {
            debug("%s is different than %s", values[i], value);
            strcpy(values[i], value);
        } else {
            debug("%s is equal than %s", values[i], value);
            return;
        }
    } else {
        debug("%s is not previously saved", id);
        strcpy(ids[lines_count], id);
        strcpy(values[lines_count], value);
        ++lines_count;
    }
    keeper_save();
}

#define KEEPER_SAVE(suffix, type, line_cast) \
void keeper_save_##suffix (const char *id, type value) \
{ \
    char buffer[BUFFER_SIZE]; \
    keeper_open(); \
    line_cast; \
    keeper_save_string(id, buffer); \
}

#define KEEPER_LOAD(suffix, type, line_cast) \
int keeper_load_##suffix (const char *id, type value) \
{ \
    int i; \
    keeper_open(); \
    for (i = 0; i < lines_count; ++i) { \
        if (strcmp(ids[i], id) == 0) { \
            line_cast; \
            return 1; \
        } \
    } \
    return 0; \
}

#define KEEPER_SAVE_A(suffix, type, line_cast) \
void keeper_save_a##suffix (const char *id, type *list, uint list_length) \
{ \
    char buffer[BUFFER_SIZE]; \
    uint i; \
    int a; \
    keeper_open(); \
    for (i = a = 0; i < list_length; ++i) { \
        a += line_cast; \
    } \
    buffer[a-1] = '\0'; \
    keeper_save_text(id, buffer); \
}

#define KEEPER_LOAD_A(suffix, type, line_cast) \
int keeper_load_a##suffix (const char *id, type **list, uint *list_length) \
{ \
    char buffer[BUFFER_SIZE]; \
    char **lines; \
    int i; \
    keeper_open(); \
    if (!keeper_load_text(id, buffer)) { \
        return 0; \
    } \
    if ((lines = strtoa(buffer, ',', NULL, list_length)) == NULL) { \
        return 0; \
    } \
    *list = calloc(*list_length, sizeof(type)); \
    for (i = 0; i < *list_length; ++i) { \
        (*list)[i] = (type) line_cast; \
    } \
    strtoa_free(lines); \
    return 1; \
}

KEEPER_SAVE  (   text,   char *, strcpy(buffer, value));
KEEPER_SAVE  (  int32,    int  , sprintf(buffer, "%d", value));
KEEPER_SAVE  (  int64,  llong  , sprintf(buffer, "%lld", value));
KEEPER_SAVE  ( uint32,   uint  , sprintf(buffer, "%u", value));
KEEPER_SAVE  ( uint64, ullong  , sprintf(buffer, "%llu", value));
KEEPER_SAVE  (float32,  float  , sprintf(buffer, "%f", value));
KEEPER_SAVE  (float64, double  , sprintf(buffer, "%lf", value));
KEEPER_SAVE_A( uint32,   uint  , sprintf(&buffer[a], "%u,", list[i]));
KEEPER_LOAD  (   text,   char *, strcpy(value, values[i]));
KEEPER_LOAD  (  int32,    int *, *value = atoi(values[i]));
KEEPER_LOAD  (  int64,  llong *, *value = atoll(values[i]));
KEEPER_LOAD  ( uint32,   uint *, *value = (uint) atoi(values[i]));
KEEPER_LOAD  ( uint64, ullong *, *value = strtoull(values[i], NULL, 10));
KEEPER_LOAD  (float32,  float *, *value = (float) atof(values[i]));
KEEPER_LOAD  (float64, double *, *value = atof(values[i]));
KEEPER_LOAD_A( uint32,   uint  , atoi(lines[i]));

#if TEST
int main(int argc, char *argv)
{
    int x;

    #if 0
    keeper_save_text("AmdMaxImcFreq", "2400000");
    keeper_save_text("NumGpus", "2");
    keeper_load_int32("NumGpus", &x);
    debug("X = %d", x);
    keeper_save_text("NumGpus", "4");
    #endif
    uint array[4] = { 1,2,3,4 };
    uint *narray;
    uint narray_count;

    keeper_save_auint32("lol", array, 4);
    keeper_load_auint32("lol", &narray, &narray_count);
    printf("%u: %u\n", 0, narray[0]);
    printf("%u: %u\n", 1, narray[1]);
    printf("%u: %u\n", 2, narray[2]);
    printf("%u: %u\n", 3, narray[3]);
    printf("T%u", narray_count);

    return 0;
}
#endif
