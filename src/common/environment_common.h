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

#ifndef _EAR_CONFIGURATION_H_
#define _EAR_CONFIGURATION_H_

#define DEFAULT_VERBOSE                 0
#define DEFAULT_DB_PATHNAME             ".ear_system_db"

/** Tries to get the EAR_TMP environment variable's value and returns it.
*   If it fails, it defaults to TMP's value and then HOME's. */ 
char * getenv_ear_tmp();
/** Tries to get the EAR_DB_PATHNAME value and returns it.*/
char * getenv_ear_db_pathname();
/** Reads the VERBOSE's level set in the environment variable and returns its value if
*   it's a valid verbose level. Otherwise returns 0. */
int getenv_ear_verbose();
//  get_ functions must be used after the corresponding getenv_ function
/** Returns the install path previously read. */
char *get_ear_install_path();
/** Returns the tmp path previously read.*/
char * get_ear_tmp();
/** Changes the tmp path to the one recieved by parameter. */
void set_ear_tmp(char *new_tmp);
/** Returns the db path previously read. */
char * get_ear_db_pathname();
/** Returns the current verbosity level. */
int get_ear_verbose();
/** Sets the verbosity level to the one recieved by parameter. */
void set_ear_verbose(int verb);
/** Reads and process all the environment variables involving the daemon. */
void ear_daemon_environment();
/** Writes ear daemon variables in $EAR_TMP/environment.txt file. */
void ear_print_daemon_environment();

#else
#endif
