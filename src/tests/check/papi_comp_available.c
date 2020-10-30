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

#include <assert.h>
#include <papi.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int state = PAPI_library_init(PAPI_VER_CURRENT);
    assert(state == PAPI_VER_CURRENT);

    state = PAPI_get_component_index(argv[1]);
    assert(state >= 0);

    return 0;
}
