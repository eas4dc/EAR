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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
//#define SHOW_DEBUGS 0
#include <common/output/verbose.h>
#include <metrics/energy/energy_gpu.h>
#include <metrics/energy/gpu/nvsmi.h>

#define SZ_BUFF_BIG     4096
#define GPU_METRICS     8

static char buffer[SZ_BUFF_BIG];
static pthread_mutex_t samp_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t       samp_t_accumu;
static FILE           *samp_t_stream;
static uint            samp_num_gpus;
static uint            samp_enabled;
static gpu_energy_t    *samp_data=NULL;
static uint            samp_ms;

typedef struct nvsmi_context_s {
	uint correct;
} nvsmi_context_t;

state_t nvsmi_gpu_status()
{
	if (system("which nvidia-smi &> /dev/null")) {
		return EAR_ERROR;
	}
	return EAR_SUCCESS;
}

static void nvsmi_gpu_sample_add(uint i, gpu_energy_t *data_aux)
{
    timestamp_getprecise(&samp_data[i].time);
	samp_data[i].freq_gpu_mhz += data_aux->freq_gpu_mhz;
	samp_data[i].freq_mem_mhz += data_aux->freq_mem_mhz;
	samp_data[i].util_gpu     += data_aux->util_gpu;
	samp_data[i].util_mem     += data_aux->util_mem;
	samp_data[i].temp_gpu     += data_aux->temp_gpu;
	samp_data[i].temp_mem     += data_aux->temp_mem;
	samp_data[i].power_w      += data_aux->power_w;
	samp_data[i].energy_j     += data_aux->power_w / (((float) samp_ms) / 1000.0);
	samp_data[i].samples      += 1;
}

static void *nvsmi_gpu_sample_main(void *p)
{
	static gpu_energy_t data_aux;
	static char bs[256];
	int i;
	int s;
	
	do {
		memset(bs, 0, 256);

		s = fscanf(samp_t_stream, "%s%s%s%s%s%s%s%s",
			&bs[  0], &bs[ 32], &bs[ 64], &bs[ 96],
			&bs[128], &bs[160], &bs[192], &bs[224]);
		debug("gpu data read <- %s %s %s %s %s %s %s %s",
			&bs[  0], &bs[ 32], &bs[ 64], &bs[ 96],
			&bs[128], &bs[160], &bs[192], &bs[224]);

		i                     =          atoi(&bs[  0]);
		data_aux.power_w      = (double) atof(&bs[ 32]);
		data_aux.freq_gpu_mhz = (ulong)  atol(&bs[ 64]);
		data_aux.freq_mem_mhz = (ulong)  atol(&bs[ 96]);
		data_aux.util_gpu     = (ulong)  atol(&bs[128]);
		data_aux.util_mem     = (ulong)  atol(&bs[160]);
		data_aux.temp_gpu     = (ulong)  atol(&bs[192]);
		data_aux.temp_mem     = (ulong)  atol(&bs[224]);

		debug("gpu data read -> %u %0.2lf %lu %lu %lu %lu %lu %lu",
			i                    , data_aux.power_w,
			data_aux.freq_gpu_mhz, data_aux.freq_mem_mhz,
			data_aux.util_gpu    , data_aux.util_mem,
			data_aux.temp_gpu    , data_aux.temp_mem);

		if (s == GPU_METRICS)
		{
			pthread_mutex_lock(&samp_lock);
			nvsmi_gpu_sample_add(i, &data_aux);
			pthread_mutex_unlock(&samp_lock);
		} 
	}
	while (s == GPU_METRICS && samp_enabled);
	
	return EAR_SUCCESS;
}

static state_t nvsmi_gpu_sample_create(pcontext_t *c, uint loop_ms)
{
	static const char *command = "nvidia-smi"                                   \
					" --query-gpu='index,power.draw,clocks.current.sm,"         \
					"clocks.current.memory,utilization.gpu,utilization.memory," \
					"temperature.gpu,temperature.memory'"                       \
 					" --format='csv,noheader,nounits'";
	int i;

	if (loop_ms == 0) {
		samp_ms = 1000;
	} else {
		samp_ms = loop_ms;
	}

	if (samp_enabled) {
		return EAR_ERROR;
	}
	
	nvsmi_gpu_data_null(c, samp_data);

	// Enabling sampling
	samp_enabled = 1;
	
	sprintf(buffer, "%s --loop-ms='%u'", command, samp_ms);
	debug("thread nvidia monitor using command '%s'", buffer);

	if ((samp_t_stream = popen(buffer, "r")) == NULL) {
		debug("thread nvidia monitor (popen) failed");
		return EAR_ERROR;
	}
	samp_enabled = 2;

	if (pthread_create(&samp_t_accumu, NULL, nvsmi_gpu_sample_main, NULL)) {
		debug("thread accumulator (pthread) failed");
		return EAR_ERROR;
	}
	samp_enabled = 3;
	debug("threads nvidia monitor and accumulator ok");
	return EAR_SUCCESS;
}

state_t nvsmi_gpu_init(pcontext_t *c, uint loop_ms)
{
	// Getting the total GPUs (it works also as init flag)
	if (samp_num_gpus == 0) {
		if (state_fail(nvsmi_gpu_count(c, &samp_num_gpus))) {
			state_return_msg(EAR_ERROR, 0, "impossible to connect with NVIDIA-SMI");
		}
		if (samp_num_gpus == 0) {
			state_return_msg(EAR_ERROR, 0, "no GPUs detected");
		}
	}
	debug("%d GPUS detected",samp_num_gpus);
	// Allocating internal accumulators
	if (samp_data == NULL){
		samp_data = calloc(samp_num_gpus, sizeof(gpu_energy_t));
		if (samp_data == NULL) {
			debug("Allocating memory for GPU");
			return EAR_ERROR;
		}
	}
	//
	if (samp_enabled == 0) {
		if (state_fail(nvsmi_gpu_sample_create(c, 1000))) {
			nvsmi_gpu_dispose(c);
			state_return_msg(EAR_ERROR, 0, "impossible to open monitor threads");
		}
	}
	return EAR_SUCCESS;
}

static state_t nvsmi_gpu_sample_cancel()
{
	uint stream_enabled = samp_enabled >= 2;
	uint accumu_enabled = samp_enabled == 3;
	samp_enabled = 0;

	if (stream_enabled) {
		pclose(samp_t_stream);
	}
	if (accumu_enabled) {
		pthread_join(samp_t_accumu, NULL);
	}
	samp_t_stream = NULL;
	return EAR_SUCCESS;
}

state_t nvsmi_gpu_dispose(pcontext_t *c)
{
	if (samp_enabled == 0) {
		return EAR_ERROR;
	}
    return nvsmi_gpu_sample_cancel();
}

state_t nvsmi_gpu_count(pcontext_t *c, uint *count)
{
	static const char *command = "nvidia-smi -L";
	FILE* file;

	if (samp_num_gpus > 0) {
		*count = samp_num_gpus;
		return EAR_SUCCESS;
	}

	//
	file = popen(command, "r");
	*count = 0;
	
	if (file == NULL) {
		return EAR_ERROR;
	}
	while (fgets(buffer, SZ_BUFF_BIG, file)) {
		*count += 1;
	}
	pclose(file);
	return EAR_SUCCESS;
}

state_t nvsmi_gpu_read(pcontext_t *c, gpu_energy_t *data_read)
{
	memset(data_read, 0, sizeof(gpu_energy_t) * samp_num_gpus);
	
	if (samp_enabled == 0) {
		return EAR_ERROR;
	}

	pthread_mutex_lock(&samp_lock);
	memcpy(data_read, samp_data, samp_num_gpus * sizeof(gpu_energy_t));
	pthread_mutex_unlock(&samp_lock);

	return EAR_SUCCESS;
}

state_t nvsmi_gpu_data_alloc(pcontext_t *c, gpu_energy_t **data_read)
{
	if (data_read != NULL) {
		*data_read = calloc(samp_num_gpus, sizeof(gpu_energy_t));
		if (*data_read == NULL) {
			return EAR_ERROR;
		}
	}
	return EAR_SUCCESS;
}

state_t nvsmi_gpu_data_free(pcontext_t *c, gpu_energy_t **data_read)
{
	if (data_read != NULL) {
		free(*data_read);
	}
	return EAR_SUCCESS;
}

state_t nvsmi_gpu_data_null(pcontext_t *c, gpu_energy_t *data_read)
{
	if (data_read != NULL) {
		memset(data_read, 0, samp_num_gpus * sizeof(gpu_energy_t));
	}
	return EAR_SUCCESS;
}

static void nvsmi_gpu_read_diff(gpu_energy_t *data_read1, gpu_energy_t *data_read2, gpu_energy_t *data_avrg, int i)
{
	ulong  usamps = data_read2[i].samples - data_read1[i].samples; 
	double fsamps = (double) usamps; 

	if (data_read2[i].samples <= data_read1[i].samples) {
		memset(&data_avrg[i], 0, sizeof(gpu_energy_t));
		return;		
	}

	// Averages
	data_avrg[i].freq_gpu_mhz = (data_read2[i].freq_gpu_mhz - data_read1[i].freq_gpu_mhz) / usamps;
	data_avrg[i].freq_mem_mhz = (data_read2[i].freq_mem_mhz - data_read1[i].freq_mem_mhz) / usamps;
	data_avrg[i].util_gpu     = (data_read2[i].util_gpu     - data_read1[i].util_gpu)     / usamps;
	data_avrg[i].util_mem     = (data_read2[i].util_mem     - data_read1[i].util_mem)     / usamps;
	data_avrg[i].temp_gpu     = (data_read2[i].temp_gpu     - data_read1[i].temp_gpu)     / usamps;
	data_avrg[i].temp_mem     = (data_read2[i].temp_mem     - data_read1[i].temp_mem)     / usamps;
	data_avrg[i].power_w      = (data_read2[i].power_w      - data_read1[i].power_w)      / fsamps;
	data_avrg[i].energy_j     = (data_read2[i].energy_j     - data_read1[i].energy_j);
}


// TODO: Overflow control.
state_t nvsmi_gpu_data_diff(pcontext_t *c, gpu_energy_t *data_read1, gpu_energy_t *data_read2, gpu_energy_t *data_avrg)
{
	int i;

	if (data_read1 == NULL || data_read2 == NULL || data_avrg == NULL) {
		return EAR_ERROR;
	}
	for (i = 0; i < samp_num_gpus; i++) {
		nvsmi_gpu_read_diff(data_read1, data_read2, data_avrg, i);
	}

	return EAR_SUCCESS;
}
