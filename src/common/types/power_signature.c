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
#include <string.h>
#include <common/types/power_signature.h>
#include <common/math_operations.h>
#include <common/config.h>

void copy_power_signature(power_signature_t *destiny, power_signature_t *source)
{
    memcpy(destiny, source, sizeof(power_signature_t));
}

void init_power_signature(power_signature_t *sig)
{
    memset(sig, 0, sizeof(power_signature_t));
}

uint are_equal_power_sig(power_signature_t *sig1, power_signature_t *sig2, double th)
{
    if (!equal_with_th(sig1->DC_power, sig2->DC_power, th)) return 0;
    if (!equal_with_th(sig1->DRAM_power, sig2->DRAM_power, th)) return 0;
    if (!equal_with_th(sig1->PCK_power, sig2->PCK_power, th)) return 0;
    if (!equal_with_th(sig1->EDP, sig2->EDP, th)) return 0;    
    return 1;
}

void print_power_signature_fd(int fd, power_signature_t *sig)
{
	/* print order: AVG.FREQ;DEF.FREQ;TIME;DC-NODE-POWER;DRAM-POWER;*/
	//int i;
    
	dprintf(fd, "%lu;%lu;", sig->avg_f, sig->def_f);
	dprintf(fd, "%lf;", sig->time);
	dprintf(fd, "%lf;%lf;%lf;", sig->DC_power, sig->DRAM_power, sig->PCK_power);
}
