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

//#define SHOW_DEBUGS 1

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <common/config.h>
#include <common/states.h>
#include <common/math_operations.h>
#include <common/output/verbose.h>
#include <management/cpufreq/frequency.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/policies/common/imc_policy_support.h>
#include <library/policies/common/generic.h>

extern volatile uint ear_periodic_mode;
extern ulong def_imc_max_khz;

#define IMC_STEP 100000

#define DIFF_SIG 0.15
uint must_start(imc_data_t *my_imc_data,uint cpu_pstate, uint imc_pstate,
        uint ref_cpu_pstate, uint ref_imc_pstate, signature_t *sig)
{
    uint ref_pstate;
    double CPI,GBS,CPI_ref,GBS_ref, rCPI, rGBS;
    ref_pstate = ref_cpu_pstate;
    CPI_ref = (double)my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].cycles
        / (double)my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].instructions;
    GBS_ref = my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].GBS;
    CPI     = sig->CPI;
    GBS	    = sig->GBS;
    if (CPI > CPI_ref) rCPI = (CPI - CPI_ref)/CPI;
    else               rCPI = (CPI_ref - CPI)/CPI_ref;
    if (GBS > GBS_ref) rGBS = (GBS - GBS_ref)/GBS;
    else               rGBS = (GBS_ref - GBS)/GBS_ref;
    if ((rCPI > DIFF_SIG) || (rGBS > DIFF_SIG)) return 1;
    else return 0;
}

uint must_increase_imc(imc_data_t *my_imc_data, uint cpu_pstate, uint imc_pstate,
        uint ref_cpu_pstate, uint ref_imc_pstate, signature_t *sig, double penalty)
{
    double time_ref = my_imc_data[ref_cpu_pstate*NUM_UNC_FREQ+ref_imc_pstate].time;
    double time_curr = sig->time;

    double cpi_ref = my_imc_data[ref_cpu_pstate*NUM_UNC_FREQ+ref_imc_pstate].CPI;
    double cpi_curr = sig->CPI;

    double gbs_ref = my_imc_data[ref_cpu_pstate*NUM_UNC_FREQ+ref_imc_pstate].GBS;
    double gbs_curr = sig->GBS;

    debug("TIME ref %lf curr %lf CPI ref %lf curr %lf GBS ref %lf curr %lf PENALTY_TH %lf",
            time_ref, time_curr, cpi_ref, cpi_curr, gbs_ref, gbs_curr, penalty);
    return above_max_penalty(time_ref, time_curr, cpi_ref, cpi_curr, gbs_ref, gbs_curr, penalty);
#if 0
    double metric,metric_ref,metric2;
    double CPI;
    double metric_max,metric_min;
#if SHOW_DEBUGS
    double metric_var, power_var;
#endif
    uint ref_pstate;

    ref_pstate = ref_cpu_pstate;
    debug("Validating IMC freq selection: cpu_pstate %u imc_pstate %u",ref_pstate,ref_imc_pstate);

    if (ear_periodic_mode == PERIODIC_MODE_OFF){
        // TODO: Should we check for an energy consumption increase before time check as
        // we do in policy_no_models_is_better_min_energy ?
        metric_ref = my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].time;
        metric_max = metric_ref * (1 + penalty);
        metric = sig->time;
#if SHOW_DEBUGS
        double power_ref = my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].power;
        power_var = metric_ratio(power_ref, sig->DC_power, RATIO_DECR);
        metric_var = metric_ratio(metric_ref, metric, RATIO_INCR);
        debug("Using time as reference : Current %.3lf Ref %3.lf  Max %.3lf energy_saving %.2lf (%.2lf - %.2lf)",
                metric,metric_ref,metric_max,(power_var-metric_var)*100,power_var,metric_var);
#endif
        return (metric > metric_max);
    }else{
        /* We were using ref_pstate = min_pstate */
        metric_ref = my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].GBS;
        metric_min = metric_ref * (1 - penalty);
        metric     = sig->GBS;
        CPI         = my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].CPI;
        metric2     = sig->CPI;
        metric_max  = CPI + CPI*penalty;
#if SHOW_DEBUGS
        debug("Using GBS&CPI as reference : Current (%.2lf,%.2lf) Ref (%.2lf,%.2lf) Limits (%.2lf,%.2lf)",
                metric,metric2,metric_ref,CPI,metric_min,metric_max);
#endif
        return ((metric < metric_min) || (metric2>metric_max));
    }
#endif
}

uint must_decrease_imc(imc_data_t *my_imc_data,uint cpu_pstate,uint my_imc_pstate,
        uint ref_cpu_pstate,uint ref_imc_pstate,signature_t *sig, double min_eff_gain)
{
    double time_ref = my_imc_data[ref_cpu_pstate*NUM_UNC_FREQ+ref_imc_pstate].time;
    double time_curr = sig->time;

    double cpi_ref = my_imc_data[ref_cpu_pstate*NUM_UNC_FREQ+ref_imc_pstate].CPI;
    double cpi_curr = sig->CPI;

    double gbs_ref = my_imc_data[ref_cpu_pstate*NUM_UNC_FREQ+ref_imc_pstate].GBS;
    double gbs_curr = sig->GBS;

    ulong ref_imc_freq;
    if (state_fail(pstate_to_freq((pstate_t *)imc_pstates, imc_num_pstates, ref_imc_pstate,&ref_imc_freq))){
        verbose_master(2, "%sIMC pstates invalid %d%s", COL_RED, ref_imc_pstate, COL_CLR);
        return 1;
    }
    double freq_ref = (double)ref_imc_freq;
    pstate_t ps;
    avgfreq_to_pstate((pstate_t *)imc_pstates, imc_num_pstates, sig->avg_imc_f, &ps);
    double freq_curr = (double)imc_pstates[ps.idx].khz;

    debug("TIME ref %lf curr %lf CPI ref %lf curr %lf GBS ref %lf curr %lf IMC FREQ ref %lf curr %lf EFF_GAIN_TH %lf", 
            time_ref, time_curr, cpi_ref, cpi_curr, gbs_ref, gbs_curr, freq_ref, freq_curr, min_eff_gain);
    return below_perf_min_benefit(time_ref, time_curr, cpi_ref, cpi_curr, gbs_ref,
            gbs_curr, freq_ref, freq_curr, min_eff_gain);
#if 0
    double metric,metric_ref,metric2;
    double CPI;
    double metric_gain, metric_gain_2;
    double imc_gain;
    uint ref_pstate;
    ulong ref_imc_freq;
    pstate_t ps;

    ref_pstate = ref_cpu_pstate;
    debug("Validating IMC freq selection: cpu_pstate %u imc_pstate %u refs %u %u",
            cpu_pstate, my_imc_pstate, ref_pstate, ref_imc_pstate);


    // We get the normalized imc frequency that corresponds with the avg imc freq
    avgfreq_to_pstate((pstate_t *)imc_pstates, imc_num_pstates, sig->avg_imc_f, &ps);
    ulong curr_imc_freq = imc_pstates[ps.idx].khz;
    if (pstate_to_freq((pstate_t *)imc_pstates, imc_num_pstates, ref_imc_pstate,&ref_imc_freq) != EAR_SUCCESS){
        debug("IMC pstates invalid %d",ref_imc_pstate);
    }
    debug("Current IMC freq %s%lu%s | Ref IMC freq %s%lu%s", COL_GRE, curr_imc_freq, COL_CLR, COL_GRE, ref_imc_freq, COL_CLR);
    imc_gain = metric_ratio((double)ref_imc_freq, (double)curr_imc_freq, RATIO_INCR);
    double perf_min_benefit = imc_gain * min_eff_gain;

    if (ear_periodic_mode == PERIODIC_MODE_OFF){
        metric_ref  = my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].time;
        metric      = sig->time;
        metric_gain = metric_ratio(metric_ref, metric, RATIO_DECR);
        debug("Using time as reference: Current %.3lf Ref %3.lf Gain %.3lf IMC gain: %.3lf",
                metric, metric_ref, metric_gain, imc_gain);
        return (metric_gain < perf_min_benefit);
    }else{
        /* We were using ref_pstate = min_pstate */
        metric_ref  = my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].GBS;
        metric      = sig->GBS;
        metric_gain = metric_ratio(metric_ref, metric, RATIO_INCR);
        CPI         = my_imc_data[ref_pstate*NUM_UNC_FREQ+ref_imc_pstate].CPI;
        metric2     = sig->CPI;
        metric_gain_2 = metric_ratio(CPI, metric2, RATIO_DECR);
        debug("Using GBS&CPI as reference: Current (%.4lf,%.4lf) Ref (%.4lf,%.4lf) Gain (%.4lf, %.4lf) IMC gain: %.3lf",
                metric, metric2, metric_ref,CPI, metric_gain, metric_gain_2, imc_gain);
        return ((metric_gain < perf_min_benefit) || (metric_gain_2 < perf_min_benefit));
    }
#endif
}

void copy_imc_data_from_signature(imc_data_t *my_imc_data,uint cpu_pstate,uint my_imc_pstate,signature_t *s)
{
    int i;
    ullong tfops = 0;
    my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].time = s->time;
    my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].power = s->DC_power;
    my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].GBS = s->GBS;
    my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].CPI = s->CPI;
    my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].instructions = s->instructions;
    my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].cycles = s->cycles;
    memcpy(my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].FLOPS , s->FLOPS,sizeof(ull)*FLOPS_EVENTS);
    for (i=0;i<FLOPS_EVENTS;i++) tfops += s->FLOPS[i];
#if SHOW_DEBUGS
    float flops = 0;
    flops = (float)tfops/(s->time*1000000000.0);
    debug("IMC data: time %.3lf power %2.lf GBS %.2lf I/s %.2f flops %.2f",
            my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].time,
            my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].power,
            my_imc_data[cpu_pstate*NUM_UNC_FREQ+my_imc_pstate].GBS,s->instructions/(s->time*1000000000.0),
            flops);
#endif
}

ulong select_imc_freq(ulong max,ulong min,float p)
{
    ulong new;
    new = max-(float)(max-min)*p;
    return new;
}

uint select_imc_pstate(int num_pstates,float p)
{
    return (uint)truncf(num_pstates * p);
}

