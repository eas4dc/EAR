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

#if !SHOW_DEBUGS
#define NDEBUG
#endif
#include <assert.h>

#include <library/common/utils.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/output/verbose.h>

state_t utils_create_plugin_path(char *result_path, char *install_path, char *custom_path, int authorized_user) {
    debug("install path: %s | custom path: %s | Authorised ? %d", install_path, custom_path, authorized_user == AUTHORIZED);
    int ret;
    if (custom_path != NULL && authorized_user == AUTHORIZED) {
        ret = sprintf(result_path, "%s/plugins", custom_path);
    } else {
        assert(install_path != NULL);
        if (install_path == NULL) return EAR_ERROR;
        ret = sprintf(result_path, "%s", install_path);
    }
    assert(ret > 0);
    state_t st = ret > 0 ? EAR_SUCCESS : EAR_ERROR;
    return st;
}
