/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _CPU_SUPPORT_H
#define _CPU_SUPPORT_H

#include <library/metrics/metrics.h>
#include <library/models/energy_model.h>

#define DOM_CPU 0
#define DOM_MEM 1
#define DOM_GPU 2
#define DOM_GPU_MEM 3

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif


/** Compute reference metrics used before applying any policy projection/decision. */
state_t compute_reference(signature_t *signature, energy_model_t energy_model, ulong *curr_freq, ulong *def_freq, ulong *freq_ref, double *time_ref, double *power_ref);

/* This function implements min_energy_to_solution policy.
 * Read wiki for more info about policies. */
state_t compute_cpu_freq_min_energy(signature_t *signature, energy_model_t energy_model, ulong freq_ref, double time_ref, double power_ref, double penalty, ulong curr_pstate, ulong minp, ulong maxp, ulong *newf);

/**
 * This function selects a new cpu freq based on min_time_to_solution policy.
 * Read wiki for more info about policies. */
state_t compute_cpu_freq_min_time(signature_t *signature, energy_model_t energy_model, int min_pstate, double time_ref, double min_eff_gain, ulong curr_pstate, ulong best_pstate, ulong best_freq, ulong def_freq, ulong *newf);

/* This function compares signatures with a given margin p. Comparison is done based on CPI and GBS 
 * TODO: This function is also used in states.c, we may put it on a higher level of the library*/
int signatures_different(signature_t *s1, signature_t *s2,float p);

/*  This functions checks whether the current process 'nproc' belongs to critical path and  set it turbo freq and returns 1.
 *  Returns 0 otherwise.*/
uint cpu_supp_try_boost_cpu_freq(int nproc, uint *critical_path, ulong *freqs, int min_pstate);

/* This function compares current freqs with default freqs 
 * */
int are_default_settings(node_freqs_t *freqs,node_freqs_t *def);

/* Copy default settings in freqs */
void set_default_settings(node_freqs_t *freqs, node_freqs_t * def);

void verbose_node_freqs(int vl, node_freqs_t *freqs);
void node_freqs_alloc(node_freqs_t *node_freq);
void node_freqs_free(node_freqs_t *node_freq);
void node_freqs_copy(node_freqs_t * dst, node_freqs_t *src);
uint node_freqs_are_diff(uint flag, node_freqs_t * nf1, node_freqs_t *nf2);

/*  Copy current information to use it in the next iteration as last information. */
state_t copy_cpufreq_sel(ulong *to, ulong *from, size_t size);

/*  Assigns 'freq_val' to 'len' positions of array 'freqs'. */
state_t set_all_cores(ulong *freqs, int len, ulong freq_val);

ulong node_freqs_avgcpufreq(ulong *f);

/** Given a value representing an averaged frequency, returns the nearest frequency (in kHz) rounded. */
ulong avg_to_khz(ulong freq_khz);

#endif

