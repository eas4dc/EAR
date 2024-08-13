/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EXTERNAL_H 
#define EXTERNAL_H

#include <common/types/types.h>

typedef struct ear_manager {
    int new_mask;
} ear_mgt_t;


ear_mgt_t *ear_connect();
state_t ear_disconnect();
ear_mgt_t *create_ear_external_shared_area(char *path);
ear_mgt_t *attach_ear_external_shared_area(char *path);
void dipose_ear_external_shared_area(char *path);
void dettach_ear_external_shared_area();
int get_ear_external_path(char *tmp, uint ID, char *path);

#endif
