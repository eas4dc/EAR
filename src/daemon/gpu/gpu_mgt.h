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

#ifndef _GPU_NODE_MGR_H
#define _GPU_NODE_MGR_H

state_t gpu_mgr_init();
state_t gpu_mgr_set_freq(uint num_dev,ulong *freqs);
state_t gpu_mgr_set_freq_all_gpus(ulong gfreq);
uint    gpu_mgr_num_gpus();


#endif


