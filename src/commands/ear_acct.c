/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define _XOPEN_SOURCE 700 // to get rid of the warning

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <common/colors.h>
#include <common/config.h>
#include <common/config/config_sched.h>
#include <common/output/verbose.h>
#include <common/system/user.h>
#include <common/types/application.h>
#include <common/types/classification_limits.h>
#include <common/types/event_type.h>
#include <common/types/log.h>
#include <common/types/loop.h>
#include <common/types/version.h>

#include <commands/ear_acct_auxiliary.h>
#include <commands/query_helpers.h>

#if USE_DB
#include <common/database/db_helper.h>
#include <common/database/mysql_io_functions.h>
#include <common/database/postgresql_io_functions.h>
#include <common/states.h>
#include <common/types/configuration/cluster_conf.h>
cluster_conf_t my_conf;
#endif

#define EACCT_USE_GPUS           USE_GPUS

#define COLORS                   0

#define STANDARD_NODENAME_LENGTH 25
#define APP_TEXT_FILE_FIELDS     22

#define PUE                      1.2
#define CARBON_INTENSITY         174

#define DEFAULT_QUERY_LIMIT      5

int32_t full_length      = 0;
int32_t verbose          = 0;
int32_t all_pow_sig      = 0;
int32_t avx              = 0;
int32_t print_gpus       = 1;
int32_t loop_extended    = 0;
bool display_user_column = false;
char csv_path[256]       = "";

#if COLORS
static void print_colors_legend()
{
    printf("%s CPU busy waiting %s memory bound %s cpu bound %s IO bound%s\n", COL_CYA, COL_GRE, COL_RED, COL_MGT,
           COL_CLR);
}

static void select_color_by_phase(double CPI, double GBS, double IO, double GFLOPS)
{
    char *col = NULL;
    printf("%s", COL_CLR);
    fflush(stdout);
    /* Memory bound */
    if ((CPI > CPI_MEM_BOUND_DEF) && (GBS > GBS_MEM_BOUND_DEF))
        col = COL_GRE;
    /* CPU bound */
    if (!col && (CPI < CPI_CPU_BOUND_DEF) && (GBS < GBS_CPU_BOUND_DEF) && (GFLOPS > GFLOPS_BUSY_WAITING_DEF))
        col = COL_RED;
    /* Busy waiting */
    if (!col && (CPI < CPI_BUSY_WAITING_DEF) && (GBS < GBS_BUSY_WAITING_DEF) && (GFLOPS < GFLOPS_BUSY_WAITING_DEF))
        col = COL_CYA;
    /* IO */
    if (!col && (IO > IO_TH_DEF))
        col = COL_MGT;
    if (col)
        printf("%s", col);
    fflush(stdout);
}
#else
#define print_colors_legend()
#define select_color_by_phase(A, B, C, D)
#endif

#if 0
    "\t\t-b\tverbose mode for debugging purposes\n" \
    "\t\t-o\tmodifies the -r option to also show the corresponding jobs. Should be used with -j.\n", app);
    printf("\t\t-f\tspecifies the file where the user-database can be found. If this option is used, the information will be read from the file and not the database.\n");
#endif
void usage(char *app)
{
#if USE_DB
    printf("Usage: %s [Optional parameters]\n"
           "\tOptional parameters: \n"
           "\t\t-h\tdisplays this message\n"
           "\t\t-v\tdisplays current EAR version\n"
           "\t\t-j\tspecifies the job id and step id to retrieve with the format [jobid.stepid] or the format "
           "[jobid1,jobid2,...,jobid_n].\n"
           "\t\t-l\tshows the information for each node for each job instead of the global statistics for said job.\n"
           "\t\t-r\tshows the EAR loop signatures. Users, job ids, and step ids can be specified as if were showing "
           "job information.\n"
           "\t\t-F\tspecifies the format string to be used. See manpage for more information.\n"
           "\t\t-a\tspecifies the application names that will be retrieved. [default: all app_ids]\n"
           "\t\t-c\tspecifies the file where the output will be stored in CSV format. If the argument is \"no_file\" "
           "the output will be printed to STDOUT [default: off]\n"
           "\t\t-t\tspecifies the energy_tag of the jobs that will be retrieved. [default: all tags].\n"
           "\t\t-s\tspecifies the minimum start time of the jobs that will be retrieved in YYYY-MM-DD. [default: no "
           "filter].\n"
           "\t\t-e\tspecifies the maximum end time of the jobs that will be retrieved in YYYY-MM-DD. [default: no "
           "filter].\n"
           "\t\t-x\tshows the last EAR events. Users, start and end times, job ids, and step ids can be specified as "
           "if were showing job information.\n"
           "\t\t-m\tprints EARD signatures regardless of whether EARL signatures are available or not.\n"
           "\t\t-u\tspecifies the user whose applications will be retrieved. Only available to privileged users. "
           "[default: all users]\n"
           "\t\t\t\tA user can only retrieve its own jobs unless said user is privileged. [default: all jobs]\n",
           app);
    printf("\t\t-n\tspecifies the number of jobs to be shown, starting from the most recent one. [default: %d][to get "
           "all jobs use -n all]\n",
           DEFAULT_QUERY_LIMIT);
#endif
    exit(0);
}

void print_values(int fd, char ***values, int columns, int rows)
{
    int i, j;
    for (i = 0; i < rows; i++) {
        dprintf(fd, "%s", values[i][0]);
        for (j = 1; j < columns; j++) {
            dprintf(fd, ";%s", values[i][j]);
        }
        dprintf(fd, "\n");
    }
}

struct avg_apps {
    application_t app;
    int num_nodes;
};

/* Averages the fields in signatures that make sense */
void average_signatures(signature_t *dst, signature_t *src, int nums)
{
    int i;
    dst->time      = src->time / nums;
    dst->EDP       = src->EDP / nums;
    dst->TPI       = src->TPI / nums;
    dst->CPI       = src->CPI / nums;
    dst->avg_f     = src->avg_f / nums;
    dst->avg_imc_f = src->avg_imc_f / nums;
    dst->def_f     = src->def_f / nums;
    dst->perc_MPI  = src->perc_MPI / nums;
#if USE_GPUS
    dst->gpu_sig.num_gpus = src->gpu_sig.num_gpus;
    for (i = 0; i < MAX_GPUS_SUPPORTED; i++) {
        dst->gpu_sig.gpu_data[i].GPU_freq     = src->gpu_sig.gpu_data[i].GPU_freq / nums;
        dst->gpu_sig.gpu_data[i].GPU_mem_freq = src->gpu_sig.gpu_data[i].GPU_mem_freq / nums;
        dst->gpu_sig.gpu_data[i].GPU_util     = src->gpu_sig.gpu_data[i].GPU_util / nums;
        dst->gpu_sig.gpu_data[i].GPU_mem_util = src->gpu_sig.gpu_data[i].GPU_mem_util / nums;
#if WF_SUPPORT
        dst->gpu_sig.gpu_data[i].GPU_temp     = src->gpu_sig.gpu_data[i].GPU_temp / nums;
        dst->gpu_sig.gpu_data[i].GPU_temp_mem = src->gpu_sig.gpu_data[i].GPU_temp_mem / nums;
#endif
    }
#endif
}

void average_power_signatures(power_signature_t *dest, int32_t num_sigs)
{
    dest->time /= num_sigs;
    dest->EDP /= num_sigs;
    dest->avg_f /= num_sigs;
    dest->def_f /= num_sigs;
}

void acum_power_sig_metrics(power_signature_t *dest, power_signature_t *src)
{
    dest->DC_power += src->DC_power;
    dest->DRAM_power += src->DRAM_power;
    dest->PCK_power += src->PCK_power;
    dest->max_DC_power += src->max_DC_power;
    dest->min_DC_power += src->min_DC_power;
    dest->EDP += src->EDP;
    dest->time += src->time;
    dest->avg_f += src->avg_f;
    dest->def_f += src->def_f;
}

void average_applications(application_t *apps, int num_apps, struct avg_apps **sorted_apps, int *num_sorted_apps)
{
    if (num_apps < 1 || apps == NULL)
        return; // for safety

    int i, current = 0;
    struct avg_apps *avg_apps = calloc(num_apps, sizeof(struct avg_apps));

    // copy the frist one manually to simplify the for loop
    memcpy(&avg_apps[current].app, &apps[0], sizeof(application_t));
    avg_apps[current].num_nodes++;

    // since the first application has been copied, we start at 1
    for (i = 1; i < num_apps; i++) {
        if (avg_apps[current].app.job.id == apps[i].job.id &&
            avg_apps[current].app.job.step_id == apps[i].job.step_id &&
            avg_apps[current].app.job.local_id == apps[i].job.local_id) { // existing application

            acum_sig_metrics(&avg_apps[current].app.signature, &apps[i].signature);
            acum_power_sig_metrics(&avg_apps[current].app.power_sig, &apps[i].power_sig);
            avg_apps[current].num_nodes++; // either way, we get a +1 to the nodes counter
        } else {                           // new application
            current++;
            memcpy(&avg_apps[current].app, &apps[i], sizeof(application_t)); // we copy the signature directly
            avg_apps[current].num_nodes++; // either way, we get a +1 to the nodes counter
        }
    }
    current++;

    // empty the free space
    avg_apps = realloc(avg_apps, sizeof(struct avg_apps) * current);

    for (i = 0; i < current; i++) {
        average_signatures(&avg_apps[i].app.signature, &avg_apps[i].app.signature, avg_apps[i].num_nodes);
        average_power_signatures(&avg_apps[i].app.power_sig, avg_apps[i].num_nodes);
    }
    *sorted_apps     = avg_apps;
    *num_sorted_apps = current;
}

bool print_power_signature_format(application_t *app, char f)
{
    // this improves warnings by the compiler
    signature_format_type fmt = f;
    switch (fmt) {
        case FREQUENCY: // frequency
            print_format(FREQ_WIDTH, app->power_sig.avg_f / 1000000.0, 2);
            break;
        case DEF_FREQ:
            print_format(FREQ_WIDTH, app->power_sig.def_f / 1000000.0, 2);
            break;
        case TIME: // time
            print_format(TIME_WIDTH, app->power_sig.time, 0);
            break;
        case ENERGY: // energy
            print_format(ENERGY_WIDTH, (app->power_sig.DC_power * app->power_sig.time) / (3600 * 1000), 2);
            break;
        case CARBON_FOOTPRINT:
            print_format(CARBON_WIDTH,
                         (((app->power_sig.DC_power * app->power_sig.time) * PUE) / (3600 * 1000)) * CARBON_INTENSITY,
                         2);
            break;
        case DC_POWER: // DC power
            print_format(POWER_WIDTH, app->power_sig.DC_power, 0);
            break;
        case DRAM_POWER: // DRAM power
            print_format(DRAM_POWER_WIDTH, app->power_sig.DRAM_power, 0);
            break;
        case PCK_POWER: // PCK power
            print_format(PCK_POWER_WIDTH, app->power_sig.PCK_power, 0);
            break;
            // these format chars exist in Signatures, but not power_signature. Should be consumed
        case IMC_FREQ: // imc freq
            print_format(IMC_WIDTH, "---");
            break;
        case GBS: // GBS
            print_format(GBS_WIDTH, "---");
            break;
        case MPI_PERC: // MPI perc
            print_format(MPI_WIDTH, "---");
            break;
        case IO_MBS: // IO_MBS
            print_format(IO_WIDTH, "---");
            break;
        case FLOPS: // FLOPS
            for (int i = 0; i < FLOPS_EVENTS; i++) {
                print_format(FLOPS_WIDTH, "---");
            }
            break;
        case GPU: // GPUs
#if EACCT_USE_GPUS
            print_format_custom("%-7.2s/%-7.2s %-7.3s %6s%%/%-7s %-10s", "---", "---", "---", "---", "---%", "---");
#endif
            break;
        case ALL_FREQS: // all frequencies
            print_format_custom("%-3.2lf/", app->power_sig.avg_f / 1000000.0);
            print_format_custom("%-3.2lf/", app->power_sig.def_f / 1000000.0);
            print_format_custom("%-6.2s ", "---");
            break;
        case CACHE_MISSES: // cache misses
            print_format(CACHE_WIDTH, "---");
            print_format(CACHE_WIDTH, "---");
            print_format(CACHE_WIDTH, "---");
            break;
        case CPI: // CPI
            print_format(CPI_WIDTH, "---");
            break;
        case TPI: // TPI
            print_format(TPI_WIDTH, "---");
            break;
        case GFLOPS: // gflops
            print_format(GFLOPS_WIDTH, "---");
            break;
        case CYCLES: // cycles
            print_format(CYCLES_WIDTH, "---");
            break;
        case INSTRUCTIONS: // instructions
            print_format(INSTRUCTIONS_WIDTH, "---");
            break;
        case VPI: // vpi
            print_format(VPI_WIDTH, "---");
            break;
        default:
            return false; // return false if there isn't any field corresponding to this format
    }
    return true;
}

bool print_signature_format(signature_t *sig, char f, int32_t time_length)
{
    // this improves warnings by the compiler
    signature_format_type fmt = f;
    switch (fmt) {
        case DC_POWER: // DC power
            print_format(POWER_WIDTH, sig->DC_power, 0);
            break;
        case DRAM_POWER: // DRAM power
            print_format(DRAM_POWER_WIDTH, sig->DRAM_power, 0);
            break;
        case PCK_POWER: // PCK power
            print_format(PCK_POWER_WIDTH, sig->PCK_power, 0);
            break;
        case GBS: // GBS
            print_format(GBS_WIDTH, sig->GBS, 0);
            break;
        case TPI: // TPI
            print_format(TPI_WIDTH, sig->TPI, 0);
            break;
        case CPI: // CPI
            print_format(CPI_WIDTH, sig->CPI, 2);
            break;
        case GFLOPS: // gflops/watt
        {
            double gflops_watt = sig->Gflops;
            for (int32_t i = 0; i < sig->gpu_sig.num_gpus; i++) {
                gflops_watt += sig->gpu_sig.gpu_data[i].GPU_GFlops;
            }
            gflops_watt /= sig->DC_power;
            print_format(GFLOPS_WIDTH, gflops_watt, 2);
        } break;
        case TIME: // time
            print_format(TIME_WIDTH, sig->time, time_length);
            break;
        case FLOPS: // FLOPS
            for (int i = 0; i < FLOPS_EVENTS; i++)
                print_format(FLOPS_WIDTH, sig->FLOPS[i]);
            break;
        case CACHE_MISSES: // L1/2/3 misses
            print_format(CACHE_WIDTH, sig->L1_misses);
            print_format(CACHE_WIDTH, sig->L2_misses);
            print_format(CACHE_WIDTH, sig->L3_misses);
            break;
        case INSTRUCTIONS: // instructions
            print_format(INSTRUCTIONS_WIDTH, sig->instructions);
            break;
        case CYCLES: // cycles
            print_format(CYCLES_WIDTH, sig->cycles);
            break;
        case ALL_FREQS: // all three frequencies
            print_format_custom("%-3.2lf/", sig->avg_f / 1000000.0);
            print_format_custom("%-3.2lf/", sig->def_f / 1000000.0);
            print_format_custom("%-6.2lf ", sig->avg_imc_f / 1000000.0);
            break;
        case FREQUENCY: // frequency
            print_format(FREQ_WIDTH, sig->avg_f / 1000000.0, 2);
            break;
        case IMC_FREQ: // imc freq
            print_format(IMC_WIDTH, sig->avg_imc_f / 1000000.0, 2);
            break;
        case DEF_FREQ: // def_f
            print_format(FREQ_WIDTH, sig->def_f / 1000000.0, 2);
            break;
        case MPI_PERC: // MPI perc
            print_format(MPI_WIDTH, sig->perc_MPI, 1);
            break;
        case IO_MBS: // IO_MBS
            print_format(IO_WIDTH, sig->IO_MBS, 1);
            break;
        case VPI: // vpi
        {
            double vpi;
            compute_sig_vpi(&vpi, sig);
            print_format(VPI_WIDTH, vpi * 100, 2);
        } break;
        case ENERGY: // energy
            print_format(ENERGY_WIDTH, sig->DC_power * sig->time / (3600 * 1000), 2);
            break;
        case CARBON_FOOTPRINT:
            print_format(CARBON_WIDTH, (((sig->DC_power * sig->time) * PUE) / (3600 * 1000)) * CARBON_INTENSITY, 2);
            break;
        case GPU: // GPUs
#if EACCT_USE_GPUS
        { // declare a custom scope to avoid compiler complains
            gpu_signature_t *ptr = &sig->gpu_sig;
            double gpu_power = 0, gpu_total_power = 0;
            double gpu_gflops  = 0;
            ulong gpu_freq     = 0;
            ulong gpu_util     = 0;
            ulong gpu_mem_util = 0;
            ;
            int num_gpus = 0;

            if (ptr->num_gpus > 0) {
                for (int i = 0; i < ptr->num_gpus; i++) {
                    gpu_total_power += ptr->gpu_data[i].GPU_power;
                    gpu_freq += ptr->gpu_data[i].GPU_freq;
                    if (ptr->gpu_data[i].GPU_util > 0) {
                        gpu_power += ptr->gpu_data[i].GPU_power;
                        gpu_util += ptr->gpu_data[i].GPU_util;
                        gpu_mem_util += ptr->gpu_data[i].GPU_mem_util;
                        gpu_gflops += ptr->gpu_data[i].GPU_GFlops;
                        num_gpus++;
                    }
                }

                if (num_gpus > 0) {
                    gpu_util /= num_gpus;
                    gpu_mem_util /= num_gpus;
                    gpu_gflops /= num_gpus;
                    gpu_freq /= num_gpus;
                    char mem_util_buf[16] = {0};
                    snprintf(mem_util_buf, 16, "%lu%%", gpu_mem_util);
                    print_format_custom("%-7.2lf/%-7.2lf %-7.3lf %6lu%%/%-7s %-10.2lf", gpu_total_power, gpu_power,
                                        (double) gpu_freq / 1000000, gpu_util, mem_util_buf, gpu_gflops);
                } else {
                    print_format_custom(" %-7.2lf/%-7.2s %-7.3s %6s%%/%-7s%% %-10s", gpu_total_power, "---", "---",
                                        "---", "---", "---");
                }
            } else {
                print_format_custom(" %-7.2lf/%-7.2s %-7.3s %6s%%/%-7s%% %-10s", gpu_total_power, "---", "---", "---",
                                    "---", "---");
            }
        }
#endif
        break;
        default:
            return false; // return false if there isn't any field corresponding to this format
    }
    return true;
}

void print_app_format(application_t *app, char *fmt, int num_nodes)
{
    uint32_t i;
    char is_sbatch;
    bool is_consumed;

    // print the color legend if possible
    if (app->is_mpi) {
        select_color_by_phase(app->signature.CPI, app->signature.GBS, app->signature.IO_MBS, app->signature.Gflops);
    }

    for (i = 0; i < strlen(fmt); i++) { // this is preferable to a while loop because of the use of continues

        // is_consumed is 1 only if that format char corresponds to signature/power signature
        if (app->is_mpi && !all_pow_sig)
            is_consumed = print_signature_format(&app->signature, fmt[i], 0);
        else
            is_consumed = print_power_signature_format(app, fmt[i]);

        if (is_consumed)
            continue;

        // this improves warnings by the compiler
        job_format_type tmp = fmt[i];

        switch (tmp) {
            case APP_ID: // application id
                if (strlen(app->job.app_id) > APP_WIDTH) {
                    if (strchr(app->job.app_id, '/') != NULL) {
                        strcpy(app->job.app_id, strrchr(app->job.app_id, '/') + 1);
                    } else { // dirty, but it works
                        app->job.app_id[APP_WIDTH] = '\0';
                    }
                }
                print_format(APP_WIDTH, app->job.app_id);
                break;
            case JOB_WSTEP: // jobid
                is_sbatch = (app->job.step_id == (uint32_t) BATCH_STEP) ? 1 : 0;
                if (is_sbatch)
                    print_format_custom("%8lu-%-4s ", app->job.id, "sb");
                else
                    print_format_custom("%8lu-%-4lu ", app->job.id, app->job.step_id);
                break;
            case LOCAL_ID: // local_id
                print_format(OTHER_WIDTH, app->job.local_id);
                break;
            case USER_ID: // user id
                if (display_user_column) {
                    char tmp[256] = {0};
                    strcpy(tmp, app->job.user_id);
                    if (strlen(app->job.user_id) > OTHER_WIDTH) {
                        tmp[OTHER_WIDTH - 1] = '\0';
                    }
                    print_format(OTHER_WIDTH, tmp);
                }
                break;
            case NODE_ID: // node id
                print_format(OTHER_WIDTH, app->node_id);
                break;
            case NUM_NODES: // num nodes
                print_format(OTHER_WIDTH, num_nodes);
                break;
            case DATE_ITER:
                // print_format_custom("%sDate('%c') format character not supported for jobs%s\n", COL_RED, DATE_ITER,
                // COL_CLR);
                {
                    struct tm *date_info = localtime((time_t *) &app->job.start_time);
                    char date_iter[128]  = {0};
                    strftime(date_iter, sizeof(date_iter), "%H:%M:%S", date_info);
                    print_format(OTHER_WIDTH, date_iter);
                    date_info = localtime((time_t *) &app->job.end_time);
                    memset(date_iter, 0, sizeof(date_iter));
                    strftime(date_iter, sizeof(date_iter), "%H:%M:%S", date_info);
                    print_format(OTHER_WIDTH, date_iter);
                }
                break;
            default:
                print_format_custom("%sunknown format char %c%s\n", COL_RED, fmt[i], COL_CLR);
                break;
        }
    }
    print_format_custom("\n");

    // reset the color
    print_format_custom("%s", COL_CLR);
    fflush(stdout);
}

void print_header_format(char *fmt, uint32_t type)
{
    while (*fmt) {
        switch (*fmt) {
            case APP_ID: // application id
                print_format(APP_WIDTH, "APPLICATION");
                break;
            case JOB_WSTEP: // jobid
                print_format_custom("%8s-%-4s ", "JOB", "STEP");
                break;
            case USER_ID: // user id
                if (display_user_column)
                    print_format(OTHER_WIDTH, "USER");
                break;
            case LOCAL_ID:
                print_format(OTHER_WIDTH, "AID");
                break;
            case DATE_ITER:
                if (type == APP_TYPE) {
                    print_format(OTHER_WIDTH, "START TIME");
                    print_format(OTHER_WIDTH, "END TIME");
                } else {
#if REPORT_TIMESTAMP
                    print_format(OTHER_WIDTH, "TIMESTAMP");
#else
                    print_format(OTHER_WIDTH, "ITER.");
#endif
                }
                break;
            case NODE_ID: // node id
                print_format(OTHER_WIDTH, "NODE");
                break;
            case NUM_NODES: // num nodes
                print_format(OTHER_WIDTH, "NODES");
                break;
            case DC_POWER: // DC power
                print_format(POWER_WIDTH, "POWER(W)");
                break;
            case GPU: // GPUs
#if EACCT_USE_GPUS
                print_format_custom("%-15s %-8s %-14s %-10s", "G-POW (T/U)", "G-FREQ", "G-UTIL(G/MEM)", "G-GFLOPS");
#endif
                break;
            case DRAM_POWER: // DRAM power
                print_format(DRAM_POWER_WIDTH, "DRAM POW(W)");
                break;
            case PCK_POWER: // PCK power
                print_format(PCK_POWER_WIDTH, "PKG POW(W)");
                break;
            case GBS: // GBS
                print_format(GBS_WIDTH, "GBS");
                break;
            case TPI: // TPI
                print_format(TPI_WIDTH, "TPI");
                break;
            case CPI: // CPI
                print_format(CPI_WIDTH, "CPI");
                break;
            case GFLOPS: // gflops
                print_format(GFLOPS_WIDTH, "GFLOPS/W");
                break;
            case TIME: // time
                print_format(TIME_WIDTH, "TIME(s)");
                break;
            case FLOPS: // FLOPS
                for (int i = 0; i < FLOPS_EVENTS; i++)
                    print_format_custom("%-*s%-*d ", FLOPS_WIDTH / 2, "FLOPS", FLOPS_WIDTH / 2, i + 1);
                break;
            case CACHE_MISSES: // L1/2/3 misses
                print_format(CACHE_WIDTH, "L1_misses");
                print_format(CACHE_WIDTH, "L2_misses");
                print_format(CACHE_WIDTH, "L3_misses");
                break;
            case INSTRUCTIONS: // instructions
                print_format(INSTRUCTIONS_WIDTH, "INSTRUCT");
                break;
            case CYCLES: // cycles
                print_format(CYCLES_WIDTH, "CYCLES");
                break;
            case ALL_FREQS: // all three freqs in format
                print_format_custom("%-3s/%-3s/%-8s ", "AVG", "DEF", "MEM(GHz)");
                break;
            case FREQUENCY: // frequency
                print_format(FREQ_WIDTH, "AVG(GHz)");
                break;
            case IMC_FREQ: // imc freq
                print_format(FREQ_WIDTH, "MEM(GHz)");
                break;
            case DEF_FREQ: // def_f
                print_format(FREQ_WIDTH, "DEF(GHz)");
                break;
            case MPI_PERC: // MPI perc
                print_format(MPI_WIDTH, "MPI(%)");
                break;
            case IO_MBS: // IO_MBGS
                print_format(IO_WIDTH, "IO(MBs)");
                break;
            case VPI: // vpi
                print_format(VPI_WIDTH, "VPI(%)");
                break;
            case ENERGY: // energy
                print_format(ENERGY_WIDTH, "ENERGY(KWh)");
                break;
            case CARBON_FOOTPRINT:
                print_format(CARBON_WIDTH, "CARBON FOOTPRINT(g)");
                break;
        }
        fmt++;
    }
    printf("\n");
}

void print_full_apps(application_t *apps, int num_apps, bool is_csv, char *format)
{
    if (is_csv) {
        if (!strcmp(csv_path, "no_file")) {
            char header[8192] = {0};
            // num_gpus is ignored.
            application_create_header_str(header, sizeof(header), NULL, 0, 1, 0);
            dprintf(STDOUT_FILENO, "%s\n", header);
            for (int i = 0; i < num_apps; i++)
                print_application_fd(STDOUT_FILENO, &apps[i], my_conf.database.report_sig_detail, 1, 0);
        } else {
            for (int i = 0; i < num_apps; i++) {
                append_application_text_file(csv_path, &apps[i], my_conf.database.report_sig_detail, 1, 0);
            }
        }
    } else {
        print_header_format(format, APP_TYPE);
        for (int i = 0; i < num_apps; i++) {
            print_app_format(&apps[i], format, 1);
        }
    }
}

void print_loop_format(loop_t *loop, char *fmt)
{
    // print the color legend if possible
    select_color_by_phase(loop->signature.CPI, loop->signature.GBS, loop->signature.IO_MBS, loop->signature.Gflops);

    for (uint32_t i = 0; i < strlen(fmt); i++) { // this is preferable to a while loop because of the use of continues

        // is_consumed is 1 only if that format char corresponds to signature/power signature
        if (print_signature_format(&loop->signature, fmt[i], 3))
            continue;

        // this improves warnings by the compiler
        job_format_type tmp = fmt[i];

        switch (tmp) {
            case APP_ID: // application id
                print_format_custom("%sApp_id('%c') format character not supported for jobs%s\n", COL_RED, APP_ID,
                                    COL_CLR);
                break;
            case JOB_WSTEP: // jobid
                print_format_custom("%8lu-%-4lu ", loop->jid, loop->step_id);
                break;
            case LOCAL_ID: // local_id
                print_format(OTHER_WIDTH, loop->local_id);
                break;
            case USER_ID: // user id
                print_format_custom("%sUSER_ID('%c') format character not supported for jobs%s\n", COL_RED, USER_ID,
                                    COL_CLR);
                break;
            case NODE_ID: // node id
                print_format(OTHER_WIDTH, loop->node_id);
                break;
            case NUM_NODES: // num nodes
                print_format(OTHER_WIDTH, 1);
                break;
            case DATE_ITER:
#if REPORT_TIMESTAMP
            {
                struct tm *date_info = localtime((time_t *) &loop->total_iterations);
                char date_iter[128]  = {0};
                strftime(date_iter, sizeof(date_iter), "%H:%M:%S", date_info);
                print_format(OTHER_WIDTH, date_iter);
            }
#else
                print_format(OTHER_WIDTH, loop->total_iterations);
#endif
            break;
            default:
                print_format_custom("%sunknown format char %c%s\n", COL_RED, fmt[i], COL_CLR);
                break;
        }
    }
    print_format_custom("\n");

    // reset the color
    print_format_custom("%s", COL_CLR);
    fflush(stdout);
}

void print_short_apps(application_t *apps, int num_apps, bool is_csv, char *format)
{
    if (format == NULL) {
        printf("Format string cannot be null\n");
        return;
    }

    if (num_apps == 0)
        printf("No applications retrieved\n");

    struct avg_apps *sorted_apps = NULL;
    int new_num_apps             = 0;
    average_applications(apps, num_apps, &sorted_apps, &new_num_apps);

    if (is_csv) {
        if (!strcmp(csv_path, "no_file")) {
            char header[8192] = {0};
            // num_gpus is ignored.
            application_create_header_str(header, sizeof(header), NULL, 0, 1, 0);
            dprintf(STDOUT_FILENO, "%s\n", header);
            for (int i = 0; i < new_num_apps; i++)
                print_application_fd(STDOUT_FILENO, &sorted_apps[i].app, my_conf.database.report_sig_detail, 1, 0);
        } else {
            for (int i = 0; i < new_num_apps; i++) {
                append_application_text_file(csv_path, &sorted_apps[i].app, my_conf.database.report_sig_detail, 1, 0);
            }
        }
    } else {
        int i;
        print_header_format(format, APP_TYPE);
        for (i = 0; i < new_num_apps; i++) {
            print_app_format(&sorted_apps[i].app, format, sorted_apps[i].num_nodes);
        }
    }
    free(sorted_apps);
}

void process_and_print_applications(application_t *apps, int num_apps, bool is_csv, bool aggregate, char *format)
{
    if (aggregate)
        print_short_apps(apps, num_apps, is_csv, format);
    else
        print_full_apps(apps, num_apps, is_csv, format);
}

void print_event_type(int type, int fd)
{
    char str[64];
    ear_event_t aux;
    aux.event = type;
    event_type_to_str(&aux, str, sizeof(str));
    dprintf(fd, "%20s ", str);
}

#if DB_MYSQL
void mysql_print_events(MYSQL_RES *result, int fd)
{
    if (fd < 0) {
        fd = 1; // The standard output
    }

    int i;
    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    int has_records = 0;
    while ((row = mysql_fetch_row(result)) != NULL) {
        if (!has_records) {
            dprintf(fd, "%12s %20s %20s %8s %8s %20s %12s\n", "Event_ID", "Timestamp", "Event_type", "Job_id",
                    "Step_id", "Value", "node_id");
            has_records = 1;
        }

        int possible_negative = 0; // This variable checks whether the event value can be negative.
        for (i = 0; i < num_fields; i++) {
            if (i == 2) {
                int event_type_int = strtol(row[i], NULL, 10);

                print_event_type(event_type_int, fd);

                if (event_type_int == ENERGY_SAVING || event_type_int == POWER_SAVING ||
                    event_type_int == POWER_SAVING_AVG || event_type_int == ENERGY_SAVING_AVG ||
                    event_type_int == PERF_PENALTY || event_type_int == PERF_PENALTY_AVG ||
                    event_type_int == GPU_POWER_SAVING_AVG || event_type_int == GPU_ENERGY_SAVING_AVG ||
                    event_type_int == GPU_POWER_SAVING_AVG || event_type_int == GPU_ENERGY_SAVING_AVG ||
                    event_type_int == GPU_PERF_PENALTY || event_type_int == GPU_PERF_PENALTY_AVG) {
                    possible_negative = 1;
                }
            } else if (i == 1)
                dprintf(fd, "%20s ", row[i] ? row[i] : "NULL");
            else if (i == 4 || i == 3)
                dprintf(fd, "%8s ", row[i] ? row[i] : "NULL");
            else if (i == 5) {
                if (row[i] != NULL) {
                    if (possible_negative) {
                        long int value = atoi(row[i]);
                        dprintf(fd, "%20ld ", *(long *) &value);
                    } else {
                        dprintf(fd, "%20s ", row[i]);
                    } // Column 5 not possible negative
                } else {
                    dprintf(fd, "%20s ", "NULL");
                } // Column 5 NULL
            } else {
                dprintf(fd, "%12s ", row[i] ? row[i] : "NULL");
            } // Column 0
        }
        dprintf(fd, "\n");
    }
    if (!has_records) {
        printf("There are no events with the specified properties.\n\n");
    }
}
#elif DB_PSQL
void postgresql_print_events(PGresult *res, int fd)
{
    if (fd < 0) {
        fd = 1; // Standard output
    }

    int i, j, num_fields, has_records = 0;
    num_fields = PQnfields(res);

    int possible_negative = 0; // This variable checks whether the event value can be negative.
    for (i = 0; i < PQntuples(res); i++) {
        if (!has_records) {
            dprintf(fd, "%12s %22s %20s %8s %8s %20s %12s\n", "Event_ID", "Timestamp", "Event_type", "Job_id",
                    "Step_id", "Value", "node_id");
            has_records = 1;
        }

        for (j = 0; j < num_fields; j++) {
            if (j == 2) {
                int event_type_int = strtol(PQgetvalue(res, i, j), NULL, 10);

                print_event_type(event_type_int, fd);

                if (event_type_int == ENERGY_SAVING || event_type_int == POWER_SAVING) {
                    possible_negative = 1;
                }
            } else if (i == 1)
                dprintf(fd, "%22s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
            else if (i == 4 || i == 3)
                dprintf(fd, "%8s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
            else if (i == 5) {

                if (PQgetvalue(res, i, j) != NULL) {

                    if (possible_negative) {
                        long int value = atoi(PQgetvalue(res, i, j));
                        dprintf(fd, "%20ld ", *(long *) &value);
                    } else {
                        dprintf(fd, "%20s ", PQgetvalue(res, i, j));
                    } // Column 5 not possible negative
                } else {
                    dprintf(fd, "%20s ", "NULL");
                } // Column 5 NULL

            } else
                dprintf(fd, "%12s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
        }

        dprintf(fd, "\n");
    }

    if (!has_records) {
        printf("There are no events with the specified properties.\n\n");
    }
}
#endif

#if DB_MYSQL
#define EVENTS_QUERY     "SELECT id, FROM_UNIXTIME(timestamp), event_type, job_id, step_id, value, node_id FROM Events"
#define EVENTS_QUERY_RAW "SELECT id, timestamp, event_type, job_id, step_id, value, node_id FROM Events"
#elif DB_PSQL
#define EVENTS_QUERY     "SELECT id, to_timestamp(timestamp), event_type, job_id, step_id, value, node_id FROM Events"
#define EVENTS_QUERY_RAW "SELECT id, timestamp, event_type, job_id, step_id, value, node_id FROM Events"
#endif

void read_events(char *user, query_adds_t *q_a)
{
    char query[512];
    char subquery[128];

    if (strlen(my_conf.database.user_commands) < 1) {
        fprintf(stderr, "Warning: commands' user is not defined in ear.conf\n");
    } else {
        strcpy(my_conf.database.user, my_conf.database.user_commands);
        strcpy(my_conf.database.pass, my_conf.database.pass_commands);
    }
    init_db_helper(&my_conf.database);

    int fd = -1;
    if (strlen(csv_path) > 0 && strcmp(csv_path, "no_file")) {
        fd = open(csv_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
    }

    if (fd > 0) {
        strcpy(query, EVENTS_QUERY_RAW);
    } else {
        strcpy(query, EVENTS_QUERY);
    }

    add_int_comp_filter(query, "event_type", 100, 0);
    if (q_a->job_id >= 0)
        add_int_filter(query, "job_id", q_a->job_id);
    else if (strlen(q_a->job_ids) > 0)
        add_int_list_filter(query, "job_id", q_a->job_ids);
    if (q_a->step_id >= 0)
        add_int_filter(query, "step_id", q_a->step_id);
    if (strlen(q_a->step_ids) > 0)
        add_string_filter_no_quotes(query, "step_id", q_a->step_ids);
    if (user != NULL)
        add_string_filter(query, "node_id", user);
    if (q_a->start_time != 0)
        add_int_comp_filter(query, "timestamp", q_a->start_time, 1);
    if (q_a->end_time != 0)
        add_int_comp_filter(query, "timestamp", q_a->end_time, 0);

    if (q_a->limit > 0) {
        sprintf(subquery, " ORDER BY timestamp desc LIMIT %d", q_a->limit);
        strcat(query, subquery);
    }

    if (verbose)
        printf("QUERY: %s\n", query);

#if DB_MYSQL
    MYSQL_RES *result = db_run_query_result(query);
#elif DB_PSQL
    PGresult *result = db_run_query_result(query);
#endif

    if (result == NULL) {
        printf("Database error\n");
        return;
    }

#if DB_MYSQL
    mysql_print_events(result, fd);
#elif DB_PSQL
    postgresql_print_events(result, fd);
#endif
}

#define LOOPS_QUERY "SELECT * FROM Loops "
#define LOOPS_FILTER_QUERY                                                                                             \
    "SELECT Loops.* from Loops INNER JOIN Jobs ON Jobs.job_id = Loops.job_id AND Jobs.step_id = Loops.step_id "

void format_loop_query(char *user, char *query, char *base_query, query_adds_t *q_a)
{
    char subquery[128];

    strcpy(query, base_query);
    if (user != NULL)
        add_string_filter(query, "user_id", user);

    if (q_a->job_id >= 0)
        add_int_filter(query, "Loops.job_id", q_a->job_id);
    else if (strlen(q_a->job_ids) > 0)
        add_int_list_filter(query, "Loops.job_id", q_a->job_ids);
    if (q_a->step_id >= 0)
        add_int_filter(query, "Loops.step_id", q_a->step_id);
    if (strlen(q_a->step_ids) > 0)
        add_string_filter_no_quotes(query, "Loops.step_id", q_a->step_ids);

    if (q_a->limit > 0 && q_a->job_id < 0) {
        sprintf(subquery, " ORDER BY Loops.job_id desc LIMIT %d", q_a->limit);
        strcat(query, subquery);
    } else
        strcat(query, " ORDER BY Loops.job_id desc");

    if (verbose)
        printf("QUERY: %s\n", query);
}

#define NUM_HEADER_NAMES 17

void read_jobs_from_loops(query_adds_t *q_a)
{
    char query[256], subquery[256], tmp_path[512];
    char ***values;
    int columns, rows;
    int fd = STDOUT_FILENO, ret;

    if (strlen(csv_path) > 0 && strcmp(csv_path, "no_file")) {
        char *aux = strrchr(csv_path, '/');
        if (aux != NULL) {
            aux++; // move the pointer from the '/' to the next part
            strncpy(tmp_path, csv_path, strlen(csv_path) - strlen(aux));
            strcat(tmp_path, "out_jobs.");
            strcat(tmp_path, aux);
        } else {
            sprintf(tmp_path, "out_jobs.%s", csv_path);
        }
        fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
    }

    ret = db_run_query_string_results("DESCRIBE Jobs", &values, &columns, &rows);
    if (verbose)
        printf("running query DESCRIBE Jobs\n");
    if (ret != EAR_SUCCESS) {
        printf("Error reading Loop description from database.\n");
        return;
    }
    const char *header_names[NUM_HEADER_NAMES] = {
        "JOBID",    "STEPID",         "AID",          "USERID",     "APPLICATION", "START_TIME",
        "END_TIME", "START_MPI_TIME", "END_MPI_TIME", "POLICY",     "POLICY_TH",   "PROCS",
        "JOB_TYPE", "DEF_FREQ_KHZ",   "USER_ACC",     "USER_GROUP", "ENERGY_TAG",
    };
    if (rows != NUM_HEADER_NAMES) {
        printf("Warning! number of fields does not correspond with nubmer of headers\n");
    }
    dprintf(fd, "%s", header_names[0]);
    for (int32_t i = 1; i < NUM_HEADER_NAMES; i++) {
        dprintf(fd, ";%s", header_names[i]);
    }
    dprintf(fd, "\n"); // print_header does not print a new_line
    db_free_results(values, columns, rows);

    // reset filters
    reset_query_filters();
    strcpy(query, "SELECT * FROM Jobs ");

    if (q_a->job_id >= 0)
        add_int_filter(query, "job_id", q_a->job_id);
    else if (strlen(q_a->job_ids) > 0)
        add_int_list_filter(query, "job_id", q_a->job_ids);
    else
        printf("WARNING: -o option is meant to be used with a -j specification\n\n");
    if (q_a->step_id >= 0)
        add_int_filter(query, "step_id", q_a->step_id);
    if (q_a->start_time > 0)
        add_int_filter(query, "start_time", q_a->step_id);
    if (q_a->end_time > 0)
        add_int_filter(query, "end_time", q_a->step_id);

    if (q_a->limit > 0 && q_a->job_id < 0) {
        sprintf(subquery, " ORDER BY job_id desc LIMIT %d", q_a->limit);
        strcat(query, subquery);
    } else
        strcat(query, " ORDER BY job_id desc");

    if (verbose)
        printf("\nQUERY: %s\n", query);
    ret = db_run_query_string_results(query, &values, &columns, &rows);
    if (ret != EAR_SUCCESS) {
        printf("Error reading Jobs from database\n");
        return;
    }

    print_values(fd, values, columns, rows);
    db_free_results(values, columns, rows);
}

void read_loops(char *user, query_adds_t *q_a, char *format)
{
    char query[1024];

    if (strlen(my_conf.database.user_commands) < 1) {
        fprintf(stderr, "Warning: commands' user is not defined in ear.conf\n");
    } else {
        strcpy(my_conf.database.user, my_conf.database.user_commands);
        strcpy(my_conf.database.pass, my_conf.database.pass_commands);
    }
    init_db_helper(&my_conf.database);

    if (user != NULL) {
        format_loop_query(user, query, LOOPS_FILTER_QUERY, q_a);
    } else {
        format_loop_query(user, query, LOOPS_QUERY, q_a);
    }

    loop_t *loops = NULL;
    int num_loops;

    num_loops = db_read_loops_query(&loops, query);

    if (num_loops < 1) {
        fprintf(stderr, "No loops retrieved\n");
        exit(1);
    }

    if (verbose)
        printf("retrieved %d loops\n", num_loops);
    if (strlen(csv_path) > 0) {
        int i;
        if (!strcmp(csv_path, "no_file")) {
            for (i = 0; i < num_loops; i++)
                print_loop_fd(STDOUT_FILENO, &loops[i]);
        } else {
            for (i = 0; i < num_loops; i++)
                append_loop_text_file_no_job(csv_path, &loops[i], 1, 0, ' ');
            printf("appended %d loops to %s\n", num_loops, csv_path);
        }
    } else {
        print_header_format(format, LOOP_TYPE);
        for (int32_t i = 0; i < num_loops; i++)
            print_loop_format(&loops[i], format);
        // print_loops(loops, num_loops, format);
    }
    if (loops != NULL)
        free(loops);
    if (loop_extended)
        read_jobs_from_loops(q_a);
}

void read_applications_from_database(char *user, query_adds_t *q_a, char *format)
{
#if USE_DB
    int num_apps = 0;
    char subquery[256];
    char query[512];

    if (strlen(my_conf.database.user_commands) < 1) {
        fprintf(stderr, "Warning: commands' user is not defined in ear.conf\n");
    } else {
        strcpy(my_conf.database.user, my_conf.database.user_commands);
        strcpy(my_conf.database.pass, my_conf.database.pass_commands);
    }
    init_db_helper(&my_conf.database);

    if (verbose) {
        printf("Preparing query statement\n");
    }

    sprintf(query, "SELECT Applications.* FROM Applications join Jobs on Applications.job_id=Jobs.job_id and "
                   "Applications.step_id = Jobs.step_id and Jobs.local_id = Applications.local_id where Jobs.job_id in "
                   "(select job_id from (select distinct(job_id) from Jobs");
    application_t *apps;
    if (q_a->job_id >= 0)
        add_int_filter(query, "job_id", q_a->job_id);
    else if (strlen(q_a->job_ids) > 0)
        add_int_list_filter(query, "job_id", q_a->job_ids);
    if (q_a->step_id >= 0)
        add_int_filter(query, "step_id", q_a->step_id);
    else if (strlen(q_a->step_ids) > 0)
        add_string_filter_no_quotes(query, "step_id", q_a->step_ids);
    if (user != NULL)
        add_string_filter(query, "user_id", user);
    if (strlen(q_a->e_tag) > 0)
        add_string_filter(query, "e_tag", q_a->e_tag);
    if (strlen(q_a->app_id) > 0)
        add_string_filter(query, "app_id", q_a->app_id);
    if (q_a->start_time != 0)
        add_int_comp_filter(query, "start_time", q_a->start_time, 1);
    if (q_a->end_time != 0)
        add_int_comp_filter(query, "end_time", q_a->end_time, 0);

    if (q_a->limit > 0) {
        sprintf(subquery, " ORDER BY Jobs.end_time desc LIMIT %d", q_a->limit);
        strcat(query, subquery);
    }
    strcat(query, ") as t1");

    reset_query_filters();
    if (q_a->job_id >= 0)
        add_int_filter(query, "job_id", q_a->job_id);
    else if (strlen(q_a->job_ids) > 0)
        add_int_list_filter(query, "job_id", q_a->job_ids);
    if (q_a->step_id >= 0)
        add_int_filter(query, "Jobs.step_id", q_a->step_id);
    else if (strlen(q_a->step_ids) > 0)
        add_string_filter_no_quotes(query, "Jobs.step_id", q_a->step_ids);
    if (user != NULL)
        add_string_filter(query, "user_id", user);

    strcat(query, ") order by Jobs.job_id desc, Jobs.step_id desc, Jobs.end_time desc");

    if (verbose) {
        printf("Retrieving applications\n");
    }

    if (verbose) {
        printf("QUERY: %s\n", query);
    }

    num_apps = db_read_applications_query(&apps, query);

    if (verbose) {
        printf("Finalized retrieving applications\n");
    }

    if (num_apps == EAR_MYSQL_ERROR) {
        exit(1); // error
    }

    if (num_apps < 1) {
        fprintf(stderr, "No jobs found.\n");
        exit(1);
    }

    if (q_a->limit == DEFAULT_QUERY_LIMIT && strlen(csv_path) < 1 && num_apps >= DEFAULT_QUERY_LIMIT)
        printf("\nBy default only the first %d jobs are retrieved.\n\n", DEFAULT_QUERY_LIMIT);

    bool is_csv    = (strlen(csv_path) > 1);
    bool aggregate = !full_length;
    process_and_print_applications(apps, num_apps, is_csv, aggregate, format);

    free(apps);

#endif
}

void read_applications_from_files(char *path, char *format)
{
    application_t *apps;
    int num_apps = read_application_text_file(path, &apps, 0);
    if (full_length)
        print_full_apps(apps, num_apps, false, format);
    else
        print_short_apps(apps, num_apps, false, format);
}

int main(int argc, char *argv[])
{
    int c;

    query_adds_t query_adds = {0};

    query_adds.limit   = DEFAULT_QUERY_LIMIT;
    query_adds.job_id  = -1;
    query_adds.step_id = -1;

    char is_loops  = 0;
    char is_events = 0;

    char path_name[256] = {0};
    char format[256]    = {0};
    char *file_name     = NULL;

    struct tm tinfo = {0};

    verb_level   = -1;
    verb_enabled = 0;

    if (check_and_unset_environment_variables()) {
        printf("Warning: using MySQL/MariaDB environment variables is not supported, unsetting them.\n");
    }

    if (get_ear_conf_path(path_name) == EAR_ERROR) {
        fprintf(stderr, "Error getting ear.conf path\n");
        exit(1);
    }

    if (read_cluster_conf(path_name, &my_conf) != EAR_SUCCESS) {
        fprintf(stderr, "ERROR reading cluster configuration\n");
    }

    user_t user_info;
    if (user_all_ids_get(&user_info) != EAR_SUCCESS) {
        fprintf(stderr, "Failed to retrieve user data\n");
        exit(1);
    }

    char *user = user_info.ruid_name;
    if (getuid() == 0 || is_privileged_command(&my_conf)) {
        user                = NULL; // by default, privilegd users or root will query all user jobs
        display_user_column = true;
    }

    char *token;
    int option_idx;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},           {"version", no_argument, 0, 'v'},
        {"no-mpi", no_argument, 0, 'm'},         {"avx", no_argument, 0, 'p'},
        {"verbose", no_argument, 0, 'b'},        {"show-gpus", no_argument, 0, 'g'},
        {"long-apps", no_argument, 0, 'l'},      {"loops", no_argument, 0, 'r'},
        {"ext_loops", no_argument, 0, 'o'},      {"help", no_argument, 0, 'h'},
        {"limit", required_argument, 0, 'n'},    {"user", required_argument, 0, 'u'},
        {"jobs", required_argument, 0, 'j'},     {"events", required_argument, 0, 'x'},
        {"csv", required_argument, 0, 'c'},      {"tag", required_argument, 0, 't'},
        {"app-id", required_argument, 0, 'a'},   {"start-time", required_argument, 0, 's'},
        {"end-time", required_argument, 0, 'e'}, {"format", required_argument, 0, 'F'},
    };

#if COLORS
    print_colors_legend();
#endif

    while (1) {
        c = getopt_long(argc, argv, "n:u:j:f:t:voma:pbglrs:e:c:hF:x::", long_options, &option_idx);

        if (c == -1)
            break;

        switch (c) {
            case 'n':
                if (!strcmp(optarg, "all"))
                    query_adds.limit = -1;
                else
                    query_adds.limit = atoi(optarg);
                break;
            case 'r':
                is_loops = 1;
                break;
            case 'u':
                if (user != NULL)
                    break;
                user = optarg;
                break;
            case 'j':
                query_adds.limit = query_adds.limit == DEFAULT_QUERY_LIMIT
                                       ? -1
                                       : query_adds.limit; // if the limit is still the default
                if (strchr(optarg, ',')) {
                    strcpy(query_adds.job_ids, optarg);
                } else {
                    query_adds.job_id = atoi(strtok(optarg, "."));
                    token             = strtok(NULL, ".");
                    if (token != NULL) {
                        if (!strcmp(token, "sbatch") || !strcmp(token, "sb")) {
                            sprintf(query_adds.step_ids, "%u", BATCH_STEP);
                        } else
                            strcpy(query_adds.step_ids, token);
                    }
                    // if (token != NULL) query_adds.step_id = atoi(token);
                }
                break;
            case 'x':
                is_events = 1;
                if (optind < argc && strchr(argv[optind], '-') == NULL) {
                    if (strchr(argv[optind], ',')) {
                        strcpy(query_adds.job_ids, argv[optind]);
                    } else {
                        query_adds.job_id = atoi(strtok(argv[optind], "."));
                        token             = strtok(NULL, ".");
                        if (token != NULL) {
                            if (!strcmp(token, "sbatch") || !strcmp(token, "sb")) {
                                sprintf(query_adds.step_ids, "%u", BATCH_STEP);
                            } else
                                strcpy(query_adds.step_ids, token);
                        }
                        // if (token != NULL) query_adds.step_id = atoi(token);
                    }
                } else if (verbose)
                    printf("No argument for -x\n");
                break;
            case 'o':
                loop_extended = 1;
                break;
            case 'g':
                print_gpus = 1;
                break;
            case 'f':
                file_name = optarg;
                break;
            case 'l':
                full_length = 1;
                break;
            case 'b':
                verbose = 1;
                break;
            case 'v':
                free_cluster_conf(&my_conf);
                print_version();
                exit(0);
                break;
            case 'm':
                all_pow_sig = 1;
                break;
            case 'c':
                strcpy(csv_path, optarg);
                break;
            case 't':
                strcpy(query_adds.e_tag, optarg);
                break;
            case 'p':
                avx = 1;
                break;
            case 'a':
                strcpy(query_adds.app_id, optarg);
                break;
            case 's':
                if (strptime(optarg, "%Y-%m-%e", &tinfo) == NULL) {
                    printf("Incorrect time format. Supported format is YYYY-MM-DD\n"); // error
                    free_cluster_conf(&my_conf);
                    exit(1);
                }
                query_adds.start_time = mktime(&tinfo);
                break;
            case 'e':
                if (strptime(optarg, "%Y-%m-%e", &tinfo) == NULL) {
                    printf("Incorrect time format. Supported format is YYYY-MM-DD\n"); // error
                    free_cluster_conf(&my_conf);
                    exit(1);
                }
                query_adds.end_time = mktime(&tinfo);
                break;
            case 'F':
                strncpy(format, optarg, sizeof(format));
                break;
            case 'h':
                free_cluster_conf(&my_conf);
                usage(argv[0]);
                break;
        }
    }

    if (verbose)
        printf("Limit set to %d\n", query_adds.limit);
    if (strlen(format) < 1) {
        if (is_loops)
            strcpy(format, "jxnXpgTCstrimG");
        else if (full_length)
            strcpy(format, "jxuapnrtgCseG");
        else
            strcpy(format, "jxuapNrtgCseG");
    }

    if (file_name != NULL)
        read_applications_from_files(file_name, format);
    else if (is_events)
        read_events(user, &query_adds);
    else if (is_loops)
        read_loops(user, &query_adds, format);
    else
        read_applications_from_database(user, &query_adds, format);

#if COLORS
    printf("%s", COL_CLR);
#endif

    free_cluster_conf(&my_conf);
    exit(0);
}
