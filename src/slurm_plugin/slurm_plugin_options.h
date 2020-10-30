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

#ifndef EAR_SLURM_PLUGIN_OPTIONS_H
#define EAR_SLURM_PLUGIN_OPTIONS_H

int _opt_register(spank_t sp, int ac, char **av);
int _opt_ear (int val, const char *optarg, int remote);
int _opt_ear_learning (int val, const char *optarg, int remote);
int _opt_ear_policy (int val, const char *optarg, int remote);
int _opt_ear_frequency (int val, const char *optarg, int remote);
int _opt_ear_threshold (int val, const char *optarg, int remote);
int _opt_ear_user_db (int val, const char *optarg, int remote);
int _opt_ear_verbose (int val, const char *optarg, int remote);
int _opt_ear_traces (int val, const char *optarg, int remote);
int _opt_ear_mpi_dist (int val, const char *optarg, int remote);
int _opt_ear_tag (int val, const char *optarg, int remote);

#endif //EAR_SLURM_PLUGIN_OPTIONS_H
