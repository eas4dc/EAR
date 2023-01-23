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

int lock_master(char *path_tmp, int job_id);

int master_getstep(int job_id, int step_id);

void unlock_slave(int job_id, int step_id);

void spinlock_slave(int job_id);

int slave_getstep(int job_id, int step_id);

void files_clean(int job_id, int step_id);

void folder_clean(int job_id);

void all_clean(char *path_tmp);
