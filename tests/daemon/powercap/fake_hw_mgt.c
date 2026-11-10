/*
 * fake_hw_mgt.c -- see fake_hw_mgt.h
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <common/config.h>
#include <common/hardware/topology.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/powercap/powercap_status_conf.h>
#include <management/cpupow/cpupow.h>
#include <metrics/common/apis.h>

#include "fake_hw_mgt.h"

fake_plugin_t fake_plug[NUM_DOMAINS];
fake_hw_t fake_hw;

/* Globals powercap_mgt.c takes from eard.c / powercap.c */
cluster_conf_t my_cluster_conf;
my_node_conf_t my_node_conf_storage;
my_node_conf_t *my_node_conf = &my_node_conf_storage;

static const char *env_names[NUM_DOMAINS] = {"EAR_POWERCAP_POLICY_NODE", "EAR_POWERCAP_POLICY_CPU",
                                             "EAR_POWERCAP_POLICY_DRAM", "EAR_POWERCAP_POLICY_GPU"};
static const char *plug_names[NUM_DOMAINS] = {"fake_node", "fake_cpu", "fake_dram", "fake_gpu"};

void fake_hw_default(int num_gpus)
{
    memset(fake_plug, 0, sizeof(fake_plug));
    memset(&fake_hw, 0, sizeof(fake_hw));
    fake_hw.sockets = 2;
    fake_hw.cpus    = 16;
    fake_hw.cores   = 16;
    fake_hw.gpus    = num_gpus;
    fake_hw.gpu_api = num_gpus ? API_NVML : API_DUMMY;
    for (int i = 0; i < FAKE_MAX_DEV; i++) {
        fake_hw.pkg_tdp[i]  = 200;
        fake_hw.dram_tdp[i] = 40;
        fake_hw.gpu_tdp[i]  = 400;
        fake_hw.gpu_min[i]  = 100;
    }
    fake_hw.load          = 8.0;
    fake_hw.policy_pstate = 1;
    for (int i = 0; i < num_gpus; i++) {
        fake_hw.gpu_raw[i].util_gpu = 80;
        fake_hw.gpu_raw[i].working  = 1;
    }
    /* Same domain settings the real cpu_generic.c and gpu.c export */
    fake_plug[DOMAIN_CPU].loaded                  = 1;
    fake_plug[DOMAIN_CPU].settings.node_ratio     = 0.1;
    fake_plug[DOMAIN_CPU].settings.security_range = 0.01;
    fake_plug[DOMAIN_GPU].loaded                  = num_gpus > 0;
    fake_plug[DOMAIN_GPU].settings.node_ratio     = 0.02;
    fake_plug[DOMAIN_GPU].settings.security_range = 0.01;
    for (int d = 0; d < NUM_DOMAINS; d++) {
        fake_plug[d].status.ok = PC_STATUS_OK;
        fake_plug[d].strategy  = PC_POWER;
    }
    my_cluster_conf.default_policy = 0;
    strcpy(my_cluster_conf.install.dir_plug, "/nonexistent");
    fake_hw_export_env();
}

void fake_hw_export_env(void)
{
    for (int d = 0; d < NUM_DOMAINS; d++) {
        if (fake_plug[d].loaded)
            setenv(env_names[d], plug_names[d], 1);
        else
            unsetenv(env_names[d]);
    }
}

ulong fake_pstate_freq(uint p)
{
    return 3000000 - p * 100000; /* 3.0 GHz .. 1.0 GHz in 100 MHz steps */
}

/* ---------------------------------------------------------- fake plugins */

#define FP_DEFINE(d)                                                                                                   \
    static state_t fp##d##_enable(suscription_t *s)                                                                    \
    {                                                                                                                  \
        fake_plug[d].enable_calls++;                                                                                   \
        return EAR_SUCCESS;                                                                                            \
    }                                                                                                                  \
    static state_t fp##d##_disable()                                                                                   \
    {                                                                                                                  \
        fake_plug[d].disable_calls++;                                                                                  \
        return EAR_SUCCESS;                                                                                            \
    }                                                                                                                  \
    static state_t fp##d##_set_powercap_value(uint pid, uint level, uint32_t *limit, uint32_t *util)                    \
    {                                                                                                                  \
        fake_plug[d].set_calls++;                                                                                      \
        fake_plug[d].last_level = level;                                                                               \
        int n = (d == DOMAIN_GPU) ? fake_hw.gpus : (d == DOMAIN_NODE ? 1 : fake_hw.sockets);                           \
        if (level != LEVEL_DEVICE)                                                                                     \
            n = 1;                                                                                                     \
        for (int i = 0; i < n && i < FAKE_MAX_DEV; i++)                                                                \
            fake_plug[d].last_limits[i] = limit[i];                                                                    \
        int nu = (d == DOMAIN_GPU) ? fake_hw.gpus : (d == DOMAIN_CPU ? fake_hw.cpus : 1);                              \
        for (int i = 0; i < nu && i < FAKE_MAX_DEV && util; i++)                                                       \
            fake_plug[d].last_util[i] = util[i];                                                                       \
        if (level != LEVEL_DEVICE)                                                                                     \
            fake_plug[d].cur = *limit;                                                                                 \
        return fake_plug[d].set_ret;                                                                                   \
    }                                                                                                                  \
    static state_t fp##d##_get_powercap_value(uint pid, uint32_t *pc)                                                  \
    {                                                                                                                  \
        int n = (d == DOMAIN_GPU) ? fake_hw.gpus : (d == DOMAIN_NODE ? 1 : fake_hw.sockets);                           \
        for (int i = 0; i < n; i++)                                                                                    \
            pc[i] = fake_plug[d].dev_caps[i];                                                                          \
        return EAR_SUCCESS;                                                                                            \
    }                                                                                                                  \
    static uint fp##d##_is_enabled(uint pid)                                                                           \
    {                                                                                                                  \
        return 1;                                                                                                      \
    }                                                                                                                  \
    static void fp##d##_set_status(uint st)                                                                            \
    {                                                                                                                  \
        fake_plug[d].last_set_status = st;                                                                             \
    }                                                                                                                  \
    static void fp##d##_set_pc_mode(uint m)                                                                            \
    {                                                                                                                  \
        fake_plug[d].last_pc_mode = m;                                                                                 \
    }                                                                                                                  \
    static uint fp##d##_get_strategy()                                                                                 \
    {                                                                                                                  \
        return fake_plug[d].strategy;                                                                                  \
    }                                                                                                                  \
    static void fp##d##_to_str(char *b)                                                                                \
    {                                                                                                                  \
        sprintf(b, "[%s %u]", plug_names[d], fake_plug[d].cur);                                                        \
    }                                                                                                                  \
    static void fp##d##_print(int fd)                                                                                  \
    {                                                                                                                  \
    }                                                                                                                  \
    static void fp##d##_set_app_req_freq(ulong *f)                                                                     \
    {                                                                                                                  \
        fake_plug[d].app_freq_calls++;                                                                                 \
        for (int i = 0; i < FAKE_MAX_DEV; i++)                                                                         \
            fake_plug[d].app_freq[i] = f[i];                                                                           \
    }                                                                                                                  \
    static void fp##d##_set_verb_channel(int fd)                                                                       \
    {                                                                                                                  \
    }                                                                                                                  \
    static void fp##d##_set_new_utilization(uint32_t *util)                                                             \
    {                                                                                                                  \
        fake_plug[d].new_util_calls++;                                                                                 \
        int nu = (d == DOMAIN_GPU) ? fake_hw.gpus : (d == DOMAIN_CPU ? fake_hw.cpus : 1);                              \
        for (int i = 0; i < nu && i < FAKE_MAX_DEV; i++)                                                               \
            fake_plug[d].new_util[i] = util[i];                                                                        \
    }                                                                                                                  \
    static uint fp##d##_get_powercap_status(domain_status_t *st)                                                       \
    {                                                                                                                  \
        fake_plug[d].status_calls++;                                                                                   \
        *st            = fake_plug[d].status;                                                                          \
        st->current_pc = fake_plug[d].cur;                                                                             \
        return st->ok == PC_STATUS_RELEASE;                                                                            \
    }                                                                                                                  \
    static state_t fp##d##_reset_powercap_value()                                                                      \
    {                                                                                                                  \
        fake_plug[d].reset_calls++;                                                                                    \
        return EAR_SUCCESS;                                                                                            \
    }                                                                                                                  \
    static state_t fp##d##_burst()                                                                                     \
    {                                                                                                                  \
        fake_plug[d].burst_calls++;                                                                                    \
        return EAR_SUCCESS;                                                                                            \
    }                                                                                                                  \
    static state_t fp##d##_relax()                                                                                     \
    {                                                                                                                  \
        fake_plug[d].relax_calls++;                                                                                    \
        return EAR_SUCCESS;                                                                                            \
    }                                                                                                                  \
    static void fp##d##_get_settings(domain_settings_t *s)                                                             \
    {                                                                                                                  \
        *s = fake_plug[d].settings;                                                                                    \
    }                                                                                                                  \
    static void *fp##d##_table[] = {                                                                                   \
        fp##d##_enable,          fp##d##_disable,     fp##d##_set_powercap_value, fp##d##_get_powercap_value,          \
        fp##d##_is_enabled,      fp##d##_set_status,  fp##d##_set_pc_mode,        fp##d##_get_strategy,                \
        fp##d##_to_str,          fp##d##_print,       fp##d##_set_app_req_freq,   fp##d##_set_verb_channel,            \
        fp##d##_set_new_utilization, fp##d##_get_powercap_status, fp##d##_reset_powercap_value, NULL, NULL,            \
        fp##d##_burst,           fp##d##_relax,       fp##d##_get_settings,                                            \
    };

FP_DEFINE(0)
FP_DEFINE(1)
FP_DEFINE(2)
FP_DEFINE(3)

static void **fp_tables[NUM_DOMAINS] = {fp0_table, fp1_table, fp2_table, fp3_table};

/* Order of the names powercap_mgt.c asks for; fp tables above follow it */
static const char *expected_names[] = {
    "enable",       "disable",          "set_powercap_value", "get_powercap_value",  "is_powercap_policy_enabled",
    "set_status",   "set_pc_mode",      "get_powercap_strategy", "powercap_to_str",  "print_powercap_value",
    "set_app_req_freq", "set_verb_channel", "set_new_utilization", "get_powercap_status", "reset_powercap_value",
    "release_powercap_allocation", "increase_powercap_allocation", "plugin_set_burst", "plugin_set_relax",
    "plugin_get_settings",
};

state_t symplug_open(char *path, void *calls[], const char *names[], uint n)
{
    int d = -1;
    for (int i = 0; i < NUM_DOMAINS; i++)
        if (path && strstr(path, plug_names[i]))
            d = i;
    if (d < 0 || !fake_plug[d].loaded)
        return EAR_ERROR;
    for (uint i = 0; i < n; i++) {
        calls[i] = NULL;
        for (uint j = 0; j < sizeof(expected_names) / sizeof(expected_names[0]); j++)
            if (!strcmp(names[i], expected_names[j]))
                calls[i] = fp_tables[d][j];
        if (fake_plug[d].no_strategy && !strcmp(names[i], "get_powercap_strategy"))
            calls[i] = NULL;
    }
    return EAR_SUCCESS;
}

/* --------------------------------------------------------- discovery */

state_t topology_init(topology_t *t)
{
    memset(t, 0, sizeof(*t));
    t->socket_count = fake_hw.sockets;
    t->cpu_count    = fake_hw.cpus;
    t->core_count   = fake_hw.cores;
    return EAR_SUCCESS;
}

void gpu_load(int options)
{
}

void gpu_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(*info));
    info->api        = fake_hw.gpu_api;
    info->devs_count = fake_hw.gpus;
}

void gpu_get_devices(gpu_devs_t **devs, uint *devs_count)
{
    if (devs_count)
        *devs_count = fake_hw.gpus;
}

void gpu_data_alloc(gpu_t **data)
{
    *data = calloc(FAKE_MAX_DEV, sizeof(gpu_t));
}

state_t gpu_read_raw(gpu_t *data)
{
    memcpy(data, fake_hw.gpu_raw, sizeof(gpu_t) * (fake_hw.gpus ? fake_hw.gpus : 1));
    return EAR_SUCCESS;
}

void mgt_cpupow_load(topology_t *tp, int eard)
{
}

state_t mgt_cpupow_tdp_get(int domain, uint32_t *watts)
{
    for (int i = 0; i < fake_hw.sockets; i++)
        watts[i] = (domain == 0) ? fake_hw.pkg_tdp[i] : fake_hw.dram_tdp[i];
    return EAR_SUCCESS;
}

state_t mgt_cpupow_limits_get(int domain, uint32_t *wmin, uint32_t *wmax)
{
    for (int i = 0; i < fake_hw.sockets; i++) {
        wmin[i] = (domain == CPUPOW_SOCKET) ? fake_hw.pkg_min[i] : fake_hw.dram_min[i];
        wmax[i] = (domain == CPUPOW_SOCKET) ? fake_hw.pkg_tdp[i] : fake_hw.dram_tdp[i];
    }
    return EAR_SUCCESS;
}

state_t mgt_cpupow_powercap_get(int domain, uint32_t *watts)
{
    for (int i = 0; i < fake_hw.sockets; i++)
        watts[i] = (domain == CPUPOW_DRAM) ? fake_hw.dram_cap[i] : 0;
    return EAR_SUCCESS;
}

int mgt_cpupow_powercap_is_capable(int domain)
{
    return !(domain == CPUPOW_DRAM && fake_hw.dram_not_capable);
}

state_t mgt_cpupow_powercap_set(int domain, uint32_t *watts)
{
    if (!mgt_cpupow_powercap_is_capable(domain))
        return EAR_ERROR;
    if (domain == CPUPOW_DRAM)
        for (int i = 0; i < fake_hw.sockets; i++)
            fake_hw.dram_cap[i] = watts[i];
    return EAR_SUCCESS;
}

state_t powercap_set_stored_device_value(uint domain, uint device_id, uint32_t v)
{
    if (domain == DOMAIN_DRAM && device_id < FAKE_MAX_DEV)
        fake_hw.stored_dram[device_id] = v;
    return EAR_SUCCESS;
}

void mgt_gpu_load(int force_api)
{
}

state_t mgt_gpu_power_cap_get_rank(void *c, uint32_t *wmin, uint32_t *wmax)
{
    for (int i = 0; i < fake_hw.gpus; i++) {
        if (wmin)
            wmin[i] = fake_hw.gpu_min[i];
        if (wmax)
            wmax[i] = fake_hw.gpu_tdp[i];
    }
    return EAR_SUCCESS;
}

state_t loadavg(float *min, float *Vmin, float *XVmin, uint *runnable, uint *total, uint *lastpid)
{
    *min = fake_hw.load;
    *Vmin = *XVmin = fake_hw.load;
    *runnable = *total = *lastpid = 0;
    return EAR_SUCCESS;
}

/* ------------------------------------------------------ policy / freq */

static policy_conf_t fake_policy;

policy_conf_t *get_my_policy_conf(my_node_conf_t *my_node, uint p_id)
{
    fake_policy.p_state = fake_hw.policy_pstate;
    return &fake_policy;
}

ulong frequency_pstate_to_freq(uint pstate_index)
{
    return fake_pstate_freq(pstate_index);
}

uint frequency_get_num_pstates()
{
    return 21;
}
