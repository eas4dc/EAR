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

// #define SHOW_DEBUGS 1
#define _GNU_SOURCE
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>

#include <common/config.h>
#include <common/states.h>
#include <common/hardware/hardware_info.h>
#include <common/output/verbose.h>
#include <common/types/projection.h>
#include <common/types/configuration/cluster_conf.h>

#include <metrics/flops/flops.h>

#include <daemon/local_api/node_mgr.h>
#include <library/common/externs.h>
#include <library/models/cpu_power_model.h>
#include <library/common/library_shared_data.h>
#include <library/common/verbose_lib.h>
#include <library/common/global_comm.h>
#include <common/environment.h>


#define ear_msync(a,b,c) msync(a,b,c)
//#define ear_msync(a,b,c) 0

/* Node mgr variables */
extern ear_njob_t *node_mgr_data;
extern uint node_mgr_index;
extern node_mgr_sh_data_t *node_mgr_job_info;
extern char lib_shared_region_path_jobs[GENERIC_NAME];
extern char sig_shared_region_path_jobs[GENERIC_NAME];
static int fd_conf,fd_signatures;
extern uint exclusive;
extern uint fake_force_shared_node;

extern sem_t *lib_shared_lock_sem;

int  get_lib_shared_data_path(char *tmp, uint ID, char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/%u/.ear_lib_shared_data",tmp, ID);
	return EAR_SUCCESS;	
}
int  get_shared_signatures_path(char *tmp, uint ID, char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/%u/.ear_shared_signatures",tmp, ID);
	return EAR_SUCCESS;	
}




/***** SPECIFIC FUNCTIONS *******************/



lib_shared_data_t * create_lib_shared_data_area(char * path)  
{      	
	lib_shared_data_t sh_data,*my_area;
	my_area=(lib_shared_data_t *)create_shared_area(path,(char *)&sh_data,sizeof(lib_shared_data_t),&fd_conf,1);
	return my_area;
}

lib_shared_data_t * attach_lib_shared_data_area(char * path, int *fd)
{
    return (lib_shared_data_t *)attach_shared_area(path,sizeof(lib_shared_data_t),O_RDWR,fd,NULL);
}                                
void dettach_lib_shared_data_area(int fd)
{
	dettach_shared_area(fd);
}

void lib_shared_data_area_dispose(char * path)
{
	dispose_shared_area(path,fd_conf);
}


void print_lib_shared_data(lib_shared_data_t *sh_data)
{
	fprintf(stderr,"sh_data num_processes %d signatures %d cas_counters %llu\n",sh_data->num_processes,sh_data->num_signatures,sh_data->cas_counters);

}

/// SIGNATURES

// Creates a shared memory region between eard and ear_lib. returns NULL if error.
shsignature_t * create_shared_signatures_area(char * path, int np)  
{      	
	shsignature_t *my_sig,*p2;

	my_sig=(shsignature_t*)malloc(sizeof(shsignature_t)*np);
	p2=create_shared_area(path,(char *)my_sig,sizeof(shsignature_t)*np,&fd_signatures,1);
	free(my_sig);
	return p2;
}

shsignature_t * attach_shared_signatures_area(char * path,int np, int *fd)
{
    return (shsignature_t *)attach_shared_area(path,sizeof(shsignature_t)*np,O_RDWR,fd,NULL);
}                                
void dettach_shared_signatures_area(int fd)
{
	dettach_shared_area(fd);
}
void shared_signatures_area_dispose(char * path)
{
	dispose_shared_area(path,fd_signatures);
}

/************** Marks the signature is computed *****************/
void signature_ready(shsignature_t *sig,int cur_state)
{
	sig->ready = 1;
	sig->app_state = cur_state;
}


int all_signatures_initialized(lib_shared_data_t *data,shsignature_t *sig)
{
    int total = 0;
    for (int i = 0; i < data->num_processes; i++) {
        total += (sig[i].pid > 0 );
    }

    return (total == data->num_processes);
}


void aggregate_all_the_cpumasks(lib_shared_data_t *data,shsignature_t *sig, cpu_set_t *m)
{
	CPU_ZERO(m);
	for (uint p = 0; p < data->num_processes; p++){
		aggregate_cpus_to_mask(m, &sig[p].cpu_mask);
	}
}


int compute_total_signatures_ready(lib_shared_data_t *data,shsignature_t *sig)
{
    int i, total = 0;

    if (ear_msync(sig, sizeof(shsignature_t)* data->num_processes,MS_SYNC) < 0)
        verbose_master(2, "Memory sync fails %s", strerror(errno));

    for (i=0; i<data->num_processes; i++) {
        total = total + sig[i].ready;
        if (!sig[i].ready) {
            debug(" Process %d not ready",i);
        }
    }

    data->num_signatures = total;

    return total;
}

int are_signatures_ready(lib_shared_data_t *data,shsignature_t *sig, uint *num_ready)
{
    int total = compute_total_signatures_ready(data, sig);
    if (num_ready)
    {
        *num_ready = total;
    }
    debug("There are %d signatures ready from %d", data->num_signatures, data->num_processes);
    return(total == data->num_processes);
}

int num_processes_exited(lib_shared_data_t *data,shsignature_t *sig)
{
  uint total = 0;
  for (uint i = 0; i < data->num_processes;i++) total += sig[i].exited;
  return total;
}


void print_sig_readiness(lib_shared_data_t *data,shsignature_t *sig)
{
    int i;
    for (i=0;i<data->num_processes;i++){
        debug("P[%d] ready %u ",i,sig[i].ready);
    }
}

void free_node_signatures(lib_shared_data_t *data,shsignature_t *sig)
{
    int i;
    fflush(stdout);
    for (i=0;i<data->num_processes;i++){
        sig[i].ready = 0;
    }
    if (ear_msync(sig, sizeof(shsignature_t)* data->num_processes,MS_SYNC|MS_INVALIDATE) < 0) verbose_master(2,"Memory sync fails %s",strerror(errno));
}

void clean_signatures(lib_shared_data_t *data,shsignature_t *sig)
{
    int i;
    fflush(stdout);
    for (i=0;i<data->num_processes;i++) sig[i].ready = 0;
    if (ear_msync(sig, sizeof(shsignature_t)* data->num_processes,MS_SYNC|MS_INVALIDATE) < 0) verbose_master(2,"Memory sync fails %s",strerror(errno));
}

void clean_mpi_info(lib_shared_data_t *data,shsignature_t *sig)
{
    int i;
    for (i=0;i<data->num_processes;i++){ 
        sig[i].mpi_info.mpi_time				=	0;
        sig[i].mpi_info.total_mpi_calls	=	0;
        sig[i].mpi_info.perc_mpi				=	0;
        sig[i].mpi_info.exec_time				=	0;
    }
}

void clean_my_mpi_info(mpi_information_t *info)
{
    info->mpi_time 				= 0;
    info->total_mpi_calls = 0;
    info->perc_mpi				= 0;
    info->exec_time				= 0;
}

int select_cp(lib_shared_data_t *data,shsignature_t *sig)
{
    int i,rank = sig[0].mpi_info.rank;
    double minp = sig[0].mpi_info.perc_mpi;
    for (i=1;i<data->num_processes;i++){
        if (sig[i].mpi_info.perc_mpi<minp){ 
            rank = sig[i].mpi_info.rank;
            minp = sig[i].mpi_info.perc_mpi;
        }
    }
    return rank;
}

double min_perc_mpi_in_node(lib_shared_data_t *data,shsignature_t *sig)
{
    int i;
    double minp =sig[0].mpi_info.perc_mpi;
    for (i=1;i<data->num_processes;i++){
        if (sig[i].mpi_info.perc_mpi<minp){
            minp = sig[i].mpi_info.perc_mpi;
        }
    }
    return minp;
}

void compute_avg_sh_signatures(int size,int max,int *ppn,shsignature_t *my_sh_sig,signature_t *sig)
{
    signature_t avgs,totals;
    int i,j;
    int nums = 0;
    signature_init(sig);
	signature_init(&avgs);
	signature_init(&totals);
  for (i=0;i<size;i++){
    for (j=0;j<ear_min(max,ppn[i]);j++){
			signature_from_ssig(&avgs,&my_sh_sig[i*max+j].sig);
			acum_sig_metrics(&totals,&avgs);
			nums++;
   	}
	} 
	compute_avg_sig(&avgs,&totals,nums);
	signature_copy(sig,&avgs);
	
}
int shsig_id(int node,int rank)
{
	int i;
	if (sh_sig_per_proces){
		for (i=node*masters_info.max_ppn;i<(node*masters_info.max_ppn+masters_info.ppn[node]);i++){
			if (masters_info.nodes_info[i].mpi_info.rank == rank) return i;
		}
  }else{
		return node;	
	}
	debug("Node %d rank %d not found in shared info",node,rank);
	return node;
}
int my_shsig_id()
{
	int max_ppn;
	  if (sh_sig_per_proces){
			max_ppn = masters_info.max_ppn;
			return masters_info.my_master_rank*max_ppn+my_node_id;
    }else{
			return masters_info.my_master_rank;
    }

}

/* This function must not be used and must be replaced by the one at mpi_support in policies folder */
int select_global_cp(int size,int max,int *ppn,shsignature_t *my_sh_sig,int *node_cp,int *rank_cp)
{
	int i,j;
	int rank = 0;
	double minp = 100.0,maxp = 0.0;
	unsigned long long int total_mpi = 0;
	unsigned long long total_mpi_time = 0, total_exec_time = 0;
	/* Node loop */
	for (i=0;i<size;i++){
		/* Inside node */
		for (j=0;j<ear_min(max,ppn[i]);j++){
			total_mpi 			+= my_sh_sig[i*max+j].mpi_info.total_mpi_calls;
			total_mpi_time	+= my_sh_sig[i*max+j].mpi_info.mpi_time;
			total_exec_time	+= my_sh_sig[i*max+j].mpi_info.exec_time;
			if (minp > my_sh_sig[i*max+j].mpi_info.perc_mpi){
				rank 			= my_sh_sig[i*max+j].mpi_info.rank;
				minp 			= my_sh_sig[i*max+j].mpi_info.perc_mpi;
				*node_cp 	= i;
			}
			if (maxp < my_sh_sig[i*max+j].mpi_info.perc_mpi) maxp = my_sh_sig[i*max+j].mpi_info.perc_mpi;
		}
	}
	*rank_cp = rank;
	debug("(MIN PERC MPI %.1lf, MAX PERC MPI %.1lf) (MPI_CALLS %llu MPI_TIME %.3fsec USER_TIME=%.3fsec)\n",minp*100.0,maxp*100.0,total_mpi,(float)total_mpi_time/1000000.0,(float)total_exec_time/1000000.0);
	return rank;
}

void print_local_mpi_info(mpi_information_t *info)
{
	fprintf(stderr,"total_mpi_calls %llu exec_time %llu mpi_time %llu rank %d perc_mpi %.3lf \n",info->total_mpi_calls,info->exec_time,info->mpi_time,info->rank,info->perc_mpi);
}

void mpi_info_to_str(mpi_information_t *info,char *msg,size_t max)
{
	snprintf(msg,max,"RANK[%d] total_mpi_calls %llu exec_time %llu mpi_time %llu perc_mpi %.3lf ",info->rank,info->total_mpi_calls,info->exec_time,info->mpi_time,info->perc_mpi*100.0);
}


void mpi_info_head_to_str_csv(char *msg,size_t max)
{
	snprintf(msg,max,"lrank;total_mpi_calls;exec_time;mpi_time;perc_mpi_time");
}


void mpi_info_to_str_csv(mpi_information_t *info, char *msg, size_t max)
{
  snprintf(msg, max, "%d;%llu;%llu;%llu;%.3lf", info->rank, info->total_mpi_calls,
           info->exec_time,info->mpi_time,info->perc_mpi*100.0);
}


void print_sh_signature(int localid,shsignature_t *sig)
{
	 	float t;
		float avgf,deff,newf;
		avgf=(float)sig->sig.avg_f/1000000.0;
		deff=(float)sig->sig.def_f/1000000.0;
		newf=(float)sig->new_freq/1000000.0;
    t = (float) sig->mpi_info.exec_time/1000000.0;

	  fprintf(stderr,"RANK[%d]= %d mpi_data={total_mpi_calls %llu mpi_time %llu exec_time %.3f PercTime %lf }\n",localid,
    sig->mpi_info.rank,sig->mpi_info.total_mpi_calls,sig->mpi_info.mpi_time,t,sig->mpi_info.perc_mpi);
    fprintf(stderr,"RANK[%d]= %d signature={cpi %.3lf tpi %.3lf time %.3lf Gflops %f dc_power %.3lf avgf %.1f deff %.1f} state %d new_freq %.1f\n",localid,sig->mpi_info.rank,sig->sig.CPI,sig->sig.TPI, sig->sig.time,sig->sig.Gflops,sig->sig.DC_power,avgf,deff,sig->app_state,newf);
}

void print_shared_signatures(lib_shared_data_t *data,shsignature_t *sig)
{
	int i;

	for (i=0;i<data->num_processes;i++){
		print_sh_signature(i,&sig[i]);
	}
}
void print_ready_shared_signatures(int master_rank,lib_shared_data_t *data,shsignature_t *sig)
{
	uint num_ready;
	if (master_rank < 0) return;
	if (are_signatures_ready(data,sig, &num_ready)){
		print_shared_signatures(data,sig);
	}
}



void copy_my_sig_info(lib_shared_data_t *data,shsignature_t *sig,shsignature_t *rem_sig)
{
	memcpy(rem_sig,sig,sizeof(shsignature_t)*data->num_processes);
}

void shsignature_copy(shsignature_t *dst,shsignature_t *src)
{
	memcpy(dst,src,sizeof(shsignature_t));
}

void compute_per_node_avg_mpi_info(lib_shared_data_t *data,shsignature_t *sig,mpi_information_t *my_mpi_info)
{
	mpi_information_t avg_mpi;
	avg_mpi.total_mpi_calls = sig[0].mpi_info.total_mpi_calls;
	avg_mpi.exec_time = sig[0].mpi_info.exec_time;
	avg_mpi.mpi_time = sig[0].mpi_info.mpi_time;
	avg_mpi.rank = sig[0].mpi_info.rank;
	avg_mpi.perc_mpi = sig[0].mpi_info.perc_mpi;
  int i;
	//debug("compute_per_node_mpi_info, num_procs %d",data->num_processes);
  for (i=1;i<data->num_processes;i++){
		avg_mpi.total_mpi_calls += sig[i].mpi_info.total_mpi_calls;
		avg_mpi.exec_time += sig[i].mpi_info.exec_time;
		avg_mpi.mpi_time += sig[i].mpi_info.mpi_time;
		avg_mpi.perc_mpi += sig[i].mpi_info.perc_mpi;
  }
	my_mpi_info->rank = 0;
	my_mpi_info->total_mpi_calls = avg_mpi.total_mpi_calls/data->num_processes;
	my_mpi_info->exec_time = avg_mpi.exec_time/data->num_processes;
	my_mpi_info->mpi_time = avg_mpi.mpi_time/data->num_processes;
	/* Should we use the minimum? */
	my_mpi_info->perc_mpi = avg_mpi.perc_mpi/data->num_processes;
}

state_t compute_job_node_instructions(const shsignature_t *sig, int n_procs, ull *instructions)
{
    if (!sig || !instructions) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else if (n_procs == 0) {
        return_msg(EAR_WARNING, "Number of processes is zero.");
    }

	int i;
	*instructions = 0;
	for (i = 0; i < n_procs; i++) {
		*instructions += sig[i].sig.instructions;
	}

    return EAR_SUCCESS;
}

state_t compute_job_node_flops(const shsignature_t *sig, int n_procs, ull *flops)
{
    if (sig == NULL || flops == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else if (n_procs == 0) {
        return_msg(EAR_ERROR, "Number of processes is zero.");
    }

	int i, f;
	memset(flops, 0, FLOPS_EVENTS * sizeof(ull));

	for (i = 0; i < n_procs; i++) {
		for (f = 0; f < FLOPS_EVENTS; f++) {
            flops[f] += sig[i].sig.FLOPS[f];
        }
	}

    return EAR_SUCCESS;
}

state_t compute_job_node_cycles(const shsignature_t *sig, int n_procs, ull *t_cycles)
{
    if (sig == NULL || t_cycles == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else if (n_procs == 0) {
        return_msg(EAR_WARNING, "Number of processes is zero.");
    }

    int i;
    *t_cycles = 0;
    for (i = 0; i < n_procs; i++) {
        *t_cycles += sig[i].sig.cycles;
    }

    return EAR_SUCCESS;
}

state_t compute_job_node_L3_misses(const shsignature_t *sig, int n_procs, ull *t_L3)
{
    if (sig == NULL || t_L3 == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else if (n_procs == 0) {
        return_msg(EAR_WARNING, "Number of processes is zero.");
    }

    int i;
    *t_L3 = 0;
    for (i = 0; i < n_procs; i++) {
        *t_L3 += sig[i].sig.L3_misses;
    }
    
    return EAR_SUCCESS;
}

state_t compute_job_node_L1_misses(const shsignature_t *sig, int n_procs, ull *t_L1)
{
    if (sig == NULL || t_L1 == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else if (n_procs == 0) {
        return_msg(EAR_WARNING, "Number of processes is zero.");
    }

    int i;
    *t_L1 = 0;
    for (i = 0; i < n_procs; i++) {
        *t_L1 += sig[i].sig.L1_misses;
    }

    return EAR_SUCCESS;
}

state_t compute_job_node_L2_misses(const shsignature_t *sig, int n_procs, ull *t_L2)
{
    if (sig == NULL || t_L2 == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else if (n_procs == 0) {
        return_msg(EAR_WARNING, "Number of processes is zero.");
    }

    int i;
    *t_L2 = 0;
    for (i = 0; i < n_procs; i++) {
        *t_L2 += sig[i].sig.L2_misses;
    }

    return EAR_SUCCESS;
}

state_t compute_job_node_gflops(const shsignature_t *sig, int n_procs, double *t_gflops)
{
    if (sig == NULL || t_gflops == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else if (!n_procs) {
        return_msg(EAR_WARNING, "Number of processes is zero.");
    }

	*t_gflops = 0;
	for (int i = 0; i < n_procs; i++) {
        *t_gflops += sig[i].sig.Gflops;
    }

    return EAR_SUCCESS;
}

state_t compute_job_node_io_mbs(const shsignature_t *sig, int n_procs, double *io_mbs)
{
    if (sig == NULL ||  io_mbs == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    } else if (!n_procs) {
        return_msg(EAR_WARNING, "Number of processes is zero.");
    }

	*io_mbs = 0;
	for (int i = 0; i < n_procs; i++) {
        *io_mbs += (double)sig[i].sig.IO_MBS;
  }

  return EAR_SUCCESS;
}


void compute_job_cpus(lib_shared_data_t *data,shsignature_t *sig,uint *cpus)
{
    int i;
    *cpus = 0;
    for (i = 0; i < data->num_processes; i++) {
        uint lcpus = num_cpus_in_mask(&sig[i].cpu_mask);
        *cpus += lcpus;
    }
    verbose_master(2,"Job %u has %u cpus ",node_mgr_index,*cpus);
}

uint compute_max_vpi_idx(const shsignature_t *sig, int n_procs, double *max_vpi)
{
    double local_max = 0.0;

    uint max_ssig_idx = 0;

    *max_vpi = 0.0;

    for (int i = 0; i < n_procs; i++) {
        compute_ssig_vpi(&local_max, &sig[i].sig);

        if (local_max > *max_vpi) { 
            *max_vpi = local_max;
            max_ssig_idx = i;
        }
    }

    return max_ssig_idx;
}


int compute_per_node_most_loaded_process(lib_shared_data_t *data,shsignature_t *sig)
{
	int i,mostload=0;
	double mostloadp;
	mostloadp = sig[0].mpi_info.perc_mpi;
	for (i=1;i<data->num_processes;i++){
		if (sig[i].mpi_info.perc_mpi < mostloadp){
			mostloadp = sig[i].mpi_info.perc_mpi;
			mostload = i;
		}
	}
	return mostload;
}


void compute_per_node_avg_sig_info(lib_shared_data_t *data, shsignature_t *sig, shsignature_t *my_node_sig)
{
	int i;

	compute_per_node_avg_mpi_info(data, sig, &my_node_sig->mpi_info);

	ssig_t avg_node;

	ssig_set_node_metrics(&avg_node, &sig[0].sig); // We first set DC, GBs and TPI

	for (i = 0; i < data->num_processes; i++) {
		ssig_accumulate(&avg_node, &sig[i].sig);
	}

	ssig_compute_avg_node(&avg_node, data->num_processes);

	copy_mini_sig(&my_node_sig->sig, &avg_node);
}


void load_app_mgr_env()
{
	char *cshow_sig=ear_getenv(FLAG_SHOW_SIGNATURES);
	char *csh_sig_per_process=ear_getenv(FLAG_SHARE_INFO_PPROC);
	char *csh_sig_per_node=ear_getenv(FLAG_SHARE_INFO_PNODE);
	char *creport_node_sig=ear_getenv(FLAG_REPORT_NODE_SIGNATURES);
	char *creport_all_sig=ear_getenv(FLAG_REPORT_ALL_SIGNATURES);

	if (cshow_sig != NULL) show_signatures = atoi(cshow_sig);
	if (csh_sig_per_process != NULL) sh_sig_per_proces = atoi(csh_sig_per_process);
	if (csh_sig_per_node != NULL) sh_sig_per_node = atoi(csh_sig_per_node);
	if (creport_node_sig != NULL) report_node_sig = atoi(creport_node_sig);	
	if (creport_all_sig != NULL) report_all_sig = atoi(creport_all_sig);

	verbose_master(2,"Show_signatures %u share_sig_per_process %u share_sig_per_node %u report_node_sig %u report_all_sig %u",
	show_signatures,sh_sig_per_proces,sh_sig_per_node,report_node_sig,report_all_sig);
}

void* mpi_info_get_perc_mpi(void* mpi_inf){
    mpi_information_t mpi_info = *((mpi_information_t*) mpi_inf);
    double* res = malloc(sizeof(double));
    *res = mpi_info.perc_mpi;
    return res;
}



void compute_total_node_avx_and_avx_fops(lib_shared_data_t *data,shsignature_t *sig,ullong *avx)
{
	ull total = 0;
	for (uint i=0;i<data->num_processes;i++){	
		total += sig[i].sig.FLOPS[INDEX_256F];
		total += sig[i].sig.FLOPS[INDEX_256D];
		total += sig[i].sig.FLOPS[INDEX_512F];
		total += sig[i].sig.FLOPS[INDEX_512D];
	}
	*avx = total;
}

/* Node mgr support functions */
void  init_earl_node_mgr_info()
{
  for (uint j = 0;j < MAX_CPUS_SUPPORTED; j++){
    uint ID;
    job_id id,sid;

		char *tmp = get_ear_tmp();
    id = node_mgr_data[j].jid;
    sid = node_mgr_data[j].sid;
    if ((id != -1) && (sid != -1)){
      /* Double validation */
      if (node_mgr_info_lock() == EAR_SUCCESS){
        id = node_mgr_data[j].jid;
        sid = node_mgr_data[j].sid;
        node_mgr_job_info[j].creation_time = node_mgr_data[j].creation_time;
        node_mgr_info_unlock();
      }
      if ((id != -1) && (sid != -1) && (j != node_mgr_index) ){
        verbose_master(2,"Job %lu.%lu is sharing the node", id,sid);
        ID = create_ID(id, sid);
        if (get_lib_shared_data_path(tmp,ID,lib_shared_region_path_jobs) == EAR_SUCCESS){
          node_mgr_job_info[j].libsh = attach_lib_shared_data_area(lib_shared_region_path_jobs, &node_mgr_job_info[j].fd_lib);
        }else{
          node_mgr_job_info[j].creation_time = 0;
        }
				if (get_shared_signatures_path(tmp,ID,sig_shared_region_path_jobs) == EAR_SUCCESS){
					node_mgr_job_info[j].shsig = attach_shared_signatures_area(sig_shared_region_path_jobs, node_mgr_job_info[j].libsh->num_processes, &node_mgr_job_info[j].fd_sig);
				}
      }else if (j == node_mgr_index){
        node_mgr_job_info[j].libsh =  lib_shared_region;
				node_mgr_job_info[j].shsig =  sig_shared_region;
      }
    }
  }
}

void update_earl_node_mgr_info()
{
    char *tmp = get_ear_tmp();

    for (uint j = 0; j < MAX_CPUS_SUPPORTED; j++) {
        job_id jid = node_mgr_data[j].jid;
        job_id sid = node_mgr_data[j].sid;

        time_t ct = node_mgr_data[j].creation_time;

        if (ct != node_mgr_job_info[j].creation_time) {
            while (node_mgr_info_lock() != EAR_SUCCESS);

            jid = node_mgr_data[j].jid;
            sid = node_mgr_data[j].sid;
            ct  = node_mgr_data[j].creation_time;

            node_mgr_info_unlock();

            /* Double validation after getting the lock */
            if (ct != node_mgr_job_info[j].creation_time) {
                if (ct == 0) {
                    dettach_lib_shared_data_area(node_mgr_job_info[j].fd_lib);
                    dettach_shared_signatures_area(node_mgr_job_info[j].fd_sig);
                    node_mgr_job_info[j].creation_time = 0;
                } else {

                    uint ID = create_ID(jid, sid);

                    get_lib_shared_data_path(tmp, ID, lib_shared_region_path_jobs);
                    node_mgr_job_info[j].libsh = attach_lib_shared_data_area(lib_shared_region_path_jobs,
                            &node_mgr_job_info[j].fd_lib);

                    get_shared_signatures_path(tmp, ID, sig_shared_region_path_jobs);
                    node_mgr_job_info[j].shsig = attach_shared_signatures_area(sig_shared_region_path_jobs,
                            node_mgr_job_info[j].libsh->num_processes,
                            &node_mgr_job_info[j].fd_sig);

                    node_mgr_job_info[j].creation_time = ct;
                }
            }
        }
    }

    uint current_jobs_in_node = 0;
    nodemgr_get_num_jobs_attached(&current_jobs_in_node);
    exclusive = exclusive && (current_jobs_in_node == 1);
    if (fake_force_shared_node) exclusive = 0;
}

void verbose_jobs_in_node(int vl, ear_njob_t *nmgr_eard, node_mgr_sh_data_t *nmgr_earl)
{
    if (VERB_GET_LV() >= vl) {
        if ((nmgr_eard == NULL) || (nmgr_earl == NULL)) return;
        verbose_master(vl, "%s--- Jobs in node list ---", COL_BLU);
        char sig_buff[1024];
        for (uint i = 0; i < MAX_CPUS_SUPPORTED; i++) {
            if (nmgr_eard[i].creation_time && nmgr_earl[i].creation_time
                    && (nmgr_earl[i].libsh != NULL) && (nmgr_earl[i].shsig != NULL)) {

				signature_to_str(&nmgr_earl[i].libsh->job_signature, sig_buff, sizeof(sig_buff));
                verbose_master(vl,"Job[%u] %lu.%lu %s\nNode [%.2lf GB/s %.2lf W]",
                        i, nmgr_eard[i].jid, nmgr_eard[i].sid, sig_buff,
                        nmgr_earl[i].libsh->node_signature.GBS,
                        nmgr_earl[i].libsh->node_signature.DC_power);

                /*
				signature_to_str(&nmgr_earl[i].libsh->node_signature, sig_buff, sizeof(sig_buff));
                verbose_master(vl,"JOB[%u]=%lu.%lu \nNode: %s",i , nmgr_eard[i].jid, nmgr_eard[i].sid, sig_buff);
                */

            }
        }
        verbose_master(vl,"%s",COL_CLR);
    }
}

void accum_estimations(lib_shared_data_t *data, shsignature_t *sig)
{
    uint p;
    double time_s;
   debug("accum_estimations");
    for (p = 0; p < data->num_processes; p++) {
        time_s = sig_shared_region[p].period;

        sig_shared_region[p].sig.accum_energy      += sig_shared_region[p].sig.DC_power * time_s;
        sig_shared_region[p].sig.accum_mem_access  += sig_shared_region[p].sig.GBS * 1024 * 1024 * 1024 * time_s;
        sig_shared_region[p].sig.accum_avg_f       += sig_shared_region[p].sig.avg_f * time_s;
        sig_shared_region[p].sig.valid_time        += time_s;
        sig_shared_region[p].sig.accum_dram_energy += sig_shared_region[p].sig.DRAM_power * time_s;
        sig_shared_region[p].sig.accum_pack_energy += sig_shared_region[p].sig.PCK_power * time_s;

        debug("Accumulated energy for process %d %lu (%lf x %lf) mem %llu avg %lu (%lu x %lf) dram %lu (%lf) pack %lu (%lf) ",
                p, sig_shared_region[p].sig.accum_energy, sig_shared_region[p].sig.DC_power,
                time_s, sig_shared_region[p].sig.accum_mem_access,
                sig_shared_region[p].sig.accum_avg_f, sig_shared_region[p].sig.avg_f,
                time_s, sig_shared_region[p].sig.accum_dram_energy,
                sig_shared_region[p].sig.DRAM_power, sig_shared_region[p].sig.accum_pack_energy,
                sig_shared_region[p].sig.PCK_power);
    }
}

void estimate_power_and_gbs(lib_shared_data_t *data, shsignature_t *sig, node_mgr_sh_data_t *nmgr)
{
    cpu_power_model_project(data, sig, nmgr);
#if SHOW_DEBUGS
    for (uint p = 0; p < data->num_processes; p++) {
        debug("LP[%d] Power %.2lf GB/s %.2lf TPI %.2lf", p, 
                sig_shared_region[p].sig.DC_power, sig_shared_region[p].sig.GBS, sig_shared_region[p].sig.TPI);
    }
#endif
    return;
}


state_t update_job_affinity_mask(lib_shared_data_t *data, shsignature_t *sig)
{
    if (data && sig)
    {
        verbose(3, "Updating the affinity mask...");

        // Backup current data to be restored in case of error.

        cpu_set_t node_mask_backup = data->node_mask;
        uint node_mask_num_cpus_backup = data->num_cpus;

        verbose_master(1, "Current job mask: %u CPUs ", node_mask_num_cpus_backup);
        verbose_affinity_mask(3, (const cpu_set_t *) &node_mask_backup, MAX_CPUS_SUPPORTED);

        cpu_set_t *cpu_masks_backup = malloc(sizeof(cpu_set_t) * data->num_processes);
        uint *num_cpus_backup = malloc(sizeof(uint) * data->num_processes);

        CPU_ZERO(&data->node_mask); // Clean because we'll update it using CPU_OR

        for (int i = 0; i < data->num_processes; i++)
        {
            cpu_masks_backup[i] = sig[i].cpu_mask; // Backup
            num_cpus_backup[i] = sig[i].num_cpus;

            // Update local cpu mask

            int sem_ret_val = sem_wait(lib_shared_lock_sem);
            if (sem_ret_val < 0)
            {
                verbose(1, "%sWARNING%s Locking semaphor for cpu mask update failed: %d",
                        COL_YLW, COL_CLR, errno);
            }

            int ret_val = sched_getaffinity(sig[i].pid, sizeof(cpu_set_t), &sig[i].cpu_mask);

            if (!sem_ret_val)
            {
                if (sem_post(lib_shared_lock_sem) < 0)
                {
                    verbose(1, "%sWARNING%s Unlocking semaphor for cpu mask update failed: %d",
                            COL_YLW, COL_CLR, errno);
                }
            }

            if (!ret_val)
            {
                // Update the number of CPUs of the process' mask
                sig[i].num_cpus = CPU_COUNT(&sig[i].cpu_mask);

                // Update node(job)_mask
                CPU_OR(&data->node_mask, &data->node_mask, &sig[i].cpu_mask);
            }
            else
            {
                // Restore masks before returning
                data->node_mask = node_mask_backup;
                data->num_cpus = node_mask_num_cpus_backup;

                // We had backed up and updated cpu masks from 0 to i
                for (int j = 0; j <= i; j++)
                {
                    sig[j].cpu_mask = cpu_masks_backup[j];
                    sig[j].num_cpus = num_cpus_backup[j];
                }
                free(cpu_masks_backup);
                free(num_cpus_backup);

                // Return and error message
                char ret_msg[64];
                ret_val = snprintf(ret_msg, sizeof(ret_msg),
                                   "Getting the affinity mask of local rank (%d)", errno);

                if (ret_val >= sizeof(ret_msg))
                {
                    return_msg(EAR_ERROR, "An error occurred and the message was truncated.")
                }

                return_msg(EAR_ERROR, ret_msg);
            }
        }

        // Affinity (local and job) updated with no error

        free(cpu_masks_backup);
        free(num_cpus_backup);

        // Update all remaining mask related fields
        data->num_cpus = CPU_COUNT(&data->node_mask);

        verbose_master(1, "New job mask: %u CPUs ", data->num_cpus);
        verbose_affinity_mask(3, (const cpu_set_t *) &data->node_mask, MAX_CPUS_SUPPORTED);

        if (data->num_cpus != node_mask_num_cpus_backup)
        {
            verbose(1, "%sWARNING%s The number of job's CPUs changed from %u to %u.", COL_YLW, COL_CLR,
                    node_mask_num_cpus_backup, data->num_cpus);
        }

        return EAR_SUCCESS;

    } else
    {
        return_msg(EAR_ERROR, Generr.input_null);
    }
}
