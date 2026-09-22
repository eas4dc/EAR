#include <daemon/power_monitor_lock.h>

state_t powermon_context_trylock_impl(pthread_mutex_t *app_lock, pthread_mutex_t *context_mutex,
                                      powermon_app_t **contexts, uint cc, uint max_contexts, powermon_app_t **pmapp)
{
    *pmapp = NULL;

    if (pthread_mutex_trylock(app_lock) != 0) {
        return EAR_ERROR;
    }

    if ((cc >= max_contexts) || (contexts[cc] == NULL)) {
        pthread_mutex_unlock(app_lock);
        return EAR_ERROR;
    }

    if (pthread_mutex_trylock(&context_mutex[cc]) != 0) {
        pthread_mutex_unlock(app_lock);
        return EAR_ERROR;
    }

    *pmapp = contexts[cc];

    pthread_mutex_unlock(app_lock);

    return EAR_SUCCESS;
}
