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
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <common/config.h>
#include <metrics/flops/flops.h>
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

    dprintf(fd, "%lu;%lu;%lu;", sig->avg_f, sig->avg_imc_f,sig->def_f);
    dprintf(fd, "%lf;%lf;%lf;%lf;%lf;%lf;", sig->time, sig->CPI, sig->TPI, sig->GBS,sig->IO_MBS,sig->perc_MPI);
    dprintf(fd, "%lf;%lf;%lf;", sig->DC_power, sig->DRAM_power, sig->PCK_power);
    dprintf(fd, "%llu;%llu;%lf", sig->cycles, sig->instructions, sig->Gflops);

    if (is_extended)
    {
        dprintf(fd, ";%llu;%llu;%llu", sig->L1_misses, sig->L2_misses, sig->L3_misses);

        for (i = 0; i < FLOPS_EVENTS; ++i) {
            dprintf(fd, ";%llu", sig->FLOPS[i]);
        }
    }

#if USE_GPUS
    int num_gpu = sig->gpu_sig.num_gpus;
    for (int j = 0; j < num_gpu; ++j) {
        dprintf(fd, ";%lf;%lu;%lu;%lu;%lu", sig->gpu_sig.gpu_data[j].GPU_power, sig->gpu_sig.gpu_data[j].GPU_freq, \
                sig->gpu_sig.gpu_data[j].GPU_mem_freq, sig->gpu_sig.gpu_data[j].GPU_util, sig->gpu_sig.gpu_data[j].GPU_mem_util);
    }
#endif
}

void signature_print_simple_fd(int fd, signature_t *sig)
{
    dprintf(fd, "[AVGF=%.2f/%.2f DEFF=%.2f TIME=%.3lf CPI=%.3lf GBS=%.2lf POWER=%.2lf]",(float)sig->avg_f/1000000.0,
            (float)sig->avg_imc_f/1000000.0, (float)sig->def_f/1000000.0,sig->time, sig->CPI,sig->GBS,sig->DC_power);
}

void signature_to_str(signature_t *sig,char *msg,size_t limit)
{
    snprintf(msg,limit, "[AVGF=%.2f/%.2f DEFF=%.2f TIME=%.3lf CPI=%.3lf GBS=%.2lf POWER=%.2lf]",(float)sig->avg_f/1000000.0,
            (float)sig->avg_imc_f/1000000.0, (float)sig->def_f/1000000.0,sig->time, sig->CPI,sig->GBS,sig->DC_power);
}

void compute_ssig_vpi(double *vpi,ssig_t *sig)
{
    ull vins;
    vins=0;
    if (sig->FLOPS[3]>0){
        vins=sig->FLOPS[3]/WEIGHT_512F;
    }

    if (sig->FLOPS[7]>0){
        vins=vins+sig->FLOPS[7]/WEIGHT_512D;
    }
    if ((vins>0) && (sig->instructions>0)) *vpi=(double)vins/(double)sig->instructions;
    else *vpi=0;
}
void compute_ssig_vpi2(double *vpi,ssig_t *sig)
{
    ull vins;
    vins=0;
    if (sig->FLOPS[2]>0){
        vins=sig->FLOPS[2]/WEIGHT_256F;
    }

    if (sig->FLOPS[6]>0){
        vins=vins+sig->FLOPS[6]/WEIGHT_256D;
    }
    if ((vins>0) && (sig->instructions>0)) *vpi=(double)vins/(double)sig->instructions;
    else *vpi=0;
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
void acum_sig_metrics(signature_t *dst,signature_t *src)
{
    int i;
    dst->DC_power += src->DC_power;
    dst->DRAM_power += src->DRAM_power;
    dst->PCK_power += src->PCK_power;
    dst->EDP += src->EDP;
    dst->GBS += src->GBS;
    dst->IO_MBS += src->IO_MBS;
    dst->TPI += src->TPI;
    dst->CPI += src->CPI;
    dst->Gflops += src->Gflops;
    dst->time += src->time;
    for (i=0;i<FLOPS_EVENTS;i++) dst->FLOPS[i] += src->FLOPS[i];
    dst->L1_misses += src->L1_misses;
    dst->L2_misses += src->L2_misses;
    dst->L3_misses += src->L3_misses;
    dst->instructions += src->instructions;
    dst->cycles += src->cycles;
    dst->avg_f += src->avg_f;
    dst->avg_imc_f += src->avg_imc_f;
    dst->def_f += src->def_f;
#if USE_GPUS
    dst->gpu_sig.num_gpus = src->gpu_sig.num_gpus;
    for (i=0;i<MAX_GPUS_SUPPORTED;i++){
        dst->gpu_sig.gpu_data[i].GPU_power += src->gpu_sig.gpu_data[i].GPU_power;
        dst->gpu_sig.gpu_data[i].GPU_freq += src->gpu_sig.gpu_data[i].GPU_freq;
        dst->gpu_sig.gpu_data[i].GPU_mem_freq += src->gpu_sig.gpu_data[i].GPU_mem_freq;
        dst->gpu_sig.gpu_data[i].GPU_util += src->gpu_sig.gpu_data[i].GPU_util;
        dst->gpu_sig.gpu_data[i].GPU_mem_util += src->gpu_sig.gpu_data[i].GPU_mem_util;
    }
#endif

}
/** Checks all the values are valid to be reported to DB and fixes potential problems*/
void clean_db_signature(signature_t *sig, double limit)
{
    if (!isnormal(sig->time))				sig->time = 0;
    if (!isnormal(sig->EDP))				sig->EDP = 0;
    if (!isnormal(sig->DC_power)) 	sig->DC_power = 0;
    if (!isnormal(sig->DRAM_power)) sig->DRAM_power = 0;
    if (!isnormal(sig->PCK_power)) 	sig->PCK_power = 0;
    if (!isnormal(sig->CPI))				sig->CPI = 0;
    if (!isnormal(sig->TPI))				sig->TPI = 0;
    if (!isnormal(sig->GBS))				sig->GBS = 0;
    if (!isnormal(sig->IO_MBS))			sig->IO_MBS = 0;
    if (!isnormal(sig->Gflops))			sig->Gflops = 0;
    if (sig->DC_power > limit)		  sig->DC_power = limit;
    if (sig->DRAM_power > limit)		sig->DRAM_power = 0;
    if (sig->PCK_power > limit)			sig->PCK_power = 0;
		if (!isnormal(sig->perc_MPI))		sig->perc_MPI = 0;

#if USE_GPUS
    if (sig->gpu_sig.num_gpus){
        int g;
        for (g=0;g<sig->gpu_sig.num_gpus;g++){
            if (!isnormal(sig->gpu_sig.gpu_data[g].GPU_power)) sig->gpu_sig.gpu_data[g].GPU_power = 0;
        }
    }
#endif
}


void acum_ssig_metrics(ssig_t *avg_sig,ssig_t *s)
{
    int i;
    avg_sig->time				+= s->time;
    avg_sig->CPI				+= s->CPI;
    avg_sig->avg_f			+= s->avg_f;
    avg_sig->def_f			+= s->def_f;
    avg_sig->Gflops			+= s->Gflops;
    for (i=0;i<FLOPS_EVENTS;i++) avg_sig->FLOPS[i] += s->FLOPS[i];
#if USE_GPUS
    avg_sig->gpu_sig.num_gpus = s->gpu_sig.num_gpus;
    for (i=0;i<MAX_GPUS_SUPPORTED;i++){
        avg_sig->gpu_sig.gpu_data[i].GPU_power += s->gpu_sig.gpu_data[i].GPU_power;
        avg_sig->gpu_sig.gpu_data[i].GPU_freq += s->gpu_sig.gpu_data[i].GPU_freq;
        avg_sig->gpu_sig.gpu_data[i].GPU_mem_freq += s->gpu_sig.gpu_data[i].GPU_mem_freq;
        avg_sig->gpu_sig.gpu_data[i].GPU_util += s->gpu_sig.gpu_data[i].GPU_util;
        avg_sig->gpu_sig.gpu_data[i].GPU_mem_util += s->gpu_sig.gpu_data[i].GPU_mem_util;
    }
#endif
}
void set_global_metrics(ssig_t *avg_sig,ssig_t *s)
{
    avg_sig->GBS=s->GBS;
    avg_sig->TPI=s->TPI;
    avg_sig->DC_power=s->DC_power;
    avg_sig->time=0;
    avg_sig->CPI=0;
    avg_sig->avg_f=0;
    avg_sig->def_f=0;
    avg_sig->Gflops=0;
    memset(avg_sig->FLOPS,0,sizeof(ull)*FLOPS_EVENTS);
}

void compute_avg_node_sig(ssig_t *avg_sig,int n)
{ 
    double t,cpi;
    unsigned long avg_f,def_f;
    int i;
    ull my_flops[FLOPS_EVENTS];
    t					=avg_sig->time/n;
    cpi				=avg_sig->CPI/n;
    avg_f			=avg_sig->avg_f/n;
    def_f			=avg_sig->def_f/n;
    avg_sig->time = t;
    avg_sig->CPI	= cpi;
    avg_sig->avg_f 			= avg_f;
    avg_sig->def_f			= def_f;
    for (i=0;i<FLOPS_EVENTS;i++){   
        my_flops[i] = avg_sig->FLOPS[i]/n;
        avg_sig->FLOPS[i] = my_flops[i];
    }
}



void compute_avg_sig(signature_t *dst,signature_t *src,int nums)
{
    int i;
    dst->DC_power = src->DC_power/nums;
    dst->DRAM_power = src->DRAM_power/nums;
    dst->PCK_power = src->PCK_power/nums;
    dst->EDP = src->EDP/nums;
    dst->GBS = src->GBS/nums;
    dst->TPI = src->TPI/nums;
    dst->CPI = src->CPI/nums;
    dst->Gflops = src->Gflops/nums;
    dst->time = src->time/nums;
    for (i=0;i<FLOPS_EVENTS;i++) dst->FLOPS[i] = src->FLOPS[i]/nums;
    dst->L1_misses = src->L1_misses/nums;
    dst->L2_misses = src->L2_misses/nums;
    dst->L3_misses = src->L3_misses/nums;
    dst->instructions = src->instructions/nums;
    dst->cycles = src->cycles/nums;
    dst->avg_f = src->avg_f/nums;
    dst->avg_imc_f = src->avg_imc_f/nums;
    dst->def_f = src->def_f/nums;
#if USE_GPUS
    dst->gpu_sig.num_gpus = src->gpu_sig.num_gpus;
    for (i=0;i<MAX_GPUS_SUPPORTED;i++){
        dst->gpu_sig.gpu_data[i].GPU_power = src->gpu_sig.gpu_data[i].GPU_power/nums;
        dst->gpu_sig.gpu_data[i].GPU_freq = src->gpu_sig.gpu_data[i].GPU_freq/nums;
        dst->gpu_sig.gpu_data[i].GPU_mem_freq = src->gpu_sig.gpu_data[i].GPU_mem_freq/nums;
        dst->gpu_sig.gpu_data[i].GPU_util = src->gpu_sig.gpu_data[i].GPU_util/nums;
        dst->gpu_sig.gpu_data[i].GPU_mem_util = src->gpu_sig.gpu_data[i].GPU_mem_util/nums;
    }
#endif
}

void from_minis_to_sig(signature_t *s,ssig_t *minis)
{
    //signature_init(s);
    s->DC_power=(double)minis->DC_power;
    s->GBS=(double)minis->GBS;
    s->TPI=(double)minis->TPI;
    s->CPI=(double)minis->CPI;
    s->Gflops=(double)minis->Gflops;
    s->time=(double)minis->time;
    memcpy(s->FLOPS,minis->FLOPS,sizeof(ull)*FLOPS_EVENTS);
    s->instructions = minis->instructions;
    s->cycles = minis->cycles;
    s->avg_f=minis->avg_f;
    s->def_f=minis->def_f;
#if USE_GPUS
    memcpy(&s->gpu_sig,&minis->gpu_sig,sizeof(gpu_signature_t));
#endif

}


void from_sig_to_mini(ssig_t *minis,signature_t *s)
{
    minis->DC_power=(float)s->DC_power;
    minis->GBS=(float)s->GBS;
    minis->TPI=(float)s->TPI;
    minis->CPI=(float)s->CPI;
    minis->Gflops=(float)s->Gflops;
    minis->time=(float)s->time;
    memcpy(minis->FLOPS,s->FLOPS,sizeof(ull)*FLOPS_EVENTS);
    minis->instructions = s->instructions;
    minis->cycles = s->cycles;
    minis->avg_f=s->avg_f;
    minis->def_f=s->def_f;
#if USE_GPUS
    memcpy(&minis->gpu_sig,&s->gpu_sig,sizeof(gpu_signature_t));
#endif
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

/* New functions for GPU support */

double sig_total_gpu_power(signature_t *s)
{
    double gpup=0;
#if USE_GPUS
    int i;
    for (i=0;i<s->gpu_sig.num_gpus;i++){
        gpup+=s->gpu_sig.gpu_data[i].GPU_power;
    }
#endif
    return gpup;
}

double sig_node_power(signature_t *s)
{
#if USE_GPUS
    return (s->DC_power - sig_total_gpu_power(s));
#else
    return s->DC_power;
#endif

}

int sig_gpus_used(signature_t *s)
{
    uint util=0;
#if USE_GPUS
    uint i;
    for (i=0;i<s->gpu_sig.num_gpus;i++){
        if (s->gpu_sig.gpu_data[i].GPU_util){
            util++;
        }
    }
#endif
    return util;
}


