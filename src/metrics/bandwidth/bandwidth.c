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

/*
 * Usage:
 * 1) Call init passing the processor model of clusters
 * nodes to initialize the uncores scan and allocate file
 * descriptors memory.
 * 2) Call reset and start when you want to begin to count
 * the bandwith for a period of time.
 * 3) Call stop passing an array of unsigned long long
 * to also get the final uncore counters values.
 * 4) Repeat steps 2 and 3 every time you need to obtain
 * counters values.
 * 4) Call dispose to close the file descriptors and also
 * free it's allocated memory.
 *
 * When an error occurs, those calls returns -1.
 */

#include <limits.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/bandwidth/cpu/dummy.h>
#include <metrics/bandwidth/cpu/amd49.h>
#include <metrics/bandwidth/cpu/intel63.h>

static struct uncore_op {
	state_t (*init)		(ctx_t *c, topology_t *tp);
	state_t (*dispose)	(ctx_t *c);
	state_t (*count)	(ctx_t *c, uint *count);
	state_t (*start)	(ctx_t *c);
	state_t (*reset)	(ctx_t *c);
	state_t (*read)		(ctx_t *c, ullong *cas);
	state_t (*stop)		(ctx_t *c, ullong *cas);
} ops;

static ctx_t context;
static ctx_t *c = &context;
static topology_t topo;

// Depending on the architecture delivered by cpu_model variable,
// the pmons structure would point to its proper reading functions.
int init_uncores(int cpu_model)
{
	topology_init(&topo);

	if (state_ok(bwidth_intel63_status(&topo)))
	{
		debug("selected intel63");
		ops.init  = bwidth_intel63_init;
		ops.count = bwidth_intel63_count;
		ops.reset = bwidth_intel63_reset;
		ops.start = bwidth_intel63_start;
		ops.stop  = bwidth_intel63_stop;
		ops.read  = bwidth_intel63_read;
		ops.dispose = bwidth_intel63_dispose;
	}
	else if (state_ok(bwidth_amd49_status(&topo)))
	{
		debug("selected amd49");
		ops.init    = bwidth_amd49_init;
		ops.count   = bwidth_amd49_count;
		ops.reset   = bwidth_amd49_reset;
		ops.start   = bwidth_amd49_start;
		ops.stop    = bwidth_amd49_stop;
		ops.read    = bwidth_amd49_read;
		ops.dispose = bwidth_amd49_dispose;
	}
	else
	{
		debug("selected dummy");
		ops.init    = bwidth_dummy_init;
		ops.count   = bwidth_dummy_count;
		ops.reset   = bwidth_dummy_reset;
		ops.start   = bwidth_dummy_start;
		ops.stop    = bwidth_dummy_stop;
		ops.read    = bwidth_dummy_read;
		ops.dispose = bwidth_dummy_dispose;
	}

	if (state_ok(ops.init(c, &topo))) {
		return count_uncores();
	}

	return 0;
}

int dispose_uncores()
{
	preturn (ops.dispose, c);
}

int count_uncores()
{
	int count = 0;
	if (ops.count != NULL) {
		ops.count(c, (uint *) &count);
	}
	return count;
	//preturn (ops.count, c);
}

int check_uncores()
{
	return EAR_SUCCESS;
}

int reset_uncores()
{
	preturn (ops.reset, c);
}

int start_uncores()
{
	#if 0
	if (ops.start != NULL) {
		ops.start(c);
	}
	
	return read_uncores(NULL);
	#endif
	preturn (ops.start, c);
}

int read_uncores(ullong *cas)
{
	preturn (ops.read, c, cas);
}

int stop_uncores(ullong *cas)
{
	preturn (ops.stop, c, cas);
}

ullong acum_uncores(ullong *cas,int N)
{
	ullong acum=0;
	int i;
  int dev_count = N;
  if (dev_count == 0) {
    return_msg(EAR_ERROR, Generr.api_uninitialized);
  }
	  for (i = 0; i < dev_count; ++i) {
    acum += cas[i] ;
  }
	return acum;
}

ullong acum_diff(ullong *end, ullong *init,int N)
{
  ullong acum=0;
  int i;
  int dev_count = N;
  if (dev_count == 0) {
    return_msg(EAR_ERROR, Generr.api_uninitialized);
  }
  for (i = 0; i < dev_count; ++i) {
		if (end[i] >= init[i]){
			acum += end[i] -init[i];
		}else{
    	acum += uncore_ullong_diff_overflow(init[i],end[i]);
		}
  }
  return acum;
}

int compute_mem_bw(ullong *cas2, ullong *cas1, double *bps, double t,int N)
{
	int dev_count = N;
	ullong accum;
	if (dev_count == 0) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	topology_init(&topo);
	accum = acum_diff(cas2,cas1,N);
	debug("Total uncore events %llu cache line size %u\n",accum,topo.cache_line_size);
	*bps = (double)(accum * topo.cache_line_size)/(t*BW_GB);
	return EAR_SUCCESS;
}

int compute_tpi(ullong *cas2, ullong *cas1, double *tpi, ullong inst,int N)
{
  int dev_count = N;
  ullong accum;
  if (dev_count == 0) {
    return_msg(EAR_ERROR, Generr.api_uninitialized);
  }
  topology_init(&topo);
  accum = acum_diff(cas2,cas1,N);
  debug("Total uncore events %llu cache line size %u\n",accum,topo.cache_line_size);
  *tpi = (double)(accum * topo.cache_line_size)/(double)inst;
  return EAR_SUCCESS;
}


int compute_uncores(ullong *cas2, ullong *cas1, double *bps, double units)
{
	int dev_count = count_uncores();
	ullong accum=0;
	if (dev_count == 0) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	accum = acum_diff(cas2,cas1,dev_count);
	*bps = (double) accum;
	*bps = ((*bps)*64.0) / (units*4.0);
	return EAR_SUCCESS;
}


int alloc_array_uncores(ullong **array)
{
	int dev_count = count_uncores();
	if (dev_count == 0) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	*array = calloc(dev_count, sizeof(ullong));
	return EAR_SUCCESS;
}

int alloc_uncores(ullong **array,int N)
{
  int dev_count = N;
  if (dev_count == 0) {
    return_msg(EAR_ERROR, Generr.api_uninitialized);
  }
  *array = calloc(dev_count, sizeof(ullong));
  return EAR_SUCCESS;

}


ullong uncore_ullong_diff_overflow(ullong begin, ullong end)
{
	ullong max_64 = ULLONG_MAX;
	ullong max_48 = 281474976710656; //2^48
	ullong ret = 0;

	if (begin < max_48 && end < max_48) {
		ret = max_48 - begin + end;
	} else {
		ret = max_64 - begin + end;
	}
	return ret;
}

void diff_uncores(ullong * diff,ullong *end,ullong  *begin,int N)
{
	int i;
	for (i=0;i<N;i++){
		if (end[i]<begin[i]){
			diff[i]=uncore_ullong_diff_overflow(begin[i],end[i]);
		}else{
			diff[i]=end[i]-begin[i];
		}
	}
}

void copy_uncores(ullong * DEST,ullong * SRC,int N)
{
	memcpy((void *)DEST, (void *)SRC, N*sizeof(ullong));
}

int uncore_are_frozen(ullong * DEST,int N)
{
	int i,frozen=1;
	for (i=0;i<N;i++){
		if (DEST[i]>0){
			frozen=0;
			break;
		}
	}
	return frozen;
}

void print_uncores(unsigned long long * DEST,int N)
{
  int i;
  for (i=0;i<N;i++){
		fprintf(stdout,"Counter %d= %llu \t",i,DEST[i]);
	}
	fprintf(stdout,"\n");
}

void uncores_to_str(unsigned long long * DEST,int N,char *txt,int len)
{
	int i;
	char coun[128];
	txt[0]='\0';
  for (i=0;i<N;i++){
		sprintf(coun,"cas[%d]= %llu ",i,DEST[i]);
		if ((strlen(txt) +  strlen(coun)) > len) return;
		strcat(txt,coun);	
  }
}

