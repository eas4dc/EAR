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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef EAR_EARDBD_BODY_H
#define EAR_EARDBD_BODY_H

#include <common/output/verbose.h>

#define VERB_LEVEL 0 // Verbose level (deprecated)
#define VL0        0
#define VL2        VL0+2
#define line "---------------------------------------------------------------"
#define col1 "\x1b[35m"
#define col2 "\x1b[0m"

#define print_line(level) \
    verb(level, col1 line col2);

#define verb_who_noarg(format) \
    verb(VERB_LEVEL, "%s, %s", str_who[mirror_iam], format);

#define verb_who(format, ...) \
    verb(VERB_LEVEL, "%s, " format, str_who[mirror_iam], __VA_ARGS__);

#define verb_master(...) \
    if (!forked || master_iam) { \
        verb(VERB_LEVEL, __VA_ARGS__); \
    }

#define verb_master_line(...) \
    if (!forked || master_iam) { \
        verbosen(VERB_LEVEL, col1 line "\n" __VA_ARGS__); \
        verbosen(VERB_LEVEL, col2 "\n"); \
    }

void body();

void release();

void dream();

#endif //EAR_EARDBD_BODY_H
