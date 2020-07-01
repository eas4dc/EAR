/********************************************************
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

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/system/file.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/application.h>

void init_application(application_t *app)
{
    memset(app, 0, sizeof(application_t));
}

void copy_application(application_t *destiny, application_t *source)
{
	memcpy(destiny, source, sizeof(application_t));
}

void application_print_channel(FILE *file, application_t *app)
{
	fprintf(file, "application_t: id '%s', job id '%lu.%lu', node id '%s'\n",
		app->job.app_id, app->job.id, app->job.step_id, app->node_id);
	fprintf(file, "application_t: learning '%d', time '%lf', avg freq '%lu'\n",
		app->is_learning, app->signature.time, app->signature.avg_f);
	fprintf(file, "application_t: pow dc '%lf', pow ram '%lf', pow pack '%lf'\n",
		app->signature.DC_power, app->signature.DRAM_power, app->signature.PCK_power);
}

/*
 *
 * Obsolete?
 *
 */

#define APP_TEXT_FILE_FIELDS	33
#define EXTENDED_DIFF			11
#define PERMISSION              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define OPTIONS                 O_WRONLY | O_CREAT | O_TRUNC | O_APPEND

int read_application_text_file(char *path, application_t **apps, char is_extended)
{
	char line[PIPE_BUF];
	application_t *apps_aux, *a;
	int lines, i, ret;
	FILE *fd;

	if (path == NULL) {
		return EAR_ERROR;
	}

	if ((fd = fopen(path, "r")) == NULL) {
		return EAR_OPEN_ERROR;
	}

	// Reading the header
	fscanf(fd, "%s\n", line);
	lines = 0;

	while(fscanf(fd, "%s\n", line) > 0) {
		lines += 1;
	}

	// Allocating memory
	apps_aux = (application_t *) malloc(lines * sizeof(application_t));
	memset(apps_aux, 0, sizeof(application_t));

	// Rewind
	rewind(fd);
	fscanf(fd, "%s\n", line);

	i = 0;
	a = apps_aux;

	while((ret = scan_application_fd(fd, a, is_extended)) == (APP_TEXT_FILE_FIELDS - !(is_extended)*EXTENDED_DIFF))
	{
		apps_aux[i].is_mpi = 1;
		i += 1;
		a = &apps_aux[i];
	}

	fclose(fd);

	if (ret >= 0 && ret < APP_TEXT_FILE_FIELDS) {
		free(apps_aux);
		return EAR_ERROR;
	}

	*apps = apps_aux;
	return i;
}

static int print_application_fd(int fd, application_t *app, int new_line, char is_extended)
{
	char buff[1024];
	sprintf(buff, "%s;", app->node_id);
	write(fd, buff, strlen(buff));
	print_job_fd(fd, &app->job);
	dprintf(fd, ";");
	signature_print_fd(fd, &app->signature, is_extended);

	if (new_line) {
		dprintf(fd, "\n");
	}

	return EAR_SUCCESS;
}

int append_application_text_file(char *path, application_t *app, char is_extended)
{
	//lacking: NODENAME(node_id in loop_t), not linked to any loop
	char *HEADER;
	int fd, ret;

	if (is_extended) {
		HEADER = "NODE_ID;JOB_ID;STEP_ID;USER_ID;GROUP_ID;APP_ID;USER_ACC;ENERGY_TAG;POLICY;"\
        "POLICY_TH;AVG.FREQ;DEF.FREQ;TIME;CPI;TPI;GBS;DC-NODE-POWER;DRAM-POWER;PCK-POWER;CYCLES;"\
        "INSTRUCTIONS;GFLOPS;L1_MISSES;L2_MISSES;L3_MISSES;SP_SINGLE;SP_128;SP_256;SP_512;DP_SINGLE;"\
        "DP_128;DP_256;DP_512";

	} else {
		HEADER = "NODE_ID;JOB_ID;STEP_ID;USER_ID;GROUP_ID;APP_ID;USER_ACC;ENERGY_TAG;POLICY;"\
        "POLICY_TH;AVG.FREQ;DEF.FREQ;TIME;CPI;TPI;GBS;DC-NODE-POWER;DRAM-POWER;PCK-POWER;CYCLES;"\
        "INSTRUCTIONS;GFLOPS";
	}

	if (path == NULL) {
		return EAR_ERROR;
	}

	fd = open(path, O_WRONLY | O_APPEND);

	if (fd < 0)
	{
		if (errno == ENOENT)
		{
			fd = open(path, OPTIONS, PERMISSION);

			// Write header
			if (fd >= 0) {
				ret = dprintf(fd, "%s\n", HEADER);
			}
		}
	}

	if (fd < 0) {
		return EAR_ERROR;
	}

	print_application_fd(fd, app, 1, is_extended);

	close(fd);

	if (ret < 0) return EAR_ERROR;
	return EAR_SUCCESS;
}

int scan_application_fd(FILE *fd, application_t *app, char is_extended)
{
	application_t *a;
	int ret;

	a = app;
	if (is_extended)
		ret = fscanf(fd, "%[^;];%lu;%lu;%[^;];" \
			 "%[^;];%[^;];%[^;];%[^;];%[^;];%lf;" \
			 "%lu;%lu;" \
			 "%lf;%lf;%lf;%lf;" \
			 "%lf;%lf;%lf;" \
			 "%llu;%llu;%lf;" \
			 "%llu;%llu;%llu;" \
			 "%llu;%llu;%llu;%llu;"	\
			 "%llu;%llu;%llu;%llu\n",
					 a->node_id, &a->job.id, &a->job.step_id, a->job.user_id,
					 a->job.group_id, a->job.app_id, a->job.user_acc, a->job.energy_tag, a->job.policy, &a->job.th,
					 &a->signature.avg_f, &a->signature.def_f,
					 &a->signature.time, &a->signature.CPI, &a->signature.TPI, &a->signature.GBS,
					 &a->signature.DC_power, &a->signature.DRAM_power, &a->signature.PCK_power,
					 &a->signature.cycles, &a->signature.instructions, &a->signature.Gflops,
					 &a->signature.L1_misses, &a->signature.L2_misses, &a->signature.L3_misses,
					 &a->signature.FLOPS[0], &a->signature.FLOPS[1], &a->signature.FLOPS[2], &a->signature.FLOPS[3],
					 &a->signature.FLOPS[4], &a->signature.FLOPS[5], &a->signature.FLOPS[6], &a->signature.FLOPS[7]);

	else
		ret = fscanf(fd, "%[^;];%lu;%lu;%[^;];" \
			 "%[^;];%[^;];%[^;];%[^;];%[^;];%lf;" \
			 "%lu;%lu;" \
			 "%lf;%lf;%lf;%lf;" \
			 "%lf;%lf;%lf;" \
			 "%llu;%llu;%lf\n",
					 a->node_id, &a->job.id, &a->job.step_id, a->job.user_id,
					 a->job.group_id, a->job.app_id, a->job.user_acc, a->job.energy_tag, a->job.policy, &a->job.th,
					 &a->signature.avg_f, &a->signature.def_f,
					 &a->signature.time, &a->signature.CPI, &a->signature.TPI, &a->signature.GBS,
					 &a->signature.DC_power, &a->signature.DRAM_power, &a->signature.PCK_power,
					 &a->signature.cycles, &a->signature.instructions, &a->signature.Gflops);

	return ret;
}

/*
 *
 * I don't know what is this
 *
 */

void print_application_fd_binary(int fd,application_t *app)
{
	write(fd,app,sizeof(application_t));
}

void read_application_fd_binary(int fd,application_t *app)
{
	read(fd,app,sizeof(application_t));
}

/*
 *
 * We have to take a look these print functions and clean
 *
 */

int print_application(application_t *app)
{
	return print_application_fd(STDOUT_FILENO, app, 1, 1);
}
void verbose_application_data(uint vl,application_t *app)
{
	float avg_f = ((double) app->signature.avg_f) / 1000000.0;
	float def_f = ((double) app->job.def_f) / 1000000.0;
	float pavg_f = ((double) app->power_sig.avg_f) / 1000000.0;
	float pdef_f = ((double) app->power_sig.def_f) / 1000000.0;
	char st[64],et[64];
	char stmpi[64],etmpi[64];
	struct tm *tmp;
	tmp=localtime(&app->job.start_time);
	strftime(st, sizeof(st), "%c", tmp);
	tmp=localtime(&app->job.end_time);
	strftime(et, sizeof(et), "%c", tmp);
	tmp=localtime(&app->job.start_mpi_time);
	strftime(stmpi, sizeof(stmpi), "%c", tmp);
	tmp=localtime(&app->job.end_mpi_time);
	strftime(etmpi, sizeof(etmpi), "%c", tmp);


	verbose(vl,"----------------------------------- Application Summary[%s] --\n",app->node_id);
	verbose(vl,"-- App id: %s, user id: %s, job id: %lu.%lu", app->job.app_id, app->job.user_id, app->job.id,app->job.step_id);
	verbose(vl,"   procs: %lu  acc %s\n", app->job.procs,app->job.user_acc);
	verbose(vl,"   start time %s end time %s start mpi %s end mpi %s\n",st,et,stmpi,etmpi);
	verbose(vl,"-- power_sig: E. time: %0.3lf (s), nom freq: %0.3f (MHz), avg freq: %0.3f (MHz), ", app->power_sig.time, pdef_f, pavg_f);
	verbose(vl,"DC/DRAM/PCK power: %0.3lf/%0.3lf/%0.3lf (W)\n", app->power_sig.DC_power, app->power_sig.DRAM_power,
			app->power_sig.PCK_power);
	verbose(vl,"\tmax_DC_power/min_DC_power: %0.3lf/%0.3lf (W)\n",app->power_sig.max_DC_power,app->power_sig.min_DC_power);
	if (app->is_mpi){

		verbose(vl,"-- mpi_sig: E. time: %0.3lf (s), nom freq: %0.3f (MHz), avg freq: %0.3f (MHz), ", app->signature.time, def_f, avg_f);
		verbose(vl,"   procs: %lu (s)\n", app->job.procs);
		verbose(vl,"\tCPI/TPI: %0.3lf/%0.3lf, GB/s: %0.3lf, GFLOPS: %0.3lf, ", app->signature.CPI, app->signature.TPI,
				app->signature.GBS, app->signature.Gflops);
		verbose(vl,"  DC/DRAM/PCK power: %0.3lf/%0.3lf/%0.3lf (W)\n", app->signature.DC_power, app->signature.DRAM_power,
				app->signature.PCK_power);
	}
	verbose(vl,"-----------------------------------------------------------------------------------------------\n");
}
void report_application_data(application_t *app)
{
	verbose_application_data(VTYPE,app);
}

void report_mpi_application_data(application_t *app)
{
	float avg_f = ((double) app->signature.avg_f) / 1000000.0;
	float def_f = ((double) app->job.def_f) / 1000000.0;
	//float pavg_f = ((double) app->power_sig.avg_f) / 1000000.0;
	//float pdef_f = ((double) app->power_sig.def_f) / 1000000.0;

	verbose(VTYPE,"---------------------------------------------- Application Summary [%s] --\n",app->node_id);
	verbose(VTYPE,"-- App id: %s, user id: %s, job id: %lu.%lu", app->job.app_id, app->job.user_id, app->job.id,app->job.step_id);
	verbose(VTYPE,"   acc %s\n", app->job.user_acc);
	if (app->is_mpi){

		verbose(VTYPE,"-- mpi_sig: E. time: %0.3lf (s), nom freq: %0.3f (MHz), avg freq: %0.3f (MHz), ", app->signature.time, def_f, avg_f);
		verbose(VTYPE,"   tasks: %lu \n", app->job.procs);
		verbose(VTYPE,"\tCPI/TPI: %0.3lf/%0.3lf, GB/s: %0.3lf, GFLOPS: %0.3lf, ", app->signature.CPI, app->signature.TPI,
				app->signature.GBS, app->signature.Gflops);
		verbose(VTYPE,"  DC/DRAM/PCK power: %0.3lf/%0.3lf/%0.3lf (W) GFlops/Watts %.3lf\n", app->signature.DC_power, app->signature.DRAM_power,
				app->signature.PCK_power,app->signature.Gflops/app->signature.DC_power);
	}
	verbose(VTYPE,"-----------------------------------------------------------------------------------------------\n");
}
