/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_USER_H
#define EAR_USER_H

#include <common/sizes.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

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

#endif // EAR_USER_H
