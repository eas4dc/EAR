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

static socket_t server_sock;
static socket_t mirror_sock;
static int enabled_server;
static int enabled_mirror;

/*
 *
 * Intern functions
 *
 */
int eardbd_is_initialized()
{
	return enabled_server || enabled_mirror;
}

static edb_state_t _send(uint content_type, char *content, ssize_t content_size)
{
	packet_header_t header;
	edb_state_t state;

	// If not initialized
	if (!eardbd_is_initialized()) {
		edb_state_return_msg(state, EAR_NOT_INITIALIZED, "server and mirror are not enabled");
	}

	//
	sockets_header_clean(&header);
	sockets_header_update(&header);
	header.content_type = content_type;
	header.content_size = content_size;
	edb_state_init(state, EAR_SUCCESS);

	if (enabled_server) {
		state.server = sockets_send(&server_sock, &header, content);

		debug("server '%s:%d', enabled %d, state %d, fd %d", server_sock.host,
			server_sock.port, enabled_server, state.server, server_sock.fd);
	}
	if (enabled_mirror) {
		state.mirror = sockets_send(&mirror_sock, &header, content);

		debug("mirror '%s:%d', enabled %d, state %d, fd %d", mirror_sock.host,
			mirror_sock.port, enabled_mirror, state.mirror, mirror_sock.fd);
	}

	return state;
}

static state_t _connect(socket_t *socket, char *host, uint port, uint protocol)
{
	state_t s;

	//
	s = sockets_init(socket, host, port, protocol);

	if (state_fail(s)) {
		return s;
	}

	//
	s = sockets_socket(socket);

	if (state_fail(s)) {
		return s;
	}

	//
	if (protocol == TCP)
	{
		s = sockets_connect(socket);

		if (state_fail(s)) {
			return s;
		}
	}

	return EAR_SUCCESS;
}

static void _disconnect(socket_t *socket)
{
    sockets_dispose(socket);
}

/*
 *
 * API functions
 *
 */

edb_state_t eardbd_send_application(application_t *app)
{
	return _send(CONTENT_TYPE_APM, (char *) app, sizeof(application_t));
}

edb_state_t eardbd_send_periodic_metric(periodic_metric_t *met)
{
	return _send(CONTENT_TYPE_PER, (char *) met, sizeof(periodic_metric_t));
}

edb_state_t eardbd_send_event(ear_event_t *eve)
{
	return _send(CONTENT_TYPE_EVE, (char *) eve, sizeof(ear_event_t));
}

edb_state_t eardbd_send_loop(loop_t *loop)
{
	return _send(CONTENT_TYPE_LOO, (char *) loop, sizeof(loop_t));
}

edb_state_t eardbd_send_ping()
{
	char ping[] = "ping";
	return _send(CONTENT_TYPE_PIN, (char *) ping, sizeof(ping));
}

edb_state_t eardbd_connect(cluster_conf_t *conf, my_node_conf_t *node)
{
	edb_state_t state;
	char *server_host = NULL;
	char *mirror_host = NULL;
	uint server_port = 0;
	uint mirror_port = 0;

	//
	if (eardbd_is_initialized()) {
		edb_state_return_msg(state, EAR_INITIALIZED, "it's already initialized");
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

	enabled_server = (server_host != NULL) && (strlen(server_host) > 0) && (server_port > 0);
	enabled_mirror = (mirror_host != NULL) && (strlen(mirror_host) > 0) && (mirror_port > 0);

	//
	debug("server '%s:%d', enabled %d", server_host, server_port, enabled_server);
	debug("mirror '%s:%d', enabled %d", mirror_host, mirror_port, enabled_mirror);

	// Maybe it's not enabled, but it's ok.
	edb_state_init(state, EAR_NOT_INITIALIZED);

	// Reconnect is connect
	state = eardbd_reconnect(conf, node, state);

	return state;
}

edb_state_t eardbd_reconnect(cluster_conf_t *conf, my_node_conf_t *node, edb_state_t state)
{
	static ullong time_rec = -1;
	static ullong time_now =  0;
	edb_state_t state_con;
	char *server_host;
	char *mirror_host;
	uint server_port;
	uint mirror_port;

	if (!eardbd_is_initialized()) {
		edb_state_return_msg(state_con, EAR_NOT_INITIALIZED, "server and mirror are not enabled");
	}

	//
	time_now = timestamp_getconvert(TIME_SECS);

	if((time_now - time_rec) < EARDBD_API_RETRY_SECS) {
		edb_state_return_msg(state_con, EAR_NOT_READY, "to many reconnections, wait a little longer");
	}

	// Configuring again the hosts and ports
	server_host = node->db_ip;
	mirror_host = node->db_sec_ip;
	server_port = conf->db_manager.tcp_port;
	mirror_port = conf->db_manager.sec_tcp_port;

	//
	edb_state_init(state_con, EAR_SUCCESS);

	if (enabled_server && state_fail(state.server))
	{
		sockets_close(&server_sock);
		state_con.server = _connect(&server_sock, server_host, server_port, TCP);

		//
		debug("server %s:%d, en %d, st %d, fd %d", server_sock.host, server_sock.port,
			  enabled_server, state_con.server, server_sock.fd);

		if (state_fail(state_con.server)) {
			_disconnect(&server_sock);
		}
	}
	if (enabled_mirror && state_fail(state.mirror))
	{
		sockets_close(&mirror_sock);
		state_con.mirror = _connect(&mirror_sock, mirror_host, mirror_port, TCP);

		//
		debug("mirror %s:%d, en %d, st %d, fd %d", mirror_sock.host, mirror_sock.port,
			  enabled_mirror, state_con.mirror, mirror_sock.fd);

		if (state_fail(state_con.mirror)) {
			_disconnect(&mirror_sock);
		}
	}

	//
	time_rec = timestamp_getconvert(TIME_SECS);

	return state_con;
}

edb_state_t eardbd_disconnect()
{
	edb_state_t state;

	if (!eardbd_is_initialized()) {
		edb_state_return(state, EAR_SUCCESS);
	}

	_disconnect(&server_sock);
	_disconnect(&mirror_sock);
	enabled_server = 0;
	enabled_mirror = 0;

	edb_state_return(state, EAR_SUCCESS);
}
