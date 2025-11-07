/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/environment.h>
#include <common/types/event_type.h>
#include <stdio.h>

void event_type_to_str(ear_event_t *ev, char *str, size_t max)
{
    if ((ev == NULL) || (str == NULL))
        return;

    switch (ev->event) {
        case ENERGY_POLICY_NEW_FREQ:
            strncpy(str, ENERGY_POLICY_NEW_FREQ_TXT, max);
            break;
        case GLOBAL_ENERGY_POLICY:
            strncpy(str, GLOBAL_ENERGY_POLICY_TXT, max);
            break;
        case ENERGY_POLICY_FAILS:
            strncpy(str, ENERGY_POLICY_FAILS_TXT, max);
            break;
        case DYNAIS_OFF:
            strncpy(str, DYNAIS_OFF_TXT, max);
            break;
        case EARL_STATE:
            strncpy(str, EARL_STATE_TXT, max);
            break;
        case EARL_PHASE:
            strncpy(str, EARL_PHASE_TXT, max);
            break;
        case EARL_POLICY_PHASE:
            strncpy(str, EARL_POLICY_PHASE_TXT, max);
            break;
        case EARL_MPI_LOAD_BALANCE:
            strncpy(str, EARL_MPI_LOAD_BALANCE_TXT, max);
            break;
        case EARL_OPT_ACCURACY:
            strncpy(str, EARL_OPT_ACCURACY_TXT, max);
            break;
        case ENERGY_SAVING:
            strncpy(str, "ENERGY_SAVING", max);
            break;
        case POWER_SAVING:
            strncpy(str, "POWER_SAVING", max);
            break;
        case PERF_PENALTY:
            strncpy(str, "PERF_PENALTY", max);
            break;
        case ENERGY_SAVING_AVG:
            strncpy(str, "ENERGY_SAVING_AVG", max);
            break;
        case POWER_SAVING_AVG:
            strncpy(str, "POWER_SAVING_AVG", max);
            break;
        case PERF_PENALTY_AVG:
            strncpy(str, "PERF_PENALTY_AVG", max);
            break;
        case GPU_ENERGY_SAVING:
            strncpy(str, "GPU_ENERGY_SAVING", max);
            break;
        case GPU_POWER_SAVING:
            strncpy(str, "GPU_POWER_SAVING", max);
            break;
        case GPU_PERF_PENALTY:
            strncpy(str, "GPU_PERF_PENALTY", max);
            break;
        case GPU_ENERGY_SAVING_AVG:
            strncpy(str, "GPU_ENERGY_SAVING_AVG", max);
            break;
        case GPU_POWER_SAVING_AVG:
            strncpy(str, "GPU_POWER_SAVING_AVG", max);
            break;
        case GPU_PERF_PENALTY_AVG:
            strncpy(str, "GPU_PERF_PENALTY_AVG", max);
            break;
        case PHASE_SUMMARY_BASE + APP_COMP_BOUND:
            strncpy(str, "COMP_PHASE", max);
            break;
        case PHASE_SUMMARY_BASE + APP_MPI_BOUND:
            strncpy(str, "MPI_PHASE", max);
            break;
        case PHASE_SUMMARY_BASE + APP_IO_BOUND:
            strncpy(str, "IO_PHASE", max);
            break;
        case PHASE_SUMMARY_BASE + APP_BUSY_WAITING:
            strncpy(str, "CPU_BUSY_WAITING", max);
            break;
        case PHASE_SUMMARY_BASE + APP_CPU_GPU:
            strncpy(str, "CPU-GPU_PHASE", max);
            break;
        case PHASE_SUMMARY_BASE + APP_COMP_CPU:
            strncpy(str, "CPU_BOUND", max);
            break;
        case PHASE_SUMMARY_BASE + APP_COMP_MEM:
            strncpy(str, "MEM_BOUND", max);
            break;
        case PHASE_SUMMARY_BASE + APP_COMP_MIX:
            strncpy(str, "MIX_COMP", max);
            break;

        /* EARD events */
        case DC_POWER_ERROR:
            strncpy(str, "DC_POW_ERROR", max);
            break;
        case TEMP_ERROR:
            strncpy(str, "TEMP_ERROR", max);
            break;
        case FREQ_ERROR:
            strncpy(str, "FREQ_ERROR", max);
            break;
        case RAPL_ERROR:
            strncpy(str, "RAPL_ERROR", max);
            break;
        case GBS_ERROR:
            strncpy(str, "GBS_ERROR", max);
            break;
        case CPI_ERROR:
            strncpy(str, "CPI_ERROR", max);
            break;
        case RESET_POWERCAP:
            strncpy(str, "RESET_POWERCAP", max);
            break;
        case INC_POWERCAP:
            strncpy(str, "INC_POWERCAP", max);
            break;
        case RED_POWERCAP:
            strncpy(str, "RED_POWERCAP", max);
            break;
        case SET_POWERCAP:
            strncpy(str, "SET_POWERCAP", max);
            break;
        case SET_ASK_DEF:
            strncpy(str, "SET_ASK_DEF", max);
            break;
        case RELEASE_POWER:
            strncpy(str, "RELEASE_POWER", max);
            break;
        case POWERCAP_VALUE:
            strncpy(str, "POWERCAP_VALUE", max);
            break;
        case CLUSTER_POWER:
            strncpy(str, "CLUSTER_POWER", max);
            break;
        case NODE_POWERCAP:
            strncpy(str, "NODE_POWERCAP", max);
            break;
        case POWER_UNLIMITED:
            strncpy(str, "POWER_UNLIMITED", max);
            break;

        default:
            snprintf(str, max, "unknown(%u)", ev->event);
    }
}

void event_value_to_str(ear_event_t *ev, char *str, size_t max)
{
    if ((ev == NULL) || (str == NULL))
        return;

    if (ev->event == EARL_OPT_ACCURACY) {
        switch (ev->value) {
            case OPT_NOT_READY:
                strncpy(str, OPT_NOT_READY_TXT, max);
                break;
            case OPT_OK:
                strncpy(str, OPT_OK_TXT, max);
                break;
            case OPT_NOT_OK:
                strncpy(str, OPT_NOT_OK_TXT, max);
                break;
            case OPT_TRY_AGAIN:
                strncpy(str, OPT_TRY_AGAIN_TXT, max);
                break;
        }
    } else {
        snprintf(str, max, "%lu", ev->value);
    }
}

void event_serialize(serial_buffer_t *b, ear_event_t *event)
{
    serial_dictionary_push_auto(b, event->jid);
    serial_dictionary_push_auto(b, event->step_id);
    serial_dictionary_push_auto(b, event->node_id);
    serial_dictionary_push_auto(b, event->event);
    serial_dictionary_push_auto(b, event->value);
    serial_dictionary_push_auto(b, event->timestamp);
}

void event_deserialize(serial_buffer_t *b, ear_event_t *event)
{
    serial_dictionary_pop_auto(b, event->jid);
    serial_dictionary_pop_auto(b, event->step_id);
    serial_dictionary_pop_auto(b, event->node_id);
    serial_dictionary_pop_auto(b, event->event);
    serial_dictionary_pop_auto(b, event->value);
    serial_dictionary_pop_auto(b, event->timestamp);
}

void event_clean_before_db(ear_event_t *event)
{
    if (event->event >= INT_MAX)
        event->event = INT_MAX;
    if (event->value >= INT_MAX)
        event->value = INT_MAX;
    // Job id, step_id and timestamp are interpreted as LONGLONG so there _should not_ be a problem with it
}
