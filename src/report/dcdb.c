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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <common/states.h>
#include <common/types/types.h>
#include <common/output/verbose.h>
#include <common/system/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <report/report.h>
/*
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
*/
#include <errno.h>

#define VDCDB 2

#define MAX_ENTRIES 2
typedef struct shmem_data {
	int	index;
	struct measurement {
		uint64_t	timestamp;
		loop_t		loop;
	} measurements[MAX_ENTRIES];
} shmem_data_t;
shmem_data_t* shmem = NULL;
int fd = -1;

state_t report_init(report_id_t *id,cluster_conf_t *cconf)
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

state_t report_dispose(report_id_t *id)
{
	munmap(shmem, sizeof(shmem_data_t));
	close(fd);
	shm_unlink("/ear_dcdb.dat");

	return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id, loop_t *loops, uint count)
{
	verbose(VDCDB, "dcdb report_loops %d", count );

	int index = (shmem->index + 1) % 2;
	verbose(VDCDB, "index: %d", shmem->index);

	timestamp ts;
	timestamp_getreal(&ts);
	shmem->measurements[index].timestamp = timestamp_convert(&ts, TIME_NSECS);
	memcpy(&shmem->measurements[index].loop, &loops[0], sizeof(loop_t));
	shmem->index = index;

	return EAR_SUCCESS;
}
