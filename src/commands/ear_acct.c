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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#define _XOPEN_SOURCE 700 //to get rid of the warning

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/config/config_sched.h>
#include <common/system/user.h>
#include <common/output/verbose.h>
#include <common/types/loop.h>
#include <common/types/version.h>
#include <common/types/application.h>
#include <common/types/log.h>
#include <common/types/event_type.h>

#if USE_DB
#include <common/states.h>
#include <common/database/db_helper.h>
#include <common/database/mysql_io_functions.h>
#include <common/database/postgresql_io_functions.h>
#include <common/types/configuration/cluster_conf.h>
cluster_conf_t my_conf;
#endif

#define STANDARD_NODENAME_LENGTH	25
#define APP_TEXT_FILE_FIELDS		22

int full_length = 0;
int verbose = 0;
int query_filters = 0;
int all_mpi = 0;
int avx = 0;
int print_gpus = 1;
int loop_extended = 0;
char csv_path[256] = "";

typedef struct query_addons 
{
	int limit;
	int job_id;
	int step_id;
	int start_time;
	int end_time;
	char e_tag[64];
	char app_id[64];
	char job_ids[64];
} query_adds_t;

void usage(char *app)
{
#if DB_MYSQL
	printf("Usage: %s [Optional parameters]\n"\
			"\tOptional parameters: \n" \
			"\t\t-h\tdisplays this message\n"\
			"\t\t-v\tdisplays current EAR version\n" \
			"\t\t-b\tverbose mode for debugging purposes\n" \
			"\t\t-u\tspecifies the user whose applications will be retrieved. Only available to privileged users. [default: all users]\n" \
			"\t\t-j\tspecifies the job id and step id to retrieve with the format [jobid.stepid] or the format [jobid1,jobid2,...,jobid_n].\n" \
			"\t\t\t\tA user can only retrieve its own jobs unless said user is privileged. [default: all jobs]\n"\
			"\t\t-a\tspecifies the application names that will be retrieved. [default: all app_ids]\n" \
			"\t\t-c\tspecifies the file where the output will be stored in CSV format. If the argument is \"no_file\" the output will be printed to STDOUT [default: off]\n" \
			"\t\t-t\tspecifies the energy_tag of the jobs that will be retrieved. [default: all tags].\n" \
			"\t\t-s\tspecifies the minimum start time of the jobs that will be retrieved in YYYY-MM-DD. [default: no filter].\n" \
			"\t\t-e\tspecifies the maximum end time of the jobs that will be retrieved in YYYY-MM-DD. [default: no filter].\n" \
			"\t\t-l\tshows the information for each node for each job instead of the global statistics for said job.\n" \
			"\t\t-x\tshows the last EAR events. Nodes, job ids, and step ids can be specified as if were showing job information.\n" \
			"\t\t-m\tprints power signatures regardless of whether mpi signatures are available or not.\n" \
			"\t\t-r\tshows the EAR loop signatures. Nodes, job ids, and step ids can be specified as if were showing job information.\n" \
			"\t\t-o\tmodifies the -r option to also show the corresponding jobs. Should be used with -j.\n", app);
	printf("\t\t-n\tspecifies the number of jobs to be shown, starting from the most recent one. [default: 20][to get all jobs use -n all]\n" );
	printf("\t\t-f\tspecifies the file where the user-database can be found. If this option is used, the information will be read from the file and not the database.\n");
#endif
	exit(0);
}

void print_values(int fd, char ***values, int columns, int rows)
{
	int i, j;
	for (i = 0; i < rows; i++) {
		dprintf(fd, "%s", values[i][0]);
		for (j = 1; j < columns; j++) {
			dprintf(fd, ";%s", values[i][j]);
		}
		dprintf(fd, "\n");
	}
}

void print_header(int fd, char ***values, int columns, int rows)
{
	int i = 0;
	dprintf(fd, "%s", values[i][0]);
	for (i = 1; i < rows; i++) {
		dprintf(fd, ";%s", values[i][0]);
	}
}

void print_full_apps(application_t *apps, int num_apps)
{
	int i;
	double avg_f, imc, vpi;

	printf("%8s-%-4s\t %-10s %-10s %-16s %-5s/%-5s %-10s %-10s %-10s %-10s %-10s %-7s %-5s %-7s",
			"JOB", "STEP", "NODE ID", "USER ID", "APPLICATION", "AVG-F", "IMC-F", "TIME(s)",
			"POWER(s)", "GBS", "CPI", "ENERGY(J)", "IO(MBS)", "MPI%", "VPI(%)");

#if USE_GPUS
	int j;
	double gpu_power, gpu_total_power;
	unsigned long gpu_freq, gpu_util, gpu_mem_util;
	char tmp[64];
	if (print_gpus) 
		printf(" %-13s %-6s %-13s ", "G-POW(T/U)", "G-FREQ", "G-UTIL(G/M)");
#endif
	printf("\n");

	for (i = 0; i < num_apps; i++)
	{
		if (strlen(apps[i].job.app_id) > 30)
			if (strchr(apps[i].job.app_id, '/') != NULL)
				strcpy(apps[i].job.app_id, strrchr(apps[i].job.app_id, '/')+1);

		if (apps[i].is_mpi && !all_mpi && (apps[i].signature.avg_f > 0))
		{
			avg_f = (double) apps[i].signature.avg_f/1000000;
			imc = (double) apps[i].signature.avg_imc_f/1000000;
			compute_sig_vpi(&vpi, &apps[i].signature);
			if (apps[i].job.step_id != BATCH_STEP)
			{
				printf("%8lu-%-4lu\t %-10s %-10s %-16s %5.2lf/%-5.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.0lf %-7.1lf %-5.1lf %-7.2lf",
						apps[i].job.id, apps[i].job.step_id, apps[i].node_id, apps[i].job.user_id, apps[i].job.app_id, 
						avg_f, imc, apps[i].signature.time, apps[i].signature.DC_power, apps[i].signature.GBS, apps[i].signature.CPI, 
						apps[i].signature.time * apps[i].signature.DC_power, apps[i].signature.IO_MBS, apps[i].signature.perc_MPI, vpi*100);
			}
			else
			{
				printf("%8lu-%-4s\t %-10s %-10s %-16s %5.2lf/%-5.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.0lf %-7.1lf %5.1lf %-7.2lf",
						apps[i].job.id, "sb", apps[i].node_id, apps[i].job.user_id, apps[i].job.app_id, 
						avg_f, imc, apps[i].signature.time, apps[i].signature.DC_power, apps[i].signature.GBS, apps[i].signature.CPI, 
						apps[i].signature.time * apps[i].signature.DC_power, apps[i].signature.IO_MBS, apps[i].signature.perc_MPI, vpi*100);
			}
#if USE_GPUS
			if (print_gpus)
			{
				if (apps[i].signature.gpu_sig.num_gpus > 0)
				{
					gpu_power = gpu_total_power = 0;
					gpu_freq = gpu_util = gpu_mem_util = 0;
					for (j = 0; j < apps[i].signature.gpu_sig.num_gpus; j++)
					{
						gpu_total_power += apps[i].signature.gpu_sig.gpu_data[j].GPU_power;
						if (apps[i].signature.gpu_sig.gpu_data[j].GPU_util)
						{
							gpu_power	+= apps[i].signature.gpu_sig.gpu_data[j].GPU_power;
							gpu_freq	 += apps[i].signature.gpu_sig.gpu_data[j].GPU_freq;
							gpu_util	 += apps[i].signature.gpu_sig.gpu_data[j].GPU_util;
							gpu_mem_util += apps[i].signature.gpu_sig.gpu_data[j].GPU_mem_util;
						}
					}
					gpu_freq	 /= apps[i].signature.gpu_sig.num_gpus;
					gpu_util	 /= apps[i].signature.gpu_sig.num_gpus;
					gpu_mem_util /= apps[i].signature.gpu_sig.num_gpus;

					sprintf(tmp, "%lu%%/%lu%%", gpu_util, gpu_mem_util);
					printf(" %-6.2lf/%-6.2lf %-6.3lf %13s", gpu_total_power, gpu_power, (double)gpu_freq/1000000, tmp);
				}
				else
				{
					printf(" %-7s %-8s %-13s", "---", "---", "---");
				}
			}
#endif
			printf("\n");
		}
		else
		{
			avg_f = (double) apps[i].power_sig.avg_f/1000000;
			if (apps[i].job.step_id != BATCH_STEP)
			{
				printf("%8lu-%-4lu\t %-10s %-10s %-16s %5.2lf/%-5s %-10.2lf %-10.2lf %-10s %-10s %-10.0lf %-7s %-5s %-7s",
						apps[i].job.id, apps[i].job.step_id, apps[i].node_id, apps[i].job.user_id, apps[i].job.app_id, 
						avg_f, "---", apps[i].power_sig.time, apps[i].power_sig.DC_power, "---", "---", 
						apps[i].power_sig.time * apps[i].power_sig.DC_power, "---", "---", "---");
			}
			else
			{
				printf("%8lu-%-4s\t %-10s %-10s %-16s %5.2lf/%-5s %-10.2lf %-10.2lf %-10s %-10s %-10.0lf %-7s %-5s %-7s",
						apps[i].job.id, "sb", apps[i].node_id, apps[i].job.user_id, apps[i].job.app_id, 
						avg_f, "---", apps[i].power_sig.time, apps[i].power_sig.DC_power, "---", "---", 
						apps[i].power_sig.time * apps[i].power_sig.DC_power, "---", "---", "---");

			}
#if USE_GPUS
			if (print_gpus) printf(" %-7s %-8s %-13s", "---", "---", "---");
#endif
			printf("\n");
		}

	}
}

void print_short_apps(application_t *apps, int num_apps, int fd, int is_csv)
{
	uint current_job_id = -1;
	uint current_step_id = -1;
	uint current_is_mpi = 0;
	uint current_apps = 0;

	int i = 0;
	double avg_time, avg_power, total_energy, avg_f, avg_frequency, avg_imc, avg_GBS, avg_CPI, avg_VPI, gflops_watt, avg_io, avg_perc;
	avg_frequency = 0;
	avg_imc = 0;
	avg_time = 0;
	avg_power = 0;
	total_energy = 0;
	avg_CPI = 0;
	avg_GBS = 0;
	avg_VPI = 0;
	avg_perc = 0;
	avg_io = 0;
	gflops_watt = 0;
	char is_sbatch = 0;
	char curr_policy[3];
	char missing_apps = -1;
	char header_format[256];
	char line_format[256];
	char mpi_line_format[256];
	char sbatch_line_format[256];
	char mpi_sbatch_line_format[256];
	double vpi;
#if USE_GPUS
	char tmp[32];
	char gpu_format[256];
	char gpu_header[256];
	double gpu_power = 0;
	double gpu_total_power = 0;
	unsigned long gpu_freq = 0;
	unsigned long gpu_util = 0;
	unsigned long gpu_mem_util = 0;
	unsigned long gpu_frequery_addsux = 0;
	unsigned long gpu_util_aux = 0;
	unsigned long gpu_mem_util_aux = 0;
	int num_gpus = 0;
	int j = 0;
#endif

	if (!is_csv)
	{
		if (avx)
		{
			strcpy(header_format, "%7s-%-4s %-10s %-16s %-6s %-5s %-3s/%-3s/%-8s %-10s %-8s %-7s %-5s %-12s %-8s %-7s %-5s %-14s");
			strcpy(line_format, "%7u-%-3u  %-10s %-16s %-6s %-5u %-4.2lf/%-4.2lf/%-6.2lf %-10.2lf %-8.2lf %-7.2lf %-5.2lf %-12.0lf %-8.4lf %-7.1lf %-5.1lf %-14.2lf");
			strcpy(mpi_line_format, "%7u-%-3u  %-10s %-16s %-6s %-5u %-4.2lf/%-4.2lf/%-6s %-10.2lf %-8.2lf %-7s %-5s %-12.0lf %-8s %-7s %-5s %-14s");
			strcpy(sbatch_line_format, "%7u-%-3s  %-10s %-16s %-6s %-5u %-4.2lf/%-4.2lf/%-6.2lf %-10.2lf %-8.2lf %-7.2lf %-5.2lf %-12.0lf %-8.4lf %-7.1lf %-5.1lf %-14.3lf");
			strcpy(mpi_sbatch_line_format, "%7u-%-3s  %-10s %-16s %-6s %-5u %-4.2lf/%-4.2lf/%-6s %-10.2lf %-8.2lf %-7s %-5s %-12.0lf %-8s %-8s %-5s %-14s");
		}
		else
		{
			strcpy(header_format, "%7s-%-4s %-10s %-16s %-6s %-5s %-3s/%-3s/%-8s %-10s %-8s %-7s %-5s %-12s %-8s %-7s %-5s");
			strcpy(line_format, "%7u-%-3u  %-10s %-16s %-6s %-5u %-4.2lf/%-4.2lf/%-6.2lf %-10.2lf %-8.2lf %-7.2lf %-5.2lf %-12.0lf %-8.4lf %-7.1lf %-5.1lf");
			strcpy(mpi_line_format, "%7u-%-3u  %-10s %-16s %-6s %-5u %-4.2lf/%-4.2lf/%-6s %-10.2lf %-8.2lf %-7s %-5s %-12.0lf %-8s %-7s %-5s");
			strcpy(sbatch_line_format, "%7u-%-3s  %-10s %-16s %-6s %-5u %-4.2lf/%-4.2lf/%-6.2lf %-10.2lf %-8.2lf %-7.2lf %-5.2lf %-12.0lf %-8.4lf %-7.1lf %-5.1lf");
			strcpy(mpi_sbatch_line_format, "%7u-%-3s  %-10s %-16s %-6s %-5u %-4.2lf/%-4.2lf/%-6s %-10.2lf %-8.2lf %-7s %-5s %-12.0lf %-8s %-7s %-5s");
		}
#if USE_GPUS
		strcpy(gpu_header, " %-13s %-7s %-13s");
		strcpy(gpu_format, " %-6.2lf/%-6.2lf %-7.3lf %-13s ");
#endif
	}
	else
	{
		if (avx)
		{
			strcpy(header_format, "%s-%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s");
			strcpy(line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf");
			strcpy(mpi_line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%s;%lf;%lf;%s;%s;%lf;%s;%s;%s;%s");
			strcpy(sbatch_line_format, "%u-%s;%s;%s;%s;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf");
			strcpy(mpi_sbatch_line_format, "%u-%s;%s;%s;%s;%u;%lf;%lf;%s;%lf;%lf;%s;%s;%lf;%s;%s;%s;%s");
		}
		else
		{
			strcpy(header_format, "%s-%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s");
			strcpy(line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf");
			strcpy(mpi_line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%s;%lf;%lf;%s;%s;%lf;%s;%s;%s");
			strcpy(sbatch_line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf");
			strcpy(mpi_sbatch_line_format, "%u-%s;%s;%s;%s;%u;%lf;%lf;%s;%lf;%lf;%s;%s;%lf;%s;%s;%s");
		}
#if USE_GPUS
		strcpy(gpu_header, ";%s;%s;%s");
		strcpy(gpu_format, ";%lf/%lf;%lf;%s");
#endif
	}
	if (avx)
		dprintf(fd, header_format,
				"JOB", "STEP", "USER", "APPLICATION", "POLICY", "NODES", "AVG","DEF", "IMC(GHz)", "TIME(s)",
				"POWER(W)", "GBS", "CPI", "ENERGY(J)", "GFLOPS/W", "IO(MBs)", "MPI%", "VPI(%)");

	else
		dprintf(fd, header_format,
				"JOB", "STEP", "USER", "APPLICATION", "POLICY", "NODES", "AVG", "DEF", "IMC(GHz)","TIME(s)",
				"POWER(W)", "GBS", "CPI", "ENERGY(J)", "GFLOPS/W", "IO(MBs)", "MPI%");

#if USE_GPUS
	if (print_gpus) dprintf(fd, gpu_header, "G-POW (T/U)", "G-FREQ", "G-UTIL(G/MEM)");
#endif
	dprintf(fd, "\n");

	for (i = 0; i < num_apps; i ++)
	{
		if (apps[i].job.id == current_job_id && apps[i].job.step_id == current_step_id)
		{
			if (current_is_mpi && !all_mpi)
			{
				compute_sig_vpi(&vpi, &apps[i].signature); 
				avg_VPI += vpi;
				avg_f = (double) apps[i].signature.avg_f/1000000;
				avg_frequency += avg_f;
				avg_imc += (double) apps[i].signature.avg_imc_f/1000000;
				avg_time += apps[i].signature.time;
				avg_power += apps[i].signature.DC_power;
				avg_GBS += apps[i].signature.GBS;
				avg_CPI += apps[i].signature.CPI;
				avg_io += apps[i].signature.IO_MBS;
				avg_perc += apps[i].signature.perc_MPI;
				gflops_watt += apps[i].signature.Gflops;
				total_energy += apps[i].signature.time * apps[i].signature.DC_power;
				current_apps++;
#if USE_GPUS
				if (print_gpus)
				{
					if (apps[i].signature.gpu_sig.num_gpus > 0)
					{
						num_gpus = 0;
						gpu_frequery_addsux = 0;
						gpu_util_aux = 0;
						gpu_mem_util_aux = 0;
						for (j = 0; j < apps[i].signature.gpu_sig.num_gpus; j++)
						{
							gpu_total_power += apps[i].signature.gpu_sig.gpu_data[j].GPU_power;
							gpu_frequery_addsux	+= apps[i].signature.gpu_sig.gpu_data[j].GPU_freq;
							if (apps[i].signature.gpu_sig.gpu_data[j].GPU_util)
							{
								gpu_power		+= apps[i].signature.gpu_sig.gpu_data[j].GPU_power;
								gpu_util_aux	 += apps[i].signature.gpu_sig.gpu_data[j].GPU_util;
								gpu_mem_util_aux += apps[i].signature.gpu_sig.gpu_data[j].GPU_mem_util;
								num_gpus++;
							}
						}

						if (apps[i].signature.gpu_sig.num_gpus)
							gpu_frequery_addsux /= apps[i].signature.gpu_sig.num_gpus;

						if (num_gpus > 0)
						{
							gpu_util_aux	 /= num_gpus;
							gpu_mem_util_aux /= num_gpus;
						}
						gpu_freq	 += gpu_frequery_addsux;
						gpu_util	 += gpu_util_aux;
						gpu_mem_util += gpu_mem_util_aux;
					}
				}
#endif
			}
			else
			{
				avg_f = (double) apps[i].power_sig.avg_f/1000000;
				avg_frequency += avg_f;
				avg_time += apps[i].power_sig.time;
				avg_power += apps[i].power_sig.DC_power;
				total_energy += apps[i].power_sig.time * apps[i].power_sig.DC_power;
				current_apps++;
			}
		}

		else
		{
			//to print: job_id.step_id \t user_id si root \t app_name \t num_nodes
			int idx = (i > 0) ? i - 1: 0;
			if (strlen(apps[idx].job.app_id) > 30)
			{
				char *token = strrchr(apps[idx].job.app_id, '/');
				if (token != NULL) 
				{
					token++;
					strcpy(apps[idx].job.app_id, token);
				}
			}
			if (strlen(apps[idx].job.app_id) > 16) {
				apps[idx].job.app_id[16] = '\0';
			}


			if (current_is_mpi && !all_mpi)
			{

				gflops_watt /= avg_power;
				avg_frequency /= current_apps;
				avg_imc /= current_apps;
				avg_time /= current_apps;
				avg_power /= current_apps;
				avg_GBS /= current_apps;
				avg_CPI /= current_apps;
				avg_VPI /= current_apps;
				avg_perc /= current_apps;
#if USE_GPUS
				if (print_gpus)
				{
					gpu_total_power /= current_apps;
					gpu_power	/= current_apps;
					gpu_freq	 /= current_apps;
					gpu_util	 /= current_apps;
					gpu_mem_util /= current_apps;
				}
#endif

				if (avg_VPI == 0)
					avg_VPI = -1;
				else avg_VPI*=100;

				if (avg_frequency > 0 && avg_time > 0 && total_energy > 0)
				{
					if (!is_sbatch)
					{
						if (avx)
							dprintf(fd, line_format,
									current_job_id, current_step_id, apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
									avg_frequency, (double) apps[idx].job.def_f/1000000.0, avg_imc, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_io, avg_perc, avg_VPI);

						else
							dprintf(fd, line_format,
									current_job_id, current_step_id, apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
									avg_frequency, (double) apps[idx].job.def_f/1000000.0, avg_imc, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_io, avg_perc);
					}
					else
					{
						if (avx)
							dprintf(fd, sbatch_line_format,
									current_job_id, "sb", apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
									avg_frequency,(double) apps[idx].job.def_f/1000000.0, avg_imc, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_io, avg_perc, avg_VPI);

						else
							dprintf(fd, sbatch_line_format,
									current_job_id, "sb", apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
									avg_frequency,(double) apps[idx].job.def_f/1000000.0, avg_imc, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_io, avg_perc);
					}
#if USE_GPUS
					if (print_gpus)
					{
						if (num_gpus > 0)
						{
							sprintf(tmp, "%ld%%/%ld%%", gpu_util, gpu_mem_util);
							dprintf(fd, gpu_format, gpu_total_power, gpu_power, (double)gpu_freq/1000000, tmp);
						}
						else {
							sprintf(tmp, "%.2lf/---", gpu_total_power);
							dprintf(fd, gpu_header, tmp, "---", "---");
						}
					}
#endif
					dprintf(fd, "\n");
				}
				else
					missing_apps++;
			}
			else
			{
				avg_frequency /= current_apps;
				avg_time /= current_apps;
				avg_power /= current_apps;
				if (avg_frequency > 0 && avg_time > 0 && total_energy > 0)
				{
					if (!is_sbatch)
					{
						if (avx)
							dprintf(fd, mpi_line_format,
									current_job_id, current_step_id, apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
									avg_frequency,(double) apps[idx].job.def_f/1000000.0, "---", avg_time, avg_power, "---", "---", total_energy, "---", "---", "---", "---");
						else
							dprintf(fd, mpi_line_format,
									current_job_id, current_step_id, apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
									avg_frequency, (double) apps[idx].job.def_f/1000000.0, "---", avg_time, avg_power, "---", "---", total_energy, "---", "---", "---");
					}
					else
					{
						if (avx)
							dprintf(fd, mpi_sbatch_line_format,
									current_job_id, "sb", apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
									avg_frequency, (double) apps[idx].job.def_f/1000000.0, "---", avg_time, avg_power, "---", "---", total_energy, "---", "---", "---", "---");
						else
							dprintf(fd, mpi_sbatch_line_format,
									current_job_id, "sb", apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
									avg_frequency, (double) apps[idx].job.def_f/1000000.0, "---", avg_time, avg_power, "---", "---", total_energy, "---", "---", "---");
					}
#if USE_GPUS
					if (print_gpus) dprintf(fd, gpu_header, "---", "---", "---");
#endif
					dprintf(fd, "\n");
				}
				else
					missing_apps++;
			}
			if (apps[i].job.id != current_job_id || apps[i].job.step_id != current_step_id)
			{
				get_short_policy(curr_policy, apps[i].job.policy, &my_conf);
				current_job_id = apps[i].job.id;
				current_step_id = apps[i].job.step_id;
				current_is_mpi = apps[i].is_mpi && (apps[i].signature.avg_f > 0);
				current_apps = 0;
				avg_frequency = 0;
				avg_time = 0;
				avg_power = 0;
				avg_GBS = 0;
				avg_CPI = 0;
				avg_VPI = 0;
				avg_io = 0;
				avg_imc = 0;
				avg_perc = 0;
				total_energy = 0;
				gflops_watt = 0;
#if USE_GPUS
				num_gpus = 0;
				gpu_freq = 0;
				gpu_util = 0;
				gpu_power = 0;
				gpu_total_power = 0;
				gpu_mem_util = 0;
#endif
				i--; //go back to current app
				is_sbatch = (current_step_id == BATCH_STEP) ? 1 : 0;
			}
		}
	}
	if (num_apps > 0)
	{
		if (strlen(apps[i-1].job.app_id) > 30)
			if (strchr(apps[i-1].job.app_id, '/') != NULL)
				strcpy(apps[i-1].job.app_id, strrchr(apps[i-1].job.app_id, '/')+1);

		if (strlen(apps[i-1].job.app_id) > 16) {
			apps[i-1].job.app_id[16] = '\0';
		}

		if (apps[i-1].is_mpi && (apps[i-1].signature.avg_f > 0) && !all_mpi)
		{
			gflops_watt /= avg_power;
			avg_frequency /= current_apps;
			avg_time /= current_apps;
			avg_power /= current_apps;
			avg_GBS /= current_apps;
			avg_perc /= current_apps;
			avg_imc /= current_apps;
			avg_CPI /= current_apps;
			avg_VPI /= current_apps;
#if USE_GPUS
			if (print_gpus)
			{
				gpu_total_power /= current_apps;
				gpu_power /= current_apps;
				gpu_freq  /= current_apps;
				gpu_util  /= current_apps;
				gpu_mem_util /= current_apps;
			}
#endif

			if (avg_VPI == 0)
				avg_VPI = -1;
			else avg_VPI *=100;
			if (avg_frequency > 0 && avg_time > 0 && total_energy > 0)
			{
				if (!is_sbatch)
				{
					if (avx)
						dprintf(fd, line_format,
								current_job_id, current_step_id, apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
								avg_frequency,(double) apps[i-1].job.def_f/1000000.0, avg_imc, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_io, avg_perc, avg_VPI);
					else
						dprintf(fd, line_format,
								current_job_id, current_step_id, apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
								avg_frequency, (double) apps[i-1].job.def_f/1000000.0, avg_imc, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_io, avg_perc);
				}
				else
				{
					if (avx)
						dprintf(fd, sbatch_line_format,
								current_job_id, "sb", apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
								avg_frequency,(double) apps[i-1].job.def_f/1000000.0, avg_imc, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_io, avg_perc, avg_VPI);
					else
						dprintf(fd, sbatch_line_format,
								current_job_id, "sb", apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
								avg_frequency,(double) apps[i-1].job.def_f/1000000.0, avg_imc, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_io, avg_perc);
				}
#if USE_GPUS
				if (print_gpus)
				{
					if (num_gpus > 0) {
						sprintf(tmp, "%ld%%/%ld%%", gpu_util, gpu_mem_util);
						dprintf(fd, gpu_format, gpu_total_power, gpu_power, (double)gpu_freq/1000000, tmp);
					}
					else {
						sprintf(tmp, "%.2lf/---", gpu_total_power);
						dprintf(fd, gpu_header, tmp, "---", "---");
					}
				}
#endif
				dprintf(fd, "\n");
			}
			else
				missing_apps++;
		}
		else
		{
			avg_frequency /= current_apps;
			avg_time /= current_apps;
			avg_power /= current_apps;
			if (avg_frequency > 0 && avg_time > 0 && total_energy > 0)
			{
				if (!is_sbatch)
				{
					if (avx)
						dprintf(fd, mpi_line_format,
								current_job_id, current_step_id, apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
								avg_frequency, (double) apps[i-1].job.def_f/1000000.0, "---", avg_time, avg_power, "---", "---", total_energy, "---", "---", "---", "---");
					else
						dprintf(fd, mpi_line_format,
								current_job_id, current_step_id, apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
								avg_frequency, (double) apps[i-1].job.def_f/1000000.0, "---", avg_time, avg_power, "---", "---", total_energy, "---", "---", "---");
				}
				else
				{
					if (avx)
						dprintf(fd, mpi_sbatch_line_format,
								current_job_id, "sb", apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
								avg_frequency,(double) apps[i-1].job.def_f/1000000.0, "---", avg_time, avg_power, "---", "---", total_energy, "---", "---", "---", "---");
					else
						dprintf(fd, mpi_sbatch_line_format,
								current_job_id, "sb", apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
								avg_frequency, (double) apps[i-1].job.def_f/1000000.0, "---", avg_time, avg_power, "---", "---", total_energy, "---", "---", "---");
				}
#if USE_GPUS
				if (print_gpus)
					dprintf(fd, gpu_header, "---", "---", "---");
#endif
				dprintf(fd, "\n");
			}
			else
				missing_apps++;

		}
	}
	if (missing_apps > 0 && !is_csv)
		printf("\nSome jobs are not being shown because either their avg. frequency, time or total energy were 0. To see those jobs run with -l option.\n");

	if (avx)
		printf("\nA -1.0 in the VPI column means an absolute 0 in that field. This is done to distinguish from very low values.\n");

	printf("\n");
}

void add_string_filter(char *query, char *addition, char *value)
{
	if (query_filters < 1)
		strcat(query, " WHERE ");
	else
		strcat(query, " AND ");

	strcat(query, addition);
	strcat(query, "=");
	strcat(query, "'");
	strcat(query, value);
	strcat(query, "'");
	//	sprintf(query, query, value);
	query_filters ++;
}

void add_int_filter(char *query, char *addition, int value)
{
	char query_tmp[512];
	strcpy(query_tmp, query);
	if (query_filters < 1)
		strcat(query_tmp, " WHERE ");
	else
		strcat(query_tmp, " AND ");

	strcat(query_tmp, addition);
	strcat(query_tmp, "=");
	strcat(query_tmp, "%llu");
	sprintf(query, query_tmp, value);
	query_filters ++;
}

void add_int_comp_filter(char *query, char *addition, int value, char greater_than)
{
	char query_tmp[512];
	strcpy(query_tmp, query);
	if (query_filters < 1)
		strcat(query_tmp, " WHERE ");
	else
		strcat(query_tmp, " AND ");

	strcat(query_tmp, addition);
	if (greater_than)
		strcat(query_tmp, ">");
	else
		strcat(query_tmp, "<");
	strcat(query_tmp, "%llu");
	sprintf(query, query_tmp, value);
	query_filters ++;
}

void add_int_list_filter(char *query, char *addition, char *value)
{
	if (query_filters < 1)
		strcat(query, " WHERE ");
	else
		strcat(query, " AND ");

	strcat(query, addition);
	strcat(query, " IN ");
	strcat(query, "(");
	strcat(query, value);
	strcat(query, ")");
	//	sprintf(query, query, value);
	query_filters ++;
}

void print_event_type(int type, int fd)
{
    char str[64];
    ear_event_t aux;
    aux.event = type;
    event_type_to_str(&aux, str, sizeof(str));
    dprintf(fd, "%15s ", str);
}

#if DB_MYSQL
void mysql_print_events(MYSQL_RES *result, int fd)
{
    if (fd <  0) {
        fd = 1; // The standard output
    }

	int i;
	int num_fields = mysql_num_fields(result);

	MYSQL_ROW row;
	int has_records = 0;
	while ((row = mysql_fetch_row(result)) != NULL) 
	{
		if (!has_records)
		{
			dprintf(fd, "%12s %20s %15s %8s %8s %20s %12s\n",
					"Event_ID", "Timestamp", "Event_type", "Job_id", "Step_id", "Value", "node_id");
			has_records = 1;
		}

        int possible_negative = 0; // This variable checks whether the event value can be negative.
		for(i = 0; i < num_fields; i++) {
			if (i == 2) {
                int event_type_int = strtol(row[i], NULL, 10);

				print_event_type(event_type_int, fd);  

                if (event_type_int == ENERGY_SAVING || event_type_int == POWER_SAVING) {
                    possible_negative = 1;
                }
            }
			else if (i == 1)
				dprintf(fd, "%20s ", row[i] ? row[i] : "NULL");
			else if (i == 4 || i == 3)
				dprintf(fd, "%8s ", row[i] ? row[i] : "NULL");
			else if (i == 5) {

                if (row[i] != NULL) {

                    if (possible_negative) {
                        long int value = atoi(row[i]);
                        dprintf(fd, "%20ld ", *(long *) &value);
                    } else {
                        dprintf(fd, "%20s ", row[i]);
                    } // Column 5 not possible negative
                } else {
                        dprintf(fd, "%20s ", "NULL");
                } // Column 5 NULL
			} else {
				dprintf(fd, "%12s ", row[i] ? row[i] : "NULL");
            } // Column 0
		}
		dprintf(fd, "\n");
	}
	if (!has_records)
	{
		printf("There are no events with the specified properties.\n\n");
	}
}
#elif DB_PSQL
void postgresql_print_events(PGresult *res, int fd)
{
    if (fd < 0) {
        fd = 1; // Standard output
    }

	int i, j, num_fields, has_records = 0;
	num_fields = PQnfields(res);

    int possible_negative = 0; // This variable checks whether the event value can be negative.
	for (i = 0; i < PQntuples(res); i++)
	{
		if (!has_records)
		{
			dprintf(fd, "%12s %22s %15s %8s %8s %20s %12s\n",
					"Event_ID", "Timestamp", "Event_type", "Job_id", "Step_id", "Value", "node_id");
			has_records = 1;
		}

		for (j = 0; j < num_fields; j++) {
			if (j == 2) {
                int event_type_int = strtol(PQgetvalue(res, i, j), NULL, 10);

				print_event_type(event_type_int, fd);

                if (event_type_int == ENERGY_SAVING || event_type_int == POWER_SAVING) {
                    possible_negative = 1;
                }
            }
			else if (i == 1)
				dprintf(fd, "%22s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
			else if (i == 4 || i == 3)
				dprintf(fd, "%8s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
			else if (i == 5) {

                if (PQgetvalue(res, i, j) != NULL) {

                    if (possible_negative) {
                        long int value = atoi(PQgetvalue(res, i, j));
                        dprintf(fd, "%20ld ", *(long *) &value);
                    } else {
                        dprintf(fd, "%20s ", PQgetvalue(res, i, j));
                    } // Column 5 not possible negative
                } else {
                        dprintf(fd, "%20s ", "NULL");
                } // Column 5 NULL

            }
			else
				dprintf(fd, "%12s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
		}

		dprintf(fd, "\n");
	}

	if (!has_records)
	{
		printf("There are no events with the specified properties.\n\n");
	}
}
#endif

#if DB_MYSQL
#define EVENTS_QUERY "SELECT id, FROM_UNIXTIME(timestamp), event_type, job_id, step_id, value, node_id FROM Events"
#define EVENTS_QUERY_RAW "SELECT id, timestamp, event_type, job_id, step_id, value, node_id FROM Events"
#elif DB_PSQL
#define EVENTS_QUERY "SELECT id, to_timestamp(timestamp), event_type, job_id, step_id, value, node_id FROM Events"
#define EVENTS_QUERY_RAW "SELECT id, timestamp, event_type, job_id, step_id, value, node_id FROM Events"
#endif

void read_events(char *user, query_adds_t *q_a) 
{
	char query[512];
	char subquery[128];

	if (strlen(my_conf.database.user_commands) < 1) 
	{
		fprintf(stderr, "Warning: commands' user is not defined in ear.conf\n");
	}
	else
	{
		strcpy(my_conf.database.user, my_conf.database.user_commands);
		strcpy(my_conf.database.pass, my_conf.database.pass_commands);
	}
	init_db_helper(&my_conf.database);


    int fd = -1;
    if (strlen(csv_path) > 0 && strcmp(csv_path, "no_file")) {
        fd = open(csv_path, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR|S_IWUSR|S_IRGRP);
    }

    if (fd > 0) {
        strcpy(query, EVENTS_QUERY_RAW);
    } else {
	    strcpy(query, EVENTS_QUERY);
    }

	add_int_comp_filter(query, "event_type", 100, 0);
	if (q_a->job_id >= 0)
		add_int_filter(query, "job_id", q_a->job_id);
	else if (strlen(q_a->job_ids) > 0)
		add_int_list_filter(query, "job_id", q_a->job_ids);
	if (q_a->step_id >= 0)
		add_int_filter(query, "step_id", q_a->step_id);
	if (user != NULL)
		add_string_filter(query, "node_id", user);
	if (q_a->start_time != 0)
		add_int_comp_filter(query, "timestamp", q_a->start_time, 1);
	if (q_a->end_time != 0)
		add_int_comp_filter(query, "timestamp", q_a->end_time, 0);

	if (q_a->limit > 0)
	{
		sprintf(subquery, " ORDER BY timestamp desc LIMIT %d", q_a->limit);
		strcat(query, subquery);
	}

	if (verbose) printf("QUERY: %s\n", query);


#if DB_MYSQL
	MYSQL_RES *result = db_run_query_result(query);
#elif DB_PSQL
	PGresult *result = db_run_query_result(query);
#endif

	if (result == NULL) 
	{
		printf("Database error\n");
		return;
	}

#if DB_MYSQL
	mysql_print_events(result, fd);
#elif DB_PSQL
	postgresql_print_events(result, fd);
#endif
}

void print_loops(loop_t *loops, int num_loops)
{
	int i;
	char line[256];
	signature_t sig;
        char date_iter[128];
#if REPORT_TIMESTAMP
        struct tm *date_info;
#endif

#if REPORT_TIMESTAMP
	strcpy(line, "%6s-%-4s\t %-10s %-9s %-8s %-8s %-8s %-8s %-8s %-5s %-5s %-7s %-5s ");
	printf(line, "JOB", "STEP", "NODE ID", "DATE", "POWER(W)", "GBS", "CPI", "GFLOPS/W", "TIME(s)", "AVG_F", "IMC_F", "IO(MBS)", "MPI%");
#else
	strcpy(line, "%6s-%-4s\t %-10s %-6s %-8s %-8s %-8s %-8s %-8s %-5s %-5s %-7s %-5s ");
	printf(line, "JOB", "STEP", "NODE ID", "ITER.", "POWER(W)", "GBS", "CPI", "GFLOPS/W", "TIME(s)", "AVG_F", "IMC_F", "IO(MBS)", "MPI%");
#endif
#if USE_GPUS
	//GPU variable declaration
	int s;
	char gpu_line[256], tmp[16];
	double gpup = 0, gpupu = 0;
	ulong  gpuf = 0, gpuu = 0, gpuused = 0, gpu_mem_util = 0;
	strcpy(line, "%-12s %-8s %-13s");
	if (print_gpus)
		printf(line, "G-POWER(T/U)","G-FREQ","G-UTIL(G/MEM)");
	//prepare gpu_line format
	strcpy(gpu_line, "%-6.1lf/%6.1lf  %-8.2lf %-13s");
#endif
	printf("\n");

	strcpy(line, "%6u-%-4u\t %-10s %-9s %-8.1lf %-8.1lf %-8.3lf %-8.3lf %-8.3lf %-5.2lf %-5.2lf %-7.1lf %-5.1lf ");
	for (i = 0; i < num_loops; i++)
	{
		signature_copy(&sig, &loops[i].signature);
#if USE_GPUS
		if (print_gpus)
		{				
			for (s=0; s < sig.gpu_sig.num_gpus; s++){
				gpup += sig.gpu_sig.gpu_data[s].GPU_power;
				gpuf += sig.gpu_sig.gpu_data[s].GPU_freq;
				if (sig.gpu_sig.gpu_data[s].GPU_util){
					gpupu += sig.gpu_sig.gpu_data[s].GPU_power;
					gpuu += sig.gpu_sig.gpu_data[s].GPU_util;
					gpu_mem_util += sig.gpu_sig.gpu_data[s].GPU_mem_util;
					gpuused++;
				}
			}
			if (sig.gpu_sig.num_gpus)
				gpuf /= sig.gpu_sig.num_gpus;
 
			if (gpuused > 0)
			{
				gpuu /= gpuused;
				gpu_mem_util /= gpuused;
			}
		}
#endif
#if REPORT_TIMESTAMP
                date_info = localtime( (time_t *)&loops[i].total_iterations);
                strftime(date_iter, sizeof(date_iter), "%H:%M:%S", date_info );
#else
                sprintf(date_iter,"%lu", loops[i].total_iterations);
#endif
		printf(line, loops[i].jid, loops[i].step_id, loops[i].node_id, date_iter,
				sig.DC_power, sig.GBS, sig.CPI, sig.Gflops/sig.DC_power, sig.time, (double)(sig.avg_f)/1000000, (double)(sig.avg_imc_f)/1000000, sig.IO_MBS, sig.perc_MPI);
#if USE_GPUS
		sprintf(tmp, "%lu%%/%lu%%", gpuu, gpu_mem_util);
		if (print_gpus) printf(gpu_line, gpup, gpupu, (double)gpuf/1000000.0, tmp);
		gpu_mem_util = 0;
		gpuused = 0;
		gpupu = 0;
		gpuf = 0;
		gpuu = 0;
		gpup = 0;
#endif
		printf("\n");
	}
}

#define LOOPS_QUERY "SELECT * FROM Loops "
#define LOOPS_FILTER_QUERY "SELECT Loops.* from Loops INNER JOIN Jobs ON Jobs.id = job_id AND Jobs.step_id = Loops.step_id "
#if 0
#if USE_GPUS
#define LOOPS_EXTENDED_QUERY "SELECT Loops.*, Signatures.*, Jobs.start_time, Jobs.end_time, CAST(Loops.total_iterations AS SIGNED)- CAST(Jobs.start_time AS SIGNED), " \
							"min_GPU_sig_id, max_GPU_sig_id " \
							"from Loops INNER JOIN Jobs ON Jobs.id = job_id AND Jobs.step_id = Loops.step_id " \
							"INNER JOIN Signatures ON signature_id = Signatures.id"
#else
#define LOOPS_EXTENDED_QUERY "SELECT Loops.*, Signatures.*, Jobs.start_time, Jobs.end_time, CAST(Loops.total_iterations AS SIGNED)- CAST(Jobs.start_time AS SIGNED), " \
							"from Loops INNER JOIN Jobs ON Jobs.id = job_id AND Jobs.step_id = Loops.step_id " \
							"INNER JOIN Signatures ON signature_id = Signatures.id"
#endif



#if USE_GPUS
void get_and_print_gpus(int fd, char *min, char *max)
{
	char query[256];
	char ***values;
	int i, j, columns, rows, ret;
	//if (verbose) printf("entering get_and_print_gpus with %s min and %s max\n", min, max);
	if (min == NULL || max == NULL) return;
	if (!strcmp(min, "NULL") || ! strcmp(max, "NULL")) return;


	sprintf(query, "SELECT * from GPU_signatures WHERE id >= %s AND id <= %s", min, max);
	if (verbose) printf("running query %s\n", query);
	ret = db_run_query_string_results(query, &values, &columns, &rows);
	if (ret != EAR_SUCCESS) {
		printf("Error reading Loops from database.\n");
		return;
	}


	for (i = 0; i < rows; i++) 
		for (j = 0; j < columns; j++) 
			dprintf(fd, "%s;", values[i][j]); //simple print

	db_free_results(values, columns, rows);

}
void print_values(int fd, char ***values, int columns, int rows)
{
	int i, j;
	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns - 2; j++) { // the last 2 colums are a repeat of max and min gpu sig, to always have them in the same place
			dprintf(fd, "%s;", values[i][j]);
		}
		get_and_print_gpus(fd, values[i][columns-2], values[i][columns-1]);
		dprintf(fd, "\n");
	}
}
#else
#endif


void extended_loop_print(char *query)
{
	char ***values;
	int ret, columns, rows, fd = STDOUT_FILENO;


	if (strlen(csv_path) > 0 && strcmp(csv_path, "no_file"))
		fd = open(csv_path, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR|S_IWUSR|S_IRGRP);

	ret = db_run_query_string_results("DESCRIBE Loops", &values, &columns, &rows);
	if (verbose) printf("running query DESCRIBE Loops\n");
	if (ret != EAR_SUCCESS) {
		printf("Error reading Loop description from database.\n");
		return;
	}
	print_header(fd, values, columns, rows);
	db_free_results(values, columns, rows);

	ret = db_run_query_string_results("DESCRIBE Signatures", &values, &columns, &rows);
	if (verbose) printf("running query DESCRIBE Signatures\n");
	if (ret != EAR_SUCCESS) {
		printf("Error reading Signature description from database.\n");
		return;
	}
	print_header(fd, values, columns, rows);
	db_free_results(values, columns, rows);


	dprintf(fd, "start_time;end_time;elapsed;");
	dprintf(fd, "\n");

#if USE_GPUS
	ret = db_run_query_string_results("DESCRIBE GPU_signatures", &values, &columns, &rows);
	if (verbose) printf("running query DESCRIBE GPU_signatures\n");
	if (ret != EAR_SUCCESS) {
		printf("Error reading GPU description from database.\n");
		return;
	}
	print_header(fd, values, columns, rows);
	db_free_results(values, columns, rows);
#endif


	ret = db_run_query_string_results(query, &values, &columns, &rows);
	if (verbose) printf("running query %s\n", query);
	if (ret != EAR_SUCCESS) {
		printf("Error reading Loops from database.\n");
		return;
	}
	print_values(fd, values, columns, rows);
	db_free_results(values, columns, rows);

}
#endif 

void format_loop_query(char *user, char *query, char *base_query, query_adds_t *q_a)
{
	char subquery[128];

	strcpy(query, base_query);
	if (user != NULL) add_string_filter(query, "user_id", user);

	if (q_a->job_id >= 0)
		add_int_filter(query, "job_id", q_a->job_id);
	else if (strlen(q_a->job_ids) > 0)
		add_int_list_filter(query, "job_id", q_a->job_ids);
	if (q_a->step_id >= 0)
		add_int_filter(query, "step_id", q_a->step_id);

	if (q_a->limit > 0 && q_a->job_id < 0)
	{
		sprintf(subquery, " ORDER BY job_id desc LIMIT %d", q_a->limit);
		strcat(query, subquery);
	}
	else strcat(query, " ORDER BY job_id desc");

	if (verbose) printf("QUERY: %s\n", query);

}

void read_jobs_from_loops(query_adds_t *q_a)
{
    char query[256], subquery[256], tmp_path[512];
    char ***values;
    int columns, rows;
    int fd = STDOUT_FILENO, ret;

	if (strlen(csv_path) > 0 && strcmp(csv_path, "no_file")) {
        sprintf(tmp_path, "out_jobs.%s", csv_path);
		fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR|S_IWUSR|S_IRGRP);
    }

	ret = db_run_query_string_results("DESCRIBE Jobs", &values, &columns, &rows);
	if (verbose) printf("running query DESCRIBE Jobs\n");
	if (ret != EAR_SUCCESS) {
		printf("Error reading Loop description from database.\n");
		return;
	}
	print_header(fd, values, columns, rows);
    dprintf(fd, "\n"); //print_header does not print a new_line
	db_free_results(values, columns, rows);

    // reset filters
    query_filters = 0;
    strcpy(query, "SELECT * FROM Jobs ");

	if (q_a->job_id >= 0)
		add_int_filter(query, "id", q_a->job_id);
	else if (strlen(q_a->job_ids) > 0)
		add_int_list_filter(query, "id", q_a->job_ids);
    else printf("WARNING: -o option is meant to be used with a -j specification\n\n");
	if (q_a->step_id >= 0)
		add_int_filter(query, "step_id", q_a->step_id);
    if (q_a->start_time > 0)
		add_int_filter(query, "start_time", q_a->step_id);
    if (q_a->end_time > 0)
		add_int_filter(query, "end_time", q_a->step_id);

	if (q_a->limit > 0 && q_a->job_id < 0)
	{
		sprintf(subquery, " ORDER BY id desc LIMIT %d", q_a->limit);
		strcat(query, subquery);
	}
	else strcat(query, " ORDER BY id desc");

	if (verbose) printf("\nQUERY: %s\n", query);
	ret = db_run_query_string_results(query, &values, &columns, &rows);
    if (ret != EAR_SUCCESS) {
        printf("Error reading Jobs from database\n");
        return;
    }

	print_values(fd, values, columns, rows);
	db_free_results(values, columns, rows);

}

void read_loops(char *user, query_adds_t *q_a) 
{
	char query[1024];

	if (strlen(my_conf.database.user_commands) < 1) 
	{
		fprintf(stderr, "Warning: commands' user is not defined in ear.conf\n");
	}
	else
	{
		strcpy(my_conf.database.user, my_conf.database.user_commands);
		strcpy(my_conf.database.pass, my_conf.database.pass_commands);
	}
	init_db_helper(&my_conf.database);

#if 0
	if (loop_extended) {
		format_loop_query(user, query, LOOPS_EXTENDED_QUERY, q_a);
		return extended_loop_print(query);
	}
#endif

	if (user != NULL) {
		format_loop_query(user, query, LOOPS_FILTER_QUERY, q_a);
	} else {
		format_loop_query(user, query, LOOPS_QUERY, q_a);
	}


	loop_t *loops = NULL;
	int num_loops;

	num_loops = db_read_loops_query(&loops, query);

	if (num_loops < 1)
	{
		printf("No loops retrieved\n");
		return;
	}

	if (verbose) printf("retrieved %d loops\n", num_loops);
	if (strlen(csv_path) > 0)
	{
		int i;
		if (!strcmp(csv_path, "no_file"))
		{
			for (i = 0; i < num_loops; i++)
				print_loop_fd(STDOUT_FILENO, &loops[i]);
		}
		else {
			for (i = 0; i < num_loops; i++)
				append_loop_text_file_no_job(csv_path, &loops[i], 1, 0, ' ');
			printf("appended %d loops to %s\n", num_loops, csv_path);
		}
	} else {
		print_loops(loops, num_loops);
	}
	if (loops != NULL) free(loops);
    if (loop_extended) read_jobs_from_loops(q_a);

}


//select Applications.* from Applications join Jobs on job_id = id where Jobs.end_time in (select end_time from (select end_time from Jobs where user_id = "xjcorbalan" and id = 284360 order by end_time desc limit 25) as t1) order by Jobs.end_time desc;
//select Applications.* from Applications join Jobs on job_id=id where Jobs.user_id = "xjcorbalan" group by job_id order by Jobs.end_time desc limit 5;
#if USE_DB
void read_from_database(char *user, query_adds_t *q_a) 
{
	int num_apps = 0, i = 0;
	char subquery[256];
	char query[512];

	if (strlen(my_conf.database.user_commands) < 1) 
	{
		fprintf(stderr, "Warning: commands' user is not defined in ear.conf\n");
	}
	else
	{
		strcpy(my_conf.database.user, my_conf.database.user_commands);
		strcpy(my_conf.database.pass, my_conf.database.pass_commands);
	}
	init_db_helper(&my_conf.database);

	set_signature_simple(my_conf.database.report_sig_detail);

	if (verbose) {
		printf("Preparing query statement\n");
	}

	sprintf(query, "SELECT Applications.* FROM Applications join Jobs on job_id=id and Applications.step_id = Jobs.step_id where Jobs.id in (select id from (select id, end_time from Jobs" );
	application_t *apps;
	if (q_a->job_id >= 0)
		add_int_filter(query, "id", q_a->job_id);
	else if (strlen(q_a->job_ids) > 0)
		add_int_list_filter(query, "id", q_a->job_ids);
	if (q_a->step_id >= 0)
		add_int_filter(query, "step_id", q_a->step_id);
	if (user != NULL)
		add_string_filter(query, "user_id", user);
	if (strlen(q_a->e_tag) > 0)
		add_string_filter(query, "e_tag", q_a->e_tag);
	if (strlen(q_a->app_id) > 0)
		add_string_filter(query, "app_id", q_a->app_id);
	if (q_a->start_time != 0)
		add_int_comp_filter(query, "start_time", q_a->start_time, 1);
	if (q_a->end_time != 0)
		add_int_comp_filter(query, "end_time", q_a->end_time, 0);

	if (q_a->limit > 0)
	{
		sprintf(subquery, " ORDER BY Jobs.end_time desc LIMIT %d", q_a->limit);
		strcat(query, subquery);
	}
	strcat(query, ") as t1");

	query_filters = 0;
	if (q_a->job_id >= 0)
		add_int_filter(query, "id", q_a->job_id);
	else if (strlen(q_a->job_ids) > 0)
		add_int_list_filter(query, "id", q_a->job_ids);
	if (q_a->step_id >= 0)
		add_int_filter(query, "Jobs.step_id", q_a->step_id);
	if (user != NULL)
		add_string_filter(query, "user_id", user);

	strcat(query, ") order by Jobs.id desc, Jobs.step_id desc, Jobs.end_time desc");

	if (verbose) {
		printf("Retrieving applications\n");
	}

	if (verbose) {
		printf("QUERY: %s\n", query);
	}

	num_apps = db_read_applications_query(&apps, query);

	if (verbose) {
		printf("Finalized retrieving applications\n");
	}

	if (num_apps == EAR_MYSQL_ERROR)
	{
		exit(1); //error
	}

	if (num_apps < 1)
	{
		printf("No jobs found.\n");
		return;
	}


	if (q_a->limit == 20 && strlen(csv_path) < 1)
		printf("\nBy default only the first 20 jobs are retrieved.\n\n");

	if (strlen(csv_path) < 1)
	{

		if (full_length) print_full_apps(apps, num_apps);
		else print_short_apps(apps, num_apps, STDOUT_FILENO, 0);
	}

	else
	{
		if (full_length) 
		{
			if (!strcmp(csv_path, "no_file")) 
			{
				for (i = 0; i < num_apps; i++)
					print_application_fd(STDOUT_FILENO, &apps[i], my_conf.database.report_sig_detail, 1, 0);
			} 
			else
			{
				for (i = 0; i < num_apps; i++) {
					append_application_text_file(csv_path, &apps[i], my_conf.database.report_sig_detail, 1, 0);
				}
			}
		}
		else
		{
			if (!strcmp(csv_path, "no_file")) 
			{
				int fd = STDOUT_FILENO;
				print_short_apps(apps, num_apps, fd, 1);
			}
			else 
			{
				int fd = open(csv_path, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR|S_IWUSR|S_IRGRP);
				print_short_apps(apps, num_apps, fd, 1);
				close(fd);
			}
		}
		if (strcmp(csv_path, "no_file")) //don't print the message if we are outputting to stdout
			printf("Successfully written applications to csv. Only applications with EARL will have its information properly written.\n");
	}

	free(apps);

}
#endif

void read_from_files(char *path, char *user, query_adds_t *q_a)
{
	application_t *apps;
	int num_apps = read_application_text_file(path, &apps, 0);
	if (full_length) print_full_apps(apps, num_apps);
	else print_short_apps(apps, num_apps, STDOUT_FILENO, 0);
}

int main(int argc, char *argv[])
{
	int c;

	query_adds_t query_adds;
	memset(&query_adds, 0, sizeof(query_adds_t));

	query_adds.limit = 20;
	query_adds.job_id = -1;
	query_adds.step_id = -1;

	char is_loops = 0;
	char is_events = 0;

	char path_name[256];
	char *file_name = NULL;

    struct tm tinfo = {0};

	verb_level = -1;
	verb_enabled = 0;
	if (get_ear_conf_path(path_name)==EAR_ERROR){
		fprintf(stderr, "Error getting ear.conf path\n");
		exit(1);
	}

	if (read_cluster_conf(path_name, &my_conf) != EAR_SUCCESS) {
		fprintf(stderr, "ERROR reading cluster configuration\n");
	}

	user_t user_info;
	if (user_all_ids_get(&user_info) != EAR_SUCCESS)
	{
		fprintf(stderr, "Failed to retrieve user data\n");
		exit(1);
	}

	char *user = user_info.ruid_name;
	if (getuid() == 0 || is_privileged_command(&my_conf))
	{
		user = NULL; //by default, privilegd users or root will query all user jobs
	}

	char *token;
	int option_idx;
	static struct option long_options [] = {
		{"help",       no_argument, 0, 'h'},
		{"version",    no_argument, 0, 'v'},
		{"no-mpi",     no_argument, 0, 'm'},
		{"avx",        no_argument, 0, 'p'},
		{"verbose",    no_argument, 0, 'b'},
		{"show-gpus",  no_argument, 0, 'g'},
		{"long-apps",  no_argument, 0, 'l'},
		{"loops",      no_argument, 0, 'r'},
		{"ext_loops",  no_argument, 0, 'o'},
		{"help",       no_argument, 0, 'h'},
		{"limit",      required_argument, 0, 'n'},
		{"user",       required_argument, 0, 'u'},
		{"jobs",       required_argument, 0, 'j'},
		{"events",     required_argument, 0, 'x'},
		{"csv",        required_argument, 0, 'c'},
		{"tag",        required_argument, 0, 't'},
		{"app-id",     required_argument, 0, 'a'},
		{"start-time", required_argument, 0, 's'},
		{"end-time",   required_argument, 0, 'e'},
	};

	while (1)
	{
		c = getopt_long(argc, argv, "n:u:j:f:t:voma:pbglrs:e:c:hx::", long_options, &option_idx);

		if (c == -1)
			break;

		switch (c)
		{
			case 'n':
				if (!strcmp(optarg, "all")) query_adds.limit = -1;
				else query_adds.limit = atoi(optarg);
				break;
			case 'r':
				is_loops = 1;
				break;
			case 'u':
				if (user != NULL) break;
				user = optarg;
				break;
			case 'j':
				query_adds.limit = query_adds.limit == 20 ? -1 : query_adds.limit; //if the limit is still the default
				if (strchr(optarg, ','))
				{
					strcpy(query_adds.job_ids, optarg);
				}
				else
				{
					query_adds.job_id = atoi(strtok(optarg, "."));
					token = strtok(NULL, ".");
					if (token != NULL) query_adds.step_id = atoi(token);
				}
				break;
			case 'x':
				is_events = 1;
				if (optind < argc && strchr(argv[optind], '-') == NULL)
				{
					if (strchr(argv[optind], ','))
					{
						strcpy(query_adds.job_ids, argv[optind]);
					}
					else
					{
						query_adds.job_id = atoi(strtok(argv[optind], "."));
						token = strtok(NULL, ".");
						if (token != NULL) query_adds.step_id = atoi(token);
					}
				}
				else if (verbose) printf("No argument for -x\n");
				break;
			case 'o':
				loop_extended = 1;
				break;
			case 'g':
				print_gpus = 1;
				break;
			case 'f':
				file_name = optarg;
				break;
			case 'l':
				full_length = 1;
				break;
			case 'b':
				verbose = 1;
				break;
			case 'v':
				free_cluster_conf(&my_conf);
				print_version();
				exit(0);
				break;
			case 'm':
				all_mpi = 1;
				break;
			case 'c':
				strcpy(csv_path, optarg);
				break;
			case 't':
				strcpy(query_adds.e_tag, optarg);
				break;
			case 'p':
				avx = 1;
				break;
			case 'a':
				strcpy(query_adds.app_id, optarg);
				break;
			case 's':
				if (strptime(optarg, "%Y-%m-%e", &tinfo) == NULL)
				{
					printf("Incorrect time format. Supported format is YYYY-MM-DD\n"); //error
					free_cluster_conf(&my_conf);
					exit(1);
				}
				query_adds.start_time = mktime(&tinfo);
				break;
			case 'e':
				if (strptime(optarg, "%Y-%m-%e", &tinfo) == NULL)
				{
					printf("Incorrect time format. Supported format is YYYY-MM-DD\n"); //error
					free_cluster_conf(&my_conf);
					exit(1);
				}
				query_adds.end_time = mktime(&tinfo);
				break;
			case 'h':
				free_cluster_conf(&my_conf);
				usage(argv[0]);
				break;
		}
	}

	if (verbose) printf("Limit set to %d\n", query_adds.limit);

	if (file_name != NULL) read_from_files(file_name, user, &query_adds);
	else if (is_events) read_events(user, &query_adds);
	else if (is_loops) read_loops(user, &query_adds);
	else read_from_database(user, &query_adds); 

	free_cluster_conf(&my_conf);
	exit(0);
}
