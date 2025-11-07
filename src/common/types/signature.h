/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_TYPES_SIGNATURE
#define _EAR_TYPES_SIGNATURE

#include <common/config.h>
#include <common/types/generic.h>
#include <common/utils/serial_buffer.h>
#include <metrics/cpi/cpi.h>
#include <metrics/flops/flops.h>
#include <metrics/gpu/gpu.h>
#include <metrics/io/io.h>

// 0: float
// 1: 128 float
// 2: 256 float
// 3: 512 float
// 4: double
// 5: 128 double
// 6: 256 double
// 7: 512 double

typedef struct gpu_app {
    double GPU_power;
    ulong GPU_freq;
    ulong GPU_mem_freq;
    ulong GPU_util;
    ulong GPU_mem_util;
#if WF_SUPPORT
    float GPU_GFlops;
    ulong GPU_temp;
    ulong GPU_temp_mem;
#endif
} gpu_app_t;

/** GPU part. */
typedef struct gpu_signature {
    int num_gpus;
    gpu_app_t gpu_data[MAX_GPUS_SUPPORTED];
} gpu_signature_t;

/** CPU part. */
typedef struct cpu_signature {
    uint devs_count;
    llong temp[MAX_SOCKETS_SUPPORTED];
    double cpu_power[MAX_SOCKETS_SUPPORTED];
    double dram_power[MAX_SOCKETS_SUPPORTED];
} cpu_signature_t;

/** Process part. */
typedef struct proc_stat_signature_s {
    uint cpu_util;
    uint mem_util;
} proc_stat_signature_t;

/** A type with cache metrics we currently can compute.
 * It contains misses, hits, total accesses, miss rate and hit rate
 * for L1d, L2, L3 and LL cache levels. */
typedef struct cache_signature_s {
    ull l1d_misses;
    ull l2_misses;
    ull l3_misses;
    ull ll_misses;

    ull l1d_hits;
    ull l2_hits;
    ull l3_hits;
    ull ll_hits;

    ull l1d_accesses;
    ull l2_accesses;
    ull l3_accesses;
    ull ll_accesses;

    double l1d_miss_rate;
    double l2_miss_rate;
    double l3_miss_rate;
    double ll_miss_rate;

    double l1d_hit_rate;
    double l2_hit_rate;
    double l3_hit_rate;
    double ll_hit_rate;
} cache_signature_t;

/** A type with cache metrics reduced to be used in a shared signature.
 * It contains the same information as 'cache_signature_t' except hit and miss rate
 * for saving memory space in the shared signatures. */
typedef struct cache_mini_signature_s {
    ull l1d_misses;
    ull l2_misses;
    ull l3_misses;
    ull ll_misses;

    ull l1d_hits;
    ull l2_hits;
    ull l3_hits;
    ull ll_hits;

    ull l1d_accesses;
    ull l2_accesses;
    ull l3_accesses;
    ull ll_accesses;
} cache_shsignature_t;

/** A reduced size signature type to be used in shared memory regions. */
typedef struct mini_sig {
    float DC_power;
    float DRAM_power;
    float PCK_power;
    float GBS;
    float TPI;
    float CPI;
    float Gflops;
    float time;
    float IO_MBS;
    ulong avg_f;
    ulong def_f;
    ulong accum_energy;
    ulong accum_dram_energy;
    ulong accum_pack_energy;
    ulong accum_avg_f;
    double valid_time;
    ull FLOPS[FLOPS_EVENTS];
    ull instructions;
    ull cycles;
    stalls_t stalls;
    ull L1_misses;
    ull L2_misses;
    ull L3_misses;
    ull accum_mem_access;
    cache_shsignature_t cache;
    io_data_t iod;
#if USE_GPUS
    gpu_signature_t gpu_sig;
#endif
} ssig_t;

// WARNING! This type is serialized through functions signature_serialize and
// signature_deserialize. If you want to add new types, make sure to update
// these functions too.
typedef struct signature {
    double DC_power;
    double DRAM_power;
    double PCK_power;
    double DC_job_power;
    double PCK_job_power;
    double DRAM_job_power;
    double EDP; // TODO: ?
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
    stalls_t stalls;
    ulong avg_f;
    ulong avg_imc_f;
    ulong def_f;
    double perc_MPI;
#if USE_GPUS
    gpu_signature_t gpu_sig;
#endif
#if WF_SUPPORT
    cpu_signature_t cpu_sig;
#endif
    proc_stat_signature_t ps_sig;
    cache_signature_t cache;
    void *sig_ext;
} signature_t;

/** Initializes all values of the signature to 0.
 * \param[out] sig The signature pointer to be initialized. */
void signature_init(signature_t *sig);

/** Replicates the signature in *source to *destiny. */
void signature_copy(signature_t *destiny, const signature_t *source);

/** Checks all the values are valid to be reported to DB and fixes potential problems. */
void signature_clean_before_db(signature_t *sig, double pwr_limit);

/** Outputs the signature contents to the file pointed by the fd.
 * The output is a semi-colon separated row of the signature data with the following order:
 * - Avg. CPU freq., Avg. IMC freq., Def. CPU freq.
 * - Time, CPI, TPI, GB/s, I/O MB/s, % MPI,
 * - DC Pwr, DRAM Pwr, PCK Pwr
 * - Cycles, Instructions, GFlop/s
 * - If extended: L1, L2 L3 misses and FLOPS events.
 * - If EAR is compiled with GPU support, GPU pwr, freq, mem_freq, util and memt_util is printed
 *   for each GPU. */
void signature_print_fd(int fd, signature_t *sig, char is_extended, int single_column, char sep);

/** Computes the VPI of \p sig and stores it at \p vpi. */
void compute_sig_vpi(double *vpi, const signature_t *sig);

/** \todo  */
void compute_ssig_vpi(double *vpi, const ssig_t *sig);

/** \todo */
void compute_ssig_vpi2(double *vpi, ssig_t *sig);

/** \todo */
void print_signature_fd_binary(int fd, signature_t *sig);

/** \todo */
void read_signature_fd_binary(int fd, signature_t *sig);

/** \todo */
state_t signature_to_str(signature_t *sig, char *msg, size_t limit);

/** \todo */
void acum_sig_metrics(signature_t *dst, signature_t *src);

/** Accumulate metrics values from \p src_sig to \p dst_sig.
 * If some of the input arguments are NULL, returns EAR_ERROR. */
state_t ssig_accumulate(ssig_t *dst_ssig, ssig_t *src_ssig);

/* Multiplies the values in src by nums, and saves them in dst.
 * This applies a weight to each signature, and then one can accumulate
 * them with acum_sig_metrics and finally divide them by the sum
 * of weights with compute_avg_sig to ponderate several signatures */
void signature_apply_weight(signature_t *dst, signature_t *src, int nums);

/** \todo */
void compute_avg_sig(signature_t *dst, signature_t *src, int nums);

/** \todo */
void adapt_signature_to_node(signature_t *dest, signature_t *src, float ratio_PPN);

/** \todo */
void signature_print_simple_fd(int fd, signature_t *sig);

/** Fills data from the signature \p src_sig to the mini signature \p dst_ssig. */
state_t ssig_from_signature(ssig_t *dst_ssig, signature_t *src_sig);

/** Fills data from the mini signature \p src_ssig to the signature \p dst_sig. */
state_t signature_from_ssig(signature_t *dst_sig, ssig_t *src_ssig);

/** Transforms \p ssig data to a readable format. The result is stored at \p dst_str, which must have
 * allocated at least \p n bytes. */
void ssig_tostr(ssig_t *ssig, char *dst_str, size_t n);

/** \todo */
double sig_total_gpu_power(signature_t *s);

/** \todo */
double sig_node_power(signature_t *s);

/** \todo */
int sig_gpus_used(signature_t *s);

void signature_serialize(serial_buffer_t *b, signature_t *sig);

void signature_deserialize(serial_buffer_t *b, signature_t *sig);

double compute_dc_nogpu_power(signature_t *lsig);

#endif
