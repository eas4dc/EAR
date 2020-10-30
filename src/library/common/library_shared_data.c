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
#include <common/output/verbose.h>
#include <common/types/configuration/cluster_conf.h>
#include <library/common/library_shared_data.h>
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
	sig->ready=1;
	sig->app_state=cur_state;
}

int compute_total_signatures_ready(lib_shared_data_t *data,shsignature_t *sig)
{
	int i,total=0;
  for (i=0;i<data->num_processes;i++) total=total+sig[i].ready;
	data->num_signatures=total;
	return total;
}
int are_signatures_ready(lib_shared_data_t *data,shsignature_t *sig)
{
	int i;
	compute_total_signatures_ready(data,sig);
	return(data->num_signatures==data->num_processes);
}

void clean_signatures(lib_shared_data_t *data,shsignature_t *sig)
{
	int i,total=0;
  for (i=0;i<data->num_processes;i++) sig[i].ready=0;
}

void clean_mpi_info(lib_shared_data_t *data,shsignature_t *sig)
{
  int i,total=0;
  for (i=0;i<data->num_processes;i++){ 
		sig[i].mpi_info.mpi_time=0;
		sig[i].mpi_info.total_mpi_calls=0;
		sig[i].mpi_info.perc_mpi=0;
		sig[i].mpi_info.exec_time=0;
	}
}

void clean_my_mpi_info(mpi_information_t *info)
{
	info->mpi_time=0;
	info->total_mpi_calls=0;
	info->perc_mpi=0;
	info->exec_time=0;
}

int select_cp(lib_shared_data_t *data,shsignature_t *sig)
{
  int i,rank=sig[0].mpi_info.rank;
	double minp=sig[0].mpi_info.perc_mpi;
  for (i=1;i<data->num_processes;i++){
    if (sig[i].mpi_info.perc_mpi<minp){ 
			rank=sig[i].mpi_info.rank;
			minp=sig[i].mpi_info.perc_mpi;
		}
  }
	return rank;
}

double min_perc_mpi_in_node(lib_shared_data_t *data,shsignature_t *sig)
{
  int i;
  double minp=sig[0].mpi_info.perc_mpi;
  for (i=1;i<data->num_processes;i++){
    if (sig[i].mpi_info.perc_mpi<minp){
      minp=sig[i].mpi_info.perc_mpi;
    }
  }
  return minp;
}


int select_global_cp(int size,int max,int *ppn,shsignature_t *my_sh_sig,int *node_cp,int *rank_cp)
{
	int i,j;
	int rank;
	double minp=100.0,maxp=0.0;
	unsigned int total_mpi=0;
	unsigned long long total_mpi_time=0, total_exec_time=0;
	/* Node loop */
	for (i=0;i<size;i++){
		/* Inside node */
		for (j=0;j<ppn[i];j++){
			total_mpi+=my_sh_sig[i*max+j].mpi_info.total_mpi_calls;
			total_mpi_time+=my_sh_sig[i*max+j].mpi_info.mpi_time;
			total_exec_time+=my_sh_sig[i*max+j].mpi_info.exec_time;
			if (minp>my_sh_sig[i*max+j].mpi_info.perc_mpi){
				rank=my_sh_sig[i*max+j].mpi_info.rank;
				minp=my_sh_sig[i*max+j].mpi_info.perc_mpi;
				*node_cp=i;
			}
			if (maxp<my_sh_sig[i*max+j].mpi_info.perc_mpi) maxp=my_sh_sig[i*max+j].mpi_info.perc_mpi;
		}
	}
	*rank_cp=rank;
	fprintf(stderr,"The (MIN MPI %lf, MAX MPI %lf) (MPI_CALLS %u MPI_TIME %llu USER_TIME=%llu)\n",minp*100.0,maxp*100.0,total_mpi,total_mpi_time/1000000000,total_exec_time/1000000000);
	return rank;
}

void print_local_mpi_info(mpi_information_t *info)
{
	fprintf(stderr,"total_mpi_calls %u exec_time %llu mpi_time %llu rank %d perc_mpi %.3lf \n",info->total_mpi_calls,info->exec_time,info->mpi_time,info->rank,info->perc_mpi);
}

void print_sh_signature(shsignature_t *sig)
{
	  fprintf(stderr," RANK %d mpi_data={total_mpi_calls %u mpi_time %llu exec_time %llu PercTime %lf }\n",
    sig->mpi_info.rank,sig->mpi_info.total_mpi_calls,sig->mpi_info.mpi_time,sig->mpi_info.exec_time,sig->mpi_info.perc_mpi);
    fprintf(stderr,"RANK %d signature={cpi %.3lf tpi %.3lf time %.3lf dc_power %.3lf} state %d new_freq %lu\n",sig->mpi_info.rank,sig->sig.CPI,sig->sig.TPI, sig->sig.time,sig->sig.DC_power,sig->app_state,sig->new_freq);
}

void print_shared_signatures(lib_shared_data_t *data,shsignature_t *sig)
{
	int i;

	for (i=0;i<data->num_processes;i++){
		print_sh_signature(&sig[i]);
	}
}



void copy_my_sig_info(lib_shared_data_t *data,shsignature_t *sig,shsignature_t *rem_sig)
{
	memcpy(rem_sig,sig,sizeof(shsignature_t)*data->num_processes);
}

void compute_per_node_mpi_info(lib_shared_data_t *data,shsignature_t *sig,mpi_information_t *my_mpi_info)
{
  int i,min_i=0;
	double min_perc=sig[0].mpi_info.perc_mpi;
  for (i=1;i<data->num_processes;i++){
		if (sig[i].mpi_info.perc_mpi<min_perc){
			min_i=i;
			min_perc=sig[i].mpi_info.perc_mpi;
		}
  }
  memcpy(&my_mpi_info,&sig[min_i].mpi_info,sizeof(mpi_information_t));
}


void compute_node_sig(signature_t *avg_sig,int n)
{
	double t,cpi,gflops;
	unsigned long avg_f,def_f;
	t=avg_sig->time/n;
	cpi=avg_sig->CPI/n;
	avg_f=avg_sig->avg_f/n;
	def_f=avg_sig->def_f/n;
	avg_sig->time=t;
	avg_sig->CPI=cpi;
	avg_sig->avg_f=avg_f;
	avg_sig->def_f=def_f;
}

void acum_signature_metrics(signature_t *avg_sig,signature_t *s)
{
	avg_sig->time+=s->time;
	avg_sig->CPI+=s->CPI;
	avg_sig->avg_f+=s->avg_f;
	avg_sig->def_f+=s->def_f;
	avg_sig->Gflops+=s->Gflops;
}
void set_global_metrics(signature_t *avg_sig,signature_t *s)
{
	avg_sig->GBS=s->GBS;
	avg_sig->TPI=s->TPI;
	avg_sig->DC_power=s->DC_power;
	avg_sig->time=0;
	avg_sig->CPI=0;
	avg_sig->avg_f=0;
	avg_sig->def_f=0;
	avg_sig->Gflops=0;
}

void compute_per_node_sig_info(lib_shared_data_t *data,shsignature_t *sig,shsignature_t *my_node_sig)
{
	int i;
	compute_per_node_mpi_info(data,sig,&my_node_sig->mpi_info);
	signature_t avg_node;
	set_global_metrics(&avg_node,&my_node_sig[0].sig);
	for (i=0;i<data->num_processes;i++){
		acum_signature_metrics(&avg_node,&my_node_sig[i].sig);
	}
	compute_node_sig(&avg_node,data->num_processes);
	signature_copy(&my_node_sig->sig,&avg_node);
	fprintf(stderr,"AVG per node sig (using rank %d) \n",my_node_sig->mpi_info.rank);
	print_sh_signature(my_node_sig);	
}




