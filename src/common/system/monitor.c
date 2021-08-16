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

//#define SHOW_DEBUGS 1
#include <error.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>

#define N_QUEUE 128

static pthread_t        thread;
static pthread_mutex_t  lock_gen = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  lock[N_QUEUE];
static uint             enabled = 0;
static uint             locked = 0;

typedef struct wait_s {
	int relax;
	int burst;
} wait_t;

typedef struct register_s
{
	suscription_t suscription;
	wait_t wait_units;
	wait_t wait_saves;
	int	suscribed;
	int	delivered;
	int	bursted;
	int	aligned;
	int	ok_init;
	int	ok_main;
} register_t;

static register_t queue[N_QUEUE];
static uint       queue_last;

static void monitor_sleep(int wait_units, int *pass_units, int *alignment)
{
	timestamp_t time;
	ullong units;

	//
	units = ((ullong) wait_units) * 100LL;
	timestamp_revert(&time, &units, TIME_MSECS);

	// Sleeping
	timestamp_t t2;
	timestamp_t t1;

	debug("waiting %llu units", units);

	timestamp_getfast(&t1);
	nanosleep(&time, NULL);
	timestamp_getfast(&t2);

	// If s is 0 then all want ok and the time
	// processing continues normally.
	*pass_units = ((int) timestamp_diff(&t2, &t1, TIME_MSECS)) + 1;
	*pass_units = *pass_units / 100;

	*alignment += *pass_units;
	*alignment %= 10;

	debug("waited %d units (alignment %d)", *pass_units, *alignment);
}

static void monitor_time_calc(register_t *reg, int *wait_units, int pass_units, int alignment)
{
	int _bursted = reg->bursted;
	int wait_required;

	// Alignment processing
	if (!reg->aligned)
	{
		reg->aligned = (alignment == 0);
		reg->ok_init = reg->aligned && (reg->suscription.call_init != NULL);
		reg->ok_main = reg->aligned;

		if (!reg->ok_main) {
			wait_required = 10 - alignment;

			if (wait_required < *wait_units) {
				*wait_units = wait_required;
			}
		
			return;
		}

		debug("sus %d, aligned suscription", reg->suscription.id);
	}

	// Normal processing
	reg->wait_units.burst -= pass_units;
	reg->wait_units.relax -= pass_units;

	if (reg->wait_units.burst <= 0 &&  _bursted) reg->ok_main = 1;
	if (reg->wait_units.relax <= 0 && !_bursted) reg->ok_main = 1;

	if (reg->wait_units.burst <= 0) reg->wait_units.burst = reg->wait_saves.burst;
	if (reg->wait_units.relax <= 0) reg->wait_units.relax = reg->wait_saves.relax;

	if (_bursted && reg->wait_units.burst < *wait_units) {
		*wait_units = reg->wait_units.burst;
	} else 	if (!_bursted && reg->wait_units.relax < *wait_units) {
		*wait_units = reg->wait_units.relax;
	}
}

static void _monitor_init()
{
        sigset_t set;
        sigfillset(&set);
        sigdelset(&set, SIGALRM);
	pthread_sigmask(SIG_SETMASK, &set, NULL);
}

static void *ear_internal_monitor(void *p)
{
	register_t *reg;
	suscription_t *sus;
	int wait_units = 0;
	int pass_units = 0;
	int alignment = 0;
	int i;
	//verbose(2,"Monitor init queue_last %d enabled %d",queue_last,enabled);

	// Initializing
	_monitor_init();

	while (enabled)
	{
		for (i = 0, wait_units = 10; i < queue_last && enabled; ++i)
		{
			while (pthread_mutex_trylock(&lock[i]));

			reg = &queue[i];
			sus = &reg->suscription;

			if (!reg->suscribed) {
				pthread_mutex_unlock(&lock[i]);
				continue;
			}

			monitor_time_calc(reg, &wait_units, pass_units, alignment);

			if (reg->ok_init) {
				debug("sus %d, called init", i);
				sus->call_init(sus->memm_init);
				reg->ok_init = 0;
			}
			if (reg->ok_main) {
				debug("sus %d, called main", i);
				sus->call_main(sus->memm_main);
				reg->ok_main = 0;
			}

			pthread_mutex_unlock(&lock[i]);
		}

		monitor_sleep(wait_units, &pass_units, &alignment);
	}
	return NULL;
}

static state_t monitor_lock_alloc()
{
	int error = 0;
	int i;

	for (i = 0; i < N_QUEUE && !error; ++i) {
		if (pthread_mutex_init(&lock[i], NULL) != 0) {
			error = 1;
		}
	}
	for (i = 0; i < N_QUEUE && error; ++i) {
		pthread_mutex_destroy(&lock[i]);
	}
	if (error) {
		return_msg(EAR_ERROR, Generr.lock);
	}
	locked = 1;
	return EAR_SUCCESS;
}

state_t monitor_init()
{
	int m_errno;
	state_t s;

	while (pthread_mutex_trylock(&lock_gen));
	if (enabled != 0) {
		pthread_mutex_unlock(&lock_gen);
		return EAR_SUCCESS;
	}
	if (!locked) {
		if (xtate_fail(s, monitor_lock_alloc())) {
			pthread_mutex_unlock(&lock_gen);
			return s;
		}
	}
	enabled = 1;
	m_errno = pthread_create(&thread, NULL, ear_internal_monitor, NULL);
	if (m_errno != 0){
		return_msg(EAR_ERROR, strerror(m_errno));
	}
	enabled = (m_errno == 0);
	if (!enabled) {
		pthread_mutex_unlock(&lock_gen);
		return_msg(EAR_ERROR, strerror(m_errno));
	}
	pthread_mutex_unlock(&lock_gen);

	return EAR_SUCCESS;
}

state_t monitor_dispose()
{
	if (!enabled) {
		return_msg(EAR_BAD_ARGUMENT, "not initialized");
	}

	while (pthread_mutex_trylock(&lock_gen));
	enabled = 0;
	pthread_join(thread, NULL);
	pthread_mutex_unlock(&lock_gen);

	return EAR_SUCCESS;
}

static state_t monitor_register_void(void *p)
{
	suscription_t *s = (suscription_t *) p;
	return monitor_register(s);
}

state_t monitor_register(suscription_t *s)
{
	if (s             == NULL) return_msg(EAR_BAD_ARGUMENT, "the suscription can't be NULL");
	if (s->time_relax  < 100 ) return_msg(EAR_BAD_ARGUMENT, "time can't be zero");
	if (s->call_main  == NULL) return_msg(EAR_BAD_ARGUMENT, "reading call is NULL");
	if (s->id          < 0   ) return_msg(EAR_BAD_ARGUMENT, "incorrect suscription index");

	while (pthread_mutex_trylock(&lock[s->id]));

	queue[s->id].suscribed      = 1;
	queue[s->id].aligned		= 0;
	queue[s->id].bursted        = 0;
	queue[s->id].wait_saves.relax = (s->time_relax / 100);
	queue[s->id].wait_saves.burst = (s->time_burst / 100);

	debug("registered suscription %d", s->id);
  debug("aligned %d",  queue[s->id].aligned);
	debug("times relax/burst: %d/%d", queue[s->id].wait_saves.relax, queue[s->id].wait_saves.burst);
	debug("calls init/main: %p/%p (%d)", s->call_init, s->call_main, queue[s->id].ok_init);

	pthread_mutex_unlock(&lock[s->id]);

	return EAR_SUCCESS;
}

state_t monitor_unregister(suscription_t *s)
{
	if (s     == NULL      ) return_msg(EAR_BAD_ARGUMENT, "the suscription can't be NULL");
	if (s->id  > queue_last) return_msg(EAR_BAD_ARGUMENT, "incorrect suscription index");
	if (s->id  < 0         ) return_msg(EAR_BAD_ARGUMENT, "incorrect suscription index");

	while (pthread_mutex_trylock(&lock[s->id]));
	memset(&queue[s->id], 0, sizeof(register_t));
	pthread_mutex_unlock(&lock[s->id]);

	return EAR_SUCCESS;
}

int monitor_is_bursting(suscription_t *s)
{
	return queue[s->id].bursted;
}

state_t monitor_burst(suscription_t *s)
{
	if (s == NULL) {
		return_msg(EAR_BAD_ARGUMENT, "the suscription can't be NULL");
	}
	if (s->time_burst == 0) {
		return_msg(EAR_BAD_ARGUMENT, "time cant be zero");
	}
	if (s->time_burst  < 0) {
		return_msg(EAR_BAD_ARGUMENT, "burst time cant be less than relax time");
	}
	queue[s->id].bursted = 1;
	return EAR_SUCCESS;
}

state_t monitor_relax(suscription_t *s)
{
	if (s == NULL) {
		return_msg(EAR_BAD_ARGUMENT, "the suscription can't be NULL");
	}
	queue[s->id].bursted = 0;
	return EAR_SUCCESS;
}

suscription_t *suscription()
{
	int i = 0;

	while (pthread_mutex_trylock(&lock_gen));

	for (; i < N_QUEUE; ++i)
	{
		if (queue[i].delivered == 0) {
			break;
		}
	}
	if (queue_last == N_QUEUE) {
		pthread_mutex_unlock(&lock_gen);
		return NULL;
	}

	queue[i].suscription.id			= i;
	queue[i].delivered				= 1;
	queue[i].suscription.suscribe	= monitor_register_void;
	queue[i].bursted				= 0;

	if (queue[i].suscription.id >= queue_last) {
		queue_last = queue[i].suscription.id + 1;
	}

	pthread_mutex_unlock(&lock_gen);

	return &queue[i].suscription;
}
