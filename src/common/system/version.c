/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <common/system/version.h>

void version_set(version_t *v, uint major, uint minor)
{
    v->minor = minor;
    v->major = major;
    v->hash  = (major << 16) | (minor);
    sprintf(v->str, "%u.%u", major, minor);
}

int version_is(int symbol, version_t *v, version_t *vtc)
{
    if (symbol == VERSION_GT) { return v->hash >  vtc->hash; }
    if (symbol == VERSION_GE) { return v->hash >= vtc->hash; }
    if (symbol == VERSION_LT) { return v->hash <  vtc->hash; }
    if (symbol == VERSION_LE) { return v->hash <= vtc->hash; }
    if (symbol == VERSION_EQ) { return v->hash == vtc->hash; }
    return 0;
}