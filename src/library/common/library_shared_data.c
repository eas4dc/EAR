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
#include <common/config.h>
#include <common/states.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <library/common/externs.h>
#include <common/types/configuration/cluster_conf.h>
#include <library/common/library_shared_data.h>
#include <library/common/verbose_lib.h>
#include <library/common/global_comm.h>

static int fd_conf,fd_signatures;

int  get_lib_shared_data_path(char *tmp,char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/.ear_lib_shared_data",tmp);
	return EAR_SUCCESS;	
}
int  get_shared_signatures_path(char *tmp,char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/.ear_shared_signatures",tmp);
	return EAR_SUCCESS;	
}




/***** SPECIFIC FUNCTIONS *******************/



lib_shared_data_t * create_lib_shared_data_area(char * path)  
{      	
	lib_shared_data_t sh_data,*my_area;
	my_area=(lib_shared_data_t *)create_shared_area(path,(char *)&sh_data,sizeof(lib_shared_data_t),&fd_conf,1);
	return my_area;
}

lib_shared_data_t * attach_lib_shared_data_area(char * path)
{
    return (lib_shared_data_t *)attach_shared_area(path,sizeof(lib_shared_data_t),O_RDWR,&fd_conf,NULL);
}                                
void dettach_lib_shared_data_area()
{
	dettach_shared_area(fd_conf);
}

void lib_shared_data_area_dispose(char * path)
{
	dispose_shared_area(path,fd_conf);
}


void print_lib_shared_data(lib_shared_data_t *sh_data)
{
	fprintf(stderr,"sh_data num_processes %d signatures %d cas_counters %lf\n",sh_data->num_processes,sh_data->num_signatures,sh_data->cas_counters);

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

shsignature_t * attach_shared_signatures_area(char * path,int np)
{
    return (shsignature_t *)attach_shared_area(path,sizeof(shsignature_t)*np,O_RDWR,&fd_signatures,NULL);
}                                
void dettach_shared_signatures_area()
{
	dettach_shared_area(fd_signatures);
}
void shared_signatures_area_dispose(char * path)
{
	dispose_shared_area(path,fd_signatures);
}

/************** Marks the signature is computed *****************/
void signature_ready(shsignature_t *sig,int cur_state)
{
	debug("Process %d rank %d signature_ready",my_node_id,sig->mpi_info.rank);
	sig->ready = 1;
	sig->app_state = cur_state;
}

int compute_total_signatures_ready(lib_shared_data_t *data,shsignature_t *sig)
{
	int i,total=0;
  for (i=0;i<data->num_processes;i++){ 
		total = total+sig[i].ready;
		if (!sig[i].ready) debug(" Process %d not ready",i);
	}
	data->num_signatures = total;
	return total;
}
int are_signatures_ready(lib_shared_data_t *data,shsignature_t *sig)
{
	compute_total_signatures_ready(data,sig);
	debug("There are %d signatures ready from %d",data->num_signatures,data->num_processes);
	return(data->num_signatures == data->num_processes);
}

void print_sig_readiness(lib_shared_data_t *data,shsignature_t *sig)
{
	int i;
	for (i=0;i<data->num_processes;i++){
		debug("P[%d] ready %u ",i,sig[i].ready);
	}
}

void clean_signatures(lib_shared_data_t *data,shsignature_t *sig)
{
	int i;
  for (i=0;i<data->num_processes;i++) sig[i].ready = 0;
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
			from_minis_to_sig(&avgs,&my_sh_sig[i*max+j].sig);
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
	unsigned int total_mpi = 0;
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
	debug("(MIN PERC MPI %.1lf, MAX PERC MPI %.1lf) (MPI_CALLS %u MPI_TIME %.3fsec USER_TIME=%.3fsec)\n",minp*100.0,maxp*100.0,total_mpi,(float)total_mpi_time/1000000.0,(float)total_exec_time/1000000.0);
	return rank;
}

void print_local_mpi_info(mpi_information_t *info)
{
	fprintf(stderr,"total_mpi_calls %u exec_time %llu mpi_time %llu rank %d perc_mpi %.3lf \n",info->total_mpi_calls,info->exec_time,info->mpi_time,info->rank,info->perc_mpi);
}

void mpi_info_to_str(mpi_information_t *info,char *msg,size_t max)
{
	snprintf(msg,max,"RANK[%d] total_mpi_calls %u exec_time %llu mpi_time %llu perc_mpi %.3lf ",info->rank,info->total_mpi_calls,info->exec_time,info->mpi_time,info->perc_mpi*100.0);
}
void mpi_info_head_to_str_csv(char *msg,size_t max)
{
	snprintf(msg,max,"lrank;total_mpi_calls;exec_time;mpi_time;perc_mpi_time");
}
void mpi_info_to_str_csv(mpi_information_t *info,char *msg,size_t max)
{
  snprintf(msg,max,"%d;%u;%llu;%llu;%.3lf",info->rank,info->total_mpi_calls,info->exec_time,info->mpi_time,info->perc_mpi*100.0);
}


void print_sh_signature(int localid,shsignature_t *sig)
{
	 	float t;
		float avgf,deff,newf;
		avgf=(float)sig->sig.avg_f/1000000.0;
		deff=(float)sig->sig.def_f/1000000.0;
		newf=(float)sig->new_freq/1000000.0;
    t = (float) sig->mpi_info.exec_time/1000000.0;

	  fprintf(stderr,"RANK[%d]= %d mpi_data={total_mpi_calls %u mpi_time %llu exec_time %.3f PercTime %lf }\n",localid,
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
	if (master_rank < 0) return;
	if (are_signatures_ready(data,sig)){
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

void compute_total_node_instructions(lib_shared_data_t *data,shsignature_t *sig,ull *t_inst)
{
	int i;
	*t_inst = 0;
	for (i=0;i<data->num_processes;i++){
		*t_inst += sig[i].sig.instructions;
	}
}

void compute_total_node_flops(lib_shared_data_t *data,shsignature_t *sig,ull *flops)
{
	int i,f;
	memset(flops,0,FLOPS_EVENTS*sizeof(ull));
	for (i=0;i<data->num_processes;i++){
		for (f=0;f<FLOPS_EVENTS;f++) flops[f] += sig[i].sig.FLOPS[f];
	}
}

void compute_total_node_cycles(lib_shared_data_t *data,shsignature_t *sig,ull *t_cycles)
{
  int i;
  *t_cycles = 0;
  for (i=0;i<data->num_processes;i++){
    *t_cycles += sig[i].sig.cycles;
  }
}

void compute_total_node_CPI(lib_shared_data_t *data,shsignature_t *sig,double *CPI)
{
  int i;
  *CPI = 0;
  for (i=0;i<data->num_processes;i++){
    *CPI += (double)sig[i].sig.CPI;
  }
	*CPI = *CPI/data->num_processes;
}




uint compute_max_vpi(lib_shared_data_t *data,shsignature_t *sig,double *max_vpi)
{
	int i;
	double local_max = 0.0;
	uint max_ssig = 0;
	*max_vpi = 0.0;
	for (i=0;i<data->num_processes;i++){
		compute_ssig_vpi(&local_max,&sig[i].sig);
		if (local_max > *max_vpi ){ 
			*max_vpi = local_max;
			max_ssig = i;
		}
	}
	return max_ssig;
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


void compute_per_node_avg_sig_info(lib_shared_data_t *data,shsignature_t *sig,shsignature_t *my_node_sig)
{
	int i;
	//debug("compute_per_node_sig_info");
	compute_per_node_avg_mpi_info(data,sig,&my_node_sig->mpi_info);
	ssig_t avg_node;
	//print_shared_signatures(data,sig);
	set_global_metrics(&avg_node,&sig[0].sig);
	for (i=0;i<data->num_processes;i++){
		acum_ssig_metrics(&avg_node,&sig[i].sig);
	}
	compute_avg_node_sig(&avg_node,data->num_processes);
	copy_mini_sig(&my_node_sig->sig,&avg_node);
	#if 0
	fprintf(stderr,"AVG per node sig (using rank %d) \n",my_node_sig->mpi_info.rank);
	print_sh_signature(my_node_sig);	
	#endif
}


void load_app_mgr_env()
{
	char *cshow_sig=getenv(SCHED_EAR_SHOW_SIGNATURES);
	char *csh_sig_per_process=getenv(SHARE_INFO_PER_PROCESS);
	char *csh_sig_per_node=getenv(SHARE_INFO_PER_NODE);
	char *creport_node_sig=getenv(REPORT_NODE_SIGNATURES);
	char *creport_all_sig=getenv(REPORT_ALL_SIGNATURES);

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


double compute_node_flops(lib_shared_data_t *data,shsignature_t *sig)
{
	double total = 0;
	int i;
	for (i=0;i<data->num_processes;i++) total += sig[i].sig.Gflops;
	return total;
}

