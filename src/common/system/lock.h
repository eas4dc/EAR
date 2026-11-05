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

#include <common/states.h>
#include <common/types/generic.h>
#include <pthread.h>
#include <stdint.h>

#define MAX_LOCK_INTENTS 1000

#define ear_lock(lock)   pthread_mutex_lock(lock);

#define ear_unlock(lock) pthread_mutex_unlock(lock);

state_t ear_trylock(pthread_mutex_t *lock);

/** Waits for the lock at most timeout_ms milliseconds. Returns EAR_SUCCESS with the lock
 * held, EAR_TIMEOUT if it was not released in time, EAR_ERROR on any other failure. For
 * locks held during slow operations (e.g. hardware writes), where ear_trylock gives up
 * too soon and pthread_mutex_lock could block forever. */
state_t ear_timedlock(pthread_mutex_t *lock, uint32_t timeout_ms);

#define return_unlock(state, lock)                                                                                     \
    {                                                                                                                  \
        ear_unlock(lock);                                                                                              \
        return state;                                                                                                  \
    }

#endif // COMMON_LOCK_H
