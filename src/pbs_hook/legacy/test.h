#ifndef _TEST_H_
#define _TEST_H_

#include "test.h"
#include <stdint.h>
#include <time.h>

#define GENERIC_NAME    256
#define ENERGY_TAG_SIZE 32
#define POLICY_NAME     32

typedef unsigned long job_id;

typedef struct job {
    job_id id;
    job_id step_id;
    char user_id[GENERIC_NAME];
    char group_id[GENERIC_NAME];
    char app_id[GENERIC_NAME];
    char user_acc[GENERIC_NAME];
    char energy_tag[ENERGY_TAG_SIZE];
    time_t start_time;
    time_t end_time;
    time_t start_mpi_time;
    time_t end_mpi_time;
    char policy[POLICY_NAME];
    double th;
    unsigned long procs;
    unsigned long type;
    unsigned long def_f;
} job_t;

typedef struct new_job_req {
    job_t job;
    uint8_t is_mpi;
    uint8_t is_learning;
} new_job_req_t;

int print_job_req(new_job_req_t *my_job_req);
int print_job(job_t job);
#endif
