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
 * EAR is an open source software, and it is licensed under both the BSD-3 license
 * and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
 * and COPYING.EPL files.
 */

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
