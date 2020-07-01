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

#ifndef MPI_INTERFACEF_H
#define MPI_INTERFACEF_H

#include <library/mpi_intercept/mpi.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**@{ Each pair of functions are called before (enter function) and after (leave) a type MPI call. Which pair depends on the
*   type of the call, and the functions called are EAR_MPY_<call>_enter and leave. */
void EAR_MPI_Allgather_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Allgather_F_leave(void);

void EAR_MPI_Allgatherv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Allgatherv_F_leave(void);

void EAR_MPI_Allreduce_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Allreduce_F_leave(void);

void EAR_MPI_Alltoall_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Alltoall_F_leave(void);

void EAR_MPI_Alltoallv_F_enter(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Alltoallv_F_leave(void);

void EAR_MPI_Barrier_F_enter(MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Barrier_F_leave(void);

void EAR_MPI_Bcast_F_enter(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Bcast_F_leave(void);

void EAR_MPI_Bsend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Bsend_F_leave(void);

void EAR_MPI_Bsend_init_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Bsend_init_F_leave(void);

void EAR_MPI_Cancel_F_enter(MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Cancel_F_leave(void);

void EAR_MPI_Cart_create_F_enter(MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror);
void EAR_MPI_Cart_create_F_leave(void);

void EAR_MPI_Cart_sub_F_enter(MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror);
void EAR_MPI_Cart_sub_F_leave(void);

void EAR_MPI_Comm_create_F_enter(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror);
void EAR_MPI_Comm_create_F_leave(void);

void EAR_MPI_Comm_dup_F_enter(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror);
void EAR_MPI_Comm_dup_F_leave(void);

void EAR_MPI_Comm_free_F_enter(MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Comm_free_F_leave(void);

void EAR_MPI_Comm_rank_F_enter(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror);
void EAR_MPI_Comm_rank_F_leave(void);

void EAR_MPI_Comm_size_F_enter(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror);
void EAR_MPI_Comm_size_F_leave(void);

void EAR_MPI_Comm_spawn_F_enter(MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror);
void EAR_MPI_Comm_spawn_F_leave(void);

void EAR_MPI_Comm_spawn_multiple_F_enter(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror);
void EAR_MPI_Comm_spawn_multiple_F_leave(void);

void EAR_MPI_Comm_split_F_enter(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror);
void EAR_MPI_Comm_split_F_leave(void);

void EAR_MPI_File_close_F_enter(MPI_File *fh, MPI_Fint *ierror);
void EAR_MPI_File_close_F_leave(void);

void EAR_MPI_File_read_F_enter(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror);
void EAR_MPI_File_read_F_leave(void);

void EAR_MPI_File_read_all_F_enter(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror);
void EAR_MPI_File_read_all_F_leave(void);

void EAR_MPI_File_read_at_F_enter(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror);
void EAR_MPI_File_read_at_F_leave(void);

void EAR_MPI_File_read_at_all_F_enter(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror);
void EAR_MPI_File_read_at_all_F_leave(void);

void EAR_MPI_File_write_F_enter(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror);
void EAR_MPI_File_write_F_leave(void);

void EAR_MPI_File_write_all_F_enter(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror);
void EAR_MPI_File_write_all_F_leave(void);

void EAR_MPI_File_write_at_F_enter(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror);
void EAR_MPI_File_write_at_F_leave(void);

void EAR_MPI_File_write_at_all_F_enter(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror);
void EAR_MPI_File_write_at_all_F_leave(void);

void EAR_MPI_Finalize_F_enter(MPI_Fint *ierror);
void EAR_MPI_Finalize_F_leave(void);

void EAR_MPI_Gather_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Gather_F_leave(void);

void EAR_MPI_Gatherv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Gatherv_F_leave(void);

void EAR_MPI_Get_F_enter(MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Get_F_leave(void);

void EAR_MPI_Ibsend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Ibsend_F_leave(void);

void EAR_MPI_Init_F_enter(MPI_Fint *ierror);
void EAR_MPI_Init_F_leave(void);

void EAR_MPI_Init_thread_F_enter(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror);
void EAR_MPI_Init_thread_F_leave(void);

void EAR_MPI_Intercomm_create_F_enter(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror);
void EAR_MPI_Intercomm_create_F_leave(void);

void EAR_MPI_Intercomm_merge_F_enter(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror);
void EAR_MPI_Intercomm_merge_F_leave(void);

void EAR_MPI_Iprobe_F_enter(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Iprobe_F_leave(void);

void EAR_MPI_Irecv_F_enter(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Irecv_F_leave(void);

void EAR_MPI_Irsend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Irsend_F_leave(void);

void EAR_MPI_Isend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Isend_F_leave(void);

void EAR_MPI_Issend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Issend_F_leave(void);

void EAR_MPI_Probe_F_enter(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Probe_F_leave(void);

void EAR_MPI_Put_F_enter(MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Put_F_leave(void);

void EAR_MPI_Recv_F_enter(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Recv_F_leave(void);

void EAR_MPI_Recv_init_F_enter(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Recv_init_F_leave(void);

void EAR_MPI_Reduce_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Reduce_F_leave(void);

void EAR_MPI_Reduce_scatter_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Reduce_scatter_F_leave(void);

void EAR_MPI_Request_free_F_enter(MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Request_free_F_leave(void);

void EAR_MPI_Request_get_status_F_enter(MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Request_get_status_F_leave(void);

void EAR_MPI_Rsend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Rsend_F_leave(void);

void EAR_MPI_Rsend_init_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Rsend_init_F_leave(void);

void EAR_MPI_Scan_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Scan_F_leave(void);

void EAR_MPI_Scatter_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Scatter_F_leave(void);

void EAR_MPI_Scatterv_F_enter(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Scatterv_F_leave(void);

void EAR_MPI_Send_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Send_F_leave(void);

void EAR_MPI_Send_init_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Send_init_F_leave(void);

void EAR_MPI_Sendrecv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Sendrecv_F_leave(void);

void EAR_MPI_Sendrecv_replace_F_enter(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Sendrecv_replace_F_leave(void);

void EAR_MPI_Ssend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Ssend_F_leave(void);

void EAR_MPI_Ssend_init_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Ssend_init_F_leave(void);

void EAR_MPI_Start_F_enter(MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Start_F_leave(void);

void EAR_MPI_Startall_F_enter(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror);
void EAR_MPI_Startall_F_leave(void);

void EAR_MPI_Test_F_enter(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Test_F_leave(void);

void EAR_MPI_Testall_F_enter(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror);
void EAR_MPI_Testall_F_leave(void);

void EAR_MPI_Testany_F_enter(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Testany_F_leave(void);

void EAR_MPI_Testsome_F_enter(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror);
void EAR_MPI_Testsome_F_leave(void);

void EAR_MPI_Wait_F_enter(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Wait_F_leave(void);

void EAR_MPI_Waitall_F_enter(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror);
void EAR_MPI_Waitall_F_leave(void);

void EAR_MPI_Waitany_F_enter(MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror);
void EAR_MPI_Waitany_F_leave(void);

void EAR_MPI_Waitsome_F_enter(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror);
void EAR_MPI_Waitsome_F_leave(void);

void EAR_MPI_Win_complete_F_enter(MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Win_complete_F_leave(void);

void EAR_MPI_Win_create_F_enter(void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Win_create_F_leave(void);

void EAR_MPI_Win_fence_F_enter(MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Win_fence_F_leave(void);

void EAR_MPI_Win_free_F_enter(MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Win_free_F_leave(void);

void EAR_MPI_Win_post_F_enter(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Win_post_F_leave(void);

void EAR_MPI_Win_start_F_enter(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Win_start_F_leave(void);

void EAR_MPI_Win_wait_F_enter(MPI_Fint *win, MPI_Fint *ierror);
void EAR_MPI_Win_wait_F_leave(void); /**@}*/ 

#if MPI_VERSION >= 3
void EAR_MPI_Iallgather_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Iallgather_F_leave(void);

void EAR_MPI_Iallgatherv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Iallgatherv_F_leave(void);

void EAR_MPI_Iallreduce_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Iallreduce_F_leave(void);

void EAR_MPI_Ialltoall_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Ialltoall_F_leave(void);

void EAR_MPI_Ialltoallv_F_enter(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror);
void EAR_MPI_Ialltoallv_F_leave(void);

void EAR_MPI_Ibarrier_F_enter(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Ibarrier_F_leave(void);

void EAR_MPI_Ibcast_F_enter(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Ibcast_F_leave(void);

void EAR_MPI_Igather_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Igather_F_leave(void);

void EAR_MPI_Igatherv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Igatherv_F_leave(void);

void EAR_MPI_Ireduce_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Ireduce_F_leave(void);

void EAR_MPI_Ireduce_scatter_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Ireduce_scatter_F_leave(void);

void EAR_MPI_Iscan_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Iscan_F_leave(void);

void EAR_MPI_Iscatter_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Iscatter_F_leave(void);

void EAR_MPI_Iscatterv_F_enter(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror);
void EAR_MPI_Iscatterv_F_leave(void);
#endif

#ifdef __cplusplus
}
#endif

#endif //MPI_INTERFACEF_H
