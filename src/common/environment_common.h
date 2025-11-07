/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_ENVIRONMENT_COMMON_H
#define COMMON_ENVIRONMENT_COMMON_H

#define DEFAULT_VERBOSE     0
#define DEFAULT_DB_PATHNAME ".ear_system_db"

/** Tries to get the EAR_TMP environment variable's value and returns it.
 *   If it fails, it defaults to TMP's value and then HOME's. */
char *getenv_ear_tmp();
/** Tries to get the EAR_DB_PATHNAME value and returns it.*/
char *getenv_ear_db_pathname();
/** Reads the VERBOSE's level set in the environment variable and returns its value if
 *   it's a valid verbose level. Otherwise returns 0. */
int getenv_ear_verbose();
//  get_ functions must be used after the corresponding getenv_ function
/** Returns the install path previously read. */
char *get_ear_install_path();
/** Returns the tmp path previously read.*/
char *get_ear_tmp();
/** Changes the tmp path to the one recieved by parameter. */
void set_ear_tmp(char *new_tmp);
/** Returns the db path previously read. */
char *get_ear_db_pathname();
/** Returns the current verbosity level. */
int get_ear_verbose();
/** Sets the verbosity level to the one recieved by parameter. */
void set_ear_verbose(int verb);
/** Reads and process all the environment variables involving the daemon. */
void ear_daemon_environment();
/** Writes ear daemon variables in $EAR_TMP/environment.txt file. */
void ear_print_daemon_environment();

char *ear_getenv(const char *name);
#endif
