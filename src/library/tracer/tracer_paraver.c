/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <common/sizes.h>
#include <common/config.h>
#include <common/config/config_env.h>
//#define SHOW_DEBUGS 1
#include <common/system/time.h>
#include <common/output/verbose.h>
#include <common/types/signature.h>
#include <common/types/event_type.h>
#include <common/system/lock.h>
#include <library/tracer/tracer_paraver.h>

#ifdef EAR_GUI
static pthread_mutex_t trace_event_lock = PTHREAD_MUTEX_INITIALIZER;




static char buffer1[SZ_BUFFER];
static char buffer2[SZ_BUFFER];

static long long time_sta;
static long long time_end;

static int edit_time_header;
static int edit_time_states;
static int file_prv;
static int file_pcf;
static int file_row;
static int enabled;
static int working;

static int trace_fin=0;


static int my_trace_rank= 0;
static int my_num_nodes = 0;
static int my_num_ranks = 0;
static int my_num_ppn   = 0;
static char my_app[GENERIC_NAME];
static char  hostname[SZ_BUFFER];
static char *pathname;

#define PARAVER_EVENTS  400 
#define EVENTS_IN_BUFFER 200 
typedef struct paraver_events{
    long long t;
    int event; 
    ullong value;
}paraver_events_t;
static paraver_events_t events_list[PARAVER_EVENTS];
static int num_events=0;


#define TRA_VER	2

static long long metrics_usecs_diff(long long end, long long init)
{
    long long to_max;

    if (end < init)
    {
        debug( "Timer overflow (end: %lld - init: %lld)\n", end, init);
        to_max = LLONG_MAX - init;
        return (to_max + end);
    }

    return (end - init);
}

static void row_file_create(char *pathname, char *hostname, int n_nodes)
{
    #define write_unused(...) \
        if (write(__VA_ARGS__)) {}

    sprintf(buffer1, "%s/%d_%s.%d.row", pathname, my_trace_rank,my_app,getpid());
    if (my_trace_rank==1){
        file_row=open(buffer1,
                O_WRONLY | O_CREAT | O_TRUNC,
                S_IRUSR  | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (file_row<0) return;
        sprintf(buffer1,"LEVEL TASK SIZE %d\n",my_num_ranks);
        write_unused(file_row, buffer1, strlen(buffer1));
    }else{
        file_row=open(buffer1,
                O_WRONLY | O_CREAT | O_APPEND,
                S_IRUSR  | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (file_row<0) return;
    }
    xsprintf(buffer1,"%s %d\n",hostname,my_trace_rank);
    write_unused(file_row, buffer1, strlen(buffer1));
    close(file_row);
}

static void config_file_create(char *pathname, char* hostname)
{
    if (my_trace_rank>1) { return; }

    sprintf(buffer1, "%s/%d_%s.%d.pcf", pathname, my_trace_rank,my_app,getpid());

    file_pcf = open(buffer1,
            O_WRONLY | O_CREAT | O_TRUNC,
            S_IRUSR  | S_IWUSR | S_IRGRP |
            S_IWGRP  | S_IROTH | S_IWOTH);

    if (file_pcf<0) { return; }

    sprintf(buffer1, "GRADIENT_COLOR\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "0\t{255,255,255}\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "GRADIENT_NAMES\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "0\twhite\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t1\tRUN\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60001\tPERIOD_ID\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60002\tPERIOD_LENGTH\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60003\tPERIOD_ITERATIONS\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60004\tPERIOD_TIME\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tCPI\n\n", TRA_CPI);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tTPI\n\n", TRA_TPI);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tGBS\n\n", TRA_GBS);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tPOWER\n\n", TRA_POW);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60009\tPERIOD_TIME_PROJECTION\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60010\tPERIOD_CPI_PROJECTION\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60011\tPERIOD_POWER_PROJECTION\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tFREQUENCY\n\n", TRA_FRQ);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60013\tENERGY\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60014\tDYNAIS_ON_OFF\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60015\tEARL_STATE\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "VALUES\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1,"2\tEVALUATING_POLICY\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1,"3\tVALIDATING_POLICY\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1,"4\tPOLICY_ERROR\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60016\tEARL_MODE\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "VALUES\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1,"1\tDYNAIS\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1,"2\tPERIODIC\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t60017\tVPI\n\n");
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tPERC_MPI\n\n",PERC_MPI);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tAVG_CPUFREQ\n\n",TRA_AVGFRQ);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tPCK_POWER\n\n",TRA_PCK_POWER);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tDRAM_POWER\n\n",TRA_DRAM_POWER);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tIO_MBS\n\n",TRA_IO);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tGFLOPS\n\n",TRA_GFLOPS);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tJOB_POWER\n\n",TRA_JOB_POWER);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tNODE_POWER\n\n",TRA_NODE_POWER);
    write_unused(file_pcf, buffer1, strlen(buffer1));



    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tMPI_CALLS\n\n",MPI_CALLS);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tMPI_SYNC_CALLS\n\n",MPI_SYNC_CALLS);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tMPI_BLOCK_CALLS\n\n",MPI_BLOCK_CALLS);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tTIME_SYNC_CALLS\n\n",TIME_SYNC_CALLS);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tTIME_BLOCK_CALLS\n\n",TIME_BLOCK_CALLS);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tTIME_MAX_BLOCK\n\n",TIME_MAX_BLOCK);
    write_unused(file_pcf, buffer1, strlen(buffer1));


    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tPCK_POWER_HIGHF\n\n",TRA_PCK_POWER_NODE);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tDRAM_POWER_HIGHF\n\n",TRA_DRAM_POWER_NODE);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tIN_BARRIER\n\n",TRA_BARRIER);
    write_unused(file_pcf, buffer1, strlen(buffer1));
    sprintf(buffer1, "EVENT_TYPE\n0\t%d\tCPUF_RAMP_UP\n\n",TRA_CPUF_RAMP_UP);
    write_unused(file_pcf, buffer1, strlen(buffer1));

    close(file_pcf);
}

static void trace_file_open(char *pathname, char *hostname)
{
    //
    sprintf(buffer1, "%s/%d_%s.%d.prv", pathname, my_trace_rank,my_app,getpid());

    debug("Creating trace file %s",buffer1);
    //
    file_prv = open(buffer1,
            O_WRONLY | O_CREAT | O_TRUNC,
            S_IRUSR  | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    //printf("FD: %s %d %s %s %s\n", buffer1, file_prv, strerror(errno), pathname, hostname);
}

static void trace_file_init(int n_nodes)
{
    char *buffer = buffer1;
    // char *b;
    // int i, j;

    time_t curr_time;
    struct tm *current_t;
    char s[256];
    char short_b[128];

    if (!enabled) {
        return;
    }
    // #Paraver (27/10/2022 at 06:17):00000000000391465931:1(48):1:48:(1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1,1:1) 
    time(&curr_time);
    current_t = localtime(&curr_time);
    strftime(s, 256, "%d/%m/%Y at %H:%M", current_t);

    // Buffer position 0
    // Trace file header only generated by master_rank 0
    if (my_trace_rank==1)
    {
        /* Valid for 1 node */

        //i = sprintf(buffer, "#Paraver (%s):%020llu:%d(", s, (unsigned long long) 0, n_nodes);

        // i = sprintf(buffer, "#Paraver (%s):%020llu:%d(%d):1:%d:(", s, (unsigned long long) 0, 1,my_num_ranks,my_num_ranks);
        //
        // nodes(1,1,1,1,1,1):1:nodes(1:1,1:1,1:1,1:1,1:1,1:1)

        sprintf(buffer, "#Paraver (%s):%020llu:%d(%d):1:%d:(", s, (unsigned long long) 0, 1,my_num_ranks,my_num_ranks);

        edit_time_header = 31;

        for (uint i = 0; i < my_num_ranks - 1; i++) {
            sprintf(short_b, "1:1,");
            strcat(buffer, short_b);
        }

        sprintf(short_b, "1:1)\n");
        strcat(buffer, short_b);

        #if 0

        //
        for (b = &buffer[i], i = 0, j = 0; i < n_nodes; ++i, j += 2)
        {
            b[j+0] = '1';
            b[j+1] = ',';
        }

        // Buffer position 2
        b = &b[j-1];
        i = sprintf(b, "):1:%d(", n_nodes);

        // Buffer position 3
        for (b = &b[i], i = 0, j = 1; i < n_nodes; ++i, ++j)
        {
            sprintf(b, "1:%d,", j);
            b = &b[strlen(b)];
        }

        b = &b[strlen(b)-1];
        b[0] = ')';
        b[1] = '\n';
        b[2] = '\0';
        #endif

        write_unused(file_prv, buffer, strlen(buffer));
    }
    // 1:cpu:app:task:thread:b_time:e_time:state
    // i = sprintf(buffer, "1:%d:1:%d:1:0:", my_trace_rank, my_trace_rank);

    sprintf(buffer, "1:%d:1:%d:1:0:", my_trace_rank, my_trace_rank);

    write_unused(file_prv, buffer, strlen(buffer));

    //
    edit_time_states = lseek(file_prv, 0, SEEK_CUR);

    //
    sprintf(buffer, "%020llu:1\n", (unsigned long long) 0);
    write_unused(file_prv, buffer, strlen(buffer));
}

static void trace_file_write_in_file()
{
    int i;
    int events,pendings=num_events,ready=0;

    if (trace_fin) debug("End trace pendings %d",pendings);
    while(pendings>0){
        if (pendings<EVENTS_IN_BUFFER) events=pendings;
        else events=EVENTS_IN_BUFFER;
        buffer2[0] = '\0';
        if (trace_fin) debug("End trace events %d max %d limit %d",events,ready+events-1,PARAVER_EVENTS);
        for (i=0;i<events;i++){
            sprintf(buffer1,"2:%d:1:%d:1:%llu:%d:%llu\n", my_trace_rank, my_trace_rank,
                    events_list[ready+i].t,events_list[ready+i].event,events_list[ready+i].value);
            if ((strlen(buffer1)+strlen(buffer2))>SZ_BUFFER){
                write_unused(file_prv, buffer2, strlen(buffer2));
                buffer2[0]='\0';
            }	
            strcat(buffer2,buffer1);
        }
        write_unused(file_prv, buffer2, strlen(buffer2));
        ready+=events;
        pendings-=events;
    }
    num_events=0;
    if (trace_fin) debug("trace_file_write_in_file end");
}

static void trace_file_write(int event, ullong value)
{
    state_t s;
	  if (state_fail(s = ear_trylock(&trace_event_lock))) {
		  return ;
	  }
    if (num_events==PARAVER_EVENTS){
        trace_file_write_in_file();
    }
    long long my_time = metrics_usecs_diff((long long) timestamp_getconvert(TIME_USECS), time_sta);
    events_list[num_events].t=my_time;
    events_list[num_events].event=event;
    events_list[num_events].value=value;
    num_events++;
    ear_unlock(&trace_event_lock);

}
static void trace_file_write_simple_event(int event)
{
    state_t s;
	  if (state_fail(s = ear_trylock(&trace_event_lock))) {
		  return ;
	  }
    if (num_events==PARAVER_EVENTS-2){
        trace_file_write_in_file();
    }
    long long my_time = metrics_usecs_diff((long long) timestamp_getconvert(TIME_USECS), time_sta);
    events_list[num_events].t=my_time;
    events_list[num_events].event=event;
    events_list[num_events].value=1;
    num_events++;
    events_list[num_events].t=my_time+10;
    events_list[num_events].event=event;
    events_list[num_events].value=0;
    num_events++;
    ear_unlock(&trace_event_lock);

}

/*
 *
 *
 * ear_api.c
 *
 *
 */

void traces_start()
{
    working = 1;
}

void traces_stop()
{
    working = 0;
}

void traces_init(char *app,int global_rank, int local_rank, int nodes, int mpis, int ppn)
{
    pathname = ear_getenv(ENV_FLAG_PATH_TRACE);

    //
    file_prv = -1;
    file_pcf = -1;
    file_row = -1;

    if (pathname == NULL) {
        debug("trace path not defined %s, trace disabled", ENV_FLAG_PATH_TRACE);
        enabled = 0;
        return;
    }
    debug("Using trace path %s",pathname);
    my_trace_rank = global_rank+1;
    strcpy(my_app,app);

    //
    time_sta = (long long) timestamp_getconvert(TIME_USECS);

    //
    gethostname(hostname, SZ_BUFFER);
    strtok(hostname,".");

    //
    trace_file_open(pathname, hostname);
    enabled = (file_prv >= 0);

    //
    my_num_nodes = nodes;
    my_num_ranks = mpis;
    my_num_ppn   = ppn;
    trace_file_init(nodes);

    if(!enabled) {
        return;
    }

    //
    config_file_create(pathname, hostname);
    if (my_trace_rank>=1) row_file_create(pathname, hostname, nodes);
}

// ear_api.c
void traces_end(int global_rank, int local_rank, unsigned long total_energy)
{
    trace_fin=1;
    //
    time_end = metrics_usecs_diff((long long) timestamp_getconvert(TIME_USECS), time_sta);

    //
    trace_file_write(TRA_ENE, total_energy);	
    trace_file_write_in_file();

    // Post process
    sprintf(buffer1, "%020llu", time_end);

    if (my_trace_rank==1){
        lseek(file_prv, edit_time_header, SEEK_SET);
        write_unused(file_prv, buffer1, 20);
    }

    lseek(file_prv, edit_time_states, SEEK_SET);
    write_unused(file_prv, buffer1, 20);

    //
    close(file_prv);



    //
    enabled = 0;
    working = 0;
}

// ear_states.c
void traces_new_period(int global_rank, int local_rank, ullong loop_id)
{
}

// ear_api.c
void traces_new_n_iter(int global_rank, int local_rank, ullong loop_id, int loop_size, int iterations)
{
#if 0
    if (!enabled || !working) {
        return;
    }

    trace_file_write(TRA_ID, (ullong) loop_id);
    trace_file_write(TRA_LEN, (ullong) loop_size);
    trace_file_write(TRA_ITS, (ullong) iterations);
#endif
}

// ear_api.c
void traces_end_period(int global_rank, int local_rank)
{
    if (!enabled || !working) {
        return;
    }

    trace_file_write(TRA_ID, 0ll);
    trace_file_write(TRA_LEN, 0ll);
    trace_file_write(TRA_ITS, 0ll);
}

// ear_states.c
void traces_new_signature(int global_rank, int local_rank, signature_t *sig)
{
    double seconds,cpi,gbs,power,vpi,tpi;
    ullong lsec;
    ullong lcpi;
    ullong ltpi;
    ullong lgbs;
    ullong lpow;
    ullong lvpi;
    ullong lfreq;
    ullong lavgf, lpck, ldram, lio, lgflops;

    if (!enabled || !working) {
        return;
    }
    if (sig->time == 0) return;
    seconds = sig->time;
    cpi   = sig->CPI;
    tpi   = sig->TPI;
    gbs   = sig->GBS;
    power = sig->DC_power;
    vpi   = (double)(sig->FLOPS[3]/16+sig->FLOPS[7]/8)/(double)sig->instructions;
    lfreq = (ullong)sig->def_f;
    lavgf = (ullong)sig->avg_f;

    lsec = (ullong) (seconds * 1000000.0);
    lcpi = (ullong) (cpi * 1000.0);
    ltpi = (ullong) (tpi * 1000.0);
    lgbs = (ullong) (gbs * 1000.0);
    lpow = (ullong) (power * 1000);
    lvpi = (ullong) (vpi * 1000.0);
    lpck = (ullong) (sig->PCK_power * 1000);
    ldram = (ullong) (sig->DRAM_power * 1000);
    lio   = (ullong) (sig->IO_MBS * 1000);
    lgflops = (ullong) (sig->Gflops * 1000);


    trace_file_write(TRA_TIM, lsec);
    trace_file_write(TRA_CPI, lcpi);
    trace_file_write(TRA_TPI, ltpi);
    trace_file_write(TRA_GBS, lgbs);
    trace_file_write(TRA_POW, lpow);
    trace_file_write(TRA_VPI, lvpi);
    trace_file_write(TRA_FRQ, lfreq);
    trace_file_write(TRA_AVGFRQ, lavgf);
    trace_file_write(TRA_PCK_POWER, lpck);
    trace_file_write(TRA_DRAM_POWER, ldram);
    trace_file_write(TRA_IO, lio);
    trace_file_write(TRA_GFLOPS, lgflops);
}

// ear_states.c
void traces_PP(int global_rank, int local_rank, double seconds, double power)
{
    ullong lpsec;
    ullong lppow;

    if (!enabled || !working) {
        return;
    }

    lpsec = (ullong) (seconds * 1000000.0);
    lppow = (ullong) (power);

    trace_file_write(TRA_PTI, lpsec);
    trace_file_write(TRA_PPO, lppow);
}

// ear_api.c, ear_states.c
void traces_frequency(int global_rank, int local_rank, unsigned long f)
{
    if (!enabled || !working) {
        return;
    }

    trace_file_write(TRA_FRQ, f);
}


void traces_policy_state(int global_rank, int local_rank, int state)
{
    if (!enabled || !working) {
        return;
    }
    trace_file_write(TRA_STA,state);
}

void traces_dynais(int global_rank, int local_rank, int state)
{
    if (!enabled || !working) {
        return;
    }
}
void traces_earl_mode_dynais(int global_rank, int local_rank)
{
    if (!enabled || !working) {
        return;
    }
}
void traces_earl_mode_periodic(int global_rank, int local_rank)
{
    if (!enabled || !working) {
        return;
    }
}

void traces_reconfiguration(int global_rank, int local_rank)
{
    if (!enabled || !working) {
        return;
    }

    trace_file_write_simple_event(TRA_REC);
}

int traces_are_on()
{
    return enabled;
}

void traces_generic_event(int global_rank, int local_rank, int event, int value)
{
  trace_file_write(event, (ullong) value);
}

#endif
