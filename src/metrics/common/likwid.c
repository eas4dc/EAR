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

#ifdef LIKWID_BASE
#include <likwid.h>
#endif

#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/types.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/environment_common.h>
#include <metrics/common/likwid.h>

// Likwid opens a daemon to read performance counters. But there is a compiling option
// to prevents that to directly open the perfmon counters. Take a look to config.mk
// file and look for this line (ACCESSMODE = accessdaemon).
//
// In version 5.2.1, we fixed a segmentation fault in an installation with ACCESSMODE
// 'direct' by setting 'perf_event'. After that, the problem was fixed even in the
// 'direct' binaries. So it seems to be a bug in LIKWID that should be investigated.

#ifndef LIKWID_BASE
typedef struct CpuInfo_s {
	char *name;
} CpuInfo_t;

typedef struct ThreadPool_s {
	uint apicId;
} ThreadPool_t;

typedef struct CpuTopology_s {
	uint numHWThreads;
	ThreadPool_t *threadPool;
} *CpuTopology_t;

#define LIKWID_PATH ""
#else
#define LIKWID_PATH LIKWID_BASE
#endif

static const char *likwid_names[] =
{
	"get_cpuInfo",
	"get_cpuTopology",
	"topology_init",
	"topology_finalize",
	"perfmon_init",
	"perfmon_addEventSet",
	"perfmon_setupCounters",
	"perfmon_startCounters",
	"perfmon_getLastResult",
	"perfmon_readCounters",
	"perfmon_readCountersCpu",
	"perfmon_finalize",
    "perfmon_setVerbosity",
};

static struct likwid_s
{
	CpuInfo_t     (*get_cpuInfo)             (void);
	CpuTopology_t (*get_cpuTopology)         (void);
	int           (*topology_init)           (void);
	void          (*topology_finalize)       (void);
	int           (*perfmon_init)            (int thread_count, const int *threads);
	int           (*perfmon_addEventSet)     (const char *events);
	int           (*perfmon_setupCounters)   (int gid);
	int           (*perfmon_startCounters)   (void);
	double        (*perfmon_getLastResult)   (int gid, int eid, int tid);
	int           (*perfmon_readCounters)    (void);
	int           (*perfmon_readCountersCpu) (int cpu);
	void          (*perfmon_finalize)        (void);
	void          (*perfmon_setVerbosity)    (int verbose);
} likwid;

#define LIKWID_N 13

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static CpuTopology_t topo;
static CpuInfo_t     info;
static int          *cpus;
static uint          ok;

static int load_test(char *path)
{
	void **p = (void **) &likwid;
	void *liblikwid;
	int error;
	int i;

	if (path == NULL) {
		return 0;
	}
	debug("trying to access to '%s'", path);
	if (access(path, X_OK) != 0) {
		return 0;
	}
	if ((liblikwid = dlopen(path, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND)) == NULL) {
		debug("dlopen fail (%s)", strerror(errno));
		return 0;
	}
	debug("dlopen ok");
	// Finding symbols
	plug_join(liblikwid, (void **) &likwid, likwid_names, LIKWID_N);

	for(i = 0, error = 0; i < LIKWID_N; ++i) {
		debug("symbol %s: %d", likwid_names[i], (p[i] != NULL));
		error += (p[i] == NULL);
	}
	if (error > 0) {
		memset((void *) &likwid, 0, sizeof(likwid));
		dlclose(liblikwid);
		return 0;
	}

	return 1;
}

static state_t static_load()
{
	state_t s = EAR_SUCCESS;
	if (load_test(ear_getenv("HACK_FILE_LIKWID"))) return s;
	if (load_test(LIKWID_PATH "/lib64/liblikwid.so")) return s;
	if (load_test(LIKWID_PATH "/lib/liblikwid.so")) return s;
	return_msg(EAR_ERROR, "Can not load liblikwid.so");
}

static state_t static_dispose(state_t s, char *msg)
{
    likwid.perfmon_finalize();
    likwid.topology_finalize();
    return_msg(s, msg);
}

static state_t static_init()
{
    int i;

    // Forcing LIKWID to overlap other counters	
    setenv("LIKWID_FORCE", "1", 1);
    // Forcing LIKWID to find its deaemon
    appenv("PATH", LIKWID_PATH "/sbin");
    // Summary
    debug("LIKWID force: %s", ear_getenv("LIKWID_FORCE"));
    debug("LIKWID path : %s", ear_getenv("PATH"));
    
    //likwid.perfmon_setVerbosity(1);
    if (likwid.topology_init() < 0) {
        return static_dispose(EAR_ERROR, "failed to initialize LIKWID's topology");
    }
    info = likwid.get_cpuInfo();
    topo = likwid.get_cpuTopology();

    if((cpus = (int *) calloc(topo->numHWThreads, sizeof(int))) == NULL) {
        return static_dispose(EAR_ERROR, Generr.alloc_error);
    }
    for (i = 0; i < topo->numHWThreads; i++) {
        cpus[i] = topo->threadPool[i].apicId;
    }
    if (likwid.perfmon_init(topo->numHWThreads, cpus) < 0) {
        return static_dispose(EAR_ERROR, "failed to initialize LIKWID's perfmon module");
    }

    return EAR_SUCCESS;
}

state_t likwid_init()
{
    state_t s = EAR_SUCCESS;
    #ifndef LIKWID_BASE
    return_msg(EAR_ERROR, "path to LIKWID not set");
    #endif
    while (pthread_mutex_trylock(&lock));
    if (!ok) {
        if (state_ok(s = static_load())) {
            if (state_ok(s = static_init())) {
                ok = 1;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    return s;
}

state_t likwid_dispose()
{
    return static_dispose(EAR_SUCCESS, "");
}


state_t likwid_events_open(likevs_t *id, double **ctrs_alloc, uint *ctrs_count, char *evs_names, uint evs_count)
{
    id->evs_count = evs_count;
    if ((id->gid = likwid.perfmon_addEventSet(evs_names)) < 0) {
        return_msg(EAR_ERROR, "failed to add event string to LIKWID's perfmon");
    }
    if (likwid.perfmon_setupCounters(id->gid) < 0) {
        return_msg(EAR_ERROR, "failed to setup LIKWID's perfmon");
    }
    if (likwid.perfmon_startCounters() < 0) {
        return_msg(EAR_ERROR, "failed to start LIKWID's counters");
    }
    *ctrs_count = evs_count*topo->numHWThreads;
    *ctrs_alloc = calloc(*ctrs_count, sizeof(double));

    return EAR_SUCCESS;
}

state_t likwid_events_read(likevs_t *id, double *ctrs)
{
    int i, j, e;

    if (likwid.perfmon_readCounters() < 0) {
        return_msg(EAR_ERROR, "failed to start LIKWID's counters");
    }
    for (e = j = 0; e < id->evs_count; e++) {
        for (i = 0; i < topo->numHWThreads; i++, j++) {
            ctrs[j] = likwid.perfmon_getLastResult(id->gid, e, i);
			#if 0
			if (ctrs[j] > 0.0) {
            	printf("- event set %d at CPU %d: %f\n", i, cpus[i], ctrs[j]);
			}
			#endif
        }
    }
    return EAR_SUCCESS;
}

state_t likwid_events_close(likevs_t *id, double **ctrs)
{
	if (ctrs != NULL) {
		free(*ctrs);
		*ctrs = NULL;
	}
	return EAR_SUCCESS;
}
