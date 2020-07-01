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

#include <library/mpi_intercept/mpi_definitions_f.h>

const char *mpi_f_names[] = {
	"mpi_allgather",
	"mpi_allgatherv",
	"mpi_allreduce",
	"mpi_alltoall",
	"mpi_alltoallv",
	"mpi_barrier",
	"mpi_bcast",
	"mpi_bsend",
	"mpi_bsend_init",
	"mpi_cancel",
	"mpi_cart_create",
	"mpi_cart_sub",
	"mpi_comm_create",
	"mpi_comm_dup",
	"mpi_comm_free",
	"mpi_comm_rank",
	"mpi_comm_size",
	"mpi_comm_spawn",
	"mpi_comm_spawn_multiple",
	"mpi_comm_split",
	"mpi_file_close",
	"mpi_file_read",
	"mpi_file_read_all",
	"mpi_file_read_at",
	"mpi_file_read_at_all",
	"mpi_file_write",
	"mpi_file_write_all",
	"mpi_file_write_at",
	"mpi_file_write_at_all",
	"mpi_finalize",
	"mpi_gather",
	"mpi_gatherv",
	"mpi_get",
	"mpi_ibsend",
	"mpi_init",
	"mpi_init_thread",
	"mpi_intercomm_create",
	"mpi_intercomm_merge",
	"mpi_iprobe",
	"mpi_irecv",
	"mpi_irsend",
	"mpi_isend",
	"mpi_issend",
	"mpi_probe",
	"mpi_put",
	"mpi_recv",
	"mpi_recv_init",
	"mpi_reduce",
	"mpi_reduce_scatter",
	"mpi_request_free",
	"mpi_request_get_status",
	"mpi_rsend",
	"mpi_rsend_init",
	"mpi_scan",
	"mpi_scatter",
	"mpi_scatterv",
	"mpi_send",
	"mpi_send_init",
	"mpi_sendrecv",
	"mpi_sendrecv_replace",
	"mpi_ssend",
	"mpi_ssend_init",
	"mpi_start",
	"mpi_startall",
	"mpi_test",
	"mpi_testall",
	"mpi_testany",
	"mpi_testsome",
	"mpi_wait",
	"mpi_waitall",
	"mpi_waitany",
	"mpi_waitsome",
	"mpi_win_complete",
	"mpi_win_create",
	"mpi_win_fence",
	"mpi_win_free",
	"mpi_win_post",
	"mpi_win_start",
	"mpi_win_wait"
	#if MPI_VERSION >= 3
 	,
	"mpi_iallgather",
	"mpi_iallgatherv",
	"mpi_iallreduce",
	"mpi_ialltoall",
	"mpi_ialltoallv",
	"mpi_ibarrier",
	"mpi_ibcast",
	"mpi_igather",
	"mpi_igatherv",
	"mpi_ireduce",
	"mpi_ireduce_scatter",
	"mpi_iscan",
	"mpi_iscatter",
	"mpi_iscatterv"
	#endif
	,
};

#if 1
mpi_f_syms_t mpi_f_syms;
#else
mpi_f_syms_t mpi_f_syms = {
	.mpi_allgather = mpi_allgather,
	.mpi_allgatherv = pmpi_allgatherv,
	.mpi_allreduce = pmpi_allreduce,
	.mpi_alltoall = pmpi_alltoall,
	.mpi_alltoallv = pmpi_alltoallv,
	.mpi_barrier = pmpi_barrier,
	.mpi_bcast = pmpi_bcast,
	.mpi_bsend = pmpi_bsend,
	.mpi_bsend_init = pmpi_bsend_init,
	.mpi_cancel = pmpi_cancel,
	.mpi_cart_create = pmpi_cart_create,
	.mpi_cart_sub = pmpi_cart_sub,
	.mpi_comm_create = pmpi_comm_create,
	.mpi_comm_dup = pmpi_comm_dup,
	.mpi_comm_free = pmpi_comm_free,
	.mpi_comm_rank = pmpi_comm_rank,
	.mpi_comm_size = pmpi_comm_size,
	.mpi_comm_spawn = pmpi_comm_spawn,
	.mpi_comm_spawn_multiple = pmpi_comm_spawn_multiple,
	.mpi_comm_split = pmpi_comm_split,
	.mpi_file_close = pmpi_file_close,
	.mpi_file_read = pmpi_file_read,
	.mpi_file_read_all = pmpi_file_read_all,
	.mpi_file_read_at = pmpi_file_read_at,
	.mpi_file_read_at_all = pmpi_file_read_at_all,
	.mpi_file_write = pmpi_file_write,
	.mpi_file_write_all = pmpi_file_write_all,
	.mpi_file_write_at = pmpi_file_write_at,
	.mpi_file_write_at_all = pmpi_file_write_at_all,
	.mpi_finalize = pmpi_finalize,
	.mpi_gather = pmpi_gather,
	.mpi_gatherv = pmpi_gatherv,
	.mpi_get = pmpi_get,
	.mpi_ibsend = pmpi_ibsend,
	.mpi_init = pmpi_init,
	.mpi_init_thread = pmpi_init_thread,
	.mpi_intercomm_create = pmpi_intercomm_create,
	.mpi_intercomm_merge = pmpi_intercomm_merge,
	.mpi_iprobe = pmpi_iprobe,
	.mpi_irecv = pmpi_irecv,
	.mpi_irsend = pmpi_irsend,
	.mpi_isend = pmpi_isend,
	.mpi_issend = pmpi_issend,
	.mpi_probe = pmpi_probe,
	.mpi_put = pmpi_put,
	.mpi_recv = pmpi_recv,
	.mpi_recv_init = pmpi_recv_init,
	.mpi_reduce = pmpi_reduce,
	.mpi_reduce_scatter = pmpi_reduce_scatter,
	.mpi_request_free = pmpi_request_free,
	.mpi_request_get_status = pmpi_request_get_status,
	.mpi_rsend = pmpi_rsend,
	.mpi_rsend_init = pmpi_rsend_init,
	.mpi_scan = pmpi_scan,
	.mpi_scatter = pmpi_scatter,
	.mpi_scatterv = pmpi_scatterv,
	.mpi_send = pmpi_send,
	.mpi_send_init = pmpi_send_init,
	.mpi_sendrecv = pmpi_sendrecv,
	.mpi_sendrecv_replace = pmpi_sendrecv_replace,
	.mpi_ssend = pmpi_ssend,
	.mpi_ssend_init = pmpi_ssend_init,
	.mpi_start = pmpi_start,
	.mpi_startall = pmpi_startall,
	.mpi_test = pmpi_test,
	.mpi_testall = pmpi_testall,
	.mpi_testany = pmpi_testany,
	.mpi_testsome = pmpi_testsome,
	.mpi_wait = pmpi_wait,
	.mpi_waitall = pmpi_waitall,
	.mpi_waitany = pmpi_waitany,
	.mpi_waitsome = pmpi_waitsome,
	.mpi_win_complete = pmpi_win_complete,
	.mpi_win_create = pmpi_win_create,
	.mpi_win_fence = pmpi_win_fence,
	.mpi_win_free = pmpi_win_free,
	.mpi_win_post = pmpi_win_post,
	.mpi_win_start = pmpi_win_start,
	.mpi_win_wait = pmpi_win_wait
	#if MPI_VERSION >= 3
	,
	.mpi_iallgather = pmpi_iallgather,
	.mpi_iallgatherv = pmpi_iallgatherv,
	.mpi_iallreduce = pmpi_iallreduce,
	.mpi_ialltoall = pmpi_ialltoall,
	.mpi_ialltoallv = pmpi_ialltoallv,
	.mpi_ibarrier = pmpi_ibarrier,
	.mpi_ibcast = pmpi_ibcast,
	.mpi_igather = pmpi_igather,
	.mpi_igatherv = pmpi_igatherv,
	.mpi_ireduce = pmpi_ireduce,
	.mpi_ireduce_scatter = pmpi_ireduce_scatter,
	.mpi_iscan = pmpi_iscan,
	.mpi_iscatter = pmpi_iscatter,
	.mpi_iscatterv = pmpi_iscatterv
	#endif
	,
};
#endif
