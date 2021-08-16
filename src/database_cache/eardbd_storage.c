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

//#define SOCKETS_DEBUG 1

#include <database_cache/eardbd.h>
#include <database_cache/eardbd_body.h>
#include <database_cache/eardbd_sync.h>
#include <database_cache/eardbd_signals.h>
#include <database_cache/eardbd_storage.h>
#if !EDB_OFFLINE
#include <common/database/db_helper.h>
#endif

#if EDB_OFFLINE
	#define db_batch_insert_applications(a, b);
	#define db_batch_insert_applications_no_mpi(a, b);
	#define db_batch_insert_applications_learning(a, b);
	#define db_batch_insert_loops(a, b);
	#define db_batch_insert_periodic_metrics(a, b);
	#define db_batch_insert_periodic_aggregations(a, b);
	#define db_batch_insert_ear_event(a, b);
#endif

#define EDB_SYNC_TYPE_ENERGY          0x001
#define EDB_SYNC_TYPE_APPS_MPI        0x002
#define EDB_SYNC_TYPE_APPS_SEQ        0x004
#define EDB_SYNC_TYPE_APPS_LEARN      0x008
#define EDB_SYNC_TYPE_LOOPS           0x010
#define EDB_SYNC_TYPE_EVENTS          0x020

#define sync_option(option, type) \
    ((option & type) > 0)

#define sync_option_m(option, type1, type2) \
    ((sync_option(option, type1)) || (sync_option(option, type2)))

// Configuration
extern cluster_conf_t conf_clus;

// Buffers
extern char input_buffer[SZ_BUFFER];
extern char extra_buffer[SZ_BUFFER];

// Mirroring
extern char master_host[SZ_NAME_MEDIUM]; 
extern char master_nick[SZ_NAME_MEDIUM];
extern char master_name[SZ_NAME_MEDIUM];
extern int master_iam;
extern int server_iam;
extern int mirror_iam;

// Time
extern struct timeval timeout_slct;
extern time_t time_slct;

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
extern uint sockets_accepted;
extern uint sockets_online;
extern uint sockets_disconnected;
extern uint sockets_unrecognized;
extern uint sockets_timeout;

// State machine
extern int listening;
extern int releasing;
extern int exitting;
extern int updating;
extern int dreaming;
extern int veteran;
extern int forked;

// Verbosity
extern char *str_who[2];
extern int verbosity;

// Status
extern eardbd_status_t status;

/*
 *
 * Time
 *
 */

void metrics_print()
{
	float time_insert = 0.0f;
	float time_recv = 0.0f;
	float percent = 0.0f;
	int n = 0;
	int i = 0;

	print_line();
	// Counting samples
	for (; i < EDB_NTYPES && n == 0; ++i) {
		n += (samples_index[i] > 0);
	}
	// If there are samples then we can print
	if (n > 0) {
	//
	tprintf_init(verb_channel, STR_MODE_DEF, "15 13 9 10 10");
	// Printing if there are more than 1 sample to insert per type.
	tprintf("sample (%d)||recv/alloc||%%||t. insr||t. recv", mirror_iam);
	tprintf("-----------||----------||--||-------||-------");

	for (i = 0; i < EDB_NTYPES; ++i)
	{
		if (samples_index[i] == 0) {
			continue;
		}
		time_insert = ((float) difftime(time_insert2[i], time_insert1[i]));
		time_recv   = ((float) difftime(time_recv2[i], time_recv1[i]));
		percent = 0.0f;

		if (type_alloc_len[i] > 0.0f) {
			percent = ((float) (samples_count[i]) / (float) (type_alloc_len[i])) * 100.0f;
		}
		tprintf("%s||%lu/%lu||%2.2f||%0.2fs||%0.2fs",
				type_name[i], samples_index[i], type_alloc_len[i], percent, time_insert, time_recv);
	}

	verbose(0, "--");
	}
	//
	verbose(0, "actv./accp. sockets: %u/%u", sockets_online, sockets_accepted);
	verbose(0, "disc./tout. sockets: %u/%u", sockets_disconnected, sockets_timeout);
	verbose(0, "recv. unknown samples: %u", sockets_timeout);
	print_line();

	sockets_accepted     = 0;
	sockets_disconnected = 0;
	sockets_unrecognized = 0;
	sockets_timeout      = 0;
}

/*
 *
 * Reset
 *
 */

static void reset_aggregations()
{
	peraggr_t *p = (peraggr_t *) type_chunk[index_aggrs];
	peraggr_t  q = (peraggr_t  ) p[samples_index[index_aggrs]];

	if (samples_index[index_aggrs] < type_alloc_len[index_aggrs] && q.n_samples > 0) {
		memcpy (p, &q, sizeof(periodic_aggregation_t));
	} else {
		init_periodic_aggregation(p, master_name);
	}
	// In case of aggregations, this value has to be hacked
	status.samples_recv[index_aggrs] = samples_index[index_aggrs];
	samples_index[index_aggrs] = 0;
}

static void reset_index(int index)
{
	time_recv1[index]   = 0;
	time_recv2[index]   = 0;
	time_insert1[index] = 0;
	time_insert2[index] = 0;
	samples_index[index] = 0;
	samples_count[index]  = 0;
}

void reset_all()
{
	int i;
	// Specific resets
	reset_aggregations();
	// Generic reset
	for (i = 0; i < EDB_NTYPES; ++i) {
		reset_index(i);
	}
}

/*
 *
 * Insert
 *
 */

static void insert_start(uint i)
{
	debug("trying to insert type %d (simple: '%d')",
		  i, !conf_clus.database.report_sig_detail);
	// Insert time update
	time(&time_insert1[i]);
}

static void insert_stop(uint i, state_t s)
{
	debug("called insert for type %d with result '%d'", i, s);
	// Insert time update
	time(&time_insert2[i]);
	// Status update (before reset)
	status.insert_states[i] = s;
	// Aggregations is a special case
	if (i != index_aggrs) {
		status.samples_recv[i] = samples_index[i];
	}
	// Reset samples
	reset_index(i);
}

static void insert_apps_mpi(int i)
{
	state_t s;
	if (samples_index[i] <= 0) {
		return;
	}
	insert_start(i);
	s = db_batch_insert_applications((application_t *) type_chunk[i], samples_index[i]);
	insert_stop(i, s);
}

static void insert_apps_non_mpi(int i)
{
	state_t s;
	if (samples_index[i] <= 0) {
		return;
	}
	insert_start(i);
	s = db_batch_insert_applications_no_mpi((application_t *) type_chunk[i], samples_index[i]);
	insert_stop(i, s);
}

static void insert_apps_learning(int i)
{
	state_t s;
	if (samples_index[i] <= 0) {
		return;
	}
	insert_start(i);
	s = db_batch_insert_applications_learning((application_t *) type_chunk[i], samples_index[i]);
	insert_stop(i, s);
}

static void insert_loops(int i)
{
	state_t s;
	if (samples_index[i] <= 0) {
		return;
	}
	insert_start(i);
	s = db_batch_insert_loops((loop_t *) type_chunk[i], samples_index[i]);
	insert_stop(i, s);
}

static void insert_energy(int i)
{
	state_t s;
	if (samples_index[i] <= 0) {
		return;
	}
	insert_start(i);
	s = db_batch_insert_periodic_metrics((periodic_metric_t *) type_chunk[i], samples_index[i]);
	insert_stop(i, s);
}

static void insert_aggregations(int i)
{
	state_t s;
	if (samples_index[i] <= 0) {
		return;
	}
	insert_start(i);
	s = db_batch_insert_periodic_aggregations((periodic_aggregation_t *) type_chunk[i], samples_index[i]);
	reset_aggregations();
	insert_stop(i, s);
}

static void insert_events(int i)
{
	state_t s;
	if (samples_index[i] <= 0) {
		return;
	}
	insert_start(i);
	s = db_batch_insert_ear_event((ear_event_t *) type_chunk[i], samples_index[i]);
	insert_stop(i, s);
}

void insert_hub(uint option, uint reason)
{
	verb_who("looking for possible DB insertion (type 0x%x, reason 0x%x)",
				  option, reason);
	// Why print is set before inserts?
	metrics_print();
	// Insert one by one samples
	if (sync_option_m(option, EDB_SYNC_TYPE_APPS_MPI, EDB_SYNC_ALL)) {
		insert_apps_mpi(index_appsm);
	}
	if (sync_option_m(option, EDB_SYNC_TYPE_APPS_SEQ, EDB_SYNC_ALL)) {
		insert_apps_non_mpi(index_appsn);
	}
	if (sync_option_m(option, EDB_SYNC_TYPE_APPS_LEARN, EDB_SYNC_ALL)) {
		insert_apps_learning(index_appsl);
	}
	if (sync_option_m(option, EDB_SYNC_TYPE_ENERGY, EDB_SYNC_ALL)) {
		insert_energy(index_enrgy);
	}
	if (sync_option_m(option, EDB_SYNC_TYPE_AGGRS, EDB_SYNC_ALL)) {
		insert_aggregations(index_aggrs);
	}
	if (sync_option_m(option, EDB_SYNC_TYPE_EVENTS, EDB_SYNC_ALL)) {
		insert_events(index_evens);
	}
	if (sync_option_m(option, EDB_SYNC_TYPE_LOOPS, EDB_SYNC_ALL)) {
		insert_loops(index_loops);
	}
}

/*
 *
 * Sample add
 *
 */

void storage_sample_add(char *buf, ulong len, ulong *index, char *cnt, size_t siz, uint opt)
{
	ulong address = siz * (*index);

	if (len == 0) {
		return;
	}
	if (cnt != NULL) {
		memcpy (&buf[address], cnt, siz);
	}
	//
	*index += 1;

	if (*index == len) {
		// If server just insert.
		if(server_iam) {
			insert_hub(opt, EDB_INSERT_BY_FULL);
		// If mirror synchronize with server.
		} else if (state_fail(sync_question(opt, veteran, NULL))) {
			// If fails, then insert.
			insert_hub(opt, EDB_INSERT_BY_FULL);
		}
	}
}

/*
 *
 * Incoming data
 *
 */

static int storage_type_extract(packet_header_t *header, char *content)
{
	application_t *app;
	uint type;

	//
	type = header->content_type;

	if (type != EDB_TYPE_APP_MPI) {
		return type;
	}
	//
	app = (application_t *) content;

	if (app->is_learning) {
		return EDB_TYPE_APP_LEARN;
	} else if (app->is_mpi) {
		return EDB_TYPE_APP_MPI;
	} else {
		return EDB_TYPE_APP_SEQ;
	}
	return -1;
}

static int storage_index_extract(int type, char **name)
{
	switch (type)
	{
		case EDB_TYPE_APP_MPI:
			*name = type_name[index_appsm];
			return index_appsm;
		case EDB_TYPE_APP_SEQ:
			*name = type_name[index_appsn];
			return index_appsn;
		case EDB_TYPE_APP_LEARN:
			*name = type_name[index_appsl];
			return index_appsl;
		case EDB_TYPE_ENERGY_REP:
			*name = type_name[index_enrgy];
			return index_enrgy;
		case EDB_TYPE_EVENT:
			*name = type_name[index_evens];
			return index_evens;
		case EDB_TYPE_LOOP:
			*name = type_name[index_loops];
			return index_loops;
		case EDB_TYPE_ENERGY_AGGR:
			*name = type_name[index_aggrs];
			return index_aggrs;
		case EDB_TYPE_SYNC_QUESTION:
			*name = "sync_question";
			return -1;
		case EDB_TYPE_SYNC_ANSWER:
			*name = "sync_answer";
			return -1;
		case EDB_TYPE_STATUS:
			*name = "status";
			return -1;
		default:
			*name = "unknown";
			return -1;
	}
}

void storage_sample_receive(int fd, packet_header_t *header, char *content)
{
	char *name;
	state_t s;
	int index;
	int type;

	// Data extraction
	type  = storage_type_extract(header, content);
	index = storage_index_extract(type, &name);

	if (verbosity) {
		#if SOCKETS_DEBUG
		verb_who("received from host '%s' an object of type: '%s' (t: '%d', i: '%d')",
			header->host_src, name, type, index);
		#else
		verb_who("received object of type: '%s' (t: '%d', i: '%d')",
			name, type, index);
		#endif
	}

	//TODO:
	// Currently, samples_index and sam_recv are the same. In the near future
	// a struct for every type has to be made. This type have to include
	// these things:
	//		- an index to the position of the empty value in the allocation
	//		- a maximum of the allocation
	//		- an ulong of the sizeof the type
	//		- a pointer to the allocation
	//		- a pointer to the MySQL insert function
	//		- a pointer to the string of the name of the type
	//		- a time_t of the insertion time
	//		- a value if the metric has overflow within a insert time
	//		- a value of the sync option

	// Storage
	if (type == EDB_TYPE_APP_MPI) {
		//print_application_fd_binary(0, (application_t *) content);
		storage_sample_add(type_chunk[index], type_alloc_len[index],
		   &samples_index[index], content, type_sizeof[index], EDB_SYNC_TYPE_APPS_MPI);
	}
	else if (type == EDB_TYPE_APP_SEQ) {
		//print_application_fd_binary(0, (application_t *) content);
		storage_sample_add(type_chunk[index], type_alloc_len[index],
		   &samples_index[index], content, type_sizeof[index], EDB_SYNC_TYPE_APPS_SEQ);
	}
	else if (type == EDB_TYPE_APP_LEARN) {
		//print_application_fd_binary(0, (application_t *) content);
		storage_sample_add(type_chunk[index], type_alloc_len[index],
			&samples_index[index], content, type_sizeof[index], EDB_SYNC_TYPE_APPS_LEARN);
	}
	else if (type == EDB_TYPE_EVENT) {
		storage_sample_add(type_chunk[index], type_alloc_len[index],
			&samples_index[index], content, type_sizeof[index], EDB_SYNC_TYPE_EVENTS);
	}
	else if (type == EDB_TYPE_LOOP) {
		storage_sample_add(type_chunk[index], type_alloc_len[index],
			&samples_index[index], content, type_sizeof[index], EDB_SYNC_TYPE_LOOPS);
	}
	else if (type == EDB_TYPE_ENERGY_REP) {
		peraggr_t *p = (peraggr_t *) type_chunk[index_aggrs];
		peraggr_t *q = (peraggr_t *) &p[samples_index[index_aggrs]];
		periodic_metric_t *met = (periodic_metric_t *) content;
		// Getting current time
		//ullong current_time = timestamp_getconvert(TIME_SECS);
		ullong current_time = (ullong) time(NULL);
		// Add sample to the aggregation
		add_periodic_aggregation(q, met->DC_energy, current_time, current_time);
		// Add sample to the energy array
		storage_sample_add(type_chunk[index], type_alloc_len[index],
			&samples_index[index], content, type_sizeof[index], EDB_SYNC_TYPE_ENERGY);
	}
	else if (type == EDB_TYPE_SYNC_QUESTION) {
		sync_question_t *question = (sync_question_t *) content;
		if (veteran || !question->veteran) {
			// Passing the question option
			insert_hub(question->sync_option, EDB_INSERT_BY_SYNC);
		}
		// Answering the mirror question
		sync_answer(fd, veteran);
		// In case it is a full sync the sync time is resetted before the answer
		// with a very small offset (5 second is enough)
		if (sync_option(question->sync_option, EDB_SYNC_ALL))
		{
			/// We get the timeout passed since the 'time_slct' got its value
			// and added to 'timeout_insr', because when 'time_slct' would be
			// substracted from both 'timeout_insr' and 'timeout_aggr', it
			// also be substracted the passed time.
			time_t timeout_passed = time_slct - timeout_slct.tv_sec;
			time_reset_timeout_insr(timeout_passed + 5);
			// I'm a veteran
			veteran = 1;
		}
	}
	else if (type == EDB_TYPE_STATUS) {
		// Returning the data
		status.sockets_online = sockets_online;
		if (state_fail(s = sockets_send(fd, EDB_TYPE_STATUS, (char *) &status, sizeof(eardbd_status_t), 0))) {
			error("Error when sending status: %s", state_msg);
		}
	}
	// Metrics
	if (index == -1) {
		return;
	}
	//	
	if (samples_count[index] == 0) {
		time(&time_recv1[index]);
	}
	//
	time(&time_recv2[index]);
	//
	samples_count[index] += 1;
}
