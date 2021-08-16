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

#include <metrics/common/pstate.h>

state_t pstate_get_index(pstate_t *list, uint count, ullong khz, uint *index)
{
	int i;
	for (i = 0; i < count; ++i) {
		if (list[i].khz == khz) {
			*index = i;
			return EAR_SUCCESS;
		}
	}
	return_msg(EAR_ERROR, Generr.not_found);
}

void pstate_get_index_closer(pstate_t *list, uint count, ullong khz, uint *index, uint mode)
{
	int i;

	if (mode == MODE_ABOVE && list[0].khz <= khz) {
		*index = 0;
	}
	for (i = 0; i < count; ++i) {
		if (mode == MODE_ABOVE && list[i].khz <= khz) {
			*index = i-1;
		}
		if (mode == MODE_BELOW && list[i].khz < khz) {
			*index = i;
		}
		if (list[i].khz == khz) {
			*index = i;
		}
	}
	*index = count-1;
}

void pstate_print(pstate_t *pstate_list, uint pstate_count, int fd)
{
	char buffer[SZ_BUFFER];
	dprintf(fd, "%s", pstate_tostr(pstate_list, pstate_count, buffer, SZ_BUFFER));
}

char *pstate_tostr(pstate_t *pstate_list, uint pstate_count, char *buffer, int length)
{
	int already = 0;
	int printed;
	int i;

	for (i = 0; i < pstate_count && length > 0; ++i) {
		printed = snprintf(&buffer[already], length, "PS%d: id%d, %llu KHz\n",
						i, pstate_list[i].idx, pstate_list[i].khz);
		length  = length  - printed;
		already = already + printed;
	}
	return buffer;
}

state_t avgfreq_to_pstate(pstate_t *pstate_list, uint pstate_count,ulong khz, pstate_t *p)
{
  int i;
  state_t s = EAR_ERROR;
  for (i = 0; i < pstate_count; i ++){
    if (pstate_list[i].khz < khz){
      memcpy(p,&pstate_list[i-1],sizeof(pstate_t));
      s = EAR_SUCCESS;
			return s;
    }
  }
	if (s == EAR_ERROR){
		memcpy(p,&pstate_list[pstate_count-1],sizeof(pstate_t));
		s = EAR_SUCCESS;
	}
  return s;
}

state_t pstate_to_freq(pstate_t *pstate_list, uint pstate_count,int pstate_idx, ulong *khz)
{
	if (pstate_idx >= pstate_count) return EAR_ERROR;
	*khz = pstate_list[pstate_idx].khz;
	return EAR_SUCCESS;
}

state_t from_freq_to_pstate(pstate_t *pstate_list, uint pstate_count,ulong khz, pstate_t *p)
{
	int i;
	state_t s = EAR_ERROR;
	for (i = 0; i < pstate_count; i ++){
		if (pstate_list[i].khz == khz){
			memcpy(p,&pstate_list[i],sizeof(pstate_t));
			s = EAR_SUCCESS;
		}
	}
	return s;
}
