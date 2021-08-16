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

#ifndef _GLOBAL_COMM_H
#define _GLOBAL_COMM_H

#include <library/common/library_shared_data.h>

#if MPI

#include <mpi.h>

typedef struct masters_info{
	int my_master_rank;
	int my_master_size;
	int *ppn;
	int max_ppn;
	shsignature_t *my_mpi_info;
	shsignature_t *nodes_info;
	int node_info_pending;
	#if NODE_LB
	mpi_summary_t *nodes_mpi_summary;
	#endif
	MPI_Request req;
	MPI_Comm masters_comm;
	MPI_Comm new_world_comm;
}masters_info_t;

state_t ishare_global_info(MPI_Comm comm,char * data_send, int size_send,char * data_recv, int size_recv,MPI_Request *req);

state_t share_global_info(MPI_Comm comm,char * data_send, int size_send,char * data_recv, int size_recv);

state_t is_info_ready(MPI_Request *req);

state_t wait_for_data(MPI_Request *req);


state_t check_mpi_info(masters_info_t *mi,int *node_cp,int *rank_cp,int show_sig);
void print_mpi_info(masters_info_t *mi);
state_t check_node_signatures(masters_info_t *mi,lib_shared_data_t *data,shsignature_t *sig);
state_t send_node_signatures(masters_info_t *mi,lib_shared_data_t *data,shsignature_t *sig,shsignature_t *all_sig,int show_sig);
int load_unbalance(masters_info_t *mi);
void print_global_signatures(masters_info_t *mi);
state_t compute_avg_app_signature(masters_info_t *mi,signature_t *gsig);

/**** MPI summary sharing ****/
state_t share_my_mpi_summary(masters_info_t *mi,mpi_summary_t *my_data);
uint is_mpi_summary_pending();
uint is_mpi_summary_ready();
void verbose_node_mpi_summary(int vl, masters_info_t *mi);

		


#else

typedef struct masters_info{
  int my_master_rank;
  int my_master_size;
  int *ppn;
  int max_ppn;
  shsignature_t *my_mpi_info;
  shsignature_t *nodes_info;
  int node_info_pending;
  #if NODE_LB
  mpi_summary_t *nodes_mpi_summary;
  #endif
}masters_info_t;



#define check_mpi_info(a,b,c,d) (*b=0;*c=0)
#define print_mpi_info(a)
#define check_node_signatures(a,b,c,d)
#define load_unbalance(a) 0
#define print_global_signatures(a)
#define compute_avg_app_signature(a,b) EAR_SUCCESS

#define share_my_mpi_summary(a,b) EAR_SUCCESS
#define is_mpi_summary_pending() 0
#define is_mpi_summary_ready()	 0
#define verbose_node_mpi_summary(a,b)

#endif
#endif
