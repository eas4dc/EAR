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

// #define SHOW_DEBUGS 1
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

#include <common/config.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <metrics/common/hwmon.h>
#include <metrics/common/apis.h>
#include <metrics/gpu/archs/pvc_power_hwmon.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static state_t pvc_hwmon_static_init(ctx_t *c);

static uint    pvc_hwmon_meter_initialized = 0;
static uint pvc_gpus_num_devices = 0;
static timestamp_t energy_time;
static uint total_samples = 1;

#define DUMMY_MEM_FREQ 500000
#define DUMMY_FREQ 1600000

typedef struct hwfds_s {
	uint  id_count;
	uint *fd_count;
	int **fds;
} hwfds_t;
static hwfds_t local_context;
static bool pvc_gpus_found = false;


state_t gpu_pvc_hwmon_load(gpu_ops_t *ops, int force_api)
{
	state_t s;
	debug("asking for status");

  debug("Received API %d", force_api);

	while (pthread_mutex_trylock(&lock));
	if (pvc_gpus_num_devices == 0) {
		if (state_ok(s = hwmon_find_drivers("i915", NULL, NULL))) {
			pvc_gpus_found = true;
		}
	}
	pthread_mutex_unlock(&lock);
	debug("PVC GPUS found");
	if (pvc_gpus_found){
    apis_set(ops->get_info,      pvc_hwmon_get_info);
    apis_set(ops->init,          pvc_hwmon_init);
    apis_set(ops->dispose,       pvc_hwmon_dispose);
    apis_set(ops->get_devices,   pvc_hwmon_get_devices);
    apis_set(ops->read,          pvc_hwmon_read);
    apis_set(ops->read_raw,      pvc_hwmon_read_raw);
    apis_set(ops->set_monitoring_mode,      pvc_hwmon_set_monitoring_mode);
	}

	return (pvc_gpus_found ? pvc_hwmon_static_init(no_ctx): EAR_ERROR);
}

void pvc_hwmon_get_info(apinfo_t *info)
{
	debug("pvc_hwmon_get_info");
	info->api         = API_PVC_HWMON;
	info->devs_count  = pvc_gpus_num_devices;
}


state_t pvc_hwmon_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

void pvc_hwmon_set_monitoring_mode(int mode_in)
{
}

static state_t pvc_hwmon_static_init(ctx_t *c)
{
	uint id_count;
	uint *ids;
	state_t s;
	int i;

	/* MUTEX pending */

	debug("asking for init");
	// Looking for ids
	debug("pvc_hwmon_init looking for devices");
	if (xtate_fail(s, hwmon_find_drivers("i915", &ids, &id_count))) {
		warning("hwmon_find_drivers: %s", state_msg);
		return s;
	}
	if (id_count == 0){
		warning("id_count = 0");
		return EAR_ERROR;
	}
	debug("PVC %d devices found", id_count);
	pvc_gpus_num_devices = id_count;
	// Allocating file descriptors)
	hwfds_t *h = &local_context;
	/* fd is a per device a per num gpu vector */
	if ((h->fds = calloc(id_count, sizeof(int *))) == NULL) {
		return_msg(s, strerror(errno));
	}
	if ((h->fd_count = calloc(id_count, sizeof(uint))) == NULL) {
		return_msg(s, strerror(errno));
	}
	h->id_count = id_count;
	//
	for (i = 0; i < h->id_count; ++i) {
		if (xtate_fail(s, hwmon_open_files(ids[i], Hwmon_pvc_energy.energy_j, &h->fds[i], &h->fd_count[i], 1))) {
			warning("Error opening hwmon files: %s", state_msg);
			return s;
		}
	}
	// Freeing ids space
	free(ids);
	debug("init succeded");

	pvc_hwmon_meter_initialized = 1;
	return EAR_SUCCESS;
}

void pvc_hwmon_get_devices(gpu_devs_t **devs_in, uint *devs_count_in)
{
    int dv;

		debug("pvc_hwmon_get_devices initialized %s", (pvc_hwmon_meter_initialized ? "yes": "no"));
		if (!pvc_hwmon_meter_initialized) return ;

    if (devs_in != NULL) {
				debug("Allocating data for %u GPU dev", pvc_gpus_num_devices);
        *devs_in = calloc(pvc_gpus_num_devices, sizeof(gpu_devs_t));
        //
        for (dv = 0; dv < pvc_gpus_num_devices; ++dv) {
            (*devs_in)[dv].serial = 0;
            (*devs_in)[dv].index  = dv;
						debug("DEV[%d] serial %d index %d", dv, (*devs_in)[dv].serial, (*devs_in)[dv].index );
        }
    }
    if (devs_count_in != NULL) {
        *devs_count_in = pvc_gpus_num_devices;
    }

    return ;
}


static state_t get_context(ctx_t *c, hwfds_t **h)
{
	*h = &local_context;
	return EAR_SUCCESS;
}

state_t pvc_hwmon_dispose(ctx_t *c)
{
	int i;
	hwfds_t *h;
	state_t s;

	if (!pvc_hwmon_meter_initialized) return EAR_ERROR;
	if (xtate_fail(s, get_context(c, &h))) {
		return s;
	}
	for (i = 0; i < h->id_count; ++i) {
		hwmon_close_files(h->fds[i], h->fd_count[i]);
	}
	free(h->fd_count);
	free(h->fds);

	return EAR_SUCCESS;
}

// Data
state_t pvc_hwmon_count_devices(ctx_t *c, uint *count)
{
	hwfds_t *h;
	state_t s;

	if (!pvc_hwmon_meter_initialized) return EAR_ERROR;
	if (xtate_fail(s, get_context(c, &h))) {
		return s;
	}
	if ((count != NULL)  && (h != NULL)){
		//*count = h->id_count;
		// To be compatible with other APIs we will use
		// gpu granularity for now
		*count = pvc_gpus_num_devices;
	}

	return EAR_SUCCESS;
}


state_t pvc_hwmon_read(ctx_t *c, gpu_t *data)
{
	char sdata[SZ_PATH];
	llong aux1, aux2 = 0;
	int i,  k;
	hwfds_t *h;
	state_t s;

	debug("pvc_hwmon_read");
	if (!pvc_hwmon_meter_initialized) return EAR_ERROR;
	if (data == NULL) return EAR_ERROR;
	memset(data, 0, sizeof(gpu_t) * pvc_gpus_num_devices);

	if (xtate_fail(s, get_context(c, &h))) {
		return s;
	}
  timestamp_getfast(&energy_time);
	for (i = 0; i < h->id_count ; ++i) {
		debug("I %d has %d Fds", i, h->fd_count[i]);
		for (k = 0; k < h->fd_count[i]; k++){
			debug("Reading ID %d,%d  fd=%d", i, k, h->fds[i][k]);

			if (xtate_fail(s, hwmon_read(h->fds[i][k], sdata, SZ_PATH))) {
				//return s;
				debug("read failed with data %s", sdata);
			}
			debug("read %s", sdata);
		  data[i].energy_j  += atoll(sdata) /1000000;
			data[i].working   = 1;
			data[i].correct   = 1;
			data[i].util_gpu  = 100 * total_samples;
			data[i].util_mem  = 100 * total_samples;
			data[i].freq_gpu  = DUMMY_FREQ * total_samples;
			data[i].freq_mem  = DUMMY_MEM_FREQ * total_samples;
			data[i].time      = energy_time;
			data[i].samples   = total_samples;
		}

    debug("dev | metric   | value    ");
    debug("--- | -------- | -----    ");
    debug("%d   | samps    | %lu     ", i, data[i].samples );
    debug("%d   | freq_gpu | %lu MHz ", i, data[i].freq_gpu);
    debug("%d   | freq_mem | %lu MHz ", i, data[i].freq_mem);
    debug("%d   | util_gpu | %lu %%  ", i, data[i].util_gpu);
    debug("%d   | util_mem | %lu %%  ", i, data[i].util_mem);
    debug("%d   | temp_gpu | %lu cels", i, data[i].temp_gpu);
    debug("%d   | temp_mem | %lu cels", i, data[i].temp_mem);
    debug("%d   | energy   | %0.2lf J", i, data[i].energy_j);
    debug("%d   | working  | %d      ", i, data[i].working );

	}
	total_samples++;

	return EAR_SUCCESS;
}

state_t pvc_hwmon_read_raw(ctx_t *c, gpu_t *data)
{
	debug("pvc_hwmon_read_raw");
	return pvc_hwmon_read(c, data);
}

state_t pvc_hwmon_set_mode(ctx_t *c, uint32_t mode)
{
	debug("pvc_hwmon_set_mode");
	return EAR_SUCCESS;
}


#if TEST

/// WARNING: It was based on energy_cpu API it has to be adapted to gpu API
// gcc -o test -DTEST -I$HOME/lrz-sng2/ear_private/src pvc_power_hwmon.c ../../../common/libcommon.a ../../libmetrics.a
int main(int argc, char *argv[]){
	if (state_fail(pvc_hwmon_meter_load())){
		printf("Error in load\n");
		exit(1);
	}
	printf("PVC hwmon meter loaded\n");
	ctx_t c;
	if (state_fail(pvc_hwmon_meter_init(&c))){
		printf("Error in init\n");
		exit(1);
	}
	printf("PVC hwmon meter initialized\n");
	uint devices;
	if (state_fail(pvc_hwmon_count_devices(&c, &devices))){
		printf("Error in count devices\n");
		exit(1);
	}
	printf("PVC hwmon meter %u devices detected\n", devices);
	ullong *energy;
	ullong *last_energy;
	ullong *diff;
	ullong acum;
	energy       = calloc(devices, sizeof(ullong));
	last_energy = calloc(devices, sizeof(ullong));
	diff        = calloc(devices, sizeof(ullong));

	// First reading
	pvc_hwmon_read(&c,last_energy, &acum);
	sleep(10);
	do
	{
	pvc_hwmon_read(&c,energy, &acum);
	for (uint i = 0; i < devices ; i++){
		diff[i] = energy[i] - last_energy[i];
		printf("PVC power [%u] = %.2lfW\n", i, (double)diff[i]/(double)(10.0*1000));
		last_energy[i] = energy[i];
	}
	sleep(10);
	}while(1);
	pvc_hwmon_meter_dispose(&c);
	return 0;


}
#endif
