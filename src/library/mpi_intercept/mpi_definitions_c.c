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

#include <library/mpi_intercept/mpi_definitions_c.h>

const char *mpi_c_names[] = {
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
#if MPI_VERSION >= 3
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
	"MPI_Iscatterv"
#endif
 	,
};

#if 1
mpi_c_syms_t mpi_c_syms;
#else
mpi_c_syms_t mpi_c_syms = {
	.MPI_Allgather = PMPI_Allgather,
	.MPI_Allgatherv = PMPI_Allgatherv,
	.MPI_Allreduce = PMPI_Allreduce,
	.MPI_Alltoall = PMPI_Alltoall,
	.MPI_Alltoallv = PMPI_Alltoallv,
	.MPI_Barrier = PMPI_Barrier,
	.MPI_Bcast = PMPI_Bcast,
	.MPI_Bsend = PMPI_Bsend,
	.MPI_Bsend_init = PMPI_Bsend_init,
	.MPI_Cancel = PMPI_Cancel,
	.MPI_Cart_create = PMPI_Cart_create,
	.MPI_Cart_sub = PMPI_Cart_sub,
	.MPI_Comm_create = PMPI_Comm_create,
	.MPI_Comm_dup = PMPI_Comm_dup,
	.MPI_Comm_free = PMPI_Comm_free,
	.MPI_Comm_rank = PMPI_Comm_rank,
	.MPI_Comm_size = PMPI_Comm_size,
	.MPI_Comm_spawn = PMPI_Comm_spawn,
	.MPI_Comm_spawn_multiple = PMPI_Comm_spawn_multiple,
	.MPI_Comm_split = PMPI_Comm_split,
	.MPI_File_close = PMPI_File_close,
	.MPI_File_read = PMPI_File_read,
	.MPI_File_read_all = PMPI_File_read_all,
	.MPI_File_read_at = PMPI_File_read_at,
	.MPI_File_read_at_all = PMPI_File_read_at_all,
	.MPI_File_write = PMPI_File_write,
	.MPI_File_write_all = PMPI_File_write_all,
	.MPI_File_write_at = PMPI_File_write_at,
	.MPI_File_write_at_all = PMPI_File_write_at_all,
	.MPI_Finalize = PMPI_Finalize,
	.MPI_Gather = PMPI_Gather,
	.MPI_Gatherv = PMPI_Gatherv,
	.MPI_Get = PMPI_Get,
	.MPI_Ibsend = PMPI_Ibsend,
	.MPI_Init = PMPI_Init,
	.MPI_Init_thread = PMPI_Init_thread,
	.MPI_Intercomm_create = PMPI_Intercomm_create,
	.MPI_Intercomm_merge = PMPI_Intercomm_merge,
	.MPI_Iprobe = PMPI_Iprobe,
	.MPI_Irecv = PMPI_Irecv,
	.MPI_Irsend = PMPI_Irsend,
	.MPI_Isend = PMPI_Isend,
	.MPI_Issend = PMPI_Issend,
	.MPI_Probe = PMPI_Probe,
	.MPI_Put = PMPI_Put,
	.MPI_Recv = PMPI_Recv,
	.MPI_Recv_init = PMPI_Recv_init,
	.MPI_Reduce = PMPI_Reduce,
	.MPI_Reduce_scatter = PMPI_Reduce_scatter,
	.MPI_Request_free = PMPI_Request_free,
	.MPI_Request_get_status = PMPI_Request_get_status,
	.MPI_Rsend = PMPI_Rsend,
	.MPI_Rsend_init = PMPI_Rsend_init,
	.MPI_Scan = PMPI_Scan,
	.MPI_Scatter = PMPI_Scatter,
	.MPI_Scatterv = PMPI_Scatterv,
	.MPI_Send = PMPI_Send,
	.MPI_Send_init = PMPI_Send_init,
	.MPI_Sendrecv = PMPI_Sendrecv,
	.MPI_Sendrecv_replace = PMPI_Sendrecv_replace,
	.MPI_Ssend = PMPI_Ssend,
	.MPI_Ssend_init = PMPI_Ssend_init,
	.MPI_Start = PMPI_Start,
	.MPI_Startall = PMPI_Startall,
	.MPI_Test = PMPI_Test,
	.MPI_Testall = PMPI_Testall,
	.MPI_Testany = PMPI_Testany,
	.MPI_Testsome = PMPI_Testsome,
	.MPI_Wait = PMPI_Wait,
	.MPI_Waitall = PMPI_Waitall,
	.MPI_Waitany = PMPI_Waitany,
	.MPI_Waitsome = PMPI_Waitsome,
	.MPI_Win_complete = PMPI_Win_complete,
	.MPI_Win_create = PMPI_Win_create,
	.MPI_Win_fence = PMPI_Win_fence,
	.MPI_Win_free = PMPI_Win_free,
	.MPI_Win_post = PMPI_Win_post,
	.MPI_Win_start = PMPI_Win_start,
	.MPI_Win_wait = PMPI_Win_wait
#if PMPI_VERSION >= 3
	,
	.MPI_Iallgather = PMPI_Iallgather,
	.MPI_Iallgatherv = PMPI_Iallgatherv,
	.MPI_Iallreduce = PMPI_Iallreduce,
	.MPI_Ialltoall = PMPI_Ialltoall,
	.MPI_Ialltoallv = PMPI_Ialltoallv,
	.MPI_Ibarrier = PMPI_Ibarrier,
	.MPI_Ibcast = PMPI_Ibcast,
	.MPI_Igather = PMPI_Igather,
	.MPI_Igatherv = PMPI_Igatherv,
	.MPI_Ireduce = PMPI_Ireduce,
	.MPI_Ireduce_scatter = PMPI_Ireduce_scatter,
	.MPI_Iscan = PMPI_Iscan,
	.MPI_Iscatter = PMPI_Iscatter,
	.MPI_Iscatterv = PMPI_Iscatterv
#endif
	,
};
#endif
