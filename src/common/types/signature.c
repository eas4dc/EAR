/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <common/config.h>
#include <common/output/debug.h>
#include <common/types/signature.h>
#include <common/math_operations.h>
#include <metrics/flops/flops.h>

static double compute_vpi(const ullong *FLOPS, ullong instructions);

void signature_copy(signature_t *destiny, const signature_t *source)
{
    memcpy(destiny, source, sizeof(signature_t));
}

void signature_init(signature_t *sig)
{
    memset(sig, 0, sizeof(signature_t));
}

void signature_print_fd(int fd, signature_t *sig, char is_extended, int single_column, char sep)
{
    int i;

    dprintf(fd, "%lu;%lu;%lu;",
            sig->avg_f,
            sig->avg_imc_f,
            sig->def_f);

    dprintf(fd, "%lf;%lf;%lf;%lf;%lf;%lf;",
            sig->time,
            sig->CPI,
            sig->TPI,
            sig->GBS,
            sig->IO_MBS,
            sig->perc_MPI);

    dprintf(fd, "%lf;%lf;%lf;",
            sig->DC_power,
            sig->DRAM_power,
            sig->PCK_power);

    dprintf(fd, "%llu;%llu;%lf",
            sig->cycles,
            sig->instructions,
            sig->Gflops);

    if (is_extended)
    {
        dprintf(fd, ";%llu;%llu;%llu",
                sig->L1_misses,
                sig->L2_misses,
                sig->L3_misses);

        for (i = 0; i < FLOPS_EVENTS; ++i) {
            dprintf(fd, ";%llu", sig->FLOPS[i]);
        }
    }

	debug("Signature with %d GPUS", sig->gpu_sig.num_gpus);

#if USE_GPUS
    int num_gpu = sig->gpu_sig.num_gpus;
    num_gpu = MAX_GPUS_SUPPORTED;
		if (single_column && num_gpu) {
			dprintf(fd, ";");
    	for (int j = 0; j < num_gpu-1; ++j) dprintf(fd, "%lf%c", sig->gpu_sig.gpu_data[j].GPU_power, sep);
			dprintf(fd, "%lf", sig->gpu_sig.gpu_data[num_gpu-1].GPU_power);
			dprintf(fd, ";");
    	for (int j = 0; j < num_gpu-1; ++j) dprintf(fd, "%lu%c", sig->gpu_sig.gpu_data[j].GPU_freq, sep);
			dprintf(fd, "%lu", sig->gpu_sig.gpu_data[num_gpu-1].GPU_freq);
			dprintf(fd, ";");
    	for (int j = 0; j < num_gpu-1; ++j) dprintf(fd, "%lu%c", sig->gpu_sig.gpu_data[j].GPU_mem_freq, sep);
			dprintf(fd, "%lu", sig->gpu_sig.gpu_data[num_gpu-1].GPU_mem_freq);
			dprintf(fd, ";");
    	for (int j = 0; j < num_gpu-1; ++j) dprintf(fd, "%lu%c", sig->gpu_sig.gpu_data[j].GPU_util, sep);
			dprintf(fd, "%lu", sig->gpu_sig.gpu_data[num_gpu-1].GPU_util);
			dprintf(fd, ";");
    	for (int j = 0; j < num_gpu-1; ++j) dprintf(fd, "%lu%c", sig->gpu_sig.gpu_data[j].GPU_mem_util, sep);
			dprintf(fd, "%lu", sig->gpu_sig.gpu_data[num_gpu-1].GPU_mem_util);
#if WF_SUPPORT
			dprintf(fd, ";");
    	for (int j = 0; j < num_gpu-1; ++j) dprintf(fd, "%f%c", sig->gpu_sig.gpu_data[j].GPU_GFlops, sep);
			dprintf(fd, "%f", sig->gpu_sig.gpu_data[num_gpu-1].GPU_GFlops);
#endif
        } else {

            for (int j = 0; j < num_gpu; ++j) {
#if WF_SUPPORT
                dprintf(fd, ";%lf;%lu;%lu;%lu;%lu;%f",
                        sig->gpu_sig.gpu_data[j].GPU_power,
                        sig->gpu_sig.gpu_data[j].GPU_freq,
                        sig->gpu_sig.gpu_data[j].GPU_mem_freq,
                        sig->gpu_sig.gpu_data[j].GPU_util,
                        sig->gpu_sig.gpu_data[j].GPU_mem_util,
                        sig->gpu_sig.gpu_data[j].GPU_GFlops);
#else
                dprintf(fd, ";%lf;%lu;%lu;%lu;%lu",
                        sig->gpu_sig.gpu_data[j].GPU_power,
                        sig->gpu_sig.gpu_data[j].GPU_freq,
                        sig->gpu_sig.gpu_data[j].GPU_mem_freq,
                        sig->gpu_sig.gpu_data[j].GPU_util,
                        sig->gpu_sig.gpu_data[j].GPU_mem_util);
#endif
            }
        }
#endif
}

void signature_print_simple_fd(int fd, signature_t *sig)
{
    dprintf(fd, "[AVGF=%.2f/%.2f DEFF=%.2f TIME=%.3lf CPI=%.3lf GBS=%.2lf TPI=%.2lf POWER=%.2lf(%.2lf/%.2lf)]\n",
            (float) sig->avg_f / 1000000.0,
            (float) sig->avg_imc_f / 1000000.0,
            (float) sig->def_f / 1000000.0,
            sig->time,
            sig->CPI,
            sig->GBS,
            sig->TPI,
            sig->DC_power,
            sig->PCK_power,
            sig->DRAM_power);
}

state_t signature_to_str(signature_t *sig, char *msg, size_t limit)
{
    int ret = snprintf(msg, limit,
                       "[AVGF=%.2f/%.2f DEFF=%.2f TIME=%.3lf CPI=%.3lf GBS=%.2lf TPI=%.2lf GFLOPS %.2lf POWER=%.2lf((%.2lf/%.2lf)]",
                       (float) sig->avg_f / 1000000.0,
                       (float) sig->avg_imc_f / 1000000.0,
                       (float) sig->def_f / 1000000.0,
                       sig->time,
                       sig->CPI,
                       sig->GBS,
                       sig->TPI,
                       sig->Gflops,
                       sig->DC_power,
                       sig->PCK_power,
                       sig->DRAM_power);
    if (ret < limit && ret > 0)
    {
        return EAR_SUCCESS;
    } else
    {
        return EAR_ERROR;
    }
}

void compute_sig_vpi(double *vpi, const signature_t *sig)
{
    *vpi = 0;

    if (sig->instructions > 0) {
        *vpi = compute_vpi(sig->FLOPS, sig->instructions);
    }
}

void compute_ssig_vpi(double *vpi, const ssig_t *sig)
{
    *vpi = 0;

    if (sig->instructions > 0) {
        *vpi = compute_vpi(sig->FLOPS, sig->instructions);
    }
}

void compute_ssig_vpi2(double *vpi,ssig_t *sig)
{
    ull vins;
    vins=0;
    if (sig->FLOPS[2]>0) {
        vins=sig->FLOPS[2]/WEIGHT_256F;
    }

    if (sig->FLOPS[6]>0) {
        vins=vins+sig->FLOPS[6]/WEIGHT_256D;
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

void adapt_signature_to_node(signature_t *dest, signature_t *src, float ratio_PPN)
{
    #if 0
    double new_TPI,new_DC_power;
    #endif
    signature_copy(dest,src);
    #if 0
    verbose(3, " TPI %.3lf Power %.3lf ratio %.4f", src->TPI, src->DC_power, ratio_PPN);

    if ((ratio_PPN == 0) || (fpclassify(ratio_PPN) != FP_NORMAL)) return;
    if ((src->TPI == 0)  || (fpclassify(src->TPI)  != FP_NORMAL)) return;
    if ((src->DC_power == 0)  || (fpclassify(src->DC_power)  != FP_NORMAL)) return;

    new_TPI=src->TPI/(double)ratio_PPN;
    new_DC_power=src->DC_power/(double)ratio_PPN;

    dest->TPI=new_TPI;
    dest->DC_power=new_DC_power;
    #endif
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
        dst->gpu_sig.gpu_data[i].GPU_power 		+= src->gpu_sig.gpu_data[i].GPU_power;
        dst->gpu_sig.gpu_data[i].GPU_freq 		+= src->gpu_sig.gpu_data[i].GPU_freq;
        dst->gpu_sig.gpu_data[i].GPU_mem_freq += src->gpu_sig.gpu_data[i].GPU_mem_freq;
        dst->gpu_sig.gpu_data[i].GPU_util 		+= src->gpu_sig.gpu_data[i].GPU_util;
        dst->gpu_sig.gpu_data[i].GPU_mem_util += src->gpu_sig.gpu_data[i].GPU_mem_util;
#if WF_SUPPORT
        dst->gpu_sig.gpu_data[i].GPU_GFlops 	+= src->gpu_sig.gpu_data[i].GPU_GFlops;
#endif
    }
#endif

}

#define MAX_CPU_FREQ 10000000
#define MAX_IMC_FREQ 10000000

void signature_clean_before_db(signature_t *sig, double pwr_limit)
{
    if (!isnormal(sig->time))          sig->time       = 0;
    if (!isnormal(sig->EDP))           sig->EDP        = 0;
    if (!isnormal(sig->DC_power)) 	   sig->DC_power   = 0;
    if (!isnormal(sig->DRAM_power))    sig->DRAM_power = 0;
    if (!isnormal(sig->PCK_power))     sig->PCK_power  = 0;
    if (!isnormal(sig->CPI))           sig->CPI        = 0;
    if (!isnormal(sig->TPI))           sig->TPI        = 0;
    if (!isnormal(sig->GBS))           sig->GBS        = 0;
    if (!isnormal(sig->IO_MBS))        sig->IO_MBS     = 0;
    if (!isnormal(sig->Gflops))        sig->Gflops     = 0;
	if (!isnormal(sig->perc_MPI))      sig->perc_MPI   = 0;

    if (sig->DC_power   > pwr_limit)   sig->DC_power   = pwr_limit;
    if (sig->DRAM_power > pwr_limit)   sig->DRAM_power = 0;
    if (sig->PCK_power  > pwr_limit)   sig->PCK_power  = 0;

    if (sig->avg_f     > MAX_CPU_FREQ) sig->avg_f      = MAX_CPU_FREQ;
    if (sig->avg_imc_f > MAX_IMC_FREQ) sig->avg_imc_f  = MAX_IMC_FREQ;
    if (sig->def_f     > MAX_CPU_FREQ) sig->def_f      = MAX_CPU_FREQ;

#if USE_GPUS
    if (sig->gpu_sig.num_gpus) {
        int g;
        for (g = 0;g < sig->gpu_sig.num_gpus; g++) {
            if (!isnormal(sig->gpu_sig.gpu_data[g].GPU_power)) {
                sig->gpu_sig.gpu_data[g].GPU_power = 0;
            }
#if WF_SUPPORT
            if (!isnormal(sig->gpu_sig.gpu_data[g].GPU_GFlops)) {
                sig->gpu_sig.gpu_data[g].GPU_GFlops = 0;
            }
#endif
        }
    }
#endif
}

state_t ssig_accumulate(ssig_t *dst_ssig, ssig_t *src_ssig)
{
    if (dst_ssig == NULL || src_ssig == NULL) {
        return EAR_ERROR;
    }

    dst_ssig->time         += src_ssig->time;
    dst_ssig->CPI          += src_ssig->CPI;
    dst_ssig->avg_f        += src_ssig->avg_f;
    dst_ssig->def_f        += src_ssig->def_f;
    dst_ssig->instructions += src_ssig->instructions;
    dst_ssig->cycles       += src_ssig->cycles;
    dst_ssig->L3_misses    += src_ssig->L3_misses;
    dst_ssig->Gflops       += src_ssig->Gflops;
    dst_ssig->IO_MBS       += src_ssig->IO_MBS;

    for (int i = 0; i < FLOPS_EVENTS; i++) {
        dst_ssig->FLOPS[i] += src_ssig->FLOPS[i];
    }

#if USE_GPUS
    dst_ssig->gpu_sig.num_gpus = src_ssig->gpu_sig.num_gpus;

    for (int i = 0; i < MAX_GPUS_SUPPORTED; i++) {
        dst_ssig->gpu_sig.gpu_data[i].GPU_power    += src_ssig->gpu_sig.gpu_data[i].GPU_power;
        dst_ssig->gpu_sig.gpu_data[i].GPU_freq     += src_ssig->gpu_sig.gpu_data[i].GPU_freq;
        dst_ssig->gpu_sig.gpu_data[i].GPU_mem_freq += src_ssig->gpu_sig.gpu_data[i].GPU_mem_freq;
        dst_ssig->gpu_sig.gpu_data[i].GPU_util     += src_ssig->gpu_sig.gpu_data[i].GPU_util;
        dst_ssig->gpu_sig.gpu_data[i].GPU_mem_util += src_ssig->gpu_sig.gpu_data[i].GPU_mem_util;
#if WF_SUPPORT
        dst_ssig->gpu_sig.gpu_data[i].GPU_GFlops   += src_ssig->gpu_sig.gpu_data[i].GPU_GFlops;
#endif
    }
#endif

    return EAR_SUCCESS;
}

void ssig_set_node_metrics(ssig_t *avg_sig, ssig_t *s)
{
    avg_sig->GBS=s->GBS;
    avg_sig->TPI=s->TPI;
    avg_sig->DC_power=s->DC_power;
    avg_sig->time=0;
    avg_sig->CPI=0;
    avg_sig->avg_f=0;
    avg_sig->def_f=0;
    avg_sig->Gflops=0;
    memset(avg_sig->FLOPS, 0, sizeof(ull) * FLOPS_EVENTS);
}

state_t ssig_compute_avg_node(ssig_t *ssig, int n)
{
    if (ssig == NULL || n <= 0) {
        return EAR_ERROR;
    }

    ssig->time /= n;

    ssig->cycles /= n;
    ssig->instructions /= n;
    ssig->CPI /= n;

    ssig->avg_f /= n;
    ssig->def_f	/= n;

    ssig->L3_misses /= n;

    ssig->IO_MBS /= n;

    for (int i = 0; i < FLOPS_EVENTS; i++) {
        ssig->FLOPS[i] /= n;
    }

    return EAR_SUCCESS;
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
        dst->gpu_sig.gpu_data[i].GPU_power    = src->gpu_sig.gpu_data[i].GPU_power/nums;
        dst->gpu_sig.gpu_data[i].GPU_freq     = src->gpu_sig.gpu_data[i].GPU_freq/nums;
        dst->gpu_sig.gpu_data[i].GPU_mem_freq = src->gpu_sig.gpu_data[i].GPU_mem_freq/nums;
        dst->gpu_sig.gpu_data[i].GPU_util     = src->gpu_sig.gpu_data[i].GPU_util/nums;
        dst->gpu_sig.gpu_data[i].GPU_mem_util = src->gpu_sig.gpu_data[i].GPU_mem_util/nums;
#if WF_SUPPORT
        dst->gpu_sig.gpu_data[i].GPU_GFlops   = src->gpu_sig.gpu_data[i].GPU_GFlops/nums;
#endif
    }
#endif
}

state_t signature_from_ssig(signature_t *dst_sig, ssig_t *src_ssig)
{
    if (dst_sig == NULL || src_ssig == NULL) {
        return EAR_ERROR;
    }

    dst_sig->DC_power   = (double) src_ssig->DC_power;
    dst_sig->DRAM_power = (double) src_ssig->DRAM_power;
    dst_sig->PCK_power  = (double) src_ssig->PCK_power;

    dst_sig->GBS        = (double) src_ssig->GBS;
    dst_sig->TPI        = (double) src_ssig->TPI;

    dst_sig->IO_MBS     = (double) src_ssig->IO_MBS;

    dst_sig->L1_misses = src_ssig->L1_misses;
    dst_sig->L2_misses = src_ssig->L2_misses;
    dst_sig->L3_misses = src_ssig->L3_misses;

    dst_sig->instructions = src_ssig->instructions;
    dst_sig->cycles       = src_ssig->cycles;

    dst_sig->CPI        = (double) src_ssig->CPI;

    dst_sig->Gflops     = (double) src_ssig->Gflops;

    dst_sig->time       = (double) src_ssig->time;

    dst_sig->avg_f = src_ssig->avg_f;
    dst_sig->def_f = src_ssig->def_f;

    memcpy(dst_sig->FLOPS, src_ssig->FLOPS, sizeof(ull) * FLOPS_EVENTS);

#if USE_GPUS
    memcpy(&dst_sig->gpu_sig, &src_ssig->gpu_sig, sizeof(gpu_signature_t));
#endif

    return EAR_SUCCESS;
}


state_t ssig_from_signature(ssig_t *dst_ssig, signature_t *src_sig)
{
    if (dst_ssig == NULL || src_sig == NULL) {
        return EAR_ERROR;
    }

    dst_ssig->DC_power	  = (float) src_sig->DC_power;
    dst_ssig->DRAM_power = (float) src_sig->DRAM_power;
    dst_ssig->PCK_power  = (float) src_sig->PCK_power;
    dst_ssig->GBS        = (float) src_sig->GBS;
    dst_ssig->TPI        = (float) src_sig->TPI;
    dst_ssig->CPI        = (float) src_sig->CPI;
    dst_ssig->Gflops     = (float) src_sig->Gflops;
    dst_ssig->time	      = (float) src_sig->time;
    dst_ssig->IO_MBS  = (float) src_sig->IO_MBS;

    dst_ssig->instructions = src_sig->instructions;
    dst_ssig->cycles       = src_sig->cycles;
    dst_ssig->L1_misses    = src_sig->L1_misses;
    dst_ssig->L2_misses    = src_sig->L2_misses;
    dst_ssig->L3_misses    = src_sig->L3_misses;
    dst_ssig->avg_f		= src_sig->avg_f;
    dst_ssig->def_f		= src_sig->def_f;

    memcpy(dst_ssig->FLOPS, src_sig->FLOPS, sizeof(ull) * FLOPS_EVENTS);

#if USE_GPUS
    memcpy(&dst_ssig->gpu_sig,&src_sig->gpu_sig,sizeof(gpu_signature_t));
#endif

    return EAR_SUCCESS;

}

void copy_mini_sig(ssig_t *dst,ssig_t *src)
{
    memcpy(dst,src,sizeof(ssig_t));
}

void ssig_tostr(ssig_t *ssig, char *dst_str, size_t n)
{
    snprintf(dst_str, n, "[dc power %.2f GB/s %.2f CPI %.3f GFlop/s %.2f I/O %.2f time %.3f avgf %lu deff %lu]",
            ssig->DC_power, ssig->GBS, ssig->CPI, ssig->Gflops, ssig->IO_MBS, ssig->time, ssig->avg_f, ssig->def_f);
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

static double compute_vpi(const ullong *FLOPS, ullong instructions) {
    ull vins = 0;

    if (FLOPS[3] > 0) {
        vins = FLOPS[3] / 16;
    }

    if (FLOPS[7] > 0) {
        vins += (FLOPS[7] / 8);
    }

    return (double) vins / (double) instructions;
}

static void gpu_signature_serialize(serial_buffer_t *b, gpu_signature_t *gpu_sig)
{
    gpu_app_t *gpu_data;
    int gpu;

    serial_dictionary_push_auto(b, gpu_sig->num_gpus);
    for (gpu = 0; gpu < gpu_sig->num_gpus; ++gpu) {
        gpu_data = &gpu_sig->gpu_data[gpu];
        serial_dictionary_push_auto(b, gpu_data->GPU_power);
        serial_dictionary_push_auto(b, gpu_data->GPU_freq);
        serial_dictionary_push_auto(b, gpu_data->GPU_mem_freq);
        serial_dictionary_push_auto(b, gpu_data->GPU_util);
        serial_dictionary_push_auto(b, gpu_data->GPU_mem_util);
#if WF_SUPPORT
        serial_dictionary_push_auto(b, gpu_data->GPU_GFlops);
#endif
    }
}

void signature_serialize(serial_buffer_t *b, signature_t *sig)
{
    serial_dictionary_push_auto(b, sig->DC_power); //
    serial_dictionary_push_auto(b, sig->DRAM_power);
    serial_dictionary_push_auto(b, sig->PCK_power);
    serial_dictionary_push_auto(b, sig->EDP);
    serial_dictionary_push_auto(b, sig->GBS);
    serial_dictionary_push_auto(b, sig->IO_MBS);
    serial_dictionary_push_auto(b, sig->TPI);
    serial_dictionary_push_auto(b, sig->CPI);
    serial_dictionary_push_auto(b, sig->Gflops);
    serial_dictionary_push_auto(b, sig->time);
    serial_dictionary_push_auto(b, sig->FLOPS);
    serial_dictionary_push_auto(b, sig->L1_misses);
    serial_dictionary_push_auto(b, sig->L2_misses);
    serial_dictionary_push_auto(b, sig->L3_misses);
    serial_dictionary_push_auto(b, sig->instructions);
    serial_dictionary_push_auto(b, sig->cycles);
    serial_dictionary_push_auto(b, sig->avg_f);
    serial_dictionary_push_auto(b, sig->avg_imc_f);
    serial_dictionary_push_auto(b, sig->def_f);
    serial_dictionary_push_auto(b, sig->perc_MPI);
#if USE_GPUS
    gpu_signature_serialize(b, &sig->gpu_sig);
#endif
}

static void gpu_signature_deserialize(serial_buffer_t *b, gpu_signature_t *gpu_sig)
{
    gpu_app_t *gpu_data;
    int gpu;

    serial_dictionary_pop_auto(b, gpu_sig->num_gpus);
    for (gpu = 0; gpu < gpu_sig->num_gpus; ++gpu) {
        gpu_data = &gpu_sig->gpu_data[gpu];
        serial_dictionary_pop_auto(b, gpu_data->GPU_power);
        serial_dictionary_pop_auto(b, gpu_data->GPU_freq);
        serial_dictionary_pop_auto(b, gpu_data->GPU_mem_freq);
        serial_dictionary_pop_auto(b, gpu_data->GPU_util);
        serial_dictionary_pop_auto(b, gpu_data->GPU_mem_util);
#if WF_SUPPORT
        serial_dictionary_pop_auto(b, gpu_data->GPU_GFlops);
#endif
    }
}

void signature_deserialize(serial_buffer_t *b, signature_t *sig)
{
    serial_dictionary_pop_auto(b, sig->DC_power);
    serial_dictionary_pop_auto(b, sig->DRAM_power);
    serial_dictionary_pop_auto(b, sig->PCK_power);
    serial_dictionary_pop_auto(b, sig->EDP);
    serial_dictionary_pop_auto(b, sig->GBS);
    serial_dictionary_pop_auto(b, sig->IO_MBS);
    serial_dictionary_pop_auto(b, sig->TPI);
    serial_dictionary_pop_auto(b, sig->CPI);
    serial_dictionary_pop_auto(b, sig->Gflops);
    serial_dictionary_pop_auto(b, sig->time);
    serial_dictionary_pop_auto(b, sig->FLOPS);
    serial_dictionary_pop_auto(b, sig->L1_misses);
    serial_dictionary_pop_auto(b, sig->L2_misses);
    serial_dictionary_pop_auto(b, sig->L3_misses);
    serial_dictionary_pop_auto(b, sig->instructions);
    serial_dictionary_pop_auto(b, sig->cycles);
    serial_dictionary_pop_auto(b, sig->avg_f);
    serial_dictionary_pop_auto(b, sig->avg_imc_f);
    serial_dictionary_pop_auto(b, sig->def_f);
    serial_dictionary_pop_auto(b, sig->perc_MPI);
#if USE_GPUS
    gpu_signature_deserialize(b, &sig->gpu_sig);
#endif
}
double compute_dc_nogpu_power(signature_t *lsig)
{
	double CPU_power = lsig->DC_power;
	double GPU_power = 0;
	#if USE_GPUS
	for (uint gid = 0; gid < lsig->gpu_sig.num_gpus; gid++){
	GPU_power +=  lsig->gpu_sig.gpu_data[gid].GPU_power;
	}
	#endif
	CPU_power -= GPU_power;
	return CPU_power;
}

