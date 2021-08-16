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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <common/system/time.h>
#include <common/output/verbose.h>
#include <common/config/config_def.h>
#include <database_cache/eardbd.h>
#include <database_cache/eardbd_api.h>

static int server_enabled;
static int mirror_enabled;
static socket_t server_sock;
static socket_t mirror_sock;
static state_t server_s;
static state_t mirror_s;
static char *server_err;
static char *mirror_err;

/*
 *
 * Intern functions
 *
 */
int eardbd_is_initialized()
{
	return server_enabled || mirror_enabled;
}

static state_t state_global()
{
	if (state_fail(server_s)) {
		return_msg(server_s, server_err);
	}
	if (state_fail(mirror_s)) {
		return_msg(mirror_s, mirror_err);
	}
	return EAR_SUCCESS;
}

static state_t static_send(uint type, char *content, ssize_t size)
{
	packet_header_t header;

	// If not initialized
	if (!eardbd_is_initialized()) {
		return_msg(EAR_ERROR, "server and mirror are not enabled/initialized");
	}
	// Preparing the states
	server_s = EAR_SUCCESS;
	mirror_s = EAR_SUCCESS;
	// Cleaning headers
	sockets_header_clean(&header);
	sockets_header_update(&header);
	// Set the content
	header.content_type = type;
	header.content_size = size;
	// Debugging 
	debug("server %s:%d, enabled %d, fd %d", server_sock.host, server_sock.port, server_enabled, server_sock.fd);
	debug("mirror %s:%d, enabled %d, fd %d", mirror_sock.host, mirror_sock.port, mirror_enabled, mirror_sock.fd);
	// Sending the data through sockets
	if (server_enabled) {
		if (state_fail (server_s = __sockets_send(&server_sock, &header, content))) {
			server_err = state_msg;
		}
		debug("server send returned '%d'", server_s);
	}
	if (mirror_enabled) {
		if (state_fail(mirror_s = __sockets_send(&mirror_sock, &header, content))) {
			mirror_err = state_msg;
		}
		debug("mirror send returned '%d'", mirror_s);
	}
	return state_global();
}

static state_t static_connect(socket_t *socket, char *host, uint port, uint protocol)
{
	state_t s;
	//
	if (state_fail(s = sockets_init(socket, host, port, protocol))) {
		return s;
	}
	if (state_fail(s = sockets_socket(socket))) {
		return s;
	}
	if (protocol == TCP) {
		if (state_fail(s = sockets_connect(socket))) {
			return s;
		}
	}
	return EAR_SUCCESS;
}

static state_t static_disconnect(socket_t *socket, state_t s)
{
    sockets_dispose(socket);
    return s;
}

/*
 *
 * API functions
 *
 */

state_t eardbd_send_application(application_t *app)
{
	return static_send(EDB_TYPE_APP_MPI, (char *) app, sizeof(application_t));
}

state_t eardbd_send_periodic_metric(periodic_metric_t *metric)
{
	return static_send(EDB_TYPE_ENERGY_REP, (char *) metric, sizeof(periodic_metric_t));
}

state_t eardbd_send_event(ear_event_t *event)
{
	return static_send(EDB_TYPE_EVENT, (char *) event, sizeof(ear_event_t));
}

state_t eardbd_send_loop(loop_t *loop)
{
	return static_send(EDB_TYPE_LOOP, (char *) loop, sizeof(loop_t));
}

state_t eardbd_connect(cluster_conf_t *conf, my_node_conf_t *node)
{
	char *server_host = NULL;
	char *mirror_host = NULL;
	uint server_port = 0;
	uint mirror_port = 0;
	
	if (eardbd_is_initialized()) {
		return_msg(EAR_ERROR, "It's already initialized");
	}
#if 1
	// Configuring hosts and ports
	server_host = node->db_ip;
	mirror_host = node->db_sec_ip;
	server_port = conf->db_manager.tcp_port;
	mirror_port = conf->db_manager.sec_tcp_port;
#else
	server_port = 4711;
	mirror_port = 4712;
	server_host = "E7450";
	mirror_host = "E7450";
#endif
	server_enabled = (server_host != NULL) && (strlen(server_host) > 0) && (server_port > 0);
	mirror_enabled = (mirror_host != NULL) && (strlen(mirror_host) > 0) && (mirror_port > 0);
	// Preparing the states (because we are connecting by reconnect)
	server_s = EAR_ERROR;
	mirror_s = EAR_ERROR;
	//
	debug("server %s:%d, enabled %d", server_host, server_port, server_enabled);
	debug("mirror %s:%d, enabled %d", mirror_host, mirror_port, mirror_enabled);
	// Reconnect is connect in reality
	return eardbd_reconnect(conf, node);
}

state_t eardbd_reconnect(cluster_conf_t *conf, my_node_conf_t *node)
{
	static ullong time_rec = -1;
	static ullong time_now =  0;
	char *server_host;
	char *mirror_host;
	uint server_port;
	uint mirror_port;

	if (!eardbd_is_initialized()) {
		return_msg(EAR_ERROR, "Server and mirror are not enabled");
	}
	// Getting the current time to test if enough time has passed
	time_now = timestamp_getconvert(TIME_SECS);
	if((time_now - time_rec) < 5) {
		return_msg(EAR_NOT_READY, "To many reconnections, wait a little longer");
	}
	// Configuring again the hosts and ports
	server_host = node->db_ip;
	mirror_host = node->db_sec_ip;
	server_port = conf->db_manager.tcp_port;
	mirror_port = conf->db_manager.sec_tcp_port;
	// Debugging 
	debug("server %s:%d, enabled %d", server_host, server_port, server_enabled);
	debug("mirror %s:%d, enabled %d", mirror_host, mirror_port, mirror_enabled);
	// Using previous state error to reconnect server
	if (server_enabled && state_fail(server_s)) {
		// Cleaning connection
		sockets_dispose(&server_sock);
		// Connecting again
		if (state_fail(server_s = static_connect(&server_sock, server_host, server_port, TCP))) {
			debug("static_connect returned %d: %s", server_s, state_msg);
			server_err = state_msg;
			sockets_dispose(&server_sock);
		}
	}
	// Using previous state error to reconnect mirror
	if (mirror_enabled && state_fail(mirror_s)) {
		// Cleaning connection
		sockets_dispose(&mirror_sock);
		// Connecting again
		if (state_fail(mirror_s = static_connect(&mirror_sock, mirror_host, mirror_port, TCP))) {
			debug("static_connect returned %d: %s", mirror_s, state_msg);
			mirror_err = state_msg;
			sockets_dispose(&mirror_sock);
		}
	}
	// Debugging 
	debug("server %s:%d, enabled %d, fd %d", server_sock.host, server_sock.port, server_enabled, server_sock.fd);
	debug("mirror %s:%d, enabled %d, fd %d", mirror_sock.host, mirror_sock.port, mirror_enabled, mirror_sock.fd);
	// Saving the last time it tried to reconnect
	time_rec = timestamp_getconvert(TIME_SECS);

	return state_global();
}

state_t eardbd_disconnect()
{
	sockets_dispose(&server_sock);
	sockets_dispose(&mirror_sock);
	server_enabled = 0;
	mirror_enabled = 0;
	return EAR_SUCCESS;
}

state_t eardbd_fakefail(int who)
{
	if (who == 1) {
		server_s = EAR_ERROR;		
	} else {
		mirror_s = EAR_ERROR;
	}
	return EAR_SUCCESS;
}

state_t eardbd_status(char *host, uint port, eardbd_status_t *status)
{
	socket_t socket;
	size_t size;
	uint type;
	state_t s;

	// Connect
	if (state_fail(s = static_connect(&socket, host, port, TCP))) {
		return static_disconnect(&socket, s);
	}
	if (state_fail(s = sockets_send(socket.fd, EDB_TYPE_STATUS, NULL, 0, 0))) {
		return static_disconnect(&socket, s);
	}
	// Receiving (in blocking mode, but with 5 seconds of timeout)
	if (state_fail(s = sockets_timeout_set(socket.fd, 5))) {
		return static_disconnect(&socket, s);
	}
	if (state_fail(s = sockets_recv_header(socket.fd, &type, &size, NULL, 0))) {
		return static_disconnect(&socket, s);
	}
	if (state_fail(s = sockets_recv(socket.fd, (char *) status, size, 0))) {
		return static_disconnect(&socket, s);
	}
	return static_disconnect(&socket, EAR_SUCCESS);
}
