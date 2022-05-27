#include <stdio.h>
#include "test.h"

int print_job(job_t job)
{
    return printf("--- JOB INFO---\nJOB ID %lu\nSTEP ID %lu\nUSER ID %s\nGROUP ID %s\n"
            "USER ACC %s\nENERGY TAG %s\nPOLICY %s\nTH %lf",
            job.id, job.step_id, job.user_id, job.group_id, job.user_acc, job.energy_tag, job.policy, job.th);
}

int print_job_req(new_job_req_t *my_job_req)
{
    int ret = printf("--- JOB REQ INFO ---\nis mpi %d. is learning %d\n",
            my_job_req->is_mpi, my_job_req->is_learning);
    if (ret < 0) return ret;
    return print_job(my_job_req->job);
}
