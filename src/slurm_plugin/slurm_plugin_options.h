/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
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
