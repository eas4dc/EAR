#ifndef _POWER_MONITOR_LOCK_H_
#define _POWER_MONITOR_LOCK_H_

#include <common/states.h>
#include <common/system/lock.h>
#include <common/types/generic.h>
#include <pthread.h>

typedef struct powermon_app powermon_app_t;

state_t powermon_context_trylock_impl(pthread_mutex_t *app_lock, pthread_mutex_t *context_mutex,
                                      powermon_app_t **contexts, uint cc, uint max_contexts, powermon_app_t **pmapp);

#endif
