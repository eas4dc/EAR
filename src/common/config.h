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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <common/config/config_def.h>
#include <common/config/config_dev.h>
#include <common/config/config_env.h>
#include <common/config/config_install.h>

/* These two options go together. USE_EXT defines if a automatic network
 * extension must be added for inter-nodes communications. Targeted to
 * architectures where hostname returned is not valid to communicate across
 * nodes. In that case, NW_EXT specified the extension to concatenate to
 * hostname */
#define USE_EXT								0
#define NW_EXT								"-opa"
/* When defined, activates dynamic traces on EARL */
#define EAR_GUI 1

#define USE_DB  DB_MYSQL || DB_PSQL

#endif
