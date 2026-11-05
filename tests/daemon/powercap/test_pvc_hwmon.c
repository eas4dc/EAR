/*********************************************************************
 * Copyright (c) 2026 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

/*
 * The PVC (i915 hwmon) GPU management backend, mgt_pvc_power_hwmon.c, against a
 * fake hwmon tree in a temporary directory. Built with ASan: every power
 * cap call must write one uint32_t per GPU, as mgt_gpu_ops_t declares.
 */
#include <stdint.h>
#include <sys/stat.h>

#include <common/states.h>
#include <management/gpu/archs/mgt_pvc_power_hwmon.h>
#include <metrics/common/hwmon_old.h>

#include "pc_test.h"

char pvc_test_root[256];

/* Two i915 hwmon devices: hwmon3 and hwmon5 */
static uint fake_ids[] = {3, 5};
#define NGPUS 2

state_t hwmon_find_drivers(const char *name, uint **ids, uint *n)
{
    if (ids != NULL) {
        *ids = malloc(sizeof(fake_ids));
        memcpy(*ids, fake_ids, sizeof(fake_ids));
    }
    if (n != NULL)
        *n = NGPUS;
    return EAR_SUCCESS;
}

state_t hwmon_open_files(uint id, char *files, int **fds, uint *n, int i_start)
{
    *fds    = malloc(sizeof(int));
    (*fds)[0] = -1;
    *n      = 1;
    return EAR_SUCCESS;
}

state_t hwmon_close_files(int *fds, uint n)
{
    free(fds);
    return EAR_SUCCESS;
}

static void put_file(uint id, const char *file, const char *value)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/hwmon%u", pvc_test_root, id);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/hwmon%u/%s", pvc_test_root, id, file);
    FILE *f = fopen(path, "w");
    fputs(value, f);
    fclose(f);
}

static mgt_gpu_ops_t ops;

static void load(void)
{
    memset(&ops, 0, sizeof(ops));
    CHECK_EQ(mgt_gpu_pvc_hwmon_load(&ops), EAR_SUCCESS, "PVC backend loads with %d i915 devices", NGPUS);
}

TEST(test_pvc_range_from_hwmon)
{
    /* GPU 0: rated 600 W, PL1 450 W. GPU 1: rated 550 W, no PL1 file */
    put_file(3, "power1_rated_max", "600000000\n");
    put_file(3, "power1_max", "450000000\n");
    put_file(5, "power1_rated_max", "550000000\n");
    load();

    /* exactly NGPUS slots: ASan flags any 8-byte (ulong) write per GPU */
    uint32_t *wmin = malloc(NGPUS * sizeof(uint32_t)), *wmax = malloc(NGPUS * sizeof(uint32_t));
    uint32_t *cur = malloc(NGPUS * sizeof(uint32_t)), *def = malloc(NGPUS * sizeof(uint32_t));
    CHECK_EQ(ops.power_cap_get_rank(NULL, wmin, wmax), EAR_SUCCESS, "get_rank");
    CHECK(wmin[0] == 0 && wmin[1] == 0, "no minimum from the driver: 0, unknown (%u %u)", wmin[0], wmin[1]);
    CHECK(wmax[0] == 600 && wmax[1] == 550, "max = power1_rated_max per device (%u %u)", wmax[0], wmax[1]);
    ops.power_cap_get_default(NULL, def);
    CHECK(def[0] == 600 && def[1] == 550, "default = rated limit (%u %u)", def[0], def[1]);
    ops.power_cap_get_current(NULL, cur);
    CHECK(cur[0] == 450, "current = power1_max (PL1) when readable (%u)", cur[0]);
    CHECK(cur[1] == 550, "... the rated limit otherwise (%u)", cur[1]);
    CHECK_EQ(ops.power_cap_get_rank(NULL, NULL, wmax), EAR_SUCCESS, "NULL min accepted");
    free(wmin), free(wmax), free(cur), free(def);
}

TEST(test_pvc_no_power_files)
{
    /* per-tile hwmon directories expose energy only: fall back to the nominal 600 W */
    load();
    uint32_t *wmin = malloc(NGPUS * sizeof(uint32_t)), *wmax = malloc(NGPUS * sizeof(uint32_t));
    ops.power_cap_get_rank(NULL, wmin, wmax);
    CHECK(wmax[0] == 600 && wmax[1] == 600, "unreadable rated limit: nominal 600 W max (%u %u)", wmax[0], wmax[1]);
    CHECK(wmin[0] == 0 && wmin[1] == 0, "... and min 0, not 600 (%u %u)", wmin[0], wmin[1]);
    free(wmin), free(wmax);
}

int main(void)
{
    char tmpl[] = "/tmp/pc_pvc_XXXXXX";
    if (mkdtemp(tmpl) == NULL)
        return 2;
    snprintf(pvc_test_root, sizeof(pvc_test_root), "%s/a", tmpl);
    mkdir(pvc_test_root, 0755);
    RUN_ISOLATED(test_pvc_range_from_hwmon);
    snprintf(pvc_test_root, sizeof(pvc_test_root), "%s/b", tmpl);
    mkdir(pvc_test_root, 0755);
    RUN_ISOLATED(test_pvc_no_power_files);
    char cmd[300];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", tmpl);
    if (system(cmd) != 0)
        fprintf(stderr, "could not remove %s\n", tmpl);
    return pc_summary("test_pvc_hwmon");
}
