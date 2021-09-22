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

#include <library/common/utils.h>
#include <common/types/configuration/cluster_conf.h>

state_t utils_create_report_plugin_path(char *result_path, char *install_path, char *custom_path, int authorized_user){
    if (custom_path != NULL && authorized_user == AUTHORIZED){
       sprintf(result_path, "%s/plugins", custom_path);
    }else{
        if (install_path == NULL) return EAR_ERROR;
        sprintf(result_path, "%s", install_path);
    }
    return EAR_SUCCESS;
}
