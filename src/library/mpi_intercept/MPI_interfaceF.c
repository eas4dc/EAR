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

#include <common/output/debug.h>
#include <library/mpi_intercept/process_MPI.h>
#include <library/mpi_intercept/MPI_calls_coded.h>
#include <library/mpi_intercept/MPI_interfaceF.h>

#pragma GCC visibility push(default)

void EAR_MPI_Allgather_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Allgather...............");
    before_mpi(Allgather, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Allgather_F_leave(void) {
    debug("<< F EAR_MPI_Allgather...............");
    after_mpi(Allgather);
}

void EAR_MPI_Allgatherv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Allgatherv...............");
    before_mpi(Allgatherv, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Allgatherv_F_leave(void) {
    debug("<< F EAR_MPI_Allgatherv...............");
    after_mpi(Allgatherv);
}

void EAR_MPI_Allreduce_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Allreduce...............");
    before_mpi(Allreduce, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Allreduce_F_leave(void) {
    debug("<< F EAR_MPI_Allreduce...............");
    after_mpi(Allreduce);
}

void EAR_MPI_Alltoall_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Alltoall...............");
    before_mpi(Alltoall, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Alltoall_F_leave(void) {
    debug("<< F EAR_MPI_Alltoall...............");
    after_mpi(Alltoall);
}

void EAR_MPI_Alltoallv_F_enter(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Alltoallv...............");
    before_mpi(Alltoallv, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Alltoallv_F_leave(void) {
    debug("<< F EAR_MPI_Alltoallv...............");
    after_mpi(Alltoallv);
}

void EAR_MPI_Barrier_F_enter(MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Barrier...............");
    before_mpi(Barrier, (p2i)comm, (p2i)ierror);
}

void EAR_MPI_Barrier_F_leave(void) {
    debug("<< F EAR_MPI_Barrier...............");
    after_mpi(Barrier);
}

void EAR_MPI_Bcast_F_enter(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Bcast...............");
    before_mpi(Bcast, (p2i)buffer, 0);
}

void EAR_MPI_Bcast_F_leave(void) {
    debug("<< F EAR_MPI_Bcast...............");
    after_mpi(Bcast);
}

void EAR_MPI_Bsend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Bsend...............");
    before_mpi(Bsend, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Bsend_F_leave(void) {
    debug("<< F EAR_MPI_Bsend...............");
    after_mpi(Bsend);
}

void EAR_MPI_Bsend_init_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Bsend_init...............");
    before_mpi(Bsend_init, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Bsend_init_F_leave(void) {
    debug("<< F EAR_MPI_Bsend_init...............");
    after_mpi(Bsend_init);
}

void EAR_MPI_Cancel_F_enter(MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Cancel...............");
    before_mpi(Cancel, (p2i)request, 0);
}

void EAR_MPI_Cancel_F_leave(void) {
    debug("<< F EAR_MPI_Cancel...............");
    after_mpi(Cancel);
}

void EAR_MPI_Cart_create_F_enter(MPI_Fint *comm_old, MPI_Fint *ndims, MPI3_CONST MPI_Fint *dims, MPI3_CONST MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Cart_create...............");
    before_mpi(Cart_create, (p2i)ndims, 0);
}

void EAR_MPI_Cart_create_F_leave(void) {
    debug("<< F EAR_MPI_Cart_create...............");
    after_mpi(Cart_create);
}

void EAR_MPI_Cart_sub_F_enter(MPI_Fint *comm, MPI3_CONST MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Cart_sub...............");
    before_mpi(Cart_sub, (p2i)remain_dims, 0);
}

void EAR_MPI_Cart_sub_F_leave(void) {
    debug("<< F EAR_MPI_Cart_sub...............");
    after_mpi(Cart_sub);
}

void EAR_MPI_Comm_create_F_enter(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Comm_create...............");
    before_mpi(Comm_create, (p2i)group, 0);
}

void EAR_MPI_Comm_create_F_leave(void) {
    debug("<< F EAR_MPI_Comm_create...............");
    after_mpi(Comm_create);
}

void EAR_MPI_Comm_dup_F_enter(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Comm_dup...............");
    before_mpi(Comm_dup, (p2i)newcomm, 0);
}

void EAR_MPI_Comm_dup_F_leave(void) {
    debug("<< F EAR_MPI_Comm_dup...............");
    after_mpi(Comm_dup);
}

void EAR_MPI_Comm_free_F_enter(MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Comm_free...............");
    before_mpi(Comm_free, (p2i)comm, 0);
}

void EAR_MPI_Comm_free_F_leave(void) {
    debug("<< F EAR_MPI_Comm_free...............");
    after_mpi(Comm_free);
}

void EAR_MPI_Comm_rank_F_enter(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Comm_rank...............");
    before_mpi(Comm_rank, (p2i)rank, 0);
}

void EAR_MPI_Comm_rank_F_leave(void) {
    debug("<< F EAR_MPI_Comm_rank...............");
    after_mpi(Comm_rank);
}

void EAR_MPI_Comm_size_F_enter(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Comm_size...............");
    before_mpi(Comm_size, (p2i)size, 0);
}

void EAR_MPI_Comm_size_F_leave(void) {
    debug("<< F EAR_MPI_Comm_size...............");
    after_mpi(Comm_size);
}

void EAR_MPI_Comm_spawn_F_enter(MPI3_CONST char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Comm_spawn...............");
    before_mpi(Comm_spawn, (p2i)command, 0);
}

void EAR_MPI_Comm_spawn_F_leave(void) {
    debug("<< F EAR_MPI_Comm_spawn...............");
    after_mpi(Comm_spawn);
}

void EAR_MPI_Comm_spawn_multiple_F_enter(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI3_CONST MPI_Fint *array_of_maxprocs, MPI3_CONST MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Comm_spawn_multiple...............");
    before_mpi(Comm_spawn_multiple, (p2i)array_of_commands, 0);
}

void EAR_MPI_Comm_spawn_multiple_F_leave(void) {
    debug("<< F EAR_MPI_Comm_spawn_multiple...............");
    after_mpi(Comm_spawn_multiple);
}

void EAR_MPI_Comm_split_F_enter(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Comm_split...............");
    before_mpi(Comm_split, (p2i)comm, 0);
}

void EAR_MPI_Comm_split_F_leave(void) {
    debug("<< F EAR_MPI_Comm_split...............");
    after_mpi(Comm_split);
}

void EAR_MPI_File_close_F_enter(MPI_File *fh, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_close...............");
    before_mpi(File_close, (p2i)fh, 0);
}

void EAR_MPI_File_close_F_leave(void) {
    debug("<< F EAR_MPI_File_close...............");
    after_mpi(File_close);
}

void EAR_MPI_File_read_F_enter(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_read...............");
    before_mpi(File_read, (p2i)buf, 0);
}

void EAR_MPI_File_read_F_leave(void) {
    debug("<< F EAR_MPI_File_read...............");
    after_mpi(File_read);
}

void EAR_MPI_File_read_all_F_enter(MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_read_all...............");
    before_mpi(File_read_all, (p2i)buf, 0);
}

void EAR_MPI_File_read_all_F_leave(void) {
    debug("<< F EAR_MPI_File_read_all...............");
    after_mpi(File_read_all);
}

void EAR_MPI_File_read_at_F_enter(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_read_at...............");
    before_mpi(File_read_at, (p2i)buf, 0);
}

void EAR_MPI_File_read_at_F_leave(void) {
    debug("<< F EAR_MPI_File_read_at...............");
    after_mpi(File_read_at);
}

void EAR_MPI_File_read_at_all_F_enter(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_read_at_all...............");
    before_mpi(File_read_at_all, (p2i)buf, 0);
}

void EAR_MPI_File_read_at_all_F_leave(void) {
    debug("<< F EAR_MPI_File_read_at_all...............");
    after_mpi(File_read_at_all);
}

void EAR_MPI_File_write_F_enter(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_write...............");
    before_mpi(File_write, (p2i)buf, 0);
}

void EAR_MPI_File_write_F_leave(void) {
    debug("<< F EAR_MPI_File_write...............");
    after_mpi(File_write);
}

void EAR_MPI_File_write_all_F_enter(MPI_File *fh, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_write_all...............");
    before_mpi(File_write_all, (p2i)buf, 0);
}

void EAR_MPI_File_write_all_F_leave(void) {
    debug("<< F EAR_MPI_File_write_all...............");
    after_mpi(File_write_all);
}

void EAR_MPI_File_write_at_F_enter(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_write_at...............");
    before_mpi(File_write_at, (p2i)buf, 0);
}

void EAR_MPI_File_write_at_F_leave(void) {
    debug("<< F EAR_MPI_File_write_at...............");
    after_mpi(File_write_at);
}

void EAR_MPI_File_write_at_all_F_enter(MPI_File *fh, MPI_Offset *offset, MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_File_write_at_all...............");
    before_mpi(File_write_at_all, (p2i)buf, 0);
}

void EAR_MPI_File_write_at_all_F_leave(void) {
    debug("<< F EAR_MPI_File_write_at_all...............");
    after_mpi(File_write_at_all);
}

void EAR_MPI_Finalize_F_enter(MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Finalize...............");
    before_finalize();
}

void EAR_MPI_Finalize_F_leave(void) {
    debug("<< F EAR_MPI_Finalize...............");
    after_finalize();
}

void EAR_MPI_Gather_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Gather...............");
    before_mpi(Gather, (p2i)sendbuf, 0);
}

void EAR_MPI_Gather_F_leave(void) {
    debug("<< F EAR_MPI_Gather...............");
    after_mpi(Gather);
}

void EAR_MPI_Gatherv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Gatherv...............");
    before_mpi(Gatherv, (p2i)sendbuf, 0);
}

void EAR_MPI_Gatherv_F_leave(void) {
    debug("<< F EAR_MPI_Gatherv...............");
    after_mpi(Gatherv);
}

void EAR_MPI_Get_F_enter(MPI_Fint *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Get...............");
    before_mpi(Get, (p2i)origin_addr, 0);
}

void EAR_MPI_Get_F_leave(void) {
    debug("<< F EAR_MPI_Get...............");
    after_mpi(Get);
}

void EAR_MPI_Ibsend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ibsend...............");
    before_mpi(Ibsend, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Ibsend_F_leave(void) {
    debug("<< F EAR_MPI_Ibsend...............");
    after_mpi(Ibsend);
}

void EAR_MPI_Init_F_enter(MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Init...............");
    before_init();
}

void EAR_MPI_Init_F_leave(void) {
    debug("<< F EAR_MPI_Init...............");
    after_init();
}

void EAR_MPI_Init_thread_F_enter(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Init_thread...............");
    before_init();
}

void EAR_MPI_Init_thread_F_leave(void) {
    debug("<< F EAR_MPI_Init_thread...............");
    after_init();
}

void EAR_MPI_Intercomm_create_F_enter(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *peer_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Intercomm_create...............");
    before_mpi(Intercomm_create,0 , 0);
}

void EAR_MPI_Intercomm_create_F_leave(void) {
    debug("<< F EAR_MPI_Intercomm_create...............");
    after_mpi(Intercomm_create);
}

void EAR_MPI_Intercomm_merge_F_enter(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintracomm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Intercomm_merge...............");
    before_mpi(Intercomm_merge, 0, 0);
}

void EAR_MPI_Intercomm_merge_F_leave(void) {
    debug("<< F EAR_MPI_Intercomm_merge...............");
    after_mpi(Intercomm_merge);
}

void EAR_MPI_Iprobe_F_enter(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Iprobe...............");
    before_mpi(Iprobe, (p2i)source, 0);
}

void EAR_MPI_Iprobe_F_leave(void) {
    debug("<< F EAR_MPI_Iprobe...............");
    after_mpi(Iprobe);
}

void EAR_MPI_Irecv_F_enter(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Irecv...............");
    before_mpi(Irecv, (p2i)buf, (p2i)source);
}

void EAR_MPI_Irecv_F_leave(void) {
    debug("<< F EAR_MPI_Irecv...............");
    after_mpi(Irecv);
}

void EAR_MPI_Irsend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Irsend...............");
    before_mpi(Irsend, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Irsend_F_leave(void) {
    debug("<< F EAR_MPI_Irsend...............");
    after_mpi(Irsend);
}

void EAR_MPI_Isend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Isend...............");
    before_mpi(Isend, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Isend_F_leave(void) {
    debug("<< F EAR_MPI_Isend...............");
    after_mpi(Isend);
}

void EAR_MPI_Issend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Issend...............");
    before_mpi(Issend, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Issend_F_leave(void) {
    debug("<< F EAR_MPI_Issend...............");
    after_mpi(Issend);
}

void EAR_MPI_Probe_F_enter(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Probe...............");
    before_mpi(Probe, (p2i)source, 0);
}

void EAR_MPI_Probe_F_leave(void) {
    debug("<< F EAR_MPI_Probe...............");
    after_mpi(Probe);
}

void EAR_MPI_Put_F_enter(MPI3_CONST void *origin_addr, MPI_Fint *origin_count, MPI_Fint *origin_datatype, MPI_Fint *target_rank, MPI_Fint *target_disp, MPI_Fint *target_count, MPI_Fint *target_datatype, MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Put...............");
    before_mpi(Put, (p2i)origin_addr, 0);
}

void EAR_MPI_Put_F_leave(void) {
    debug("<< F EAR_MPI_Put...............");
    after_mpi(Put);
}

void EAR_MPI_Recv_F_enter(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Recv...............");
    before_mpi(Recv, (p2i)buf, (p2i)source);
}

void EAR_MPI_Recv_F_leave(void) {
    debug("<< F EAR_MPI_Recv...............");
    after_mpi(Recv);
}

void EAR_MPI_Recv_init_F_enter(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Recv_init...............");
    before_mpi(Recv_init, (p2i)buf, (p2i)source);
}

void EAR_MPI_Recv_init_F_leave(void) {
    debug("<< F EAR_MPI_Recv_init...............");
    after_mpi(Recv_init);
}

void EAR_MPI_Reduce_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Reduce...............");
    before_mpi(Reduce, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Reduce_F_leave(void) {
    debug("<< F EAR_MPI_Reduce...............");
    after_mpi(Reduce);
}

void EAR_MPI_Reduce_scatter_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Reduce_scatter...............");
    before_mpi(Reduce_scatter, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Reduce_scatter_F_leave(void) {
    debug("<< F EAR_MPI_Reduce_scatter...............");
    after_mpi(Reduce_scatter);
}

void EAR_MPI_Request_free_F_enter(MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Request_free...............");
    before_mpi(Request_free, (p2i)request, 0);
}

void EAR_MPI_Request_free_F_leave(void) {
    debug("<< F EAR_MPI_Request_free...............");
    after_mpi(Request_free);
}

void EAR_MPI_Request_get_status_F_enter(MPI_Fint *request, int *flag, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Request_get_status...............");
    before_mpi(Request_get_status, (p2i)request, 0);
}

void EAR_MPI_Request_get_status_F_leave(void) {
    debug("<< F EAR_MPI_Request_get_status...............");
    after_mpi(Request_get_status);
}

void EAR_MPI_Rsend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Rsend...............");
    before_mpi(Rsend, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Rsend_F_leave(void) {
    debug("<< F EAR_MPI_Rsend...............");
    after_mpi(Rsend);
}

void EAR_MPI_Rsend_init_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Rsend_init...............");
    before_mpi(Rsend_init, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Rsend_init_F_leave(void) {
    debug("<< F EAR_MPI_Rsend_init...............");
    after_mpi(Rsend_init);
}

void EAR_MPI_Scan_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Scan...............");
    before_mpi(Scan, (p2i)sendbuf, 0);
}

void EAR_MPI_Scan_F_leave(void) {
    debug("<< F EAR_MPI_Scan...............");
    after_mpi(Scan);
}

void EAR_MPI_Scatter_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Scatter...............");
    before_mpi(Scatter, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Scatter_F_leave(void) {
    debug("<< F EAR_MPI_Scatter...............");
    after_mpi(Scatter);
}

void EAR_MPI_Scatterv_F_enter(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Scatterv...............");
    before_mpi(Scatterv, (p2i)sendbuf, 0);
}

void EAR_MPI_Scatterv_F_leave(void) {
    debug("<< F EAR_MPI_Scatterv...............");
    after_mpi(Scatterv);
}

void EAR_MPI_Send_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Send...............");
    before_mpi(Send, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Send_F_leave(void) {
    debug("<< F EAR_MPI_Send...............");
    after_mpi(Send);
}

void EAR_MPI_Send_init_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Send_init...............");
    before_mpi(Send_init, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Send_init_F_leave(void) {
    debug("<< F EAR_MPI_Send_init...............");
    after_mpi(Send_init);
}

void EAR_MPI_Sendrecv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Sendrecv...............");
    before_mpi(Sendrecv, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Sendrecv_F_leave(void) {
    debug("<< F EAR_MPI_Sendrecv...............");
    after_mpi(Sendrecv);
}

void EAR_MPI_Sendrecv_replace_F_enter(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Sendrecv_replace...............");
    before_mpi(Sendrecv_replace, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Sendrecv_replace_F_leave(void) {
    debug("<< F EAR_MPI_Sendrecv_replace...............");
    after_mpi(Sendrecv_replace);
}

void EAR_MPI_Ssend_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ssend...............");
    before_mpi(Ssend, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Ssend_F_leave(void) {
    debug("<< F EAR_MPI_Ssend...............");
    after_mpi(Ssend);
}

void EAR_MPI_Ssend_init_F_enter(MPI3_CONST void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ssend_init...............");
    before_mpi(Ssend_init, (p2i)buf, (p2i)dest);
}

void EAR_MPI_Ssend_init_F_leave(void) {
    debug("<< F EAR_MPI_Ssend_init...............");
    after_mpi(Ssend_init);
}

void EAR_MPI_Start_F_enter(MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Start...............");
    before_mpi(Start, (p2i)request, 0);
}

void EAR_MPI_Start_F_leave(void) {
    debug("<< F EAR_MPI_Start...............");
    after_mpi(Start);
}

void EAR_MPI_Startall_F_enter(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Startall...............");
    before_mpi(Startall, (p2i)count, 0);
}

void EAR_MPI_Startall_F_leave(void) {
    debug("<< F EAR_MPI_Startall...............");
    after_mpi(Startall);
}

void EAR_MPI_Test_F_enter(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Test...............");
    before_mpi(Test, (p2i)request, 0);
}

void EAR_MPI_Test_F_leave(void) {
    debug("<< F EAR_MPI_Test...............");
    after_mpi(Test);
}

void EAR_MPI_Testall_F_enter(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *flag, MPI_Fint *array_of_statuses, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Testall...............");
    before_mpi(Testall, 0, 0);
}

void EAR_MPI_Testall_F_leave(void) {
    debug("<< F EAR_MPI_Testall...............");
    after_mpi(Testall);
}

void EAR_MPI_Testany_F_enter(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Testany...............");
    before_mpi(Testany, 0, 0);
}

void EAR_MPI_Testany_F_leave(void) {
    debug("<< F EAR_MPI_Testany...............");
    after_mpi(Testany);
}

void EAR_MPI_Testsome_F_enter(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Testsome...............");
    before_mpi(Testsome, 0, 0);
}

void EAR_MPI_Testsome_F_leave(void) {
    debug("<< F EAR_MPI_Testsome...............");
    after_mpi(Testsome);
}

void EAR_MPI_Wait_F_enter(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Wait...............");
    before_mpi(Wait, (p2i)request, 0);
}

void EAR_MPI_Wait_F_leave(void) {
    debug("<< F EAR_MPI_Wait...............");
    after_mpi(Wait);
}

void EAR_MPI_Waitall_F_enter(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Waitall...............");
    before_mpi(Waitall, (p2i)count, 0);
}

void EAR_MPI_Waitall_F_leave(void) {
    debug("<< F EAR_MPI_Waitall...............");
    after_mpi(Waitall);
}

void EAR_MPI_Waitany_F_enter(MPI_Fint *count, MPI_Fint *requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Waitany...............");
    before_mpi(Waitany, 0, 0);
}

void EAR_MPI_Waitany_F_leave(void) {
    debug("<< F EAR_MPI_Waitany...............");
    after_mpi(Waitany);
}

void EAR_MPI_Waitsome_F_enter(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Waitsome...............");
    before_mpi(Waitsome, 0, 0);
}

void EAR_MPI_Waitsome_F_leave(void) {
    debug("<< F EAR_MPI_Waitsome...............");
    after_mpi(Waitsome);
}

void EAR_MPI_Win_complete_F_enter(MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Win_complete...............");
    before_mpi(Win_complete, 0, 0);
}

void EAR_MPI_Win_complete_F_leave(void) {
    debug("<< F EAR_MPI_Win_complete...............");
    after_mpi(Win_complete);
}

void EAR_MPI_Win_create_F_enter(void *base, MPI_Aint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Win_create...............");
    before_mpi(Win_create, 0, 0);
}

void EAR_MPI_Win_create_F_leave(void) {
    debug("<< F EAR_MPI_Win_create...............");
    after_mpi(Win_create);
}

void EAR_MPI_Win_fence_F_enter(MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Win_fence...............");
    before_mpi(Win_fence, 0, 0);
}

void EAR_MPI_Win_fence_F_leave(void) {
    debug("<< F EAR_MPI_Win_fence...............");
    after_mpi(Win_fence);
}

void EAR_MPI_Win_free_F_enter(MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Win_free...............");
    before_mpi(Win_free, 0, 0);
}

void EAR_MPI_Win_free_F_leave(void) {
    debug("<< F EAR_MPI_Win_free...............");
    after_mpi(Win_free);
}

void EAR_MPI_Win_post_F_enter(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Win_post...............");
    before_mpi(Win_post, 0, 0);
}

void EAR_MPI_Win_post_F_leave(void) {
    debug("<< F EAR_MPI_Win_post...............");
    after_mpi(Win_post);
}

void EAR_MPI_Win_start_F_enter(MPI_Fint *group, MPI_Fint *assert, MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Win_start...............");
    before_mpi(Win_start, 0, 0);
}

void EAR_MPI_Win_start_F_leave(void) {
    debug("<< F EAR_MPI_Win_start...............");
    after_mpi(Win_start);
}

void EAR_MPI_Win_wait_F_enter(MPI_Fint *win, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Win_wait...............");
    before_mpi(Win_wait, 0, 0);
}

void EAR_MPI_Win_wait_F_leave(void) {
    debug("<< F EAR_MPI_Win_wait...............");
    after_mpi(Win_wait);
}

#if MPI_VERSION >= 3
void EAR_MPI_Iallgather_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Iallgather...............");
    before_mpi(Iallgather, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Iallgather_F_leave(void) {
    debug("<< F EAR_MPI_Iallgather...............");
    after_mpi(Iallgather);
}

void EAR_MPI_Iallgatherv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcount, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Iallgatherv...............");
    before_mpi(Iallgatherv, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Iallgatherv_F_leave(void) {
    debug("<< F EAR_MPI_Iallgatherv...............");
    after_mpi(Iallgatherv);
}

void EAR_MPI_Iallreduce_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Iallreduce...............");
    before_mpi(Iallreduce, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Iallreduce_F_leave(void) {
    debug("<< F EAR_MPI_Iallreduce...............");
    after_mpi(Iallreduce);
}

void EAR_MPI_Ialltoall_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ialltoall...............");
    before_mpi(Ialltoall, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Ialltoall_F_leave(void) {
    debug("<< F EAR_MPI_Ialltoall...............");
    after_mpi(Ialltoall);
}

void EAR_MPI_Ialltoallv_F_enter(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *request, MPI_Fint *comm, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ialltoallv...............");
    before_mpi(Ialltoallv, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Ialltoallv_F_leave(void) {
    debug("<< F EAR_MPI_Ialltoallv...............");
    after_mpi(Ialltoallv);
}

void EAR_MPI_Ibarrier_F_enter(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ibarrier...............");
    before_mpi(Ibarrier, (p2i)request, 0);
}

void EAR_MPI_Ibarrier_F_leave(void) {
    debug("<< F EAR_MPI_Ibarrier...............");
    after_mpi(Ibarrier);
}

void EAR_MPI_Ibcast_F_enter(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ibcast...............");
    before_mpi(Ibcast, (p2i)buffer, 0);
}

void EAR_MPI_Ibcast_F_leave(void) {
    debug("<< F EAR_MPI_Ibcast...............");
    after_mpi(Ibcast);
}

void EAR_MPI_Igather_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Igather...............");
    before_mpi(Igather, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Igather_F_leave(void) {
    debug("<< F EAR_MPI_Igather...............");
    after_mpi(Igather);
}

void EAR_MPI_Igatherv_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Igatherv...............");
    before_mpi(Igatherv, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Igatherv_F_leave(void) {
    debug("<< F EAR_MPI_Igatherv...............");
    after_mpi(Igatherv);
}

void EAR_MPI_Ireduce_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ireduce...............");
    before_mpi(Ireduce, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Ireduce_F_leave(void) {
    debug("<< F EAR_MPI_Ireduce...............");
    after_mpi(Ireduce);
}

void EAR_MPI_Ireduce_scatter_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI3_CONST MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Ireduce_scatter...............");
    before_mpi(Ireduce_scatter, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Ireduce_scatter_F_leave(void) {
    debug("<< F EAR_MPI_Ireduce_scatter...............");
    after_mpi(Ireduce_scatter);
}

void EAR_MPI_Iscan_F_enter(MPI3_CONST void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Iscan...............");
    before_mpi(Iscan, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Iscan_F_leave(void) {
    debug("<< F EAR_MPI_Iscan...............");
    after_mpi(Iscan);
}

void EAR_MPI_Iscatter_F_enter(MPI3_CONST void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Iscatter...............");
    before_mpi(Iscatter, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Iscatter_F_leave(void) {
    debug("<< F EAR_MPI_Iscatter...............");
    after_mpi(Iscatter);
}

void EAR_MPI_Iscatterv_F_enter(MPI3_CONST void *sendbuf, MPI3_CONST MPI_Fint *sendcounts, MPI3_CONST MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror) {
    debug(">> F EAR_EAR_MPI_Iscatterv...............");
    before_mpi(Iscatterv, (p2i)sendbuf, (p2i)recvbuf);
}

void EAR_MPI_Iscatterv_F_leave(void) {
    debug("<< F EAR_MPI_Iscatterv...............");
    after_mpi(Iscatterv);
}
#endif

#pragma GCC visibility pop

