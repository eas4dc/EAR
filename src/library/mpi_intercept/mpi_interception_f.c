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

#define _GNU_SOURCE
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <library/mpi_intercept/MPI_interfaceF.h>
#include <library/mpi_intercept/mpi_interception_f.h>

extern const char *mpi_f_names[];
extern mpi_f_syms_t mpi_f_syms;
static int f_symbols_loaded;

static void mpi_f_symbol_load()
{
	if (f_symbols_loaded == 1) {
		return;
	}
	debug("mpi_f_symbol_load");
	symplug_join(RTLD_NEXT, (void *) &mpi_f_syms, mpi_f_names, MPI_F_SYMS_N);
	f_symbols_loaded = 1;
}

void mpi_allgather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Allgather...............");
	EAR_MPI_Allgather_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
	mpi_f_syms.mpi_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
	EAR_MPI_Allgather_F_leave();
	debug("<< F MPI_Allgather...............");
}
void mpi_allgather_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allgather")));
void mpi_allgather__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allgather")));

void mpi_allgatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Allgatherv...............");
	EAR_MPI_Allgatherv_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierror);
	mpi_f_syms.mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierror);
	EAR_MPI_Allgatherv_F_leave();
	debug("<< F MPI_Allgatherv...............");
}
void mpi_allgatherv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allgatherv")));
void mpi_allgatherv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allgatherv")));

void mpi_allreduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Allreduce...............");
	EAR_MPI_Allreduce_F_enter(sendbuf, recvbuf, count, datatype, op, comm, ierror);
	mpi_f_syms.mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, ierror);
	EAR_MPI_Allreduce_F_leave();
	debug("<< F MPI_Allreduce...............");
}
void mpi_allreduce_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allreduce")));
void mpi_allreduce__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_allreduce")));

void mpi_alltoall(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Alltoall...............");
	EAR_MPI_Alltoall_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
	mpi_f_syms.mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
	EAR_MPI_Alltoall_F_leave();
	debug("<< F MPI_Alltoall...............");
}
void mpi_alltoall_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_alltoall")));
void mpi_alltoall__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_alltoall")));

void mpi_alltoallv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Alltoallv...............");
	EAR_MPI_Alltoallv_F_enter(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, ierror);
	mpi_f_syms.mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, ierror);
	EAR_MPI_Alltoallv_F_leave();
	debug("<< F MPI_Alltoallv...............");
}
void mpi_alltoallv_ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_alltoallv")));
void mpi_alltoallv__ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_alltoallv")));

void mpi_barrier(MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Barrier...............");
	EAR_MPI_Barrier_F_enter(comm, ierror);
	mpi_f_syms.mpi_barrier(comm, ierror);
	EAR_MPI_Barrier_F_leave();
	debug("<< F MPI_Barrier...............");
}
void mpi_barrier_ (MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_barrier")));
void mpi_barrier__ (MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_barrier")));

void mpi_bcast(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Bcast...............");
	EAR_MPI_Bcast_F_enter(buffer, count, datatype, root, comm, ierror);
	mpi_f_syms.mpi_bcast(buffer, count, datatype, root, comm, ierror);
	EAR_MPI_Bcast_F_leave();
	debug("<< F MPI_Bcast...............");
}
void mpi_bcast_ (void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_bcast")));
void mpi_bcast__ (void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_bcast")));

void mpi_bsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Bsend...............");
	EAR_MPI_Bsend_F_enter(buf, count, datatype, dest, tag, comm, ierror);
	mpi_f_syms.mpi_bsend(buf, count, datatype, dest, tag, comm, ierror);
	EAR_MPI_Bsend_F_leave();
	debug("<< F MPI_Bsend...............");
}
void mpi_bsend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_bsend")));
void mpi_bsend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_bsend")));

void mpi_bsend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Bsend_init...............");
	EAR_MPI_Bsend_init_F_enter(buf, count, datatype, dest, tag, comm, request, ierror);
	mpi_f_syms.mpi_bsend_init(buf, count, datatype, dest, tag, comm, request, ierror);
	EAR_MPI_Bsend_init_F_leave();
	debug("<< F MPI_Bsend_init...............");
}
void mpi_bsend_init_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_bsend_init")));
void mpi_bsend_init__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_bsend_init")));

void mpi_cancel(MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Cancel...............");
	EAR_MPI_Cancel_F_enter(request, ierror);
	mpi_f_syms.mpi_cancel(request, ierror);
	EAR_MPI_Cancel_F_leave();
	debug("<< F MPI_Cancel...............");
}
void mpi_cancel_ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_cancel")));
void mpi_cancel__ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_cancel")));

void mpi_cart_create(MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror)
{
	debug(">> F MPI_Cart_create...............");
	EAR_MPI_Cart_create_F_enter(comm_old, ndims, dims, periods, reorder, comm_cart, ierror);
	mpi_f_syms.mpi_cart_create(comm_old, ndims, dims, periods, reorder, comm_cart, ierror);
	EAR_MPI_Cart_create_F_leave();
	debug("<< F MPI_Cart_create...............");
}
void mpi_cart_create_ (MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror) __attribute__((alias("mpi_cart_create")));
void mpi_cart_create__ (MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror) __attribute__((alias("mpi_cart_create")));

void mpi_cart_sub(MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror)
{
	debug(">> F MPI_Cart_sub...............");
	EAR_MPI_Cart_sub_F_enter(comm, remain_dims, comm_new, ierror);
	mpi_f_syms.mpi_cart_sub(comm, remain_dims, comm_new, ierror);
	EAR_MPI_Cart_sub_F_leave();
	debug("<< F MPI_Cart_sub...............");
}
void mpi_cart_sub_ (MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror) __attribute__((alias("mpi_cart_sub")));
void mpi_cart_sub__ (MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror) __attribute__((alias("mpi_cart_sub")));

void mpi_comm_create(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror)
{
	debug(">> F MPI_Comm_create...............");
	EAR_MPI_Comm_create_F_enter(comm, group, newcomm, ierror);
	mpi_f_syms.mpi_comm_create(comm, group, newcomm, ierror);
	EAR_MPI_Comm_create_F_leave();
	debug("<< F MPI_Comm_create...............");
}
void mpi_comm_create_ (MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_create")));
void mpi_comm_create__ (MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_create")));

void mpi_comm_dup(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror)
{
	debug(">> F MPI_Comm_dup...............");
	EAR_MPI_Comm_dup_F_enter(comm, newcomm, ierror);
	mpi_f_syms.mpi_comm_dup(comm, newcomm, ierror);
	EAR_MPI_Comm_dup_F_leave();
	debug("<< F MPI_Comm_dup...............");
}
void mpi_comm_dup_ (MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_dup")));
void mpi_comm_dup__ (MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_dup")));

void mpi_comm_free(MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Comm_free...............");
	EAR_MPI_Comm_free_F_enter(comm, ierror);
	mpi_f_syms.mpi_comm_free(comm, ierror);
	EAR_MPI_Comm_free_F_leave();
	debug("<< F MPI_Comm_free...............");
}
void mpi_comm_free_ (MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_free")));
void mpi_comm_free__ (MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_free")));

void mpi_comm_rank(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror)
{
	debug(">> F MPI_Comm_rank...............");
	EAR_MPI_Comm_rank_F_enter(comm, rank, ierror);
	mpi_f_syms.mpi_comm_rank(comm, rank, ierror);

	EAR_MPI_Comm_rank_F_leave();
	debug("<< F MPI_Comm_rank...............");
}
void mpi_comm_rank_ (MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror) __attribute__((alias("mpi_comm_rank")));
void mpi_comm_rank__ (MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror) __attribute__((alias("mpi_comm_rank")));

void mpi_comm_size(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror)
{
	debug(">> F MPI_Comm_size...............");
	EAR_MPI_Comm_size_F_enter(comm, size, ierror);
	mpi_f_syms.mpi_comm_size(comm, size, ierror);
	EAR_MPI_Comm_size_F_leave();
	debug("<< F MPI_Comm_size...............");
}
void mpi_comm_size_ (MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror) __attribute__((alias("mpi_comm_size")));
void mpi_comm_size__ (MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror) __attribute__((alias("mpi_comm_size")));

void mpi_comm_spawn(MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror)
{
	debug(">> F MPI_Comm_spawn...............");
	EAR_MPI_Comm_spawn_F_enter(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, ierror);
	mpi_f_syms.mpi_comm_spawn(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, ierror);
	EAR_MPI_Comm_spawn_F_leave();
	debug("<< F MPI_Comm_spawn...............");
}
void mpi_comm_spawn_ (MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) __attribute__((alias("mpi_comm_spawn")));
void mpi_comm_spawn__ (MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) __attribute__((alias("mpi_comm_spawn")));

void mpi_comm_spawn_multiple(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror)
{
	debug(">> F MPI_Comm_spawn_multiple...............");
	EAR_MPI_Comm_spawn_multiple_F_enter(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes, ierror);
	mpi_f_syms.mpi_comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes, ierror);
	EAR_MPI_Comm_spawn_multiple_F_leave();
	debug("<< F MPI_Comm_spawn_multiple...............");
}
void mpi_comm_spawn_multiple_ (MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) __attribute__((alias("mpi_comm_spawn_multiple")));
void mpi_comm_spawn_multiple__ (MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) __attribute__((alias("mpi_comm_spawn_multiple")));

void mpi_comm_split(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror)
{
	debug(">> F MPI_Comm_split...............");
	EAR_MPI_Comm_split_F_enter(comm, color, key, newcomm, ierror);
	mpi_f_syms.mpi_comm_split(comm, color, key, newcomm, ierror);
	EAR_MPI_Comm_split_F_leave();
	debug("<< F MPI_Comm_split...............");
}
void mpi_comm_split_ (MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_split")));
void mpi_comm_split__ (MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror) __attribute__((alias("mpi_comm_split")));

void mpi_file_close(MPI_File *fh, MPI_Fint *ierror)
{
	debug(">> F MPI_File_close...............");
	EAR_MPI_File_close_F_enter(fh, ierror);
	mpi_f_syms.mpi_file_close(fh, ierror);
	EAR_MPI_File_close_F_leave();
	debug("<< F MPI_File_close...............");
}
void mpi_file_close_ (MPI_File *fh, MPI_Fint *ierror) __attribute__((alias("mpi_file_close")));
void mpi_file_close__ (MPI_File *fh, MPI_Fint *ierror) __attribute__((alias("mpi_file_close")));

void mpi_file_read(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	debug(">> F MPI_File_read...............");
	EAR_MPI_File_read_F_enter(fh, buf, count, datatype, status, ierror);
	mpi_f_syms.mpi_file_read(fh, buf, count, datatype, status, ierror);
	EAR_MPI_File_read_F_leave();
	debug("<< F MPI_File_read...............");
}
void mpi_file_read_ (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read")));
void mpi_file_read__ (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read")));

void mpi_file_read_all(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	debug(">> F MPI_File_read_all...............");
	EAR_MPI_File_read_all_F_enter(fh, buf, count, datatype, status, ierror);
	mpi_f_syms.mpi_file_read_all(fh, buf, count, datatype, status, ierror);
	EAR_MPI_File_read_all_F_leave();
	debug("<< F MPI_File_read_all...............");
}
void mpi_file_read_all_ (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_all")));
void mpi_file_read_all__ (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_all")));

void mpi_file_read_at(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	debug(">> F MPI_File_read_at...............");
	EAR_MPI_File_read_at_F_enter(fh, offset, buf, count, datatype, status, ierror);
	mpi_f_syms.mpi_file_read_at(fh, offset, buf, count, datatype, status, ierror);
	EAR_MPI_File_read_at_F_leave();
	debug("<< F MPI_File_read_at...............");
}
void mpi_file_read_at_ (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_at")));
void mpi_file_read_at__ (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_at")));

void mpi_file_read_at_all(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	debug(">> F MPI_File_read_at_all...............");
	EAR_MPI_File_read_at_all_F_enter(fh, offset, buf, count, datatype, status, ierror);
	mpi_f_syms.mpi_file_read_at_all(fh, offset, buf, count, datatype, status, ierror);
	EAR_MPI_File_read_at_all_F_leave();
	debug("<< F MPI_File_read_at_all...............");
}
void mpi_file_read_at_all_ (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_at_all")));
void mpi_file_read_at_all__ (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_read_at_all")));

void mpi_file_write(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	debug(">> F MPI_File_write...............");
	EAR_MPI_File_write_F_enter(fh, buf, count, datatype, status, ierror);
	mpi_f_syms.mpi_file_write(fh, buf, count, datatype, status, ierror);
	EAR_MPI_File_write_F_leave();
	debug("<< F MPI_File_write...............");
}
void mpi_file_write_ (MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write")));
void mpi_file_write__ (MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write")));

void mpi_file_write_all(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	debug(">> F MPI_File_write_all...............");
	EAR_MPI_File_write_all_F_enter(fh, buf, count, datatype, status, ierror);
	mpi_f_syms.mpi_file_write_all(fh, buf, count, datatype, status, ierror);
	EAR_MPI_File_write_all_F_leave();
	debug("<< F MPI_File_write_all...............");
}
void mpi_file_write_all_ (MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_all")));
void mpi_file_write_all__ (MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_all")));

void mpi_file_write_at(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	debug(">> F MPI_File_write_at...............");
	EAR_MPI_File_write_at_F_enter(fh, offset, buf, count, datatype, status, ierror);
	mpi_f_syms.mpi_file_write_at(fh, offset, buf, count, datatype, status, ierror);
	EAR_MPI_File_write_at_F_leave();
	debug("<< F MPI_File_write_at...............");
}
void mpi_file_write_at_ (MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_at")));
void mpi_file_write_at__ (MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_at")));

void mpi_file_write_at_all(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
{
	debug(">> F MPI_File_write_at_all...............");
	EAR_MPI_File_write_at_all_F_enter(fh, offset, buf, count, datatype, status, ierror);
	mpi_f_syms.mpi_file_write_at_all(fh, offset, buf, count, datatype, status, ierror);
	EAR_MPI_File_write_at_all_F_leave();
	debug("<< F MPI_File_write_at_all...............");
}
void mpi_file_write_at_all_ (MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_at_all")));
void mpi_file_write_at_all__ (MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) __attribute__((alias("mpi_file_write_at_all")));

void mpi_finalize(MPI_Fint *ierror)
{
	debug(">> F MPI_Finalize...............");
	EAR_MPI_Finalize_F_enter(ierror);
	mpi_f_syms.mpi_finalize(ierror);
	EAR_MPI_Finalize_F_leave();
	debug("<< F MPI_Finalize...............");
}
void mpi_finalize_ (MPI_Fint *ierror) __attribute__((alias("mpi_finalize")));
void mpi_finalize__ (MPI_Fint *ierror) __attribute__((alias("mpi_finalize")));

void mpi_gather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Gather...............");
	EAR_MPI_Gather_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
	mpi_f_syms.mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
	EAR_MPI_Gather_F_leave();
	debug("<< F MPI_Gather...............");
}
void mpi_gather_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_gather")));
void mpi_gather__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_gather")));

void mpi_gatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Gatherv...............");
	EAR_MPI_Gatherv_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierror);
	mpi_f_syms.mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierror);
	EAR_MPI_Gatherv_F_leave();
	debug("<< F MPI_Gatherv...............");
}
void mpi_gatherv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_gatherv")));
void mpi_gatherv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_gatherv")));

void mpi_get(MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Get...............");
	EAR_MPI_Get_F_enter(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, ierror);
	mpi_f_syms.mpi_get(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, ierror);
	EAR_MPI_Get_F_leave();
	debug("<< F MPI_Get...............");
}
void mpi_get_ (MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_get")));
void mpi_get__ (MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_get")));

void mpi_ibsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Ibsend...............");
	EAR_MPI_Ibsend_F_enter(buf, count, datatype, dest, tag, comm, request, ierror);
	mpi_f_syms.mpi_ibsend(buf, count, datatype, dest, tag, comm, request, ierror);
	EAR_MPI_Ibsend_F_leave();
	debug("<< F MPI_Ibsend...............");
}
void mpi_ibsend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibsend")));
void mpi_ibsend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibsend")));

void mpi_init(MPI_Fint *ierror)
{
	debug(">> F MPI_Init...............");
	mpi_f_symbol_load();
	EAR_MPI_Init_F_enter(ierror);
	mpi_f_syms.mpi_init(ierror);
	EAR_MPI_Init_F_leave();
	debug("<< F MPI_Init...............");
}
void mpi_init_ (MPI_Fint *ierror) __attribute__((alias("mpi_init")));
void mpi_init__ (MPI_Fint *ierror) __attribute__((alias("mpi_init")));

void mpi_init_thread(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror)
{
	debug(">> F MPI_Init_thread...............");
	mpi_f_symbol_load();
	EAR_MPI_Init_thread_F_enter(required, provided, ierror);
	mpi_f_syms.mpi_init_thread(required, provided, ierror);
	EAR_MPI_Init_thread_F_leave();
	debug("<< F MPI_Init_thread...............");
}
void mpi_init_thread_ (MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror) __attribute__((alias("mpi_init_thread")));
void mpi_init_thread__ (MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror) __attribute__((alias("mpi_init_thread")));

void mpi_intercomm_create(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror)
{
	debug(">> F MPI_Intercomm_create...............");
	EAR_MPI_Intercomm_create_F_enter(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm, ierror);
	mpi_f_syms.mpi_intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm, ierror);
	EAR_MPI_Intercomm_create_F_leave();
	debug("<< F MPI_Intercomm_create...............");
}
void mpi_intercomm_create_ (MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror) __attribute__((alias("mpi_intercomm_create")));
void mpi_intercomm_create__ (MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror) __attribute__((alias("mpi_intercomm_create")));

void mpi_intercomm_merge(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror)
{
	debug(">> F MPI_Intercomm_merge...............");
	EAR_MPI_Intercomm_merge_F_enter(intercomm, high, newintracomm, ierror);
	mpi_f_syms.mpi_intercomm_merge(intercomm, high, newintracomm, ierror);
	EAR_MPI_Intercomm_merge_F_leave();
	debug("<< F MPI_Intercomm_merge...............");
}
void mpi_intercomm_merge_ (MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror) __attribute__((alias("mpi_intercomm_merge")));
void mpi_intercomm_merge__ (MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror) __attribute__((alias("mpi_intercomm_merge")));

void mpi_iprobe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Iprobe...............");
	EAR_MPI_Iprobe_F_enter(source, tag, comm, flag, status, ierror);
	mpi_f_syms.mpi_iprobe(source, tag, comm, flag, status, ierror);
	EAR_MPI_Iprobe_F_leave();
	debug("<< F MPI_Iprobe...............");
}
void mpi_iprobe_ (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_iprobe")));
void mpi_iprobe__ (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_iprobe")));

void mpi_irecv(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Irecv...............");
	EAR_MPI_Irecv_F_enter(buf, count, datatype, source, tag, comm, request, ierror);
	mpi_f_syms.mpi_irecv(buf, count, datatype, source, tag, comm, request, ierror);
	EAR_MPI_Irecv_F_leave();
	debug("<< F MPI_Irecv...............");
}
void mpi_irecv_ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_irecv")));
void mpi_irecv__ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_irecv")));

void mpi_irsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Irsend...............");
	EAR_MPI_Irsend_F_enter(buf, count, datatype, dest, tag, comm, request, ierror);
	mpi_f_syms.mpi_irsend(buf, count, datatype, dest, tag, comm, request, ierror);
	EAR_MPI_Irsend_F_leave();
	debug("<< F MPI_Irsend...............");
}
void mpi_irsend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_irsend")));
void mpi_irsend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_irsend")));

void mpi_isend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Isend...............");
	EAR_MPI_Isend_F_enter(buf, count, datatype, dest, tag, comm, request, ierror);
	mpi_f_syms.mpi_isend(buf, count, datatype, dest, tag, comm, request, ierror);
	EAR_MPI_Isend_F_leave();
	debug("<< F MPI_Isend...............");
}
void mpi_isend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_isend")));
void mpi_isend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_isend")));

void mpi_issend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Issend...............");
	EAR_MPI_Issend_F_enter(buf, count, datatype, dest, tag, comm, request, ierror);
	mpi_f_syms.mpi_issend(buf, count, datatype, dest, tag, comm, request, ierror);
	EAR_MPI_Issend_F_leave();
	debug("<< F MPI_Issend...............");
}
void mpi_issend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_issend")));
void mpi_issend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_issend")));

void mpi_probe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Probe...............");
	EAR_MPI_Probe_F_enter(source, tag, comm, status, ierror);
	mpi_f_syms.mpi_probe(source, tag, comm, status, ierror);
	EAR_MPI_Probe_F_leave();
	debug("<< F MPI_Probe...............");
}
void mpi_probe_ (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_probe")));
void mpi_probe__ (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_probe")));

void mpi_put(MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Put...............");
	EAR_MPI_Put_F_enter(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, ierror);
	mpi_f_syms.mpi_put(origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win, ierror);
	EAR_MPI_Put_F_leave();
	debug("<< F MPI_Put...............");
}
void mpi_put_ (MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_put")));
void mpi_put__ (MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_put")));

void mpi_recv(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Recv...............");
	EAR_MPI_Recv_F_enter(buf, count, datatype, source, tag, comm, status, ierror);
	mpi_f_syms.mpi_recv(buf, count, datatype, source, tag, comm, status, ierror);
	EAR_MPI_Recv_F_leave();
	debug("<< F MPI_Recv...............");
}
void mpi_recv_ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_recv")));
void mpi_recv__ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_recv")));

void mpi_recv_init(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Recv_init...............");
	EAR_MPI_Recv_init_F_enter(buf, count, datatype, source, tag, comm, request, ierror);
	mpi_f_syms.mpi_recv_init(buf, count, datatype, source, tag, comm, request, ierror);
	EAR_MPI_Recv_init_F_leave();
	debug("<< F MPI_Recv_init...............");
}
void mpi_recv_init_ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_recv_init")));
void mpi_recv_init__ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_recv_init")));

void mpi_reduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Reduce...............");
	EAR_MPI_Reduce_F_enter(sendbuf, recvbuf, count, datatype, op, root, comm, ierror);
	mpi_f_syms.mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm, ierror);
	EAR_MPI_Reduce_F_leave();
	debug("<< F MPI_Reduce...............");
}
void mpi_reduce_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_reduce")));
void mpi_reduce__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_reduce")));

void mpi_reduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Reduce_scatter...............");
	EAR_MPI_Reduce_scatter_F_enter(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierror);
	mpi_f_syms.mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierror);
	EAR_MPI_Reduce_scatter_F_leave();
	debug("<< F MPI_Reduce_scatter...............");
}
void mpi_reduce_scatter_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_reduce_scatter")));
void mpi_reduce_scatter__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_reduce_scatter")));

void mpi_request_free(MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Request_free...............");
	EAR_MPI_Request_free_F_enter(request, ierror);
	mpi_f_syms.mpi_request_free(request, ierror);
	EAR_MPI_Request_free_F_leave();
	debug("<< F MPI_Request_free...............");
}
void mpi_request_free_ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_request_free")));
void mpi_request_free__ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_request_free")));

void mpi_request_get_status(MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Request_get_status...............");
	EAR_MPI_Request_get_status_F_enter(request, flag, status, ierror);
	mpi_f_syms.mpi_request_get_status(request, flag, status, ierror);
	EAR_MPI_Request_get_status_F_leave();
	debug("<< F MPI_Request_get_status...............");
}
void mpi_request_get_status_ (MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_request_get_status")));
void mpi_request_get_status__ (MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_request_get_status")));

void mpi_rsend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Rsend...............");
	EAR_MPI_Rsend_F_enter(buf, count, datatype, dest, tag, comm, ierror);
	mpi_f_syms.mpi_rsend(buf, count, datatype, dest, tag, comm, ierror);
	EAR_MPI_Rsend_F_leave();
	debug("<< F MPI_Rsend...............");
}
void mpi_rsend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_rsend")));
void mpi_rsend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_rsend")));

void mpi_rsend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Rsend_init...............");
	EAR_MPI_Rsend_init_F_enter(buf, count, datatype, dest, tag, comm, request, ierror);
	mpi_f_syms.mpi_rsend_init(buf, count, datatype, dest, tag, comm, request, ierror);
	EAR_MPI_Rsend_init_F_leave();
	debug("<< F MPI_Rsend_init...............");
}
void mpi_rsend_init_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_rsend_init")));
void mpi_rsend_init__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_rsend_init")));

void mpi_scan(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Scan...............");
	EAR_MPI_Scan_F_enter(sendbuf, recvbuf, count, datatype, op, comm, ierror);
	mpi_f_syms.mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, ierror);
	EAR_MPI_Scan_F_leave();
	debug("<< F MPI_Scan...............");
}
void mpi_scan_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scan")));
void mpi_scan__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scan")));

void mpi_scatter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Scatter...............");
	EAR_MPI_Scatter_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
	mpi_f_syms.mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
	EAR_MPI_Scatter_F_leave();
	debug("<< F MPI_Scatter...............");
}
void mpi_scatter_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scatter")));
void mpi_scatter__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scatter")));

void mpi_scatterv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Scatterv...............");
	EAR_MPI_Scatterv_F_enter(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
	mpi_f_syms.mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
	EAR_MPI_Scatterv_F_leave();
	debug("<< F MPI_Scatterv...............");
}
void mpi_scatterv_ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scatterv")));
void mpi_scatterv__ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_scatterv")));

void mpi_send(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Send...............");
	EAR_MPI_Send_F_enter(buf, count, datatype, dest, tag, comm, ierror);
	mpi_f_syms.mpi_send(buf, count, datatype, dest, tag, comm, ierror);
	EAR_MPI_Send_F_leave();
	debug("<< F MPI_Send...............");
}
void mpi_send_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_send")));
void mpi_send__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_send")));

void mpi_send_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
debug(">> F MPI_Send_init...............");
EAR_MPI_Send_init_F_enter(buf, count, datatype, dest, tag, comm, request, ierror);
mpi_f_syms.mpi_send_init(buf, count, datatype, dest, tag, comm, request, ierror);
EAR_MPI_Send_init_F_leave();
debug("<< F MPI_Send_init...............");
}
void mpi_send_init_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_send_init")));
void mpi_send_init__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_send_init")));

void mpi_sendrecv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Sendrecv...............");
	EAR_MPI_Sendrecv_F_enter(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status, ierror);
	mpi_f_syms.mpi_sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status, ierror);
	EAR_MPI_Sendrecv_F_leave();
	debug("<< F MPI_Sendrecv...............");
}
void mpi_sendrecv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_sendrecv")));
void mpi_sendrecv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_sendrecv")));

void mpi_sendrecv_replace(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Sendrecv_replace...............");
	EAR_MPI_Sendrecv_replace_F_enter(buf, count, datatype, dest, sendtag, source, recvtag, comm, status, ierror);
	mpi_f_syms.mpi_sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status, ierror);
	EAR_MPI_Sendrecv_replace_F_leave();
	debug("<< F MPI_Sendrecv_replace...............");
}
void mpi_sendrecv_replace_ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_sendrecv_replace")));
void mpi_sendrecv_replace__ (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_sendrecv_replace")));

void mpi_ssend(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
{
	debug(">> F MPI_Ssend...............");
	EAR_MPI_Ssend_F_enter(buf, count, datatype, dest, tag, comm, ierror);
	mpi_f_syms.mpi_ssend(buf, count, datatype, dest, tag, comm, ierror);
	EAR_MPI_Ssend_F_leave();
	debug("<< F MPI_Ssend...............");
}
void mpi_ssend_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_ssend")));
void mpi_ssend__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_ssend")));

void mpi_ssend_init(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Ssend_init...............");
	EAR_MPI_Ssend_init_F_enter(buf, count, datatype, dest, tag, comm, request, ierror);
	mpi_f_syms.mpi_ssend_init(buf, count, datatype, dest, tag, comm, request, ierror);
	EAR_MPI_Ssend_init_F_leave();
	debug("<< F MPI_Ssend_init...............");
}
void mpi_ssend_init_ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ssend_init")));
void mpi_ssend_init__ (MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ssend_init")));

void mpi_start(MPI_Fint *request, MPI_Fint *ierror)
{
	debug(">> F MPI_Start...............");
	EAR_MPI_Start_F_enter(request, ierror);
	mpi_f_syms.mpi_start(request, ierror);
	EAR_MPI_Start_F_leave();
	debug("<< F MPI_Start...............");
}
void mpi_start_ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_start")));
void mpi_start__ (MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_start")));

void mpi_startall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror)
{
	debug(">> F MPI_Startall...............");
	EAR_MPI_Startall_F_enter(count, array_of_requests, ierror);
	mpi_f_syms.mpi_startall(count, array_of_requests, ierror);
	EAR_MPI_Startall_F_leave();
	debug("<< F MPI_Startall...............");
}
void mpi_startall_ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror) __attribute__((alias("mpi_startall")));
void mpi_startall__ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror) __attribute__((alias("mpi_startall")));

void mpi_test(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Test...............");
	EAR_MPI_Test_F_enter(request, flag, status, ierror);
	mpi_f_syms.mpi_test(request, flag, status, ierror);
	EAR_MPI_Test_F_leave();
	debug("<< F MPI_Test...............");
}
void mpi_test_ (MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_test")));
void mpi_test__ (MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_test")));

void mpi_testall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
	debug(">> F MPI_Testall...............");
	EAR_MPI_Testall_F_enter(count, array_of_requests, flag, array_of_statuses, ierror);
	mpi_f_syms.mpi_testall(count, array_of_requests, flag, array_of_statuses, ierror);
	EAR_MPI_Testall_F_leave();
	debug("<< F MPI_Testall...............");
}
void mpi_testall_ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_testall")));
void mpi_testall__ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_testall")));

void mpi_testany(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Testany...............");
	EAR_MPI_Testany_F_enter(count, array_of_requests, index, flag, status, ierror);
	mpi_f_syms.mpi_testany(count, array_of_requests, index, flag, status, ierror);
	EAR_MPI_Testany_F_leave();
	debug("<< F MPI_Testany...............");
}
void mpi_testany_ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_testany")));
void mpi_testany__ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_testany")));

void mpi_testsome(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
	debug(">> F MPI_Testsome...............");
	EAR_MPI_Testsome_F_enter(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
	mpi_f_syms.mpi_testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
	EAR_MPI_Testsome_F_leave();
	debug("<< F MPI_Testsome...............");
}
void mpi_testsome_ (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_testsome")));
void mpi_testsome__ (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_testsome")));

void mpi_wait(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Wait...............");
	EAR_MPI_Wait_F_enter(request, status, ierror);
	mpi_f_syms.mpi_wait(request, status, ierror);
	EAR_MPI_Wait_F_leave();
	debug("<< F MPI_Wait...............");
}
void mpi_wait_ (MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_wait")));
void mpi_wait__ (MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_wait")));

void mpi_waitall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
	debug(">> F MPI_Waitall...............");
	EAR_MPI_Waitall_F_enter(count, array_of_requests, array_of_statuses, ierror);
	mpi_f_syms.mpi_waitall(count, array_of_requests, array_of_statuses, ierror);
	EAR_MPI_Waitall_F_leave();
	debug("<< F MPI_Waitall...............");
}
void mpi_waitall_ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_waitall")));
void mpi_waitall__ (MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_waitall")));

void mpi_waitany(MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror)
{
	debug(">> F MPI_Waitany...............");
	EAR_MPI_Waitany_F_enter(count, requests, index, status, ierror);
	mpi_f_syms.mpi_waitany(count, requests, index, status, ierror);
	EAR_MPI_Waitany_F_leave();
	debug("<< F MPI_Waitany...............");
}
void mpi_waitany_ (MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_waitany")));
void mpi_waitany__ (MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror) __attribute__((alias("mpi_waitany")));

void mpi_waitsome(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror)
{
	debug(">> F MPI_Waitsome...............");
	EAR_MPI_Waitsome_F_enter(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
	mpi_f_syms.mpi_waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
	EAR_MPI_Waitsome_F_leave();
	debug("<< F MPI_Waitsome...............");
}
void mpi_waitsome_ (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_waitsome")));
void mpi_waitsome__ (MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) __attribute__((alias("mpi_waitsome")));

void mpi_win_complete(MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Win_complete...............");
	EAR_MPI_Win_complete_F_enter(win, ierror);
	mpi_f_syms.mpi_win_complete(win, ierror);
	EAR_MPI_Win_complete_F_leave();
	debug("<< F MPI_Win_complete...............");
}
void mpi_win_complete_ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_complete")));
void mpi_win_complete__ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_complete")));

void mpi_win_create(void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Win_create...............");
	EAR_MPI_Win_create_F_enter(base, size, disp_unit, info, comm, win, ierror);
	mpi_f_syms.mpi_win_create(base, size, disp_unit, info, comm, win, ierror);
	EAR_MPI_Win_create_F_leave();
	debug("<< F MPI_Win_create...............");
}
void mpi_win_create_ (void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_create")));
void mpi_win_create__ (void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_create")));

void mpi_win_fence(MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Win_fence...............");
	EAR_MPI_Win_fence_F_enter(assert, win, ierror);
	mpi_f_syms.mpi_win_fence(assert, win, ierror);
	EAR_MPI_Win_fence_F_leave();
	debug("<< F MPI_Win_fence...............");
}
void mpi_win_fence_ (MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_fence")));
void mpi_win_fence__ (MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_fence")));

void mpi_win_free(MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Win_free...............");
	EAR_MPI_Win_free_F_enter(win, ierror);
	mpi_f_syms.mpi_win_free(win, ierror);
	EAR_MPI_Win_free_F_leave();
	debug("<< F MPI_Win_free...............");
}
void mpi_win_free_ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_free")));
void mpi_win_free__ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_free")));

void mpi_win_post(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Win_post...............");
	EAR_MPI_Win_post_F_enter(group, assert, win, ierror);
	mpi_f_syms.mpi_win_post(group, assert, win, ierror);
	EAR_MPI_Win_post_F_leave();
	debug("<< F MPI_Win_post...............");
}
void mpi_win_post_ (MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_post")));
void mpi_win_post__ (MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_post")));

void mpi_win_start(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Win_start...............");
	EAR_MPI_Win_start_F_enter(group, assert, win, ierror);
	mpi_f_syms.mpi_win_start(group, assert, win, ierror);
	EAR_MPI_Win_start_F_leave();
	debug("<< F MPI_Win_start...............");
}
void mpi_win_start_ (MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_start")));
void mpi_win_start__ (MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_start")));

void mpi_win_wait(MPI_Fint *win, MPI_Fint *ierror)
{
	debug(">> F MPI_Win_wait...............");
	EAR_MPI_Win_wait_F_enter(win, ierror);
	mpi_f_syms.mpi_win_wait(win, ierror);
	EAR_MPI_Win_wait_F_leave();
	debug("<< F MPI_Win_wait...............");
}
void mpi_win_wait_ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_wait")));
void mpi_win_wait__ (MPI_Fint *win, MPI_Fint *ierror) __attribute__((alias("mpi_win_wait")));

#if MPI_VERSION >= 3
void mpi_iallgather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Iallgather...............");
    EAR_MPI_Iallgather_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierror);
    mpi_f_syms.mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierror);
    EAR_MPI_Iallgather_F_leave();
    debug("<< F MPI_Iallgather...............");
}
void mpi_iallgather_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallgather")));
void mpi_iallgather__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallgather")));

void mpi_iallgatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Iallgatherv...............");
    EAR_MPI_Iallgatherv_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, displs, recvtype, comm, request, ierror);
    mpi_f_syms.mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcount, displs, recvtype, comm, request, ierror);
    EAR_MPI_Iallgatherv_F_leave();
    debug("<< F MPI_Iallgatherv...............");
}
void mpi_iallgatherv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallgatherv")));
void mpi_iallgatherv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallgatherv")));

void mpi_iallreduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Iallreduce...............");
    EAR_MPI_Iallreduce_F_enter(sendbuf, recvbuf, count, datatype, op, comm, request, ierror);
    mpi_f_syms.mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, request, ierror);
    EAR_MPI_Iallreduce_F_leave();
    debug("<< F MPI_Iallreduce...............");
}
void mpi_iallreduce_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallreduce")));
void mpi_iallreduce__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iallreduce")));

void mpi_ialltoall(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Ialltoall...............");
    EAR_MPI_Ialltoall_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierror);
    mpi_f_syms.mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierror);
    EAR_MPI_Ialltoall_F_leave();
    debug("<< F MPI_Ialltoall...............");
}
void mpi_ialltoall_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ialltoall")));
void mpi_ialltoall__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ialltoall")));

void mpi_ialltoallv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror)
{
    debug(">> F MPI_Ialltoallv...............");
    EAR_MPI_Ialltoallv_F_enter(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, request, comm, ierror);
    mpi_f_syms.mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, request, comm, ierror);
    EAR_MPI_Ialltoallv_F_leave();
    debug("<< F MPI_Ialltoallv...............");
}
void mpi_ialltoallv_ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_ialltoallv")));
void mpi_ialltoallv__ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror) __attribute__((alias("mpi_ialltoallv")));

void mpi_ibarrier(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Ibarrier...............");
    EAR_MPI_Ibarrier_F_enter(comm, request, ierror);
    mpi_f_syms.mpi_ibarrier(comm, request, ierror);
    EAR_MPI_Ibarrier_F_leave();
    debug("<< F MPI_Ibarrier...............");
}
void mpi_ibarrier_ (MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibarrier")));
void mpi_ibarrier__ (MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibarrier")));

void mpi_ibcast(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Ibcast...............");
    EAR_MPI_Ibcast_F_enter(buffer, count, datatype, root, comm, request, ierror);
    mpi_f_syms.mpi_ibcast(buffer, count, datatype, root, comm, request, ierror);
    EAR_MPI_Ibcast_F_leave();
    debug("<< F MPI_Ibcast...............");
}
void mpi_ibcast_ (void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibcast")));
void mpi_ibcast__ (void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ibcast")));

void mpi_igather(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Igather...............");
    EAR_MPI_Igather_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    mpi_f_syms.mpi_igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    EAR_MPI_Igather_F_leave();
    debug("<< F MPI_Igather...............");
}
void mpi_igather_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_igather")));
void mpi_igather__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_igather")));

void mpi_igatherv(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Igatherv...............");
    EAR_MPI_Igatherv_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request, ierror);
    mpi_f_syms.mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request, ierror);
    EAR_MPI_Igatherv_F_leave();
    debug("<< F MPI_Igatherv...............");
}
void mpi_igatherv_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_igatherv")));
void mpi_igatherv__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_igatherv")));

void mpi_ireduce(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Ireduce...............");
    EAR_MPI_Ireduce_F_enter(sendbuf, recvbuf, count, datatype, op, root, comm, request, ierror);
    mpi_f_syms.mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request, ierror);
    EAR_MPI_Ireduce_F_leave();
    debug("<< F MPI_Ireduce...............");
}
void mpi_ireduce_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ireduce")));
void mpi_ireduce__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ireduce")));

void mpi_ireduce_scatter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Ireduce_scatter...............");
    EAR_MPI_Ireduce_scatter_F_enter(sendbuf, recvbuf, recvcounts, datatype, op, comm, request, ierror);
    mpi_f_syms.mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, request, ierror);
    EAR_MPI_Ireduce_scatter_F_leave();
    debug("<< F MPI_Ireduce_scatter...............");
}
void mpi_ireduce_scatter_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ireduce_scatter")));
void mpi_ireduce_scatter__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_ireduce_scatter")));

void mpi_iscan(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Iscan...............");
    EAR_MPI_Iscan_F_enter(sendbuf, recvbuf, count, datatype, op, comm, request, ierror);
    mpi_f_syms.mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, request, ierror);
    EAR_MPI_Iscan_F_leave();
    debug("<< F MPI_Iscan...............");
}
void mpi_iscan_ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscan")));
void mpi_iscan__ (MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscan")));

void mpi_iscatter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Iscatter...............");
    EAR_MPI_Iscatter_F_enter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    mpi_f_syms.mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    EAR_MPI_Iscatter_F_leave();
    debug("<< F MPI_Iscatter...............");
}
void mpi_iscatter_ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscatter")));
void mpi_iscatter__ (MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscatter")));

void mpi_iscatterv(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror)
{
    debug(">> F MPI_Iscatterv...............");
    EAR_MPI_Iscatterv_F_enter(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    mpi_f_syms.mpi_iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierror);
    EAR_MPI_Iscatterv_F_leave();
    debug("<< F MPI_Iscatterv...............");
}
void mpi_iscatterv_ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscatterv")));
void mpi_iscatterv__ (MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) __attribute__((alias("mpi_iscatterv")));

#endif
