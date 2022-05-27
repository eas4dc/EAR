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

//#define SHOW_DEBUGS 1

#include <errno.h>
#include <stdlib.h>
#include <common/system/lock.h>
#include <daemon/local_api/eard_api_rpc.h>

pthread_mutex_t  lock_rpc = PTHREAD_MUTEX_INITIALIZER;
__thread char   *rpc_buffer = NULL;
__thread size_t  rpc_size  = 0;
static int       rpc_disconnected;
extern int       ear_fd_req;
extern int       ear_fd_ack;

ulong create_sec_tag();

static state_t and_disconnect(state_t s, char *msg)
{
    eards_disconnect();
    rpc_disconnected = 1;
    return_print(s, msg, strerror(errno));
}

static state_t eard_send(int fd, uint call, state_t state, char *data, size_t size, char *error)
{
	char  *send_data = data;
	size_t send_size = size;
	size_t head_szof;
	eard_head_t head;
	int ret;
	debug("Preparing to send RPC %u of size %lu through FD %d", call, size, fd);
	// If we are sending an error or content
	if (state_fail(state)) {
		send_size = 0;
		if (error != NULL) {
			send_size = strlen(error);
			send_data = error;
		}
	}
	// Preparing the header
	head_szof         = sizeof(eard_head_t);
	head.req_service  = call;
	head.size         = send_size;
	head.sec          = create_sec_tag();
	head.state        = state;
	
	debug("Sending eard req RPC header with size %lu", head_szof);
	// Sending
	if ((ret = eards_write(fd, (char *) &head, head_szof)) != head_szof) {
		debug("RPC header recv failed with ret %d", ret);
		return and_disconnect(EAR_ERROR, "Problem when writing EARD pipe (%s)");
	}
	if (head.size > 0) {
		debug("Sending RPC data with size %lu", head.size);
		if ((ret = eards_write(fd, send_data, head.size)) != head.size) {
		    debug("RPC header recv failed with ret %d", ret);
			return and_disconnect(EAR_ERROR, "Problem when writing EARD pipe (%s)");
		}
	}
	return EAR_SUCCESS;
}

static void rpc_buffer_test(size_t size)
{
	if (rpc_buffer == NULL) {
		rpc_buffer = calloc(1, 8192);
		rpc_size = 8192;
	}
	if (size > rpc_size) {
		rpc_buffer = realloc(rpc_buffer, size);
		rpc_size = size;
	}
    memset(rpc_buffer, 0, rpc_size);
}

static state_t eard_recv(int fd, uint call, char *data, size_t *size, size_t expc_size)
{
	eard_head_t head;
	size_t head_szof;
	int ret;
	//
	head_szof  = sizeof(eard_head_t);
	debug("Waiting RPC header %u of expected size %lu through FD %d", call, expc_size, fd);
	// Receiving
	if ((ret = eards_read(fd, (char *) &head, head_szof, WAIT_DATA)) != head_szof) {
		debug("RPC header recv failed with ret %d", ret);
		return and_disconnect(EAR_ERROR, "Problem when reading EARD pipe (%s)");
	}
	if (head.req_service != call) {
		return_print(EAR_ERROR, "Expected header of service %u, received %lu",
			call, head.req_service);
	}
	if (head.size > 0) {
		if (data == rpc_buffer) {
			rpc_buffer_test(head.size);
		}
		if (state_fail(head.state)) {
			data = state_buffer;
		}
		debug("Waiting RPC data of size %lu", head.size);	
		if ((ret = eards_read(fd, data, head.size, WAIT_DATA)) != head.size) {
			debug("RPC body failed with ret %d", ret);
			return and_disconnect(EAR_ERROR, "Problem when reading EARD pipe (%s)");
		}
		if (state_fail(head.state)) {
			return_msg(head.state, state_buffer);
		}
		if (expc_size != 0 && head.size != expc_size) {
			return_msg(EAR_ERROR, "The read and expected size from the data pipe differs");
		}
	}
	if (size != NULL) {
		*size = head.size;
	}
	return EAR_SUCCESS;
}

static state_t data_check(char *data, size_t size)
{
	if ((size == 0 && data != NULL) || (size > 0 && data == NULL)) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	return EAR_SUCCESS;
}

static int rpc_is_connected()
{
    if (rpc_disconnected) {
        return 0;
    }
    return eards_connected();
}

static state_t static_rpc(uint call, char *data, size_t size, char *recv_data, size_t *recv_size, size_t expc_size)
{
	state_t s;
	if (!rpc_is_connected()) {
        return_msg(EAR_ERROR, "EARD disconnected");
    }
	// Testing data dissonances
	if (state_fail(s = data_check(data, size))) {
		return s;
	}
    if (state_fail(s = ear_trylock(&lock_rpc))) {
        return s;
    }
	// Sending header+data
	if (state_fail(s = eard_send(ear_fd_req, call, EAR_SUCCESS, data, size, NULL))) {
		return_unlock(s, &lock_rpc);
	}
	// Receiving the data and returning the RPC state
    s = eard_recv(ear_fd_ack, call, recv_data, recv_size, expc_size);
	return_unlock(s, &lock_rpc);
}

state_t eard_rpc(uint call, char *data, size_t size, char *recv_data, size_t expc_size)
{
    if (recv_data != NULL && expc_size > 0) {
        memset(recv_data, 0, expc_size);
    }
	return static_rpc(call, data, size, recv_data, NULL, expc_size);
}

state_t eard_rpc_buffered(uint call, char *data, size_t size, char **buffer, size_t *recv_size)
{
	if (buffer == NULL) {
		return_msg(EAR_ERROR, "passed buffer can't be NULL");
	}
	//
	rpc_buffer_test(size);
	*buffer = rpc_buffer;
	//
	return static_rpc(call, data, size, rpc_buffer, recv_size, 0);
}

state_t eard_rpc_answer(int fd, uint call, state_t s, char *data, size_t size, char *error)
{
	state_t s2;
	// Testing data dissonances
	if (state_fail(s2 = data_check(data, size))) {
		return s2;
	}
    if (state_fail(s2 = ear_trylock(&lock_rpc))) {
        return s2;
    }
    s2 = eard_send(fd, call, s, data, size, error);
	return_unlock(s2, &lock_rpc);
}

static state_t protected_read(int fd, char *buffer, int size, uint wait_mode)
{
    size_t aux_size;
    state_t s;

    if (state_fail(s = ear_trylock(&lock_rpc))) {
        return s;
    }
    aux_size = eards_read(fd, buffer, size, wait_mode);
    ear_unlock(&lock_rpc);

    if (aux_size != size) {
        return_print(EAR_ERROR, "problem when reading EARD pipe (%s)", strerror(errno));
    }
    return EAR_SUCCESS;
}

state_t eard_rpc_read_pending(int fd, char *recv_data, size_t read_size, size_t expc_size)
{
	state_t s;
	// Testing data dissonances
	if (state_fail(s = data_check(recv_data, read_size))) {
		return s;
	}
    if (state_fail(s = protected_read(fd, recv_data, read_size, WAIT_DATA))) {
        return s;
    }
	if (expc_size != 0 && read_size != expc_size) {
		return_msg(EAR_ERROR, "the read and expected size from the data pipe differs");
	}
	return EAR_SUCCESS;
}

state_t eard_rpc_clean(int fd, size_t size)
{
    state_t s;
	rpc_buffer_test(size);
	//
    if (state_fail(s = protected_read(fd, rpc_buffer, size, WAIT_DATA))) {
        return s;
    }
	return EAR_SUCCESS;	
}
