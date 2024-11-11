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

state_t ear_trylock(pthread_mutex_t *lock)
{
    int counter = 0;
    int result  = 0;

    while((result = pthread_mutex_trylock(lock)) && (counter < MAX_LOCK_INTENTS)) {
				if (result != EBUSY) counter = MAX_LOCK_INTENTS -1 ;
        counter++;
    }
    if (result && counter == MAX_LOCK_INTENTS) {
        return_msg(EAR_ERROR, Generr.lock);
    }
    return EAR_SUCCESS;
}
