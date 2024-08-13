/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
