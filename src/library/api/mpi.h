/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef LIBRARY_API_MPI_H
#define LIBRARY_API_MPI_H

#include <mpi.h>
#include <library/api/mpi_support.h>

#ifndef MPI3_CONST
#if MPI_VERSION >= 3
#define MPI3_CONST const
#else
#define MPI3_CONST
#endif
#endif

typedef struct mpic_s {
	int (*Allgather)(MPI3_CONST void *sendbuf, int sendcount,
			 MPI_Datatype sendtype, void *recvbuf, int recvcount,
			 MPI_Datatype recvtype, MPI_Comm comm);
	int (*Allgatherv)(MPI3_CONST void *sendbuf, int sendcount,
			  MPI_Datatype sendtype, void *recvbuf,
			  MPI3_CONST int *recvcounts, MPI3_CONST int *displs,
			  MPI_Datatype recvtype, MPI_Comm comm);
	int (*Allreduce)(MPI3_CONST void *sendbuf, void *recvbuf, int count,
			 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
	int (*Alltoall)(MPI3_CONST void *sendbuf, int sendcount,
			MPI_Datatype sendtype, void *recvbuf, int recvcount,
			MPI_Datatype recvtype, MPI_Comm comm);
	int (*Alltoallv)(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts,
			 MPI3_CONST int *sdispls, MPI_Datatype sendtype,
			 void *recvbuf, MPI3_CONST int *recvcounts,
			 MPI3_CONST int *rdispls, MPI_Datatype recvtype,
			 MPI_Comm comm);
	int (*Barrier)(MPI_Comm comm);
	int (*Bcast)(void *buffer, int count, MPI_Datatype datatype, int root,
		     MPI_Comm comm);
	int (*Bsend)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		     int dest, int tag, MPI_Comm comm);
	int (*Bsend_init)(MPI3_CONST void *buf, int count,
			  MPI_Datatype datatype, int dest, int tag,
			  MPI_Comm comm, MPI_Request * request);
	int (*Cancel)(MPI_Request * request);
	int (*Cart_create)(MPI_Comm comm_old, int ndims, MPI3_CONST int dims[],
			   MPI3_CONST int periods[], int reorder,
			   MPI_Comm * comm_cart);
	int (*Cart_sub)(MPI_Comm comm, MPI3_CONST int remain_dims[],
			MPI_Comm * newcomm);
	int (*Comm_create)(MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm);
	int (*Comm_dup)(MPI_Comm comm, MPI_Comm * newcomm);
	int (*Comm_free)(MPI_Comm * comm);
	int (*Comm_rank)(MPI_Comm comm, int *rank);
	int (*Comm_size)(MPI_Comm comm, int *size);
	int (*Comm_spawn)(MPI3_CONST char *command, char *argv[], int maxprocs,
			  MPI_Info info, int root, MPI_Comm comm,
			  MPI_Comm * intercomm, int array_of_errcodes[]);
	int (*Comm_spawn_multiple)(int count, char *array_of_commands[],
				   char **array_of_argv[],
				   MPI3_CONST int array_of_maxprocs[],
				   MPI3_CONST MPI_Info array_of_info[],
				   int root, MPI_Comm comm,
				   MPI_Comm * intercomm,
				   int array_of_errcodes[]);
	int (*Comm_split)(MPI_Comm comm, int color, int key,
			  MPI_Comm * newcomm);
	int (*File_close)(MPI_File * fh);
	int (*File_read)(MPI_File fh, void *buf, int count,
			 MPI_Datatype datatype, MPI_Status * status);
	int (*File_read_all)(MPI_File fh, void *buf, int count,
			     MPI_Datatype datatype, MPI_Status * status);
	int (*File_read_at)(MPI_File fh, MPI_Offset offset, void *buf,
			    int count, MPI_Datatype datatype,
			    MPI_Status * status);
	int (*File_read_at_all)(MPI_File fh, MPI_Offset offset, void *buf,
				int count, MPI_Datatype datatype,
				MPI_Status * status);
	int (*File_write)(MPI_File fh, MPI3_CONST void *buf, int count,
			  MPI_Datatype datatype, MPI_Status * status);
	int (*File_write_all)(MPI_File fh, MPI3_CONST void *buf, int count,
			      MPI_Datatype datatype, MPI_Status * status);
	int (*File_write_at)(MPI_File fh, MPI_Offset offset,
			     MPI3_CONST void *buf, int count,
			     MPI_Datatype datatype, MPI_Status * status);
	int (*File_write_at_all)(MPI_File fh, MPI_Offset offset,
				 MPI3_CONST void *buf, int count,
				 MPI_Datatype datatype, MPI_Status * status);
	int (*Finalize)(void);
	int (*Gather)(MPI3_CONST void *sendbuf, int sendcount,
		      MPI_Datatype sendtype, void *recvbuf, int recvcount,
		      MPI_Datatype recvtype, int root, MPI_Comm comm);
	int (*Gatherv)(MPI3_CONST void *sendbuf, int sendcount,
		       MPI_Datatype sendtype, void *recvbuf,
		       MPI3_CONST int *recvcounts, MPI3_CONST int *displs,
		       MPI_Datatype recvtype, int root, MPI_Comm comm);
	int (*Get)(void *origin_addr, int origin_count,
		   MPI_Datatype origin_datatype, int target_rank,
		   MPI_Aint target_disp, int target_count,
		   MPI_Datatype target_datatype, MPI_Win win);
	int (*Ibsend)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		      int dest, int tag, MPI_Comm comm, MPI_Request * request);
	int (*Init)(int *argc, char ***argv);
	int (*Init_thread)(int *argc, char ***argv, int required,
			   int *provided);
	int (*Intercomm_create)(MPI_Comm local_comm, int local_leader,
				MPI_Comm peer_comm, int remote_leader, int tag,
				MPI_Comm * newintercomm);
	int (*Intercomm_merge)(MPI_Comm intercomm, int high,
			       MPI_Comm * newintracomm);
	int (*Iprobe)(int source, int tag, MPI_Comm comm, int *flag,
		      MPI_Status * status);
	int (*Irecv)(void *buf, int count, MPI_Datatype datatype, int source,
		     int tag, MPI_Comm comm, MPI_Request * request);
	int (*Irsend)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		      int dest, int tag, MPI_Comm comm, MPI_Request * request);
	int (*Isend)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		     int dest, int tag, MPI_Comm comm, MPI_Request * request);
	int (*Issend)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		      int dest, int tag, MPI_Comm comm, MPI_Request * request);
	int (*Probe)(int source, int tag, MPI_Comm comm, MPI_Status * status);
	int (*Put)(MPI3_CONST void *origin_addr, int origin_count,
		   MPI_Datatype origin_datatype, int target_rank,
		   MPI_Aint target_disp, int target_count,
		   MPI_Datatype target_datatype, MPI_Win win);
	int (*Recv)(void *buf, int count, MPI_Datatype datatype, int source,
		    int tag, MPI_Comm comm, MPI_Status * status);
	int (*Recv_init)(void *buf, int count, MPI_Datatype datatype,
			 int source, int tag, MPI_Comm comm,
			 MPI_Request * request);
	int (*Reduce)(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		      MPI_Datatype datatype, MPI_Op op, int root,
		      MPI_Comm comm);
	int (*Reduce_scatter)(MPI3_CONST void *sendbuf, void *recvbuf,
			      MPI3_CONST int *recvcounts, MPI_Datatype datatype,
			      MPI_Op op, MPI_Comm comm);
	int (*Request_free)(MPI_Request * request);
	int (*Request_get_status)(MPI_Request request, int *flag,
				  MPI_Status * status);
	int (*Rsend)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		     int dest, int tag, MPI_Comm comm);
	int (*Rsend_init)(MPI3_CONST void *buf, int count,
			  MPI_Datatype datatype, int dest, int tag,
			  MPI_Comm comm, MPI_Request * request);
	int (*Scan)(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
	int (*Scatter)(MPI3_CONST void *sendbuf, int sendcount,
		       MPI_Datatype sendtype, void *recvbuf, int recvcount,
		       MPI_Datatype recvtype, int root, MPI_Comm comm);
	int (*Scatterv)(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts,
			MPI3_CONST int *displs, MPI_Datatype sendtype,
			void *recvbuf, int recvcount, MPI_Datatype recvtype,
			int root, MPI_Comm comm);
	int (*Send)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		    int dest, int tag, MPI_Comm comm);
	int (*Send_init)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
			 int dest, int tag, MPI_Comm comm,
			 MPI_Request * request);
	int (*Sendrecv)(MPI3_CONST void *sendbuf, int sendcount,
			MPI_Datatype sendtype, int dest, int sendtag,
			void *recvbuf, int recvcount, MPI_Datatype recvtype,
			int source, int recvtag, MPI_Comm comm,
			MPI_Status * status);
	int (*Sendrecv_replace)(void *buf, int count, MPI_Datatype datatype,
				int dest, int sendtag, int source, int recvtag,
				MPI_Comm comm, MPI_Status * status);
	int (*Ssend)(MPI3_CONST void *buf, int count, MPI_Datatype datatype,
		     int dest, int tag, MPI_Comm comm);
	int (*Ssend_init)(MPI3_CONST void *buf, int count,
			  MPI_Datatype datatype, int dest, int tag,
			  MPI_Comm comm, MPI_Request * request);
	int (*Start)(MPI_Request * request);
	int (*Startall)(int count, MPI_Request array_of_requests[]);
	int (*Test)(MPI_Request * request, int *flag, MPI_Status * status);
	int (*Testall)(int count, MPI_Request array_of_requests[], int *flag,
		       MPI_Status array_of_statuses[]);
	int (*Testany)(int count, MPI_Request array_of_requests[], int *indx,
		       int *flag, MPI_Status * status);
	int (*Testsome)(int incount, MPI_Request array_of_requests[],
			int *outcount, int array_of_indices[],
			MPI_Status array_of_statuses[]);
	int (*Wait)(MPI_Request * request, MPI_Status * status);
	int (*Waitall)(int count, MPI_Request * array_of_requests,
		       MPI_Status * array_of_statuses);
	int (*Waitany)(int count, MPI_Request * requests, int *index,
		       MPI_Status * status);
	int (*Waitsome)(int incount, MPI_Request * array_of_requests,
			int *outcount, int *array_of_indices,
			MPI_Status * array_of_statuses);
	int (*Win_complete)(MPI_Win win);
	int (*Win_create)(void *base, MPI_Aint size, int disp_unit,
			  MPI_Info info, MPI_Comm comm, MPI_Win * win);
	int (*Win_fence)(int assert, MPI_Win win);
	int (*Win_free)(MPI_Win * win);
	int (*Win_post)(MPI_Group group, int assert, MPI_Win win);
	int (*Win_start)(MPI_Group group, int assert, MPI_Win win);
	int (*Win_wait)(MPI_Win win);
//#if MPI_VERSION >= 3
	int (*Iallgather)(MPI3_CONST void *sendbuf, int sendcount,
			  MPI_Datatype sendtype, void *recvbuf, int recvcount,
			  MPI_Datatype recvtype, MPI_Comm comm,
			  MPI_Request * request);
	int (*Iallgatherv)(MPI3_CONST void *sendbuf, int sendcount,
			   MPI_Datatype sendtype, void *recvbuf,
			   MPI3_CONST int recvcounts[], MPI3_CONST int displs[],
			   MPI_Datatype recvtype, MPI_Comm comm,
			   MPI_Request * request);
	int (*Iallreduce)(MPI3_CONST void *sendbuf, void *recvbuf, int count,
			  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
			  MPI_Request * request);
	int (*Ialltoall)(MPI3_CONST void *sendbuf, int sendcount,
			 MPI_Datatype sendtype, void *recvbuf, int recvcount,
			 MPI_Datatype recvtype, MPI_Comm comm,
			 MPI_Request * request);
	int (*Ialltoallv)(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[],
			  MPI3_CONST int sdispls[], MPI_Datatype sendtype,
			  void *recvbuf, MPI3_CONST int recvcounts[],
			  MPI3_CONST int rdispls[], MPI_Datatype recvtype,
			  MPI_Comm comm, MPI_Request * request);
	int (*Ibarrier)(MPI_Comm comm, MPI_Request * request);
	int (*Ibcast)(void *buffer, int count, MPI_Datatype datatype, int root,
		      MPI_Comm comm, MPI_Request * request);
	int (*Igather)(MPI3_CONST void *sendbuf, int sendcount,
		       MPI_Datatype sendtype, void *recvbuf, int recvcount,
		       MPI_Datatype recvtype, int root, MPI_Comm comm,
		       MPI_Request * request);
	int (*Igatherv)(MPI3_CONST void *sendbuf, int sendcount,
			MPI_Datatype sendtype, void *recvbuf,
			MPI3_CONST int recvcounts[], MPI3_CONST int displs[],
			MPI_Datatype recvtype, int root, MPI_Comm comm,
			MPI_Request * request);
	int (*Ireduce)(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		       MPI_Datatype datatype, MPI_Op op, int root,
		       MPI_Comm comm, MPI_Request * request);
	int (*Ireduce_scatter)(MPI3_CONST void *sendbuf, void *recvbuf,
			       MPI3_CONST int recvcounts[],
			       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
			       MPI_Request * request);
	int (*Iscan)(MPI3_CONST void *sendbuf, void *recvbuf, int count,
		     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
		     MPI_Request * request);
	int (*Iscatter)(MPI3_CONST void *sendbuf, int sendcount,
			MPI_Datatype sendtype, void *recvbuf, int recvcount,
			MPI_Datatype recvtype, int root, MPI_Comm comm,
			MPI_Request * request);
	int (*Iscatterv)(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[],
			 MPI3_CONST int displs[], MPI_Datatype sendtype,
			 void *recvbuf, int recvcount, MPI_Datatype recvtype,
			 int root, MPI_Comm comm, MPI_Request * request);
//#endif
} mpic_t;

typedef struct mpif_s {
	void (*allgather)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			  MPI_Fint * sendtype, void *recvbuf,
			  MPI_Fint * recvcount, MPI_Fint * recvtype,
			  MPI_Fint * comm, MPI_Fint * ierror);
	void (*allgatherv)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			   MPI_Fint * sendtype, void *recvbuf,
			   MPI3_CONST MPI_Fint * recvcounts,
			   MPI3_CONST MPI_Fint * displs, MPI_Fint * recvtype,
			   MPI_Fint * comm, MPI_Fint * ierror);
	void (*allreduce)(MPI3_CONST void *sendbuf, void *recvbuf,
			  MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * op,
			  MPI_Fint * comm, MPI_Fint * ierror);
	void (*alltoall)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			 MPI_Fint * sendtype, void *recvbuf,
			 MPI_Fint * recvcount, MPI_Fint * recvtype,
			 MPI_Fint * comm, MPI_Fint * ierror);
	void (*alltoallv)(MPI3_CONST void *sendbuf,
			  MPI3_CONST MPI_Fint * sendcounts,
			  MPI3_CONST MPI_Fint * sdispls, MPI_Fint * sendtype,
			  void *recvbuf, MPI3_CONST MPI_Fint * recvcounts,
			  MPI3_CONST MPI_Fint * rdispls, MPI_Fint * recvtype,
			  MPI_Fint * comm, MPI_Fint * ierror);
	void (*barrier)(MPI_Fint * comm, MPI_Fint * ierror);
	void (*bcast)(void *buffer, MPI_Fint * count, MPI_Fint * datatype,
		      MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror);
	void (*bsend)(MPI3_CONST void *buf, MPI_Fint * count,
		      MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
		      MPI_Fint * comm, MPI_Fint * ierror);
	void (*bsend_init)(MPI3_CONST void *buf, MPI_Fint * count,
			   MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			   MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
	void (*cancel)(MPI_Fint * request, MPI_Fint * ierror);
	void (*cart_create)(MPI_Fint * comm_old, MPI_Fint * ndims,
			    MPI3_CONST MPI_Fint * dims,
			    MPI3_CONST MPI_Fint * periods, MPI_Fint * reorder,
			    MPI_Fint * comm_cart, MPI_Fint * ierror);
	void (*cart_sub)(MPI_Fint * comm, MPI3_CONST MPI_Fint * remain_dims,
			 MPI_Fint * comm_new, MPI_Fint * ierror);
	void (*comm_create)(MPI_Fint * comm, MPI_Fint * group,
			    MPI_Fint * newcomm, MPI_Fint * ierror);
	void (*comm_dup)(MPI_Fint * comm, MPI_Fint * newcomm,
			 MPI_Fint * ierror);
	void (*comm_free)(MPI_Fint * comm, MPI_Fint * ierror);
	void (*comm_rank)(MPI_Fint * comm, MPI_Fint * rank, MPI_Fint * ierror);
	void (*comm_size)(MPI_Fint * comm, MPI_Fint * size, MPI_Fint * ierror);
	void (*comm_spawn)(MPI3_CONST char *command, char *argv,
			   MPI_Fint * maxprocs, MPI_Fint * info,
			   MPI_Fint * root, MPI_Fint * comm,
			   MPI_Fint * intercomm, MPI_Fint * array_of_errcodes,
			   MPI_Fint * ierror);
	void (*comm_spawn_multiple)(MPI_Fint * count, char *array_of_commands,
				    char *array_of_argv,
				    MPI3_CONST MPI_Fint * array_of_maxprocs,
				    MPI3_CONST MPI_Fint * array_of_info,
				    MPI_Fint * root, MPI_Fint * comm,
				    MPI_Fint * intercomm,
				    MPI_Fint * array_of_errcodes,
				    MPI_Fint * ierror);
	void (*comm_split)(MPI_Fint * comm, MPI_Fint * color, MPI_Fint * key,
			   MPI_Fint * newcomm, MPI_Fint * ierror);
	void (*file_close)(MPI_File * fh, MPI_Fint * ierror);
	void (*file_read)(MPI_File * fh, void *buf, MPI_Fint * count,
			  MPI_Fint * datatype, MPI_Status * status,
			  MPI_Fint * ierror);
	void (*file_read_all)(MPI_File * fh, void *buf, MPI_Fint * count,
			      MPI_Fint * datatype, MPI_Status * status,
			      MPI_Fint * ierror);
	void (*file_read_at)(MPI_File * fh, MPI_Offset * offset, void *buf,
			     MPI_Fint * count, MPI_Fint * datatype,
			     MPI_Status * status, MPI_Fint * ierror);
	void (*file_read_at_all)(MPI_File * fh, MPI_Offset * offset, void *buf,
				 MPI_Fint * count, MPI_Fint * datatype,
				 MPI_Status * status, MPI_Fint * ierror);
	void (*file_write)(MPI_File * fh, MPI3_CONST void *buf,
			   MPI_Fint * count, MPI_Fint * datatype,
			   MPI_Status * status, MPI_Fint * ierror);
	void (*file_write_all)(MPI_File * fh, MPI3_CONST void *buf,
			       MPI_Fint * count, MPI_Fint * datatype,
			       MPI_Status * status, MPI_Fint * ierror);
	void (*file_write_at)(MPI_File * fh, MPI_Offset * offset,
			      MPI3_CONST void *buf, MPI_Fint * count,
			      MPI_Fint * datatype, MPI_Status * status,
			      MPI_Fint * ierror);
	void (*file_write_at_all)(MPI_File * fh, MPI_Offset * offset,
				  MPI3_CONST void *buf, MPI_Fint * count,
				  MPI_Fint * datatype, MPI_Status * status,
				  MPI_Fint * ierror);
	void (*finalize)(MPI_Fint * ierror);
	void (*gather)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
		       MPI_Fint * sendtype, void *recvbuf, MPI_Fint * recvcount,
		       MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm,
		       MPI_Fint * ierror);
	void (*gatherv)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			MPI_Fint * sendtype, void *recvbuf,
			MPI3_CONST MPI_Fint * recvcounts,
			MPI3_CONST MPI_Fint * displs, MPI_Fint * recvtype,
			MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror);
	void (*get)(MPI_Fint * origin_addr, MPI_Fint * origin_count,
		    MPI_Fint * origin_datatype, MPI_Fint * target_rank,
		    MPI_Fint * target_disp, MPI_Fint * target_count,
		    MPI_Fint * target_datatype, MPI_Fint * win,
		    MPI_Fint * ierror);
	void (*ibsend)(MPI3_CONST void *buf, MPI_Fint * count,
		       MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
		       MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror);
	void (*init)(MPI_Fint * ierror);
	void (*init_thread)(MPI_Fint * required, MPI_Fint * provided,
			    MPI_Fint * ierror);
	void (*intercomm_create)(MPI_Fint * local_comm, MPI_Fint * local_leader,
				 MPI_Fint * peer_comm, MPI_Fint * remote_leader,
				 MPI_Fint * tag, MPI_Fint * newintercomm,
				 MPI_Fint * ierror);
	void (*intercomm_merge)(MPI_Fint * intercomm, MPI_Fint * high,
				MPI_Fint * newintracomm, MPI_Fint * ierror);
	void (*iprobe)(MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
		       MPI_Fint * flag, MPI_Fint * status, MPI_Fint * ierror);
	void (*irecv)(void *buf, MPI_Fint * count, MPI_Fint * datatype,
		      MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
		      MPI_Fint * request, MPI_Fint * ierror);
	void (*irsend)(MPI3_CONST void *buf, MPI_Fint * count,
		       MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
		       MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror);
	void (*isend)(MPI3_CONST void *buf, MPI_Fint * count,
		      MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
		      MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror);
	void (*issend)(MPI3_CONST void *buf, MPI_Fint * count,
		       MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
		       MPI_Fint * comm, MPI_Fint * request, MPI_Fint * ierror);
	void (*probe)(MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
		      MPI_Fint * status, MPI_Fint * ierror);
	void (*put)(MPI3_CONST void *origin_addr, MPI_Fint * origin_count,
		    MPI_Fint * origin_datatype, MPI_Fint * target_rank,
		    MPI_Fint * target_disp, MPI_Fint * target_count,
		    MPI_Fint * target_datatype, MPI_Fint * win,
		    MPI_Fint * ierror);
	void (*recv)(void *buf, MPI_Fint * count, MPI_Fint * datatype,
		     MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
		     MPI_Fint * status, MPI_Fint * ierror);
	void (*recv_init)(void *buf, MPI_Fint * count, MPI_Fint * datatype,
			  MPI_Fint * source, MPI_Fint * tag, MPI_Fint * comm,
			  MPI_Fint * request, MPI_Fint * ierror);
	void (*reduce)(MPI3_CONST void *sendbuf, void *recvbuf,
		       MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * op,
		       MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror);
	void (*reduce_scatter)(MPI3_CONST void *sendbuf, void *recvbuf,
			       MPI3_CONST MPI_Fint * recvcounts,
			       MPI_Fint * datatype, MPI_Fint * op,
			       MPI_Fint * comm, MPI_Fint * ierror);
	void (*request_free)(MPI_Fint * request, MPI_Fint * ierror);
	void (*request_get_status)(MPI_Fint * request, int *flag,
				   MPI_Fint * status, MPI_Fint * ierror);
	void (*rsend)(MPI3_CONST void *buf, MPI_Fint * count,
		      MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
		      MPI_Fint * comm, MPI_Fint * ierror);
	void (*rsend_init)(MPI3_CONST void *buf, MPI_Fint * count,
			   MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			   MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
	void (*scan)(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint * count,
		     MPI_Fint * datatype, MPI_Fint * op, MPI_Fint * comm,
		     MPI_Fint * ierror);
	void (*scatter)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			MPI_Fint * sendtype, void *recvbuf,
			MPI_Fint * recvcount, MPI_Fint * recvtype,
			MPI_Fint * root, MPI_Fint * comm, MPI_Fint * ierror);
	void (*scatterv)(MPI3_CONST void *sendbuf,
			 MPI3_CONST MPI_Fint * sendcounts,
			 MPI3_CONST MPI_Fint * displs, MPI_Fint * sendtype,
			 void *recvbuf, MPI_Fint * recvcount,
			 MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm,
			 MPI_Fint * ierror);
	void (*send)(MPI3_CONST void *buf, MPI_Fint * count,
		     MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
		     MPI_Fint * comm, MPI_Fint * ierror);
	void (*send_init)(MPI3_CONST void *buf, MPI_Fint * count,
			  MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			  MPI_Fint * comm, MPI_Fint * request,
			  MPI_Fint * ierror);
	void (*sendrecv)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			 MPI_Fint * sendtype, MPI_Fint * dest,
			 MPI_Fint * sendtag, void *recvbuf,
			 MPI_Fint * recvcount, MPI_Fint * recvtype,
			 MPI_Fint * source, MPI_Fint * recvtag, MPI_Fint * comm,
			 MPI_Fint * status, MPI_Fint * ierror);
	void (*sendrecv_replace)(void *buf, MPI_Fint * count,
				 MPI_Fint * datatype, MPI_Fint * dest,
				 MPI_Fint * sendtag, MPI_Fint * source,
				 MPI_Fint * recvtag, MPI_Fint * comm,
				 MPI_Fint * status, MPI_Fint * ierror);
	void (*ssend)(MPI3_CONST void *buf, MPI_Fint * count,
		      MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
		      MPI_Fint * comm, MPI_Fint * ierror);
	void (*ssend_init)(MPI3_CONST void *buf, MPI_Fint * count,
			   MPI_Fint * datatype, MPI_Fint * dest, MPI_Fint * tag,
			   MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
	void (*start)(MPI_Fint * request, MPI_Fint * ierror);
	void (*startall)(MPI_Fint * count, MPI_Fint * array_of_requests,
			 MPI_Fint * ierror);
	void (*test)(MPI_Fint * request, MPI_Fint * flag, MPI_Fint * status,
		     MPI_Fint * ierror);
	void (*testall)(MPI_Fint * count, MPI_Fint * array_of_requests,
			MPI_Fint * flag, MPI_Fint * array_of_statuses,
			MPI_Fint * ierror);
	void (*testany)(MPI_Fint * count, MPI_Fint * array_of_requests,
			MPI_Fint * index, MPI_Fint * flag, MPI_Fint * status,
			MPI_Fint * ierror);
	void (*testsome)(MPI_Fint * incount, MPI_Fint * array_of_requests,
			 MPI_Fint * outcount, MPI_Fint * array_of_indices,
			 MPI_Fint * array_of_statuses, MPI_Fint * ierror);
	void (*wait)(MPI_Fint * request, MPI_Fint * status, MPI_Fint * ierror);
	void (*waitall)(MPI_Fint * count, MPI_Fint * array_of_requests,
			MPI_Fint * array_of_statuses, MPI_Fint * ierror);
	void (*waitany)(MPI_Fint * count, MPI_Fint * requests, MPI_Fint * index,
			MPI_Fint * status, MPI_Fint * ierror);
	void (*waitsome)(MPI_Fint * incount, MPI_Fint * array_of_requests,
			 MPI_Fint * outcount, MPI_Fint * array_of_indices,
			 MPI_Fint * array_of_statuses, MPI_Fint * ierror);
	void (*win_complete)(MPI_Fint * win, MPI_Fint * ierror);
	void (*win_create)(void *base, MPI_Aint * size, MPI_Fint * disp_unit,
			   MPI_Fint * info, MPI_Fint * comm, MPI_Fint * win,
			   MPI_Fint * ierror);
	void (*win_fence)(MPI_Fint * assert, MPI_Fint * win, MPI_Fint * ierror);
	void (*win_free)(MPI_Fint * win, MPI_Fint * ierror);
	void (*win_post)(MPI_Fint * group, MPI_Fint * assert, MPI_Fint * win,
			 MPI_Fint * ierror);
	void (*win_start)(MPI_Fint * group, MPI_Fint * assert, MPI_Fint * win,
			  MPI_Fint * ierror);
	void (*win_wait)(MPI_Fint * win, MPI_Fint * ierror);
//#if MPI_VERSION >= 3
	void (*iallgather)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			   MPI_Fint * sendtype, void *recvbuf,
			   MPI_Fint * recvcount, MPI_Fint * recvtype,
			   MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
	void (*iallgatherv)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			    MPI_Fint * sendtype, void *recvbuf,
			    MPI3_CONST MPI_Fint * recvcount,
			    MPI3_CONST MPI_Fint * displs, MPI_Fint * recvtype,
			    MPI_Fint * comm, MPI_Fint * request,
			    MPI_Fint * ierror);
	void (*iallreduce)(MPI3_CONST void *sendbuf, void *recvbuf,
			   MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * op,
			   MPI_Fint * comm, MPI_Fint * request,
			   MPI_Fint * ierror);
	void (*ialltoall)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			  MPI_Fint * sendtype, void *recvbuf,
			  MPI_Fint * recvcount, MPI_Fint * recvtype,
			  MPI_Fint * comm, MPI_Fint * request,
			  MPI_Fint * ierror);
	void (*ialltoallv)(MPI3_CONST void *sendbuf,
			   MPI3_CONST MPI_Fint * sendcounts,
			   MPI3_CONST MPI_Fint * sdispls, MPI_Fint * sendtype,
			   MPI_Fint * recvbuf, MPI3_CONST MPI_Fint * recvcounts,
			   MPI3_CONST MPI_Fint * rdispls, MPI_Fint * recvtype,
			   MPI_Fint * request, MPI_Fint * comm,
			   MPI_Fint * ierror);
	void (*ibarrier)(MPI_Fint * comm, MPI_Fint * request,
			 MPI_Fint * ierror);
	void (*ibcast)(void *buffer, MPI_Fint * count, MPI_Fint * datatype,
		       MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request,
		       MPI_Fint * ierror);
	void (*igather)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			MPI_Fint * sendtype, void *recvbuf,
			MPI_Fint * recvcount, MPI_Fint * recvtype,
			MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request,
			MPI_Fint * ierror);
	void (*igatherv)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			 MPI_Fint * sendtype, void *recvbuf,
			 MPI3_CONST MPI_Fint * recvcounts,
			 MPI3_CONST MPI_Fint * displs, MPI_Fint * recvtype,
			 MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request,
			 MPI_Fint * ierror);
	void (*ireduce)(MPI3_CONST void *sendbuf, void *recvbuf,
			MPI_Fint * count, MPI_Fint * datatype, MPI_Fint * op,
			MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request,
			MPI_Fint * ierror);
	void (*ireduce_scatter)(MPI3_CONST void *sendbuf, void *recvbuf,
				MPI3_CONST MPI_Fint * recvcounts,
				MPI_Fint * datatype, MPI_Fint * op,
				MPI_Fint * comm, MPI_Fint * request,
				MPI_Fint * ierror);
	void (*iscan)(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint * count,
		      MPI_Fint * datatype, MPI_Fint * op, MPI_Fint * comm,
		      MPI_Fint * request, MPI_Fint * ierror);
	void (*iscatter)(MPI3_CONST void *sendbuf, MPI_Fint * sendcount,
			 MPI_Fint * sendtype, void *recvbuf,
			 MPI_Fint * recvcount, MPI_Fint * recvtype,
			 MPI_Fint * root, MPI_Fint * comm, MPI_Fint * request,
			 MPI_Fint * ierror);
	void (*iscatterv)(MPI3_CONST void *sendbuf,
			  MPI3_CONST MPI_Fint * sendcounts,
			  MPI3_CONST MPI_Fint * displs, MPI_Fint * sendtype,
			  void *recvbuf, MPI_Fint * recvcount,
			  MPI_Fint * recvtype, MPI_Fint * root, MPI_Fint * comm,
			  MPI_Fint * request, MPI_Fint * ierror);
//#endif
} mpif_t;

#endif				//LIBRARY_API_MPI_H
