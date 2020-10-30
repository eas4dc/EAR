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

#include <library/loader/module_mpi.h>

extern mpic_t ear_mpic;

int MPI_Allgather(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	return ear_mpic.Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int MPI_Allgatherv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
	return ear_mpic.Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

int MPI_Allreduce(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	return ear_mpic.Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Alltoall(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
	return ear_mpic.Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

int MPI_Alltoallv(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *sdispls, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
	return ear_mpic.Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

int MPI_Barrier(MPI_Comm comm)
{
	return ear_mpic.Barrier(comm);
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
	return ear_mpic.Bcast(buffer, count, datatype, root, comm);
}

int MPI_Bsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	return ear_mpic.Bsend(buf, count, datatype, dest, tag, comm);
}

int MPI_Bsend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Bsend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Cancel(MPI_Request *request)
{
	return ear_mpic.Cancel(request);
}

int MPI_Cart_create(MPI_Comm comm_old, int ndims, MPI3_CONST int dims[], MPI3_CONST int periods[], int reorder, MPI_Comm *comm_cart)
{
	return ear_mpic.Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart);
}

int MPI_Cart_sub(MPI_Comm comm, MPI3_CONST int remain_dims[], MPI_Comm *newcomm)
{
	return ear_mpic.Cart_sub(comm, remain_dims, newcomm);
}

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
	return ear_mpic.Comm_create(comm, group, newcomm);
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
	return ear_mpic.Comm_dup(comm, newcomm);
}

int MPI_Comm_free(MPI_Comm *comm)
{
	return ear_mpic.Comm_free(comm);
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
	return ear_mpic.Comm_rank(comm, rank);
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
	return ear_mpic.Comm_size(comm, size);
}

int MPI_Comm_spawn(MPI3_CONST char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
	return ear_mpic.Comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes);
}

int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], MPI3_CONST int array_of_maxprocs[], MPI3_CONST MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
	return ear_mpic.Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes);
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
	return ear_mpic.Comm_split(comm, color, key, newcomm);
}

int MPI_File_close(MPI_File *fh)
{
	return ear_mpic.File_close(fh);
}

int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	return ear_mpic.File_read(fh, buf, count, datatype, status);
}

int MPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	return ear_mpic.File_read_all(fh, buf, count, datatype, status);
}

int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	return ear_mpic.File_read_at(fh, offset, buf, count, datatype, status);
}

int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	return ear_mpic.File_read_at_all(fh, offset, buf, count, datatype, status);
}

int MPI_File_write(MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	return ear_mpic.File_write(fh, buf, count, datatype, status);
}

int MPI_File_write_all(MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	return ear_mpic.File_write_all(fh, buf, count, datatype, status);
}

int MPI_File_write_at(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	return ear_mpic.File_write_at(fh, offset, buf, count, datatype, status);
}

int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
	return ear_mpic.File_write_at_all(fh, offset, buf, count, datatype, status);
}

int MPI_Finalize(void)
{
	return ear_mpic.Finalize();
}

int MPI_Gather(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	return ear_mpic.Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int MPI_Gatherv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	return ear_mpic.Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
}

int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
	return ear_mpic.Get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
}

int MPI_Ibsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Ibsend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Init(int *argc, char ***argv)
{
	return ear_mpic.Init(argc, argv);
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
	return ear_mpic.Init_thread(argc, argv, required, provided);
}

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm)
{
	return ear_mpic.Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
}

int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
	return ear_mpic.Intercomm_merge(intercomm, high, newintracomm);
}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
	return ear_mpic.Iprobe(source, tag, comm, flag, status);
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Irecv(buf, count, datatype, source, tag, comm, request);
}

int MPI_Irsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Irsend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Isend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Isend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Issend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Issend(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
	return ear_mpic.Probe(source, tag, comm, status);
}

int MPI_Put(MPI3_CONST void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
	return ear_mpic.Put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
	return ear_mpic.Recv(buf, count, datatype, source, tag, comm, status);
}

int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Recv_init(buf, count, datatype, source, tag, comm, request);
}

int MPI_Reduce(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
	return ear_mpic.Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int MPI_Reduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	return ear_mpic.Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}

int MPI_Request_free(MPI_Request *request)
{
	return ear_mpic.Request_free(request);
}

int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
	return ear_mpic.Request_get_status(request, flag, status);
}

int MPI_Rsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	return ear_mpic.Rsend(buf, count, datatype, dest, tag, comm);
}

int MPI_Rsend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Rsend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Scan(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	return ear_mpic.Scan(sendbuf, recvbuf, count, datatype, op, comm);
}

int MPI_Scatter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	return ear_mpic.Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int MPI_Scatterv(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	return ear_mpic.Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int MPI_Send(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	return ear_mpic.Send(buf, count, datatype, dest, tag, comm);
}

int MPI_Send_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Send_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Sendrecv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
	return ear_mpic.Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}

int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
	return ear_mpic.Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
}

int MPI_Ssend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	return ear_mpic.Ssend(buf, count, datatype, dest, tag, comm);
}

int MPI_Ssend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	return ear_mpic.Ssend_init(buf, count, datatype, dest, tag, comm, request);
}

int MPI_Start(MPI_Request *request)
{
	return ear_mpic.Start(request);
}

int MPI_Startall(int count, MPI_Request array_of_requests[])
{
	return ear_mpic.Startall(count, array_of_requests);
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
	return ear_mpic.Test(request, flag, status);
}

int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[])
{
	return ear_mpic.Testall(count, array_of_requests, flag, array_of_statuses);
}

int MPI_Testany(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status)
{
	return ear_mpic.Testany(count, array_of_requests, indx, flag, status);
}

int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
	return ear_mpic.Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
	return ear_mpic.Wait(request, status);
}

int MPI_Waitall(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses)
{
	return ear_mpic.Waitall(count, array_of_requests, array_of_statuses);
}

int MPI_Waitany(int count, MPI_Request *requests, int *index, MPI_Status *status)
{
	return ear_mpic.Waitany(count, requests, index, status);
}

int MPI_Waitsome(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
	return ear_mpic.Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

int MPI_Win_complete(MPI_Win win)
{
	return ear_mpic.Win_complete(win);
}

int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
	return ear_mpic.Win_create(base, size, disp_unit, info, comm, win);
}

int MPI_Win_fence(int assert, MPI_Win win)
{
	return ear_mpic.Win_fence(assert, win);
}

int MPI_Win_free(MPI_Win *win)
{
	return ear_mpic.Win_free(win);
}

int MPI_Win_post(MPI_Group group, int assert, MPI_Win win)
{
	return ear_mpic.Win_post(group, assert, win);
}

int MPI_Win_start(MPI_Group group, int assert, MPI_Win win)
{
	return ear_mpic.Win_start(group, assert, win);
}

int MPI_Win_wait(MPI_Win win)
{
	return ear_mpic.Win_wait(win);
}

//#MPI_VERSION >= 3
int MPI_Iallgather(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

int MPI_Iallgatherv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

int MPI_Iallreduce(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, request);
}

int MPI_Ialltoall(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

int MPI_Ialltoallv(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Ibarrier(comm, request);
}

int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Ibcast(buffer, count, datatype, root, comm, request);
}

int MPI_Igather(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

int MPI_Igatherv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request);
}

int MPI_Ireduce(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request);
}

int MPI_Ireduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, request);
}

int MPI_Iscan(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Iscan(sendbuf, recvbuf, count, datatype, op, comm, request);
}

int MPI_Iscatter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

int MPI_Iscatterv(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return ear_mpic.Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}
//#endif
