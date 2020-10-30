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
#include <common/states.h> 
#include <common/config.h>
#include <common/output/verbose.h>
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
	int flag;
	PMPI_Test(req, &flag, MPI_STATUS_IGNORE);
	if (flag) return EAR_SUCCESS;
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
/************************* Checks if node signatures are ready or not yet **************************/
/************************* If they are ready, they are sent to other masters **********************/
/**************************************************************************************************/
void check_node_signatures(masters_info_t *mi,lib_shared_data_t *data,shsignature_t *sig,int show_sig)
{
    #if SHARE_INFO_PER_PROCESS
    int max_ppn=mi->max_ppn;
    #endif
    #if SHARE_INFO_PER_NODE
    int max_ppn=1;
    #endif

    /* If the master signature is ready we check the others */
  if ((mi->my_master_rank>=0) && sig[0].ready){
    if (are_signatures_ready(data,sig)){
			/* here we print my node signatures, not all of them */
      if (show_sig) print_shared_signatures(data,sig);
      clean_signatures(data,sig);
			#if DISTRIBUTE_SH_SIG
      if (!mi->node_info_pending){
        #if SHARE_INFO_PER_PROCESS
        copy_my_sig_info(data,sig,mi->my_mpi_info);
        #endif
        #if SHARE_INFO_PER_NODE
        compute_per_node_sig(data,sig,mi->my_mpi_info);
        #endif
        if (ishare_global_info(mi->masters_comm,(char *)mi->my_mpi_info,
          sizeof(shsignature_t)*max_ppn,
          (char *)mi->nodes_info,sizeof(shsignature_t)*max_ppn,&mi->req)!=EAR_SUCCESS){
          error("Sending node sh_signature to other masters master_rank %d",mi->my_master_rank);
        }else{
          verbose(1,"sh_signatures for master_rank %d sent to other nodes",mi->my_master_rank);
          mi->node_info_pending=1;
        }
      }
			#endif
    }
  }
}

void print_mpi_info(masters_info_t *mi)
{
	int nodei,node_ppn;
	for (nodei=0;nodei<mi->my_master_size;nodei++){
		fprintf(stderr,"Printing info for node %d",nodei);
		for (node_ppn=0;node_ppn<mi->ppn[nodei];node_ppn++){
			print_local_mpi_info(&mi->nodes_info[nodei*mi->max_ppn+node_ppn].mpi_info);
		}
	}
}

void print_global_signatures(masters_info_t *mi)
{
  int nodei,node_ppn;
  for (nodei=0;nodei<mi->my_master_size;nodei++){
    fprintf(stderr,"Printing info for node %d",nodei);
    for (node_ppn=0;node_ppn<mi->ppn[nodei];node_ppn++){
      print_sh_signature(&mi->nodes_info[nodei*mi->max_ppn+node_ppn]);
    }
  }
}

/**************************************************************************************************/
/******************* This function checks if info from other nodes is ready ***********************/
/**************************************************************************************************/

void check_mpi_info(masters_info_t *mi,int *node_cp,int *rank_cp,int show_sig)
{
  *node_cp=-1;
  *rank_cp=-1;
  if ((mi->my_master_rank>=0) && mi->node_info_pending && (is_info_ready(&mi->req)==EAR_SUCCESS)){
    #if SHARE_INFO_PER_PROCESS
    int max_ppn=mi->max_ppn;
    #endif
    #if SHARE_INFO_PER_NODE
    int max_ppn=1;
    #endif

		if (show_sig && mi->my_master_rank==0) print_global_signatures(mi);

    verbose(1,"Info received in master %d",mi->my_master_rank);
    select_global_cp(mi->my_master_size,max_ppn,mi->ppn,mi->nodes_info,node_cp,rank_cp);
    verbose(1,"Global CP is node %d rank %d",*node_cp,*rank_cp);
    mi->node_info_pending=0;
  }
}

int load_unbalance(masters_info_t *mi)
{
	return 0;
}
