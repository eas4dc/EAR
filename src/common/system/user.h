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

#ifndef EAR_USER_H
#define EAR_USER_H

#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>

typedef struct user {
	char ruid_name[SZ_PATH_KERNEL];
	char euid_name[SZ_PATH_KERNEL];
	char rgid_name[SZ_PATH_KERNEL];
	char egid_name[SZ_PATH_KERNEL];
	uid_t ruid;
	uid_t euid;
	gid_t rgid;
	gid_t egid;
} user_t;

/** Instantaneus root check */
int user_is_root();

/** Get complete user information */
state_t user_all_ids_get(user_t *user);

/** Get real user id */
state_t user_ruid_get(uid_t *uid, char *uname);

/** Get effective user id */
state_t user_euid_get(uid_t *uid, char *uname);

/** Get real group id */
state_t user_rgid_get(gid_t *gid, char *gname);

/** Get effective group id */
state_t user_egid_get(gid_t *gid, char *gname);

/** Checks the ruid and rgid and returns 1 if it's a privileged user, 0 if it's not. */
int is_privileged_command(cluster_conf_t *my_conf);

#endif //EAR_USER_H
