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

#include <common/types/gm_warning.h>
#include <string.h>
#include <common/types/generic.h>
#define NODE_SIZE 256


/** Replicates the periodic_metric in *source to *destiny */
void copy_gm_warning(gm_warning_t *destiny, gm_warning_t *source)
{
    memcpy(destiny, source, sizeof(gm_warning_t));
}

/** Initializes all values of the gm_warning to */
void init_gm_warning(gm_warning_t *gmw)
{
    memset(gmw, 0, sizeof(gm_warning_t));
}


