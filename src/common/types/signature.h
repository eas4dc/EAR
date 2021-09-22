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

#ifndef _EAR_TYPES_SIGNATURE
#define _EAR_TYPES_SIGNATURE

#include <common/config.h>
#include <common/types/generic.h>
#if USE_GPUS
#include <metrics/gpu/gpu.h>
#endif
#include <metrics/io/io.h>

// 0: float
// 1: 128 float
// 2: 256 float
// 3: 512 float
// 4: double
// 5: 128 double
// 6: 256 double
// 7: 512 double
#define FLOPS_EVENTS 8

typedef struct gpu_app{
    double GPU_power;
    ulong  GPU_freq;
    ulong  GPU_mem_freq;
    ulong  GPU_util;
    ulong  GPU_mem_util;
}gpu_app_t;

typedef struct gpu_signature{
    int num_gpus;
    gpu_app_t gpu_data[MAX_GPUS_SUPPORTED];
}gpu_signature_t;

typedef struct mini_sig
{
    float DC_power;
    float GBS;
    float TPI;
    float CPI;
    float Gflops;
    float time;
    ull FLOPS[FLOPS_EVENTS];
    ull instructions;
    ull cycles;
    ulong avg_f;
    ulong def_f;
    io_data_t iod;
#if USE_GPUS
    gpu_signature_t gpu_sig;
#endif
}ssig_t;


typedef struct signature
{
    double DC_power;
    double DRAM_power;
    double PCK_power;
    double EDP;
    double GBS;
    double IO_MBS;
    double TPI;
    double CPI;
    double Gflops;
    double time;
    ull FLOPS[FLOPS_EVENTS];
    ull L1_misses;
    ull L2_misses;
    ull L3_misses;
    ull instructions;
    ull cycles;
    ulong avg_f;
    ulong avg_imc_f;
    ulong def_f;
    double perc_MPI;
#if USE_GPUS
    gpu_signature_t gpu_sig;
#endif
    void *sig_ext;
} signature_t;



// Misc

/** Initializes all values of the signature to 0.*/
void signature_init(signature_t *sig);

/** Replicates the signature in *source to *destiny */
void signature_copy(signature_t *destiny, signature_t *source);
/** Checks all the values are valid to be reported to DB and fixes potential problems*/
void clean_db_signature(signature_t *sig, double limit);

/** Outputs the signature contents to the file pointed by the fd. */
void signature_print_fd(int fd, signature_t *sig, char is_extended);
void compute_vpi(double *vpi,signature_t *sig);
void compute_ssig_vpi(double *vpi,ssig_t *sig);
void compute_ssig_vpi2(double *vpi,ssig_t *sig);



void print_signature_fd_binary(int fd, signature_t *sig);
void read_signature_fd_binary(int fd, signature_t *sig);
void signature_to_str(signature_t *sig,char *msg,size_t limit);


void acum_sig_metrics(signature_t *dst,signature_t *src);
void acum_ssig_metrics(ssig_t *avg_sig,ssig_t *s);
void compute_avg_sig(signature_t *dst,signature_t *src,int nums);
void compute_avg_node_sig(ssig_t *avg_sig,int n);
void adapt_signature_to_node(signature_t *dest,signature_t *src,float ratio_PPN);
void signature_print_simple_fd(int fd, signature_t *sig);

void from_sig_to_mini(ssig_t *minis,signature_t *s);
void from_minis_to_sig(signature_t *s,ssig_t *minis);
void copy_mini_sig(ssig_t *dst,ssig_t *src);
void minis_to_str(ssig_t *s,char *b);


double sig_total_gpu_power(signature_t *s);
double sig_node_power(signature_t *s);
int sig_gpus_used(signature_t *s);

void set_global_metrics(ssig_t *avg_sig,ssig_t *s);




#endif
