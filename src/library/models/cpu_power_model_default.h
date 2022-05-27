/*
 * *
 * * This program is part of the EAR software.
 * *
 * * EAR provides a dynamic, transparent and ligth-weigth solution for
 * * Energy management. It has been developed in the context of the
 * * Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
 * *
 * * Copyright © 2017-present BSC-Lenovo
 * * BSC Contact   mailto:ear-support@bsc.es
 * * Lenovo contact  mailto:hpchelp@lenovo.com
 * *
 * * This file is licensed under both the BSD-3 license for individual/non-commercial
 * * use and EPL-1.0 license for commercial use. Full text of both licenses can be
 * * found in COPYING.BSD and COPYING.EPL files.
 * */

#ifndef CPU_POWER_MODEL_INTEL_SKL_H_
#define CPU_POWER_MODEL_INTEL_SKL_H_

#define IPC_COEFF (double)11.40897054
#define GBS_COEFF (double)0.601652592
#define VPI_COEFF (double)28.21247619
#define F_COEFF   (double)9.26892E-05
#define INTERCEPT (double)-28.85109789


typedef struct intel_skl{
  double ipc;
  double gbs;
  double vpi;
  double f;
  double inter;
}intel_skl_t;

#endif
