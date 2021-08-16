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

//#define SHOW_DEBUGS 0

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <metrics/common/hwmon.h>

static state_t static_read(int fd, char *buffer, size_t size)
{
	int i, r, p;
	size -= 1;
	for (i = 0, r = 1, p = size; i < size && r > 0;) {
		r = pread(fd, (void *) &buffer[i], p, i);
		i =    i + r;
		p = size - i;
	}
	buffer[i] = '\0';
	if (r == -1) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t hwmon_find_drivers(const char *name, uint **ids, uint *n)
{
	char data[SZ_PATH];
	int i = 0; // index
	int c = 0; // count
	int m = 0; // mode
	int fd;

	while(1)
	{
		do {
	        sprintf(data, "/sys/class/hwmon/hwmon%d/name", i);
			debug("opening file '%s'", data);
			//
			if ((fd = open(data, O_RDONLY)) < 0) {
				break;
			}
	
	        if (state_ok(static_read(fd, data, SZ_PATH))) {
	            int len = strlen(data);
	            if (data[len-1] == '\n') {
	                data[len-1] =  '\0';
	            }
	            #if SHOW_DEBUGS
	            debug("comparing the driver name '%s' with '%s'", name, data);
            	#endif
				if (strstr(name, data) != NULL) {
					if (m == 1) {
						(*ids)[c] = i;
					}
					c += 1;
				}
			}
			close(fd);
        	//
			++i;	
    	} while(1);

		if (c == 0) {
			return_msg(EAR_ERROR, "no drivers found");
		}
		if (ids == NULL) {
			return EAR_SUCCESS;
		}
		if (m == 1) {
			return EAR_SUCCESS;
		}
		
		*ids = (uint *) calloc(c, sizeof(uint));
		*n   = c;
		// Resetting for next iteration
		i = 0;
		c = 0;
		m = 1;
	}

	return EAR_SUCCESS;
}

state_t hwmon_open_files(uint id, hwmon_t files, int **fds, uint *n)
{
	char data[SZ_PATH];
	int i = 1; // index
	int c = 0; // count
	int m = 0; // mode
	int fd;

	while(1)
	{
		do {
	        sprintf(data, (char *) files, id, i);
			debug("opening file '%s'", data);
			//
			if ((fd = open(data, O_RDONLY)) < 0) {
				break;
			}
			if (m == 0) {
				close(fd);
			} else {
				debug("copying fd %d", fd);
				(*fds)[c] = fd;
			}
			i += 1;
			c += 1;	
    	} while(1);

		if (c == 0) {
			return_msg(EAR_ERROR, "no files found");
		}
		if (m == 1) {
			return EAR_SUCCESS;
		}
		//
		if ((*fds = calloc(c, sizeof(int))) == NULL) {
			return_msg(EAR_ERROR, strerror(errno));
		}
		*fds = (int *) memset(*fds, -1, c*sizeof(int));
		*n   = c;
		// 
		i = 1;
		m = 1;
		c = 0;
	}

	return EAR_SUCCESS;
}

state_t hwmon_close_files(int *fds, uint n)
{
	int i;
	if (fds == NULL || n == 0) {
		return EAR_SUCCESS;
	}
	for (i = 0; i < n; ++i) {
		if (fds[i] != -1) {
		debug("closing fd %d", fds[i]);
			close(fds[i]);
		}
	}
	free(fds);
	return EAR_SUCCESS;
}

state_t hwmon_read(int fd, char *buffer, size_t size)
{
	return static_read(fd, buffer, size);
}
