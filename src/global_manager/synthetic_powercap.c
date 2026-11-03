/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/powercap.h>
#include <global_manager/synthetic_powercap.h>

typedef enum synthetic_powercap_mode {
    SYNTHETIC_POWERCAP_DISABLED = 0,
    SYNTHETIC_POWERCAP_CONSTANT,
    SYNTHETIC_POWERCAP_TRACE,
} synthetic_powercap_mode_t;

typedef struct synthetic_trace_row {
    ulong iteration;
    uint num_active_nodes;
    ulong requested_power_per_node;
    size_t sequence;
} synthetic_trace_row_t;

static synthetic_powercap_mode_t synthetic_mode    = SYNTHETIC_POWERCAP_DISABLED;
static ulong synthetic_avg_node_power              = 0;
static synthetic_trace_row_t *synthetic_trace_rows = NULL;
static size_t synthetic_trace_row_count            = 0;
static size_t synthetic_trace_row_index            = 0;
static ulong synthetic_next_iteration              = 0;
static bool synthetic_trace_end_warned             = false;

static void synthetic_powercap_clear_trace(void)
{
    free(synthetic_trace_rows);
    synthetic_trace_rows       = NULL;
    synthetic_trace_row_count  = 0;
    synthetic_trace_row_index  = 0;
    synthetic_next_iteration   = 0;
    synthetic_trace_end_warned = false;
}

static void synthetic_powercap_reset(void)
{
    synthetic_powercap_clear_trace();
    synthetic_mode           = SYNTHETIC_POWERCAP_DISABLED;
    synthetic_avg_node_power = 0;
}

static int synthetic_trace_row_cmp(const void *a, const void *b)
{
    const synthetic_trace_row_t *ra = (const synthetic_trace_row_t *) a;
    const synthetic_trace_row_t *rb = (const synthetic_trace_row_t *) b;

    if (ra->iteration < rb->iteration)
        return -1;
    if (ra->iteration > rb->iteration)
        return 1;
    if (ra->sequence < rb->sequence)
        return -1;
    if (ra->sequence > rb->sequence)
        return 1;
    return 0;
}

static state_t synthetic_trace_parse_row(char *line, size_t line_no, synthetic_trace_row_t *row)
{
    char *save_ptr = NULL;
    char *token;
    char *end_ptr;
    const char *delims = ",:\t\r\n ";

    token = strtok_r(line, delims, &save_ptr);
    if (token == NULL)
        return EAR_WARNING;
    if (token[0] == '#')
        return EAR_WARNING;
    if (token[0] == '-')
        return_print(EAR_ERROR, "Synthetic trace line %zu has a negative iteration value", line_no);

    errno          = 0;
    end_ptr        = NULL;
    row->iteration = strtoul(token, &end_ptr, 10);
    if (errno != 0 || end_ptr == token || *end_ptr != '\0') {
        if (line_no == 1)
            return EAR_WARNING;
        return_print(EAR_ERROR, "Synthetic trace line %zu has an invalid iteration value", line_no);
    }

    token = strtok_r(NULL, delims, &save_ptr);
    if (token == NULL)
        return_print(EAR_ERROR, "Synthetic trace line %zu is missing num_active_nodes", line_no);
    if (token[0] == '-')
        return_print(EAR_ERROR, "Synthetic trace line %zu has a negative num_active_nodes value", line_no);

    errno                      = 0;
    end_ptr                    = NULL;
    unsigned long active_nodes = strtoul(token, &end_ptr, 10);
    if (errno != 0 || end_ptr == token || *end_ptr != '\0')
        return_print(EAR_ERROR, "Synthetic trace line %zu has an invalid num_active_nodes value", line_no);
    if (active_nodes > UINT_MAX)
        return_print(EAR_ERROR, "Synthetic trace line %zu has too many active nodes", line_no);

    token = strtok_r(NULL, delims, &save_ptr);
    if (token == NULL)
        return_print(EAR_ERROR, "Synthetic trace line %zu is missing requested_power_per_node", line_no);
    if (token[0] == '-')
        return_print(EAR_ERROR, "Synthetic trace line %zu has a negative requested_power_per_node value", line_no);

    errno                         = 0;
    end_ptr                       = NULL;
    row->requested_power_per_node = strtoul(token, &end_ptr, 10);
    if (errno != 0 || end_ptr == token || *end_ptr != '\0')
        return_print(EAR_ERROR, "Synthetic trace line %zu has an invalid requested_power_per_node value", line_no);

    row->num_active_nodes = (uint) active_nodes;
    return EAR_SUCCESS;
}

static state_t synthetic_trace_load(const char *csv_path)
{
    FILE *fp;
    char *line      = NULL;
    size_t line_cap = 0;
    ssize_t line_len;
    size_t line_no              = 0;
    size_t count                = 0;
    size_t capacity             = 0;
    size_t sequence             = 0;
    state_t status              = EAR_SUCCESS;
    synthetic_trace_row_t *rows = NULL;

    if (csv_path == NULL || csv_path[0] == '\0')
        return_print(EAR_ERROR, "Synthetic trace path is empty");

    fp = fopen(csv_path, "r");
    if (fp == NULL)
        return_print(EAR_ERROR, "Could not open synthetic trace %s: %s", csv_path, strerror(errno));

    synthetic_powercap_reset();

    while ((line_len = getline(&line, &line_cap, fp)) != -1) {
        synthetic_trace_row_t row = {0};

        line_no++;
        status = synthetic_trace_parse_row(line, line_no, &row);
        if (status == EAR_WARNING)
            continue;
        if (state_fail(status))
            goto fail;

        row.sequence = sequence++;
        if (count == capacity) {
            size_t new_capacity             = (capacity == 0) ? 32 : capacity * 2;
            synthetic_trace_row_t *new_rows = realloc(rows, new_capacity * sizeof(*new_rows));
            if (new_rows == NULL) {
                status = EAR_ERROR;
                goto fail;
            }
            rows     = new_rows;
            capacity = new_capacity;
        }
        rows[count++] = row;
    }

    if (count == 0) {
        status = EAR_ERROR;
        goto fail;
    }

    qsort(rows, count, sizeof(*rows), synthetic_trace_row_cmp);

    synthetic_trace_rows       = rows;
    synthetic_trace_row_count  = count;
    synthetic_trace_row_index  = 0;
    synthetic_next_iteration   = 0;
    synthetic_trace_end_warned = false;
    synthetic_mode             = SYNTHETIC_POWERCAP_TRACE;
    verbose(VGM_PC, "Synthetic EARGM trace enabled from %s (%zu rows, iterations %lu..%lu)", csv_path, count,
            synthetic_trace_rows[0].iteration, synthetic_trace_rows[count - 1].iteration);

    free(line);
    fclose(fp);
    return EAR_SUCCESS;

fail:
    free(rows);
    free(line);
    fclose(fp);
    synthetic_powercap_reset();
    if (state_ok(status))
        return_print(EAR_ERROR, "Synthetic trace %s does not contain any usable rows", csv_path);
    return status;
}

static void synthetic_powercap_fill_sample(ulong current_powercap, ulong default_powercap, uint max_nodes,
                                           uint active_nodes, ulong requested_power_per_node, ulong iteration,
                                           synthetic_powercap_sample_t *sample)
{
    ulong demand_total  = (ulong) active_nodes * requested_power_per_node;
    ulong current_power = demand_total;
    ulong requested     = 0;
    ulong freeable      = 0;
    ulong extra_power   = 0;

    sample->iteration                = iteration;
    sample->total_nodes              = max_nodes;
    sample->idle_nodes               = (max_nodes > active_nodes) ? max_nodes - active_nodes : 0;
    sample->requested_power_per_node = requested_power_per_node;
    sample->demand_total             = demand_total;

    if (active_nodes > max_nodes) {
        verbose(VGM_PC,
                "Synthetic row iteration %lu requests %u running EARDs but only %u nodes are configured; "
                "clamping to the configured node count",
                iteration, active_nodes, max_nodes);
        active_nodes         = max_nodes;
        demand_total         = (ulong) active_nodes * requested_power_per_node;
        sample->demand_total = demand_total;
        sample->total_nodes  = max_nodes;
        sample->idle_nodes   = 0;
    }

    if (current_powercap == POWER_CAP_UNLIMITED) {
        sample->current_power  = demand_total;
        sample->requested      = 0;
        sample->freeable_power = 0;
        sample->extra_power    = 0;
        return;
    }

    if (demand_total > current_powercap) {
        requested = demand_total - current_powercap;
        if (current_powercap < default_powercap) {
            ulong default_gap = default_powercap - current_powercap;
            if (requested > default_gap)
                requested = default_gap;
        }
        current_power = current_powercap;
    } else {
        current_power = demand_total;
        freeable      = (current_powercap > current_power) ? current_powercap - current_power : 0;
    }

    extra_power = (current_powercap > default_powercap) ? current_powercap - default_powercap : 0;

    sample->current_power  = current_power;
    sample->requested      = requested;
    sample->freeable_power = (requested > 0) ? 0 : freeable;
    sample->extra_power    = extra_power;
}

void cluster_powercap_enable_synthetic_mode(ulong avg_node_power)
{
    synthetic_powercap_reset();
    synthetic_mode           = SYNTHETIC_POWERCAP_CONSTANT;
    synthetic_avg_node_power = avg_node_power;
    verbose(VGM_PC, "Synthetic EARGM mode enabled with average node power %lu W", synthetic_avg_node_power);
}

state_t cluster_powercap_enable_synthetic_trace(const char *csv_path)
{
    return synthetic_trace_load(csv_path);
}

bool cluster_powercap_is_synthetic_mode(void)
{
    return synthetic_mode != SYNTHETIC_POWERCAP_DISABLED;
}

state_t synthetic_powercap_sample(ulong current_powercap, ulong default_powercap, uint max_nodes,
                                  synthetic_powercap_sample_t *sample)
{
    synthetic_trace_row_t row = {0};

    if (sample == NULL)
        return EAR_ERROR;
    if (synthetic_mode == SYNTHETIC_POWERCAP_DISABLED)
        return EAR_ERROR;

    memset(sample, 0, sizeof(*sample));

    if (synthetic_mode == SYNTHETIC_POWERCAP_CONSTANT) {
        ulong active_nodes = max_nodes;

        sample->iteration = synthetic_next_iteration++;
        synthetic_powercap_fill_sample(current_powercap, default_powercap, max_nodes, (uint) active_nodes,
                                       synthetic_avg_node_power, sample->iteration, sample);
        return EAR_SUCCESS;
    }

    if (synthetic_trace_row_count == 0 || synthetic_trace_rows == NULL)
        return EAR_ERROR;

    while (synthetic_trace_row_index + 1 < synthetic_trace_row_count &&
           synthetic_trace_rows[synthetic_trace_row_index + 1].iteration <= synthetic_next_iteration) {
        synthetic_trace_row_index++;
    }

    row               = synthetic_trace_rows[synthetic_trace_row_index];
    sample->iteration = synthetic_next_iteration++;
    synthetic_powercap_fill_sample(current_powercap, default_powercap, max_nodes, row.num_active_nodes,
                                   row.requested_power_per_node, sample->iteration, sample);

    if (synthetic_trace_row_index + 1 >= synthetic_trace_row_count && !synthetic_trace_end_warned &&
        sample->iteration > row.iteration) {
        synthetic_trace_end_warned = true;
        verbose(VGM_PC, "Synthetic trace reached the last row at iteration %lu, holding the final values",
                row.iteration);
    }

    return EAR_SUCCESS;
}
