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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
//#define SHOW_DEBUGS 1
#include <common/states.h> 
#include <common/config.h>
#include <common/output/verbose.h>
#include <library/common/verbose_lib.h>
#include <library/common/externs.h>
#include <library/common/global_comm.h>

/**************************************************************************************************/
/******************************* ASYNC comunication between masters *******************************/
/**************************************************************************************************/
state_t ishare_global_info(MPI_Comm comm,char * data_send, int size_send,char * data_recv, int size_recv,MPI_Request *req)
{
	if ((data_send == NULL) || (data_recv==NULL))	return EAR_ERROR;
	if ((size_send==0) || (size_recv==0)) return EAR_ERROR;
	PMPI_Iallgather(data_send, size_send, MPI_SIGNED_CHAR, 	
									data_recv, size_recv, MPI_SIGNED_CHAR, comm,req);
	return EAR_SUCCESS;
}

/**************************************************************************************************/
/******************************* SYNC comunication between masters *******************************/
/**************************************************************************************************/
state_t share_global_info(MPI_Comm comm,char * data_send, int size_send,char * data_recv, int size_recv)
{
	PMPI_Allgather(data_send, size_send, MPI_SIGNED_CHAR, 	
									data_recv, size_recv, MPI_SIGNED_CHAR, comm);
	return EAR_SUCCESS;
}

/**************************************************************************************************/
/******************************* Is masters info ready              *******************************/
/**************************************************************************************************/

state_t is_info_ready(MPI_Request *req)
{
	int flag = 0;
	int ret;
	ret = PMPI_Test(req, &flag, MPI_STATUS_IGNORE);
	if ((ret == MPI_SUCCESS) && flag) return EAR_SUCCESS;
	else return EAR_NOT_READY;
}
/**************************************************************************************************/
/******************************* WAIT for master data  *******************************/
/**************************************************************************************************/

state_t wait_for_data(MPI_Request *req)
{
	
	PMPI_Wait(req, MPI_STATUS_IGNORE);
	return EAR_SUCCESS;
}


/**************************************************************************************************/
/************************* Checks if node signatures are readyor not yet **************************/
/************************* If they are ready, they are sent to other masters **********************/
/**************************************************************************************************/
state_t check_node_signatures(masters_info_t *mi,lib_shared_data_t *data,shsignature_t *sig)
{
		state_t ret=EAR_NOT_READY;


    /* If the master signature is ready we check the others */
  if ((mi->my_master_rank >= 0) && sig[0].ready){
    if (are_signatures_ready(data,sig)){
			return EAR_SUCCESS;
		}
	}
	return ret;
}

state_t send_node_signatures(masters_info_t *mi,lib_shared_data_t *data,shsignature_t *sig,shsignature_t *all_sig,int show_sig)
{
		state_t ret;
		int max_ppn;
  	if (sh_sig_per_node) max_ppn = 1;
  	else                 max_ppn=mi->max_ppn;
  	if (!mi->node_info_pending){
  		clean_signatures(data,all_sig);
      if (sh_sig_per_proces){
          copy_my_sig_info(data,all_sig,mi->my_mpi_info);
      }else{
					shsignature_copy(mi->my_mpi_info,sig);
      }
        //debug("Sending %d elements per node, Total elements %d",max_ppn,max_ppn*mi->my_master_size);
      if (ishare_global_info(mi->masters_comm,(char *)mi->my_mpi_info,
      sizeof(shsignature_t)*max_ppn,
        (char *)mi->nodes_info,sizeof(shsignature_t)*max_ppn,&mi->req)!=EAR_SUCCESS){
        	error("Sending node sh_signature to other masters master_rank %d",mi->my_master_rank);
      }else{
        //debug("sh_signatures for master_rank %d sent to other nodes",mi->my_master_rank);
        	mi->node_info_pending=1;
					if (show_sig) print_shared_signatures(data,all_sig);
      }
     }
     ret = EAR_SUCCESS;
		return ret;
}



void print_mpi_info(masters_info_t *mi)
{
	int nodei,node_ppn;
	for (nodei=0;nodei<mi->my_master_size;nodei++){
		fprintf(stderr,"Printing info for node %d\n",nodei);
		for (node_ppn=0;node_ppn<mi->ppn[nodei];node_ppn++){
			print_local_mpi_info(&mi->nodes_info[nodei*mi->max_ppn+node_ppn].mpi_info);
		}
	}
}

void print_global_signatures(masters_info_t *mi)
{
  int nodei,node_ppn;
	int max_ppn;
  if (sh_sig_per_proces){
      max_ppn=mi->max_ppn;
  }else{
      max_ppn=1;
  }

  for (nodei=0;nodei<mi->my_master_size;nodei++){
    fprintf(stderr,"Printing info for node %d\n",nodei);
    for (node_ppn=0;node_ppn<ear_min(max_ppn,mi->ppn[nodei]);node_ppn++){
      print_sh_signature(node_ppn,&mi->nodes_info[nodei*ear_min(max_ppn,mi->max_ppn)+node_ppn]);
    }
  }
}

/**************************************************************************************************/
/******************* This function checks if info from other nodes is ready ***********************/
/**************************************************************************************************/

state_t check_mpi_info(masters_info_t *mi,int *node_cp,int *rank_cp,int show_sig)
{
	int max_ppn;
	state_t ret = EAR_NOT_READY;
  *node_cp=-1;
  *rank_cp=-1;
	//debug("check_mpi_info");
  if ((mi->my_master_rank>=0) && mi->node_info_pending && (is_info_ready(&mi->req)==EAR_SUCCESS)){
		if (sh_sig_per_proces){
    	max_ppn=mi->max_ppn;
		}else{
    	max_ppn=1;
		}

		if (show_sig && mi->my_master_rank==0) print_global_signatures(mi);

    //debug("Info received in master %d",mi->my_master_rank);
    select_global_cp(mi->my_master_size,max_ppn,mi->ppn,mi->nodes_info,node_cp,rank_cp);
    //debug("Global CP is node %d rank %d",*node_cp,*rank_cp);
    mi->node_info_pending=0;
		ret = EAR_SUCCESS;
  }
	return ret;
}

state_t compute_avg_app_signature(masters_info_t *mi,signature_t *gsig)
{
	state_t ret = EAR_SUCCESS;
	int max_ppn;

	if (mi->my_master_rank>=0){
    if (sh_sig_per_proces){
      max_ppn=mi->max_ppn;
    }else{
      max_ppn=1;
    }
		compute_avg_sh_signatures(mi->my_master_size,max_ppn,mi->ppn,mi->nodes_info,gsig);			
	}
	return ret;
}


int load_unbalance(masters_info_t *mi)
{
	return 0;
}

/*********************************************/
/******* mpi statistics : 1 x node ***********/
/*********************************************/
static uint        mpi_summary_pending = 0;
static MPI_Request mpi_summary_req;


state_t share_my_mpi_summary(masters_info_t *mi,mpi_summary_t *my_data)
{
#if NODE_LB
	if (ishare_global_info(mi->masters_comm,(char *)my_data, sizeof(mpi_summary_t),(char *)mi->nodes_mpi_summary,sizeof(mpi_summary_t),&mpi_summary_req)!=EAR_SUCCESS){
		verbose_master(2,"MR[%d]:Sending node sh_signature to other masters master_rank",mi->my_master_rank);
	}else{
		verbose_master(3,"Successfully sent mpi_stats");
		mpi_summary_pending = 1;
	}
#endif
	return EAR_SUCCESS;	
}

uint is_mpi_summary_pending()
{
	return mpi_summary_pending;
}

uint is_mpi_summary_ready()
{
#if NODE_LB
	if (is_info_ready(&mpi_summary_req) == EAR_SUCCESS){ 
		mpi_summary_pending = 0;
		return 1;
	}
	return 0;
#else
	return 1;
#endif
}

void verbose_node_mpi_summary(int vl, masters_info_t *mi)
{
	int i;
	mpi_summary_t *l;
	#if NODE_LB
	for (i=0;i<mi->my_master_size;i++){
		l = &mi->nodes_mpi_summary[i];
		verbose_master(vl,"MR[%d] maxp %.2f minp %.2f sd %.2f mean %.2f mag %.2f",i,l->max,l->min,l->sd,l->mean,l->mag);
	}
	#endif
}
