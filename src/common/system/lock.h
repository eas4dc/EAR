/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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