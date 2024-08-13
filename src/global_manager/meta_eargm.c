/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _XOPEN_SOURCE 700 //to get rid of the warning
#define _GNU_SOURCE 

// #define SHOW_DEBUGS 1

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <common/states.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <common/types/powercap.h>
#include <common/messaging/msg_internals.h>

#include <global_manager/eargm_rapi.h>
#include <global_manager/meta_eargm.h>

static ulong total_power, total_req, total_req_no_extra, total_req_ask_def, total_freeable, total_extra, total_powercap;
static uint num_ask_def, num_greedy, num_greedy_no_extra;

static ulong default_powercap = 0;
static ulong current_free_power = 0;
extern uint total_nodes;
extern uint my_port;
cluster_conf_t *conf = NULL;


int eargm_table_init(eargm_table_t *egm_table, cluster_conf_t *conf, int egm_idx)
{
    int i, j;

    egm_table->num_eargms    = conf->eargm.eargms[egm_idx].num_subs;
    egm_table->ids           = calloc(egm_table->num_eargms, sizeof(int));
    egm_table->eargm_ips     = calloc(egm_table->num_eargms, sizeof(int));
    egm_table->eargm_ports   = calloc(egm_table->num_eargms, sizeof(int));
    egm_table->actions       = calloc(egm_table->num_eargms, sizeof(char));
    egm_table->action_values = calloc(egm_table->num_eargms, sizeof(uint));

    for (i = 0; i < conf->eargm.eargms[egm_idx].num_subs; i++)
    {
        for (j = 0; j < conf->eargm.num_eargms; j++) {
            if (conf->eargm.eargms[j].id == conf->eargm.eargms[egm_idx].subs[i]) {
                egm_table->ids[i]   = conf->eargm.eargms[j].id;
                egm_table->eargm_ports[i] = conf->eargm.eargms[j].port;
                egm_table->eargm_ips[i] = get_ip(conf->eargm.eargms[j].node, conf);
                default_powercap += conf->eargm.eargms[i].power;
            }
        }
    }
    return EAR_SUCCESS;
}

void print_eargm_status(eargm_status_t *status, uint num_status)
{
    int i;
    for (i = 0; i < num_status; i++) {
        verbose(VGM, "%sid %d powercap %lu power %lu requested %lu freeable %lu extra %lu status %u%s", 
                COL_CLR,
                status[i].id, status[i].current_powercap, status[i].current_power, status[i].requested, 
                status[i].freeable_power, status[i].extra_power, status[i].status, COL_CLR);
    }

}
void print_eargm_actions(eargm_table_t *egm_table)
{
    int i;
    for (i = 0; i < egm_table->num_eargms; i++) {
        verbose(VGM, "%sEARGM %d has action %u and value %u%s", COL_GRE, egm_table->ids[i], egm_table->actions[i], 
                                    egm_table->action_values[i], COL_CLR);
    }
}

void meta_aggregate_data(eargm_status_t *status, uint num_status)
{
    int i;

    //reset values
    total_power = total_req = total_freeable = total_extra = total_req_no_extra = total_powercap = total_req_ask_def = 0;
    num_ask_def = num_greedy = num_greedy_no_extra = 0;

    //aggregate data
    for (i = 0; i < num_status; i++)
    {
        total_power += status[i].current_power;
        total_powercap += status[i].current_powercap;
        //total_requested += status[i].requested;
        total_freeable += status[i].freeable_power;
        total_extra += status[i].extra_power;

        if (status[i].status == PC_STATUS_ASK_DEF) {
            num_ask_def ++;
            total_req_ask_def += status[i].requested;
            //total_req_ask_def += status[i].def_power - status[i].current_powercap;
        } else if (status[i].status == PC_STATUS_GREEDY) {
            if (!status[i].extra_power) {
                num_greedy_no_extra++;
                total_req_no_extra += status[i].requested;
            } else {
                num_greedy++;
                total_req += status[i].requested;
            }
        }
    }
    if (default_powercap < total_powercap) {
        verbose(VGM, "ERROR: TOTAL POWERCAP > DEFAULT POWERCAP FOR META_EARGM (default %lu current %lu)", 
                default_powercap, total_powercap);

        current_free_power = 0;
    } else {
        current_free_power = default_powercap - total_powercap;
        verbose(VGM, "powercap left to allocate %lu (default %lu current %lu)", current_free_power, default_powercap, total_powercap);
    }

}

int get_eargm_position(eargm_table_t *egm_table, uint id)
{
    int i;
    for (i = 0; i < egm_table->num_eargms; i++)
        if (egm_table->ids[i] == id) return i;

    return -1;
}

void assign_ask_def(eargm_status_t *status, uint num_status, eargm_table_t *egm_table)
{
    int i, egm_id;
    for (i = 0; i < num_status; i++)
    {
        if (status[i].status == PC_STATUS_ASK_DEF) {
            egm_id = get_eargm_position(egm_table, status[i].id);
            if (egm_id >= 0) {
                //egm_table->actions[egm_id] = EARGM_RESET_PC;
                egm_table->actions[egm_id] = EARGM_INC_PC;
                egm_table->action_values[egm_id] += status[i].requested;
            }
        }
    }
}

void reduce_freeable_power(eargm_status_t *status, uint num_status, eargm_table_t *egm_table, ulong *pending, ulong *freeable)
{
    int i = 0, egm_id;
    ulong tmp_var = 0;
    while (*pending && i < num_status)
    {
        if (status[i].freeable_power) {
            egm_id = get_eargm_position(egm_table, status[i].id);
            if (egm_id >= 0) {
                tmp_var = ear_min(*pending, status[i].freeable_power); //store it in an auxiliary value since we will need to remove it from both variables
                
                egm_table->actions[egm_id] = EARGM_RED_PC;
                egm_table->action_values[egm_id] += tmp_var; //assign the power to the node
                
                *freeable -= tmp_var; //remove it from the free power pool
                *pending  -= tmp_var; //remove it from the pending power pool
                status[i].freeable_power -= tmp_var; //remove it from the individual node freeable power pool
            }
        }
        i++;
    }
}

void reduce_extra_power(eargm_status_t *status, uint num_status, eargm_table_t *egm_table, ulong *pending)
{
    int i = 0, egm_id;
    ulong tmp_var = 0;

    while (*pending && i < num_status)
    {
        if (status[i].extra_power) {
            egm_id = get_eargm_position(egm_table, status[i].id);
            if (egm_id >= 0) {
                tmp_var = ear_min(*pending, status[i].extra_power); //store it in an auxiliary value since we will need to remove it from both variables

                egm_table->actions[egm_id] = EARGM_RED_PC;
                egm_table->action_values[egm_id] += tmp_var; //increase the reduction value for the node

                *pending  -= tmp_var; //remove it from the pending power pool
                status[i].extra_power -= tmp_var; //remove it from the individual node extra power
            }
        }
        i++;
    }
}

void assign_extra_power(eargm_status_t *status, uint num_status, eargm_table_t *egm_table, ulong *free_power, ulong *pending, uint has_extra)
{
    int i = 0, egm_id;
    ulong tmp_var = 0;

    for (i = 0; (i < num_status) && *free_power; i++)
    {
        if (status[i].status == PC_STATUS_GREEDY) {
            // if status[i].extra_power > 0 && has_extra == 1 -> enter
            // if status[i].extra_power > 0 && has_extra == 0 -> DO NOT enter
            // if status[i].extra_power == 0 && has_extra == 1 -> DO NOT enter
            // if status[i].extra_power == 0 && has_extra == 0 -> enter
            if ((status[i].extra_power > 0) && (has_extra == 0)) continue;
            if ((status[i].extra_power == 0) && (has_extra == 1)) continue;

            egm_id = get_eargm_position(egm_table, status[i].id);
            if (egm_id >= 0) {
                tmp_var = ear_min(*free_power, status[i].requested); //store it in an auxiliary value since we will need to remove it from both variables

                egm_table->actions[egm_id] = EARGM_INC_PC;
                egm_table->action_values[egm_id] += tmp_var; //increase the main value

                *free_power -= tmp_var; //remove it from the pool of free power
                *pending    -= tmp_var; //remove it from the pool of pending to assign power
                status[i].requested -= tmp_var; //remove it from the individual node requested power (since it has already been granted)
            }
        }
    }
}

void meta_reallocate_power(eargm_status_t *status, uint num_status, eargm_table_t *egm_table)
{
    meta_aggregate_data(status, num_status);
    ulong pending, pending_2;
    // first PC_STATUS_ASK_DEF
    if (num_ask_def > 0) {
        assign_ask_def(status, num_status, egm_table);
        if (current_free_power >= total_req_ask_def) { //if there is power not assigned to any EARGM and is enough to cover PC_STATUS_ASK_DEF
            current_free_power -= total_req_ask_def;
        } else {
            pending = total_req_ask_def - current_free_power; //first assign power not used by any EARGM
            if (total_freeable) { // assign free power already given to other EARGMS
                reduce_freeable_power(status, num_status, egm_table, &pending, &total_freeable);
            }
            if (pending) { // if still needed, remove extra power given to other EARGMS
                reduce_extra_power(status, num_status, egm_table, &pending);
                if (pending>0) { // this should never happen
                    verbose(VGM, "ERROR: NOT ENOUGH POWER TO ASSIGN PC_STATUS_ASK_DEF TO EARGMS (pending %lu)", pending); 
                }
                return;
            }
        }
    }

    // second PC_STATUS_GREEDY that have 0 extra power
    if (num_greedy_no_extra) {
        if (current_free_power) {
            assign_extra_power(status, num_status, egm_table, &current_free_power, &total_req_no_extra, 0); 
        }
        if (total_req_no_extra > 0 && total_freeable) {
            pending = pending_2 = ear_min(total_freeable, total_req_no_extra); //the function modifies pending, that's why we have pending_2
            reduce_freeable_power(status, num_status, egm_table, &pending, &total_freeable);
            assign_extra_power(status, num_status, egm_table, &pending_2, &total_req_no_extra, 0); 
            if (total_freeable == 0) return;
        }
    }

    // last PC_STATUS_GREEDY that have extra power
    if (num_greedy) {
        if (current_free_power) {
            assign_extra_power(status, num_status, egm_table, &current_free_power, &total_req, 1); 
        }
        if (total_req > 0 && total_freeable) {
            pending = pending_2 = ear_min(total_freeable, total_req); //the function modifies pending, that's why we have pending_2
            reduce_freeable_power(status, num_status, egm_table, &pending, &total_freeable);
            assign_extra_power(status, num_status, egm_table, &pending_2, &total_req, 1); 
        }
    }

}

void *meta_eargm(void *config)
{
    int i, num_status, eargm_idx = -1;
    eargm_def_t *e_def;
    eargm_status_t *status;
    eargm_table_t egm_table;
    char host[GENERIC_NAME];

    conf = (cluster_conf_t *) config;

    gethostname(host, sizeof(host));
    strtok(host, ".");
    e_def = get_eargm_conf(conf, host);
    if (e_def == NULL) {
        error("Could not get configuration for current node's meta_EARGM, exiting thread");
        pthread_exit(0);
    }

    if (pthread_setname_np(pthread_self(), "meta_eargm")) verbose(0, "Setting name for meta_eargm thread %s", strerror(errno));
    debug("created meta_eargm thread");

    for (i = 0; i < conf->eargm.num_eargms; i++) {
        if (conf->eargm.eargms[i].id == e_def->id) {
            eargm_idx = i;
            break;
        }
    }

    if (eargm_idx < 0) {
        error("EARGM is not defined in the configuration file (ear.conf), closing meta_eargm");
        pthread_exit(0);
    }

    if (eargm_table_init(&egm_table, conf, eargm_idx) != EAR_SUCCESS)
    {
        error("Error initializing meta_eargm node tables, closing meta_eargm");
        pthread_exit(0);
    }

    while(1)
    {
        debug("waiting period of time before asking nodes");
        sleep(conf->eargm.t1_power*2);

        num_status = eargm_get_all_status(conf, &status, eargm_idx);
        debug("Received %d status in meta thread", num_status);
        if (num_status > 0) 
        {
            print_eargm_status(status, num_status);

            //reset actions
            for (i = 0; i < egm_table.num_eargms; i++) {
                egm_table.actions[i] = NO_COMMAND;
                egm_table.action_values[i] = 0;
            }

            meta_reallocate_power(status, num_status, &egm_table);
            print_eargm_actions(&egm_table);

            eargm_send_table_commands(&egm_table);

            free(status);
        }
    }


    verbose(VGM, "exiting meta_eargm thread");
    pthread_exit(0);
}
