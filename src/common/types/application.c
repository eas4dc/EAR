/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
// #define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/system/file.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/types/application.h>
#include <common/output/debug.h>

void init_application(application_t *app)
{
    memset(app, 0, sizeof(application_t));
}

void copy_application(application_t *destiny, application_t *source)
{
    memcpy(destiny, source, sizeof(application_t));
}

void application_print_channel(FILE *file, application_t *app)
{
    fprintf(file, "application_t: id '%s', job id '%lu.%lu', node id '%s'\n",
            app->job.app_id, app->job.id, app->job.step_id, app->node_id);
    fprintf(file, "application_t: learning '%d', time '%lf', avg freq '%lu'\n",
            app->is_learning, app->signature.time, app->signature.avg_f);
    fprintf(file, "application_t: pow dc '%lf', pow ram '%lf', pow pack '%lf'\n",
            app->signature.DC_power, app->signature.DRAM_power, app->signature.PCK_power);
}

/*
 *
 * Obsolete?
 *
 */

#define APP_TEXT_FILE_FIELDS	33
#define EXTENDED_DIFF			11
#define PERMISSION              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define OPTIONS                 O_WRONLY | O_CREAT | O_TRUNC | O_APPEND

int read_application_text_file(char *path, application_t **apps, char is_extended)
{
    char line[PIPE_BUF];
    application_t *apps_aux, *a;
    int lines, i, ret;
    FILE *fd;

    if (path == NULL) {
        return EAR_ERROR;
    }

    if ((fd = fopen(path, "r")) == NULL) {
        return EAR_OPEN_ERROR;
    }

    // Reading the header
    fscanf(fd, "%s\n", line);
    lines = 0;

    while(fscanf(fd, "%s\n", line) > 0) {
        lines += 1;
    }

    // Allocating memory
    apps_aux = (application_t *) malloc(lines * sizeof(application_t));
    memset(apps_aux, 0, sizeof(application_t));

    // Rewind
    rewind(fd);
    fscanf(fd, "%s\n", line);

    i = 0;
    a = apps_aux;

    while((ret = scan_application_fd(fd, a, is_extended)) == (APP_TEXT_FILE_FIELDS - !(is_extended)*EXTENDED_DIFF))
    {
        apps_aux[i].is_mpi = 1;
        i += 1;
        a = &apps_aux[i];
    }

    fclose(fd);

    if (ret >= 0 && ret < APP_TEXT_FILE_FIELDS) {
        free(apps_aux);
        return EAR_ERROR;
    }

    *apps = apps_aux;
    return i;
}

int print_application_fd(int fd, application_t *app, int new_line, char is_extended, int single_column)
{
    char buff[1024];
    sprintf(buff, "%s;", app->node_id);
    write(fd, buff, strlen(buff));
    print_job_fd(fd, &app->job);
    dprintf(fd, ";");
    if (app->signature.DRAM_power == 0) app->signature.DRAM_power = app->power_sig.DRAM_power;
    if (app->signature.PCK_power == 0) app->signature.PCK_power = app->power_sig.PCK_power;
    if (app->signature.DC_power == 0) app->signature.DC_power = app->power_sig.DC_power;
    if (app->signature.avg_f == 0) app->signature.avg_f = app->power_sig.avg_f;
    if (app->signature.def_f == 0) app->signature.def_f = app->power_sig.def_f;

    signature_print_fd(fd, &app->signature, is_extended, single_column,',');

    if (new_line) {
        dprintf(fd, "\n");
    }

    return EAR_SUCCESS;
}

int create_app_header(char * header, char *path, uint num_gpus, char is_extended, int single_column)
{
    char *HEADER_BASE = "NODENAME;JOBID;STEPID;USERID;GROUPID;JOBNAME;USER_ACC;ENERGY_TAG;POLICY;POLICY_TH;"\
                        "AVG_CPUFREQ_KHZ;AVG_IMCFREQ_KHZ;DEF_FREQ_KHZ;TIME_SEC;CPI;TPI;MEM_GBS;IO_MBS;PERC_MPI;DC_NODE_POWER_W;DRAM_POWER_W;"\
                        "PCK_POWER_W;CYCLES;INSTRUCTIONS;CPU-GFLOPS";
		char HEADER[1024];

		/* If file already exists we will not add the header */
		if (file_is_regular(path)) return EAR_SUCCESS;

		if (header != NULL){ 
			strncpy(HEADER, header, sizeof(HEADER));
			strncat(HEADER, HEADER_BASE, sizeof(HEADER));
		}else{
			strncpy(HEADER, HEADER_BASE, sizeof(HEADER));
		}

    if (is_extended) {
        char *ext_header = ";L1_MISSES;L2_MISSES;L3_MISSES;SPOPS_SINGLE;SPOPS_128;SPOPS_256;SPOPS_512;DPOPS_SINGLE;"\
                           "DPOPS_128;DP_256;DPOPS_512";
        xstrncat(HEADER, ext_header, strlen(ext_header));
    }
		debug("Creating header with %d GPUS", num_gpus);
#if USE_GPUS
		if (single_column) num_gpus = ear_min(num_gpus, 1);
    for (int i = 0; i < num_gpus; i++){
        char gpu_hdr[128];
        xsnprintf(gpu_hdr, sizeof(gpu_hdr), ";GPU%d_POWER_W;GPU%d_FREQ_KHZ;GPU%d_MEM_FREQ_KHZ;GPU%d_UTIL_PERC;GPU%d_MEM_UTIL_PERC",
                i, i, i, i, i);
        xstrncat(HEADER, gpu_hdr, sizeof(gpu_hdr));
    }
#endif
	  int fd = open(path, OPTIONS, PERMISSION);
    if (fd >= 0) {
    	dprintf(fd, "%s\n", HEADER);
    }
		close(fd);

	return EAR_SUCCESS;
}

int append_application_text_file(char *path, application_t *app, char is_extended, int add_header, int single_column)
{
    //lacking: NODENAME(node_id in loop_t), not linked to any loop
    int fd;
		uint num_gpus = 0;
#if USE_GPUS
		num_gpus = app->signature.gpu_sig.num_gpus;
#endif
    if (add_header) create_app_header(NULL, path, num_gpus, is_extended, single_column);
		fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0)
    {
			return EAR_ERROR;
		}

    print_application_fd(fd, app, 1, is_extended, single_column);

    close(fd);

    return EAR_SUCCESS;
}

int scan_application_fd(FILE *fd, application_t *app, char is_extended)
{
    application_t *a;
    int ret;

    a = app;
    if (is_extended)
        ret = fscanf(fd, "%[^;];%lu;%lu;%[^;];" \
                "%[^;];%[^;];%[^;];%[^;];%[^;];%lf;" \
                "%lu;%lu;" \
                "%lf;%lf;%lf;%lf;" \
                "%lf;%lf;%lf;" \
                "%llu;%llu;%lf;" \
                "%llu;%llu;%llu;" \
                "%llu;%llu;%llu;%llu;"	\
                "%llu;%llu;%llu;%llu\n",
                a->node_id, &a->job.id, &a->job.step_id, a->job.user_id,
                a->job.group_id, a->job.app_id, a->job.user_acc, a->job.energy_tag, a->job.policy, &a->job.th,
                &a->signature.avg_f, &a->signature.def_f,
                &a->signature.time, &a->signature.CPI, &a->signature.TPI, &a->signature.GBS,
                &a->signature.DC_power, &a->signature.DRAM_power, &a->signature.PCK_power,
                &a->signature.cycles, &a->signature.instructions, &a->signature.Gflops,
                &a->signature.L1_misses, &a->signature.L2_misses, &a->signature.L3_misses,
                &a->signature.FLOPS[0], &a->signature.FLOPS[1], &a->signature.FLOPS[2], &a->signature.FLOPS[3],
                &a->signature.FLOPS[4], &a->signature.FLOPS[5], &a->signature.FLOPS[6], &a->signature.FLOPS[7]);

    else
        ret = fscanf(fd, "%[^;];%lu;%lu;%[^;];" \
                "%[^;];%[^;];%[^;];%[^;];%[^;];%lf;" \
                "%lu;%lu;" \
                "%lf;%lf;%lf;%lf;" \
                "%lf;%lf;%lf;" \
                "%llu;%llu;%lf\n",
                a->node_id, &a->job.id, &a->job.step_id, a->job.user_id,
                a->job.group_id, a->job.app_id, a->job.user_acc, a->job.energy_tag, a->job.policy, &a->job.th,
                &a->signature.avg_f, &a->signature.def_f,
                &a->signature.time, &a->signature.CPI, &a->signature.TPI, &a->signature.GBS,
                &a->signature.DC_power, &a->signature.DRAM_power, &a->signature.PCK_power,
                &a->signature.cycles, &a->signature.instructions, &a->signature.Gflops);

    return ret;
}

/*
 *
 * I don't know what is this
 *
 */

void print_application_fd_binary(int fd,application_t *app)
{
    write(fd,app,sizeof(application_t));
}

void read_application_fd_binary(int fd,application_t *app)
{
    read(fd,app,sizeof(application_t));
}

/*
 *
 * We have to take a look these print functions and clean
 *
 */

int print_application(application_t *app)
{
    return print_application_fd(STDOUT_FILENO, app, 1, 1, 0);
}

void verbose_gpu_app(uint vl,application_t *myapp)
{
#if USE_GPUS
    signature_t *app=&myapp->signature;
    gpu_app_t  *mys;
    uint gpui;
    for (gpui=0;gpui<app->gpu_sig.num_gpus;gpui++){
        mys=&app->gpu_sig.gpu_data[gpui];
        verbose(vl,"GPU%u [Power %.2lf freq %lu mem_freq %lu util %lu mem_util %lu]\n",gpui,mys->GPU_power,mys->GPU_freq,mys->GPU_mem_freq,mys->GPU_util,mys->GPU_mem_util);
    }
#endif
}

void verbose_application_data(uint vl, application_t *app)
{
    /* Convert values to GHz */
    float avg_f     = ((double) app->signature.avg_f)     / 1000000.0;
    float avg_imc_f = ((double) app->signature.avg_imc_f) / 1000000.0;
    float def_f     = ((double) app->job.def_f)           / 1000000.0;

    float pavg_f = ((double) app->power_sig.avg_f) / 1000000.0;
    float pdef_f = ((double) app->power_sig.def_f) / 1000000.0;

    char st[64], et[64];
    char stmpi[64],etmpi[64];

    struct tm *tmp;
    tmp=localtime(&app->job.start_time);
    strftime(st, sizeof(st), "%c", tmp);

    tmp=localtime(&app->job.end_time);
    strftime(et, sizeof(et), "%c", tmp);

    tmp=localtime(&app->job.start_mpi_time);
    strftime(stmpi, sizeof(stmpi), "%c", tmp);

    tmp=localtime(&app->job.end_mpi_time);
    strftime(etmpi, sizeof(etmpi), "%c", tmp);

    /* Formatting the message */

    char job_info_txt[1024]; // app / user / account / job.step
    int ret = snprintf(job_info_txt, sizeof(job_info_txt),
                       "--- App id: %s / user id: %s / account: %s / job.step: %lu.%lu ---\n\n"
                       "Processes: %lu\n"
                       "Start time: %s End time: %s\n"
                       "Start MPI : %s End MPI :  %s\n\n"
                       "(Power sign.) Wall time: %0.3lf s, nominal freq.: %0.2f GHz, avg. freq: %0.2f GHz\n"
                       "              DC/DRAM/PCK power: %0.3lf/%0.3lf/%0.3lf (W)\n"
                       "              Max/Min DC Power (W): %0.3lf/%0.3lf\n\n",
                       app->job.app_id, app->job.user_id, app->job.user_acc, app->job.id, app->job.step_id,
                       app->job.procs,
                       st, et,
                       stmpi, etmpi,
                       app->power_sig.time, pdef_f, pavg_f,
                       app->power_sig.DC_power, app->power_sig.DRAM_power, app->power_sig.PCK_power,
                       app->power_sig.max_DC_power,app->power_sig.min_DC_power);
    if (ret < 0) {
        verbose(vl, "%sERROR%s Formatting application summary.", COL_RED, COL_CLR);
    } else if (ret >= sizeof(job_info_txt)) {
        verbose(vl, "%sWARNING%s The application summary message was truncated. Required size: %d", COL_YLW, COL_CLR, ret);
    }


    if (app->is_mpi) {

        char mpi_info_txt[512]; // signature details

        snprintf(mpi_info_txt, sizeof(mpi_info_txt),
                "(mpi_sig) Wall time: %0.3lf s, nominal freq.: %0.2f GHz, avg. freq: %0.2f GHz, avg imc freq: %0.2f GHz\n"
                "          Avg. mpi_time/exec_time %.1lf %%\n\n"
                "          tasks: %lu\n\n"
                "          CPI/TPI: %0.3lf/%0.3lf, GB/s: %0.3lf, GFLOPS: %0.3lf, IO: %0.3lf (MB/s)\n"
                "          DC/DRAM/PCK power (W): %0.3lf/%0.3lf/%0.3lf GFlops/Watt %.3lf\n\n",
                app->signature.time, def_f, avg_f, avg_imc_f,
                app->signature.perc_MPI,
                app->job.procs,
                app->signature.CPI, app->signature.TPI, app->signature.GBS, app->signature.Gflops, app->signature.IO_MBS,
                app->signature.DC_power, app->signature.DRAM_power, app->signature.PCK_power, app->signature.Gflops/app->signature.DC_power);

        size_t remain_job_buff_len = sizeof(job_info_txt) - strlen(job_info_txt) - 1;
        strncat(job_info_txt, mpi_info_txt, remain_job_buff_len);
    }

    /* Encapsulate the message in a beautiful way */

    char app_summ_hdr_txt[512];
    snprintf(app_summ_hdr_txt, sizeof(app_summ_hdr_txt), " Application Summary [%s] ", app->node_id);

    size_t max_str_len = 90;
    int app_summ_hdr_dec_len = (int) ((max_str_len - strlen(app_summ_hdr_txt)) / 2); // Compute the length of the decorator, e.g. "---"
    char *app_summ_hdr_dec_txt = malloc(app_summ_hdr_dec_len + 1);

    memset(app_summ_hdr_dec_txt, '-', app_summ_hdr_dec_len);
    app_summ_hdr_dec_txt[app_summ_hdr_dec_len] = '\0';

    /* Build a beautiful footer */

    int app_summ_ftr_dec_len = (int)(strlen(app_summ_hdr_txt) + app_summ_hdr_dec_len * 2); // Length of the decorator
    char *app_summ_ftr_dec = malloc(app_summ_ftr_dec_len + 1);

    memset(app_summ_ftr_dec, '-', app_summ_ftr_dec_len);
    app_summ_ftr_dec[app_summ_ftr_dec_len] = '\0';

    /* Verbose the summary */

    /*
    if (app->is_mpi) {
        verbose(vl, "%s", mpi_info_txt); // signature details
    }
    */

    // verbose_gpu_app(vl, app); // This method outputs GPU summary

#if USE_GPUS
    char *gpu_sig_buff = (char *) malloc(512 * sizeof(char));

    signature_t *app_sig = &app->signature;

    if (gpu_sig_buff && app_sig->gpu_sig.num_gpus) {

        size_t remain_gpusig_buff_len = 512;

        gpu_app_t *mys;

        for (int gpui = 0; gpui < app_sig->gpu_sig.num_gpus; gpui++) {

            mys = &app_sig->gpu_sig.gpu_data[gpui];

            char gpu_i_sig_buff[128];
            snprintf(gpu_i_sig_buff, sizeof(gpu_i_sig_buff),
                    "GPU%u [Power %.2lf freq %lu mem_freq %lu util %lu mem_util %lu]\n",
                    gpui, mys->GPU_power, mys->GPU_freq, mys->GPU_mem_freq, mys->GPU_util, mys->GPU_mem_util);

            remain_gpusig_buff_len -= (strlen(gpu_i_sig_buff) - 1);
            strncat(gpu_sig_buff, gpu_i_sig_buff, remain_gpusig_buff_len);
            
        }
        verbose(vl, "\n%s%s%s\n%s\n%s\n%s",
                app_summ_hdr_dec_txt, app_summ_hdr_txt, app_summ_hdr_dec_txt, job_info_txt,
                gpu_sig_buff, app_summ_ftr_dec); // header + job info + gpu info + footer
        free(gpu_sig_buff);
    } else {
        verbose(vl, "\n%s%s%s\n%s\n%s",
                app_summ_hdr_dec_txt, app_summ_hdr_txt, app_summ_hdr_dec_txt, job_info_txt, app_summ_ftr_dec); // header + job info + footer
    }
#else
    verbose(vl, "\n%s%s%s\n%s\n%s",
            app_summ_hdr_dec_txt, app_summ_hdr_txt, app_summ_hdr_dec_txt, job_info_txt, app_summ_ftr_dec); // header + job info + footer
#endif

    free(app_summ_hdr_dec_txt);
    free(app_summ_ftr_dec);
}

void report_application_data(application_t *app)
{
    verbose_application_data(VTYPE,app);
}

// TODO: Try to port the below implementation to verbose_application_data function
void report_mpi_application_data(application_t *app)
{
    /* Convert values to GHz */
    float avg_f     = ((double) app->signature.avg_f)     / 1000000.0;
    float avg_imc_f = ((double) app->signature.avg_imc_f) / 1000000.0;
    float def_f     = ((double) app->job.def_f)           / 1000000.0;

    /* Formatting the message */

    char job_info_txt[1024]; // app / user / account / job.step

    snprintf(job_info_txt, sizeof(job_info_txt),
             "--- App id: %s / user id: %s / account: %s / job.step: %lu.%lu ---\n",
             app->job.app_id, app->job.user_id, app->job.user_acc, app->job.id, app->job.step_id);

    char mpi_info_txt[512]; // signature details

    if (app->is_mpi) {
        snprintf(mpi_info_txt, sizeof(mpi_info_txt),
                "(mpi_sig) Wall time: %0.3lf s, nominal freq.: %0.2f GHz, avg. freq: %0.2f GHz, avg imc freq: %0.2f GHz\n"
                "          Avg. mpi_time/exec_time %.1lf %%\n\n"
                "          tasks: %lu\n\n"
                "          CPI/TPI: %0.3lf/%0.3lf, GB/s: %0.3lf, GFLOPS: %0.3lf, IO: %0.3lf (MB/s)\n"
                "          DC/DRAM/PCK power: %0.3lf/%0.3lf/%0.3lf (W) GFlops/Watts %.3lf\n\n",
                app->signature.time, def_f, avg_f, avg_imc_f,
                app->signature.perc_MPI,
                app->job.procs,
                app->signature.CPI, app->signature.TPI, app->signature.GBS, app->signature.Gflops, app->signature.IO_MBS,
                app->signature.DC_power, app->signature.DRAM_power, app->signature.PCK_power, app->signature.Gflops/app->signature.DC_power);
    }

    /* Encapsulate the message in a beautiful way */

    size_t max_str_len = strlen(job_info_txt);

    char app_summ_hdr_txt[512];
    snprintf(app_summ_hdr_txt, sizeof(app_summ_hdr_txt), " Application Summary [%s] ", app->node_id);

    int app_summ_hdr_dec_len = (int) ((max_str_len - strlen(app_summ_hdr_txt)) / 2); // Compute the length of the decorator, e.g. "---"
    char *app_summ_hdr_dec_txt = malloc(app_summ_hdr_dec_len + 1);

    memset(app_summ_hdr_dec_txt, '-', app_summ_hdr_dec_len);
    app_summ_hdr_dec_txt[app_summ_hdr_dec_len] = '\0';

    /* Verbose the summary */

    verbose(VTYPE, "%s%s%s\n%s\n", app_summ_hdr_dec_txt, app_summ_hdr_txt, app_summ_hdr_dec_txt, job_info_txt); // header and job info

    free(app_summ_hdr_dec_txt);

    if (app->is_mpi) {
        verbose(VTYPE, "%s", mpi_info_txt); // signature details
    }

    verbose_gpu_app(VTYPE, app); // This method outputs GPU summary

    /* Build a beautiful footer */

    int app_summ_ftr_dec_len = (int)(strlen(app_summ_hdr_txt) + app_summ_hdr_dec_len * 2); // Length of the decorator
    char *app_summ_ftr_dec = malloc(app_summ_ftr_dec_len + 1);

    memset(app_summ_ftr_dec, '-', app_summ_ftr_dec_len);
    app_summ_ftr_dec[app_summ_ftr_dec_len] = '\0';

    verbose(VTYPE, "%s", app_summ_ftr_dec);

    free(app_summ_ftr_dec);
}

void mark_as_eard_connected(int jid,int sid,int pid)
{
    int my_id;	
    char *tmp,con_file[128];
    int fd;
    tmp=getenv("EAR_TMP");
    if (tmp == NULL){
        debug("EAR_TMP not defined in mark_as_eard_connected");
        return;
    }
    my_id=create_ID(jid,sid);
    sprintf(con_file,"%s/.master.%d.%d",tmp,my_id,pid);
    debug("Creating %s",con_file);	
    fd=open(con_file,O_CREAT|O_WRONLY,S_IRUSR|S_IWUSR);
    close(fd);
    return;
}
uint is_already_connected(int jid,int sid,int pid)
{
    int my_id;
    int fd;
    char *tmp,con_file[128];;
    tmp=getenv("EAR_TMP");
    if (tmp == NULL){ 
        debug("EAR_TMP not defined in is_already_connected");
        return 0;
    }
    my_id=create_ID(jid,sid);
    sprintf(con_file,"%s/.master.%d.%d",tmp,my_id,pid);
    debug("Looking for %s ",con_file);
    fd=open(con_file,O_RDONLY);
    if (fd>=0){
        close(fd);
        return 1;
    }
    return 0;
}

void mark_as_eard_disconnected(int jid,int sid,int pid)
{
    int my_id;
    char *tmp,con_file[128];
    tmp=getenv("EAR_TMP");
    if (tmp == NULL){ 
        debug("EAR_TMP not defined in mark_as_eard_disconnected");
        return;
    }
    my_id=create_ID(jid,sid);
    sprintf(con_file,"%s/.master.%d.%d",tmp,my_id,pid);
    debug("Removing %s",con_file);
    unlink(con_file);
}
