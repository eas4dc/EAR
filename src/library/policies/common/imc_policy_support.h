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

#ifndef _IMC_SUPPORT_H
#define _IMC_SUPPORT_H

#include <common/types/signature.h>
#include <management/imcfreq/imcfreq.h>

typedef struct imc_data{
    double time;
    double GBS;
    double power;
    double CPI;
    ull instructions;
    ull cycles;
    ull FLOPS[FLOPS_EVENTS];
}imc_data_t;

extern uint imc_devices;
extern uint imc_num_pstates;
extern const pstate_t *imc_pstates;

#define NUM_UNC_FREQ 20

#define IMC_STEP        100000
#define IMC_RANGE       100000

#define IMC_MAX 0
#define IMC_MIN 1
#define IMC_VAL imc_devices

/*  These values selects which metric the policy will use to take control of IMC.
 *  Only one of the four must be assigned a 1, the others must be assigned a 0.
 *  -- NOT USED -- */
#define USE_FLOPS 0
#define USE_GBS 0
#define USE_CPI	0
#define USE_GBS_CPI 1

/*  Stores relevant data from signature to take care of IMC control. */
void copy_imc_data_from_signature(imc_data_t *my_imc_data,uint cpu_pstate,uint imc_pstate,signature_t *s);

/*  This function decides whether policy must stop to decrease IMC frequency.
 *  If policy is guided by DynAIS, this function will check time penalty.
 *  If not, it will check for penalty of one of the metrics defined above. */
uint must_increase_imc(imc_data_t *my_imc_data,uint cpu_pstate,uint imc_pstate,uint ref_cpu_pstate,uint ref_imc_pstate,
        signature_t *sig,double penalty);

/*  This function decides whether policy must stop to increase IMC frequency.
 *  It's decision is based on whether performance gain of some metric is lower than
 *  the increase of current IMC frequency with respect to reference IMC frequency. */
uint must_decrease_imc(imc_data_t *my_imc_data,uint cpu_pstate,uint imc_pstate,
        uint ref_cpu_pstate,uint ref_imc_pstate,signature_t *sig, double min_eff_gain);

/* This function evaluates if the current signature is too much different from our reference and we should start again 
 */
uint must_start(imc_data_t *my_imc_data,uint cpu_pstate, uint imc_pstate,
        uint ref_cpu_pstate, uint ref_imc_pstate, signature_t *sig);


uint select_imc_pstate(int num_pstates,float p);

#endif
