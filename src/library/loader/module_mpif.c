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

extern mpif_t ear_mpif;

void mpi_allgather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
}
void mpi_allgather_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allgather")));
void mpi_allgather__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allgather")));

void mpi_allgatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierror);
}
void mpi_allgatherv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allgatherv")));
void mpi_allgatherv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allgatherv")));

void mpi_allreduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.allreduce(sendbuf, recvbuf, count, datatype, op, comm, ierror);
}
void mpi_allreduce_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allreduce")));
void mpi_allreduce__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allreduce")));

void mpi_alltoall(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
}
void mpi_alltoall_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_alltoall")));
void mpi_alltoall__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_alltoall")));

void mpi_alltoallv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, ierror);
}
void mpi_alltoallv_ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_alltoallv")));
void mpi_alltoallv__ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_alltoallv")));

void mpi_barrier(MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.barrier(comm, ierror);
}
void mpi_barrier_ (MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_barrier")));
void mpi_barrier__ (MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_barrier")));

void mpi_bcast(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.bcast(buffer, count, datatype, root, comm, ierror);
}
void mpi_bcast_ (void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_bcast")));
void mpi_bcast__ (void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_bcast")));

void mpi_bsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.bsend(buf, count, datatype, dest, tag, comm, ierror);
}
void mpi_bsend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_bsend")));
void mpi_bsend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_bsend")));

void mpi_bsend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.bsend_init(buf, count, datatype, dest, tag, comm, request, ierror);
}
void mpi_bsend_init_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_bsend_init")));
void mpi_bsend_init__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_bsend_init")));

void mpi_cancel(MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.cancel(request, ierror);
}
void mpi_cancel_ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_cancel")));
void mpi_cancel__ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_cancel")));

void mpi_cart_create(MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror)
{
	ear_mpif.cart_create(comm_old, ndims, dims, periods, reorder, comm_cart, ierror);
}
void mpi_cart_create_ (MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror) __attribute__((alias("mpi_cart_create")));
void mpi_cart_create__ (MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror) __attribute__((alias("mpi_cart_create")));

void mpi_cart_sub(MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror)
{
	ear_mpif.cart_sub(comm, remain_dims, comm_new, ierror);
}
void mpi_cart_sub_ (MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror) __attribute__((alias("mpi_cart_sub")));
void mpi_cart_sub__ (MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror) __attribute__((alias("mpi_cart_sub")));

void mpi_comm_create(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror)
{
	ear_mpif.comm_create(comm, group, newcomm, ierror);
}
void mpi_comm_create_ (MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_create")));
void mpi_comm_create__ (MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_create")));

void mpi_comm_dup(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror)
{
	ear_mpif.comm_dup(comm, newcomm, ierror);
}
void mpi_comm_dup_ (MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_dup")));
void mpi_comm_dup__ (MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_dup")));

void mpi_comm_free(MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.comm_free(comm, ierror);
}
void mpi_comm_free_ (MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_free")));
void mpi_comm_free__ (MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_free")));

void mpi_comm_rank(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror)
{
	ear_mpif.comm_rank(comm, rank, ierror);

}
void mpi_comm_rank_ (MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror) __attribute__((alias("mpi_comm_rank")));
void mpi_comm_rank__ (MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror) __attribute__((alias("mpi_comm_rank")));

void mpi_comm_size(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror)
{
	ear_mpif.comm_size(comm, size, ierror);
}
void mpi_comm_size_ (MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror) __attribute__((alias("mpi_comm_size")));
void mpi_comm_size__ (MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror) __attribute__((alias("mpi_comm_size")));

void mpi_comm_spawn(MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror)
{
ear_mpif.comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, ierror);
}
void mpi_comm_spawn_ (MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) __attribute__((alias("mpi_comm_spawn")));
void mpi_comm_spawn__ (MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) __attribute__((alias("mpi_comm_spawn")));

void mpi_comm_spawn_multiple(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror)
{
ear_mpif.comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes, ierror);
}
void mpi_comm_spawn_multiple_ (MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) __attribute__((alias("mpi_comm_spawn_multiple")));
void mpi_comm_spawn_multiple__ (MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) __attribute__((alias("mpi_comm_spawn_multiple")));

void mpi_comm_split(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror)
{
	ear_mpif.comm_split(comm, color, key, newcomm, ierror);
}
void mpi_comm_split_ (MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_split")));
void mpi_comm_split__ (MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_split")));

void mpi_file_close(MPI_File *fh, MPI_Fint *ierror)
{
	ear_mpif.file_close(fh, ierror);
}
void mpi_file_close_ (MPI_File *fh, MPI_Fint *ierror) __attribute__((alias("mpi_file_close")));
void mpi_file_close__ (MPI_File *fh, MPI_Fint *ierror) __attribute__((alias("mpi_file_close")));

void mpi_file_read(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	ear_mpif.file_read(fh, buf, count, datatype, status, ierror);
}
void mpi_file_read_ (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read")));
void mpi_file_read__ (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read")));

void mpi_file_read_all(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	ear_mpif.file_read_all(fh, buf, count, datatype, status, ierror);
}
void mpi_file_read_all_ (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_all")));
void mpi_file_read_all__ (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_all")));

void mpi_file_read_at(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	ear_mpif.file_read_at(fh, offset, buf, count, datatype, status, ierror);
}
void mpi_file_read_at_ (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_at")));
void mpi_file_read_at__ (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_at")));

void mpi_file_read_at_all(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	ear_mpif.file_read_at_all(fh, offset, buf, count, datatype, status, ierror);
}
void mpi_file_read_at_all_ (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_at_all")));
void mpi_file_read_at_all__ (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_at_all")));

void mpi_file_write(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	ear_mpif.file_write(fh, buf, count, datatype, status, ierror);
}
void mpi_file_write_ (MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write")));
void mpi_file_write__ (MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write")));

void mpi_file_write_all(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	ear_mpif.file_write_all(fh, buf, count, datatype, status, ierror);
}
void mpi_file_write_all_ (MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_all")));
void mpi_file_write_all__ (MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_all")));

void mpi_file_write_at(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	ear_mpif.file_write_at(fh, offset, buf, count, datatype, status, ierror);
}
void mpi_file_write_at_ (MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_at")));
void mpi_file_write_at__ (MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_at")));

void mpi_file_write_at_all(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	ear_mpif.file_write_at_all(fh, offset, buf, count, datatype, status, ierror);
}
void mpi_file_write_at_all_ (MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_at_all")));
void mpi_file_write_at_all__ (MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_at_all")));

void mpi_finalize(MPI_Fint *ierror)
{
	ear_mpif.finalize(ierror);
}
void mpi_finalize_ (MPI_Fint *ierror) __attribute__((alias("mpi_finalize")));
void mpi_finalize__ (MPI_Fint *ierror) __attribute__((alias("mpi_finalize")));

void mpi_gather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
}
void mpi_gather_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_gather")));
void mpi_gather__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_gather")));

void mpi_gatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierror);
}
void mpi_gatherv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_gatherv")));
void mpi_gatherv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_gatherv")));

void mpi_get(MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, ierror);
}
void mpi_get_ (MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_get")));
void mpi_get__ (MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_get")));

void mpi_ibsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.ibsend(buf, count, datatype, dest, tag, comm, request, ierror);
}
void mpi_ibsend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibsend")));
void mpi_ibsend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibsend")));

void mpi_init(MPI_Fint *ierror)
{
	ear_mpif.init(ierror);
}
void mpi_init_ (MPI_Fint *ierror) __attribute__((alias("mpi_init")));
void mpi_init__ (MPI_Fint *ierror) __attribute__((alias("mpi_init")));

void mpi_init_thread(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror)
{
	ear_mpif.init_thread(required, provided, ierror);
}
void mpi_init_thread_ (MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror) __attribute__((alias("mpi_init_thread")));
void mpi_init_thread__ (MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror) __attribute__((alias("mpi_init_thread")));

void mpi_intercomm_create(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror)
{
	ear_mpif.intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm, ierror);
}
void mpi_intercomm_create_ (MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror) __attribute__((alias("mpi_intercomm_create")));
void mpi_intercomm_create__ (MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror) __attribute__((alias("mpi_intercomm_create")));

void mpi_intercomm_merge(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror)
{
	ear_mpif.intercomm_merge(intercomm, high, newintracomm, ierror);
}
void mpi_intercomm_merge_ (MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror) __attribute__((alias("mpi_intercomm_merge")));
void mpi_intercomm_merge__ (MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror) __attribute__((alias("mpi_intercomm_merge")));

void mpi_iprobe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.iprobe(source, tag, comm, flag, status, ierror);
}
void mpi_iprobe_ (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_iprobe")));
void mpi_iprobe__ (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_iprobe")));

void mpi_irecv(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.irecv(buf, count, datatype, source, tag, comm, request, ierror);
}
void mpi_irecv_ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_irecv")));
void mpi_irecv__ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_irecv")));

void mpi_irsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.irsend(buf, count, datatype, dest, tag, comm, request, ierror);
}
void mpi_irsend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_irsend")));
void mpi_irsend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_irsend")));

void mpi_isend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.isend(buf, count, datatype, dest, tag, comm, request, ierror);
}
void mpi_isend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_isend")));
void mpi_isend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_isend")));

void mpi_issend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.issend(buf, count, datatype, dest, tag, comm, request, ierror);
}
void mpi_issend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_issend")));
void mpi_issend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_issend")));

void mpi_probe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.probe(source, tag, comm, status, ierror);
}
void mpi_probe_ (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_probe")));
void mpi_probe__ (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_probe")));

void mpi_put(MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, ierror);
}
void mpi_put_ (MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_put")));
void mpi_put__ (MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_put")));

void mpi_recv(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.recv(buf, count, datatype, source, tag, comm, status, ierror);
}
void mpi_recv_ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_recv")));
void mpi_recv__ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_recv")));

void mpi_recv_init(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.recv_init(buf, count, datatype, source, tag, comm, request, ierror);
}
void mpi_recv_init_ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_recv_init")));
void mpi_recv_init__ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_recv_init")));

void mpi_reduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.reduce(sendbuf, recvbuf, count, datatype, op, root, comm, ierror);
}
void mpi_reduce_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_reduce")));
void mpi_reduce__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_reduce")));

void mpi_reduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierror);
}
void mpi_reduce_scatter_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_reduce_scatter")));
void mpi_reduce_scatter__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_reduce_scatter")));

void mpi_request_free(MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.request_free(request, ierror);
}
void mpi_request_free_ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_request_free")));
void mpi_request_free__ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_request_free")));

void mpi_request_get_status(MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.request_get_status(request, flag, status, ierror);
}
void mpi_request_get_status_ (MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_request_get_status")));
void mpi_request_get_status__ (MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_request_get_status")));

void mpi_rsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.rsend(buf, count, datatype, dest, tag, comm, ierror);
}
void mpi_rsend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_rsend")));
void mpi_rsend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_rsend")));

void mpi_rsend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.rsend_init(buf, count, datatype, dest, tag, comm, request, ierror);
}
void mpi_rsend_init_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_rsend_init")));
void mpi_rsend_init__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_rsend_init")));

void mpi_scan(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.scan(sendbuf, recvbuf, count, datatype, op, comm, ierror);
}
void mpi_scan_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scan")));
void mpi_scan__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scan")));

void mpi_scatter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
}
void mpi_scatter_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scatter")));
void mpi_scatter__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scatter")));

void mpi_scatterv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
}
void mpi_scatterv_ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scatterv")));
void mpi_scatterv__ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scatterv")));

void mpi_send(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.send(buf, count, datatype, dest, tag, comm, ierror);
}
void mpi_send_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_send")));
void mpi_send__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_send")));

void mpi_send_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.send_init(buf, count, datatype, dest, tag, comm, request, ierror);
}
void mpi_send_init_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_send_init")));
void mpi_send_init__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_send_init")));

void mpi_sendrecv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status, ierror);
}
void mpi_sendrecv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_sendrecv")));
void mpi_sendrecv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_sendrecv")));

void mpi_sendrecv_replace(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status, ierror);
}
void mpi_sendrecv_replace_ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_sendrecv_replace")));
void mpi_sendrecv_replace__ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_sendrecv_replace")));

void mpi_ssend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
	ear_mpif.ssend(buf, count, datatype, dest, tag, comm, ierror);
}
void mpi_ssend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_ssend")));
void mpi_ssend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_ssend")));

void mpi_ssend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.ssend_init(buf, count, datatype, dest, tag, comm, request, ierror);
}
void mpi_ssend_init_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ssend_init")));
void mpi_ssend_init__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ssend_init")));

void mpi_start(MPI_Fint *request, MPI_Fint *ierror)
{
	ear_mpif.start(request, ierror);
}
void mpi_start_ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_start")));
void mpi_start__ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_start")));

void mpi_startall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror)
{
	ear_mpif.startall(count, array_of_requests, ierror);
}
void mpi_startall_ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror) __attribute__((alias("mpi_startall")));
void mpi_startall__ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror) __attribute__((alias("mpi_startall")));

void mpi_test(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.test(request, flag, status, ierror);
}
void mpi_test_ (MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_test")));
void mpi_test__ (MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_test")));

void mpi_testall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
	ear_mpif.testall(count, array_of_requests, flag, array_of_statuses, ierror);
}
void mpi_testall_ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_testall")));
void mpi_testall__ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_testall")));

void mpi_testany(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.testany(count, array_of_requests, index, flag, status, ierror);
}
void mpi_testany_ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_testany")));
void mpi_testany__ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_testany")));

void mpi_testsome(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
	ear_mpif.testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
}
void mpi_testsome_ (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_testsome")));
void mpi_testsome__ (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_testsome")));

void mpi_wait(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.wait(request, status, ierror);
}
void mpi_wait_ (MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_wait")));
void mpi_wait__ (MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_wait")));

void mpi_waitall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
	ear_mpif.waitall(count, array_of_requests, array_of_statuses, ierror);
}
void mpi_waitall_ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_waitall")));
void mpi_waitall__ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_waitall")));

void mpi_waitany(MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror)
{
	ear_mpif.waitany(count, requests, index, status, ierror);
}
void mpi_waitany_ (MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_waitany")));
void mpi_waitany__ (MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_waitany")));

void mpi_waitsome(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
	ear_mpif.waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
}
void mpi_waitsome_ (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_waitsome")));
void mpi_waitsome__ (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_waitsome")));

void mpi_win_complete(MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.win_complete(win, ierror);
}
void mpi_win_complete_ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_complete")));
void mpi_win_complete__ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_complete")));

void mpi_win_create(void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.win_create(base, size, disp_unit, info, comm, win, ierror);
}
void mpi_win_create_ (void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_create")));
void mpi_win_create__ (void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_create")));

void mpi_win_fence(MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.win_fence(assert, win, ierror);
}
void mpi_win_fence_ (MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_fence")));
void mpi_win_fence__ (MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_fence")));

void mpi_win_free(MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.win_free(win, ierror);
}
void mpi_win_free_ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_free")));
void mpi_win_free__ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_free")));

void mpi_win_post(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.win_post(group, assert, win, ierror);
}
void mpi_win_post_ (MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_post")));
void mpi_win_post__ (MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_post")));

void mpi_win_start(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.win_start(group, assert, win, ierror);
}
void mpi_win_start_ (MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_start")));
void mpi_win_start__ (MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_start")));

void mpi_win_wait(MPI_Fint *win, MPI_Fint *ierror)
{
	ear_mpif.win_wait(win, ierror);
}
void mpi_win_wait_ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_wait")));
void mpi_win_wait__ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_wait")));

//#if MPI_VERSION >= 3
void mpi_iallgather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierror);
}
void mpi_iallgather_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallgather")));
void mpi_iallgather__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallgather")));

void mpi_iallgatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcount, displs, recvtype, comm, request, ierror);
}
void mpi_iallgatherv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallgatherv")));
void mpi_iallgatherv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallgatherv")));

void mpi_iallreduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.iallreduce(sendbuf, recvbuf, count, datatype, op, comm, request, ierror);
}
void mpi_iallreduce_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallreduce")));
void mpi_iallreduce__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallreduce")));

void mpi_ialltoall(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierror);
}
void mpi_ialltoall_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ialltoall")));
void mpi_ialltoall__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ialltoall")));

void mpi_ialltoallv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror)
{
    ear_mpif.ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, request, comm, ierror);
}
void mpi_ialltoallv_ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_ialltoallv")));
void mpi_ialltoallv__ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_ialltoallv")));

void mpi_ibarrier(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.ibarrier(comm, request, ierror);
}
void mpi_ibarrier_ (MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibarrier")));
void mpi_ibarrier__ (MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibarrier")));

void mpi_ibcast(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.ibcast(buffer, count, datatype, root, comm, request, ierror);
}
void mpi_ibcast_ (void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibcast")));
void mpi_ibcast__ (void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibcast")));

void mpi_igather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
}
void mpi_igather_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_igather")));
void mpi_igather__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_igather")));

void mpi_igatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request, ierror);
}
void mpi_igatherv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_igatherv")));
void mpi_igatherv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_igatherv")));

void mpi_ireduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request, ierror);
}
void mpi_ireduce_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ireduce")));
void mpi_ireduce__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ireduce")));

void mpi_ireduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, request, ierror);
}
void mpi_ireduce_scatter_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ireduce_scatter")));
void mpi_ireduce_scatter__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ireduce_scatter")));

void mpi_iscan(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.iscan(sendbuf, recvbuf, count, datatype, op, comm, request, ierror);
}
void mpi_iscan_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscan")));
void mpi_iscan__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscan")));

void mpi_iscatter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
}
void mpi_iscatter_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscatter")));
void mpi_iscatter__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscatter")));

void mpi_iscatterv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    ear_mpif.iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
}
void mpi_iscatterv_ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscatterv")));
void mpi_iscatterv__ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscatterv")));
//#endif
