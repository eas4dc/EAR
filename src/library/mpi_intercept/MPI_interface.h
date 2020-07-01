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

 /*   This program is part of the Energy Aware Runtime (EAR).
    It has been developed in the context of the BSC-Lenovo Collaboration project.
    
    Copyright (C) 2017  Authors: Julita Corbalan (julita.corbalan@bsc.es) 
    and Luigi Brochard (lbrochard@lenovo.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MPI_INTERFACE_H
#define MPI_INTERFACE_H

#include <library/mpi_intercept/mpi.h>

/**@{* Each pair of functions are called before (enter function) and after (leave) a type MPI call. Which pair depends on the
*   type of the call, and the functions called are EAR_MPY_<call>_enter and leave. */
void EAR_MPI_Allgather_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
void EAR_MPI_Allgather_leave(void);

void EAR_MPI_Allgatherv_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, MPI_Comm comm);
void EAR_MPI_Allgatherv_leave(void);

void EAR_MPI_Allreduce_enter(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
void EAR_MPI_Allreduce_leave(void);

void EAR_MPI_Alltoall_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
void EAR_MPI_Alltoall_leave(void);

void EAR_MPI_Alltoallv_enter(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *sdispls, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
void EAR_MPI_Alltoallv_leave(void);

void EAR_MPI_Barrier_enter(MPI_Comm comm);
void EAR_MPI_Barrier_leave(void);

void EAR_MPI_Bcast_enter(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
void EAR_MPI_Bcast_leave(void);

void EAR_MPI_Bsend_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
void EAR_MPI_Bsend_leave(void);

void EAR_MPI_Bsend_init_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Bsend_init_leave(void);

void EAR_MPI_Cancel_enter(MPI_Request *request);
void EAR_MPI_Cancel_leave(void);

void EAR_MPI_Cart_create_enter(MPI_Comm comm_old, int ndims, MPI3_CONST int dims[], MPI3_CONST int periods[], int reorder, MPI_Comm *comm_cart);
void EAR_MPI_Cart_create_leave(void);

void EAR_MPI_Cart_sub_enter(MPI_Comm comm, MPI3_CONST int remain_dims[], MPI_Comm *newcomm);
void EAR_MPI_Cart_sub_leave(void);

void EAR_MPI_Comm_create_enter(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
void EAR_MPI_Comm_create_leave(void);

void EAR_MPI_Comm_dup_enter(MPI_Comm comm, MPI_Comm *newcomm);
void EAR_MPI_Comm_dup_leave(void);

void EAR_MPI_Comm_free_enter(MPI_Comm *comm);
void EAR_MPI_Comm_free_leave(void);

void EAR_MPI_Comm_rank_enter(MPI_Comm comm, int *rank);
void EAR_MPI_Comm_rank_leave(void);

void EAR_MPI_Comm_size_enter(MPI_Comm comm, int *size);
void EAR_MPI_Comm_size_leave(void);

void EAR_MPI_Comm_spawn_enter(MPI3_CONST char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
void EAR_MPI_Comm_spawn_leave(void);

void EAR_MPI_Comm_spawn_multiple_enter(int count, char *array_of_commands[], char **array_of_argv[], MPI3_CONST int array_of_maxprocs[], MPI3_CONST MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
void EAR_MPI_Comm_spawn_multiple_leave(void);

void EAR_MPI_Comm_split_enter(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
void EAR_MPI_Comm_split_leave(void);

void EAR_MPI_File_close_enter(MPI_File *fh);
void EAR_MPI_File_close_leave(void);

void EAR_MPI_File_read_enter(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
void EAR_MPI_File_read_leave(void);

void EAR_MPI_File_read_all_enter(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
void EAR_MPI_File_read_all_leave(void);

void EAR_MPI_File_read_at_enter(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
void EAR_MPI_File_read_at_leave(void);

void EAR_MPI_File_read_at_all_enter(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
void EAR_MPI_File_read_at_all_leave(void);

void EAR_MPI_File_write_enter(MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
void EAR_MPI_File_write_leave(void);

void EAR_MPI_File_write_all_enter(MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
void EAR_MPI_File_write_all_leave(void);

void EAR_MPI_File_write_at_enter(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
void EAR_MPI_File_write_at_leave(void);

void EAR_MPI_File_write_at_all_enter(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
void EAR_MPI_File_write_at_all_leave(void);

void EAR_MPI_Finalize_enter(void);
void EAR_MPI_Finalize_leave(void);

void EAR_MPI_Gather_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
void EAR_MPI_Gather_leave(void);

void EAR_MPI_Gatherv_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
void EAR_MPI_Gatherv_leave(void);

void EAR_MPI_Get_enter(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
void EAR_MPI_Get_leave(void);

void EAR_MPI_Ibsend_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Ibsend_leave(void);

void EAR_MPI_Init_enter(int *argc, char ***argv);
void EAR_MPI_Init_leave(void);

void EAR_MPI_Init_thread_enter(int *argc, char ***argv, int required, int *provided);
void EAR_MPI_Init_thread_leave(void);

void EAR_MPI_Intercomm_create_enter(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm);
void EAR_MPI_Intercomm_create_leave(void);

void EAR_MPI_Intercomm_merge_enter(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
void EAR_MPI_Intercomm_merge_leave(void);

void EAR_MPI_Iprobe_enter(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
void EAR_MPI_Iprobe_leave(void);

void EAR_MPI_Irecv_enter(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Irecv_leave(void);

void EAR_MPI_Irsend_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Irsend_leave(void);

void EAR_MPI_Isend_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Isend_leave(void);

void EAR_MPI_Issend_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Issend_leave(void);

void EAR_MPI_Probe_enter(int source, int tag, MPI_Comm comm, MPI_Status *status);
void EAR_MPI_Probe_leave(void);

void EAR_MPI_Put_enter(MPI3_CONST void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
void EAR_MPI_Put_leave(void);

void EAR_MPI_Recv_enter(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
void EAR_MPI_Recv_leave(void);

void EAR_MPI_Recv_init_enter(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Recv_init_leave(void);

void EAR_MPI_Reduce_enter(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
void EAR_MPI_Reduce_leave(void);

void EAR_MPI_Reduce_scatter_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
void EAR_MPI_Reduce_scatter_leave(void);

void EAR_MPI_Request_free_enter(MPI_Request *request);
void EAR_MPI_Request_free_leave(void);

void EAR_MPI_Request_get_status_enter(MPI_Request request, int *flag, MPI_Status *status);
void EAR_MPI_Request_get_status_leave(void);

void EAR_MPI_Rsend_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
void EAR_MPI_Rsend_leave(void);

void EAR_MPI_Rsend_init_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Rsend_init_leave(void);

void EAR_MPI_Scan_enter(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
void EAR_MPI_Scan_leave(void);

void EAR_MPI_Scatter_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
void EAR_MPI_Scatter_leave(void);

void EAR_MPI_Scatterv_enter(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
void EAR_MPI_Scatterv_leave(void);

void EAR_MPI_Send_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
void EAR_MPI_Send_leave(void);

void EAR_MPI_Send_init_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Send_init_leave(void);

void EAR_MPI_Sendrecv_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
void EAR_MPI_Sendrecv_leave(void);

void EAR_MPI_Sendrecv_replace_enter(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
void EAR_MPI_Sendrecv_replace_leave(void);

void EAR_MPI_Ssend_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
void EAR_MPI_Ssend_leave(void);

void EAR_MPI_Ssend_init_enter(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Ssend_init_leave(void);

void EAR_MPI_Start_enter(MPI_Request *request);
void EAR_MPI_Start_leave(void);

void EAR_MPI_Startall_enter(int count, MPI_Request array_of_requests[]);
void EAR_MPI_Startall_leave(void);

void EAR_MPI_Test_enter(MPI_Request *request, int *flag, MPI_Status *status);
void EAR_MPI_Test_leave(void);

void EAR_MPI_Testall_enter(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[]);
void EAR_MPI_Testall_leave(void);

void EAR_MPI_Testany_enter(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status);
void EAR_MPI_Testany_leave(void);

void EAR_MPI_Testsome_enter(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]);
void EAR_MPI_Testsome_leave(void);

void EAR_MPI_Wait_enter(MPI_Request *request, MPI_Status *status);
void EAR_MPI_Wait_leave(void);

void EAR_MPI_Waitall_enter(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses);
void EAR_MPI_Waitall_leave(void);

void EAR_MPI_Waitany_enter(int count, MPI_Request *requests, int *index, MPI_Status *status);
void EAR_MPI_Waitany_leave(void);

void EAR_MPI_Waitsome_enter(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses);
void EAR_MPI_Waitsome_leave(void);

void EAR_MPI_Win_complete_enter(MPI_Win win);
void EAR_MPI_Win_complete_leave(void);

void EAR_MPI_Win_create_enter(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
void EAR_MPI_Win_create_leave(void);

void EAR_MPI_Win_fence_enter(int assert, MPI_Win win);
void EAR_MPI_Win_fence_leave(void);

void EAR_MPI_Win_free_enter(MPI_Win *win);
void EAR_MPI_Win_free_leave(void);

void EAR_MPI_Win_post_enter(MPI_Group group, int assert, MPI_Win win);
void EAR_MPI_Win_post_leave(void);

void EAR_MPI_Win_start_enter(MPI_Group group, int assert, MPI_Win win);
void EAR_MPI_Win_start_leave(void);

void EAR_MPI_Win_wait_enter(MPI_Win win);
void EAR_MPI_Win_wait_leave(void); 
/**@}*/ 

#if MPI_VERSION >= 3
void EAR_MPI_Iallgather_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Iallgather_leave(void);

void EAR_MPI_Iallgatherv_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Iallgatherv_leave(void);

void EAR_MPI_Iallreduce_enter(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Iallreduce_leave(void);

void EAR_MPI_Ialltoall_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Ialltoall_leave(void);

void EAR_MPI_Ialltoallv_enter(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Ialltoallv_leave(void);

void EAR_MPI_Ibarrier_enter(MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Ibarrier_leave(void);

void EAR_MPI_Ibcast_enter(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Ibcast_leave(void);

void EAR_MPI_Igather_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Igather_leave(void);

void EAR_MPI_Igatherv_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Igatherv_leave(void);

void EAR_MPI_Ireduce_enter(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Ireduce_leave(void);

void EAR_MPI_Ireduce_scatter_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Ireduce_scatter_leave(void);

void EAR_MPI_Iscan_enter(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Iscan_leave(void);

void EAR_MPI_Iscatter_enter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Iscatter_leave(void);

void EAR_MPI_Iscatterv_enter(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
void EAR_MPI_Iscatterv_leave(void);
#endif

void EAR_MPI_node_barrier(void);

#endif //MPI_INTERFACE_H
