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
