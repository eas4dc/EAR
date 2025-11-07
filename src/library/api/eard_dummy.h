/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARD_DUMMY_H_
#include <common/states.h>
state_t create_eard_dummy_shared_regions(char *ear_tmp, uint ID);
void create_dummy_path_lock(char *ear_tmp, uint ID, char *dummy_areas_path, uint size);
void eard_dummy_replace_base_freq(ulong base);

#endif
