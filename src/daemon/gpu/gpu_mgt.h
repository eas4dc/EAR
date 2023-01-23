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

#ifndef _GPU_NODE_MGR_H
#define _GPU_NODE_MGR_H

state_t gpu_mgr_init();
state_t gpu_mgr_set_freq(uint num_dev,ulong *freqs);
state_t gpu_mgr_set_freq_all_gpus(ulong gfreq);
uint    gpu_mgr_num_gpus();
state_t gpu_mgr_get_min(uint gpu, ulong *gfreq);
state_t gpu_mgr_get_max(uint gpu, ulong *gfreq);


#endif


