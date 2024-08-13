/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>
#include <common/system/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <report/report.h>
#include <errno.h>

#define VDCDB 1
#define PM_MAX_ENTRIES 5
#define LOOP_MAX_ENTRIES 10
#define EVENT_MAX_ENTRIES 5

typedef struct shmem_data {
	int	pm_index;
	struct pm_measurement {
		uint64_t	timestamp;
		periodic_metric_t pm;
	} pm_measurement[PM_MAX_ENTRIES];
	int	loop_index;
	struct loop_measurement {
		uint64_t	timestamp;
		loop_t		loop;
	} loop_measurement[LOOP_MAX_ENTRIES];
	int	event_index;
	struct event_measurement {
		uint64_t	timestamp;
		ear_event_t		event;
	} event_measurement[EVENT_MAX_ENTRIES];
} shmem_data_t;
shmem_data_t* shmem = NULL;
int fd = -1;

state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{
	verbose(VDCDB, "dcdb report_init");

	fd = shm_open("/ear_dcdb.dat", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fd == -1) {
		verbose(VDCDB, "Error opening shmem object: %s", strerror(errno));
		return EAR_ERROR;
	}
	if (ftruncate(fd, sizeof(shmem_data_t)) == -1) {
		verbose(VDCDB, "Error setting size of shmem object: %s", strerror(errno));
		return EAR_ERROR;
	}
	shmem = (shmem_data_t*) mmap(0, sizeof(shmem_data_t), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (shmem == (shmem_data_t*) -1) {
		verbose(VDCDB, "Error mapping shared memory: %s", strerror(errno));
		return EAR_ERROR;
	}

	return EAR_SUCCESS;
}

state_t report_periodic_metrics(report_id_t *id, periodic_metric_t *metric_list, uint count)
{
	periodic_metric_t *metric;

	for (int i=0;i<count;i++){

		metric = &metric_list[i];

		int index = (shmem->pm_index + 1) % 5;

		timestamp ts;
		timestamp_getreal(&ts);

		shmem->pm_measurement[index].timestamp = timestamp_convert(&ts, TIME_NSECS);
		memcpy(&shmem->pm_measurement[index].pm, &metric[0], sizeof(periodic_metric_t));
		shmem->pm_index = index;
	}

	return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id, loop_t *loops_list, uint count)
{
	loop_t *loops;

	for (int i=0;i<count;i++){

		loops = &loops_list[i];

		int index = (shmem->loop_index + 1) % 10;

		timestamp ts;
		timestamp_getreal(&ts);

		shmem->loop_measurement[index].timestamp = timestamp_convert(&ts, TIME_NSECS);
		memcpy(&shmem->loop_measurement[index].loop, &loops[0], sizeof(loop_t));
		shmem->loop_index = index;
	}

	return EAR_SUCCESS;
}

state_t report_events(report_id_t *id, ear_event_t *eves_list, uint count)
{
	ear_event_t *eves;

	for (int i=0;i<count;i++){

		eves = &eves_list[i];

		int index = (shmem->event_index + 1) % 5;

		timestamp ts;
		timestamp_getreal(&ts);

		shmem->event_measurement[index].timestamp = timestamp_convert(&ts, TIME_NSECS);
		memcpy(&shmem->event_measurement[index].event, &eves[0], sizeof(ear_event_t));
		shmem->event_index = index;
	}

	return EAR_SUCCESS;
}

state_t report_dispose(report_id_t *id)
{
	munmap(shmem, sizeof(shmem_data_t));
	close(fd);
	shm_unlink("/ear_dcdb.dat");

	return EAR_SUCCESS;
}

