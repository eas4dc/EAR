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

#ifndef _EAR_FD_SETS_H_
#define _EAR_FD_SETS_H_
#include <common/states.h>
void clean_fd_set(fd_set *set,int *total);
state_t new_fd(fd_set *set,int fd,int *max,int *total);
state_t clean_fd(fd_set *set,int fd,int *max,int *total);
state_t check_fd(fd_set *set,int fd);
#endif

