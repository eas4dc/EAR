/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <metrics/common/msr.h>
#include <metrics/frequency/imc/intel_haswell.h>
#include <common/hardware/hardware_info.h>

static uint64_t *_clocks_start = NULL;
static uint64_t *_clocks_stop = NULL;
static int *_fds = NULL;

static uint64_t _cmdsta;
static uint64_t _cmdsto;
static off_t _offctl;
static off_t _offctr;
static off_t _offurl;

static uint _cpus_num = 0;
static uint _start = 0;
static uint _init = 0;

static uint _supported_architecture(uint cpus_model)
{
	switch (cpus_model)
	{
		case CPU_HASWELL_X:
		case CPU_BROADWELL_X:
		case CPU_BROADWELL_XEON_D:
		case CPU_SKYLAKE_X:
			return 1;
		default:
			return 0;
	}
}

static uint _fill_architecture_bits(uint cpus_model)
{
	_cmdsto = U_MSR_PMON_FIXED_CTL_STO;
	_cmdsta = U_MSR_PMON_FIXED_CTL_STA;
	_offctl = U_MSR_PMON_FIXED_CTL_OFF;
	_offctr = U_MSR_PMON_FIXED_CTR_OFF;
	_offurl = U_MSR_UNCORE_RATIO_LIMIT;
	return EAR_SUCCESS;
}

state_t frequency_uncore_init(uint sockets_num, uint cores_num, uint cores_model)
{
	state_t r;
	uint offset_cpu;
	uint i_socket;
	uint i_cpu;

	if (_init) {
		return EAR_BUSY;
	}

	if (!_supported_architecture(cores_model)) {
		return EAR_ARCH_NOT_SUPPORTED;
	}

	if (cores_num == 0 || sockets_num > 2) {
		return EAR_BAD_ARGUMENT;
	}

	_fds  = malloc(sizeof(int) * sockets_num);
	_clocks_start = malloc(sizeof(uint64_t) * sockets_num);
	_clocks_stop  = malloc(sizeof(uint64_t) * sockets_num);

	offset_cpu = cores_num / sockets_num;

	for (i_socket = 0, i_cpu = 0; i_socket < sockets_num; ++i_socket, i_cpu += offset_cpu)
	{
		_fds[i_socket] = -1;

		if ((r = msr_open(i_cpu, &_fds[i_socket])) != EAR_SUCCESS)
		{
			frequency_uncore_dispose();
			return r;
		}
	}

	_init = 1;
	_start = 0;
	_cpus_num = sockets_num;
	_fill_architecture_bits(cores_model);

	return EAR_SUCCESS;
}

state_t frequency_uncore_dispose()
{
	int i;

	if (!_init) {
		return EAR_NOT_INITIALIZED;
	}

	for (i = 0; i < _cpus_num; ++i) {
		msr_close(&_fds[i]);
	}

	free(_clocks_start);
	free(_clocks_stop);

	_init = 0;
	_cpus_num = 0;

	return EAR_SUCCESS;
}

state_t frequency_uncore_counters_start()
{
	state_t r;
	int i;

	if (!_init) {
		return EAR_NOT_INITIALIZED;
	}

	if (_start) {
		return EAR_BUSY;
	}

	for (i = 0; i < _cpus_num; ++i)
	{
		// Read
		if ((r = msr_read(&_fds[i], &_clocks_start[i], sizeof(uint64_t), _offctr)) != EAR_SUCCESS) {
			return r;
		}
		// Start
		if ((r = msr_write(&_fds[i], &_cmdsta, sizeof(uint64_t), _offctl)) != EAR_SUCCESS) {
			return r;
		}
	}

	_start = 1;
	return EAR_SUCCESS;
}

state_t frequency_uncore_counters_stop(uint64_t *buffer)
{
	state_t r;
	int i;

	if (!_init) {
		return EAR_NOT_INITIALIZED;
	}

	if (!_start) {
		return EAR_NOT_READY;
	}

	for (i = 0; i < _cpus_num; ++i)
	{
		// Stop
		if ((r = msr_write(&_fds[i], &_cmdsto, sizeof(uint64_t), _offctl)) != EAR_SUCCESS) {
			return r;
		}
		// Read
		if ((r = msr_read(&_fds[i], &_clocks_stop[i], sizeof(uint64_t), _offctr)) != EAR_SUCCESS) {
			return r;
		}

		buffer[i] = _clocks_stop[i] - _clocks_start[i];
	}

	_start = 0;
	return EAR_SUCCESS;
}

state_t frequency_uncore_set_limits(uint32_t *buffer)
{
    uint64_t set0 = 0;
    uint64_t set1 = 0;
    state_t r;
    int i, j;

    if (!_init) {
        return EAR_NOT_INITIALIZED;
    }   

    for (i = 0, j = 0; i < _cpus_num; ++i, j += 2)
    {   
        set0 = (buffer[j+0] << 8) & U_MSR_UNCORE_RL_MASK_MIN;
        set1 = (buffer[j+1] << 0) & U_MSR_UNCORE_RL_MASK_MAX;
		set0 = set0 | set1;

		if ((r = msr_write(&_fds[i], &set0, sizeof(uint64_t), _offurl)) != EAR_SUCCESS) {
			return r;
		}
    }   

    return EAR_SUCCESS;
}

state_t frequency_uncore_get_limits(uint32_t *buffer)
{
	uint64_t result0 = 0;
	uint64_t result1 = 0;
	state_t r;
	int i, j;

	if (!_init) {
		return EAR_NOT_INITIALIZED;
	}

	for (i = 0, j = 0; i < _cpus_num; ++i, j += 2)
	{
		buffer[j+0] = 0;
		buffer[j+1] = 0;

		// Read
		if ((r = msr_read(&_fds[i], &result0, sizeof(uint64_t), _offurl)) != EAR_SUCCESS) {
			return r;
		}

		result1 = (result0 & U_MSR_UNCORE_RL_MASK_MAX) >> 0;
		result0 = (result0 & U_MSR_UNCORE_RL_MASK_MIN) >> 8; 

		buffer[j+0] = (uint32_t) result0;
		buffer[j+1] = (uint32_t) result1;
		result0 = 0;
		result1 = 0;
	}

	return EAR_SUCCESS;
}

