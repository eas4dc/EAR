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

#ifndef _MPI_SUPPORT_
#define _MPI_SUPPORT_

enum type_mpi_call {
    _Unknown=0,
    _Send=1,
    _Receive=2,
    _Barrier=3,
    _Wait=4,
    _Bcast=5,
    _All2All=6,
    _Gather=7,
    _Scatter=8,
    _Scan=9,
    _Reduce=10,
    _SendRecv=11,
    _Test=12,
    _Comm=13,
    _IO=14,
    _Blocking=0x10
};

typedef enum
{
	Unknown=0,
	Allgather = _Gather|_Blocking,
	Allgatherv = _Gather|_Blocking,
	Allreduce = _Reduce|_Blocking,
	Alltoall = _All2All|_Blocking,
	Alltoallv = _All2All|_Blocking,
	Barrier = _Barrier|_Blocking,
	Bcast = _Bcast|_Blocking,
	Bsend = _Send|_Blocking,
	Bsend_init = _Send,
	Cancel = _Unknown,
	Cart_create = _Comm|_Blocking,
	Cart_sub = _Comm,
	Comm_create = _Comm,
	Comm_dup = _Comm,
	Comm_free = _Comm,
	Comm_rank = _Comm,
	Comm_size = _Comm,
	Comm_spawn = _Comm,
	Comm_spawn_multiple = _Comm,
	Comm_split = _Comm,
	File_close = _IO,
	File_read = _IO,
	File_read_all = _IO,
	File_read_at = _IO,
	File_read_at_all = _IO,
	File_write = _IO,
	File_write_all = _IO,
	File_write_at = _IO,
	File_write_at_all = _IO,
	Finalize = _Unknown,
	Gather = _Gather|_Blocking,
	Gatherv = _Gather|_Blocking,
	Get = _Unknown,
	Iallgather = _Gather,
	Iallgatherv = _Gather,
	Iallreduce = _Reduce,
	Ialltoall = _All2All,
	Ialltoallv = _All2All,
	Ibarrier = _Barrier,
	Ibcast = _Bcast,
	Ibsend = _Send,
	Igather = _Gather,
	Igatherv = _Gather,
	Init = _Unknown,
	Init_thread = _Unknown,
	Intercomm_create = _Unknown,
	Intercomm_merge = _Unknown,
	Iprobe = _Test,
	Irecv = _Receive,
	Ireduce = _Reduce,
	Ireduce_scatter = _Reduce,
	Irsend = _Send,
	Iscan = _Scan,
	Iscatter = _Scatter,
	Iscatterv = _Scatter,
	Isend = _Send,
	Issend = _Send,
	Probe = _Test,
	Put = _Unknown,
	Recv = _Receive|_Blocking,
	Recv_init = _Receive,
	Reduce = _Reduce|_Blocking,
	Reduce_scatter = _Reduce|_Blocking,
	Request_free = _Unknown,
	Request_get_status = _Test,
	Rsend = _Send|_Blocking,
	Rsend_init = _Send,
	Scan = _Scan|_Blocking,
	Scatter = _Scatter|_Blocking,
	Scatterv = _Scatter|_Blocking,
	Send = _Send|_Blocking,
	Send_init = _Send,
	Sendrecv = _SendRecv|_Blocking,
	Sendrecv_replace = _SendRecv|_Blocking,
	Ssend = _Send|_Blocking,
	Ssend_init = _Send,
	Start = _Unknown,
	Startall = _Unknown,
	Test = _Test,
	Testall = _Test,
	Testany = _Test,
	Testsome = _Test,
	Wait = _Wait|_Blocking,
	Waitall = _Wait|_Blocking,
	Waitany = _Wait|_Blocking,
	Waitsome = _Wait|_Blocking,
	Win_complete = _Unknown,
	Win_create = _Unknown,
	Win_fence = _Unknown,
	Win_free = _Unknown,
	Win_post = _Unknown,
	Win_start = _Unknown,
	Win_wait = _Unknown,
	/* Added for comparison */
	Only_Blocking = _Blocking,
	Only_Scatter = _Scatter,
	Only_Gather = _Gather, 
	Only_All2All = _All2All,
	Only_Reduce = _Reduce,
	Only_Barrier = _Barrier,	
	Only_Receive = _Receive,
	Only_SendRecv = _SendRecv,
	Only_Bcast = _Bcast,	
	Only_Wait = _Wait,
	Only_Test = _Test,
	Not_implemented
} mpi_call;
#define p2i unsigned long

#define is_mpi_blocking(call) 	(call&Only_Blocking)
#define is_collective(call) 		((call == Only_Reduce) || (call == Only_Scatter) || (call == Only_Gather) || (call == Only_All2All) || (call == Only_Bcast))
#define is_synchronization(call) ((call == Only_Wait) || (call == Only_Barrier) || (call == Only_Test))
#define remove_blocking(call)		(call&~Only_Blocking)
#define is_recv(call)						((call == Only_Receive) || (call == Only_SendRecv))

int is_mpi_enabled();
int is_intelmpi();
int is_openmpi();
int openmpi_num_nodes();


#define BUSY_WAITING_BLOCK  0
#define YIELD_BLOCK					1
#endif
