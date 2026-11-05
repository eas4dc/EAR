/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/system/lock.h>
#include <errno.h>
#include <time.h>

state_t ear_trylock(pthread_mutex_t *lock)
{
    int counter = 0;
    int result  = 0;

    while ((result = pthread_mutex_trylock(lock)) && (counter < MAX_LOCK_INTENTS)) {
        if (result != EBUSY)
            counter = MAX_LOCK_INTENTS - 1;
        counter++;
    }
    if (result && counter == MAX_LOCK_INTENTS) {
        return_msg(EAR_ERROR, Generr.lock);
    }
    return EAR_SUCCESS;
}

state_t ear_timedlock(pthread_mutex_t *lock, uint32_t timeout_ms)
{
    struct timespec deadline;
    /* pthread_mutex_timedlock takes an absolute CLOCK_REALTIME deadline */
    if (clock_gettime(CLOCK_REALTIME, &deadline)) {
        return_msg(EAR_ERROR, Generr.lock);
    }
    deadline.tv_sec += timeout_ms / 1000;
    deadline.tv_nsec += (long) (timeout_ms % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    int result = pthread_mutex_timedlock(lock, &deadline);
    if (result == ETIMEDOUT) {
        return_msg(EAR_TIMEOUT, "timeout waiting for mutex_lock");
    }
    if (result) {
        return_msg(EAR_ERROR, Generr.lock);
    }
    return EAR_SUCCESS;
}
