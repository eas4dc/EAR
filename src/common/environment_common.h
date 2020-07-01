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
