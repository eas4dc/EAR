/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef LIBRARY_API_MPIF_H
#define LIBRARY_API_MPIF_H

#if 0
// These declarations are not required. We are using dlsym+mpif_t.
void ear_mpif_Allgather_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			      MPI_Fint * sendtype, void *recvbuf,
			      MPI_Fint * recvcount, MPI_Fint * recvtype,
			      MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Allgatherv_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			       MPI_Fint * sendtype, void *recvbuf,
			       MPI3_CONST MPI_Fint * recvcounts,
			       MPI3_CONST MPI_Fint * displs,
			       MPI_Fint * recvtype, MPI_Fint * comm,
			       MPI_Fint * ierror);
void ear_mpif_Allreduce_enter(MPI3_CONST void *sendbuf, void *recvbuf,
			      MPI_Fint * count, MPI_Fint * datatype,
			      MPI_Fint * op, MPI_Fint * comm,
			      MPI_Fint * ierror);
void ear_mpif_Alltoall_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			     MPI_Fint * sendtype, void *recvbuf,
			     MPI_Fint * recvcount, MPI_Fint * recvtype,
			     MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Alltoallv_enter(MPI3_CONST void *sendbuf,
			      MPI3_CONST MPI_Fint * sendcounts,
			      MPI3_CONST MPI_Fint * sdispls,
			      MPI_Fint * sendtype, void *recvbuf,
			      MPI3_CONST MPI_Fint * recvcounts,
			      MPI3_CONST MPI_Fint * rdispls,
			      MPI_Fint * recvtype, MPI_Fint * comm,
			      MPI_Fint * ierror);
void ear_mpif_Barrier_enter(MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Bcast_enter(void *buffer, MPI_Fint * count, MPI_Fint * datatype,
			  MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Bsend_enter(MPI3_CONST void *buf, MPI_Fint * count,
			  MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			  MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Bsend_init_enter(MPI3_CONST void *buf, MPI_Fint * count,
			       MPI_Fint * datatype, MPI_Fint * dest,
			       MPI_Fint * tag, MPI_Fint * comm,
			       MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Cancel_enter(MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Cart_create_enter(MPI_Fint * comm_old, MPI_Fint * ndims,
				MPI3_CONST MPI_Fint * dims,
				MPI3_CONST MPI_Fint * periods,
				MPI_Fint * reorder, MPI_Fint * comm_cart,
				MPI_Fint * ierror);
void ear_mpif_Cart_sub_enter(MPI_Fint * comm, MPI3_CONST MPI_Fint * remain_dims,
			     MPI_Fint * comm_new, MPI_Fint * ierror);
void ear_mpif_Comm_create_enter(MPI_Fint * comm, MPI_Fint * group,
				MPI_Fint * newcomm, MPI_Fint * ierror);
void ear_mpif_Comm_dup_enter(MPI_Fint * comm, MPI_Fint * newcomm,
			     MPI_Fint * ierror);
void ear_mpif_Comm_free_enter(MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Comm_rank_enter(MPI_Fint * comm, MPI_Fint * rank,
			      MPI_Fint * ierror);
void ear_mpif_Comm_size_enter(MPI_Fint * comm, MPI_Fint * size,
			      MPI_Fint * ierror);
void ear_mpif_Comm_spawn_enter(MPI3_CONST char *command, char *argv,
			       MPI_Fint * maxprocs, MPI_Fint * info,
			       MPI_Fint * root, MPI_Fint * comm,
			       MPI_Fint * intercomm,
			       MPI_Fint * array_of_errcodes, MPI_Fint * ierror);
void ear_mpif_Comm_spawn_multiple_enter(MPI_Fint * count,
					char *array_of_commands,
					char *array_of_argv,
					MPI3_CONST MPI_Fint * array_of_maxprocs,
					MPI3_CONST MPI_Fint * array_of_info,
					MPI_Fint * root, MPI_Fint * comm,
					MPI_Fint * intercomm,
					MPI_Fint * array_of_errcodes,
					MPI_Fint * ierror);
void ear_mpif_Comm_split_enter(MPI_Fint * comm, MPI_Fint * color,
			       MPI_Fint * key, MPI_Fint * newcomm,
			       MPI_Fint * ierror);
void ear_mpif_File_close_enter(MPI_File * fh, MPI_Fint * ierror);
void ear_mpif_File_read_enter(MPI_File * fh, void *buf, MPI_Fint * count,
			      MPI_Fint * datatype, MPI_Status * status,
			      MPI_Fint * ierror);
void ear_mpif_File_read_all_enter(MPI_File * fh, void *buf, MPI_Fint * count,
				  MPI_Fint * datatype, MPI_Status * status,
				  MPI_Fint * ierror);
void ear_mpif_File_read_at_enter(MPI_File * fh, MPI_Offset * offset, void *buf,
				 MPI_Fint * count, MPI_Fint * datatype,
				 MPI_Status * status, MPI_Fint * ierror);
void ear_mpif_File_read_at_all_enter(MPI_File * fh, MPI_Offset * offset,
				     void *buf, MPI_Fint * count,
				     MPI_Fint * datatype, MPI_Status * status,
				     MPI_Fint * ierror);
void ear_mpif_File_write_enter(MPI_File * fh, MPI3_CONST void *buf,
			       MPI_Fint * count, MPI_Fint * datatype,
			       MPI_Status * status, MPI_Fint * ierror);
void ear_mpif_File_write_all_enter(MPI_File * fh, MPI3_CONST void *buf,
				   MPI_Fint * count, MPI_Fint * datatype,
				   MPI_Status * status, MPI_Fint * ierror);
void ear_mpif_File_write_at_enter(MPI_File * fh, MPI_Offset * offset,
				  MPI3_CONST void *buf, MPI_Fint * count,
				  MPI_Fint * datatype, MPI_Status * status,
				  MPI_Fint * ierror);
void ear_mpif_File_write_at_all_enter(MPI_File * fh, MPI_Offset * offset,
				      MPI3_CONST void *buf, MPI_Fint * count,
				      MPI_Fint * datatype, MPI_Status * status,
				      MPI_Fint * ierror);
void ear_mpif_Finalize_enter(MPI_Fint * ierror);
void ear_mpif_Gather_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			   MPI_Fint * sendtype, void *recvbuf,
			   MPI_Fint * recvcount, MPI_Fint * recvtype,
			   MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Gatherv_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			    MPI_Fint * sendtype, void *recvbuf,
			    MPI3_CONST MPI_Fint * recvcounts,
			    MPI3_CONST MPI_Fint * displs, MPI_Fint * recvtype,
			    MPI_Fint * root, MPI_Fint * comm,
			    MPI_Fint * ierror);
void ear_mpif_Get_enter(MPI_Fint * origin_addr, MPI_Fint * origin_count,
			MPI_Fint * origin_datatype, MPI_Fint * target_rank,
			MPI_Fint * target_disp, MPI_Fint * target_count,
			MPI_Fint * target_datatype, MPI_Fint * win,
			MPI_Fint * ierror);
void ear_mpif_Ibsend_enter(MPI3_CONST void *buf, MPI_Fint * count,
			   MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			   MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
void ear_mpif_Init_enter(MPI_Fint * ierror);
void ear_mpif_Init_thread_enter(MPI_Fint * required, MPI_Fint * provided,
				MPI_Fint * ierror);
void ear_mpif_Intercomm_create_enter(MPI_Fint * local_comm,
				     MPI_Fint * local_leader,
				     MPI_Fint * peer_comm,
				     MPI_Fint * remote_leader, MPI_Fint * tag,
				     MPI_Fint * newintercomm,
				     MPI_Fint * ierror);
void ear_mpif_Intercomm_merge_enter(MPI_Fint * intercomm, MPI_Fint * high,
				    MPI_Fint * newintracomm, MPI_Fint * ierror);
void ear_mpif_Iprobe_enter(MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
			   MPI_Fint * flag, MPI_Fint * status,
			   MPI_Fint * ierror);
void ear_mpif_Irecv_enter(void *buf, MPI_Fint * count, MPI_Fint * datatype,
			  MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
			  MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Irsend_enter(MPI3_CONST void *buf, MPI_Fint * count,
			   MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			   MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
void ear_mpif_Isend_enter(MPI3_CONST void *buf, MPI_Fint * count,
			  MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			  MPI_Fint * comm, MPI_Fint * request,
			  MPI_Fint * ierror);
void ear_mpif_Issend_enter(MPI3_CONST void *buf, MPI_Fint * count,
			   MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			   MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
void ear_mpif_Probe_enter(MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
			  MPI_Fint * status, MPI_Fint * ierror);
void ear_mpif_Put_enter(MPI3_CONST void *origin_addr, MPI_Fint * origin_count,
			MPI_Fint * origin_datatype, MPI_Fint * target_rank,
			MPI_Fint * target_disp, MPI_Fint * target_count,
			MPI_Fint * target_datatype, MPI_Fint * win,
			MPI_Fint * ierror);
void ear_mpif_Recv_enter(void *buf, MPI_Fint * count, MPI_Fint * datatype,
			 MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
			 MPI_Fint * status, MPI_Fint * ierror);
void ear_mpif_Recv_init_enter(void *buf, MPI_Fint * count, MPI_Fint * datatype,
			      MPI_Fint * source, MPI_Fint * tag,
			      MPI_Fint * comm, MPI_Fint * request,
			      MPI_Fint * ierror);
void ear_mpif_Reduce_enter(MPI3_CONST void *sendbuf, void *recvbuf,
			   MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * op,
			   MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Reduce_scatter_enter(MPI3_CONST void *sendbuf, void *recvbuf,
				   MPI3_CONST MPI_Fint * recvcounts,
				   MPI_Fint * datatype, MPI_Fint * op,
				   MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Request_free_enter(MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Request_get_status_enter(MPI_Fint * request, int *flag,
				       MPI_Fint * status, MPI_Fint * ierror);
void ear_mpif_Rsend_enter(MPI3_CONST void *buf, MPI_Fint * count,
			  MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			  MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Rsend_init_enter(MPI3_CONST void *buf, MPI_Fint * count,
			       MPI_Fint * datatype, MPI_Fint * dest,
			       MPI_Fint * tag, MPI_Fint * comm,
			       MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Scan_enter(MPI3_CONST void *sendbuf, void *recvbuf,
			 MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * op,
			 MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Scatter_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			    MPI_Fint * sendtype, void *recvbuf,
			    MPI_Fint * recvcount, MPI_Fint * recvtype,
			    MPI_Fint * root, MPI_Fint * comm,
			    MPI_Fint * ierror);
void ear_mpif_Scatterv_enter(MPI3_CONST void *sendbuf,
			     MPI3_CONST MPI_Fint * sendcounts,
			     MPI3_CONST MPI_Fint * displs, MPI_Fint * sendtype,
			     void *recvbuf, MPI_Fint * recvcount,
			     MPI_Fint * recvtype, MPI_Fint * root,
			     MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Send_enter(MPI3_CONST void *buf, MPI_Fint * count,
			 MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			 MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Send_init_enter(MPI3_CONST void *buf, MPI_Fint * count,
			      MPI_Fint * datatype, MPI_Fint * dest,
			      MPI_Fint * tag, MPI_Fint * comm,
			      MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Sendrecv_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			     MPI_Fint * sendtype, MPI_Fint * dest,
			     MPI_Fint * sendtag, void *recvbuf,
			     MPI_Fint * recvcount, MPI_Fint * recvtype,
			     MPI_Fint * source, MPI_Fint * recvtag,
			     MPI_Fint * comm, MPI_Fint * status,
			     MPI_Fint * ierror);
void ear_mpif_Sendrecv_replace_enter(void *buf, MPI_Fint * count,
				     MPI_Fint * datatype, MPI_Fint * dest,
				     MPI_Fint * sendtag, MPI_Fint * source,
				     MPI_Fint * recvtag, MPI_Fint * comm,
				     MPI_Fint * status, MPI_Fint * ierror);
void ear_mpif_Ssend_enter(MPI3_CONST void *buf, MPI_Fint * count,
			  MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			  MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Ssend_init_enter(MPI3_CONST void *buf, MPI_Fint * count,
			       MPI_Fint * datatype, MPI_Fint * dest,
			       MPI_Fint * tag, MPI_Fint * comm,
			       MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Start_enter(MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Startall_enter(MPI_Fint * count, MPI_Fint * array_of_requests,
			     MPI_Fint * ierror);
void ear_mpif_Test_enter(MPI_Fint * request, MPI_Fint * flag, MPI_Fint * status,
			 MPI_Fint * ierror);
void ear_mpif_Testall_enter(MPI_Fint * count, MPI_Fint * array_of_requests,
			    MPI_Fint * flag, MPI_Fint * array_of_statuses,
			    MPI_Fint * ierror);
void ear_mpif_Testany_enter(MPI_Fint * count, MPI_Fint * array_of_requests,
			    MPI_Fint * index, MPI_Fint * flag,
			    MPI_Fint * status, MPI_Fint * ierror);
void ear_mpif_Testsome_enter(MPI_Fint * incount, MPI_Fint * array_of_requests,
			     MPI_Fint * outcount, MPI_Fint * array_of_indices,
			     MPI_Fint * array_of_statuses, MPI_Fint * ierror);
void ear_mpif_Wait_enter(MPI_Fint * request, MPI_Fint * status,
			 MPI_Fint * ierror);
void ear_mpif_Waitall_enter(MPI_Fint * count, MPI_Fint * array_of_requests,
			    MPI_Fint * array_of_statuses, MPI_Fint * ierror);
void ear_mpif_Waitany_enter(MPI_Fint * count, MPI_Fint * requests,
			    MPI_Fint * index, MPI_Fint * status,
			    MPI_Fint * ierror);
void ear_mpif_Waitsome_enter(MPI_Fint * incount, MPI_Fint * array_of_requests,
			     MPI_Fint * outcount, MPI_Fint * array_of_indices,
			     MPI_Fint * array_of_statuses, MPI_Fint * ierror);
void ear_mpif_Win_complete_enter(MPI_Fint * win, MPI_Fint * ierror);
void ear_mpif_Win_create_enter(void *base, MPI_Aint * size,
			       MPI_Fint * disp_unit, MPI_Fint * info,
			       MPI_Fint * comm, MPI_Fint * win,
			       MPI_Fint * ierror);
void ear_mpif_Win_fence_enter(MPI_Fint * assert, MPI_Fint * win,
			      MPI_Fint * ierror);
void ear_mpif_Win_free_enter(MPI_Fint * win, MPI_Fint * ierror);
void ear_mpif_Win_post_enter(MPI_Fint * group, MPI_Fint * assert,
			     MPI_Fint * win, MPI_Fint * ierror);
void ear_mpif_Win_start_enter(MPI_Fint * group, MPI_Fint * assert,
			      MPI_Fint * win, MPI_Fint * ierror);
void ear_mpif_Win_wait_enter(MPI_Fint * win, MPI_Fint * ierror);
//#if MPI_VERSION >= 3
void ear_mpif_Iallgather_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			       MPI_Fint * sendtype, void *recvbuf,
			       MPI_Fint * recvcount, MPI_Fint * recvtype,
			       MPI_Fint * comm, MPI_Fint * request,
			       MPI_Fint * ierror);
void ear_mpif_Iallgatherv_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
				MPI_Fint * sendtype, void *recvbuf,
				MPI3_CONST MPI_Fint * recvcount,
				MPI3_CONST MPI_Fint * displs,
				MPI_Fint * recvtype, MPI_Fint * comm,
				MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Iallreduce_enter(MPI3_CONST void *sendbuf, void *recvbuf,
			       MPI_Fint * count, MPI_Fint * datatype,
			       MPI_Fint * op, MPI_Fint * comm,
			       MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Ialltoall_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			      MPI_Fint * sendtype, void *recvbuf,
			      MPI_Fint * recvcount, MPI_Fint * recvtype,
			      MPI_Fint * comm, MPI_Fint * request,
			      MPI_Fint * ierror);
void ear_mpif_Ialltoallv_enter(MPI3_CONST void *sendbuf,
			       MPI3_CONST MPI_Fint * sendcounts,
			       MPI3_CONST MPI_Fint * sdispls,
			       MPI_Fint * sendtype, MPI_Fint * recvbuf,
			       MPI3_CONST MPI_Fint * recvcounts,
			       MPI3_CONST MPI_Fint * rdispls,
			       MPI_Fint * recvtype, MPI_Fint * request,
			       MPI_Fint * comm, MPI_Fint * ierror);
void ear_mpif_Ibarrier_enter(MPI_Fint * comm, MPI_Fint * request,
			     MPI_Fint * ierror);
void ear_mpif_Ibcast_enter(void *buffer, MPI_Fint * count, MPI_Fint * datatype,
			   MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
void ear_mpif_Igather_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			    MPI_Fint * sendtype, void *recvbuf,
			    MPI_Fint * recvcount, MPI_Fint * recvtype,
			    MPI_Fint * root, MPI_Fint * comm,
			    MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Igatherv_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			     MPI_Fint * sendtype, void *recvbuf,
			     MPI3_CONST MPI_Fint * recvcounts,
			     MPI3_CONST MPI_Fint * displs, MPI_Fint * recvtype,
			     MPI_Fint * root, MPI_Fint * comm,
			     MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Ireduce_enter(MPI3_CONST void *sendbuf, void *recvbuf,
			    MPI_Fint * count, MPI_Fint * datatype,
			    MPI_Fint * op, MPI_Fint * root, MPI_Fint * comm,
			    MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Ireduce_scatter_enter(MPI3_CONST void *sendbuf, void *recvbuf,
				    MPI3_CONST MPI_Fint * recvcounts,
				    MPI_Fint * datatype, MPI_Fint * op,
				    MPI_Fint * comm, MPI_Fint * request,
				    MPI_Fint * ierror);
void ear_mpif_Iscan_enter(MPI3_CONST void *sendbuf, void *recvbuf,
			  MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * op,
			  MPI_Fint * comm, MPI_Fint * request,
			  MPI_Fint * ierror);
void ear_mpif_Iscatter_enter(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			     MPI_Fint * sendtype, void *recvbuf,
			     MPI_Fint * recvcount, MPI_Fint * recvtype,
			     MPI_Fint * root, MPI_Fint * comm,
			     MPI_Fint * request, MPI_Fint * ierror);
void ear_mpif_Iscatterv_enter(MPI3_CONST void *sendbuf,
			      MPI3_CONST MPI_Fint * sendcounts,
			      MPI3_CONST MPI_Fint * displs, MPI_Fint * sendtype,
			      void *recvbuf, MPI_Fint * recvcount,
			      MPI_Fint * recvtype, MPI_Fint * root,
			      MPI_Fint * comm, MPI_Fint * request,
			      MPI_Fint * ierror);
#endif

const char *ear_mpif_names[] __attribute__((weak, visibility("hidden"))) = {
	"ear_mpif_Allgather",
	"ear_mpif_Allgatherv",
	"ear_mpif_Allreduce",
	"ear_mpif_Alltoall",
	"ear_mpif_Alltoallv",
	"ear_mpif_Barrier",
	"ear_mpif_Bcast",
	"ear_mpif_Bsend",
	"ear_mpif_Bsend_init",
	"ear_mpif_Cancel",
	"ear_mpif_Cart_create",
	"ear_mpif_Cart_sub",
	"ear_mpif_Comm_create",
	"ear_mpif_Comm_dup",
	"ear_mpif_Comm_free",
	"ear_mpif_Comm_rank",
	"ear_mpif_Comm_size",
	"ear_mpif_Comm_spawn",
	"ear_mpif_Comm_spawn_multiple",
	"ear_mpif_Comm_split",
	"ear_mpif_File_close",
	"ear_mpif_File_read",
	"ear_mpif_File_read_all",
	"ear_mpif_File_read_at",
	"ear_mpif_File_read_at_all",
	"ear_mpif_File_write",
	"ear_mpif_File_write_all",
	"ear_mpif_File_write_at",
	"ear_mpif_File_write_at_all",
	"ear_mpif_Finalize",
	"ear_mpif_Gather",
	"ear_mpif_Gatherv",
	"ear_mpif_Get",
	"ear_mpif_Ibsend",
	"ear_mpif_Init",
	"ear_mpif_Init_thread",
	"ear_mpif_Intercomm_create",
	"ear_mpif_Intercomm_merge",
	"ear_mpif_Iprobe",
	"ear_mpif_Irecv",
	"ear_mpif_Irsend",
	"ear_mpif_Isend",
	"ear_mpif_Issend",
	"ear_mpif_Probe",
	"ear_mpif_Put",
	"ear_mpif_Recv",
	"ear_mpif_Recv_init",
	"ear_mpif_Reduce",
	"ear_mpif_Reduce_scatter",
	"ear_mpif_Request_free",
	"ear_mpif_Request_get_status",
	"ear_mpif_Rsend",
	"ear_mpif_Rsend_init",
	"ear_mpif_Scan",
	"ear_mpif_Scatter",
	"ear_mpif_Scatterv",
	"ear_mpif_Send",
	"ear_mpif_Send_init",
	"ear_mpif_Sendrecv",
	"ear_mpif_Sendrecv_replace",
	"ear_mpif_Ssend",
	"ear_mpif_Ssend_init",
	"ear_mpif_Start",
	"ear_mpif_Startall",
	"ear_mpif_Test",
	"ear_mpif_Testall",
	"ear_mpif_Testany",
	"ear_mpif_Testsome",
	"ear_mpif_Wait",
	"ear_mpif_Waitall",
	"ear_mpif_Waitany",
	"ear_mpif_Waitsome",
	"ear_mpif_Win_complete",
	"ear_mpif_Win_create",
	"ear_mpif_Win_fence",
	"ear_mpif_Win_free",
	"ear_mpif_Win_post",
	"ear_mpif_Win_start",
	"ear_mpif_Win_wait"
//#if MPI_VERSION >= 3
	    ,
	"ear_mpif_Iallgather",
	"ear_mpif_Iallgatherv",
	"ear_mpif_Iallreduce",
	"ear_mpif_Ialltoall",
	"ear_mpif_Ialltoallv",
	"ear_mpif_Ibarrier",
	"ear_mpif_Ibcast",
	"ear_mpif_Igather",
	"ear_mpif_Igatherv",
	"ear_mpif_Ireduce",
	"ear_mpif_Ireduce_scatter",
	"ear_mpif_Iscan",
	"ear_mpif_Iscatter",
	"ear_mpif_Iscatterv",
//#endif
};

#endif				//LIBRARY_API_MPIF_H
