/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <common/database/db_helper.h>
#include <common/output/verbose.h>
#include <common/string_enhanced.h>
#include <common/types/application.h>
#include <common/types/configuration/cluster_conf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *paint[6] = {STR_RED, STR_GRE, STR_YLW, STR_BLU, STR_MGT, STR_CYA};
static unsigned int opt_p;
static unsigned int opt_c;
static unsigned int opt_o;
static unsigned int opt_g;

void usage(int argc, char *argv[])
{
    int i = 0;

    if (argc < 2) {
        verbose(0, "Usage: %s node.id [OPTIONS]\n", argv[0]);
        verbose(0, "  The node.id of the node to display the information.Use \"all\" to show all the nodes");
        verbose(0, "\nOptions:");
        verbose(0, "\t-P <num>\tPrints the output with a different color,");
        verbose(0, "\t\tcan be used when displaying different batch of");
        verbose(0, "\t\tapplications by script.");
        verbose(0, "\t-C\tShows other jobs of the same application,");
        verbose(0, "\t\tnode, policy and number of processes.");
        verbose(0, "\t-G\tinclude GPU data (possible if GPU is used).'");
        verbose(0, "\t-O\tgenerate csv file with name 'learning_show.<node_id>.csv'");
        exit(1);
    }

    for (i = 2; i < argc; ++i) {
        if (!opt_c)
            opt_c = (strcmp(argv[i], "-C") == 0);
        if (!opt_o)
            opt_o = (strcmp(argv[i], "-O") == 0);
        if (!opt_g)
            opt_g = (strcmp(argv[i], "-G") == 0);
        if (!opt_p) {
            opt_p = (strcmp(argv[i], "-P") == 0);
            if (opt_p) {
                opt_p = atoi(argv[i + 1]) % 6;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char buffer[256];
    char *node_name = NULL;
    cluster_conf_t my_conf;
    application_t *apps;
    // int total_apps = 0; Not used.
    int num_apps = 0;
    int i;
    double VPI;
    unsigned int use_gpu;
    double gpup;
    double gpuu;
    double gpum;

    //
    usage(argc, argv);

    //
    node_name = argv[1];

    // CSV output
    char csv_name[SZ_PATH];
    FILE *fd;

    if (strcmp(argv[1], "all") == 0)
        node_name = NULL;

    if (get_ear_conf_path(buffer) == EAR_ERROR) {
        printf("ERROR while getting ear.conf path\n");
        exit(0);
    } else {
        printf("Reading from %s\n", buffer);
    }

    //
    read_cluster_conf(buffer, &my_conf);
    strcpy(my_conf.database.user, my_conf.database.user_commands);
    strcpy(my_conf.database.pass, my_conf.database.pass_commands);
    init_db_helper(&my_conf.database);

    num_apps = db_read_applications(&apps, 1, 50, node_name);

    strcpy(csv_name, "learning_show.");
    strcat(csv_name, node_name);
    strcat(csv_name, ".csv");
    fd = fopen(csv_name, "w");
    if (fd == NULL) {
        verbose(0, "Can't open the outpout file %s, csv will not be generated", csv_name);
        opt_o = 0;
    }

#if USE_GPUS
    if (opt_g) {
        use_gpu = 1;
    } else {
        use_gpu = 0;
    }
#else
    use_gpu = 0;
#endif

    //
    if (!use_gpu) {
        if (!opt_c) {
            tprintf_init(fdout, STR_MODE_COL, "15 10 10 17 10 10 8 10 12 12 8 8 8 8 10 8");
            tprintf("Node name||JID||StepID||App name||Def. F.||Avg. "
                    "F.||Seconds||DC_power||DRAM_power||PCK_power||GBS||CPI||TPI||Gflops||MPI_perc||VPI");
            tprintf("---------||---||------||--------||-------||-------||-------||--------||----------||---------||---|"
                    "|---||---||------||--------||---");
        } else {
            verbose(0, "Node name;JID;StepID;App name;Def. F.;Avg. "
                       "F.;Seconds;DC_power;DRAM_power;PCK_power;GBS;CPI;TPI;Gflops;MPI_perc;VPI");
        }
    } else {
        if (!opt_c) {
            tprintf_init(fdout, STR_MODE_COL, "15 10 10 17 10 10 8 10 12 12 8 8 8 8 10 8 10 10 10");
            tprintf("Node name||JID||StepID||App name||Def. F.||Avg. "
                    "F.||Seconds||DC_power||DRAM_power||PCK_power||GBS||CPI||TPI||Gflops||MPI_perc||VPI||GPU_power||"
                    "GPU_util||GPU_mem_util");
            tprintf("---------||---||------||--------||-------||-------||-------||--------||----------||---------||---|"
                    "|---||---||------||--------||---||---------||--------||------------");
        } else {
            verbose(0, "Node name;JID;StepID;App name;Def. F.;Avg. "
                       "F.;Seconds;DC_power;DRAM_power;PCK_power;GBS;CPI;TPI;Gflops;MPI_perc;VPI;GPU_power;GPU_power;"
                       "GPU_util;GPU_mem_util");
        }
    }

    // to store results on csv file
    if (opt_o) {
        // header
        if (!use_gpu) {
            fprintf(fd, "node_id\tjob_id\tstep_id\tapp_id\tdef_f\tavg_f\ttime\tDC_power\tDRAM_power\tPCK_"
                        "power\tGBS\tCPI\tTPI\tGflops\tMPI_perc\tVPI\n");
        } else {
            fprintf(fd, "node_id\tjob_id\tstep_id\tapp_id\tdef_f\tavg_f\ttime\tDC_power\tDRAM_power\tPCK_"
                        "power\tGBS\tCPI\tTPI\tGflops\tMPI_perc\tVPI\tGPU_power\tGPU_power\tGPU_util;GPU_mem_util\n");
        }
    }

    while (num_apps > 0) {
        // total_apps += num_apps; Not used.
        for (i = 0; i < num_apps; i++) {
            if (strcmp(buffer, apps[i].node_id) != 0) {
                strcpy(buffer, apps[i].node_id);
            }

            if (strlen(apps[i].node_id) > 15) {
                apps[i].node_id[15] = '\0';
            }

#if USE_GPUS
            if (use_gpu) {
                gpup = 0.0;
                gpuu = 0.0;
                gpum = 0.0;
                for (uint gpuid = 0; gpuid < apps[i].signature.gpu_sig.num_gpus; gpuid++) {
                    gpup += apps[i].signature.gpu_sig.gpu_data[gpuid].GPU_power;
                    gpuu += apps[i].signature.gpu_sig.gpu_data[gpuid].GPU_util;
                    gpum += apps[i].signature.gpu_sig.gpu_data[gpuid].GPU_mem_util;
                }
                gpuu = gpuu / apps[i].signature.gpu_sig.num_gpus;
                gpum = gpum / apps[i].signature.gpu_sig.num_gpus;
            }
#endif

            compute_sig_vpi(&VPI, &apps[i].signature);
            if (!use_gpu) {
                if (opt_c) {
                    verbose(
                        0,
                        "%s;%lu;%lu;%s;%lu;%lu;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf",
                        apps[i].node_id, apps[i].job.id, apps[i].job.step_id, apps[i].job.app_id, apps[i].job.def_f,
                        apps[i].signature.avg_f, apps[i].signature.time, apps[i].signature.DC_power,
                        apps[i].signature.DRAM_power, apps[i].signature.PCK_power, apps[i].signature.GBS,
                        apps[i].signature.CPI, apps[i].signature.TPI, apps[i].signature.Gflops,
                        apps[i].signature.perc_MPI, VPI);
                } else {
                    tprintf("%s%s||%lu||%lu||%s||%lu||%lu||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0."
                            "2lf||%0.2lf||%0.2lf",
                            paint[opt_p], apps[i].node_id, apps[i].job.id, apps[i].job.step_id, apps[i].job.app_id,
                            apps[i].job.def_f, apps[i].signature.avg_f, apps[i].signature.time,
                            apps[i].signature.DC_power, apps[i].signature.DRAM_power, apps[i].signature.PCK_power,
                            apps[i].signature.GBS, apps[i].signature.CPI, apps[i].signature.TPI,
                            apps[i].signature.Gflops, apps[i].signature.perc_MPI, VPI);
                }
            } else {
                if (opt_c) {
                    verbose(0,
                            "%s;%lu;%lu;%s;%lu;%lu;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0.2lf;%0."
                            "2lf;%0.2lf;%0.2lf;%0.2lf",
                            apps[i].node_id, apps[i].job.id, apps[i].job.step_id, apps[i].job.app_id, apps[i].job.def_f,
                            apps[i].signature.avg_f, apps[i].signature.time, apps[i].signature.DC_power,
                            apps[i].signature.DRAM_power, apps[i].signature.PCK_power, apps[i].signature.GBS,
                            apps[i].signature.CPI, apps[i].signature.TPI, apps[i].signature.Gflops,
                            apps[i].signature.perc_MPI, VPI, gpup, gpuu, gpum);
                } else {
                    tprintf("%s%s||%lu||%lu||%s||%lu||%lu||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0."
                            "2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf||%0.2lf",
                            paint[opt_p], apps[i].node_id, apps[i].job.id, apps[i].job.step_id, apps[i].job.app_id,
                            apps[i].job.def_f, apps[i].signature.avg_f, apps[i].signature.time,
                            apps[i].signature.DC_power, apps[i].signature.DRAM_power, apps[i].signature.PCK_power,
                            apps[i].signature.GBS, apps[i].signature.CPI, apps[i].signature.TPI,
                            apps[i].signature.Gflops, apps[i].signature.perc_MPI, VPI, gpup, gpuu, gpum);
                }
            }

            if (opt_o) {
                if (!use_gpu) {
                    fprintf(fd,
                            "%s\t%lu\t%lu\t%s\t%lu\t%lu\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0."
                            "4lf\t%0.4lf\t%0.4lf\n",
                            apps[i].node_id, apps[i].job.id, apps[i].job.step_id, apps[i].job.app_id, apps[i].job.def_f,
                            apps[i].signature.avg_f, apps[i].signature.time, apps[i].signature.DC_power,
                            apps[i].signature.DRAM_power, apps[i].signature.PCK_power, apps[i].signature.GBS,
                            apps[i].signature.CPI, apps[i].signature.TPI, apps[i].signature.Gflops,
                            apps[i].signature.perc_MPI, VPI);
                } else {
                    fprintf(fd,
                            "%s\t%lu\t%lu\t%s\t%lu\t%lu\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0."
                            "4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\t%0.4lf\n",
                            apps[i].node_id, apps[i].job.id, apps[i].job.step_id, apps[i].job.app_id, apps[i].job.def_f,
                            apps[i].signature.avg_f, apps[i].signature.time, apps[i].signature.DC_power,
                            apps[i].signature.DRAM_power, apps[i].signature.PCK_power, apps[i].signature.GBS,
                            apps[i].signature.CPI, apps[i].signature.TPI, apps[i].signature.Gflops,
                            apps[i].signature.perc_MPI, VPI, gpup, gpuu, gpum);
                }
            }
        }
        free(apps);
        num_apps = db_read_applications(&apps, 1, 50, node_name);
    }

    if (opt_o) {
        verbose(0, "Results stored in: %s", csv_name);
    }
    fclose(fd);

    return 0;
}
