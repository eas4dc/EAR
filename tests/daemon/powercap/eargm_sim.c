/*
 * eargm_sim.c -- see eargm_sim.h
 */
#include <stdlib.h>
#include <string.h>

#include <common/config.h>

#include "eargm_sim.h"

int eargm_sim_node_ip = 0x0a000001;

/* ---- verbatim copy of dynamic_configuration.c:process_pmgt_status() ---- */
static void process_pmgt_status(powercap_status_t *my_status, pmgt_status_t *p_status)
{
    int my_ip = eargm_sim_node_ip;
    switch (p_status->status) {
        case PC_STATUS_OK:
            if (p_status->extra <= 0)
                break;
            p_status->requested = 0;
        case PC_STATUS_GREEDY:
            /* Memory management */
            if (my_status->num_greedy < 1) {
                my_status->greedy_nodes = NULL;
                my_status->greedy_data  = NULL;
            }
            my_status->num_greedy++;
            if (my_status->num_greedy < 1)
                break;
            my_status->greedy_nodes = realloc(my_status->greedy_nodes, sizeof(int) * my_status->num_greedy);
            my_status->greedy_data  = realloc(my_status->greedy_data, sizeof(greedy_bytes_t) * my_status->num_greedy);
            /* Data management */
            my_status->greedy_nodes[my_status->num_greedy - 1]            = my_ip;
            my_status->greedy_data[my_status->num_greedy - 1].requested   = p_status->requested;
            my_status->greedy_data[my_status->num_greedy - 1].extra_power = p_status->extra;
            my_status->greedy_data[my_status->num_greedy - 1].stress      = p_status->stress;
            break;
        default:
            break;
    }
}

/* ---- verbatim copy of dynamic_configuration.c:dyncon_filter_opt() ------ */
static int dyncon_filter_opt(powercap_opt_t *opt)
{
    int my_ip = eargm_sim_node_ip;
    for (int i = 0; i < opt->num_greedy; i++) {
        if (opt->greedy_nodes[i] == my_ip)
            return i;
    }
    return -1;
}

void eargm_get_status(eargm_reply_t *r, int release_power)
{
    memset(r, 0, sizeof(*r));
    powercap_get_status(&r->st, &r->pmgt, release_power);
    process_pmgt_status(&r->st, &r->pmgt);
}

void eargm_reply_free(eargm_reply_t *r)
{
    free(r->st.greedy_nodes);
    free(r->st.greedy_data);
    r->st.greedy_nodes = NULL;
    r->st.greedy_data  = NULL;
}

int eargm_send_opt(uint32_t num_greedy, const int32_t *greedy_ips, const int32_t *extra, uint8_t cluster_perc)
{
    powercap_opt_t opt;
    memset(&opt, 0, sizeof(opt));
    opt.num_greedy         = num_greedy;
    opt.cluster_perc_power = cluster_perc;
    /* The remote API hands EARD heap copies it frees after the call */
    opt.greedy_nodes = calloc(num_greedy ? num_greedy : 1, sizeof(int32_t));
    opt.extra_power  = calloc(num_greedy ? num_greedy : 1, sizeof(int32_t));
    for (uint32_t i = 0; i < num_greedy; i++) {
        opt.greedy_nodes[i] = greedy_ips[i];
        opt.extra_power[i]  = extra[i];
    }
    int id = dyncon_filter_opt(&opt);
    powercap_set_opt(&opt, id);
    free(opt.greedy_nodes);
    free(opt.extra_power);
    return id;
}

uint32_t eargm_release_idle(void)
{
    pc_release_data_t rel = {0};
    powercap_release_idle_power(&rel);
    return rel.released;
}

void eargm_set_powerlimit(uint32_t limit)
{
    powercap_set_powercap(limit);
}

void eargm_reduce_def(uint32_t watts)
{
    powercap_reduce_def_power(watts);
}

void eargm_increase_def(uint32_t watts)
{
    powercap_increase_def_power(watts);
}

void eargm_reset_def(void)
{
    powercap_reset_default_power();
}
