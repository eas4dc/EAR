/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <common/config.h>
#include <library/metrics/dlb_talp_lib.h>

#if HAVE_DLB_TALP_H
#include <dlb_talp.h>
#else
#include <stdint.h>
#endif
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>	/* For mode constants */
#include <fcntl.h>		/* For O_* constants */
#include <semaphore.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include <common/types/generic.h>
#include <common/system/monitor.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <library/loader/module_mpi.h>

#define TALP_SHM_TIMEOUT 5 // in seconds
#define TALP_SHM_NAME "/earl_dlb_talp.dat"
#define TALP_SEM_NAME "/earl_dlb_talp_sem"

/* unlink 'name' shared object if the local
 * variable 'master_process' is neq to 0. */
#define master_unlink(name) \
	do {\
		if (master_process) {\
			shm_unlink(name);\
		}\
	} while (0);

/* Unmaps shared memory pointed by 'addr' and
 * calls master_unlink (defined above). */
#define unmap_and_unlink(addr, size, name) \
	do {\
		munmap(addr, size);\
		master_unlink(name);\
	} while (0);

#if !HAVE_DLB_TALP_H

#define DLB_MonitoringRegionRegister(...) NULL;
#define DLB_TALP_CollectPOPNodeMetrics(...) -1;
#define DLB_MonitoringRegionStart(...) NULL;
#define DLB_MonitoringRegionReset(...) NULL;
#define DLB_MonitoringRegionReport(...) NULL;
#define DLB_MonitoringRegionStop(...) NULL;


enum { DLB_MONITOR_NAME_MAX = 128 };

typedef void *dlb_monitor_t;

typedef struct dlb_node_metrics_t {
    char    name[DLB_MONITOR_NAME_MAX];
    int     node_id;
    int     processes_per_node;
    int64_t total_useful_time;
    int64_t total_mpi_time;
    int64_t max_useful_time;
    int64_t max_mpi_time;
    float   parallel_efficiency;
    float   communication_efficiency;
    float   load_balance;
} dlb_node_metrics_t;
#endif // HAVE_DLB_TALP_H

typedef struct earl_talp_shared_data
{
	uint must_call_libdlb; // A control variable to rule processes to call DLB blocking calls.
	uint max_libdlb_calls; // The maximum time DLB TALP API was called.
	uint processes_ready; // A control variable useful for process synch.
} earl_talp_shd_t;

typedef struct earl_dlb_talp_metrics
{
	timestamp_t timestamp;	
	dlb_node_metrics_t dlb_node_metrics;
} earl_dlb_talp_metrics_t;


static suscription_t* earl_talp;
static FILE *talp_report = NULL;

static dlb_monitor_t *dlb_loop_monitor = NULL;
static dlb_monitor_t *dlb_app_monitor = NULL;

static uint talp_monitor = 1;

static int  libdlb_calls_cnt; // Counter for DLB API's blocking functions calls.

static earl_talp_shd_t *dlb_talp_data;
static sem_t *earl_talp_sem;

static earl_dlb_talp_metrics_t curr_talp_data;


/** Creates a subscription to the EARL periodic monitor.
 * Burst and relax timing is the same (10 seconds).
 * Init subscription function is earl_periodic_actions_talp_init, and the main rutine is
 * earl_periodic_actions_talp.
 * If an error occurred, state_msg can be used to report the cause of error.
 *
 * \param[out] sus The address of the created subscription.
 *
 * \return EAR_ERROR if an error ocurred registering the subscription.
 * \return EAR_SUCCESS otherwise. */
static state_t talp_monitor_create(suscription_t *sus);


/** \todo */
static state_t talp_monitor_finalize();


/** \todo */
static state_t talp_monitor_periodic_action(void *p);


/** \todo  */
static state_t talp_monitor_init(void *p);


/** Reports DLB's TALP API node collected metrics.
 * Just the master process prints something.
 *
 * \param node_metrics The address where target data is stored. */
static void report_talp_node_metrics(dlb_node_metrics_t *node_metrics);


static state_t collect_talp_node_metrics(dlb_node_metrics_t *dst, dlb_monitor_t *region_monitor);


static void collect_and_report_talp_node_metrics();


/** \todo  */
static state_t talp_init_shared_area(uint master_process, uint process_count);


/** Unmap this module's shared memory objects. If \p master_process, also unlinks
 * the shared memory object. */
static void talp_shared_area_dispose(uint master_process);


/** Truncates the file referenced by \p fd to a size of \p length bytes
 * just if \p master_process is non-zero.
 * \return EAR_ERROR on system call error.
 * \return EAR_SUCCESS otherwise. */
static state_t mstr_trunc_sm(uint master_process, int fd, off_t length);


state_t earl_dlb_talp_init()
{
  if (state_fail(talp_monitor_create(earl_talp)))
  {
		char err_msg[128];
    snprintf(err_msg, sizeof(err_msg), "Error creating TALP monitor");
  }
  return EAR_SUCCESS;
}


state_t earl_dlb_talp_dispose()
{
  return talp_monitor_finalize();
}


state_t earl_dlb_talp_read(earl_talp_t *earl_talp_data)
{
	if (earl_talp_data)
	{
		earl_talp_data->timestamp = curr_talp_data.timestamp;
		earl_talp_data->load_balance = curr_talp_data.dlb_node_metrics.load_balance;
		earl_talp_data->parallel_efficiency = curr_talp_data.dlb_node_metrics.parallel_efficiency;

		return EAR_SUCCESS;
	} else
	{
		return EAR_ERROR;
	}
}


static state_t talp_monitor_create(suscription_t *sus)
{
  if (module_mpi_is_enabled())
  {
		/** Init subscription function is talp_monitor_init,
		 * and the main rutine is talp_monitor_periodic_action. */
    sus = suscription();

    sus->call_init = talp_monitor_init;
    sus->call_main = talp_monitor_periodic_action;

    sus->time_relax = 10000; // 10 seconds
    sus->time_burst = 10000;

    if (state_fail(monitor_register(sus)))
    {
      char ret_msg[128];

      int ret = snprintf(ret_msg, sizeof ret_msg, "Registering subscription (%s)", state_msg);
      return_msg(EAR_ERROR, ret_msg);
    }

    monitor_relax(sus);
    verbose_info2("DLB-TALP subscription created.");
  }

  return EAR_SUCCESS;
}


static state_t talp_monitor_init(void *p)
{
	uint master = masters_info.my_master_rank >= 0;
	uint process_cnt = lib_shared_region->num_processes;

	/* First init the shared area used by this module. */
	if (state_fail(talp_init_shared_area(master, process_cnt)))
	{
		talp_monitor = 0;
		verbose_error("Creating shared area.");
		return EAR_ERROR;
	}

	if (HAVE_DLB_TALP_H)
	{
		dlb_app_monitor = DLB_MonitoringRegionRegister("app");
		dlb_loop_monitor = DLB_MonitoringRegionRegister("loop");
	}

	if (dlb_app_monitor && dlb_loop_monitor)
	{
		DLB_MonitoringRegionStart(dlb_app_monitor);
		DLB_MonitoringRegionStart(dlb_loop_monitor);

		verbose_info_master("DLB-TALP Init done");
	} else
	{
		dlb_app_monitor = dlb_loop_monitor = NULL;
		talp_monitor = 0;

		sem_wait(earl_talp_sem);
		dlb_talp_data->must_call_libdlb = 0;
		sem_post(earl_talp_sem);

		/* Dispose shared area created at talp_init_shared_area. */
		talp_shared_area_dispose(master);

		verbose_error("Registering regions.");
	}

	return (talp_monitor ? EAR_SUCCESS : EAR_ERROR);
}


static state_t talp_monitor_periodic_action(void *p)
{
  if (!talp_monitor)
  {
    return EAR_SUCCESS;
  }

  int must_call_libdlb = 0;
  verbose_info2("DLB-TALP Periodic action");

  sem_wait(earl_talp_sem); // Wait/Lock the semaphor

  if (dlb_talp_data->must_call_libdlb)
  {
    libdlb_calls_cnt++;
    must_call_libdlb = 1;

    if (libdlb_calls_cnt > dlb_talp_data->max_libdlb_calls)
    {
      dlb_talp_data->max_libdlb_calls = libdlb_calls_cnt;

      msync(dlb_talp_data, sizeof(earl_talp_shd_t), MS_SYNC);
    }
  }

  sem_post(earl_talp_sem); // Increment/free the semaphor

  if (must_call_libdlb)
  {
    collect_and_report_talp_node_metrics();

    if (HAVE_DLB_TALP_H && dlb_loop_monitor)
    {
      DLB_MonitoringRegionReset(dlb_loop_monitor);
      DLB_MonitoringRegionStart(dlb_loop_monitor);
    }
  }
  return EAR_SUCCESS;
}


static state_t talp_monitor_finalize()
{
  if (!talp_monitor)
  {
    return EAR_SUCCESS;
  }

  debug("TALP monitor finalize");

  int must_call_libdlb = 0;

  sem_wait(earl_talp_sem); // Wait/Lock the semaphor

  dlb_talp_data->must_call_libdlb = 0;

  if (msync(lib_shared_region, sizeof(lib_shared_data_t), MS_SYNC) < 0)
  {
    verbose_warning("Memory sync. for lib shared region failed: %s", strerror(errno));
  }

  debug("libdlb_cnt %d max %d", libdlb_calls_cnt, dlb_talp_data->max_libdlb_calls);
  if (libdlb_calls_cnt < dlb_talp_data->max_libdlb_calls)
  {
    must_call_libdlb = 1;
    libdlb_calls_cnt++;
  }

  sem_post(earl_talp_sem); // Increment/free the semaphor

  if (must_call_libdlb)
  {
    collect_and_report_talp_node_metrics();
  }

  if (masters_info.my_master_rank >= 0 && talp_report)
  {
    if (fclose(talp_report) == EOF)
    {
      verbose_warning("Error when closing TALP report file (%d)", errno);
    }
  }

  if (HAVE_DLB_TALP_H && dlb_app_monitor)
  {
    DLB_MonitoringRegionStop(dlb_app_monitor);
    // DLB_MonitoringRegionReport(dlb_app_monitor);
  }

	uint master = masters_info.my_master_rank >= 0;
	talp_shared_area_dispose(master);

  debug("TALP monitor unregistering");
  monitor_unregister(earl_talp);

  return EAR_SUCCESS;
}


static void report_talp_node_metrics(dlb_node_metrics_t *node_metrics)
{
	if (HAVE_DLB_TALP_H && masters_info.my_master_rank >= 0)
	{
		if (!talp_report)
		{
			// The report file is not created yet

			char buff[GENERIC_NAME];
			int ret = snprintf(buff, sizeof buff, "%d-%d-%d-talp.csv",
												 my_job_id, my_step_id, masters_info.my_master_rank);

			if (ret >= sizeof buff)
			{
				verbose_error("TALP report file name couldn't be created: increase the buffer size.");
				return;
			}

			talp_report = fopen(buff, "a");
			if (!talp_report)
			{
				verbose_error("Creating and openning TALP report file (%d)", errno);
				return;
			}

			char talp_report_header[] = "time,node_id,total_useful_time_ns,"
				"total_mpi_time_ns,max_useful_time_ns,"
				"max_mpi_time_ns,load_balance,parallel_efficiency";

			size_t header_len = strlen(talp_report_header);

			size_t fwrite_ret = fwrite((const void *) talp_report_header,
					sizeof(char), header_len, talp_report);

			// Error checking below... You can skip to the next condition block
			if (fwrite_ret != header_len)
			{
				verbose_error("Writing TALP report header: (%d)", errno);

				fclose(talp_report);
				talp_report = NULL;

				return;
			}
		}

		if (!talp_report)
		{
			verbose_warning_master("TALP report file not created.");
			return;
		}

		// Get the current time
		time_t curr_time = time(NULL);

		struct tm *loctime = localtime (&curr_time);

		char time_buff[32];
		strftime(time_buff, sizeof time_buff, "%c", loctime);

		char talp_node_metrics_sum[512];

		int ret = snprintf(talp_node_metrics_sum, sizeof talp_node_metrics_sum,
				"\n%s,%d,%"PRId64",%"PRId64",%"PRId64",%"PRId64",%f,%f",
				time_buff, node_metrics->node_id, node_metrics->total_useful_time,
				node_metrics->total_mpi_time, node_metrics->max_useful_time,
				node_metrics->max_mpi_time, node_metrics->load_balance,
				node_metrics->parallel_efficiency);

		if (ret >= sizeof talp_node_metrics_sum)
		{
			verbose_error("Writing the node metrics line. Increase the buffer size");
			return;
		}

		size_t char_count = strlen(talp_node_metrics_sum);
		size_t fwrite_ret = fwrite((const void *) talp_node_metrics_sum,
															 sizeof(char), char_count, talp_report);

		// Error checks below... You can skip reading this function
		if (fwrite_ret != char_count)
		{
			verbose_error("Writing TALP info: %d", errno);
		}
	}
}


static state_t collect_talp_node_metrics(dlb_node_metrics_t *dst, dlb_monitor_t *region_monitor)
{
	if (HAVE_DLB_TALP_H)
	{
		if (dst)
		{
      int ret = DLB_TALP_CollectPOPNodeMetrics(region_monitor, dst);

      return (ret) ? EAR_ERROR : EAR_SUCCESS;
		} else
		{
			return EAR_ERROR;
		}
	} else
	{
		return EAR_SUCCESS;
	}
}


static void collect_and_report_talp_node_metrics()
{
	dlb_node_metrics_t dlb_node_metrics;

	state_t ret_st = collect_talp_node_metrics(&dlb_node_metrics, dlb_loop_monitor);
	if (state_fail(ret_st))
	{
		return;
	}

	timestamp_get(&curr_talp_data.timestamp);
	memcpy(&curr_talp_data.dlb_node_metrics, &dlb_node_metrics, sizeof(dlb_node_metrics_t));

	if (masters_info.my_master_rank >= 0)
	{
		// sig_ext_t *node_sig_ext = (sig_ext_t *) lib_shared_region->node_signature.sig_ext;
		// node_sig_ext->dlb_talp_node_metrics = sig_ext_tmp.dlb_talp_node_metrics;

		report_talp_node_metrics(&dlb_node_metrics);
	}
}


static state_t talp_init_shared_area(uint master_process, uint process_count)
{
#define close_and_unlink(fd, name) \
	do {\
		close(fd);\
		master_unlink(name);\
} while (0);

	ullong elapsed = 0;
	timestamp_t start;
	timestamp_get(&start);

	int fd = -1;

	// The master also creates the file
	int shm_oflag = (master_process) ? O_CREAT | O_RDWR : O_RDWR;

	/* Slaves will try to open the shared memory object and
	 * map to it more than once, up to TALP_SHM_TIMEOUT seconds */
	do
	{
		fd = shm_open(TALP_SHM_NAME, shm_oflag, 0600);
		if (fd >= 0)
		{
			/* The master truncates the file. */
			state_t st = mstr_trunc_sm(master_process, fd, sizeof(earl_talp_shd_t));
			if (state_fail(st))
			{
				close_and_unlink(fd, TALP_SHM_NAME);
				fd = -1; // After closing we set -1 to mark an error has occurred.
			}
		} else
		{
			/* We get the elapsed time since the start of this function. */
			elapsed = timestamp_diffnow(&start, TIME_SECS);
			usleep(100000);
		}

		// Slaves retry if the master didn't open the shared memory object yet
	} while (!master_process && fd == -1 && elapsed < TALP_SHM_TIMEOUT);

	/* Now lets check if all processes reached this section without errors. */
	char err_msg[128];

	if (fd < 0)
	{
		/* shm_open error handling */
		if (master_process)
		{
			snprintf(err_msg, sizeof(err_msg),
							 "Creating DLB TALP shared memory object (%d).", errno);
		} else
		{
			snprintf(err_msg, sizeof(err_msg),
							 "Opening DLB TALP shared memory object (%d).", errno);
		}
    return_msg(EAR_ERROR, err_msg);
	}

	/* Shared memory map */
	dlb_talp_data = mmap(NULL, sizeof(earl_talp_shd_t),
											 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	/* mmap error handling */
	if (dlb_talp_data == MAP_FAILED)
	{
		/* Just the master removes the shared memory object,
		 * slaves simply close the fd */
		close_and_unlink(fd, TALP_SHM_NAME);
		snprintf(err_msg, sizeof(err_msg), "Mapping to shared memory (%d)", errno);
		return_msg(EAR_ERROR, err_msg);
	}

	/* According to the documentation, once the mmap returns it is save to close the fd. */
	close(fd);

	/* Open/Create the semaphore. It is initialized to 0 to force slaves
	 * wait for master to init the shared data. */
	earl_talp_sem = sem_open(TALP_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);

	/* sem_open error handling */
	if (earl_talp_sem == SEM_FAILED)
	{
		/* Just the master removes the shared memory object. */
		unmap_and_unlink(dlb_talp_data, sizeof(earl_talp_shd_t), TALP_SHM_NAME);

		snprintf(err_msg, sizeof(err_msg), "Openning the named semaphore (%d)", errno);
		return_msg(EAR_ERROR, err_msg);
	}

	if (master_process)
	{
		dlb_talp_data->must_call_libdlb = 1;
		dlb_talp_data->processes_ready = 1;

		msync(dlb_talp_data, sizeof(earl_talp_shd_t), MS_SYNC);

		sem_post(earl_talp_sem); /* Master marks the semaphore to be used. */
	} else
	{
		sem_wait(earl_talp_sem); /* Slaves wait for data to be initiated */
		dlb_talp_data->processes_ready += 1;
		msync(dlb_talp_data, sizeof(earl_talp_shd_t), MS_SYNC);
		sem_post(earl_talp_sem);
	}

	/* Each process waits until all processes are ready. */
	timestamp_get(&start);
	elapsed = 0;
	while (dlb_talp_data->processes_ready < process_count && elapsed < TALP_SHM_TIMEOUT)
	{
		usleep(100000);
		elapsed = timestamp_diffnow(&start, TIME_SECS);
	}

	if (dlb_talp_data->processes_ready < process_count)
	{
		snprintf(err_msg, sizeof(err_msg), "%u/%u processes ready after %llu seconds elapsed.",
						 dlb_talp_data->processes_ready, process_count, elapsed);

		return_msg(EAR_ERROR, err_msg);
	}
	
	return EAR_SUCCESS;
}


static state_t mstr_trunc_sm(uint master_process, int fd, off_t length)
{
	if (master_process)
	{
		return (ftruncate(fd, sizeof(earl_talp_shd_t)) == -1) ? EAR_ERROR : EAR_SUCCESS;
	}
	return EAR_SUCCESS;
}


static void talp_shared_area_dispose(uint master_process)
{
	unmap_and_unlink(dlb_talp_data, sizeof(earl_talp_shd_t), TALP_SHM_NAME);

	sem_close(earl_talp_sem);
	if (master_process)
	{
		sem_unlink(TALP_SEM_NAME);
	}
}
