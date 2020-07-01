/**************************************************************
 * Energy Aware Runtime (EAR)
*This program is part of the Energy Aware Runtime (EAR).
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
*modify it under the terms of the GNU Lesser General Public
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

#ifndef EAR_FREQUENCY_UNCORE_SKL_H
#define EAR_FREQUENCY_UNCORE_SKL_H

#include <common/states.h>
#include <common/types/generic.h>

/* Vol. 3B
 *  *  Words
 *   *    - MSR_U_PMON_UCLK_FIXED_CTR */
#define U_MSR_PMON_FIXED_CTR_OFF0x000704
#define U_MSR_PMON_FIXED_CTL_OFF0x000703
#define U_MSR_PMON_FIXED_CTL_STA0x400000
#define U_MSR_PMON_FIXED_CTL_STO0x000000
#define U_MSR_UNCORE_RATIO_LIMIT0x000620

typedef struct snap_uncore
{
uint64_t freq;
time_t time;
} snap_uncore_t;

/* */
state_t frequency_uncore_init(uint sockets_num, uint core_num, uint64_t cores_nom_freq);

/* */
state_t frequency_uncore_dispose();

/* snaps must be a vector with sockets_num entries */
state_t frequency_uncore_snap(snap_uncore_t *snaps);

/* snaps and freqs must be a vector with sockets_num entries */
state_t frequency_uncore_read(snap_uncore_t *snaps, uint64_t *freqs);

state_t frequency_uncore_get_limits(uint32_t *buffer);
state_t frequency_uncore_set_limits(uint32_t *buffer);


#endif //EAR_FREQUENCY_UNCORE_SKL_H

