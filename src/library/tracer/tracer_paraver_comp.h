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

#ifndef _EAR_TRACES_COMP_H_
#define _EAR_TRACES_COMP_H_

#define EV_AVG_CPUFREQ_KHZ 3
#define EV_AVG_IMCFREQ_KHZ 4
#define EV_DEF_FREQ_KHZ    5
#define EV_ITER_TIME_SEC   6
#define EV_CPI             7
#define EV_TPI             8
#define EV_MEM_GBS         9
#define EV_IO_MBS         10
#define EV_PERC_MPI       11
#define EV_DC_NODE_POWER_W 12
#define EV_DRAM_POWER_W    13
#define EV_PCK_POWER_W     14
#define EV_GFLOPS          17
#define EV_gpu_power       33
#define EV_gpu_freq        34
#define EV_gpu_mem_freq    35
#define EV_gpu_util        36
#define EV_gpu_mem_util    37


#define USE_TRACER_COMP

#ifdef USE_TRACER_COMP

#define TRA_CPI   EV_CPI
#define TRA_TPI   EV_TPI
#define TRA_GBS   EV_MEM_GBS
#define TRA_POW   EV_DC_NODE_POWER_W
#define TRA_FRQ   EV_DEF_FREQ_KHZ
#else
#define TRA_CPI   60005
#define TRA_TPI   60006
#define TRA_GBS   60007
#define TRA_POW   60008
#define TRA_FRQ   60012
#endif // USE_TRACER_COMP

#define TRA_ID    60001
#define TRA_LEN   60002
#define TRA_ITS   60003
#define TRA_TIM   60004
#define TRA_PTI   60009
#define TRA_PCP   60010
#define TRA_PPO   60011
#define TRA_DYN   60014
#define TRA_STA   60015
#define TRA_MOD   60016
#define TRA_REC   60018
#define TRA_ENE   60013
#define TRA_VPI   60017
#define TRA_PCK_POWER_NODE  60018
#define TRA_DRAM_POWER_NODE 60020
#define TRA_BARRIER         60022
#define TRA_CPUF_RAMP_UP    60024

#define TRA_AVGFRQ EV_AVG_CPUFREQ_KHZ
#define TRA_PCK_POWER  EV_PCK_POWER_W
#define TRA_DRAM_POWER EV_DRAM_POWER_W
#define TRA_IO          EV_IO_MBS
#define TRA_GFLOPS      EV_GFLOPS
#define TRA_JOB_POWER   JOB_POWER
#define TRA_NODE_POWER  NODE_POWER
#endif // _EAR_TRACES_COMP_H_
