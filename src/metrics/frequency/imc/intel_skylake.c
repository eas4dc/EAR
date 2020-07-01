/**************************************************************
* *Energy Aware Runtime (EAR)
* *This program is part of the Energy Aware Runtime (EAR).
* *
* *EAR provides a dynamic, transparent and ligth-weigth solution for
* *Energy management.
* *
* *    It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
* *
* *       Copyright (C) 2017
* *BSC Contact mailto:ear-support@bsc.es
* *Lenovo contact mailto:hpchelp@lenovo.com
* *
* *EAR is free software; you can redistribute it and/or
* *modify it under the terms of the GNU Lesser General Public
* *License as published by the Free Software Foundation; either
* *version 2.1 of the License, or (at your option) any later version.
* *
* *EAR is distributed in the hope that it will be useful,
* *but WITHOUT ANY WARRANTY; without even the implied warranty of
* *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* *Lesser General Public License for more details.
* *
* *You should have received a copy of the GNU Lesser General Public
* *License along with EAR; if not, write to the Free Software
* *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
* *The GNU LEsser General Public License is contained in the file COPYING
*/

#include <stdio.h>
#include <string.h>
#include <metrics/custom/hardware_info.h>
#include <metrics/custom/frequency_uncore.h>

static uint64_t freq_division;
static uint sockets_num;
static uint cores_num;
static uint bits_num;
static uint enabled;

static state_t counters_start(uint cpu)
{
uint64_t command;
state_t r;
int fd=-1;

/* Open*/
if ((r = msr_open(cpu, &fd)) != EAR_SUCCESS) {
return r;
}

/* Start*/
command = U_MSR_PMON_FIXED_CTL_STA;

if ((r = msr_write(&fd, &command, sizeof(uint64_t), U_MSR_PMON_FIXED_CTL_OFF)) != EAR_SUCCESS) {
return r;
}


close(fd);

return EAR_SUCCESS;
}

static state_t counters_read(uint cpu, snap_uncore_t *snap)
{
state_t r;
int fd = -1;

/* Open */
if ((r = msr_open(cpu, &fd)) != EAR_SUCCESS) {
return r;
}

if ((r = msr_read(&fd, &snap->freq, sizeof(uint64_t), U_MSR_PMON_FIXED_CTR_OFF)) != EAR_SUCCESS) {
return r;
}


close(fd);

return EAR_SUCCESS;
}

state_t frequency_uncore_init(uint _sockets_num, uint _cores_num, uint64_t _cores_nom_freq)
{
int i, c;

if (!hwinfo_is_compatible_uncore()) {
state_return_msg(EAR_ARCH_NOT_SUPPORTED, 0, "CPU model not supported");
}


sockets_num = _sockets_num;
cores_num = _cores_num;

/* Depending on the nominal frequency received, the
 *  *  returned frequency will be in Hz, KHz or MHz.*/
freq_division = (_cores_nom_freq < 100000000) ? 1000 : 1;
freq_division = (_cores_nom_freq < 100000) ? 1000000 : freq_division;


for (i = 0, c = 0; i < sockets_num; ++i, c += cores_num)
{
counters_start(c);
}

enabled = 1;

return EAR_SUCCESS;
}

/* */
state_t frequency_uncore_dispose()
{
enabled = 0;

return EAR_SUCCESS;
}

/* */
state_t frequency_uncore_snap(snap_uncore_t *snaps)
{
state_t s;
int i, c;

if (!enabled) {
return EAR_NOT_INITIALIZED;
}


for (i = 0, c = 0; i < sockets_num; ++i, c += cores_num)
{
s = counters_read(c, &snaps[i]);

if (state_fail(s)) {
return s;
}
}

return EAR_SUCCESS;
}

/* */
state_t frequency_uncore_read(snap_uncore_t *snaps, uint64_t *freqs)
{
snap_uncore_t snap_new;
state_t s;
int i, c;

if (!enabled) {
return EAR_NOT_INITIALIZED;
}


for (i = 0, c = 0; i < sockets_num; ++i, c += cores_num)
{
s = counters_read(c, &snap_new);

if (state_fail(s)) {
return s;
}

freqs[i] = snap_new.freq - snaps[i].freq;
freqs[i] = freqs[i] / freq_division;
snaps[i].freq = snap_new.freq;
}

return EAR_SUCCESS;
}
