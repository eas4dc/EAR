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

#ifndef COMMON_LOCK_H
#define COMMON_LOCK_H

#include <pthread.h>
#include <common/states.h>
#include <common/types/generic.h>

#define MAX_LOCK_INTENTS 1000000000

#define ear_lock(lock) \
    pthread_mutex_lock(lock);

#define ear_unlock(lock) \
    pthread_mutex_unlock(lock);

state_t ear_trylock(pthread_mutex_t *lock);

#define return_unlock(state, lock) \
    { ear_unlock(lock); \
    return state; }

#endif //COMMON_LOCK_H