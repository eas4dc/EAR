/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef LIBRARY_API_MPIC_H
#define LIBRARY_API_MPIC_H

#if 0
// These declarations are not required. We are using dlsym+mpic_t.
int ear_mpic_Allgather(MPI3_CONST void *sendbuf, int sendcount,
		       MPI_Datatype sendtype, void *recvbuf, int recvcount,
		       MPI_Datatype recvtype, MPI_Comm comm);
int ear_mpic_Allgatherv(MPI3_CONST void *sendbuf, int sendcount,
			MPI_Datatype sendtype, void *recvbuf,
			MPI3_CONST int *recvcounts, MPI3_CONST int *displs,
			MPI_Datatype recvtype, MPI_Comm comm);
int ear_mpic_Allreduce(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int ear_mpic_Alltoall(MPI3_CONST void *sendbuf, int sendcount,
		      MPI_Datatype sendtype, void *recvbuf, int recvcount,
		      MPI_Datatype recvtype, MPI_Comm comm);
int ear_mpic_Alltoallv(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts,
		       MPI3_CONST int *sdispls, MPI_Datatype sendtype,
		       void *recvbuf, MPI3_CONST int *recvcounts,
		       MPI3_CONST int *rdispls, MPI_Datatype recvtype,
		       MPI_Comm comm);
int ear_mpic_Barrier(MPI_Comm comm);
int ear_mpic_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
		   MPI_Comm comm);
int ear_mpic_Bsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		   int dest, int tag, MPI_Comm comm);
int ear_mpic_Bsend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
			int dest, int tag, MPI_Comm comm,
			MPI_Request * request);
int ear_mpic_Cancel(MPI_Request * request);
int ear_mpic_Cart_create(MPI_Comm comm_old, int ndims, MPI3_CONST int dims[],
			 MPI3_CONST int periods[], int reorder,
			 MPI_Comm * comm_cart);
int ear_mpic_Cart_sub(MPI_Comm comm, MPI3_CONST int remain_dims[],
		      MPI_Comm * newcomm);
int ear_mpic_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm);
int ear_mpic_Comm_dup(MPI_Comm comm, MPI_Comm * newcomm);
int ear_mpic_Comm_free(MPI_Comm * comm);
int ear_mpic_Comm_rank(MPI_Comm comm, int *rank);
int ear_mpic_Comm_size(MPI_Comm comm, int *size);
int ear_mpic_Comm_spawn(MPI3_CONST char *command, char *argv[], int maxprocs,
			MPI_Info info, int root, MPI_Comm comm,
			MPI_Comm * intercomm, int array_of_errcodes[]);
int ear_mpic_Comm_spawn_multiple(int count, char *array_of_commands[],
				 char **array_of_argv[],
				 MPI3_CONST int array_of_maxprocs[],
				 MPI3_CONST MPI_Info array_of_info[], int root,
				 MPI_Comm comm, MPI_Comm * intercomm,
				 int array_of_errcodes[]);
int ear_mpic_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm * newcomm);
int ear_mpic_File_close(MPI_File * fh);
int ear_mpic_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
		       MPI_Status * status);
int ear_mpic_File_read_all(MPI_File fh, void *buf, int count,
			   MPI_Datatype datatype, MPI_Status * status);
int ear_mpic_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
			  MPI_Datatype datatype, MPI_Status * status);
int ear_mpic_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf,
			      int count, MPI_Datatype datatype,
			      MPI_Status * status);
int ear_mpic_File_write(MPI_File fh, MPI3_CONST void *buf, int count,
			MPI_Datatype datatype, MPI_Status * status);
int ear_mpic_File_write_all(MPI_File fh, MPI3_CONST void *buf, int count,
			    MPI_Datatype datatype, MPI_Status * status);
int ear_mpic_File_write_at(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf,
			   int count, MPI_Datatype datatype,
			   MPI_Status * status);
int ear_mpic_File_write_at_all(MPI_File fh, MPI_Offset offset,
			       MPI3_CONST void *buf, int count,
			       MPI_Datatype datatype, MPI_Status * status);
int ear_mpic_Finalize(void);
int ear_mpic_Gather(MPI3_CONST void *sendbuf, int sendcount,
		    MPI_Datatype sendtype, void *recvbuf, int recvcount,
		    MPI_Datatype recvtype, int root, MPI_Comm comm);
int ear_mpic_Gatherv(MPI3_CONST void *sendbuf, int sendcount,
		     MPI_Datatype sendtype, void *recvbuf,
		     MPI3_CONST int *recvcounts, MPI3_CONST int *displs,
		     MPI_Datatype recvtype, int root, MPI_Comm comm);
int ear_mpic_Get(void *origin_addr, int origin_count,
		 MPI_Datatype origin_datatype, int target_rank,
		 MPI_Aint target_disp, int target_count,
		 MPI_Datatype target_datatype, MPI_Win win);
int ear_mpic_Ibsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		    int dest, int tag, MPI_Comm comm, MPI_Request * request);
int ear_mpic_Init(int *argc, char ***argv);
int ear_mpic_Init_thread(int *argc, char ***argv, int required, int *provided);
int ear_mpic_Intercomm_create(MPI_Comm local_comm, int local_leader,
			      MPI_Comm peer_comm, int remote_leader, int tag,
			      MPI_Comm * newintercomm);
int ear_mpic_Intercomm_merge(MPI_Comm intercomm, int high,
			     MPI_Comm * newintracomm);
int ear_mpic_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
		    MPI_Status * status);
int ear_mpic_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
		   int tag, MPI_Comm comm, MPI_Request * request);
int ear_mpic_Irsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		    int dest, int tag, MPI_Comm comm, MPI_Request * request);
int ear_mpic_Isend(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		   int dest, int tag, MPI_Comm comm, MPI_Request * request);
int ear_mpic_Issend(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		    int dest, int tag, MPI_Comm comm, MPI_Request * request);
int ear_mpic_Probe(int source, int tag, MPI_Comm comm, MPI_Status * status);
int ear_mpic_Put(MPI3_CONST void *origin_addr, int origin_count,
		 MPI_Datatype origin_datatype, int target_rank,
		 MPI_Aint target_disp, int target_count,
		 MPI_Datatype target_datatype, MPI_Win win);
int ear_mpic_Recv(void *buf, int count, MPI_Datatype datatype, int source,
		  int tag, MPI_Comm comm, MPI_Status * status);
int ear_mpic_Recv_init(void *buf, int count, MPI_Datatype datatype, int source,
		       int tag, MPI_Comm comm, MPI_Request * request);
int ear_mpic_Reduce(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		    MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int ear_mpic_Reduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf,
			    MPI3_CONST int *recvcounts, MPI_Datatype datatype,
			    MPI_Op op, MPI_Comm comm);
int ear_mpic_Request_free(MPI_Request * request);
int ear_mpic_Request_get_status(MPI_Request request, int *flag,
				MPI_Status * status);
int ear_mpic_Rsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		   int dest, int tag, MPI_Comm comm);
int ear_mpic_Rsend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
			int dest, int tag, MPI_Comm comm,
			MPI_Request * request);
int ear_mpic_Scan(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int ear_mpic_Scatter(MPI3_CONST void *sendbuf, int sendcount,
		     MPI_Datatype sendtype, void *recvbuf, int recvcount,
		     MPI_Datatype recvtype, int root, MPI_Comm comm);
int ear_mpic_Scatterv(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts,
		      MPI3_CONST int *displs, MPI_Datatype sendtype,
		      void *recvbuf, int recvcount, MPI_Datatype recvtype,
		      int root, MPI_Comm comm);
int ear_mpic_Send(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		  int dest, int tag, MPI_Comm comm);
int ear_mpic_Send_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		       int dest, int tag, MPI_Comm comm, MPI_Request * request);
int ear_mpic_Sendrecv(MPI3_CONST void *sendbuf, int sendcount,
		      MPI_Datatype sendtype, int dest, int sendtag,
		      void *recvbuf, int recvcount, MPI_Datatype recvtype,
		      int source, int recvtag, MPI_Comm comm,
		      MPI_Status * status);
int ear_mpic_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
			      int dest, int sendtag, int source, int recvtag,
			      MPI_Comm comm, MPI_Status * status);
int ear_mpic_Ssend(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		   int dest, int tag, MPI_Comm comm);
int ear_mpic_Ssend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
			int dest, int tag, MPI_Comm comm,
			MPI_Request * request);
int ear_mpic_Start(MPI_Request * request);
int ear_mpic_Startall(int count, MPI_Request array_of_requests[]);
int ear_mpic_Test(MPI_Request * request, int *flag, MPI_Status * status);
int ear_mpic_Testall(int count, MPI_Request array_of_requests[], int *flag,
		     MPI_Status array_of_statuses[]);
int ear_mpic_Testany(int count, MPI_Request array_of_requests[], int *indx,
		     int *flag, MPI_Status * status);
int ear_mpic_Testsome(int incount, MPI_Request array_of_requests[],
		      int *outcount, int array_of_indices[],
		      MPI_Status array_of_statuses[]);
int ear_mpic_Wait(MPI_Request * request, MPI_Status * status);
int ear_mpic_Waitall(int count, MPI_Request * array_of_requests,
		     MPI_Status * array_of_statuses);
int ear_mpic_Waitany(int count, MPI_Request * requests, int *index,
		     MPI_Status * status);
int ear_mpic_Waitsome(int incount, MPI_Request * array_of_requests,
		      int *outcount, int *array_of_indices,
		      MPI_Status * array_of_statuses);
int ear_mpic_Win_complete(MPI_Win win);
int ear_mpic_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
			MPI_Comm comm, MPI_Win * win);
int ear_mpic_Win_fence(int assert, MPI_Win win);
int ear_mpic_Win_free(MPI_Win * win);
int ear_mpic_Win_post(MPI_Group group, int assert, MPI_Win win);
int ear_mpic_Win_start(MPI_Group group, int assert, MPI_Win win);
int ear_mpic_Win_wait(MPI_Win win);
//#if MPI_VERSION >= 3
int ear_mpic_Iallgather(MPI3_CONST void *sendbuf, int sendcount,
			MPI_Datatype sendtype, void *recvbuf, int recvcount,
			MPI_Datatype recvtype, MPI_Comm comm,
			MPI_Request * request);
int ear_mpic_Iallgatherv(MPI3_CONST void *sendbuf, int sendcount,
			 MPI_Datatype sendtype, void *recvbuf,
			 MPI3_CONST int recvcounts[], MPI3_CONST int displs[],
			 MPI_Datatype recvtype, MPI_Comm comm,
			 MPI_Request * request);
int ear_mpic_Iallreduce(MPI3_CONST void *sendbuf, void *recvbuf, int count,
			MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
			MPI_Request * request);
int ear_mpic_Ialltoall(MPI3_CONST void *sendbuf, int sendcount,
		       MPI_Datatype sendtype, void *recvbuf, int recvcount,
		       MPI_Datatype recvtype, MPI_Comm comm,
		       MPI_Request * request);
int ear_mpic_Ialltoallv(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[],
			MPI3_CONST int sdispls[], MPI_Datatype sendtype,
			void *recvbuf, MPI3_CONST int recvcounts[],
			MPI3_CONST int rdispls[], MPI_Datatype recvtype,
			MPI_Comm comm, MPI_Request * request);
int ear_mpic_Ibarrier(MPI_Comm comm, MPI_Request * request);
int ear_mpic_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root,
		    MPI_Comm comm, MPI_Request * request);
int ear_mpic_Igather(MPI3_CONST void *sendbuf, int sendcount,
		     MPI_Datatype sendtype, void *recvbuf, int recvcount,
		     MPI_Datatype recvtype, int root, MPI_Comm comm,
		     MPI_Request * request);
int ear_mpic_Igatherv(MPI3_CONST void *sendbuf, int sendcount,
		      MPI_Datatype sendtype, void *recvbuf,
		      MPI3_CONST int recvcounts[], MPI3_CONST int displs[],
		      MPI_Datatype recvtype, int root, MPI_Comm comm,
		      MPI_Request * request);
int ear_mpic_Ireduce(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		     MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
		     MPI_Request * request);
int ear_mpic_Ireduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf,
			     MPI3_CONST int recvcounts[], MPI_Datatype datatype,
			     MPI_Op op, MPI_Comm comm, MPI_Request * request);
int ear_mpic_Iscan(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
		   MPI_Request * request);
int ear_mpic_Iscatter(MPI3_CONST void *sendbuf, int sendcount,
		      MPI_Datatype sendtype, void *recvbuf, int recvcount,
		      MPI_Datatype recvtype, int root, MPI_Comm comm,
		      MPI_Request * request);
int ear_mpic_Iscatterv(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[],
		       MPI3_CONST int displs[], MPI_Datatype sendtype,
		       void *recvbuf, int recvcount, MPI_Datatype recvtype,
		       int root, MPI_Comm comm, MPI_Request * request);
#endif

const char *ear_mpic_names[] __attribute__((weak, visibility("hidden"))) = {
    "ear_mpic_Allgather",
    "ear_mpic_Allgatherv",
    "ear_mpic_Allreduce",
    "ear_mpic_Alltoall",
    "ear_mpic_Alltoallv",
    "ear_mpic_Barrier",
    "ear_mpic_Bcast",
    "ear_mpic_Bsend",
    "ear_mpic_Bsend_init",
    "ear_mpic_Cancel",
    "ear_mpic_Cart_create",
    "ear_mpic_Cart_sub",
    "ear_mpic_Comm_create",
    "ear_mpic_Comm_dup",
    "ear_mpic_Comm_free",
    "ear_mpic_Comm_rank",
    "ear_mpic_Comm_size",
    "ear_mpic_Comm_spawn",
    "ear_mpic_Comm_spawn_multiple",
    "ear_mpic_Comm_split",
    "ear_mpic_File_close",
    "ear_mpic_File_read",
    "ear_mpic_File_read_all",
    "ear_mpic_File_read_at",
    "ear_mpic_File_read_at_all",
    "ear_mpic_File_write",
    "ear_mpic_File_write_all",
    "ear_mpic_File_write_at",
    "ear_mpic_File_write_at_all",
    "ear_mpic_Finalize",
    "ear_mpic_Gather",
    "ear_mpic_Gatherv",
    "ear_mpic_Get",
    "ear_mpic_Ibsend",
    "ear_mpic_Init",
    "ear_mpic_Init_thread",
    "ear_mpic_Intercomm_create",
    "ear_mpic_Intercomm_merge",
    "ear_mpic_Iprobe",
    "ear_mpic_Irecv",
    "ear_mpic_Irsend",
    "ear_mpic_Isend",
    "ear_mpic_Issend",
    "ear_mpic_Probe",
    "ear_mpic_Put",
    "ear_mpic_Recv",
    "ear_mpic_Recv_init",
    "ear_mpic_Reduce",
    "ear_mpic_Reduce_scatter",
    "ear_mpic_Request_free",
    "ear_mpic_Request_get_status",
    "ear_mpic_Rsend",
    "ear_mpic_Rsend_init",
    "ear_mpic_Scan",
    "ear_mpic_Scatter",
    "ear_mpic_Scatterv",
    "ear_mpic_Send",
    "ear_mpic_Send_init",
    "ear_mpic_Sendrecv",
    "ear_mpic_Sendrecv_replace",
    "ear_mpic_Ssend",
    "ear_mpic_Ssend_init",
    "ear_mpic_Start",
    "ear_mpic_Startall",
    "ear_mpic_Test",
    "ear_mpic_Testall",
    "ear_mpic_Testany",
    "ear_mpic_Testsome",
    "ear_mpic_Wait",
    "ear_mpic_Waitall",
    "ear_mpic_Waitany",
    "ear_mpic_Waitsome",
    "ear_mpic_Win_complete",
    "ear_mpic_Win_create",
    "ear_mpic_Win_fence",
    "ear_mpic_Win_free",
    "ear_mpic_Win_post",
    "ear_mpic_Win_start",
    "ear_mpic_Win_wait"
    // #if MPI_VERSION >= 3
    ,
    "ear_mpic_Iallgather",
    "ear_mpic_Iallgatherv",
    "ear_mpic_Iallreduce",
    "ear_mpic_Ialltoall",
    "ear_mpic_Ialltoallv",
    "ear_mpic_Ibarrier",
    "ear_mpic_Ibcast",
    "ear_mpic_Igather",
    "ear_mpic_Igatherv",
    "ear_mpic_Ireduce",
    "ear_mpic_Ireduce_scatter",
    "ear_mpic_Iscan",
    "ear_mpic_Iscatter",
    "ear_mpic_Iscatterv",
    // #endif
};

#endif // LIBRARY_API_MPIC_H
