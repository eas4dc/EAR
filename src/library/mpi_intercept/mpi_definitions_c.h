/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
*/

#ifndef LIBRARY_MPI_DEFINITIONS_C_H
#define LIBRARY_MPI_DEFINITIONS_C_H

#include <library/mpi_intercept/mpi.h>

#define MPI_C_SYMS_N 93

typedef struct mpi_c_syms_s
{
	int (*MPI_Allgather) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
	int (*MPI_Allgatherv) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, MPI_Comm comm);
	int (*MPI_Allreduce) (MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
	int (*MPI_Alltoall) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
	int (*MPI_Alltoallv) (MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *sdispls, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
	int (*MPI_Barrier) (MPI_Comm comm);
	int (*MPI_Bcast) (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
	int (*MPI_Bsend) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
	int (*MPI_Bsend_init) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Cancel) (MPI_Request *request);
	int (*MPI_Cart_create) (MPI_Comm comm_old, int ndims, MPI3_CONST int dims[], MPI3_CONST int periods[], int reorder, MPI_Comm *comm_cart);
	int (*MPI_Cart_sub) (MPI_Comm comm, MPI3_CONST int remain_dims[], MPI_Comm *newcomm);
	int (*MPI_Comm_create) (MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
	int (*MPI_Comm_dup) (MPI_Comm comm, MPI_Comm *newcomm);
	int (*MPI_Comm_free) (MPI_Comm *comm);
	int (*MPI_Comm_rank) (MPI_Comm comm, int *rank);
	int (*MPI_Comm_size) (MPI_Comm comm, int *size);
	int (*MPI_Comm_spawn) (MPI3_CONST char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
	int (*MPI_Comm_spawn_multiple) (int count, char *array_of_commands[], char **array_of_argv[], MPI3_CONST int array_of_maxprocs[], MPI3_CONST MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
	int (*MPI_Comm_split) (MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
	int (*MPI_File_close) (MPI_File *fh);
	int (*MPI_File_read) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
	int (*MPI_File_read_all) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
	int (*MPI_File_read_at) (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
	int (*MPI_File_read_at_all) (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
	int (*MPI_File_write) (MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
	int (*MPI_File_write_all) (MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
	int (*MPI_File_write_at) (MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
	int (*MPI_File_write_at_all) (MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
	int (*MPI_Finalize) (void);
	int (*MPI_Gather) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
	int (*MPI_Gatherv) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
	int (*MPI_Get) (void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
	int (*MPI_Ibsend) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Init) (int *argc, char ***argv);
	int (*MPI_Init_thread) (int *argc, char ***argv, int required, int *provided);
	int (*MPI_Intercomm_create) (MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm);
	int (*MPI_Intercomm_merge) (MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
	int (*MPI_Iprobe) (int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
	int (*MPI_Irecv) (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Irsend) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Isend) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Issend) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Probe) (int source, int tag, MPI_Comm comm, MPI_Status *status);
	int (*MPI_Put) (MPI3_CONST void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
	int (*MPI_Recv) (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
	int (*MPI_Recv_init) (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Reduce) (MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
	int (*MPI_Reduce_scatter) (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
	int (*MPI_Request_free) (MPI_Request *request);
	int (*MPI_Request_get_status) (MPI_Request request, int *flag, MPI_Status *status);
	int (*MPI_Rsend) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
	int (*MPI_Rsend_init) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Scan) (MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
	int (*MPI_Scatter) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
	int (*MPI_Scatterv) (MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
	int (*MPI_Send) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
	int (*MPI_Send_init) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Sendrecv) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
	int (*MPI_Sendrecv_replace) (void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
	int (*MPI_Ssend) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
	int (*MPI_Ssend_init) (MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Start) (MPI_Request *request);
	int (*MPI_Startall) (int count, MPI_Request array_of_requests[]);
	int (*MPI_Test) (MPI_Request *request, int *flag, MPI_Status *status);
	int (*MPI_Testall) (int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[]);
	int (*MPI_Testany) (int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status);
	int (*MPI_Testsome) (int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);
	int (*MPI_Wait) (MPI_Request *request, MPI_Status *status);
	int (*MPI_Waitall) (int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses);
	int (*MPI_Waitany) (int count, MPI_Request *requests, int *index, MPI_Status *status);
	int (*MPI_Waitsome) (int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses);
	int (*MPI_Win_complete) (MPI_Win win);
	int (*MPI_Win_create) (void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
	int (*MPI_Win_fence) (int assert, MPI_Win win);
	int (*MPI_Win_free) (MPI_Win *win);
	int (*MPI_Win_post) (MPI_Group group, int assert, MPI_Win win);
	int (*MPI_Win_start) (MPI_Group group, int assert, MPI_Win win);
	int (*MPI_Win_wait) (MPI_Win win);
#if MPI_VERSION >= 3
	int (*MPI_Iallgather) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Iallgatherv) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Iallreduce) (MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Ialltoall) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Ialltoallv) (MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Ibarrier) (MPI_Comm comm, MPI_Request *request);
	int (*MPI_Ibcast) (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Igather) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Igatherv) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Ireduce) (MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Ireduce_scatter) (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Iscan) (MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Iscatter) (MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
	int (*MPI_Iscatterv) (MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
#endif
} mpi_c_syms_t;

#if 0
// These definitions are not used today, but could be used someday
int MPI_Allgather_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Allgatherv_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Allreduce_empty(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Alltoall_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Alltoallv_empty(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *sdispls, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Barrier_empty(MPI_Comm comm);
int MPI_Bcast_empty(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int MPI_Bsend_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Bsend_init_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Cancel_empty(MPI_Request *request);
int MPI_Cart_create_empty(MPI_Comm comm_old, int ndims, MPI3_CONST int dims[], MPI3_CONST int periods[], int reorder, MPI_Comm *comm_cart);
int MPI_Cart_sub_empty(MPI_Comm comm, MPI3_CONST int remain_dims[], MPI_Comm *newcomm);
int MPI_Comm_create_empty(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
int MPI_Comm_dup_empty(MPI_Comm comm, MPI_Comm *newcomm);
int MPI_Comm_free_empty(MPI_Comm *comm);
int MPI_Comm_rank_empty(MPI_Comm comm, int *rank);
int MPI_Comm_size_empty(MPI_Comm comm, int *size);
int MPI_Comm_spawn_empty(MPI3_CONST char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int MPI_Comm_spawn_multiple_empty(int count, char *array_of_commands[], char **array_of_argv[], MPI3_CONST int array_of_maxprocs[], MPI3_CONST MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int MPI_Comm_split_empty(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
int MPI_File_close_empty(MPI_File *fh);
int MPI_File_read_empty(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_all_empty(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_at_empty(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_at_all_empty(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_empty(MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_all_empty(MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_at_empty(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_at_all_empty(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_Finalize_empty(void);
int MPI_Gather_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Gatherv_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Get_empty(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int MPI_Ibsend_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Init_empty(int *argc, char ***argv);
int MPI_Init_thread_empty(int *argc, char ***argv, int required, int *provided);
int MPI_Intercomm_create_empty(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm);
int MPI_Intercomm_merge_empty(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
int MPI_Iprobe_empty(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
int MPI_Irecv_empty(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Irsend_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Isend_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Issend_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Probe_empty(int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Put_empty(MPI3_CONST void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int MPI_Recv_empty(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Recv_init_empty(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Reduce_empty(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int MPI_Reduce_scatter_empty(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Request_free_empty(MPI_Request *request);
int MPI_Request_get_status_empty(MPI_Request request, int *flag, MPI_Status *status);
int MPI_Rsend_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Rsend_init_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Scan_empty(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Scatter_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Scatterv_empty(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Send_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Send_init_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Sendrecv_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int MPI_Sendrecv_replace_empty(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int MPI_Ssend_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Ssend_init_empty(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Start_empty(MPI_Request *request);
int MPI_Startall_empty(int count, MPI_Request array_of_requests[]);
int MPI_Test_empty(MPI_Request *request, int *flag, MPI_Status *status);
int MPI_Testall_empty(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[]);
int MPI_Testany_empty(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status);
int MPI_Testsome_empty(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);
int MPI_Wait_empty(MPI_Request *request, MPI_Status *status);
int MPI_Waitall_empty(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses);
int MPI_Waitany_empty(int count, MPI_Request *requests, int *index, MPI_Status *status);
int MPI_Waitsome_empty(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses);
int MPI_Win_complete_empty(MPI_Win win);
int MPI_Win_create_empty(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
int MPI_Win_fence_empty(int assert, MPI_Win win);
int MPI_Win_free_empty(MPI_Win *win);
int MPI_Win_post_empty(MPI_Group group, int assert, MPI_Win win);
int MPI_Win_start_empty(MPI_Group group, int assert, MPI_Win win);
int MPI_Win_wait_empty(MPI_Win win);
#if MPI_VERSION >= 3
int MPI_Iallgather_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Iallgatherv_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Iallreduce_empty(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Ialltoall_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ialltoallv_empty(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ibarrier_empty(MPI_Comm comm, MPI_Request *request);
int MPI_Ibcast_empty(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Igather_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Igatherv_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Ireduce_empty(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Ireduce_scatter_empty(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Iscan_empty(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Iscatter_empty(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Iscatterv_empty(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
#endif
#endif

#endif //LIBRARY_MPI_DEFINITIONS_C_H
