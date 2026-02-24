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

#define UID_ROOT ((uid_t) 0)
#define UID_REAL ((uid_t) - 1)

typedef struct user {
    char ruid_name[SZ_PATH_KERNEL];
    char euid_name[SZ_PATH_KERNEL];
    char rgid_name[SZ_PATH_KERNEL];
    char egid_name[SZ_PATH_KERNEL];
    uid_t ruid; // real user-id (the name of the user)
    uid_t euid; // effective user-id (the setuid of the binary)
    gid_t rgid; // real group-id (the group of the user)
    gid_t egid; // effective group-id (the setgrp of the binary)
} user_t;

/** Instantaneus root check */
int user_is_root();

/** Get complete user information */
state_t user_get_ids(user_t *user);

/** Set effective user id. **/
state_t user_set_euid(uid_t uid);

/** Set effective group id. **/
state_t user_set_egid(gid_t gid);

/** Get UID, GID for a given user */
state_t user_all_ids_by_user_get(user_t *user, char *username);

/** Checks the ruid and rgid and returns 1 if it's a privileged user, 0 if it's not. */
int is_privileged_command(cluster_conf_t *my_conf);

#endif // EAR_USER_H
