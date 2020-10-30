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
#include <unistd.h>
#include <common/types/signature.h>
#include <common/math_operations.h>

void signature_copy(signature_t *destiny, signature_t *source)
{
    memcpy(destiny, source, sizeof(signature_t));
}

void signature_init(signature_t *sig)
{
    memset(sig, 0, sizeof(signature_t));
}

void signature_print_fd(int fd, signature_t *sig, char is_extended)
{
    int i;
    
	dprintf(fd, "%lu;%lu;", sig->avg_f, sig->def_f);
	dprintf(fd, "%lf;%lf;%lf;%lf;", sig->time, sig->CPI, sig->TPI, sig->GBS);
	dprintf(fd, "%lf;%lf;%lf;", sig->DC_power, sig->DRAM_power, sig->PCK_power);
	dprintf(fd, "%llu;%llu;%lf", sig->cycles, sig->instructions, sig->Gflops);

    if (is_extended)
    {
	    dprintf(fd, ";%llu;%llu;%llu", sig->L1_misses, sig->L2_misses, sig->L3_misses);

        for (i = 0; i < FLOPS_EVENTS; ++i) {
	    	dprintf(fd, ";%llu", sig->FLOPS[i]);
    	}
    }
}

void signature_print_simple_fd(int fd, signature_t *sig)
{
	dprintf(fd, "[AVGF=%lu DEFF=%lu TIME=%.3lf CPI=%.3lf GBS=%.2lf POWER=%.2lf]",sig->avg_f, sig->def_f,sig->time, sig->CPI,sig->GBS,sig->DC_power);
}


void compute_vpi(double *vpi,signature_t *sig)
{
    ull vins;
    vins=0;
    if (sig->FLOPS[3]>0){
        vins=sig->FLOPS[3]/16;
    }   

    if (sig->FLOPS[7]>0){
        vins=vins+sig->FLOPS[7]/8;
    }   
    if ((vins>0) && (sig->instructions>0)) *vpi=(double)vins/(double)sig->instructions;
    else *vpi=0;
}

void print_signature_fd_binary(int fd, signature_t *sig)
{
    write(fd,sig,sizeof(signature_t));
}
void read_signature_fd_binary(int fd, signature_t *sig)
{
    read(fd,sig,sizeof(signature_t));
}

void adapt_signature_to_node(signature_t *dest,signature_t *src,float ratio_PPN)
{
	double new_TPI,new_DC_power;
	signature_copy(dest,src);
	new_TPI=src->TPI/(double)ratio_PPN;
	new_DC_power=src->DC_power/(double)ratio_PPN;
	dest->TPI=new_TPI;
	dest->DC_power=new_DC_power;
}


void from_sig_to_mini(ssig_t *minis,signature_t *s)
{
	minis->DC_power=(float)s->DC_power;
	minis->GBS=(float)s->GBS;
	minis->CPI=(float)s->CPI;
	minis->Gflops=(float)s->Gflops;
	minis->time=(float)s->time;
	minis->avg_f=s->avg_f;
	minis->def_f=s->def_f;
}
void copy_mini_sig(ssig_t *dst,ssig_t *src)
{
	memcpy(dst,src,sizeof(ssig_t));
}

void minis_to_str(ssig_t *s,char *b)
{
	sprintf(b,"[power %.2f GBs %.2f CPI %.3f GFlops %.2f time %.3f avgf %lu deff %lu]",s->DC_power,s->GBS,s->CPI,s->Gflops,s->time,
	s->avg_f,s->def_f);
}

