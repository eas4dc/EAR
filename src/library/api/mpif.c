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
#include <library/api/mpif.h>
#include <library/api/ear_mpi.h>

static mpif_t next_mpif;

void ear_mpif_setnext(mpif_t *_next_mpif)
{
	debug(">> F setnext...............");
	memcpy(&next_mpif, _next_mpif, sizeof(mpif_t));
	debug("<< F setnext...............");
}

void ear_mpif_Allgather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Allgather...............");
    before_mpi(Allgather, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
    debug("<< F Allgather...............");
    after_mpi(Allgather);
}

void ear_mpif_Allgatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Allgatherv...............");
    before_mpi(Allgatherv, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierror);
    debug("<< F Allgatherv...............");
    after_mpi(Allgatherv);
}

void ear_mpif_Allreduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Allreduce...............");
    before_mpi(Allreduce, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.allreduce(sendbuf, recvbuf, count, datatype, op, comm, ierror);
    debug("<< F Allreduce...............");
    after_mpi(Allreduce);
}

void ear_mpif_Alltoall(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Alltoall...............");
    before_mpi(Alltoall, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
    debug("<< F Alltoall...............");
    after_mpi(Alltoall);
}

void ear_mpif_Alltoallv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Alltoallv...............");
    before_mpi(Alltoallv, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, ierror);
    debug("<< F Alltoallv...............");
    after_mpi(Alltoallv);
}

void ear_mpif_Barrier(MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Barrier...............");
    before_mpi(Barrier, (p2i)comm, (p2i)ierror);
    next_mpif.barrier(comm, ierror);
    debug("<< F Barrier...............");
    after_mpi(Barrier);
}

void ear_mpif_Bcast(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Bcast...............");
    before_mpi(Bcast, (p2i)buffer, 0);
    next_mpif.bcast(buffer, count, datatype, root, comm, ierror);
    debug("<< F Bcast...............");
    after_mpi(Bcast);
}

void ear_mpif_Bsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Bsend...............");
    before_mpi(Bsend, (p2i)buf, (p2i)dest);
    next_mpif.bsend(buf, count, datatype, dest, tag, comm, ierror);
    debug("<< F Bsend...............");
    after_mpi(Bsend);
}

void ear_mpif_Bsend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Bsend_init...............");
    before_mpi(Bsend_init, (p2i)buf, (p2i)dest);
    next_mpif.bsend_init(buf, count, datatype, dest, tag, comm, request, ierror);
    debug("<< F Bsend_init...............");
    after_mpi(Bsend_init);
}

void ear_mpif_Cancel(MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Cancel...............");
    before_mpi(Cancel, (p2i)request, 0);
    next_mpif.cancel(request, ierror);
    debug("<< F Cancel...............");
    after_mpi(Cancel);
}

void ear_mpif_Cart_create(MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror)
{
    debug(">> F Cart_create...............");
    before_mpi(Cart_create, (p2i)ndims, 0);
    next_mpif.cart_create(comm_old, ndims, dims, periods, reorder, comm_cart, ierror);
    debug("<< F Cart_create...............");
    after_mpi(Cart_create);
}

void ear_mpif_Cart_sub(MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror)
{
    debug(">> F Cart_sub...............");
    before_mpi(Cart_sub, (p2i)remain_dims, 0);
    next_mpif.cart_sub(comm, remain_dims, comm_new, ierror);
    debug("<< F Cart_sub...............");
    after_mpi(Cart_sub);
}

void ear_mpif_Comm_create(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror)
{
    debug(">> F Comm_create...............");
    before_mpi(Comm_create, (p2i)group, 0);
    next_mpif.comm_create(comm, group, newcomm, ierror);
    debug("<< F Comm_create...............");
    after_mpi(Comm_create);
}

void ear_mpif_Comm_dup(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror)
{
    debug(">> F Comm_dup...............");
    before_mpi(Comm_dup, (p2i)newcomm, 0);
    next_mpif.comm_dup(comm, newcomm, ierror);
    debug("<< F Comm_dup...............");
    after_mpi(Comm_dup);
}

void ear_mpif_Comm_free(MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Comm_free...............");
    before_mpi(Comm_free, (p2i)comm, 0);
    next_mpif.comm_free(comm, ierror);
    debug("<< F Comm_free...............");
    after_mpi(Comm_free);
}

void ear_mpif_Comm_rank(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror)
{
    debug(">> F Comm_rank...............");
    before_mpi(Comm_rank, (p2i)rank, 0);
    next_mpif.comm_rank(comm, rank, ierror);
    debug("<< F Comm_rank...............");
    after_mpi(Comm_rank);
}

void ear_mpif_Comm_size(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror)
{
    debug(">> F Comm_size...............");
    before_mpi(Comm_size, (p2i)size, 0);
    next_mpif.comm_size(comm, size, ierror);
    debug("<< F Comm_size...............");
    after_mpi(Comm_size);
}

void ear_mpif_Comm_spawn(MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror)
{
    debug(">> F Comm_spawn...............");
    before_mpi(Comm_spawn, (p2i)command, 0);
    next_mpif.comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, ierror);
    debug("<< F Comm_spawn...............");
    after_mpi(Comm_spawn);
}

void ear_mpif_Comm_spawn_multiple(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror)
{
    debug(">> F Comm_spawn_multiple...............");
    before_mpi(Comm_spawn_multiple, (p2i)array_of_commands, 0);
    next_mpif.comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes, ierror);
    debug("<< F Comm_spawn_multiple...............");
    after_mpi(Comm_spawn_multiple);
}

void ear_mpif_Comm_split(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror)
{
    debug(">> F Comm_split...............");
    before_mpi(Comm_split, (p2i)comm, 0);
    next_mpif.comm_split(comm, color, key, newcomm, ierror);
    debug("<< F Comm_split...............");
    after_mpi(Comm_split);
}

void ear_mpif_File_close(MPI_File *fh, MPI_Fint *ierror)
{
    debug(">> F File_close...............");
    before_mpi(File_close, (p2i)fh, 0);
    next_mpif.file_close(fh, ierror);
    debug("<< F File_close...............");
    after_mpi(File_close);
}

void ear_mpif_File_read(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
    debug(">> F File_read...............");
    before_mpi(File_read, (p2i)buf, 0);
    next_mpif.file_read(fh, buf, count, datatype, status, ierror);
    debug("<< F File_read...............");
    after_mpi(File_read);
}

void ear_mpif_File_read_all(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
    debug(">> F File_read_all...............");
    before_mpi(File_read_all, (p2i)buf, 0);
    next_mpif.file_read_all(fh, buf, count, datatype, status, ierror);
    debug("<< F File_read_all...............");
    after_mpi(File_read_all);
}

void ear_mpif_File_read_at(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
    debug(">> F File_read_at...............");
    before_mpi(File_read_at, (p2i)buf, 0);
    next_mpif.file_read_at(fh, offset, buf, count, datatype, status, ierror);
    debug("<< F File_read_at...............");
    after_mpi(File_read_at);
}

void ear_mpif_File_read_at_all(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
    debug(">> F File_read_at_all...............");
    before_mpi(File_read_at_all, (p2i)buf, 0);
    next_mpif.file_read_at_all(fh, offset, buf, count, datatype, status, ierror);
    debug("<< F File_read_at_all...............");
    after_mpi(File_read_at_all);
}

void ear_mpif_File_write(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
    debug(">> F File_write...............");
    before_mpi(File_write, (p2i)buf, 0);
    next_mpif.file_write(fh, buf, count, datatype, status, ierror);
    debug("<< F File_write...............");
    after_mpi(File_write);
}

void ear_mpif_File_write_all(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
    debug(">> F File_write_all...............");
    before_mpi(File_write_all, (p2i)buf, 0);
    next_mpif.file_write_all(fh, buf, count, datatype, status, ierror);
    debug("<< F File_write_all...............");
    after_mpi(File_write_all);
}

void ear_mpif_File_write_at(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
    debug(">> F File_write_at...............");
    before_mpi(File_write_at, (p2i)buf, 0);
    next_mpif.file_write_at(fh, offset, buf, count, datatype, status, ierror);
    debug("<< F File_write_at...............");
    after_mpi(File_write_at);
}

void ear_mpif_File_write_at_all(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
    debug(">> F File_write_at_all...............");
    before_mpi(File_write_at_all, (p2i)buf, 0);
    next_mpif.file_write_at_all(fh, offset, buf, count, datatype, status, ierror);
    debug("<< F File_write_at_all...............");
    after_mpi(File_write_at_all);
}

void ear_mpif_Finalize(MPI_Fint *ierror)
{
    debug(">> F Finalize...............");
    before_finalize();
    next_mpif.finalize(ierror);
    debug("<< F Finalize...............");
    after_finalize();
}

void ear_mpif_Gather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Gather...............");
    before_mpi(Gather, (p2i)sendbuf, 0);
    next_mpif.gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
    debug("<< F Gather...............");
    after_mpi(Gather);
}

void ear_mpif_Gatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Gatherv...............");
    before_mpi(Gatherv, (p2i)sendbuf, 0);
    next_mpif.gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierror);
    debug("<< F Gatherv...............");
    after_mpi(Gatherv);
}

void ear_mpif_Get(MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Get...............");
    before_mpi(Get, (p2i)origin_addr, 0);
    next_mpif.get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, ierror);
    debug("<< F Get...............");
    after_mpi(Get);
}

void ear_mpif_Ibsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Ibsend...............");
    before_mpi(Ibsend, (p2i)buf, (p2i)dest);
    next_mpif.ibsend(buf, count, datatype, dest, tag, comm, request, ierror);
    debug("<< F Ibsend...............");
    after_mpi(Ibsend);
}

void ear_mpif_Init(MPI_Fint *ierror)
{
    debug(">> F Init...............");
    before_init();
    next_mpif.init(ierror);
    debug("<< F Init...............");
    after_init();
}

void ear_mpif_Init_thread(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror)
{
    debug(">> F Init_thread...............");
    before_init();
    next_mpif.init_thread(required, provided, ierror);
    debug("<< F Init_thread...............");
    after_init();
}

void ear_mpif_Intercomm_create(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror)
{
    debug(">> F Intercomm_create...............");
    before_mpi(Intercomm_create,0 , 0);
    next_mpif.intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm, ierror);
    debug("<< F Intercomm_create...............");
    after_mpi(Intercomm_create);
}

void ear_mpif_Intercomm_merge(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror)
{
    debug(">> F Intercomm_merge...............");
    before_mpi(Intercomm_merge, 0, 0);
    next_mpif.intercomm_merge(intercomm, high, newintracomm, ierror);
    debug("<< F Intercomm_merge...............");
    after_mpi(Intercomm_merge);
}

void ear_mpif_Iprobe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Iprobe...............");
    before_mpi(Iprobe, (p2i)source, 0);
    next_mpif.iprobe(source, tag, comm, flag, status, ierror);
    debug("<< F Iprobe...............");
    after_mpi(Iprobe);
}

void ear_mpif_Irecv(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Irecv...............");
    before_mpi(Irecv, (p2i)buf, (p2i)source);
    next_mpif.irecv(buf, count, datatype, source, tag, comm, request, ierror);
    debug("<< F Irecv...............");
    after_mpi(Irecv);
}

void ear_mpif_Irsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Irsend...............");
    before_mpi(Irsend, (p2i)buf, (p2i)dest);
    next_mpif.irsend(buf, count, datatype, dest, tag, comm, request, ierror);
    debug("<< F Irsend...............");
    after_mpi(Irsend);
}

void ear_mpif_Isend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Isend...............");
    before_mpi(Isend, (p2i)buf, (p2i)dest);
    next_mpif.isend(buf, count, datatype, dest, tag, comm, request, ierror);
    debug("<< F Isend...............");
    after_mpi(Isend);
}

void ear_mpif_Issend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Issend...............");
    before_mpi(Issend, (p2i)buf, (p2i)dest);
    next_mpif.issend(buf, count, datatype, dest, tag, comm, request, ierror);
    debug("<< F Issend...............");
    after_mpi(Issend);
}

void ear_mpif_Probe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Probe...............");
    before_mpi(Probe, (p2i)source, 0);
    next_mpif.probe(source, tag, comm, status, ierror);
    debug("<< F Probe...............");
    after_mpi(Probe);
}

void ear_mpif_Put(MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Put...............");
    before_mpi(Put, (p2i)origin_addr, 0);
    next_mpif.put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, ierror);
    debug("<< F Put...............");
    after_mpi(Put);
}

void ear_mpif_Recv(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Recv...............");
    before_mpi(Recv, (p2i)buf, (p2i)source);
    next_mpif.recv(buf, count, datatype, source, tag, comm, status, ierror);
    debug("<< F Recv...............");
    after_mpi(Recv);
}

void ear_mpif_Recv_init(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Recv_init...............");
    before_mpi(Recv_init, (p2i)buf, (p2i)source);
    next_mpif.recv_init(buf, count, datatype, source, tag, comm, request, ierror);
    debug("<< F Recv_init...............");
    after_mpi(Recv_init);
}

void ear_mpif_Reduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Reduce...............");
    before_mpi(Reduce, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.reduce(sendbuf, recvbuf, count, datatype, op, root, comm, ierror);
    debug("<< F Reduce...............");
    after_mpi(Reduce);
}

void ear_mpif_Reduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Reduce_scatter...............");
    before_mpi(Reduce_scatter, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierror);
    debug("<< F Reduce_scatter...............");
    after_mpi(Reduce_scatter);
}

void ear_mpif_Request_free(MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Request_free...............");
    before_mpi(Request_free, (p2i)request, 0);
    next_mpif.request_free(request, ierror);
    debug("<< F Request_free...............");
    after_mpi(Request_free);
}

void ear_mpif_Request_get_status(MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Request_get_status...............");
    before_mpi(Request_get_status, (p2i)request, 0);
    next_mpif.request_get_status(request, flag, status, ierror);
    debug("<< F Request_get_status...............");
    after_mpi(Request_get_status);
}

void ear_mpif_Rsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Rsend...............");
    before_mpi(Rsend, (p2i)buf, (p2i)dest);
    next_mpif.rsend(buf, count, datatype, dest, tag, comm, ierror);
    debug("<< F Rsend...............");
    after_mpi(Rsend);
}

void ear_mpif_Rsend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Rsend_init...............");
    before_mpi(Rsend_init, (p2i)buf, (p2i)dest);
    next_mpif.rsend_init(buf, count, datatype, dest, tag, comm, request, ierror);
    debug("<< F Rsend_init...............");
    after_mpi(Rsend_init);
}

void ear_mpif_Scan(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Scan...............");
    before_mpi(Scan, (p2i)sendbuf, 0);
    next_mpif.scan(sendbuf, recvbuf, count, datatype, op, comm, ierror);
    debug("<< F Scan...............");
    after_mpi(Scan);
}

void ear_mpif_Scatter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Scatter...............");
    before_mpi(Scatter, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
    debug("<< F Scatter...............");
    after_mpi(Scatter);
}

void ear_mpif_Scatterv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Scatterv...............");
    before_mpi(Scatterv, (p2i)sendbuf, 0);
    next_mpif.scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
    debug("<< F Scatterv...............");
    after_mpi(Scatterv);
}

void ear_mpif_Send(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Send...............");
    before_mpi(Send, (p2i)buf, (p2i)dest);
    next_mpif.send(buf, count, datatype, dest, tag, comm, ierror);
    debug("<< F Send...............");
    after_mpi(Send);
}

void ear_mpif_Send_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Send_init...............");
    before_mpi(Send_init, (p2i)buf, (p2i)dest);
    next_mpif.send_init(buf, count, datatype, dest, tag, comm, request, ierror);
    debug("<< F Send_init...............");
    after_mpi(Send_init);
}

void ear_mpif_Sendrecv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Sendrecv...............");
    before_mpi(Sendrecv, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status, ierror);
    debug("<< F Sendrecv...............");
    after_mpi(Sendrecv);
}

void ear_mpif_Sendrecv_replace(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Sendrecv_replace...............");
    before_mpi(Sendrecv_replace, (p2i)buf, (p2i)dest);
    next_mpif.sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status, ierror);
    debug("<< F Sendrecv_replace...............");
    after_mpi(Sendrecv_replace);
}

void ear_mpif_Ssend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Ssend...............");
    before_mpi(Ssend, (p2i)buf, (p2i)dest);
    next_mpif.ssend(buf, count, datatype, dest, tag, comm, ierror);
    debug("<< F Ssend...............");
    after_mpi(Ssend);
}

void ear_mpif_Ssend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Ssend_init...............");
    before_mpi(Ssend_init, (p2i)buf, (p2i)dest);
    next_mpif.ssend_init(buf, count, datatype, dest, tag, comm, request, ierror);
    debug("<< F Ssend_init...............");
    after_mpi(Ssend_init);
}

void ear_mpif_Start(MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Start...............");
    before_mpi(Start, (p2i)request, 0);
    next_mpif.start(request, ierror);
    debug("<< F Start...............");
    after_mpi(Start);
}

void ear_mpif_Startall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror)
{
    debug(">> F Startall...............");
    before_mpi(Startall, (p2i)count, 0);
    next_mpif.startall(count, array_of_requests, ierror);
    debug("<< F Startall...............");
    after_mpi(Startall);
}

void ear_mpif_Test(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Test...............");
    before_mpi(Test, (p2i)request, 0);
    next_mpif.test(request, flag, status, ierror);
    debug("<< F Test...............");
    after_mpi(Test);
}

void ear_mpif_Testall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
    debug(">> F Testall...............");
    before_mpi(Testall, 0, 0);
    next_mpif.testall(count, array_of_requests, flag, array_of_statuses, ierror);
    debug("<< F Testall...............");
    after_mpi(Testall);
}

void ear_mpif_Testany(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Testany...............");
    before_mpi(Testany, 0, 0);
    next_mpif.testany(count, array_of_requests, index, flag, status, ierror);
    debug("<< F Testany...............");
    after_mpi(Testany);
}

void ear_mpif_Testsome(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
    debug(">> F Testsome...............");
    before_mpi(Testsome, 0, 0);
    next_mpif.testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
    debug("<< F Testsome...............");
    after_mpi(Testsome);
}

void ear_mpif_Wait(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Wait...............");
    before_mpi(Wait, (p2i)request, 0);
    next_mpif.wait(request, status, ierror);
    debug("<< F Wait...............");
    after_mpi(Wait);
}

void ear_mpif_Waitall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
    debug(">> F Waitall...............");
    before_mpi(Waitall, (p2i)count, 0);
    next_mpif.waitall(count, array_of_requests, array_of_statuses, ierror);
    debug("<< F Waitall...............");
    after_mpi(Waitall);
}

void ear_mpif_Waitany(MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror)
{
    debug(">> F Waitany...............");
    before_mpi(Waitany, 0, 0);
    next_mpif.waitany(count, requests, index, status, ierror);
    debug("<< F Waitany...............");
    after_mpi(Waitany);
}

void ear_mpif_Waitsome(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
    debug(">> F Waitsome...............");
    before_mpi(Waitsome, 0, 0);
    next_mpif.waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
    debug("<< F Waitsome...............");
    after_mpi(Waitsome);
}

void ear_mpif_Win_complete(MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Win_complete...............");
    before_mpi(Win_complete, 0, 0);
    next_mpif.win_complete(win, ierror);
    debug("<< F Win_complete...............");
    after_mpi(Win_complete);
}

void ear_mpif_Win_create(void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Win_create...............");
    before_mpi(Win_create, 0, 0);
    next_mpif.win_create(base, size, disp_unit, info, comm, win, ierror);
    debug("<< F Win_create...............");
    after_mpi(Win_create);
}

void ear_mpif_Win_fence(MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Win_fence...............");
    before_mpi(Win_fence, 0, 0);
    next_mpif.win_fence(assert, win, ierror);
    debug("<< F Win_fence...............");
    after_mpi(Win_fence);
}

void ear_mpif_Win_free(MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Win_free...............");
    before_mpi(Win_free, 0, 0);
    next_mpif.win_free(win, ierror);
    debug("<< F Win_free...............");
    after_mpi(Win_free);
}

void ear_mpif_Win_post(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Win_post...............");
    before_mpi(Win_post, 0, 0);
    next_mpif.win_post(group, assert, win, ierror);
    debug("<< F Win_post...............");
    after_mpi(Win_post);
}

void ear_mpif_Win_start(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Win_start...............");
    before_mpi(Win_start, 0, 0);
    next_mpif.win_start(group, assert, win, ierror);
    debug("<< F Win_start...............");
    after_mpi(Win_start);
}

void ear_mpif_Win_wait(MPI_Fint *win, MPI_Fint *ierror)
{
    debug(">> F Win_wait...............");
    before_mpi(Win_wait, 0, 0);
    next_mpif.win_wait(win, ierror);
    debug("<< F Win_wait...............");
    after_mpi(Win_wait);
}

//#if MPI_VERSION >= 3
void ear_mpif_Iallgather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Iallgather...............");
    before_mpi(Iallgather, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierror);
    debug("<< F Iallgather...............");
    after_mpi(Iallgather);
}

void ear_mpif_Iallgatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Iallgatherv...............");
    before_mpi(Iallgatherv, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcount, displs, recvtype, comm, request, ierror);
    debug("<< F Iallgatherv...............");
    after_mpi(Iallgatherv);
}

void ear_mpif_Iallreduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Iallreduce...............");
    before_mpi(Iallreduce, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.iallreduce(sendbuf, recvbuf, count, datatype, op, comm, request, ierror);
    debug("<< F Iallreduce...............");
    after_mpi(Iallreduce);
}

void ear_mpif_Ialltoall(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Ialltoall...............");
    before_mpi(Ialltoall, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierror);
    debug("<< F Ialltoall...............");
    after_mpi(Ialltoall);
}

void ear_mpif_Ialltoallv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F Ialltoallv...............");
    before_mpi(Ialltoallv, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, request, comm, ierror);
    debug("<< F Ialltoallv...............");
    after_mpi(Ialltoallv);
}

void ear_mpif_Ibarrier(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Ibarrier...............");
    before_mpi(Ibarrier, (p2i)request, 0);
    next_mpif.ibarrier(comm, request, ierror);
    debug("<< F Ibarrier...............");
    after_mpi(Ibarrier);
}

void ear_mpif_Ibcast(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Ibcast...............");
    before_mpi(Ibcast, (p2i)buffer, 0);
    next_mpif.ibcast(buffer, count, datatype, root, comm, request, ierror);
    debug("<< F Ibcast...............");
    after_mpi(Ibcast);
}

void ear_mpif_Igather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Igather...............");
    before_mpi(Igather, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    debug("<< F Igather...............");
    after_mpi(Igather);
}

void ear_mpif_Igatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Igatherv...............");
    before_mpi(Igatherv, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request, ierror);
    debug("<< F Igatherv...............");
    after_mpi(Igatherv);
}

void ear_mpif_Ireduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Ireduce...............");
    before_mpi(Ireduce, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request, ierror);
    debug("<< F Ireduce...............");
    after_mpi(Ireduce);
}

void ear_mpif_Ireduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Ireduce_scatter...............");
    before_mpi(Ireduce_scatter, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, request, ierror);
    debug("<< F Ireduce_scatter...............");
    after_mpi(Ireduce_scatter);
}

void ear_mpif_Iscan(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Iscan...............");
    before_mpi(Iscan, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.iscan(sendbuf, recvbuf, count, datatype, op, comm, request, ierror);
    debug("<< F Iscan...............");
    after_mpi(Iscan);
}

void ear_mpif_Iscatter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Iscatter...............");
    before_mpi(Iscatter, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    debug("<< F Iscatter...............");
    after_mpi(Iscatter);
}

void ear_mpif_Iscatterv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F Iscatterv...............");
    before_mpi(Iscatterv, (p2i)sendbuf, (p2i)recvbuf);
    next_mpif.iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    debug("<< F Iscatterv...............");
    after_mpi(Iscatterv);
}
//#endif
