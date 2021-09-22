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

#include <stdlib.h>
#include <string.h>
#include <errno.h>  
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <common/config.h>
#include <common/system/time.h>
#include <common/types/loop.h>
#include <common/states.h>


#define PERMISSION S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define OPTIONS O_WRONLY | O_CREAT | O_TRUNC | O_APPEND


int create_loop_id(loop_id_t *lid,ulong event, ulong size, ulong level)
{
    if (lid!=NULL){
        lid->event = event;
        lid->size = size;
        lid->level = level;
        return EAR_SUCCESS;
    }else return EAR_ERROR;
}

int create_loop(loop_t *l)
{
    if (l!=NULL)
    {
        l->jid=0;
        l->step_id=0;
        l->total_iterations=0;
        memset(&l->signature,0,sizeof(signature_t));
        return EAR_SUCCESS;
    }else return EAR_ERROR;
}

int loop_init(loop_t *loop, job_t *job,ulong event, ulong size, ulong level)
{
    int ret;
    if ((ret=create_loop(loop))!=EAR_SUCCESS) return ret;
    if ((ret=create_loop_id(&loop->id,event,size,level))!=EAR_SUCCESS) return ret;
    loop->jid = job->id;
    loop->step_id=job->step_id;
    gethostname(loop->node_id,sizeof(loop->node_id));
    strtok(loop->node_id,".");
    return EAR_SUCCESS;
}

void clean_db_loop(loop_t *loop, double limit)
{
    if (loop->id.event > INT_MAX) loop->id.event = INT_MAX;
    if (loop->id.size > INT_MAX) loop->id.size = INT_MAX; 
    if (loop->id.level > INT_MAX) loop->id.level = INT_MAX;
    clean_db_signature(&loop->signature, limit);
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
    print_loop_id_fd(fd, &loop->id);
    signature_print_fd(fd, &loop->signature, 1);
    dprintf(fd, "%lu\n", loop->total_iterations);
}


int append_loop_text_file(char *path, loop_t *loop,job_t *job)
{
#if DB_FILES
    if (path == NULL) {
        return EAR_ERROR;
    }

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
    assert(loop!=NULL);
    assert(loop->node_id!=NULL);
    dprintf(fd, "%s;", loop->node_id);
    print_job_fd(fd, job);
    signature_print_fd(fd, &loop->signature,1);
    print_loop_id_fd(fd, &loop->id);
    dprintf(fd, "%lu\n", loop->total_iterations);

    close(fd);

    if (ret < 0) return EAR_ERROR;
#endif

    return EAR_SUCCESS;
}

static int append_loop_text_file_no_job_int(char *path, loop_t *loop,int ts,ullong currtime);
int append_loop_text_file_no_job(char *path, loop_t *loop)
{
    return append_loop_text_file_no_job_int(path,loop,0,0);
}

int append_loop_text_file_no_job_with_ts(char *path, loop_t *loop, ullong currtime)
{
    return append_loop_text_file_no_job_int(path,loop,1, currtime);
}
static int append_loop_text_file_no_job_int(char *path, loop_t *loop,int ts, ullong currtime)
{
    char gpu_header[128];
    if (path == NULL) {
        return EAR_ERROR;
    }


    char *HEADER_NOTS = "JID;STEPID;NODENAME;AVG.CPUFREQ;AVG.IMCFREQ;DEF.FREQ;" \
                         "TIME;CPI;TPI;GBS;IO_MBS;P.MPI;DC-NODE-POWER;DRAM-POWER;PCK-POWER;CYCLES;INSTRUCTIONS;GFLOPS;L1_MISSES;" \
                         "L2_MISSES;L3_MISSES;SP_SINGLE;SP_128;SP_256;SP_512;DP_SINGLE;DP_128;DP_256;" \
                         "DP_512";

#if USE_GPUS
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
    assert(loop!=NULL);
    assert(loop->node_id!=NULL);
    dprintf(fd,"%lu;%lu;",loop->jid,loop->step_id);
    dprintf(fd, "%s;", loop->node_id);
    signature_print_fd(fd, &loop->signature,1);
    print_loop_id_fd(fd, &loop->id);
    if (ts){
        dprintf(fd, "%lu;%llu", loop->total_iterations, currtime);
    }else{
        dprintf(fd, "%lu", loop->total_iterations);
    }
    dprintf(fd,"\n");

    close(fd);

    if (ret < 0) return EAR_ERROR;

    return EAR_SUCCESS;
}
