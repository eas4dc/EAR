/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <daemon/eard_checkpoint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern char nodename[MAX_PATH_SIZE];
extern ulong eard_min_pstate;

void save_eard_conf(eard_dyn_conf_t *eard_dyn_conf)
{
}

void restore_eard_conf(eard_dyn_conf_t *eard_dyn_conf)
{
}
