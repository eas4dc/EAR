/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef LIBRARY_LOADER_MPIC_H
#define LIBRARY_LOADER_MPIC_H

#define MPIC_N 93

const char *mpic_names[] __attribute__((weak, visibility("hidden"))) =
{
    "MPI_Allgather",
    "MPI_Allgatherv",
    "MPI_Allreduce",
    "MPI_Alltoall",
    "MPI_Alltoallv",
    "MPI_Barrier",
    "MPI_Bcast",
    "MPI_Bsend",
    "MPI_Bsend_init",
    "MPI_Cancel",
    "MPI_Cart_create",
    "MPI_Cart_sub",
    "MPI_Comm_create",
    "MPI_Comm_dup",
    "MPI_Comm_free",
    "MPI_Comm_rank",
    "MPI_Comm_size",
    "MPI_Comm_spawn",
    "MPI_Comm_spawn_multiple",
    "MPI_Comm_split",
    "MPI_File_close",
    "MPI_File_read",
    "MPI_File_read_all",
    "MPI_File_read_at",
    "MPI_File_read_at_all",
    "MPI_File_write",
    "MPI_File_write_all",
    "MPI_File_write_at",
    "MPI_File_write_at_all",
    "MPI_Finalize",
    "MPI_Gather",
    "MPI_Gatherv",
    "MPI_Get",
    "MPI_Ibsend",
    "MPI_Init",
    "MPI_Init_thread",
    "MPI_Intercomm_create",
    "MPI_Intercomm_merge",
    "MPI_Iprobe",
    "MPI_Irecv",
    "MPI_Irsend",
    "MPI_Isend",
    "MPI_Issend",
    "MPI_Probe",
    "MPI_Put",
    "MPI_Recv",
    "MPI_Recv_init",
    "MPI_Reduce",
    "MPI_Reduce_scatter",
    "MPI_Request_free",
    "MPI_Request_get_status",
    "MPI_Rsend",
    "MPI_Rsend_init",
    "MPI_Scan",
    "MPI_Scatter",
    "MPI_Scatterv",
    "MPI_Send",
    "MPI_Send_init",
    "MPI_Sendrecv",
    "MPI_Sendrecv_replace",
    "MPI_Ssend",
    "MPI_Ssend_init",
    "MPI_Start",
    "MPI_Startall",
    "MPI_Test",
    "MPI_Testall",
    "MPI_Testany",
    "MPI_Testsome",
    "MPI_Wait",
    "MPI_Waitall",
    "MPI_Waitany",
    "MPI_Waitsome",
    "MPI_Win_complete",
    "MPI_Win_create",
    "MPI_Win_fence",
    "MPI_Win_free",
    "MPI_Win_post",
    "MPI_Win_start",
    "MPI_Win_wait"
    //#if MPI_VERSION >= 3
    ,
    "MPI_Iallgather",
    "MPI_Iallgatherv",
    "MPI_Iallreduce",
    "MPI_Ialltoall",
    "MPI_Ialltoallv",
    "MPI_Ibarrier",
    "MPI_Ibcast",
    "MPI_Igather",
    "MPI_Igatherv",
    "MPI_Ireduce",
    "MPI_Ireduce_scatter",
    "MPI_Iscan",
    "MPI_Iscatter",
    "MPI_Iscatterv",
    //#endif
};

#endif //LIBRARY_LOADER_MPIC_H