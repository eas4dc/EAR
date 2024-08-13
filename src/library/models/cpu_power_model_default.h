/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
