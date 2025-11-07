#ifndef _POLICY_GENERIC_H
#define _POLICY_GENERIC_H

#include <common/types.h>
#include <common/types/signature.h>
#include <library/metrics/metrics.h>
#include <library/models/energy_model.h>
#include <library/policies/policy_ctx.h>

#define RATIO_DECR 0
#define RATIO_INCR 1

/*  Returns the gain of metric_curr with respect to metric_ref.
 *  'type' parameter determines whether the gain must be treated as
 *  an increment (RATIO_INCR) or decrement (RATIO_DECR) of the metric. */
double metric_ratio(double metric_ref, double metric_curr, uint type);

/* This function returns whether either time (when using DynAIS) or CPI or GBS (Periodic Mode) are above
 * some threshold from their respective reference value. */
uint above_max_penalty(double time_ref, double time_curr, double cpi_ref, double cpi_curr, double gbs_ref,
                       double gbs_curr, double penalty_th);

/* This functions returns whether either time (when using DynAIS) or CPI or GBS (Periodic Mode) improve below
 * some threshold than the frecuency gain. */
uint below_perf_min_benefit(double time_ref, double time_curr, double cpi_ref, double cpi_curr, double gbs_ref,
                            double gbs_curr, double freq_ref, double freq_curr, double penalty_th);

void compute_energy_savings(signature_t *curr, signature_t *prev);

void compute_policy_savings(energy_model_t energy_model, signature_t *ns, node_freqs_t *freqs, node_freq_domain_t *dom);

void compute_gpu_policy_savings(energy_model_t energy_model, signature_t *ns, node_freqs_t *freqs);
void compute_gpu_energy_savings(signature_t *curr);
#endif
