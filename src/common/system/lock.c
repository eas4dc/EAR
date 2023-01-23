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

#include <common/system/lock.h>

state_t ear_trylock(pthread_mutex_t *lock)
{
    int counter = 0;
    int result  = 0;

    while((result = pthread_mutex_trylock(lock)) && (counter < MAX_LOCK_INTENTS)) {
        counter++;
    }
    if (result && counter == MAX_LOCK_INTENTS) {
        return_msg(EAR_ERROR, Generr.lock);
    }
    return EAR_SUCCESS;
}