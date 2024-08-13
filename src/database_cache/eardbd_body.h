/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
