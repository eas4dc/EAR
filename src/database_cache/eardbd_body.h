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

#ifndef EAR_EARDBD_BODY_H
#define EAR_EARDBD_BODY_H

#include <common/output/verbose.h>

#define line "---------------------------------------------------------------"
#define col1 "\x1b[35m"
#define col2 "\x1b[0m"

#define print_line() \
		verb(0, col1 line col2);

#define verb_who_noarg(format) \
	verb(0, "%s, %s", str_who[mirror_iam], format);

#define verb_who(format, ...) \
	verb(0, "%s, " format, str_who[mirror_iam], __VA_ARGS__);

#define verb_master(...) \
    if (!forked || master_iam) { \
		verb(0, __VA_ARGS__); \
    }

#define verb_master_line(...) \
    if (!forked || master_iam) { \
        dprintf(verb_channel, col1 line "\n" __VA_ARGS__); \
        dprintf(verb_channel, col2 "\n"); \
	}

void body();

void release();

void dream();

#endif //EAR_EARDBD_BODY_H
