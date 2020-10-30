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

#ifndef _SHARED_AREAS_H
#define _SHARED_AREAS_H

void *create_shared_area(char *path,char *data,int area_size,int *shared_fd,int must_clean);
void * attach_shared_area(char *path,int area_size,uint perm,int *shared_fd,int *s);
void dettach_shared_area(int fd);
void dispose_shared_area(char *path,int fd);
#endif
