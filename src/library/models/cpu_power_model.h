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
#ifndef _CPU_POWER_MODEL_H_
#define _CPU_POWER_MODEL_H_
#include <common/states.h>

#include <common/hardware/architecture.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/shared_configuration.h>
#include <library/common/library_shared_data.h>


state_t cpu_power_model_load(settings_conf_t *libconf,conf_install_t *data,architecture_t *arch_desc, uint API);
state_t cpu_power_model_init();
state_t cpu_power_model_status();
state_t cpu_power_model_project(lib_shared_data_t *data,shsignature_t *sig, node_mgr_sh_data_t *nmgr);

#endif



