/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include "common/config/config_def.h"
#include "common/types/generic.h"
#include "metrics/imcfreq/imcfreq.h"
#include <common/system/monitor.h>
#include <common/system/symplug.h>
#include <common/utils/special.h>
#include <common/utils/stress.h>
#include <common/utils/strscreen.h>
#include <common/utils/strtable.h>
#include <metrics/metrics.h>
#include <stdio.h>
#include <stdlib.h>

#include <metrics/metrics.h>

#include <daemon/powercap/powercap_status_conf.h>

#include <sys/types.h>

#include <management/cpufreq/frequency.h>

#define DEFAULT_PC_PLUGIN_NAME_CPU "cpu_generic"

typedef struct powercap_symbols {
    state_t (*enable)(suscription_t *sus);
    state_t (*disable)();
    state_t (*set_powercap_value)(uint pid, uint domain, uint limit, ulong *util);
    state_t (*get_powercap_value)(uint pid, uint *powercap);
    uint (*is_powercap_policy_enabled)(uint pid);
    void (*set_status)(uint status);
    void (*set_pc_mode)(uint mode);
    uint (*get_powercap_strategy)();
    void (*powercap_to_str)(char *b);
    void (*print_powercap_value)(int fd);
    void (*set_app_req_freq)(ulong *f);
    void (*set_verb_channel)(int fd);
    void (*set_new_utilization)(ulong *util);
    uint (*get_powercap_status)(domain_status_t *status);
    state_t (*reset_powercap_value)();
    state_t (*release_powercap_allocation)(uint decrease);
    state_t (*increase_powercap_allocation)(uint increase);
    state_t (*plugin_set_burst)();
    state_t (*plugin_set_relax)();
    void (*plugin_get_settings)(domain_settings_t *settings);
} powercapsym_t;

const char *pcsyms_names[] = {
    "enable",
    "disable",
    "set_powercap_value",
    "get_powercap_value",
    "is_powercap_policy_enabled",
    "set_status",
    "set_pc_mode",
    "get_powercap_strategy",
    "powercap_to_str",
    "print_powercap_value",
    "set_app_req_freq",
    "set_verb_channel",
    "set_new_utilization",
    "get_powercap_status",
    "reset_powercap_value",
    "release_powercap_allocation",
    "increase_powercap_allocation",
    "plugin_set_burst",
    "plugin_set_relax",
    "plugin_get_settings",
};

const int pcsyms_n = 20;

static topology_t tp;
static metrics_info_t m;
static metrics_read_t mr1;
static metrics_read_t mr2;
static metrics_diff_t mrD;

static state_t metrics_apis_init(void *whatever)
{
    topology_init(&tp);
    metrics_load(&m, &tp, "/home/void/ear_install/lib/plugins/energy/energy_cpu_gpu.so", 0);
    // metrics_init_screen(&m, &tp);
    metrics_data_alloc(&mr1, &mr2, &mrD);
    metrics_read(&mr1);
    return EAR_SUCCESS;
}

static char *nodepow_data_tostr(ulong avrg, char *buffer, size_t length)
{
    sprintf(buffer, "!%lu", avrg);
    return buffer;
}

static char *dram_data_tostr(ullong *diffs, double secs, char *buffer, size_t length)
{
    double power = 0.0;
    double mean  = 0.0;
    int i, j;

    buffer[0] = '\0';
    for (i = j = 0; i < tp.socket_count; ++i) {
        power = energy_cpu_compute_power(diffs[i], secs);
        j += sprintf(&buffer[j], "%0.1lf ", power);
        mean += power;
    }
    sprintf(&buffer[j], "!%0.1lf", mean);
    return buffer;
}

static char *cpupow_data_tostr(ullong *diffs, double secs, char *buffer, size_t length)
{
    double power = 0.0;
    double mean  = 0.0;
    int i, j;

    buffer[0] = '\0';
    for (i = j = 0; i < tp.socket_count; ++i) {
        power = energy_cpu_compute_power(diffs[tp.socket_count + i], secs);
        j += sprintf(&buffer[j], "%0.1lf ", power);
        mean += power;
    }
    sprintf(&buffer[j], "!%0.1lf", mean);
    return buffer;
}

static void energy_to_power_tostr(ulong *energy, ulong elapsed_time, char *buffer)
{
    sprintf(buffer, "!%.2lf", (float) (*energy) / elapsed_time);
}

static state_t metrics_apis_update(void *whatever)
{
    printf(" ====================================== \n");

    metrics_read_copy(&mr2, &mr1, &mrD);
    // metrics_data_print(&mrD, 0);
    char tmp[1024];
    cpufreq_data_tostr(mrD.cpu_diff, mrD.cpu_avrg, tmp, sizeof(tmp));
    printf("CPU frequency %s\n", tmp);
    imcfreq_data_tostr(mrD.imc_diff, &mrD.imc_avrg, tmp, sizeof(tmp));
    printf("IMC frequency %s\n", tmp);
    cpupow_data_tostr(mrD.pow_diff, mrD.time, tmp, sizeof(tmp));
    printf("CPU power %s\n", tmp);
    dram_data_tostr(mrD.pow_diff, mrD.time, tmp, sizeof(tmp));
    printf("DRAM power %s\n", tmp);
    energy_to_power_tostr(&mrD.nod_avrg, 2000, tmp);
    printf("Node power %s\n", tmp);

    printf(" ====================================== \n");

    return EAR_SUCCESS;
}

uint32_t powercap_get_stress(uint32_t freq1, uint32_t freq2, uint32_t freq3, uint32_t freq4)
{
    return 0;
}

ulong pmgt_idle_def_freq = 3501000;

void pmgt_get_app_req_freq(ullong domain, ullong *freq, uint32_t nodesize)
{
    for (int32_t i = 0; i < nodesize; i++) {
        freq[i] = pmgt_idle_def_freq;
    }
    return;
}

int main(int argc, char *argv[])
{
    uint32_t power = POWER_CAP_UNLIMITED;
    if (argc > 1) {
        power = atoi(argv[1]);
    }
    verb_level = 2;
    // Monitoring
    monitor_init();
    suscription_t *sus = suscription();
    sus->call_init     = metrics_apis_init;
    sus->call_main     = metrics_apis_update;
    sus->time_relax    = 2000;
    sus->time_burst    = 2000;
    sus->suscribe(sus);

    char *time_env = getenv("POWERCAP_TEST_SLEEP");
    uint32_t ttime = 100;
    if (time_env) {
        ttime = atoi(time_env);
    }

    // ear conf
    char ear_path[256]  = {0};
    cluster_conf_t conf = {0};
    if (get_ear_conf_path(ear_path) == EAR_ERROR) {
        printf("Error getting ear.conf path\n");
        exit(0);
    }
    read_cluster_conf(ear_path, &conf);

    char *obj_path;
    char basic_path[4098];
    powercapsym_t pcsym_fun = {0};
    state_t ret;
    /* DOMAIN_CPU */
    obj_path = ear_getenv(EAR_POWERCAP_POLICY_CPU); // environment variable takes priority
    if (obj_path == NULL) {                         // if envar is not defined, we try with the ear.conf var
        if (obj_path != NULL && strlen(obj_path) > 1) {
            sprintf(basic_path, "%s/powercap/%s", conf.install.dir_plug, obj_path);
            obj_path = basic_path;
        }
        /* Plugin per domain node defined */
        else if (strcmp(DEFAULT_PC_PLUGIN_NAME_CPU, "noplugin")) {
            sprintf(basic_path, "%s/powercap/%s.so", conf.install.dir_plug, DEFAULT_PC_PLUGIN_NAME_CPU);
            obj_path = basic_path;
        }
    }
    if (obj_path != NULL) {
        verbose(VEARD_PC, "Loading %s powercap plugin domain cpu", obj_path);
        ret = symplug_open(obj_path, (void **) &pcsym_fun, pcsyms_names, pcsyms_n);
        if (ret == EAR_SUCCESS) {
            debug("CPU plugin correctly loaded");
        } else {
            error("Domain CPU not loaded");
            exit(1);
        }
    }
    suscription_t *pc_sus = suscription();
    pcsym_fun.enable(pc_sus);
    uint64_t cpu_util = 0;

    printf("Setting power to %u\n", power);
    pcsym_fun.set_powercap_value(0, 0, power, &cpu_util);

    sleep(ttime);

    return 0;
}
