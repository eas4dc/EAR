/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
*/

#define STANDARD_NODENAME_LENGTH    25
#define APP_TEXT_FILE_FIELDS        22

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/system/user.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/application.h>
#include <common/types/version.h>

#if USE_DB
#include <common/states.h>
#include <common/database/db_helper.h>
#include <common/database/mysql_io_functions.h>
#include <common/types/configuration/cluster_conf.h>
cluster_conf_t my_conf;
#endif


int full_length = 0;
int verbose = 0;
int query_filters = 0;
int all_mpi = 0;
int avx = 0;
char csv_path[256] = "";

void usage(char *app)
{
    #if DB_MYSQL
    printf("Usage: %s [Optional parameters]\n"\
"\tOptional parameters: \n" \
"\t\t-h\tdisplays this message\n"\
"\t\t-v\tdisplays current EAR version\n" \
"\t\t-b\tverbose mode for debugging purposes\n" \
"\t\t-u\tspecifies the user whose applications will be retrieved. Only available to privileged users. [default: all users]\n" \
"\t\t-j\tspecifies the job id and step id to retrieve with the format [jobid.stepid] or the format [jobid1,jobid2,...,jobid_n].\n" \
"\t\t\t\tA user can only retrieve its own jobs unless said user is privileged. [default: all jobs]\n"\
"\t\t-c\tspecifies the file where the output will be stored in CSV format. [default: no file]\n" \
"\t\t-t\tspecifies the energy_tag of the jobs that will be retrieved. [default: all tags].\n" \
"\t\t-l\tshows the information for each node for each job instead of the global statistics for said job.\n" \
"\t\t-x\tshows the last EAR events. Nodes, job ids, and step ids can be specified as if were showing job information.\n" \
"\t\t-n\tspecifies the number of jobs to be shown, starting from the most recent one. [default: 20][to get all jobs use -n all]\n" \
"", app);
    printf("\t\t-f\tspecifies the file where the user-database can be found. If this option is used, the information will be read from the file and not the database.\n");
    #endif
	exit(0);
}

void read_from_files2(int job_id, int step_id, char verbose, char *file_path)
{
    int i, num_nodes;
	char **nodes;

    char nodelist_file_path[256], *nodelog_file_path;
    char line_buf[256];
    FILE *nodelist_file, *node_file;
    
    
    strcpy(nodelist_file_path, EAR_INSTALL_PATH);
    strcat(nodelist_file_path, "/etc/sysconf/nodelist.conf");

    nodelist_file = fopen(nodelist_file_path, "r");
    if (nodelist_file == NULL)
    {
        printf("Error opening nodelist file at \"$EAR_INSTALL_PATH/etc/sysconf/nodelist.conf\".\n");
        printf("Filepath: %s\n", nodelist_file_path);
        exit(1);
    }
    if (verbose) printf("Nodelist found at: %s\n", nodelist_file_path);
    
    //count the total number of nodes
    if (verbose) printf("Counting nodes...\n");
    num_nodes = 0;
    while(fscanf(nodelist_file, "%s\n", line_buf) > 0) {
        num_nodes += 1;
    }
    rewind(nodelist_file);
    
    if (num_nodes == 0)
    {
        verbose(0, "No nodes found."); //error
        return;
    }
    
    if (verbose) printf("Found %d nodes.\n", num_nodes);

    //initialize memory for each 
    nodes = calloc(num_nodes, sizeof(char*));
    for (i = 0; i < num_nodes; i++)
    {
        nodes[i] = malloc(sizeof(char) * STANDARD_NODENAME_LENGTH);
        fscanf(nodelist_file, "%s", nodes[i]);
    }
    //all the information needed from nodelist_file has been read, it can be closed
    fclose(nodelist_file);


    char *nodename_extension = ".hpc.eu.lenovo.com.csv";
    char *nodename_prepend = file_path;
    nodelog_file_path = malloc(strlen(nodename_extension) + strlen(nodename_prepend) + STANDARD_NODENAME_LENGTH + 1);


    //allocate memory to hold all possible found jobs
    application_t **apps = (application_t**) malloc(sizeof(application_t*)*num_nodes);
    int jobs_counter = 0;

    //parse and filter job executions
    for (i = 0; i < num_nodes; i++)
    {
        sprintf(nodelog_file_path, "%s%s%s", nodename_prepend, nodes[i], nodename_extension);
        if (verbose) printf("Opening file for node %s (%s).\n", nodes[i], nodelog_file_path);
        node_file = fopen(nodelog_file_path, "r");

        if (node_file == NULL)
        {
            continue;
        }
        if (verbose) printf("File for node %s (%s) opened.\n", nodes[i], nodelog_file_path);

        //first line of each file is the header
        fscanf(node_file, "%s\n", line_buf);
        
        apps[jobs_counter] = (application_t*) malloc(sizeof(application_t));
        init_application(apps[jobs_counter]);
        if (verbose) printf("Checking node for signatures with %d job id.\n", job_id);
        while (scan_application_fd(node_file, apps[jobs_counter], 0) == APP_TEXT_FILE_FIELDS)
        {
            if (apps[jobs_counter]->job.id == job_id && apps[jobs_counter]->job.step_id == step_id)
            {
                if (verbose) printf("Found job_id %ld in file %s\n", apps[jobs_counter]->job.id, nodelog_file_path);
                jobs_counter++;
                break;
            }
        }

        fclose(node_file);
    }

    if (i == 1)
    {
        verbose(0, "No jobs were found with id: %u", job_id); //error

        free(nodelog_file_path);
        for (i = 0; i < num_nodes; i++)
            free(nodes[i]);
        free(nodes);
        return;
    }

    printf("Node information:\nJob_id \tNodename \t\t\tTime (secs) \tDC Power (Watts) \tEnergy (Joules) \tAvg_freq (GHz)\n");

    double avg_time, avg_power, total_energy, avg_f, avg_frequency;
    avg_frequency = 0;
    avg_time = 0;
    avg_power = 0;
    total_energy = 0;
    for (i = 0; i < jobs_counter; i++)
    {
        avg_f = (double) apps[i]->signature.avg_f/1000000;
        printf("%ld \t%s \t%.2lf \t\t%.2lf \t\t\t%.2lf \t\t%.2lf\n", 
                apps[i]->job.id, apps[i]->node_id, apps[i]->signature.time, apps[i]->signature. DC_power, 
		apps[i]->signature.DC_power * apps[i]->signature.time, avg_f);
        avg_frequency += avg_f;
        avg_time += apps[i]->signature.time;
        avg_power += apps[i]->signature. DC_power;
        total_energy += apps[i]->signature.time * apps[i]->signature.DC_power;

    }

    avg_frequency /= jobs_counter;
    avg_time /= jobs_counter;
    avg_power /= jobs_counter;

    printf("\nApplication average:\nJob_id \tTime (secs.) \tDC Power (Watts) \tAccumulated Energy (Joules) \tAvg_freq (GHz)\n");

    printf("%d.%d \t%.2lf \t\t%.2lf \t\t\t%.2lf \t\t\t%.2lf\n", 
            job_id, step_id, avg_time, avg_power, total_energy, avg_frequency);


    for (i = 0; i < jobs_counter; i++)
        free(apps[i]);
    free(apps);

    free(nodelog_file_path);


    //freeing variables
    for (i = 0; i < num_nodes; i++)
        free(nodes[i]);
    free(nodes);

}

void print_full_apps(application_t *apps, int num_apps)
{
    int i = 0;
    double avg_f, vpi;

    printf("%-6s-%-7s\t %-10s %-15s %-20s %-10s %-10s %-10s %-10s %-10s %-10s %-20s %-7s %-10s\n",
            "JOB ID", "STEP ID", "NODE ID", "USER ID", "APPLICATION ID", "FREQ", "TIME",
            "POWER", "GBS", "CPI", "ENERGY", "START TIME", "VPI(%)", "MAX POWER");

    for (i = 0; i < num_apps; i++)
    {
        if (strlen(apps[i].job.app_id) > 30)
            if (strchr(apps[i].job.app_id, '/') != NULL)
                strcpy(apps[i].job.app_id, strrchr(apps[i].job.app_id, '/')+1);

        time_t start = apps[i].job.start_time;
        char buff[25];
        strftime(buff, 25, "%Y-%m-%d %H:%M:%S", localtime(&start));
        if (apps[i].is_mpi && !all_mpi)
        {
            avg_f = (double) apps[i].signature.avg_f/1000000;
            compute_vpi(&vpi, &apps[i].signature);
            if (apps[i].job.step_id != 4294967294)
            {
                printf("%8lu-%-3lu\t %-10s %-15s %-20s %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-20s %-7.3lf %-10.2lf\n",
                    apps[i].job.id, apps[i].job.step_id, apps[i].node_id, apps[i].job.user_id, apps[i].job.app_id, 
                    avg_f, apps[i].signature.time, apps[i].signature.DC_power, apps[i].signature.GBS, apps[i].signature.CPI, 
                    apps[i].signature.time * apps[i].signature.DC_power, buff, vpi*100, apps[i].power_sig.max_DC_power);
            }
            else
            {
                printf("%8lu-%-6s\t %-10s %-15s %-20s %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-10.2lf %-20s %-7.3lf %-10.2lf\n",
                    apps[i].job.id, "sbatch", apps[i].node_id, apps[i].job.user_id, apps[i].job.app_id, 
                    avg_f, apps[i].signature.time, apps[i].signature.DC_power, apps[i].signature.GBS, apps[i].signature.CPI, 
                    apps[i].signature.time * apps[i].signature.DC_power, buff, vpi*100, apps[i].power_sig.max_DC_power);
            }
        }
        else
        {
            avg_f = (double) apps[i].power_sig.avg_f/1000000;
            if (apps[i].job.step_id != 4294967294)
            {
                printf("%8lu-%-3lu\t %-10s %-15s %-20s %-10.2lf %-10.2lf %-10.2lf %-10s %-10s %-10.2lf %-20s %-7s %-10.2lf\n",
                    apps[i].job.id, apps[i].job.step_id, apps[i].node_id, apps[i].job.user_id, apps[i].job.app_id, 
                    avg_f, apps[i].power_sig.time, apps[i].power_sig.DC_power, "NO-EARL", "NO-EARL", 
                    apps[i].power_sig.time * apps[i].power_sig.DC_power, buff, "NO-EARL", apps[i].power_sig.max_DC_power);
            }
            else
            {
                printf("%8lu-%-6s\t %-10s %-15s %-20s %-10.2lf %-10.2lf %-10.2lf %-10s %-10s %-10.2lf %-20s %-7s %-10.2lf\n",
                    apps[i].job.id, "sbatch", apps[i].node_id, apps[i].job.user_id, apps[i].job.app_id, 
                    avg_f, apps[i].power_sig.time, apps[i].power_sig.DC_power, "NO-EARL", "NO-EARL", 
                    apps[i].power_sig.time * apps[i].power_sig.DC_power, buff, "NO-EARL", apps[i].power_sig.max_DC_power);

            }
        }

    }
}

void print_short_apps(application_t *apps, int num_apps, int fd)
{
    uint current_job_id = -1;
    uint current_step_id = -1;
    uint current_is_mpi = 0;
    uint current_apps = 0;

    int i = 0;
    double avg_time, avg_power, total_energy, avg_f, avg_frequency, avg_GBS, avg_CPI, avg_VPI, gflops_watt, max_dc_power;
    avg_frequency = 0;
    avg_time = 0;
    avg_power = 0;
    total_energy = 0;
    avg_CPI = 0;
    avg_GBS = 0;
		avg_VPI=0;
    gflops_watt = 0;
    max_dc_power = 0;
    char is_sbatch = 0;
    char curr_policy[3];
    char missing_apps = -1;
    char header_format[256];
    char line_format[256];
    char mpi_line_format[256];
    char sbatch_line_format[256];
    char mpi_sbatch_line_format[256];
    double vpi;

    if (fd == STDOUT_FILENO)
    {
        if (avx)
        {
            strcpy(header_format, "%6s-%-7s\t %-10s %-20s %-6s %-7s %-10s %-10s %-14s %-10s %-10s %-14s %-14s %-14s %-10s\n");
            strcpy(line_format, "%8u-%-3u\t %-10s %-20s %-6s %-7u %-10.2lf %-10.2lf %-14.2lf %-10.2lf %-10.2lf %-14.2lf %-14.4lf %-14.3lf %-10.2lf\n");
            strcpy(mpi_line_format, "%8u-%-3u\t %-10s %-20s %-6s %-7u %-10.2lf %-10.2lf %-14.2lf %-10s %-10s %-14.2lf %-14s %-14s %-10.2lf\n");
            strcpy(sbatch_line_format, "%8u-%-6s\t %-10s %-20s %-6s %-7u %-10.2lf %-10.2lf %-14.2lf %-10.2lf %-10.2lf %-14.2lf %-14.4lf %-14.3lf %-10.2lf\n");
            strcpy(mpi_sbatch_line_format, "%8u-%-6s\t %-10s %-20s %-6s %-7u %-10.2lf %-10.2lf %-14.2lf %-10s %-10s %-14.2lf %-14s %-14s %-10.2lf\n");
        }
        else
        {
            strcpy(header_format, "%6s-%-7s\t %-10s %-20s %-6s %-7s %-10s %-10s %-14s %-10s %-10s %-14s %-14s %-10s\n");
            strcpy(line_format, "%8u-%-3u\t %-10s %-20s %-6s %-7u %-10.2lf %-10.2lf %-14.2lf %-10.2lf %-10.2lf %-14.2lf %-14.4lf %-10.2lf\n");
            strcpy(mpi_line_format, "%8u-%-3u\t %-10s %-20s %-6s %-7u %-10.2lf %-10.2lf %-14.2lf %-10s %-10s %-14.2lf %-14s %-10.2lf\n");
            strcpy(sbatch_line_format, "%8u-%-6s\t %-10s %-20s %-6s %-7u %-10.2lf %-10.2lf %-14.2lf %-10.2lf %-10.2lf %-14.2lf %-14.4lf %-10.2lf\n");
            strcpy(mpi_sbatch_line_format, "%8u-%-6s\t %-10s %-20s %-6s %-7u %-10.2lf %-10.2lf %-14.2lf %-10s %-10s %-14.2lf %-14s %-10.2lf\n");
        }
    }
    else
    {
        if (avx)
        {
            strcpy(header_format, "%s-%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s\n");
            strcpy(line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n");
            strcpy(mpi_line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%lf;%s;%s;%lf;%s;%s;%lf\n");
            strcpy(sbatch_line_format, "%u-%s;%s;%s;%s;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n");
            strcpy(mpi_sbatch_line_format, "%u-%s;%s;%s;%s;%u;%lf;%lf;%lf;%s;%s;%lf;%s;%s;%lf\n");
        }
        else
        {
            strcpy(header_format, "%s-%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s\n");
            strcpy(line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n");
            strcpy(mpi_line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%lf;%s;%s;%lf;%s;%lf\n");
            strcpy(sbatch_line_format, "%u-%u;%s;%s;%s;%u;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n");
            strcpy(mpi_sbatch_line_format, "%u-%s;%s;%s;%s;%u;%lf;%lf;%lf;%s;%s;%lf;%s;%lf\n");
        }
    }
    if (avx)
        dprintf(fd, header_format,
            "JOB", "STEP", "USER", "APPLICATION", "POLICY", "NODES#", "FREQ(GHz)", "TIME(s)",
            "POWER(Watts)", "GBS", "CPI", "ENERGY(J)", "GFLOPS/WATT", "VPI(%)", "MAX POWER(W)");

    else
        dprintf(fd, header_format,
            "JOB", "STEP", "USER", "APPLICATION", "POLICY", "NODES#", "FREQ(GHz)", "TIME(s)",
            "POWER(Watts)", "GBS", "CPI", "ENERGY(J)", "GFLOPS/WATT", "MAX POWER(W)");

    for (i = 0; i < num_apps; i ++)
    {
        if (apps[i].job.id == current_job_id && apps[i].job.step_id == current_step_id)
        {
            if (current_is_mpi && !all_mpi)
            {
                compute_vpi(&vpi, &apps[i].signature); 
                avg_VPI += vpi;
                avg_f = (double) apps[i].signature.avg_f/1000000;
                avg_frequency += avg_f;
                avg_time += apps[i].signature.time;
                avg_power += apps[i].signature.DC_power;
                avg_GBS += apps[i].signature.GBS;
                avg_CPI += apps[i].signature.CPI;
                max_dc_power = max_dc_power < apps[i].power_sig.max_DC_power ? apps[i].power_sig.max_DC_power : max_dc_power;
                gflops_watt += apps[i].signature.Gflops;
                total_energy += apps[i].signature.time * apps[i].signature.DC_power;
                current_apps++;
            }
            else
            {
                avg_f = (double) apps[i].power_sig.avg_f/1000000;
                avg_frequency += avg_f;
                avg_time += apps[i].power_sig.time;
                avg_power += apps[i].power_sig.DC_power;
                max_dc_power = max_dc_power < apps[i].power_sig.max_DC_power ? apps[i].power_sig.max_DC_power : max_dc_power;
                total_energy += apps[i].power_sig.time * apps[i].power_sig.DC_power;
                current_apps++;
            }
        }
        
        else
        {
            //to print: job_id.step_id \t user_id si root \t app_name \t num_nodes
            int idx = (i > 0) ? i - 1: 0;
            if (strlen(apps[idx].job.app_id) > 30)
            {
                char *token = strrchr(apps[i-1].job.app_id, '/');
                if (token != NULL) 
                {
                    token++;
                    strcpy(apps[idx].job.app_id, token);
                }
            }


            if (current_is_mpi && !all_mpi)
            {
                
                gflops_watt /= avg_power;
                avg_frequency /= current_apps;
                avg_time /= current_apps;
                avg_power /= current_apps;
                avg_GBS /= current_apps;
                avg_CPI /= current_apps;
                avg_VPI /= current_apps;
                

				if (avg_VPI == 0)
					avg_VPI = -1;
                else avg_VPI*=100;

                if (avg_frequency > 0 && avg_time > 0 && total_energy > 0)
                {
                        if (!is_sbatch)
                        {
                            if (avx)
                                dprintf(fd, line_format,
                                    current_job_id, current_step_id, apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
                                    avg_frequency, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_VPI, max_dc_power);
        
                            else
                                dprintf(fd, line_format,
                                    current_job_id, current_step_id, apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
                                    avg_frequency, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, max_dc_power);
                        }
                        else
                        {
                            if (avx)
                                dprintf(fd, sbatch_line_format,
                                    current_job_id, "sbatch", apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
                                    avg_frequency, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_VPI, max_dc_power);
    
                            else
                                dprintf(fd, sbatch_line_format,
                                    current_job_id, "sbatch", apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
                                    avg_frequency, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, max_dc_power);
                        }
                }
                else
                    missing_apps++;
            }
            else
            {
                avg_frequency /= current_apps;
                avg_time /= current_apps;
                avg_power /= current_apps;
                if (avg_frequency > 0 && avg_time > 0 && total_energy > 0)
                {
                        if (!is_sbatch)
                        {
                            if (avx)
                                dprintf(fd, mpi_line_format,
                                    current_job_id, current_step_id, apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
                                    avg_frequency, avg_time, avg_power, "NO-EARL", "NO-EARL", total_energy, "NO-EARL", "NO-EARL", max_dc_power);
                            else
                                dprintf(fd, mpi_line_format,
                                    current_job_id, current_step_id, apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
                                    avg_frequency, avg_time, avg_power, "NO-EARL", "NO-EARL", total_energy, "NO-EARL", max_dc_power);
                        }
                        else
                        {
                            if (avx)
                                dprintf(fd, mpi_sbatch_line_format,
                                    current_job_id, "sbatch", apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
                                    avg_frequency, avg_time, avg_power, "NO-EARL", "NO-EARL", total_energy, "NO-EARL", "NO-EARL", max_dc_power);
                            else
                                dprintf(fd, mpi_sbatch_line_format,
                                    current_job_id, "sbatch", apps[idx].job.user_id, apps[idx].job.app_id, curr_policy, current_apps, 
                                    avg_frequency, avg_time, avg_power, "NO-EARL", "NO-EARL", total_energy, "NO-EARL", max_dc_power);
                        }
                }
                else
                    missing_apps++;
            }
            if (apps[i].job.id != current_job_id || apps[i].job.step_id != current_step_id)
            {
                get_short_policy(curr_policy, apps[i].job.policy, &my_conf);
                current_job_id = apps[i].job.id;
                current_step_id = apps[i].job.step_id;
                current_is_mpi = apps[i].is_mpi;
                current_apps = 0;
                avg_frequency = 0;
                avg_time = 0;
                avg_power = 0;
                avg_GBS = 0;
                avg_CPI = 0;
                avg_VPI = 0;
                total_energy = 0;
                gflops_watt = 0;
                max_dc_power = 0;
                i--; //go back to current app
                is_sbatch = (current_step_id == 4294967294) ? 1 : 0;
            }
        }
    }
    if (num_apps > 0)
    {
        if (strlen(apps[i-1].job.app_id) > 30)
            if (strchr(apps[i-1].job.app_id, '/') != NULL)
                strcpy(apps[i-1].job.app_id, strrchr(apps[i-1].job.app_id, '/')+1);

        if (apps[i-1].is_mpi)
        {
            gflops_watt /= avg_power;
            avg_frequency /= current_apps;
            avg_time /= current_apps;
            avg_power /= current_apps;
            avg_GBS /= current_apps;
            avg_CPI /= current_apps;
            avg_VPI /= current_apps;

			if (avg_VPI == 0)
				avg_VPI = -1;
            else avg_VPI *=100;
            if (avg_frequency > 0 && avg_time > 0 && total_energy > 0)
            {
                    if (!is_sbatch)
                    {
                        if (avx)
                            dprintf(fd, line_format,
                                current_job_id, current_step_id, apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
                                avg_frequency, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_VPI, max_dc_power);
                        else
                            dprintf(fd, line_format,
                                current_job_id, current_step_id, apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
                                avg_frequency, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, max_dc_power);
                    }
                    else
                    {
                        if (avx)
                            dprintf(fd, sbatch_line_format,
                                current_job_id, "sbatch", apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
                                avg_frequency, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, avg_VPI, max_dc_power);
                        else
                            dprintf(fd, sbatch_line_format,
                                current_job_id, "sbatch", apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
                                avg_frequency, avg_time, avg_power, avg_GBS, avg_CPI, total_energy, gflops_watt, max_dc_power);
                    }
            }
            else
                missing_apps++;
        }
        else
        {
            avg_frequency /= current_apps;
            avg_time /= current_apps;
            avg_power /= current_apps;
            if (avg_frequency > 0 && avg_time > 0 && total_energy > 0)
            {
                if (!is_sbatch)
                {
                    if (avx)
                        dprintf(fd, mpi_line_format,
                            current_job_id, current_step_id, apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
                            avg_frequency, avg_time, avg_power, "NO-EARL", "NO-EARL", total_energy, "NO-EARL", "NO-EARL", max_dc_power);
                    else
                        dprintf(fd, mpi_line_format,
                            current_job_id, current_step_id, apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
                            avg_frequency, avg_time, avg_power, "NO-EARL", "NO-EARL", total_energy, "NO-EARL", max_dc_power);
                }
                else
                {
                    if (avx)
                        dprintf(fd, mpi_sbatch_line_format,
                            current_job_id, "sbatch", apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
                            avg_frequency, avg_time, avg_power, "NO-EARL", "NO-EARL", total_energy, "NO-EARL", "NO-EARL", max_dc_power);
                    else
                        dprintf(fd, mpi_sbatch_line_format,
                            current_job_id, "sbatch", apps[i-1].job.user_id, apps[i-1].job.app_id, curr_policy, current_apps, 
                            avg_frequency, avg_time, avg_power, "NO-EARL", "NO-EARL", total_energy, "NO-EARL", max_dc_power);
                }
            }
            else
                missing_apps++;

        }
    }
    if (missing_apps > 0 && fd==STDOUT_FILENO)
        printf("\nSome jobs are not being shown because either their avg. frequency, time or total energy were 0. To see those jobs run with -l option.\n");

    if (avx)
        printf("\nA -1.0 in the VPI column means an absolute 0 in that field. This is done to distinguish from very low values.\n");

    printf("\n");
}

void add_string_filter(char *query, char *addition, char *value)
{
    if (query_filters < 1)
        strcat(query, " WHERE ");
    else
        strcat(query, " AND ");

    strcat(query, addition);
    strcat(query, "=");
    strcat(query, "'");
    strcat(query, value);
    strcat(query, "'");
//    sprintf(query, query, value);
    query_filters ++;
}

void add_int_filter(char *query, char *addition, int value)
{
    char query_tmp[512];
    strcpy(query_tmp, query);
    if (query_filters < 1)
        strcat(query_tmp, " WHERE ");
    else
        strcat(query_tmp, " AND ");
    
    strcat(query_tmp, addition);
    strcat(query_tmp, "=");
    strcat(query_tmp, "%llu");
    sprintf(query, query_tmp, value);
    query_filters ++;
}

void add_int_comp_filter(char *query, char *addition, int value, char greater_than)
{
    char query_tmp[512];
    strcpy(query_tmp, query);
    if (query_filters < 1)
        strcat(query_tmp, " WHERE ");
    else
        strcat(query_tmp, " AND ");
    
    strcat(query_tmp, addition);
    if (greater_than)
        strcat(query_tmp, ">");
    else
        strcat(query_tmp, "<");
    strcat(query_tmp, "%llu");
    sprintf(query, query_tmp, value);
    query_filters ++;
}

void add_int_list_filter(char *query, char *addition, char *value)
{
    if (query_filters < 1)
        strcat(query, " WHERE ");
    else
        strcat(query, " AND ");

    strcat(query, addition);
    strcat(query, " IN ");
    strcat(query, "(");
    strcat(query, value);
    strcat(query, ")");
//    sprintf(query, query, value);
    query_filters ++;
}

#define ENERGY_POLICY_NEW_FREQ	0
#define GLOBAL_ENERGY_POLICY	1
#define ENERGY_POLICY_FAILS		2
#define DYNAIS_OFF				3

void print_event_type(int type)
{
    switch(type)
    {
        case 0:
            printf("%15s ", "NEW FREQ");
            break;
        case 1:
            printf("%15s ", "GLOBAL POLICY");
            break;
        case 2:
            printf("%15s ", "POLICY FAILS");
            break;
        case 3:
            printf("%15s ", "DYNAIS OFF");
            break;
        default:
            if (verbose)
                printf("%12s(%d) ","UNKNOWN", type);
            else
                printf("%15s ", "UNKNOWN CODE");
            break;
    }
}

#if DB_MYSQL
void mysql_print_events(MYSQL_RES *result)
{

    int i;
    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    int has_records = 0;
    while ((row = mysql_fetch_row(result))!= NULL) 
    { 
        if (!has_records)
        {
            printf("%12s %20s %15s %8s %8s %20s %12s\n",
               "Event ID", "Timestamp", "Event type", "Job id", "Step id", "Freq.", "node_id");
            has_records = 1;
        }
        for(i = 0; i < num_fields; i++) {
            if (i == 2)
               print_event_type(atoi(row[i]));  
            else if (i == 1)
               printf("%20s ", row[i] ? row[i] : "NULL");
            else if (i == 4 || i == 3)
               printf("%8s ", row[i] ? row[i] : "NULL");
            else if (i == 5)
               printf("%20s ", row[i] ? row[i] : "NULL");
            else
               printf("%12s ", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }
    if (!has_records)
    {
        printf("There are no events with the specified properties.\n\n");
    }

}
#elif DB_PSQL
void postgresql_print_events(PGresult *res)
{
    int i, j, num_fields, has_records = 0;
    num_fields = PQnfields(res);

    for (i = 0; i < PQntuples(res); i++)
    {
        if (!has_records)
        {
            printf("%12s %22s %15s %8s %8s %20s %12s\n",
               "Event ID", "Timestamp", "Event type", "Job id", "Step id", "Freq.", "node_id");
            has_records = 1;
        }
        for (j = 0; j < num_fields; j++) {
            if (j == 2)
                print_event_type(atoi(PQgetvalue(res, i, j)));
            else if (i == 1)
               printf("%22s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
            else if (i == 4 || i == 3)
               printf("%8s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
            else if (i == 5)
               printf("%20s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
            else
               printf("%12s ", PQgetvalue(res, i, j) ? PQgetvalue(res, i, j) : "NULL");
        }
        printf("\n");
    }
}
#endif

#if DB_MYSQL
#define EVENTS_QUERY "SELECT id, FROM_UNIXTIME(timestamp), event_type, job_id, step_id, freq, node_id FROM Events"
#elif DB_PSQL
#define EVENTS_QUERY "SELECT id, to_timestamp(timestamp), event_type, job_id, step_id, freq, node_id FROM Events"
#endif

void read_events(char *user, int job_id, int limit, int step_id, char *job_ids) 
{
    char query[512];
    char subquery[128];

    if (strlen(my_conf.database.user_commands) < 1) 
    {
        verbose(0, "Warning: commands' user is not defined in ear.conf");
    }
    else
    {
        strcpy(my_conf.database.user, my_conf.database.user_commands);
        strcpy(my_conf.database.pass, my_conf.database.pass_commands);
    }
    init_db_helper(&my_conf.database);


    strcpy(query, EVENTS_QUERY);

    add_int_comp_filter(query, "event_type", 100, 0);
    if (job_id >= 0)
        add_int_filter(query, "job_id", job_id);
    else if (strlen(job_ids) > 0)
        add_int_list_filter(query, "job_id", job_ids);
    if (step_id >= 0)
        add_int_filter(query, "step_id", step_id);
    if (user != NULL)
        add_string_filter(query, "node_id", user);

    if (limit > 0)
    {
        sprintf(subquery, " ORDER BY timestamp desc LIMIT %d", limit);
        strcat(query, subquery);
    }

    if (verbose) printf("QUERY: %s\n", query);
  
#if DB_MYSQL
    MYSQL_RES *result = db_run_query_result(query);
#elif DB_PSQL
    PGresult *result = db_run_query_result(query);
#endif
  
    if (result == NULL) 
    {
        printf("Database error\n");
        return;
    }

#if DB_MYSQL
    mysql_print_events(result);
#elif DB_PSQL
    postgresql_print_events(result);
#endif
}


//select Applications.* from Applications join Jobs on job_id = id where Jobs.end_time in (select end_time from (select end_time from Jobs where user_id = "xjcorbalan" and id = 284360 order by end_time desc limit 25) as t1) order by Jobs.end_time desc;
//select Applications.* from Applications join Jobs on job_id=id where Jobs.user_id = "xjcorbalan" group by job_id order by Jobs.end_time desc limit 5;
#if USE_DB
void read_from_database(char *user, int job_id, int limit, int step_id, char *e_tag, char *job_ids) 
{
    int num_apps = 0;
    if (strlen(my_conf.database.user_commands) < 1) 
    {
        verbose(0, "Warning: commands' user is not defined in ear.conf");
    }
    else
    {
        strcpy(my_conf.database.user, my_conf.database.user_commands);
        strcpy(my_conf.database.pass, my_conf.database.pass_commands);
    }
    init_db_helper(&my_conf.database);

    set_signature_simple(my_conf.database.report_sig_detail);

    char subquery[256];
    char query[512];
    
    if (verbose) {
        verbose(0, "Preparing query statement");
    }
    
    sprintf(query, "SELECT Applications.* FROM Applications join Jobs on job_id=id and Applications.step_id = Jobs.step_id where Jobs.id in (select id from (select id, end_time from Jobs" );
    application_t *apps;
    if (job_id >= 0)
        add_int_filter(query, "id", job_id);
    else if (strlen(job_ids) > 0)
        add_int_list_filter(query, "id", job_ids);
    if (step_id >= 0)
        add_int_filter(query, "step_id", step_id);
    if (user != NULL)
        add_string_filter(query, "user_id", user);
    if (strlen(e_tag) > 0)
        add_string_filter(query, "e_tag", e_tag);

    if (limit > 0)
    {
        sprintf(subquery, " ORDER BY Jobs.end_time desc LIMIT %d", limit);
        strcat(query, subquery);
    }
    strcat(query, ") as t1");

    query_filters = 0;
    if (job_id >= 0)
        add_int_filter(query, "id", job_id);
    else if (strlen(job_ids) > 0)
        add_int_list_filter(query, "id", job_ids);
    if (step_id >= 0)
        add_int_filter(query, "Jobs.step_id", step_id);
    if (user != NULL)
        add_string_filter(query, "user_id", user);
    
    strcat(query, ") order by Jobs.id desc, Jobs.step_id desc, Jobs.end_time desc");

    if (verbose) {
        verbose(0, "Retrieving applications");
    }

    if (verbose) {
        verbose(0, "QUERY: %s", query);
    }

    num_apps = db_read_applications_query(&apps, query);

    if (verbose) {
        verbose(0, "Finalized retrieving applications");
    }

    if (num_apps == EAR_MYSQL_ERROR)
    {
        exit(1); //error
    }

    if (num_apps < 1)
    {
        printf("No jobs found.\n");
        return;
    }


    int i = 0;    
    if (limit == 20 && strlen(csv_path) < 1)
        printf("\nBy default only the first 20 jobs are retrieved.\n\n");

    if (strlen(csv_path) < 1)
    {
    
        if (full_length) print_full_apps(apps, num_apps);
        else print_short_apps(apps, num_apps, STDOUT_FILENO);
    }

    else
    {
        if (full_length)
            for (i = 0; i < num_apps; i++)
                append_application_text_file(csv_path, &apps[i], 0);
        else
        {
            int fd = open(csv_path, O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR|S_IWUSR|S_IRGRP);
            print_short_apps(apps, num_apps, fd);
            close(fd);
        }
        printf("Successfully written applications to csv. Only applications with EARL will have its information properly written.\n");
    }

    free(apps);

}
#endif

void read_from_files(char *path, char *user, int job_id, int limit, int step_id)
{
    application_t *apps;
    int num_apps = read_application_text_file(path, &apps, 0);
    if (full_length) print_full_apps(apps, num_apps);
    else print_short_apps(apps, num_apps, STDOUT_FILENO);
}

int main(int argc, char *argv[])
{
    int job_id = -1;
    int step_id = -1;
    int limit = 20;
    char is_events = 0;
    int opt;
    char path_name[256];
    char *file_name = NULL;
    char e_tag[64] = "";
    char job_ids[256] = "";

    if (get_ear_conf_path(path_name)==EAR_ERROR){
        printf("Error getting ear.conf path\n");
        exit(1);
    }

    if (read_cluster_conf(path_name, &my_conf) != EAR_SUCCESS) {
        verbose(0, "ERROR reading cluster configuration");
    }
    
    user_t user_info;
    if (user_all_ids_get(&user_info) != EAR_SUCCESS)
    {
        warning("Failed to retrieve user data\n");
        exit(1);
    }

    char *user = user_info.ruid_name;
    if (getuid() == 0 || is_privileged_command(&my_conf))
    {
        user = NULL; //by default, privilegd users or root will query all user jobs
    }

    char *token;
    while ((opt = getopt(argc, argv, "n:u:j:f:t:vmablc:hx::")) != -1) 
    {
        switch (opt)
        {
            case 'n':
                if (!strcmp(optarg, "all")) limit = -1;
                else limit = atoi(optarg);
                break;
            case 'u':
                if (user != NULL) break;
                user = optarg;
                break;
            case 'j':
                if (strchr(optarg, ','))
                {
                    strcpy(job_ids, optarg);
                }
                else
                {
                    job_id = atoi(strtok(optarg, "."));
                    token = strtok(NULL, ".");
                    if (token != NULL) step_id = atoi(token);
                }
                break;
            case 'x':
                is_events = 1;
                if (optind < argc && strchr(argv[optind], '-') == NULL)
                {
                    if (strchr(argv[optind], ','))
                    {
                        strcpy(job_ids, argv[optind]);
                    }
                    else
                    {
                        job_id = atoi(strtok(argv[optind], "."));
                        token = strtok(NULL, ".");
                        if (token != NULL) step_id = atoi(token);
                    }
                }
                else if (verbose) printf("No argument for -x\n");
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
                all_mpi = 1;
                break;
            case 'c':
                strcpy(csv_path, optarg);
                break;
            case 't':
                strcpy(e_tag, optarg);
                break;
            case 'a':
                avx = 1;
                break;
            case 'h':
                free_cluster_conf(&my_conf);
                usage(argv[0]);
                break;
        }
    }
    
    if (verbose) printf("Limit set to %d\n", limit);

    if (file_name != NULL) read_from_files(file_name, user, job_id, limit, step_id);
    else if (is_events) read_events(user, job_id, limit, step_id, job_ids);
    else read_from_database(user, job_id, limit, step_id, e_tag, job_ids); 

    free_cluster_conf(&my_conf);
    exit(0);
}
