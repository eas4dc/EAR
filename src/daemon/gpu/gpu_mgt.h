/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _GPU_NODE_MGR_H
#define _GPU_NODE_MGR_H

state_t gpu_mgr_init();
state_t gpu_mgr_set_freq(uint num_dev,ulong *freqs);
state_t gpu_mgr_set_freq_all_gpus(ulong gfreq);
uint    gpu_mgr_num_gpus();
state_t gpu_mgr_get_min(uint gpu, ulong *gfreq);
state_t gpu_mgr_get_max(uint gpu, ulong *gfreq);


#endif


