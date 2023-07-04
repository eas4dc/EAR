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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/system/time.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/environment_common.h>
#include <metrics/common/cupti.h>
#include <metrics/gpu/archs/cupti.h>

#define MET_N        2
#define GRP_N        8  // Events in a group
#define DEV_N        8  // Supported devices

typedef struct CUevents_s {
    CUpti_EventID          evs_ids[GRP_N];
    ullong                 evs_reads[GRP_N];
    ullong                 evs_normals[GRP_N];
    char                   evs_names[GRP_N][128];
    uint                   evs_count;
    uint                   ins_count; // instances
    uint                   tins_count; // total instances
} CUevents;

typedef struct cuAssoc_s {
    CUdevice               device;
    CUcontext              context;
    CUpti_MetricID         metric_id;
    CUpti_MetricValueKind  metric_kind;
    CUpti_MetricValue      metric_value;
    CUpti_EventGroupSets  *events_bag;
    CUpti_EventGroupSet   *events_passes;
    CUpti_EventGroup      *events_groups;
    CUevents               events_groupsD[GRP_N];
    uint                   events_groups_count;
    uint                   events_count;
    uint                   already_added;
    uint                   already_created;
    uint                   already_banned;
    timestamp_t            time;
} CUassoc;

typedef struct CUmetric_s {
    char                   name[32];
    uint                   num;
} CUmetric;

typedef struct CUroot_s {
    CUpti_SubscriberHandle subscriber;
    uint                   devs_count;
    uint                   already_initialized;
    uint                   already_protected;
    uint                   already_paused;
    uint                   set_resume;
    uint                   set_pause;
} CUroot;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static CUassoc         assocs[DEV_N][MET_N];
static CUmetric        metrics[MET_N];
static uint            metrics_count;
static gpuproc_t       pool[DEV_N];
static cupti_t         cupti;
static cuda_t          cuda;
static CUroot          root;
static suscription_t  *sus;

#define METRIC_UNKNOWN   0
#define METRIC_CPI       1
#define METRIC_FLOPS_SP  2
#define METRIC_FLOPS_DP  3

#define cc(call, ret)                                                           \
    do {                                                                        \
        CUptiResult _status = call;                                             \
        if (_status != CUPTI_SUCCESS) {                                         \
            const char *errstr;                                                 \
            cupti.GetResultString(_status, &errstr);                            \
            debug(#call ": FAILED, %s", errstr);       \
            ret                                                                 \
        }                                                                       \
    } while (0)
#define cci(call) cc(call, return 0;)
#define ccv(call) cc(call, return  ;)
#define ccn(call) cc(call,         ;)

static uint aux_listtoar(const char *list, char matrix[][128], uint *rows_count)
{
    uint char_count = 0;
    uint i;

    for (*rows_count = i = 0; i < strlen(list)+1; ++i) {
        debug("list[%d]: %c", i, list[i]);
        if (list[i] == ',' || list[i] == '\0') {
            *rows_count += (char_count > 0);
            char_count   = 0;
        } else {
            matrix[*rows_count][char_count++] = list[i];
        }
    }
    debug("Detected metrics #%d", *rows_count);
    return (*rows_count > 0);
}

static int aux_metrics_build(CUmetric *metrics)
{
    char matrix[128][128] = { 0 };
    char *names;
    int i;

    if ((names = ear_getenv("CUPTI_METRIC")) == NULL) {
        debug("CUPTI_METRIC environent variable doesn't found");
        return 0;
    }
    if (!aux_listtoar(names, matrix, &metrics_count)) {
        return 0;
    }
    if (metrics_count > MET_N) {
        metrics_count = MET_N;
    }
    for (i = 0; i < metrics_count; ++i) {
        strncpy(metrics[i].name, matrix[i], 32);
        debug("MET%d: '%s'", i, metrics[i].name);
        if (!strcmp(matrix[i], "flop_count_dp")) { metrics[i].num = METRIC_FLOPS_DP; }
        if (!strcmp(matrix[i], "flop_count_sp")) { metrics[i].num = METRIC_FLOPS_SP; }
        if (!strcmp(matrix[i], "ipc"          )) { metrics[i].num = METRIC_CPI;      }
    }
    return 1;
}

void gpu_cupti_load(gpuproc_ops_t *ops)
{
    if (!aux_metrics_build(metrics)) {
        return;
    }
    if (state_fail(cupti_open(&cupti, &cuda))) {
        debug("Can not open CUPTI: %s", state_msg);
        return;
    }
    cupti_count_devices(&root.devs_count);
    //
    apis_set(ops->get_api,       gpu_cupti_get_api);
    apis_set(ops->init,          gpu_cupti_init);
    apis_set(ops->dispose,       gpu_cupti_dispose);
    apis_set(ops->count_devices, gpu_cupti_count_devices);
    apis_set(ops->read,          gpu_cupti_read);
    apis_set(ops->enable,        gpu_cupti_enable);
    apis_set(ops->disable,       gpu_cupti_disable);

    debug("Loaded CUPTI (%d devices)", root.devs_count);
}

void gpu_cupti_get_api(uint *api)
{
    *api = API_CUPTI;
}

state_t gpu_cupti_dispose(ctx_t *c)
{
    return cupti_close();
}

static state_t low_callback_set();
static state_t low_pool(void *p);

state_t gpu_cupti_init(ctx_t *c)
{
    state_t s;

    while (pthread_mutex_trylock(&lock));
    debug("Initializing CUPTI");
    if (root.already_initialized) {
        goto out;
    }
    // Setting callbacks
    if (state_fail(s = low_callback_set())) {
        debug("Error while initializing CUPTI: %s", state_msg);
        goto out;
    }
    // Setting suscriptions
    sus = suscription();
    sus->call_main  = low_pool;
    sus->time_relax = 2000;
    sus->time_burst = 2000;
    sus->suscribe(sus);
    debug("Initialized CUPTI");
    root.already_initialized = 1;
out:
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t gpu_cupti_count_devices(ctx_t *c, uint *devs_count)
{
    *devs_count = root.devs_count;
    return EAR_SUCCESS;
}

state_t gpu_cupti_read(ctx_t *c, gpuproc_t *data)
{
    while (pthread_mutex_trylock(&lock));
    memcpy(data, pool, root.devs_count * sizeof(gpuproc_t));
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t gpu_cupti_enable(ctx_t *c)
{
    root.set_resume = 1;
    root.already_paused = 0;
    return EAR_SUCCESS;
}

state_t gpu_cupti_disable(ctx_t *c)
{
    root.set_pause = 1;
    return EAR_SUCCESS;
}

static int low_assoc_read(CUassoc *a, CUmetric *m, uint g)
{
    // The NVidia 3080 has 8704 stream processors.
    static ullong buffer[16384];
    uint events_count;
    size_t events_read;
    size_t size1;
    size_t size2;
    int e, i;

    debug("Reading...");
    // Reading events
    size1 = sizeof(buffer);
    size2 = sizeof(a->events_groupsD[g].evs_ids);
    cci(cupti.EventGroupReadAllEvents(a->events_groups[g], CUPTI_EVENT_READ_FLAG_NONE, &size1, buffer, &size2, a->events_groupsD[g].evs_ids, &events_read));
    //
    events_count = a->events_groupsD[g].evs_count;
    debug("Pooled %lu/%d events (ctx %p)", events_read, a->events_groupsD[g].evs_count, a->context);
    //
    for (e = 0; e < events_count; ++e) {
        for (i = 0; i < a->events_groupsD[g].ins_count; ++i) {
            a->events_groupsD[g].evs_reads[e] += buffer[(i * events_count) + e];
        }
        // Future: normalize these values
        debug("D%d, %s: %llu", a->device, a->events_groupsD[g].evs_names[e], a->events_groupsD[g].evs_reads[e]);
    }
    // Kind: Events
    // --------------------
    // CPI     : DOUBLE (0)
    // FLOPS_SP: UINT64 (1): fadd + fmul + ffma (OPS)
    // FLOPS_DP: UINT64 (1): fadd (0x16000090) + fmul (0x16000091) + ffma (0x16000092)
    #if 0
    // This doesn't works well
    cci(cupti.MetricGetValue(a->device, a->metric_id,
        a->evs_count * sizeof(CUpti_EventID), a->events_groupsD[g].evs_ids,
        a->evs_count * sizeof(uint64_t), (uint64_t *) a->events_groupsD[g].evs_reads,
        (uint64_t) timestamp_diffnow(&a->time, TIME_NSECS), &a->metric_value));
    #endif
    while (pthread_mutex_trylock(&lock));
    if (m->num == METRIC_CPI) {
        pool[a->device].insts  += (ullong) a->events_groupsD[g].evs_reads[0];
        pool[a->device].cycles += (ullong) a->events_groupsD[g].evs_reads[1];
    }
    if (m->num == METRIC_FLOPS_SP) {
        pool[a->device].flops_sp += (ullong) a->events_groupsD[g].evs_reads[0];
        pool[a->device].flops_sp += (ullong) a->events_groupsD[g].evs_reads[1];
        pool[a->device].flops_sp += (ullong) a->events_groupsD[g].evs_reads[2] * 2LLU;
    }
    if (m->num == METRIC_FLOPS_DP) {
        pool[a->device].flops_dp += (ullong) a->events_groupsD[g].evs_reads[0];
        pool[a->device].flops_dp += (ullong) a->events_groupsD[g].evs_reads[1];
        pool[a->device].flops_dp += (ullong) a->events_groupsD[g].evs_reads[2] * 2LLU;
    }
    pthread_mutex_unlock(&lock);
    // Saving metadata
    pool[a->device].time     = a->time;
    pool[a->device].samples += 1;
    // Getting new time
    timestamp_get(&a->time);

    return 1;
}

static int low_assoc_print(CUassoc *m)
{
#if SHOW_DEBUGS
    CUpti_EventID events_ids[GRP_N];
    char event_name[128];
    uint events_count;
    uint p, g, e;
    size_t size;

    cci(cupti.MetricGetNumEvents(m->metric_id, &events_count));

    debug("-- [CUPTI] ----------------------");
    debug("device : %d",    m->device);
    debug("context: %p",   m->context);
    debug("met. id: 0x%X", m->metric_id);
    debug("#passes: %d",   m->events_bag->numSets);
    debug("#events: %u",   events_count);
    // Sets
    for (p = 0; p < m->events_bag->numSets; ++p) {
        CUpti_EventGroupSet *set = &m->events_bag->sets[p];
        debug("-- [PASS %d/%d] -------------------", p+1, m->events_bag->numSets);
        debug("#groups: %d", set->numEventGroups);
        // Groups
        for (g = 0; g < m->events_bag->sets[p].numEventGroups; ++g) {
            debug("-- [GROUP %d (%p)] -----------", g, set->eventGroups[g]);
            CUpti_EventGroup* group = set->eventGroups[g];
            //
            size = sizeof(events_count);
            cci(cupti.EventGroupGetAttribute(group, CUPTI_EVENT_GROUP_ATTR_NUM_EVENTS, &size, &events_count));
            size = sizeof(events_ids);
            cci(cupti.EventGroupGetAttribute(group, CUPTI_EVENT_GROUP_ATTR_EVENTS, &size, events_ids));
            // Events
            for (e = 0; e < events_count; ++e) {
                size = sizeof(event_name);
                cci(cupti.EventGetAttribute(events_ids[e], CUPTI_EVENT_ATTR_NAME, &size, event_name));
                if (!strcmp(event_name, "event_name")) {
                    debug("unknown (id 0x%X)", events_ids[e]);
                } else {
                    debug("%s (id 0x%X)", event_name, events_ids[e]);
                }
            }
        }
        debug("---------------------------------");
    }
#endif
    return 1;
}

static int low_assoc_fill(CUassoc *m)
{
    size_t size;
    int g, e;
    int aux;

    // We don't support more than 1 pass
    if (m->events_bag->numSets > 1) {
        debug("More than 1 passes are not supported.");
        return 0;
    }
    // We are interested just in one pass, so we save the first set
    m->events_passes = m->events_bag->sets;
    m->events_groups = m->events_bag->sets->eventGroups;
    m->events_groups_count = m->events_bag->sets->numEventGroups;
    // Sets/set/groups/events
    for (g = 0; g < m->events_groups_count; ++g) {
        // Counting events per group
        size = sizeof(m->events_groupsD[g].evs_count);
        cci(cupti.EventGroupGetAttribute(m->events_groups[g], CUPTI_EVENT_GROUP_ATTR_NUM_EVENTS, &size, &m->events_groupsD[g].evs_count));
        // Counting and retrieving event IDs
        size = sizeof(m->events_groupsD[g].evs_ids);
        cci(cupti.EventGroupGetAttribute(m->events_groups[g], CUPTI_EVENT_GROUP_ATTR_EVENTS, &size, m->events_groupsD[g].evs_ids));
        // Counting instances
        size = sizeof(m->events_groupsD[g].ins_count);
        cci(cupti.EventGroupGetAttribute(m->events_groups[g], CUPTI_EVENT_GROUP_ATTR_INSTANCE_COUNT, &size, &m->events_groupsD[g].ins_count));
        // Counting total instances
        //size = sizeof(m->events_groupsD[g].tins_count);
        //cci(cupti.DeviceGetEventDomainAttribute(metricData->device, groupDomain, CUPTI_EVENT_DOMAIN_ATTR_TOTAL_INSTANCE_COUNT, &m->events_groupsD[g].tins_count));
        // Enabling all domains
        cci(cupti.EventGroupSetAttribute(m->events_groups[g], CUPTI_EVENT_GROUP_ATTR_PROFILE_ALL_DOMAIN_INSTANCES, sizeof(aux), &aux));
        // Enabling metric
        debug("Enabling group %p", m->events_groups[g]);
        cci(cupti.EventGroupEnable(m->events_groups[g]));

        for (e = 0; e < m->events_groupsD[g].evs_count; ++e) {
            size = sizeof(m->events_groupsD[g].evs_names[e]);
            cci(cupti.EventGetAttribute(m->events_groupsD[g].evs_ids[e], CUPTI_EVENT_ATTR_NAME, &size, m->events_groupsD[g].evs_names[e]));
            if (!strcmp(m->events_groupsD[g].evs_names[e], "event_name")) {
                sprintf(m->events_groupsD[g].evs_names[e], "ev%X", m->events_groupsD[g].evs_ids[e]);
            }
        }
        size = sizeof(CUpti_MetricValueKind);
        cci(cupti.MetricGetAttribute(m->metric_id, CUPTI_METRIC_ATTR_VALUE_KIND, &size, &m->metric_kind));
    }
    return 1;
}

static int low_assoc_create(CUassoc *m, char *metric_name)
{
    debug("Creating metric from name '%s'", metric_name);
    cci(cupti.SetEventCollectionMode(m->context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS));
    cci(cupti.MetricGetIdFromName(m->device, metric_name, &m->metric_id));
    // When returned by cuptiMetricCreateEventGroupSets, indicates the number
    // of passes required to collect all the events, and the event groups that
    // should be collected during each pass.
    //
    // Details:
    // CUpti_EventGroupSets: a struct that contains an array of sets (passes)
    // CUpti_EventGroupSet: a struct that contains an array of CUpti_EventGroup
    // CUpti_EventGroup: an array of events
    cci(cupti.MetricCreateEventGroupSets(m->context, sizeof(m->metric_id), &m->metric_id, &m->events_bag));
    // Print (or debugging)
    if (!low_assoc_print(m)) {
        return 0;
    }
    if (!low_assoc_fill(m)) {
        return 0;
    }
    // State control
    m->already_created = 1;

    return 1;
}

static void low_assoc_clean(CUassoc *a)
{
    // It has just 1 pass, so it is enough
//    cupti.EventGroupSetsDestroy(a->events_passes);
    cupti.EventGroupSetsDestroy(a->events_bag);
    // Cleaning
    memset(a, 0, sizeof(CUassoc));
}

#if 0
static int low_assoc_new_opportunity(CUdevice device)
{
    int m;
    for (m = 0; m < metrics_count; ++m) {
        if (assocs[device][m].already_banned > 3) {
            return 0;
        }
    }
    for (m = 0; m < metrics_count; ++m) {
        if (assocs[device][m].already_banned > 0) {
            return 1;
        }
    }
    return 0;
}

static void low_assoc_ban(CUassoc *a)
{
    debug("Banning ASSOC of DEV%d", a->device);
    a->already_banned += 1;
    // It has just 1 pass, so it is enough
    cupti.EventGroupSetsDestroy(a->events_bag);
}
#endif

static void low_assoc_add(CUcontext context, CUdevice device)
{
    uint i;
    //debug("Adding context %p for device %d", context, device);
    if (root.already_paused) {
        return;
    }
    if (assocs[device][0].context == context) {
        return;
    }
    debug("Added context %p (previous %p) for device %d", context, assocs[device][0].context, device);
    for (i = 0; i < metrics_count; ++i) {
        if (assocs[device][i].already_created) {
            low_assoc_clean(&assocs[device][i]);
        }
        // Cleaning
        assocs[device][i].device  = device;
        assocs[device][i].context = context;
        assocs[device][i].already_added = 1;
    }
}

static void low_callback(void *user_data, CUpti_CallbackDomain domain, CUpti_CallbackId id, CUpti_CallbackData *data)
{
    //debug("Callback ... %s %p", data->functionName, data->context);
    uint enter = (data->callbackSite != CUPTI_API_EXIT);
    CUdevice device;

    if (data->context == NULL) {
        return;
    }
    if (id == CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel) {
        if (enter) {
            if (cuda.CtxGetDevice(&device) != CUDA_SUCCESS) {
                return;
            }
            low_assoc_add(data->context, device);
        }
    }
}

static state_t low_callback_set()
{
    CUptiResult s;
    char *str;
    if ((s = cupti.Subscribe(&root.subscriber, (CUpti_CallbackFunc) low_callback, NULL)) != CUPTI_SUCCESS) {
        cupti.GetResultString(s, (const char **) &str);
        return_msg(EAR_ERROR, str);
    }
    if ((s = cupti.EnableCallback(1, root.subscriber, CUPTI_CB_DOMAIN_DRIVER_API, CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel)) != CUPTI_SUCCESS) {
        cupti.GetResultString(s, (const char **) &str);
        return_msg(EAR_ERROR, str);
    }
    return EAR_SUCCESS;
}

static state_t low_pool(void *p)
{
    CUassoc *assoc;
    uint d, m, g;

    debug("Pooling... (paused %d)", root.already_paused);
    if (root.already_paused) {
        return EAR_SUCCESS;
    }
    for (d = 0; d < DEV_N; ++d) {
        for (m = 0; m < MET_N; ++m) {
            assoc = &assocs[d][m];
            if (assoc->already_added && !assoc->already_created) {
                if (!low_assoc_create(assoc, metrics[m].name)) {
                    low_assoc_clean(assoc);
                }
            }
            if (assoc->already_created) {
                for (g = 0; g < assoc->events_groups_count; ++g) {
                    if (root.set_pause) {
                        debug("IN cupti.EventGroupDisable %p", assoc->events_groups[g]);
                        ccn(cupti.EventGroupDisable(assoc->events_groups[g]));
                        debug("OUT cupti.EventGroupDisable %p", assoc->events_groups[g]);
                        continue;
                    }
                    if (root.set_resume) {
                        debug("IN cupti.EventGroupEnable %p", assoc->events_groups[g]);
                        ccn(cupti.EventGroupEnable(assoc->events_groups[g]));
                        debug("OUT cupti.EventGroupEnable %p", assoc->events_groups[g]);
                        continue;
                    }
                    if (!low_assoc_read(assoc, &metrics[m], g)) {
                        low_assoc_clean(assoc);
                    }
                }
            }
        }
    }
    if (root.set_pause) {
        debug("Paused");
        root.already_paused = 1;
        root.set_pause = 0;
    }
    if (root.set_resume) {
        debug("Resumed");
        root.set_resume = 0;
    }
    return EAR_SUCCESS;
}
