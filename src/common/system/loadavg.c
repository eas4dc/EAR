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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/states.h>

state_t loadavg(float *min,float *Vmin,float *XVmin,uint * runnable,uint *total,uint *lastpid)
{
  FILE *f;
  f = fopen("/proc/loadavg","r");
  if (f == NULL)
  {
		return EAR_ERROR;
  }
  if (fscanf(f,"%f %f %f %u/%u %u",min,Vmin,XVmin,runnable,total,lastpid)) {
      // Just for warning purposes
  }
  fclose(f);
  return EAR_SUCCESS;
}

