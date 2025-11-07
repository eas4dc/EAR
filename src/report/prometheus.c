/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <microhttpd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/output/verbose.h>
#include <common/states.h>
#include <common/types/types.h>

#include <data_center_monitor/plugins/nodesensors.h>
#include <report/report.h>

#define NUM_LINES 30
#define MAX_TIME  30
#define LINE_SIZE 258

// Following the microhttpd example
static struct MHD_Daemon *d;

static enum MHD_Result ahc_echo(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
                                const char *version, const char *upload_data, size_t *upload_data_size, void **ptr)
{
    static int dummy;
    const char *page = cls;
    struct MHD_Response *response;
    int ret;

    if (0 != strcmp(method, "GET"))
        return MHD_NO; /* unexpected method */
    if (&dummy != *ptr) {
        /* The first time only the headers are valid,
           do not respond in the first round... */
        *ptr = &dummy;
        return MHD_YES;
    }
    if (0 != *upload_data_size)
        return MHD_NO; /* upload data in a GET!? */
    *ptr     = NULL;   /* clear context pointer */
    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_PERSISTENT);
    ret      = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

typedef struct text_data {
    char *text;
    time_t insert_timestamp;
    time_t metric_timestamp;
} text_data_t;

// custom type to store the current data
typedef struct text_buffer {
    text_data_t *data;
    uint32_t max_num_entries;

    // This is where the final text is stored
    char *final_text;
} text_buffer_t;

static text_buffer_t general_buffer;

static sem_t prom_sem;

// allocates the texts and timestamps inside the struct. The length of a certain line is defined by LINE_SIZE
static state_t buffer_alloc(text_buffer_t *buffer, uint32_t buff_size)
{

    int i;

    buffer->max_num_entries = buff_size;
    buffer->data            = calloc(buff_size, sizeof(text_data_t));
    for (i = 0; i < buff_size; i++)
        buffer->data[i].text = calloc(LINE_SIZE, sizeof(char));

    buffer->final_text = calloc(buff_size * LINE_SIZE, sizeof(char));

    return EAR_SUCCESS;
}

// frees all the memory from the struct
static state_t buffer_dispose(text_buffer_t *buffer)
{
    int i;
    for (i = 0; i < buffer->max_num_entries; i++)
        free(buffer->data[i].text);

    free(buffer->data);

    return EAR_SUCCESS;
}

// places a 0 in the old timestamps and removes their string from the buffer
static void buffer_clean(text_buffer_t *buffer)
{
    int i;
    time_t current_time = time(NULL);
    for (i = 0; i < buffer->max_num_entries; i++) {
        if (buffer->data[i].insert_timestamp < current_time - MAX_TIME) {
            strcpy(buffer->data[i].text, "");
            buffer->data[i].insert_timestamp = 0;
            buffer->data[i].metric_timestamp = 0;
        }
    }
}

// finds the first empty text inside the buffer and copies text_to_insert in that position
static state_t buffer_insert(text_buffer_t *buffer, char *text_to_insert, time_t *metric_time)
{
    int i;
    time_t aux_time;

    for (i = 0; i < buffer->max_num_entries; i++) {
        if (buffer->data[i].insert_timestamp == 0) {
            strcpy(buffer->data[i].text, text_to_insert);
            aux_time = time(NULL);
            buffer->data[i].insert_timestamp =
                aux_time; // the timestamp is when the data reaches the plugin, not when it was generated

            if (metric_time != NULL)
                buffer->data[i].metric_timestamp = *metric_time;
            else
                buffer->data[i].metric_timestamp = aux_time;
            return EAR_SUCCESS;
        }
    }
    debug("Current buffer is full, increasing size");
    // new alloc
    buffer->data = realloc(buffer->data, sizeof(text_data_t) * (buffer->max_num_entries + 1));
    memset(&buffer->data[buffer->max_num_entries], 0, sizeof(text_data_t));

    // normal procedure
    i = buffer->max_num_entries;
    strcpy(buffer->data[i].text, text_to_insert);
    aux_time = time(NULL);
    buffer->data[i].insert_timestamp =
        aux_time; // the timestamp is when the data reaches the plugin, not when it was generated

    if (metric_time != NULL)
        buffer->data[i].metric_timestamp = *metric_time;
    else
        buffer->data[i].metric_timestamp = aux_time;

    buffer->max_num_entries++;

    return EAR_ERROR;
}

int cmp_func(const void *a, const void *b)
{
    text_data_t *ta = (text_data_t *) a;
    text_data_t *tb = (text_data_t *) b;

    if (ta->insert_timestamp == 0 && ta->insert_timestamp == 0)
        return 0;
    if (ta->metric_timestamp > tb->metric_timestamp)
        return 1;
    if (ta->metric_timestamp < tb->metric_timestamp)
        return -1;
    if (ta->metric_timestamp == tb->metric_timestamp)
        return 1;
    return 0;
}

static void buffer_sort(text_buffer_t *buffer)
{
    qsort(buffer->data, buffer->max_num_entries, sizeof(text_data_t), cmp_func);
}

// naive implementation of the buffer rebuilding
void buffer_rebuild(text_buffer_t *buffer)
{
    int i;
    strcpy(buffer->final_text, "");

    for (i = 0; i < buffer->max_num_entries; i++)
        strcat(buffer->final_text, buffer->data[i].text);
}

state_t report_init(report_id_t *id, cluster_conf_t *cconf)
{

    sem_init(&prom_sem, 0, 1);
    sem_wait(&prom_sem);
    buffer_alloc(&general_buffer, 100);

    d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, 9011, NULL, NULL, &ahc_echo, general_buffer.final_text,
                         MHD_OPTION_END);

    if (d == NULL) {
        warning("Couldn't open server for Prometheus to scrape, returning");
        return EAR_ERROR;
    }
    verbose(0, "prometheus report_init");

    sem_post(&prom_sem);

    return EAR_SUCCESS;
}

state_t report_dispose(report_id_t *id)
{
    MHD_stop_daemon(d);

    buffer_dispose(&general_buffer);

    return EAR_SUCCESS;
}

state_t report_periodic_metrics(report_id_t *id, periodic_metric_t *mets, uint count)
{

    char job_text[64];
    char tmp_text[256];
    char node_id[64];
    int i;

    verbose(0, "prometheus report_metrics");
    sem_wait(&prom_sem);
    buffer_clean(&general_buffer);

    for (i = 0; i < count; i++) {
        strcpy(job_text, "");
        strncpy(node_id, mets[i].node_id, 64); // to prevent buffer overflows since the original node_id is 256 chars
        if (mets[i].job_id != 0) {
            sprintf(job_text, ", jobid=%lu, stepid=%lu", mets[i].job_id, mets[i].step_id);
        }
        sprintf(tmp_text, "periodic_metric_DC_power_Watts{node=\"%s%s\"} %lu %lu\n", node_id, job_text,
                mets[i].DC_energy / (mets[i].end_time - mets[i].start_time), mets[i].end_time * 1000);
        buffer_insert(&general_buffer, tmp_text, &mets[i].end_time);

        sprintf(tmp_text, "periodic_metric_avg_freq_kHz{node=\"%s%s\"} %lu %lu\n", node_id, job_text, mets[i].avg_f,
                mets[i].end_time * 1000);
        buffer_insert(&general_buffer, tmp_text, &mets[i].end_time);

        sprintf(tmp_text, "periodic_metric_temp_Celsius{node=\"%s%s\"} %lu %lu\n", node_id, job_text, mets[i].temp,
                mets[i].end_time * 1000);
        buffer_insert(&general_buffer, tmp_text, &mets[i].end_time);

#if USE_GPUS
        sprintf(tmp_text, "periodic_metric_GPU_power_Watts{node=\"%s%s\"} %lu %lu\n", node_id, job_text,
                mets[i].GPU_energy / (mets[i].end_time - mets[i].start_time), mets[i].end_time * 1000);
        buffer_insert(&general_buffer, tmp_text, &mets[i].end_time);
#endif

        sprintf(tmp_text, "periodic_metric_PCK_power_Watts{node=\"%s%s\"} %lu %lu\n", node_id, job_text,
                mets[i].PCK_energy / (mets[i].end_time - mets[i].start_time), mets[i].end_time * 1000);
        buffer_insert(&general_buffer, tmp_text, &mets[i].end_time);

        sprintf(tmp_text, "periodic_metric_DRAM_power_Watts{node=\"%s%s\"} %lu %lu\n", node_id, job_text,
                mets[i].DRAM_energy / (mets[i].end_time - mets[i].start_time), mets[i].end_time * 1000);
        buffer_insert(&general_buffer, tmp_text, &mets[i].end_time);
    }

    buffer_sort(&general_buffer);
    buffer_rebuild(&general_buffer);
    sem_post(&prom_sem);

    return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id, application_t *apps, uint count)
{
    verbose(0, "prometheus report_applications");
    return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id, loop_t *loops, uint count)
{
    int i;
    char loop_info[128];
    char tmp_text[256];

    verbose(0, "prometheus report_loops");
    sem_wait(&prom_sem);
    buffer_clean(&general_buffer);
    time_t insert_time = time(NULL);

    for (i = 0; i < count; i++) {
        snprintf(loop_info, 64, "jobid=%lu,stepid=%lu,node=\"%s\"", loops[i].jid, loops[i].step_id, loops[i].node_id);

        sprintf(tmp_text, "loops_DC_power_Watts{%s} %.2lf %lu \n", loop_info, loops[i].signature.DC_power, insert_time);
        buffer_insert(&general_buffer, tmp_text, &insert_time);

        sprintf(tmp_text, "loops_DRAM_power_Watts{%s} %.2lf %lu \n", loop_info, loops[i].signature.DRAM_power,
                insert_time);
        buffer_insert(&general_buffer, tmp_text, &insert_time);

        sprintf(tmp_text, "loops_PCK_power_Watts{%s} %.2lf %lu \n", loop_info, loops[i].signature.PCK_power,
                insert_time);
        buffer_insert(&general_buffer, tmp_text, &insert_time);

        sprintf(tmp_text, "loops_avg_freq_KHz{%s} %lu %lu \n", loop_info, loops[i].signature.avg_f, insert_time);
        buffer_insert(&general_buffer, tmp_text, &insert_time);

        sprintf(tmp_text, "loops_def_freq_KHz{%s} %lu %lu \n", loop_info, loops[i].signature.def_f, insert_time);
        buffer_insert(&general_buffer, tmp_text, &insert_time);
    }

    buffer_sort(&general_buffer);
    buffer_rebuild(&general_buffer);
    sem_post(&prom_sem);
    return EAR_SUCCESS;
}

state_t report_events(report_id_t *id, ear_event_t *events, uint count)
{
    int i;
    char ev_type[64];
    char tmp_text[256];
    char job_text[62];

    verbose(0, "prometheus report_events");
    sem_wait(&prom_sem);
    buffer_clean(&general_buffer);
    time_t insert_time = time(NULL);

    for (i = 0; i < count; i++) {
        if (events[i].jid != 0) {
            sprintf(job_text, ", jobid=%lu, stepid=%lu", events[i].jid, events[i].step_id);
        }
        event_type_to_str(&events[i], ev_type, 64);
        sprintf(tmp_text, "%s{%s%s} %lu %lu\n", ev_type, events[i].node_id, job_text, events[i].value,
                events[i].timestamp * 1000);
        buffer_insert(&general_buffer, tmp_text, &insert_time);
    }

    buffer_sort(&general_buffer);
    buffer_rebuild(&general_buffer);
    sem_post(&prom_sem);
    return EAR_SUCCESS;
}

state_t report_misc(report_id_t *id, uint type, const char *data_in, uint count)
{
    char tmp_text[1024];

    verbose(0, "prometheus report miscelaneous");
    sem_wait(&prom_sem);
    buffer_clean(&general_buffer);
    time_t insert_time = time(NULL);

    nodesensor_t *data = (nodesensor_t *) data_in;

    for (uint i = 0; i < count; i++) {
        switch (type) {
            case NODESENSORS_TYPE:
                snprintf(tmp_text, sizeof(tmp_text), "%s{%s} %lf %lu\n", pdu_type_to_str(data[i].type),
                         data[i].nodename, data[i].power, data[i].timestamp * 1000);
                buffer_insert(&general_buffer, tmp_text, &insert_time);

                break;
        }
    }
    buffer_sort(&general_buffer);
    buffer_rebuild(&general_buffer);
    sem_post(&prom_sem);

    return EAR_SUCCESS;
}
