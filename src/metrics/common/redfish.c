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

#ifdef REDFISH_BASE
#include <redfish.h>
#include <redfishService.h>
#endif

#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/environment_common.h>
#include <metrics/common/redfish.h>

typedef const char cchar;

#ifndef REDFISH_BASE
typedef int redfishService;
typedef int redfishPayload;
typedef struct enumeratorAuthentication_s {
    uint authType;
    union {
        struct {
            char* username;
            char* password;
        } userPass;
    } authCodes;
} enumeratorAuthentication;
#define REDFISH_AUTH_BASIC 0
#define REDFISH_PATH ""
#else
#define REDFISH_PATH REDFISH_BASE
#endif

#define REDFISH_N 11

static struct redfish_s
{
    redfishService *(*createService)    (cchar *host, cchar *uri, enumeratorAuthentication* auth, uint flags);
    redfishPayload *(*getPayload)       (redfishService* service, cchar *path);
    char *          (*payloadToJson)    (redfishPayload* payload, int prettyPrint);
    char *          (*payloadToString)  (redfishPayload* payload);
    int             (*payloadToInt)     (redfishPayload* payload);
    llong           (*payloadToLlong)   (redfishPayload* payload);
    double          (*payloadToDouble)  (redfishPayload* payload, int *isDouble);
    size_t          (*getCollectionSize)(redfishPayload* payload);
    size_t          (*getArraySize)     (redfishPayload* payload);
    void            (*cleanupPayload)   (redfishPayload* payload);
    void            (*cleanupService)   (redfishService* service);
} redfish;

static const char *redfish_names[] =
{
    "createServiceEnumerator",
    "getPayloadByPath",
    "payloadToString",
    "getPayloadStringValue",
    "getPayloadIntValue",
    "getPayloadLongLongValue",
    "getPayloadDoubleValue",
    "getCollectionSize",
    "getArraySize",
    "cleanupPayload",
    "cleanupServiceEnumerator",
};

static pthread_mutex_t          lock = PTHREAD_MUTEX_INITIALIZER;
static redfishService          *service;
static enumeratorAuthentication auth;
static uint                     loaded;

static int load(char *path)
{
    int flags = RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND;
    debug("loading: %s", path);
    if (state_fail(plug_open(path, (void **) &redfish, redfish_names, REDFISH_N, flags))) {
        debug("failed while loading %s: %s", path, state_msg);
        return 0;
    }
    return 1;
}

static state_t static_load()
{
    state_t s = EAR_SUCCESS;
    if (load(ear_getenv("HACK_FILE_REDFISH"))) return s;
    if (load(REDFISH_PATH "/lib64/libredfish.so")) return s;
    if (load(REDFISH_PATH "/lib/libredfish.so")) return s;
    return_msg(EAR_ERROR, "Can't load libredfish.so");
}

static state_t static_init(char *host, char *user, char *pass)
{
    enumeratorAuthentication *pauth = NULL;

    if (pass != NULL) {
        auth.authType = REDFISH_AUTH_BASIC;
        auth.authCodes.userPass.username = user;
        auth.authCodes.userPass.password = pass;
        pauth = &auth;
    }
    // host, uri, auth, flags
    // uri, if null is /redfish
    service = redfish.createService(host, NULL, pauth, 1);
    
    return EAR_SUCCESS;
}

state_t redfish_open(char *host, char *user, char *pass)
{
    state_t s = EAR_SUCCESS;
    #ifndef REDFISH_BASE
    return_msg(EAR_ERROR, "path to libredfish.so installation not set");
    #endif
    if (host == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    }
    if (loaded) {
        return s;
    }
    while (pthread_mutex_trylock(&lock));
    if (!loaded) {
        if (state_ok(s = static_load())) {
            if (state_ok(s = static_init(host, user, pass))) {
                loaded = 1;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    return s;
}

state_t redfish_dispose()
{
    redfish.cleanupService(service);
    return EAR_SUCCESS;
}

//"/Chassis[%d]/Sensors[Id=TotalPower]/Reading"
//"/Chassis/Members[%d]/Sensors[Id=TotalPower]/Reading (int)

static int count_iterators(char *field)
{
    uint count = 0;
    int i;
    for(i = 0; i < strlen(field); ++i) {
        count += (field[i] == '%');
    }
    debug("Interators: %d", count)
    return count;
}

static state_t read_single(char *field, void *content, void *accum, uint *count, int type_flag)
{
    // Gets the content, if repeat, returns the next element
    redfishPayload *payload = redfish.getPayload(service, field);

    debug("Looking in field: %s", field);
    // If no content found, return
    if (payload == NULL) {
        redfish.cleanupPayload(payload);
        return_msg(EAR_ERROR, "No content.");
    }
    // If counter is not NULL, adding one
    if (count != NULL) {
        *count += 1;
    }
    // If content is string we don't have interest in accumulate values
    if (type_flag == REDTYPE_JSON) {
        #if SHOW_DEBUGS
        debug("JSON:\n%s", redfish.payloadToJson(payload, 1));
        #else
        if (content != NULL) {
            char *p = (char *) content;
            char *s = redfish.payloadToJson(payload, 1);
            if (s != NULL) {
                sprintf(p, "%s", s);
            }
        }
        #endif
        goto leave;
    }
    if (type_flag == REDTYPE_STRING) {
        if (content != NULL) {
            char *p = (char *) content;
            char *s = redfish.payloadToString(payload);
            if (s != NULL) {
                sprintf(p, "%s", s);
            }
            debug("STR: %s", s);
        }
        goto leave;
    }
    // If content is a number, we return the conetnt in int format and the cumulative
    if (type_flag == REDTYPE_INT) {
        int v = redfish.payloadToInt(payload);
        if (content != NULL) {
            int *p = (int *) content;
            *p = v;
            debug("I32v: %d", *p);
        }
        if (accum != NULL) {
            int *p = (int *) accum;
            *p += v;
            debug("I32a: %d", *p);
        }
    }
    if (type_flag == REDTYPE_LLONG) {
        llong v = redfish.payloadToLlong(payload);
        if (content != NULL) {
            llong *p = (llong *) content;
            *p = v;
            debug("I64v: %lld", *p);
        }
        if (accum != NULL) {
            llong *p = (llong *) accum;
            *p += v;
            debug("I64a: %lld", *p);
        }
    }
    if (type_flag == REDTYPE_DOUBLE) {
        int is_double;
        double v = redfish.payloadToDouble(payload, &is_double);
        if (content != NULL) {
            double *p = (double *) content;
            *p = v;
            debug("F64v: %lf", *p);
        }
        if (accum != NULL) {
            double *p = (double *) accum;
            *p += v;
            debug("F64a: %lf", *p);
        }
    }
leave:
    redfish.cleanupPayload(payload);
    return EAR_SUCCESS;
}

static state_t read_multiple(char *field, void *array, void *accum, uint *count, int type_flag)
{
    char *p = (char *) array;
    char buffer[4096];
    int c1, c2, i1, i2;
    int bytes = 4;
    state_t s;

    if (count != NULL) {
        *count = 0;
    }
    // Iterator is a '%d' in the field argument. It serves to access to multiple
    // indexes at the same time and perform an accumulation. We assume if the
    // field is a type of string, there are no iterators.
    // This functions is compatible with up to 2 iterators.
    // 
    // c1: counts the '%d' occurrences
    // c2: counts the number of read values
 
    // Extending the arrays to 64 bits
    if (type_flag == REDTYPE_LLONG || type_flag == REDTYPE_DOUBLE) {
        bytes = 8;
    }
    if (accum != NULL) {
        memset(accum, 0, (size_t) bytes);
    }
    // Asking for string or found 0 iterators
    if ((c1 = count_iterators(field)) == 0) {
        return read_single(field, array, accum, count, type_flag);
    }
    // i1: exterior iterator
    // i2: interior iterator
    i1 = -1;
    c2 =  0;

    if (c1 == 1) {
        do {
            ++i1;
            sprintf(buffer, field, i1);
            s = read_single(buffer, &p[c2*bytes], accum, count, type_flag);
            c2++;
        // if index is 0 and fails, maybe it is because the array starts at 1.
        } while(state_ok(s) || (i1 == 0));
    }
    if (c1 == 2) {
        do {
            ++i1;
            i2 = -1;
            do {
                ++i2;
                sprintf(buffer, field, i1, i2);
                s = read_single(buffer, &p[c2*bytes], accum, count, type_flag);
                c2++;
            } while(state_ok(s) || (i2 == 0));
        } while((i1 == 0) || i2 > 1);
    }
    if (c2 == 1) {
        return_msg(EAR_ERROR, "Nothing found with that path.")
    }
    return EAR_SUCCESS;
}

state_t redfish_read(char *field, void *content, void *accum, uint *count, int type_flag)
{
    return read_multiple(field, content, accum, count, type_flag);
}

state_t redfish_count_members(char *field, uint *count)
{
    redfishPayload* payload = redfish.getPayload(service, field);
    if (payload == NULL) {
        redfish.cleanupPayload(payload);
        return_msg(EAR_ERROR, "No content.");
    }
    size_t size = redfish.getArraySize(payload);
    *count = (uint) size;
    debug("Counter: %u", *count);
    redfish.cleanupPayload(payload);
    return EAR_SUCCESS;
}
