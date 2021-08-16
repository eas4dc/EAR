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

#ifndef _GPU_SUPPORT_H_
#define _GPU_SUPPORT_H_
#define _GNU_SOURCE
#include <common/includes.h>
#include <library/policies/policy_ctx.h>


state_t policy_gpu_load(settings_conf_t *app_settings,polsym_t *psyms);
state_t policy_gpu_init_ctx(polctx_t *c);
#endif

