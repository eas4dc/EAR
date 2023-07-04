/*
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

// #define SHOW_DEBUGS 1

#include <pthread.h>

#include <common/states.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <common/types/generic.h> 
#include <common/output/verbose.h>
#include <common/system/monitor.h>
#include <metrics/common/redfish.h>
#include <metrics/energy/node/energy_node.h>


#define MAX_LOCK_TRIES  1000000000
#define MIN_READ_PERIOD 2000       /*!< The minimum period time to for the periodic thread to read the power. */


static pthread_mutex_t ompi_lock  = PTHREAD_MUTEX_INITIALIZER;


#define try_lock() \
    int etries = 0, lret; \
    while ((lret = pthread_mutex_trylock(&ompi_lock)) && (etries < MAX_LOCK_TRIES)) \
    { \
        etries++; \
    } \
    if ((etries == MAX_LOCK_TRIES) && lret) \
    { \
        return_msg(EAR_ERROR, Generr.lock); \
    } \


/* For error control */
#define VPOWER_ERROR 1


/* Monitor thread control */
static       ulong          redfish_timeframe  = MIN_READ_PERIOD;
static       suscription_t *redfish_sus;
static       int            monitor_done;

/* Energy computation control */
static       ulong          accumulated_energy;
static       llong          last_power_read;
static       timestamp_t    last_timestamp;

/* Redfish API */
static const char          *redfish_host       = "https://169.254.95.118";
static const char          *redfish_user       = "monitor";
static const char          *redfish_pass       = "Monitorme1";
static const char          *redfish_query      = "/Chassis[0]/Power/PowerControl[0]/PowerConsumedWatts";
static const char          *redfish_read_err   = "Reading power through Redfish API.";

/** This function calls the REDFISH API to get the current Power
 * and updates the accumulated energy. Thread-safe.
 * You can use state_msg for error-checking.
 *
 * \param[out] energy If non-NULL, the address pointed is filled with the accumulated energy just aupdated.
 * \param[out] time_ms If non-NULL, the address pointed is filled with the elapsed time (in miliseconds)
 *
 * \return EAR_ERROR if the pthread mutex is locked for more than MAX_LOCK_TRIES.
 * \return EAR_ERROR if Redfish API returns an error on power reading.
 * \return EAR_ERROR if the power computed is less or equal than zero.
 * \return EAR_SUCCESS otherwise. */
static state_t compute_energy(ulong *energy, ulong *time_ms);


/** The monitor thread's periodic function. It is used to periodically accumulate the energy.
 * If there is some error computing the energy, the function tries to init again the Redfish API.
 *
 * \param p Not used.
 *
 * \return On error computing the energy, returns whatever redfish_thread_init returns.
 * \return EAR_SUCCESS otherwise. */
static state_t redfish_thread_main(void *p);


/** The monitor thread's main (initial) function. It is basically used to open a connection with the Redfish API.
 * It also updates (always) the last_timestamp member.
 *
 * \param p Not used.
 *
 * \return EAR_SUCCESS if the Redfish API connection succeed, EAR_ERROR otherwise. */
static state_t redfish_thread_init(void *p);


state_t energy_init(void **c)
{
    if (!c)
    {
        return_msg(EAR_ERROR, Generr.input_null);
    }

    try_lock();

    if (state_fail(redfish_open((char *) redfish_host, (char *) redfish_user, (char *) redfish_pass)))
    {
        pthread_mutex_unlock(&ompi_lock);
        return EAR_ERROR;
    }

    llong redfish_pwr_out;
    if (state_fail(redfish_read((char *) redfish_query, (void *) &redfish_pwr_out,
                                NULL, NULL, REDTYPE_LLONG)))
    {
        pthread_mutex_unlock(&ompi_lock);
        return EAR_ERROR;
    }

    timestamp_get(&last_timestamp);
    last_power_read = redfish_pwr_out;

    if (!monitor_done)
    {
        redfish_sus = suscription();
        redfish_sus->call_main = redfish_thread_main;
        redfish_sus->call_init = redfish_thread_init;
        redfish_sus->time_relax = ear_max(redfish_timeframe, MIN_READ_PERIOD);
        redfish_sus->time_burst = ear_max(redfish_timeframe, MIN_READ_PERIOD);
        redfish_sus->suscribe(redfish_sus);
        debug("redfish energy plugin suscription initialized with timeframe %lu ms", redfish_timeframe);
        monitor_done = 1;
    }

    pthread_mutex_unlock(&ompi_lock);

	verbose(2, "REDFISH Init ok");

    return EAR_SUCCESS;
}


state_t energy_dc_read(void *c, edata_t energy_mj)
{
    debug("energy_dc_read");

    state_t st = EAR_SUCCESS;

    if (state_fail(compute_energy((ulong *) energy_mj, NULL)))
    {
        verbose(VPOWER_ERROR + 1, "%sERROR%s Computing node energy: %s", COL_RED, COL_CLR, state_msg);
        st = EAR_ERROR;
    }

    return st;
}


state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms) 
{
    debug("energy_dc_time_read");

    state_t st = EAR_SUCCESS;

    if (state_fail(compute_energy((ulong *) energy_mj, time_ms)))
    {
        verbose(VPOWER_ERROR + 1, "%sERROR%s Computing node energy: %s", COL_RED, COL_CLR, state_msg);
        st = EAR_ERROR;
    }

    return st;
}


state_t energy_dispose(void **c)
{
    return EAR_SUCCESS;
}


state_t energy_datasize(size_t *size)
{
	debug("energy_datasize %lu\n", sizeof(ulong));

	*size = sizeof(ulong);

	return EAR_SUCCESS;
}


state_t energy_frequency(ulong *freq_us)
{
	*freq_us = redfish_timeframe;
	return EAR_SUCCESS;
}


state_t energy_to_str(char *str, edata_t e)
{
        ulong *pe = (ulong *) e;
        sprintf(str, "%lu", *pe);
        return EAR_SUCCESS;
}


unsigned long diff_node_energy(ulong init, ulong end)
{
  ulong ret = 0;
  if (end >= init) {
    ret = end-init;
  } else {
    ret = ulong_diff_overflow(init, end);
  }
  return ret;
}


state_t energy_units(uint *units)
{
  *units = 1000;
  return EAR_SUCCESS;
}


state_t energy_accumulated(unsigned long *e, edata_t init, edata_t end)
{
  ulong *pinit = (ulong *) init, *pend = (ulong *) end;

  ulong total = diff_node_energy(*pinit, *pend);
  *e = total;
  return EAR_SUCCESS;
}


uint energy_data_is_null(edata_t e)  
{
    ulong *pe=(ulong *)e;
    return (*pe == 0);

}


static state_t compute_energy(ulong *energy, ulong *time_ms)
{
    try_lock();

    llong redfish_pwr_out;
    if (state_fail(redfish_read((char *) redfish_query, (void *) &redfish_pwr_out,
                                NULL, NULL, REDTYPE_LLONG)))
    {
        pthread_mutex_unlock(&ompi_lock);
        return_msg(EAR_ERROR, (char *) redfish_read_err);
    }

    // Read
    timestamp_t   curr_time;
    timestamp_get(&curr_time);

    // Diff
    ullong redfish_elapsed;
    redfish_elapsed = timestamp_diff(&curr_time, &last_timestamp, TIME_SECS);

    // Update
    last_timestamp = curr_time;

    state_t return_st = EAR_SUCCESS;

    if (redfish_pwr_out <= 0)
    {
        verbose(VPOWER_ERROR, "%sWARNING%s Current power read: %lld W. Using last power read: %lld W",
                COL_YLW, COL_CLR, redfish_pwr_out, last_power_read);

        redfish_pwr_out = last_power_read;

        return_st = EAR_ERROR;
    }

    ulong current_elapsed = (ulong) redfish_elapsed;
    ulong current_energy = current_elapsed * redfish_pwr_out;

    /* Energy is reported in MJ */
    accumulated_energy += (current_energy * 1000);

    if (energy)
    {
        *energy = accumulated_energy;
    }
    if (time_ms)
    {
        *time_ms = (ulong) timestamp_convert(&last_timestamp, TIME_MSECS);
    }

    debug("inst. power %lu W / elapsed %lu s / energy %lu J / acc. energy %lu MJ",
          redfish_pwr_out, current_elapsed, current_energy, accumulated_energy);		

    pthread_mutex_unlock(&ompi_lock);

    // state_msg will only be used in case of EAR_ERROR.
    return_msg(return_st, (char *) redfish_read_err);
}


static state_t redfish_thread_main(void *p)
{
    if (state_fail(compute_energy(NULL, NULL)))
    {
        verbose(VPOWER_ERROR + 1, "%sERROR%s Computing node energy: %s", COL_RED, COL_CLR, state_msg);
	    return redfish_thread_init(NULL);
    }
    return EAR_SUCCESS;
}


static state_t redfish_thread_init(void *p)
{
    if (state_fail(redfish_open((char *) redfish_host,
                                (char *) redfish_user,
                                (char *) redfish_pass)))
    {
        return EAR_ERROR;
    }

    timestamp_get(&last_timestamp);

    debug("thread_init for redfish power OK");
    return EAR_SUCCESS;
}
