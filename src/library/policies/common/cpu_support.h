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

#ifndef _CPU_SUPPORT_H
#define _CPU_SUPPORT_H

/* Compute reference metrics used before applying any policy projection/decision. */
state_t compute_reference(polctx_t *c, signature_t *my_app, ulong *curr_freq, ulong *def_freq, ulong *freq_ref, double *time_ref, double *power_ref);

/* This function implements min_energy_to_solution policy.
 * Read wiki for more ifo about policies. */
state_t compute_cpu_freq_min_energy(polctx_t *c,signature_t *my_app,ulong freq_ref,double time_ref,double power_ref,double penalty,ulong curr_pstate,ulong  minp,ulong maxp,ulong *newf);

/**
 * This function selects a new cpu freq based on min_time_to_solution policy.
 * Read wiki for more info about policies.  */
state_t compute_cpu_freq_min_time(signature_t *my_app, int min_pstate, double time_ref,
        double min_eff_gain, ulong curr_pstate, ulong best_pstate, ulong best_freq, ulong def_freq, ulong *newf);

/* Computes the average CPU frequency across all processes. */
void process_avg_cpu_freq(cpu_set_t *aff, ulong *cpuflist, ulong *avgcpu);

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
void node_freqs_copy(node_freqs_t * dst, node_freqs_t *src);

/*  Copy current information to use it in the next iteration as last information. */
state_t copy_cpufreq_sel(ulong *to, ulong *from, size_t size);

/*  Assigns 'freq_val' to 'len' positions of array 'freqs'. */
state_t set_all_cores(ulong *freqs, int len, ulong freq_val);

ulong node_freqs_avgcpufreq(ulong *f);

#endif

