/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


/**
 * \file shared_configuration.h
 * \brief This file defines the API to create/attach/dettach/release the shared memory area between the EARD and the EARL.
 * It is used to communicate the EARLib updates remotelly requested.
*/


#ifndef _SHARED_PMON_H
#define _SHARED_PMON_H

#include <daemon/power_monitor_app.h>



/******************** NODE JOB LIST **********************/

/* This is the job of jobs the EARD is aware of */

state_t get_joblist_path(char *tmp, char *path);
uint  * create_joblist_shared_area(char *path, int *fd, uint *joblist, int joblist_elems);
uint  * attach_joblist_shared_area(char * path, int *fd, int *size);
void dettach_joblib_shared_area(int fd);
void joblist_shared_area_dispose(char *path, uint  *mem, int joblist_elems, int fd);

/* This is the pmapp are for a given job context */
state_t get_jobmon_path(char *tmp, uint ID, char *path);
powermon_app_t  * create_jobmon_shared_area(char *path, powermon_app_t  * pmapp, int *fd);
powermon_app_t  * attach_jobmon_shared_area(char * path, int *fd);
void dettach_jobmon_shared_area(int fd);
void jobmon_shared_area_dispose(char *path, powermon_app_t  *mem, int fd);


#endif
