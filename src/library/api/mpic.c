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

#include <common/output/debug.h>
#include <library/api/mpic.h>
#include <library/api/ear_mpi.h>

static mpic_t next_mpic;

void ear_mpic_setnext(mpic_t *_next_mpic)
{
	debug(">> C setnext...............");
	memcpy(&next_mpic, _next_mpic, sizeof(mpic_t));
	debug("<< C setnext...............");
}

int ear_mpic_Allgather(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    debug(">> C Allgather...............");
    before_mpi(Allgather, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    debug("<< C Allgather...............");
    after_mpi(Allgather);
	return res;
}

int ear_mpic_Allgatherv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, MPI_Comm comm)
{
    debug(">> C Allgatherv...............");
    before_mpi(Allgatherv,(p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    debug("<< C Allgatherv...............");
    after_mpi(Allgatherv);
	return res;
}

int ear_mpic_Allreduce(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    debug(">> C Allreduce...............");
    before_mpi(Allreduce, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
    debug("<< C Allreduce...............");
    after_mpi(Allreduce);
	return res;
}

int ear_mpic_Alltoall(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    debug(">> C Alltoall...............");
    before_mpi(Alltoall, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    debug("<< C Alltoall...............");
    after_mpi(Alltoall);
	return res;
}

int ear_mpic_Alltoallv(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *sdispls, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
    debug(">> C Alltoallv...............");
    before_mpi(Alltoallv, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
    debug("<< C Alltoallv...............");
    after_mpi(Alltoallv);
	return res;
}

int ear_mpic_Barrier(MPI_Comm comm)
{
    debug(">> C Barrier...............");
    before_mpi(Barrier, (p2i)comm,0);
    int res = next_mpic.Barrier(comm);
    debug("<< C Barrier...............");
    after_mpi(Barrier);
	return res;
}

int ear_mpic_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    debug(">> C Bcast...............");
    before_mpi(Bcast, (p2i)comm,0);
    int res = next_mpic.Bcast(buffer, count, datatype, root, comm);
    debug("<< C Bcast...............");
    after_mpi(Bcast);
	return res;
}

int ear_mpic_Bsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    debug(">> C Bsend...............");
    before_mpi(Bsend, (p2i)buf,(p2i)dest);
    int res = next_mpic.Bsend(buf, count, datatype, dest, tag, comm);
    debug("<< C Bsend...............");
    after_mpi(Bsend);
	return res;
}

int ear_mpic_Bsend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Bsend_init...............");
    before_mpi(Bsend_init, (p2i)buf,(p2i)dest);
    int res = next_mpic.Bsend_init(buf, count, datatype, dest, tag, comm, request);
    debug("<< C Bsend_init...............");
    after_mpi(Bsend_init);
	return res;
}

int ear_mpic_Cancel(MPI_Request *request)
{
    debug(">> C Cancel...............");
    before_mpi(Cancel, (p2i)request,(p2i)0);
    int res = next_mpic.Cancel(request);
    debug("<< C Cancel...............");
    after_mpi(Cancel);
	return res;
}

int ear_mpic_Cart_create(MPI_Comm comm_old, int ndims, MPI3_CONST int dims[], MPI3_CONST int periods[], int reorder, MPI_Comm *comm_cart)
{
    debug(">> C Cart_create...............");
    before_mpi(Cart_create, (p2i)ndims,(p2i)comm_cart);
    int res = next_mpic.Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart);
    debug("<< C Cart_create...............");
    after_mpi(Cart_create);
	return res;
}

int ear_mpic_Cart_sub(MPI_Comm comm, MPI3_CONST int remain_dims[], MPI_Comm *newcomm)
{
    debug(">> C Cart_sub...............");
    before_mpi(Cart_sub, (p2i)remain_dims,(p2i)newcomm);
    int res = next_mpic.Cart_sub(comm, remain_dims, newcomm);
    debug("<< C Cart_sub...............");
    after_mpi(Cart_sub);
	return res;
}

int ear_mpic_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    debug(">> C Comm_create...............");
    before_mpi(Comm_create, (p2i)group,(p2i)newcomm);
    int res = next_mpic.Comm_create(comm, group, newcomm);
    debug("<< C Comm_create...............");
    after_mpi(Comm_create);
	return res;
}

int ear_mpic_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    debug(">> C Comm_dup...............");
    before_mpi(Comm_dup, (p2i)newcomm,(p2i)0);
    int res = next_mpic.Comm_dup(comm, newcomm);
    debug("<< C Comm_dup...............");
    after_mpi(Comm_dup);
	return res;
}

int ear_mpic_Comm_free(MPI_Comm *comm)
{
    debug(">> C Comm_free...............");
    before_mpi(Comm_free, (p2i)comm,(p2i)0);
    int res = next_mpic.Comm_free(comm);
    debug("<< C Comm_free...............");
    after_mpi(Comm_free);
	return res;
}

int ear_mpic_Comm_rank(MPI_Comm comm, int *rank)
{
    debug(">> C Comm_rank...............");
    before_mpi(Comm_rank, (p2i)comm,(p2i)rank);
    int res = next_mpic.Comm_rank(comm, rank);
    debug("<< C Comm_rank...............");
    after_mpi(Comm_rank);
	return res;
}

int ear_mpic_Comm_size(MPI_Comm comm, int *size)
{
    debug(">> C Comm_size...............");
    before_mpi(Comm_size, (p2i)size,(p2i)0);
    int res = next_mpic.Comm_size(comm, size);
    debug("<< C Comm_size...............");
    after_mpi(Comm_size);
	return res;
}

int ear_mpic_Comm_spawn(MPI3_CONST char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    debug(">> C Comm_spawn...............");
    before_mpi(Comm_spawn, (p2i)command,(p2i)info);
    int res = next_mpic.Comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes);
    debug("<< C Comm_spawn...............");
    after_mpi(Comm_spawn);
	return res;
}

int ear_mpic_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], MPI3_CONST int array_of_maxprocs[], MPI3_CONST MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    debug(">> C Comm_spawn_multiple...............");
    before_mpi(Comm_spawn_multiple, (p2i)array_of_commands,(p2i)array_of_info);
    int res = next_mpic.Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes);
    debug("<< C Comm_spawn_multiple...............");
    after_mpi(Comm_spawn_multiple);
	return res;
}

int ear_mpic_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    debug(">> C Comm_split...............");
    before_mpi(Comm_split, (p2i)key,(p2i)newcomm);
    int res = next_mpic.Comm_split(comm, color, key, newcomm);
    debug("<< C Comm_split...............");
    after_mpi(Comm_split);
	return res;
}

int ear_mpic_File_close(MPI_File *fh)
{
    debug(">> C File_close...............");
    before_mpi(File_close, (p2i)fh,(p2i)0);
    int res = next_mpic.File_close(fh);
    debug("<< C File_close...............");
    after_mpi(File_close);
	return res;
}

int ear_mpic_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    debug(">> C File_read...............");
    before_mpi(File_read, (p2i)buf,(p2i)datatype);
    int res = next_mpic.File_read(fh, buf, count, datatype, status);
    debug("<< C File_read...............");
    after_mpi(File_read);
	return res;
}

int ear_mpic_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    debug(">> C File_read_all...............");
    before_mpi(File_read_all, (p2i)buf,(p2i)datatype);
    int res = next_mpic.File_read_all(fh, buf, count, datatype, status);
    debug("<< C File_read_all...............");
    after_mpi(File_read_all);
	return res;
}

int ear_mpic_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    debug(">> C File_read_at...............");
    before_mpi(File_read_at, (p2i)buf,(p2i)datatype);
    int res = next_mpic.File_read_at(fh, offset, buf, count, datatype, status);
    debug("<< C File_read_at...............");
    after_mpi(File_read_at);
	return res;
}

int ear_mpic_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    debug(">> C File_read_at_all...............");
    before_mpi(File_read_at_all, (p2i)buf,(p2i)datatype);
    int res = next_mpic.File_read_at_all(fh, offset, buf, count, datatype, status);
    debug("<< C File_read_at_all...............");
    after_mpi(File_read_at_all);
	return res;
}

int ear_mpic_File_write(MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    debug(">> C File_write...............");
    before_mpi(File_write, (p2i)buf,(p2i)datatype);
    int res = next_mpic.File_write(fh, buf, count, datatype, status);
    debug("<< C File_write...............");
    after_mpi(File_write);
	return res;
}

int ear_mpic_File_write_all(MPI_File fh, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    debug(">> C File_write_all...............");
    before_mpi(File_write_all, (p2i)buf,(p2i)datatype);
    int res = next_mpic.File_write_all(fh, buf, count, datatype, status);
    debug("<< C File_write_all...............");
    after_mpi(File_write_all);
	return res;
}

int ear_mpic_File_write_at(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    debug(">> C File_write_at...............");
    before_mpi(File_write_at, (p2i)buf,(p2i)datatype);
    int res = next_mpic.File_write_at(fh, offset, buf, count, datatype, status);

    debug("<< C File_write_at...............");
    after_mpi(File_write_at);
	return res;
}

int ear_mpic_File_write_at_all(MPI_File fh, MPI_Offset offset, MPI3_CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    debug(">> C File_write_at_all...............");
    before_mpi(File_write_at_all, (p2i)buf,(p2i)datatype);
    int res = next_mpic.File_write_at_all(fh, offset, buf, count, datatype, status);
    debug("<< C File_write_at_all...............");
    after_mpi(File_write_at_all);
	return res;
}

int ear_mpic_Finalize(void)
{
    debug(">> C Finalize...............");
    before_finalize();
    int res = next_mpic.Finalize();
    debug("<< C Finalize...............");
    after_finalize();
	return res;
}

int ear_mpic_Gather(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    debug(">> C Gather...............");
    before_mpi(Gather, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    debug("<< C Gather...............");
    after_mpi(Gather);
	return res;
}

int ear_mpic_Gatherv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int *recvcounts, MPI3_CONST int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    debug(">> C Gatherv...............");
    before_mpi(Gatherv, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
	debug("<< C Gatherv...............");
    after_mpi(Gatherv);
	return res;
}

int ear_mpic_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    debug(">> C Get...............");
    before_mpi(Get, (p2i)origin_addr,(p2i)origin_datatype);
    int res = next_mpic.Get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    debug("<< C Get...............");
    after_mpi(Get);
	return res;
}

int ear_mpic_Ibsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Ibsend...............");
    before_mpi(Ibsend, (p2i)buf,(p2i)datatype);
    int res = next_mpic.Ibsend(buf, count, datatype, dest, tag, comm, request);
    debug("<< C Ibsend...............");
    after_mpi(Ibsend);
	return res;
}

int ear_mpic_Init(int *argc, char ***argv)
{
    debug(">> C Init...............");
    before_init();
    int res = next_mpic.Init(argc, argv);
    debug("<< C Init...............");
    after_init();
	return res;
}

int ear_mpic_Init_thread(int *argc, char ***argv, int required, int *provided)
{
    debug(">> C Init_thread...............");
    before_init();
    int res = next_mpic.Init_thread(argc, argv, required, provided);
    debug("<< C Init_thread...............");
    after_init();
	return res;
}

int ear_mpic_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm)
{
    debug(">> C Intercomm_create...............");
    before_mpi(Intercomm_create, (p2i)local_leader,(p2i)remote_leader);
    int res = next_mpic.Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm);
    debug("<< C Intercomm_create...............");
    after_mpi(Intercomm_create);
	return res;
}

int ear_mpic_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm)
{
    debug(">> C Intercomm_merge...............");
    before_mpi(Intercomm_merge, (p2i)newintracomm,(p2i)0);
    int res = next_mpic.Intercomm_merge(intercomm, high, newintracomm);
    debug("<< C Intercomm_merge...............");
    after_mpi(Intercomm_merge);
	return res;
}

int ear_mpic_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
    debug(">> C Iprobe...............");
    before_mpi(Iprobe, (p2i)flag,(p2i)status);
    int res = next_mpic.Iprobe(source, tag, comm, flag, status);
    debug("<< C Iprobe...............");
    after_mpi(Iprobe);
	return res;
}

int ear_mpic_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Irecv...............");
    before_mpi(Irecv, (p2i)buf,(p2i)request);
    int res = next_mpic.Irecv(buf, count, datatype, source, tag, comm, request);
    debug("<< C Irecv...............");
    after_mpi(Irecv);
	return res;
}

int ear_mpic_Irsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Irsend...............");
    before_mpi(Irsend, (p2i)buf,(p2i)dest);
    int res = next_mpic.Irsend(buf, count, datatype, dest, tag, comm, request);
    debug("<< C Irsend...............");
    after_mpi(Irsend);
	return res;
}

int ear_mpic_Isend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Isend...............");
    before_mpi(Isend, (p2i)buf,(p2i)dest);
    int res = next_mpic.Isend(buf, count, datatype, dest, tag, comm, request);
    debug("<< C Isend...............");
    after_mpi(Isend);
	return res;
}

int ear_mpic_Issend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Issend...............");
    before_mpi(Issend, (p2i)buf,(p2i)dest);
    int res = next_mpic.Issend(buf, count, datatype, dest, tag, comm, request);
    debug("<< C Issend...............");
    after_mpi(Issend);
	return res;
}

int ear_mpic_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    debug(">> C Probe...............");
    before_mpi(Probe, (p2i)source,(p2i)0);
    int res = next_mpic.Probe(source, tag, comm, status);
    debug("<< C Probe...............");
    after_mpi(Probe);
	return res;
}

int ear_mpic_Put(MPI3_CONST void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win)
{
    debug(">> C Put...............");
    before_mpi(Put, (p2i)origin_addr,(p2i)0);
    int res = next_mpic.Put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);
    debug("<< C Put...............");
    after_mpi(Put);
	return res;
}

int ear_mpic_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    debug(">> C Recv...............");
    before_mpi(Recv, (p2i)buf,(p2i)source);
    int res = next_mpic.Recv(buf, count, datatype, source, tag, comm, status);
    debug("<< C Recv...............");
    after_mpi(Recv);
	return res;
}

int ear_mpic_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Recv_init...............");
    before_mpi(Recv_init, (p2i)buf,(p2i)source);
    int res = next_mpic.Recv_init(buf, count, datatype, source, tag, comm, request);
    debug("<< C Recv_init...............");
    after_mpi(Recv_init);
	return res;
}

int ear_mpic_Reduce(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    debug(">> C Reduce...............");
    before_mpi(Reduce, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
    debug("<< C Reduce...............");
    after_mpi(Reduce);
	return res;
}

int ear_mpic_Reduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    debug(">> C Reduce_scatter...............");
    before_mpi(Reduce_scatter, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm);
    debug("<< C Reduce_scatter...............");
    after_mpi(Reduce_scatter);
	return res;
}

int ear_mpic_Request_free(MPI_Request *request)
{
    debug(">> C Request_free...............");
    before_mpi(Request_free, (p2i)request,(p2i)0);
    int res = next_mpic.Request_free(request);
    debug("<< C Request_free...............");
    after_mpi(Request_free);
	return res;
}

int ear_mpic_Request_get_status(MPI_Request request, int *flag, MPI_Status *status)
{
    debug(">> C Request_get_status...............");
    before_mpi(Request_get_status, (p2i)request,(p2i)0);
    int res = next_mpic.Request_get_status(request, flag, status);
    debug("<< C Request_get_status...............");
    after_mpi(Request_get_status);
	return res;
}

int ear_mpic_Rsend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    debug(">> C Rsend...............");
    before_mpi(Rsend, (p2i)buf,(p2i)dest);
    int res = next_mpic.Rsend(buf, count, datatype, dest, tag, comm);
    debug("<< C Rsend...............");
    after_mpi(Rsend);
	return res;
}

int ear_mpic_Rsend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Rsend_init...............");
    before_mpi(Rsend_init, (p2i)buf,(p2i)0);
    int res = next_mpic.Rsend_init(buf, count, datatype, dest, tag, comm, request);
    debug("<< C Rsend_init...............");
    after_mpi(Rsend_init);
	return res;
}

int ear_mpic_Scan(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    debug(">> C Scan...............");
    before_mpi(Scan, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Scan(sendbuf, recvbuf, count, datatype, op, comm);
    debug("<< C Scan...............");
    after_mpi(Scan);
	return res;
}

int ear_mpic_Scatter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    debug(">> C Scatter...............");
    before_mpi(Scatter, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    debug("<< C Scatter...............");
    after_mpi(Scatter);
	return res;
}

int ear_mpic_Scatterv(MPI3_CONST void *sendbuf, MPI3_CONST int *sendcounts, MPI3_CONST int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    debug(">> C Scatterv...............");
    before_mpi(Scatterv, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
    debug("<< C Scatterv...............");
    after_mpi(Scatterv);
	return res;
}

int ear_mpic_Send(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    debug(">> C Send...............");
    before_mpi(Send, (p2i)buf,(p2i)dest);
    int res = next_mpic.Send(buf, count, datatype, dest, tag, comm);
    debug("<< C Send...............");
    after_mpi(Send);
	return res;
}

int ear_mpic_Send_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Send_init...............");
    before_mpi(Send_init, (p2i)buf,(p2i)dest);
    int res = next_mpic.Send_init(buf, count, datatype, dest, tag, comm, request);
    debug("<< C Send_init...............");
    after_mpi(Send_init);
	return res;
}

int ear_mpic_Sendrecv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    debug(">> C Sendrecv...............");
    before_mpi(Sendrecv, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
    debug("<< C Sendrecv...............");
    after_mpi(Sendrecv);
	return res;
}

int ear_mpic_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    debug(">> C Sendrecv_replace...............");
    before_mpi(Sendrecv_replace, (p2i)buf,(p2i)dest);
    int res = next_mpic.Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
    debug("<< C Sendrecv_replace...............");
    after_mpi(Sendrecv_replace);
	return res;
}

int ear_mpic_Ssend(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    debug(">> C Ssend...............");
    before_mpi(Ssend, (p2i)buf,(p2i)dest);
    int res = next_mpic.Ssend(buf, count, datatype, dest, tag, comm);
    debug("<< C Ssend...............");
    after_mpi(Ssend);
	return res;
}

int ear_mpic_Ssend_init(MPI3_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Ssend_init...............");
    before_mpi(Ssend_init, (p2i)buf,(p2i)dest);
    int res = next_mpic.Ssend_init(buf, count, datatype, dest, tag, comm, request);
    debug("<< C Ssend_init...............");
    after_mpi(Ssend_init);
	return res;
}

int ear_mpic_Start(MPI_Request *request)
{
    debug(">> C Start...............");
    before_mpi(Start, (p2i)request,(p2i)0);
    int res = next_mpic.Start(request);
    debug("<< C Start...............");
    after_mpi(Start);
	return res;
}

int ear_mpic_Startall(int count, MPI_Request array_of_requests[])
{
    debug(">> C Startall...............");
    before_mpi(Startall, (p2i)array_of_requests,(p2i)0);
    int res = next_mpic.Startall(count, array_of_requests);
    debug("<< C Startall...............");
    after_mpi(Startall);
	return res;
}

int ear_mpic_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
    debug(">> C Test...............");
    before_mpi(Test, (p2i)request,(p2i)0);
    int res = next_mpic.Test(request, flag, status);
    debug("<< C Test...............");
    after_mpi(Test);
	return res;
}

int ear_mpic_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[])
{
    debug(">> C Testall...............");
    before_mpi(Testall, (p2i)array_of_requests,(p2i)array_of_statuses);
    int res = next_mpic.Testall(count, array_of_requests, flag, array_of_statuses);
    debug("<< C Testall...............");
    after_mpi(Testall);
	return res;
}

int ear_mpic_Testany(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status)
{
    debug(">> C Testany...............");
    before_mpi(Testany, (p2i)array_of_requests,(p2i)flag);
    int res = next_mpic.Testany(count, array_of_requests, indx, flag, status);
    debug("<< C Testany...............");
    after_mpi(Testany);
	return res;
}

int ear_mpic_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    debug(">> C Testsome...............");
    before_mpi(Testsome, (p2i)array_of_requests,(p2i)outcount);
    int res = next_mpic.Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    debug("<< C Testsome...............");
    after_mpi(Testsome);
	return res;
}

int ear_mpic_Wait(MPI_Request *request, MPI_Status *status)
{
    debug(">> C Wait...............");
    before_mpi(Wait, (p2i)request,(p2i)0);
    int res = next_mpic.Wait(request, status);
    debug("<< C Wait...............");
    after_mpi(Wait);
	return res;
}

int ear_mpic_Waitall(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses)
{
    debug(">> C Waitall...............");
    before_mpi(Waitall, (p2i)array_of_requests,(p2i)array_of_statuses);
    int res = next_mpic.Waitall(count, array_of_requests, array_of_statuses);
    debug("<< C Waitall...............");
    after_mpi(Waitall);
	return res;
}

int ear_mpic_Waitany(int count, MPI_Request *requests, int *index, MPI_Status *status)
{
    debug(">> C Waitany...............");
    before_mpi(Waitany, (p2i)requests,(p2i)index);
    int res = next_mpic.Waitany(count, requests, index, status);
    debug("<< C Waitany...............");
    after_mpi(Waitany);
	return res;
}

int ear_mpic_Waitsome(int incount, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
    debug(">> C Waitsome...............");
    before_mpi(Waitsome, (p2i)array_of_requests,(p2i)outcount);
    int res = next_mpic.Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    debug("<< C Waitsome...............");
    after_mpi(Waitsome);
	return res;
}

int ear_mpic_Win_complete(MPI_Win win)
{
    debug(">> C Win_complete...............");
    before_mpi(Win_complete, (p2i)win,(p2i)0);
    int res = next_mpic.Win_complete(win);
    debug("<< C Win_complete...............");
    after_mpi(Win_complete);
	return res;
}

int ear_mpic_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    debug(">> C Win_create...............");
    before_mpi(Win_create, (p2i)base,(p2i)info);
    int res = next_mpic.Win_create(base, size, disp_unit, info, comm, win);
    debug("<< C Win_create...............");
    after_mpi(Win_create);
	return res;
}

int ear_mpic_Win_fence(int assert, MPI_Win win)
{
    debug(">> C Win_fence...............");
    before_mpi(Win_fence, (p2i)win,(p2i)0);
    int res = next_mpic.Win_fence(assert, win);
    debug("<< C Win_fence...............");
    after_mpi(Win_fence);
	return res;
}

int ear_mpic_Win_free(MPI_Win *win)
{
    debug(">> C Win_free...............");
    before_mpi(Win_free, (p2i)win,(p2i)0);
    int res = next_mpic.Win_free(win);
    debug("<< C Win_free...............");
    after_mpi(Win_free);
	return res;
}

int ear_mpic_Win_post(MPI_Group group, int assert, MPI_Win win)
{
    debug(">> C Win_post...............");
    before_mpi(Win_post, (p2i)win,(p2i)0);
    int res = next_mpic.Win_post(group, assert, win);
    debug("<< C Win_post...............");
    after_mpi(Win_post);
	return res;
}

int ear_mpic_Win_start(MPI_Group group, int assert, MPI_Win win)
{
    debug(">> C Win_start...............");
    before_mpi(Win_start, (p2i)win,(p2i)0);
    int res = next_mpic.Win_start(group, assert, win);
    debug("<< C Win_start...............");
    after_mpi(Win_start);
	return res;
}

int ear_mpic_Win_wait(MPI_Win win)
{
    debug(">> C Win_wait...............");
    before_mpi(Win_wait, (p2i)win,(p2i)0);
    int res = next_mpic.Win_wait(win);
    debug("<< C Win_wait...............");
    after_mpi(Win_wait);
	return res;
}

#if MPI_VERSION >= 3
int ear_mpic_Iallgather(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Iallgather...............");
    before_mpi(Iallgather, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
    debug("<< C Iallgather...............");
    after_mpi(Iallgather);
	return res;
}

int ear_mpic_Iallgatherv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Iallgatherv...............");
    before_mpi(Iallgatherv, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
    debug("<< C Iallgatherv...............");
    after_mpi(Iallgatherv);
	return res;
}

int ear_mpic_Iallreduce(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Iallreduce...............");
    before_mpi(Iallreduce, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, request);
    debug("<< C Iallreduce...............");
    after_mpi(Iallreduce);
	return res;
}

int ear_mpic_Ialltoall(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Ialltoall...............");
    before_mpi(Ialltoall, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
    debug("<< C Ialltoall...............");
    after_mpi(Ialltoall);
	return res;
}

int ear_mpic_Ialltoallv(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Ialltoallv...............");
    before_mpi(Ialltoallv, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
    debug("<< C Ialltoallv...............");
    after_mpi(Ialltoallv);
	return res;
}

int ear_mpic_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Ibarrier...............");
    before_mpi(Ibarrier, (p2i)request,(p2i)0);
	int res = next_mpic.Ibarrier(comm, request);
    debug("<< C Ibarrier...............");
    after_mpi(Ibarrier);
	return res;
}

int ear_mpic_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Ibcast...............");
    before_mpi(Ibcast, (p2i)buffer,(p2i)request);
	int res = next_mpic.Ibcast(buffer, count, datatype, root, comm, request);
    debug("<< C Ibcast...............");
    after_mpi(Ibcast);
	return res;
}

int ear_mpic_Igather(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Igather...............");
    before_mpi(Igather, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
    debug("<< C Igather...............");
    after_mpi(Igather);
	return res;
}

int ear_mpic_Igatherv(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, MPI3_CONST int recvcounts[], MPI3_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Igatherv...............");
    before_mpi(Igatherv, (p2i)sendbuf,(p2i)recvbuf);
    int res = next_mpic.Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request);
    debug("<< C Igatherv...............");
    after_mpi(Igatherv);
	return res;
}

int ear_mpic_Ireduce(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Ireduce...............");
    before_mpi(Ireduce, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request);
    debug("<< C Ireduce...............");
    after_mpi(Ireduce);
	return res;
}

int ear_mpic_Ireduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Ireduce_scatter...............");
    before_mpi(Ireduce_scatter, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, request);
    debug("<< C Ireduce_scatter...............");
    after_mpi(Ireduce_scatter);
	return res;
}

int ear_mpic_Iscan(MPI3_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Iscan...............");
    before_mpi(Iscan, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Iscan(sendbuf, recvbuf, count, datatype, op, comm, request);
    debug("<< C Iscan...............");
    after_mpi(Iscan);
	return res;
}

int ear_mpic_Iscatter(MPI3_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Iscatter...............");
    before_mpi(Iscatter, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
    debug("<< C Iscatter...............");
    after_mpi(Iscatter);
	return res;
}

int ear_mpic_Iscatterv(MPI3_CONST void *sendbuf, MPI3_CONST int sendcounts[], MPI3_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    debug(">> C Iscatterv...............");
    before_mpi(Iscatterv, (p2i)sendbuf,(p2i)recvbuf);
	int res = next_mpic.Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
    debug("<< C Iscatterv...............");
    after_mpi(Iscatterv);
	return res;
}
#endif
