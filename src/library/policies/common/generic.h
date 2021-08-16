#ifndef _POLICY_GENERIC_H
#define _POLICY_GENERIC_H

#include <common/types.h>

#define RATIO_DECR 0
#define RATIO_INCR 1

/*  Returns the gain of metric_curr with respect to metric_ref.
 *  'type' parameter determines whether the gain must be treated as
 *  an increment (RATIO_INCR) or decrement (RATIO_DECR) of the metric. */
double metric_ratio(double metric_ref, double metric_curr, uint type);

/* This function returns whether either time (when using DynAIS) or CPI or GBS (Periodic Mode) are above 
 * some threshold from their respective reference value. */
uint above_max_penalty(double time_ref, double time_curr, double cpi_ref, double cpi_curr,
        double gbs_ref, double gbs_curr, double penalty_th);

/* This functions returns whether either time (when using DynAIS) or CPI or GBS (Periodic Mode) improve below
 * some threshold than the frecuency gain. */
uint below_perf_min_benefit(double time_ref, double time_curr, double cpi_ref, double cpi_curr,
        double gbs_ref, double gbs_curr, double freq_ref, double freq_curr, double penalty_th);
#endif
