/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1
#include <common/types/projection.h>


double basic_project_cpi(signature_t *sign, coefficient_t *coeff)
{
  return (coeff->D * sign->CPI) +
    (coeff->E * sign->TPI) +
    (coeff->F);
}


double basic_project_time(signature_t *sign,coefficient_t *coeff)
{
  double ptime=0;
  if (coeff->available){
    double proj_cpi = basic_project_cpi(sign, coeff);
    double freq_src = (double) coeff->pstate_ref;
    double freq_dst = (double) coeff->pstate;

    ptime= ((sign->time * proj_cpi) / sign->CPI) *
      (freq_src / freq_dst);
  }
  return ptime;
}


double basic_project_power(signature_t *sign, coefficient_t *coeff)
{
  double ppower=0;
  if (coeff->available){
    ppower= (coeff->A * sign->DC_power) +
      (coeff->B * sign->TPI) +
      (coeff->C);
  }
  return ppower;
}
