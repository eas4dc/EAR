/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/system/user.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>

int user_is_root()
{
	return getuid() == 0;
}

// Returns the uid, gid for a given username
state_t user_all_ids_by_user_get(user_t *user, char *username)
{

    struct passwd *pw = getpwnam(username);
    if (!pw) {
        return EAR_ERROR;
    }

    user->ruid = pw->pw_uid;
    user->rgid = pw->pw_gid;
    return EAR_SUCCESS;
}

static int user_get_uid(uid_t uid, uid_t *id, char *name)
{
    struct passwd *pwd = getpwuid(uid);
    if (pwd == NULL) return 0;
    strcpy(name, pwd->pw_name);
    *id = uid;
    return 1;
}

static int user_get_gid(gid_t gid, gid_t *id, char *name)
{
    struct group *pwd = getgrgid(gid);
    if (pwd == NULL) return 0;
    strcpy(name, pwd->gr_name);
    *id = gid;
    return 1;
}

state_t user_get_ids(user_t *user)
{
    if (!user_get_uid(getuid() , &user->ruid, user->ruid_name)) return EAR_ERROR;
    if (!user_get_uid(geteuid(), &user->euid, user->euid_name)) return EAR_ERROR;
    if (!user_get_gid(getgid() , &user->rgid, user->rgid_name)) return EAR_ERROR;
    if (!user_get_gid(getegid(), &user->egid, user->egid_name)) return EAR_ERROR;
    debug("user.ruid %4d: %s", user->ruid, user->ruid_name);
    debug("user.euid %4d: %s", user->euid, user->euid_name);
    return EAR_SUCCESS;
}

state_t user_set_euid(uid_t uid)
{
	user_t user = {0};
	state_t s;

	if (uid == UID_REAL) {
		if (state_fail(s = user_get_ids(&user))) {
			return s;
		}
		uid = user.ruid;
	}
    if (seteuid(uid) != 0) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

state_t user_set_egid(gid_t gid)
{
    if (setegid(gid) != 0) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

int is_privileged_command(cluster_conf_t *my_conf)
{
    user_t user_info;
    if (user_get_ids(&user_info) != EAR_SUCCESS)
    {
        warning("Failed to retrieve user data\n");
        return 0;
    }

    return is_privileged(my_conf, user_info.ruid_name, user_info.rgid_name, NULL);
}
