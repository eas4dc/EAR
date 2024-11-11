/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <stdlib.h>
#include <string.h>
#include <errno.h>  
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define NDEBUG
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <common/config.h>
#include <common/system/time.h>
#include <common/system/file.h>
#include <common/types/loop.h>
#include <common/states.h>


#define PERMISSION S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define OPTIONS O_WRONLY | O_CREAT | O_TRUNC | O_APPEND


int create_loop_id(loop_id_t *lid, ulong event, ulong size, ulong level)
{
    if (lid != NULL) {
        lid->event = event;
        lid->size  = size;
        lid->level = level;

        return EAR_SUCCESS;
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }
}

int create_loop(loop_t *l)
{
    if (l != NULL)
    {
        l->jid              = 0;
        l->step_id          = 0;
        l->total_iterations = 0;

        memset(&l->signature, 0, sizeof(signature_t));

        return EAR_SUCCESS;
    } else {
        return_msg(EAR_ERROR, Generr.input_null);
    }
}

int loop_init(loop_t *loop, ulong job_id, ulong step_id, ulong local_id, const char *node_id, ulong event, ulong size, ulong level)
{
    int ret;

    if ((ret=create_loop(loop)) != EAR_SUCCESS) return ret;

    if ((ret=create_loop_id(&loop->id, event, size, level)) != EAR_SUCCESS) return ret;

    loop->jid      = job_id;
    loop->step_id  = step_id;
#if WF_SUPPORT
    loop->local_id = local_id;
#endif

    memcpy(loop->node_id, node_id, sizeof loop->node_id);

    return EAR_SUCCESS;
}

void clean_db_loop(loop_t *loop, double limit)
{
    if (loop->id.event > INT_MAX) loop->id.event = INT_MAX;
    if (loop->id.size > INT_MAX) loop->id.size = INT_MAX; 
    if (loop->id.level > INT_MAX) loop->id.level = INT_MAX;
    signature_clean_before_db(&loop->signature, limit);
}

int set_null_loop(loop_t *loop)
{
    return create_loop(loop);

}
/** Returns true if the loop data is not, return -1 in case of error */
int is_null(loop_t *loop)
{
    if (loop==NULL) return EAR_ERROR;
    return (loop->total_iterations==0);
}



void add_loop_signature(loop_t *loop,  signature_t *sig)
{
    memcpy(&loop->signature,sig,sizeof(signature_t));
}

void end_loop(loop_t *loop, ulong iterations)
{
    loop->total_iterations = iterations;
}

void copy_loop(loop_t *destiny, loop_t *source)
{
    memcpy(destiny, source, sizeof(loop_t));
}

void print_loop_id_fd(int fd, loop_id_t *loop_id)
{
    dprintf(fd, ";%lu;%lu;%lu;", loop_id->event, loop_id->level, loop_id->size);
}

void print_loop_fd(int fd, loop_t *loop)
{
    dprintf(fd,"%lu;%lu;", loop->jid, loop->step_id);
    dprintf(fd, "%s;", loop->node_id);
    signature_print_fd(fd, &loop->signature, 1, 0 , ' ');
    print_loop_id_fd(fd, &loop->id);
    dprintf(fd, "%lu\n", loop->total_iterations);
}

int append_loop_text_file(char *path, loop_t *loop,job_t *job, int add_header, int single_column, char sep)
{
#if DB_FILES
    if (path == NULL) {
        return EAR_ERROR;
    }
    uint num_gpus = 0;
#if USE_GPUS
    num_gpus = loop->signature.gpu_sig.num_gpus;
#endif

	if (add_header) create_loop_header(NULL, path, 0, num_gpus);
    int fd, ret;
    fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0){
      error("Couldn't open loop file\n");
      return EAR_ERROR;
    }

    assert(loop!=NULL);
    assert(loop->node_id!=NULL);
    dprintf(fd, "%s;", loop->node_id);
    print_job_fd(fd, job);
    signature_print_fd(fd, &loop->signature,1, single_column, sep);
    print_loop_id_fd(fd, &loop->id);
    dprintf(fd, "%lu\n", loop->total_iterations);

    close(fd);

    if (ret < 0) return EAR_ERROR;
#endif

    return EAR_SUCCESS;
}

int create_loop_header(char * header, char *path, int ts, uint num_gpus, int single_column)
{
#if WF_SUPPORT
	char *HEADER_JOB = "JOBID;STEPID;APPID";
#else
	char *HEADER_JOB = "JOBID;STEPID";
#endif // WF_SUPPORT

	char *HEADER_NOTS = ";NODENAME;AVG_CPUFREQ_KHZ;AVG_IMCFREQ_KHZ;DEF_FREQ_KHZ;"\
											 "ITER_TIME_SEC;CPI;TPI;MEM_GBS;IO_MBS;PERC_MPI;DC_NODE_POWER_W;"\
											 "DRAM_POWER_W;PCK_POWER_W;CYCLES;INSTRUCTIONS;GFLOPS;L1_MISSES;"\
											 "L2_MISSES;L3_MISSES;SPOPS_SINGLE;SPOPS_128;SPOPS_256;"\
											 "SPOPS_512;DPOPS_SINGLE;DPOPS_128;DPOPS_256;DPOPS_512";
#if WF_SUPPORT
		uint num_sockets = MAX_SOCKETS_SUPPORTED;
		debug("Creating header for CPU signature with %u sockets", num_sockets);
		char cpu_sig_hdr[256] = "";
		for (uint s = 0 ; s < num_sockets ; s++){
			char temp_hdr[16];
			snprintf(temp_hdr, sizeof(temp_hdr), ";TEMP%u", s);
			strcat(cpu_sig_hdr, temp_hdr);
		}
	
#else
	char *cpu_sig_hdr = "";
#endif

#if USE_GPUS
    char gpu_header[512];
#if WF_SUPPORT
    char *HEADER_GPU_SIG = ";GPU%d_POWER_W;GPU%d_FREQ_KHZ;GPU%d_MEM_FREQ_KHZ;"\
														"GPU%d_UTIL_PERC;GPU%d_MEM_UTIL_PERC;GPU%d_GFLOPS;"\
														"GPU%d_TEMP;GPU%d_MEMTEMP";
#else
    char *HEADER_GPU_SIG = ";GPU%d_POWER_W;GPU%d_FREQ_KHZ;GPU%d_MEM_FREQ_KHZ;"\
														"GPU%d_UTIL_PERC;GPU%d_MEM_UTIL_PERC";
#endif // WF_SUPPORT
#else
		char HEADER_GPU_SIG[1] = "\0";
#endif

#if REPORT_TIMESTAMP
    char *HEADER_LOOP = ";LOOPID;LOOP_NEST_LEVEL;LOOP_SIZE;TIMESTAMP";
#else
    char *HEADER_LOOP = ";LOOPID;LOOP_NEST_LEVEL;LOOP_SIZE;ITERATIONS";
#endif

		char HEADER_TS[16] = ";ELAPSED;DATE";
    if (!ts) {
			memset(HEADER_TS, 0, sizeof(HEADER_TS));
    }

		// Be careful if some day the number of GPUs has more than two digits :) .
		size_t header_len = strlen(HEADER_JOB) + strlen(HEADER_NOTS) + strlen(HEADER_GPU_SIG) * num_gpus \
			+ strlen(HEADER_LOOP) + strlen(HEADER_TS) + 1 + strlen(cpu_sig_hdr);
		if (header)
		{
			header_len += strlen(header);
		}

		// Allocate exactly what we need
    char *HEADER = calloc(header_len, sizeof(char));

    if (file_is_regular(path)) {
        debug("%s is a regular file", path);
				free(HEADER);
        return EAR_SUCCESS;
    }

    if (header != NULL) strncpy(HEADER, header, header_len - 1);
    strncat(HEADER, HEADER_JOB, header_len - strlen(HEADER) - 1);
    strncat(HEADER, HEADER_NOTS, header_len - strlen(HEADER) - 1);

		/* Temperature */
    strncat(HEADER, cpu_sig_hdr, header_len - strlen(HEADER) - 1);

#if USE_GPUS
    if (single_column) num_gpus = ear_min(1, num_gpus);
    for (uint j = 0; j < num_gpus; ++j) {
#if WF_SUPPORT
        sprintf(gpu_header,HEADER_GPU_SIG,j,j,j,j,j,j,j,j);
#else
        sprintf(gpu_header,HEADER_GPU_SIG,j,j,j,j,j);
#endif
        strncat(HEADER, gpu_header,  header_len - strlen(HEADER) - 1);
    }
#endif // USE_GPUS

    strncat(HEADER, HEADER_LOOP, header_len - strlen(HEADER) - 1);

    if (ts) {
        strncat(HEADER, HEADER_TS, header_len - strlen(HEADER) - 1);
    }

    int fd = open(path, OPTIONS, PERMISSION);
    if (fd < 0) {
        debug("File %s could not be opened: %d", path, errno);
				free(HEADER);
        return EAR_ERROR;
    }

    int ret = dprintf(fd, "%s\n", HEADER);

		free(HEADER);

    close(fd);

    if (ret < 0) {
        debug("The header (%s) could not be written to fd %d", HEADER, fd);
        return EAR_ERROR;
    }

    return EAR_SUCCESS;
}

static int append_loop_text_file_no_job_int(char *path, loop_t *loop, int ts, ullong currtime,
        int add_header, int single_column, char sep);

int append_loop_text_file_no_job(char *path, loop_t *loop, int add_header,
        int single_column, char sep)
{
    return append_loop_text_file_no_job_int(path,loop, 1, 0, add_header, single_column, sep);
}

int append_loop_text_file_no_job_with_ts(char *path, loop_t *loop, ullong currtime,
        int add_header, int single_column, char sep)
{
    return append_loop_text_file_no_job_int(path, loop, 1, currtime, add_header, single_column, sep);
}

static int append_loop_text_file_no_job_int(char *path, loop_t *loop,int ts, ullong currtime, int add_header, int single_column, char sep)
{
    if (path == NULL) {
        return EAR_ERROR;
    }

    uint num_gpus = 0;

#if USE_GPUS
    num_gpus = MAX_GPUS_SUPPORTED; // loop->signature.gpu_sig.num_gpus;
    if (single_column) num_gpus = ear_min(num_gpus,1);
#endif

    if (add_header) {
        if (create_loop_header(NULL, path, ts, num_gpus, single_column) < 0) {
            debug("%sWARNING%s Error creating loop header.", COL_RED, COL_CLR);
        }
    }

    int fd;
    fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) {
        error("Couldn't open loop file: %d", errno);
        return EAR_ERROR;
    }

    assert(loop!=NULL);
    assert(loop->node_id!=NULL);
#if WF_SUPPORT
    dprintf(fd,"%lu;%lu;%lu;",loop->jid,loop->step_id,loop->local_id);
#else
    dprintf(fd,"%lu;%lu;",loop->jid,loop->step_id);
#endif
    dprintf(fd, "%s;", loop->node_id);
    signature_print_fd(fd, &loop->signature,1, single_column, sep);
    print_loop_id_fd(fd, &loop->id);
    if (ts){
        struct tm *current_t;
        char s[256];
        current_t = localtime((time_t *)&loop->total_iterations);
        strftime(s, 256, "%d-%m-%Y %H:%M:%S", current_t);

        dprintf(fd, "%lu;%llu;%s", loop->total_iterations, currtime, s);
    }else{
        dprintf(fd, "%lu", loop->total_iterations);
    }
    dprintf(fd,"\n");

    close(fd);


    return EAR_SUCCESS;
}

void loop_serialize(serial_buffer_t *b, loop_t *loop)
{
    serial_dictionary_push_auto(b, loop->id.event);
    serial_dictionary_push_auto(b, loop->id.size);
    serial_dictionary_push_auto(b, loop->id.level);
    serial_dictionary_push_auto(b, loop->jid);
    serial_dictionary_push_auto(b, loop->step_id);
#if WF_SUPPORT
    serial_dictionary_push_auto(b, loop->local_id);
#endif
    serial_dictionary_push_auto(b, loop->node_id);
    serial_dictionary_push_auto(b, loop->total_iterations);
    signature_serialize(b, &loop->signature);
}

void loop_deserialize(serial_buffer_t *b, loop_t *loop)
{
    serial_dictionary_pop_auto(b, loop->id.event);
    serial_dictionary_pop_auto(b, loop->id.size);
    serial_dictionary_pop_auto(b, loop->id.level);
    serial_dictionary_pop_auto(b, loop->jid);
    serial_dictionary_pop_auto(b, loop->step_id);
#if WF_SUPPORT
    serial_dictionary_pop_auto(b, loop->local_id);
#endif
    serial_dictionary_pop_auto(b, loop->node_id);
    serial_dictionary_pop_auto(b, loop->total_iterations);
    signature_deserialize(b, &loop->signature);
}
