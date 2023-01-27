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

// #define SHOW_DEBUGS 1

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

int loop_init(loop_t *loop, ulong job_id, ulong step_id, const char *node_id, ulong event, ulong size, ulong level)
{
    int ret;

    if ((ret=create_loop(loop)) != EAR_SUCCESS) return ret;

    if ((ret=create_loop_id(&loop->id, event, size, level)) != EAR_SUCCESS) return ret;

    loop->jid     = job_id;
    loop->step_id = step_id;

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
#if 0
    char *HEADER = "NODENAME;USERNAME;JOB_ID;APPNAME;POLICY;POLICY_TH;AVG.CPUFREQ;AVG.IMC.FREQ;DEF.FREQ;" \
                    "TIME;CPI;TPI;GBS;IO_MBS;P.MPI;DC-NODE-POWER;DRAM-POWER;PCK-POWER;CYCLES;INSTRUCTIONS;GFLOPS;L1_MISSES;" \
                    "L2_MISSES;L3_MISSES;SP_SINGLE;SP_128;SP_256;SP_512;DP_SINGLE;DP_128;DP_256;" \
                    "DP_512;FIRST_EVENT;LEVEL;SIZE;ITERATIONS";

    int fd, ret;

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
#endif
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
    char *HEADER_NOTS = "JOBID;STEPID;NODENAME;AVG_CPUFREQ_KHZ;AVG_IMCFREQ_KHZ;DEF_FREQ_KHZ;" \
                         "ITER_TIME_SEC;CPI;TPI;MEM_GBS;IO_MBS;PERC_MPI;DC_NODE_POWER_W;DRAM_POWER_W;PCK_POWER_W;CYCLES;INSTRUCTIONS;GFLOPS;L1_MISSES;" \
                         "L2_MISSES;L3_MISSES;SPOPS_SINGLE;SPOPS_128;SPOPS_256;SPOPS_512;DPOPS_SINGLE;DPOPS_128;DPOPS_256;" \
                         "DPOPS_512";

#if USE_GPUS
    char gpu_header[128];
    char *HEADER_GPU_SIG = ";GPU%d_POWER_W;GPU%d_FREQ_KHZ;GPU%d_MEM_FREQ_KHZ;GPU%d_UTIL_PERC;GPU%d_MEM_UTIL_PERC";
#endif
#if REPORT_TIMESTAMP
    char *HEADER_LOOP = ";LOOPID;LOOP_NEST_LEVEL;LOOP_SIZE;TIMESTAMP";
#else
    char *HEADER_LOOP = ";LOOPID;LOOP_NEST_LEVEL;LOOP_SIZE;ITERATIONS";
#endif
    char HEADER[1024] = "";

    if (file_is_regular(path)) {
        debug("%s is a regular file", path);
        return EAR_SUCCESS;
    }

    if (header != NULL) strncpy(HEADER, header, sizeof(HEADER));
    strncat(HEADER,HEADER_NOTS,sizeof(HEADER));

#if USE_GPUS
    if (single_column) num_gpus = ear_min(1, num_gpus);
    for (uint j = 0; j < num_gpus; ++j) {
        sprintf(gpu_header,HEADER_GPU_SIG,j,j,j,j,j);
        strncat(HEADER,gpu_header,strlen(gpu_header));
    }
#endif

    strncat(HEADER, HEADER_LOOP,sizeof(HEADER));

    if (ts) {
        xstrncat(HEADER, ";ELAPSED", sizeof(HEADER));
    }

    int fd = open(path, OPTIONS, PERMISSION);
    if (fd < 0) {
        debug("File %s could not be opened: %d", path, errno);
        return EAR_ERROR;
    }

    int ret = dprintf(fd, "%s\n", HEADER);

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
    return append_loop_text_file_no_job_int(path,loop, 0, 0, add_header, single_column, sep);
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

#if 0
    char *HEADER_NOTS = "JID;STEPID;NODENAME;AVG.CPUFREQ;AVG.IMCFREQ;DEF.FREQ;" \
                         "TIME;CPI;TPI;GBS;IO_MBS;P.MPI;DC-NODE-POWER;DRAM-POWER;PCK-POWER;CYCLES;INSTRUCTIONS;GFLOPS;L1_MISSES;" \
                         "L2_MISSES;L3_MISSES;SP_SINGLE;SP_128;SP_256;SP_512;DP_SINGLE;DP_128;DP_256;" \
                         "DP_512";

#if USE_GPUS
    char gpu_header[128];
    char *HEADER_GPU_SIG = ";GPOWER%d;GFREQ%d;GMEMFREQ%d;GUTIL%d;GMEMUTIL%d";
#endif

    char *HEADER_LOOP = ";FIRST_EVENT;LEVEL;SIZE;ITERATIONS";
    char HEADER[1024];

    strncpy(HEADER,HEADER_NOTS,sizeof(HEADER));
#if USE_GPUS
    for (uint j = 0; j < loop->signature.gpu_sig.num_gpus; ++j) {
        sprintf(gpu_header,HEADER_GPU_SIG,j,j,j,j,j);
        strncat(HEADER,gpu_header,strlen(gpu_header));
    }
#endif
    strncat(HEADER,HEADER_LOOP,sizeof(HEADER));
    if (ts){ 
        xstrncat(HEADER,";TIMESTAMP",sizeof(HEADER));
    }


    int fd, ret;

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
        error("Couldn't open loop file\n");
        return EAR_ERROR;
    }
#endif
    int fd;
    fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) {
        error("Couldn't open loop file: %d", errno);
        return EAR_ERROR;
    }

    assert(loop!=NULL);
    assert(loop->node_id!=NULL);
    dprintf(fd,"%lu;%lu;",loop->jid,loop->step_id);
    dprintf(fd, "%s;", loop->node_id);
    signature_print_fd(fd, &loop->signature,1, single_column, sep);
    print_loop_id_fd(fd, &loop->id);
    if (ts){
        dprintf(fd, "%lu;%llu", loop->total_iterations, currtime);
    }else{
        dprintf(fd, "%lu", loop->total_iterations);
    }
    dprintf(fd,"\n");

    close(fd);


    return EAR_SUCCESS;
}
