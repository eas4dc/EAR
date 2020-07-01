/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING
*/

#ifndef _APP_API_H
#define _APP_API_H
#define EAR_SUCCESS 0
#define EAR_ERROR -1
#define EAR_TIMEOUT -21
/** returns the accumulated energy i(mili joules) and time (miliseconds) */
int ear_energy(unsigned long *energy_mj,unsigned long *time_ms);
/** Given two measures of energy and time computes the difference */
void ear_energy_diff(unsigned long ebegin,unsigned long eend, unsigned long *ediff, unsigned long tbegin, unsigned long tend, unsigned long *tdiff);

int ear_debug_energy(ulong *energy_j,ulong *energy_mj,ulong *time_sec,ulong *time_ms,ulong *os_time_sec,ulong *os_time_ms);
int ear_connect();
void ear_disconnect();

#else
#endif
