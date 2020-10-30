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
extern char input_buffer[SZ_BUFF_BIG];
extern char extra_buffer[SZ_BUFF_BIG];

// Sockets
extern socket_t *smets_srv;
extern socket_t *smets_mir;
extern socket_t *ssync_srv;
extern socket_t *ssync_mir;

// Descriptors
extern struct sockaddr_storage addr_cli;
extern fd_set fds_incoming;
extern fd_set fds_active;
extern int fd_cli;
extern int fd_min;
extern int fd_max;

// PID
extern process_data_t proc_data_srv;
extern process_data_t proc_data_mir;
extern pid_t server_pid;
extern pid_t mirror_pid;
extern pid_t others_pid;

// Mirroring
extern char master_host[SZ_NAME_MEDIUM];
extern char master_alia[SZ_NAME_MEDIUM];
extern char master_name[SZ_NAME_MEDIUM];
extern int master_iam;
extern int server_iam;
extern int mirror_iam;
extern int server_too;
extern int mirror_too;

// Data
extern time_t glb_time1[MAX_TYPES];
extern time_t glb_time2[MAX_TYPES];
extern time_t ins_time1[MAX_TYPES];
extern time_t ins_time2[MAX_TYPES];
extern size_t typ_sizof[MAX_TYPES];
extern uint   typ_prcnt[MAX_TYPES];
extern uint   typ_index[MAX_TYPES];
extern char  *typ_alloc[MAX_TYPES];
extern char  *sam_iname[MAX_TYPES];
extern ulong  sam_inmax[MAX_TYPES];
extern ulong  sam_index[MAX_TYPES];
extern uint   sam_recvd[MAX_TYPES];
extern uint   soc_accpt;
extern uint   soc_activ;
extern uint   soc_discn;
extern uint   soc_unkwn;
extern uint   soc_tmout;

// Times (alarms)
extern struct timeval timeout_insr;
extern struct timeval timeout_aggr;
extern struct timeval timeout_slct;
extern time_t time_insr;
extern time_t time_aggr;
extern time_t time_slct;

// State machine
extern int reconfiguring;
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

static void body_alarm(struct timeval *timeout_slct)
{
	if (timeout_slct->tv_sec == 0 && timeout_slct->tv_usec == 0)
	{
		time_substract_timeouts();

		if (timeout_aggr.tv_sec == 0)
		{
			if (sam_inmax[i_aggrs] > 0)
			{
				peraggr_t *p = (peraggr_t *) typ_alloc[i_aggrs];
				peraggr_t *q = (peraggr_t *) &p[sam_index[i_aggrs]];

				if (q->n_samples > 0)
				{
					verbose_xaxxw("completed the aggregation number '%lu' with energy '%lu'",
						 sam_index[i_aggrs], q->DC_energy);

					// Aggregation time done, so new aggregation incoming
					storage_sample_add(NULL, sam_inmax[i_aggrs], &sam_index[i_aggrs], NULL, 0, SYNC_AGGRS);

					// Initializing the new element
					q = (peraggr_t *) &p[sam_index[i_aggrs]];

					init_periodic_aggregation(q, master_name);
				}
			}

			//
			time_reset_timeout_aggr();
		}

		if (timeout_insr.tv_sec == 0)
		{
			// Synchronizing with the MAIN
			if (mirror_iam)
			{
				sync_ans_t answer;

				//TODO: MPKFA
				// Asking the question

				// In case of fail the mirror have to insert the data
				if(state_fail(sync_question(SYNC_ALL, veteran, &answer)))
				{
					insert_hub(SYNC_ALL, RES_TIME);
				// In case I'm veteran and the server is not
				} else if (!answer.veteran && veteran)
				{
					insert_hub(SYNC_ALL, RES_TIME);
				// In case of the answer is received just clear the data
				} else {
					reset_all();
				}
			} else {
				// Server normal insertion
				insert_hub(SYNC_ALL, RES_TIME);
			}

			// If server the time reset is do it after the insert
			time_reset_timeout_insr(0);

			// I'm a veteran
			veteran = 1;
		}

		time_reset_timeout_slct();
	}
}

#if 0
static int body_new_connection(int fd)
{
	int nc;

	nc  = !mirror_iam && (fd == smets_srv->fd || fd == ssync_srv->fd);
	nc |=  mirror_iam && (fd == smets_mir->fd);

	return nc;
}
#endif

static void body_connections()
{
	long ip;
	state_t s;
	int fd_old;
	int i;

	for(i = fd_min; i <= fd_max && listening && !updating; i++)
	{
		if (listening && FD_ISSET(i, &fds_incoming)) // we got one!!
		{
			// Handle new connections (just for TCP)
			if (sync_fd_is_new(i))
			{
				do {
					// i      -> listening descriptor
					// fd_cli -> current communications descriptor
					// fd_old -> previous communications descriptor
					s = sockets_accept(i, &fd_cli, &addr_cli);

					if (state_fail(s)) {
						break;
					}

					if (verbosity) {
						sockets_get_hostname(&addr_cli, extra_buffer, SZ_BUFF_BIG);
					}

					if (!sync_fd_is_mirror(i))
					{
						// Disconnect if previously connected
						sockets_get_ip(&addr_cli, &ip);

						if (sync_fd_exists(ip, &fd_old))
						{
							//log("multiple connections from host '%s', disconnecting previous", extra_buffer);
							if (verbosity) {
								verbose_xaxxw("disconnecting from host '%s' (host was previously connected)",
									extra_buffer);
							}

							sync_fd_disconnect(fd_old);
						}
					}

					// Test if the maximum number of connection has been reached
					// (soc_activ >= MAX_CONNECTIONS), and if fulfills that
					// condition disconnect inmediately.
					if (!sync_fd_is_mirror(i) && soc_activ >= MAX_CONNECTIONS)
					{
						if (verbosity) {
							verbose_xaxxw("disconnecting from host '%s' (maximum connections reached)",
								extra_buffer);
						}

						sync_fd_disconnect(fd_old);
					} else
					{
						sync_fd_add(fd_cli, ip);

						if (verbosity) {
							verbose_xaxxw("accepted fd '%d' from host '%s'", fd_cli, extra_buffer);
						}
					}
				} while(state_ok(s));
				// Handle data transfers
			}
			else
			{
				s = sockets_receive(i, &input_header, input_buffer, sizeof(input_buffer), 0);

				if (state_ok(s))
				{
					//
					storage_sample_receive(i, &input_header, input_buffer);
				}
				else
				{
					if (state_is(s, EAR_SOCK_DISCONNECTED)) {
						soc_discn += 1;
					} if (state_is(s, EAR_TIMEOUT)) {
						soc_tmout += 1;
					} else {
						soc_unkwn += 1;
					}

					if (verbosity)
					{
						sockets_get_hostname_fd(i, extra_buffer, SZ_BUFF_BIG);
	
						verbose_xaxxw("disconnecting from host %s (errno: '%d', strerrno: '%s')",
								extra_buffer, intern_error_num, intern_error_str);
					}

					sync_fd_disconnect(i);
				}
			}
		} // FD_ISSET
	}
}

static void body_insert()
{

}

void body()
{
	int s;

	// BODY
	while(listening)
	{
		fds_incoming = fds_active;

		if ((s = select(fd_max + 1, &fds_incoming, NULL, NULL, &timeout_slct)) == -1)
		{
			if (listening && !updating)
			{
				_error("during select (%s)", strerror(errno));
			}
		}

		body_alarm(&timeout_slct);

		body_connections();

		body_insert();

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
	sockets_dispose(smets_srv);
	sockets_dispose(smets_mir);
	sockets_dispose(ssync_srv);
	sockets_dispose(ssync_mir);

	FD_ZERO(&fds_incoming);
	FD_ZERO(&fds_active);

	// Freeing data
	free_cluster_conf(&conf_clus);

	for (i = 0; i < MAX_TYPES; ++i)
	{
		if (typ_alloc[i] != NULL)
		{
			free(typ_alloc[i]);
			typ_alloc[i] = NULL;
		}
	}

	// Reconfiguring
	if (reconfiguring)
	{
		sleep(20);
		reconfiguring = 0;
	}

	// Process PID cleaning
	if (server_iam && server_too) {
		process_pid_file_clean(&proc_data_srv);
	}
	if (mirror_iam && mirror_too) {
		process_pid_file_clean(&proc_data_mir);
	}

	//
	log_close();

	// End releasing
	releasing = 0;
}
