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

#include <database_cache/eardbd.h>
#include <database_cache/eardbd_body.h>
#include <database_cache/eardbd_sync.h>
#include <database_cache/eardbd_signals.h>
#include <database_cache/eardbd_storage.h>

// Configuration
extern cluster_conf_t conf_clus;

// Buffers
extern packet_header_t input_header;
extern char input_buffer[SZ_BUFFER];
extern char extra_buffer[SZ_BUFFER];

// Sockets
extern socket_t *socket_server;
extern socket_t *socket_mirror;
extern socket_t *socket_sync01;
extern socket_t *socket_sync02;

// Descriptors
extern struct sockaddr_storage addr_new;
extern fd_set fds_incoming;
extern fd_set fds_active;
extern int fd_new;
extern int fd_min;
extern int fd_max;

// PID
extern process_data_t proc_server;
extern process_data_t proc_mirror;
extern pid_t server_pid;
extern pid_t mirror_pid;
extern pid_t others_pid;

// Mirroring
extern char master_host[SZ_NAME_MEDIUM];
extern char master_nick[SZ_NAME_MEDIUM];
extern char master_name[SZ_NAME_MEDIUM];
extern int master_iam;
extern int server_iam;
extern int mirror_iam;
extern int server_enabled;
extern int mirror_enabled;

// Data
extern time_t time_recv1[EDB_NTYPES];
extern time_t time_recv2[EDB_NTYPES];
extern time_t time_insert1[EDB_NTYPES];
extern time_t time_insert2[EDB_NTYPES];
extern size_t type_sizeof[EDB_NTYPES];
extern uint   type_alloc_100[EDB_NTYPES];
extern char  *type_chunk[EDB_NTYPES];
extern char  *type_name[EDB_NTYPES];
extern ulong  type_alloc_len[EDB_NTYPES];
extern ulong  samples_index[EDB_NTYPES];
extern uint   samples_count[EDB_NTYPES];

// Sockets info
extern uint   sockets_accepted;
extern uint   sockets_online;
extern uint   sockets_disconnected;
extern uint   sockets_unrecognized;
extern uint   sockets_timeout;

// Times (alarms)
extern struct timeval timeout_insr;
extern struct timeval timeout_aggr;
extern struct timeval timeout_slct;
extern time_t time_insr;
extern time_t time_aggr;
extern time_t time_slct;

// State machine
extern int listening;
extern int releasing;
extern int exitting;
extern int updating;
extern int dreaming;
extern int veteran;
extern int forked;

// Extras
extern sigset_t sigset;

// Verbosity
extern char *str_who[2];
extern int verbosity;

/*
 *
 *
 *
 */

static void manage_alarms(struct timeval *timeout_slct)
{
	if (timeout_slct->tv_sec == 0 && timeout_slct->tv_usec == 0)
	{
		time_substract_timeouts();

		if (timeout_aggr.tv_sec == 0) {
			if (type_alloc_len[index_aggrs] > 0) {
				peraggr_t *p = (peraggr_t *) type_chunk[index_aggrs];
				peraggr_t *q = (peraggr_t *) &p[samples_index[index_aggrs]];

				if (q->n_samples > 0) {
					verb_who("completed the aggregation number '%lu' with energy '%lu'",
						 samples_index[index_aggrs], q->DC_energy);
					// Aggregation time done, so new aggregation incoming
					storage_sample_add(NULL, type_alloc_len[index_aggrs], &samples_index[index_aggrs], NULL, 0, EDB_SYNC_TYPE_AGGRS);
					// Initializing the new element
					q = (peraggr_t *) &p[samples_index[index_aggrs]];
					init_periodic_aggregation(q, master_name);
				}
			}
			//
			time_reset_timeout_aggr();
		}

		if (timeout_insr.tv_sec == 0) {
			// Synchronizing with the MAIN
			if (mirror_iam) {
				sync_answer_t answer;
				// Asking the question
				// In case of fail the mirror have to insert the data
				if(state_fail(sync_question(EDB_SYNC_ALL, veteran, &answer))) {
					insert_hub(EDB_SYNC_ALL, EDB_INSERT_BY_TIME);
				// In case I'm veteran and the server is not
				} else if (!answer.veteran && veteran) {
					insert_hub(EDB_SYNC_ALL, EDB_INSERT_BY_TIME);
				// In case of the answer is received just clear the data
				} else {
					reset_all();
				}
			} else {
				// Server normal insertion
				insert_hub(EDB_SYNC_ALL, EDB_INSERT_BY_TIME);
			}
			// If server the time reset is do it after the insert
			time_reset_timeout_insr(0);
			// I'm a veteran
			veteran = 1;
		}
		//
		time_reset_timeout_slct();
	}
}

#if 0
static int body_new_connection(int fd)
{
	int nc;
	nc  = !mirror_iam && (fd == socket_server->fd || fd == socket_sync01->fd);
	nc |=  mirror_iam && (fd == socket_mirror->fd);
	return nc;
}
#endif

static void manage_sockets()
{
	long ip;
	state_t s;
	int fd_old;
	int i;

	// Nothing to be done here
	if (updating) {
		return;
	}

	for(i = fd_min; i <= fd_max && listening; i++)
	{
		if (listening && FD_ISSET(i, &fds_incoming)) // we got one!!
		{
			// Handle new connections (just for TCP)
			if (sync_fd_is_new(i))
			{
				do {
					// i      -> listening descriptor
					// fd_new -> current communications descriptor
					// fd_old -> previous communications descriptor
					if (state_fail(s = sockets_accept(i, &fd_new, &addr_new))) {
						break;
					}
					if (verbosity) {
						sockets_get_hostname(&addr_new, extra_buffer, SZ_BUFFER);
					}
					if (!sync_fd_is_mirror(i)) {
						// Disconnect if previously connected
						sockets_get_ip(&addr_new, &ip);

						if (sync_fd_exists(ip, &fd_old)) {
							//log("multiple connections from host '%s', disconnecting previous", extra_buffer);
							if (verbosity) {
								verb_who("disconnecting from host '%s' (host was previously connected)",
									extra_buffer);
							}
							sync_fd_disconnect(fd_old);
						}
					}
					// Test if the maximum number of connection has been reached
					// (sockets_online >= EDB_MAX_CONNECTIONS), and if fulfills that
					// condition disconnect inmediately.
					if (!sync_fd_is_mirror(i) && sockets_online >= EDB_MAX_CONNECTIONS) {
						if (verbosity) {
							verb_who("disconnecting from host '%s' (maximum connections reached)",
								extra_buffer);
						}
						sync_fd_disconnect(fd_old);
					} else {
						sync_fd_add(fd_new, ip);
						if (verbosity) {
							verb_who("accepted fd '%d' from host '%s'", fd_new, extra_buffer);
						}
					}
				} while(state_ok(s));
				// Handle data transfers
			} else {
				if (state_ok(s = __sockets_recv(i, &input_header, input_buffer, sizeof(input_buffer), 0))) {
					//
					storage_sample_receive(i, &input_header, input_buffer);
				} else {
					if (state_is(s, EAR_SOCK_DISCONNECTED)) {
						sockets_disconnected += 1;
					} if (state_is(s, EAR_TIMEOUT)) {
						sockets_timeout += 1;
					} else {
						sockets_unrecognized += 1;
					}
					if (verbosity) {
						sockets_get_hostname_fd(i, extra_buffer, SZ_BUFFER);
						verb_who("disconnecting from host %s: %s)",
								extra_buffer, state_msg);
					}
					sync_fd_disconnect(i);
				}
			}
		} // FD_ISSET
	}
}

void body()
{
	int s;

	while(listening)
	{
		fds_incoming = fds_active;

		if ((s = select(fd_max + 1, &fds_incoming, NULL, NULL, &timeout_slct)) == -1) {
			if (listening && !updating) {
				edb_error("during select (%s)", strerror(errno));
			}
		}
		// Checks alarms states
		manage_alarms(&timeout_slct);
		// Checks new incoming connections or data
		manage_sockets();
		// Update finished
		updating = 0;
	}
}

void dream()
{
	while (dreaming) {
		sigsuspend(&sigset);
	}
}

void release()
{
	int i;

	// Socket closing
	for(i = fd_max; i >= fd_min && !listening; --i) {
		close(i);
	}
	// Cleaning sockets
	sockets_dispose(socket_server);
	sockets_dispose(socket_mirror);
	sockets_dispose(socket_sync01);
	sockets_dispose(socket_sync02);
	//
	FD_ZERO(&fds_incoming);
	FD_ZERO(&fds_active);
	// Freeing data
	free_cluster_conf(&conf_clus);

	for (i = 0; i < EDB_NTYPES; ++i)
	{
		if (type_chunk[i] != NULL)
		{
			free(type_chunk[i]);
			type_chunk[i] = NULL;
		}
	}
	// Process PID cleaning
	if (server_iam && server_enabled) {
		process_pid_file_clean(&proc_server);
	}
	if (mirror_iam && mirror_enabled) {
		process_pid_file_clean(&proc_mirror);
	}
	//
	log_close();
	// End releasing
	releasing = 0;
}
