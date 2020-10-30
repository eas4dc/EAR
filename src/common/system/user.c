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

#include <string.h>
#include <common/system/user.h>
#include <common/output/verbose.h>

int user_is_root()
{
	return getuid() == 0;
}

state_t user_all_ids_get(user_t *user)
{
	if (state_fail(user_ruid_get(&user->ruid, user->ruid_name))) return EAR_ERROR;
	if (state_fail(user_euid_get(&user->euid, user->euid_name))) return EAR_ERROR;
	if (state_fail(user_rgid_get(&user->rgid, user->rgid_name))) return EAR_ERROR;
	if (state_fail(user_rgid_get(&user->egid, user->egid_name))) return EAR_ERROR;
	return EAR_SUCCESS;
}

state_t user_ruid_get(uid_t *uid, char *uname)
{
	struct passwd *upw;
	*uid = getuid();
	upw = getpwuid(*uid);
	if (upw == NULL) return EAR_ERROR;
	strcpy(uname, upw->pw_name);
	return EAR_SUCCESS;
}

state_t user_euid_get(uid_t *uid, char *uname)
{
	struct passwd *upw;
	*uid = geteuid();
	upw = getpwuid(*uid);
	if (upw == NULL) return EAR_ERROR;
	strcpy(uname, upw->pw_name);
	return EAR_SUCCESS;
}

state_t user_rgid_get(gid_t *gid, char *gname)
{
	static struct group *gpw;
	*gid = getgid();
	gpw = getgrgid(*gid);
	if (gpw == NULL) return EAR_ERROR;
	strcpy(gname, gpw->gr_name);
	return EAR_SUCCESS;
}

state_t user_egid_get(gid_t *gid, char *gname)
{
	static struct group *gpw;
	*gid = getegid();
	gpw = getgrgid(*gid);
	if (gpw == NULL) return EAR_ERROR;
	strcpy(gname, gpw->gr_name);
	return EAR_SUCCESS;
}

int is_privileged_command(cluster_conf_t *my_conf)
{
    user_t user_info;
    if (user_all_ids_get(&user_info) != EAR_SUCCESS)
    {
        warning("Failed to retrieve user data\n");
        return 0;
    }

    return is_privileged(my_conf, user_info.ruid_name, user_info.rgid_name, NULL);
}
