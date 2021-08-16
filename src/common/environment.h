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

#include <common/states.h>

#define DEFAULT_LEARNING_PHASE          0
#define DEFAULT_RESET_FREQ              0
#define DEFAULT_VERBOSE                 0
#define DEFAULT_DB_PATHNAME             ".ear_system_db"
#define DEFAULT_USER_DB_PATHNAME        ".ear_user_db"
#define DEFAULT_COEFF_PATHNAME          "/etc/ear/coeffs"
#define DEFAULT_POWER_POLICY            0
#define DEFAULT_MIN_ENERGY_TH           0.1
#define DEFAULT_MIN_TIME_TH             PERFORMANCE_GAIN
#define DEFAULT_PERFORMANCE_ACURACY     10000000
#define DEFAULT_VERBOSE                 0
#define DEFAULT_MAX_P_STATE             1
#define DEFAULT_MIN_P_STATE             EAR_MIN_P_STATE
#define DEFAULT_P_STATE                 DEFAULT_MAX_P_STATE
#define DEFAULT_DYNAIS_LEVELS           10
#define DEFAULT_DYNAIS_WINDOW_SIZE      200
#define DYNAIS_TIMEOUT                  15

/** Tries to get the EAR_TMP environment variable's value and returns it.
*   If it fails, it defaults to TMP's value and then HOME's. */ 
char * getenv_ear_tmp();
/** Tries to get the EAR_INSTALL_PATH value and returns it.*/
char * getenv_ear_install_pathname();
/** Tries to get the EAR_DB_PATHNAME value and returns it.*/
char * getenv_ear_db_pathname();
/** Tries to get the EAR_USER_DB_PATHNAME value and returns it.*/
char * getenv_ear_user_db_pathname();
/** Tries to get the EAR_GUI_PATHNAME value and returns it.*/
char * getenv_ear_gui_pathname();
/** Tries to get the COEFF_DB_PATHNAME value and returns it.*/
char * getenv_ear_coeff_db_pathname();
/** Tries to get the EAR_APP_NAME value and returns it.*/
char * getenv_ear_app_name();
/** Sets the configuration's app name to the value recieved by parameter. */
void set_ear_app_name(char *name);
/** Checks if the EAR_LEARNING_PHASE environment variable is set. Returns
*   1 if it's set to 1, 0 in any other case. */ 
int getenv_ear_learning_phase();
/** Checks what the EAR_POWER_POLICY environment variable is set to. Returns 0
*   if it's not set or it's set to MIN_ENERGY_TO_SOLUTION or min_energy_to_solution,
*   1 if it's set to MIN_TIME_TO_SOLUTION or min_time_to_solution and 2 if it's set
*   to MONITORING_ONLY or monitoring_only. */
int getenv_ear_power_policy();
/** Reads the power threshold for the power policy currently set. If none is 
*   specified, it sets itself to the default for that power policy. 
*   getenv_ear_power_policy() is automatically called when using this function if it
*   had not been called before. */
double getenv_ear_power_policy_th();
/** Reads the EAR_RESET_FREQ environment variable's value. Returns 1 if it's set to
*   1, 0 otherwise */
int getenv_ear_reset_freq();
/** Reads the EAR_P_STATE environment variable's value and returns it if it's set, 
*   otherwise returns the default value for the current power policy.
*   getenv_ear_power_policy() is automatically called when using this function if it
*   had not been called before.*/
unsigned long getenv_ear_p_state();
/** Reads the PERFORMANCE_ACCURACY environment variable's value and returns it if
*   it's set, otherwise returns returns the default value. */
double getenv_ear_performance_accuracy();
/** Reads the VERBOSE's level set in the environment variable and returns its value if
*   it's a valid verbose level. Otherwise returns 0. */
int getenv_ear_verbose();
/** Returns -1. That value is computed later based on the number of total processes 
*   and EAR_NUM_NODES*/
int getenv_ear_local_id();
/** Reads the number of nodes from the EAR_NUM_NODES environment variable and returns
*   it. If it's not set, it tries to see if SLURM_STEP_NUM_NODES is set and returns it.
*   Otherwise returns 1. */
int getenv_ear_num_nodes();
/** Sets the number of nodes explicitly */
void set_ear_num_nodes(int num_nodes);
/** Reads the number of dynais levels in the DYNAIS_LEVELS environment variable and
*   returns it. If it's not set, defaults to 4 and returns. */
int getenv_ear_dynais_levels();
/** Reads the number of dynais window size in the EAR_DYNAIS_WINDOW_SIZE environment
*   variable and returns it. If it's not set, defaults to 300 and returns. */
int getenv_ear_dynais_window_size();

//  get_ functions must be used after the corresponding getenv_ function
/** Returns the tmp path previously read.*/
char * get_ear_tmp();
/** Changes the tmp path to the one recieved by parameter. */
void set_ear_tmp(char *new_tmp);
/** Returns the db path previously read. */
char * get_ear_db_pathname();
/** Returns the user db path previously read. */
char * get_ear_user_db_pathname();
/** Returns the gui path previously read. */
char * get_ear_gui_pathname();
/** Returns the coef db path previously read. */
char * get_ear_coeff_db_pathname();
/** Returns the app name previously read. */
char * get_ear_app_name();
/** Returns 1 if the app is currently set in the learning phase, 0 otherwise. */
int get_ear_learning_phase();
/** Returns the power policy currently set. */
int get_ear_power_policy();
/** Returns the current threshold for the current power policy. */
double get_ear_power_policy_th();
/** Returns the reset frequency value previously read. */
int get_ear_reset_freq();
/** Returns the current p_state. */
unsigned long get_ear_p_state();
/** Returns the performance accuracy previously set. */
double get_ear_performance_accuracy();
/** Returns the current verbosity level. */
int get_ear_verbose();
/** Sets the verbosity level to the one recieved by parameter. */
void set_ear_verbose(int verb);
/** Returns the number of nodes. */
int get_ear_num_nodes();
/** Sets the number of processes to the one recieved by parameter. */
void set_ear_total_processes(int procs);
/** Returns the current number of processes. */
int get_ear_total_processes();
/** Returns the number of dynais levels set. */
int get_ear_dynais_levels();
/** Returns the dynais window size. */
int get_ear_dynais_window_size();

/** Reads and process all the environment variables involving the library. */
void ear_lib_environment();
/** Writes ear library variables in $EAR_TMP/environment.txt file. */
void ear_print_lib_environment();

/** Returns the number of processes per node. */
int get_total_resources();



void set_ear_power_policy(int pid);
void set_ear_power_policy_th(double th);
void set_ear_p_state(ulong pstate);
void set_ear_coeff_db_pathname(char *path);
void set_ear_dynais_levels(int levels);
void set_ear_dynais_window_size(int size);
void set_ear_learning(int learning);


state_t read_config_env(char *var, const char* sched_env_var);
state_t read_config(uint *var, const char *config_var);



#else
#endif
