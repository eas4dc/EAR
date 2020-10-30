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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <common/config.h>
#include <sys/types.h>
#include <common/hardware/cpupower.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multifit.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/database/db_helper.h>
#include <common/types/signature.h>
#include <common/types/projection.h>
#include <common/types/application.h>
#include <common/types/coefficient.h>
#include <common/types/configuration/cluster_conf.h>

#define CREATE_FLAGS S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH

application_t *app_list;
application_t **sorted_app_list;
coefficient_t **coeffs_list;

uint *samples_per_app;
uint num_diff_apps;

char nodename[256],*coeff_root;
unsigned long *node_freq_list;
uint num_node_p_states;
uint min_freq;
uint nom_freq;

uint num_apps, current_app = 0;
uint *samples_f, i, *current;

#define MALLOC(var, type, mult)                       \
    var = (type *) malloc(sizeof(type) * mult);       \
    if (var == NULL) {                                \
        verbose(0, "Error allocating memory"); \
        exit(1);                                      \
    }
#define OPEN(desc, file_name, permissions, flags)     \
    desc = open(file_name, permissions, flags);       \
    if (desc < 0) {                                   \
        perror("Error opening/creating file.");       \
        exit(1);                                      \
    }


unsigned long p_state_to_freq(int i) {
    return node_freq_list[i];
}

uint freq_to_p_state(ulong freq)
{
    int is_greater;
    int is_equal;
    int has_next;
    int found = 0;
    int i = 0;

    while (i < num_node_p_states && !found) {
        is_equal = freq == node_freq_list[i];
        has_next = i < num_node_p_states - 1;
        is_greater = has_next && freq > node_freq_list[i + 1];
        found = is_equal || is_greater || !has_next;
        i = i + !found;
    }

    return i;
}

uint fill_list_p_states()
{
	  unsigned long num_pstates = 0;
		unsigned long *flist;
		flist=CPUfreq_get_available_frequencies(0,&num_pstates);
		MALLOC(node_freq_list, unsigned long, num_pstates);
		memcpy(node_freq_list,flist,sizeof(unsigned long)*num_pstates);
		CPUfreq_put_available_frequencies(flist);
    return (uint)num_pstates;
}

int app_exists(application_t *Applist, uint total_apps, application_t *newApp) {
    uint pos = 0, found = 0;

    while ((pos < total_apps) && (found == 0)) {
        if ((strcmp(Applist[pos].job.app_id, newApp->job.app_id) == 0) &&
            (Applist[pos].signature.def_f == newApp->signature.def_f)) {
            found = 1;
        } else {
            pos++;
        }
    }
    if (found == 0) return -1;
    else return pos;
}

void average_list_samples(signature_t *current, uint samples)
{
    double foravg = (double) samples;
    current->time = current->time / foravg;
    current->GBS = current->GBS / foravg;
    current->DC_power = current->DC_power / foravg;
    current->CPI = current->CPI / foravg;
    current->TPI = current->TPI / foravg;
}

// A=A+B metrics
void accum_app(signature_t *A, signature_t *B)
{
    A->time += B->time;
    A->GBS += B->GBS;
    A->DC_power += B->DC_power;
    A->TPI += B->TPI;
    A->CPI += B->CPI;
}

void write_app(application_t *A, application_t *B)
{
    strcpy(A->job.app_id, B->job.app_id);
    A->signature.def_f = B->signature.def_f;
    A->job.procs = B->job.procs;
    signature_t *sig_a = &A->signature;
    signature_t *sig_b = &B->signature;
    sig_a->time = sig_b->time;
    sig_a->GBS = sig_b->GBS;
    sig_a->DC_power = sig_b->DC_power;
    sig_a->TPI = sig_b->TPI;
    sig_a->CPI = sig_b->CPI;
}

void nominal_for_power(uint ref, char *app_name, double *power, double *tpi)
{
    int i = 0, found = 0;
    *power = 0;
    *tpi = 0;

    while ((i < samples_f[ref]) && (found == 0))
    {
        if (strcmp(sorted_app_list[ref][i].job.app_id, app_name) == 0) {
            *power = sorted_app_list[ref][i].signature.DC_power;
            *tpi = sorted_app_list[ref][i].signature.TPI;
            found = 1;
        } else i++;
    }
}

void nominal_for_cpi(uint ref, char *app_name, double *cpi, double *tpi)
{
    int i = 0, found = 0;
    *cpi = 0;
    *tpi = 0;

    while ((i < samples_f[ref]) && (found == 0))
    {
        if (strcmp(sorted_app_list[ref][i].job.app_id, app_name) == 0)
        {
            *cpi = sorted_app_list[ref][i].signature.CPI;
            *tpi = sorted_app_list[ref][i].signature.TPI;
            found = 1;
        } else i++;
    }
}

void init_list_coeffs(uint ref, uint i, uint f, double A, double B, double C, double D, double E, double F)
{
	coeffs_list[ref][i].pstate_ref=p_state_to_freq(ref);
    coeffs_list[ref][i].pstate = f;
    coeffs_list[ref][i].available = 1;
    coeffs_list[ref][i].A = A;
    coeffs_list[ref][i].B = B;
    coeffs_list[ref][i].C = C;
    coeffs_list[ref][i].D = D;
    coeffs_list[ref][i].E = E;
    coeffs_list[ref][i].F = F;
}

void usage(char *app)
{
    verbose(0, "Usage: %s isle.path min.freq node.name", app);
    verbose(0, "  Computes a coefficients file for a given node.");
    exit(1);
}

int main(int argc, char *argv[])
{
	double power, cpi, tpi;
	uint filtered_apps = 0, is_learning=1;
	int ret;
	uint f, pos, ref, i;
	ulong p_state_max;
	char path_coef_file[256];
	char coef_file[256];
	int fd, index;

    if (!((argc ==3) || (argc ==4))){
        usage(argv[0]);
    }

	coeff_root=argv[1];
    min_freq = (uint) atoi(argv[2]);
	if (argc==4){
		strcpy(nodename,argv[3]);
	}else{
    	if (gethostname(nodename, sizeof(nodename)) < 0)
    	{
    	    verbose(0, "Error getting node name (%s)", strerror(errno)); //error
    	    _exit(1);
    	}
    	strtok(nodename, ".");
	}

    // We get how many samples per frequency we have
    num_node_p_states = fill_list_p_states();

    // samples_f is to compute how many samples per frequency we have
    MALLOC(samples_f, uint, num_node_p_states);
    MALLOC(current, uint, num_node_p_states);

    for (i = 0; i < num_node_p_states; i++) {
        samples_f[i] = 0;
        current[i] = 0;
    }

    application_t *apps,*tmp_apps;
	#if 0
    num_apps = read_application_text_file(argv[1], &apps);
	#endif
	uint total_apps=0;
	
  	char ear_path[256];
    cluster_conf_t my_conf;
	my_node_conf_t * my_node;
    if (get_ear_conf_path(ear_path)==EAR_ERROR){
            verbose(0, "Error getting ear.conf path");
            exit(0);
    }
    read_cluster_conf(ear_path,&my_conf);
	my_node=get_my_node_conf(&my_conf,nodename);
    if (my_node==NULL){
    	verbose(0," Error in cluster configuration, node %s not found", nodename); //error
    }
	verbose(0, "ear.conf ready");
    init_db_helper(&my_conf.database);
	num_apps =get_num_applications(is_learning, nodename);
 	MALLOC(app_list, application_t, num_apps);
 	MALLOC(apps, application_t, num_apps);
	verbose(0, "%d applications in DB for learning phase", num_apps);
    ret=db_read_applications(&tmp_apps,is_learning, num_apps,nodename);
    if (ret>0){
        for (i=0;i<ret;i++){
            if (strcmp(tmp_apps[i].node_id,nodename)==0){
                copy_application(&apps[total_apps],&tmp_apps[i]);
                total_apps++;
            }   
        }   
        free(tmp_apps);
    }else{
        verbose(0, "Warning, DB has reported %d jobs for node %s",ret,nodename);
    }

    verbose(0, "Total apps:%d, expected %d", total_apps,num_apps);
    MALLOC(samples_per_app, uint, num_apps);
   
    for (i = 0; i < num_apps; i++) {
		print_application(&apps[i]);
        samples_per_app[i] = 0;
    }
 
    for (i = 0; i < num_apps; i++)
    {
        verbose(0, "APP SIGNATURE %lu", apps[i].signature.def_f);
        if (apps[i].signature.def_f >= min_freq) {

            if ((index = app_exists(app_list, filtered_apps, &apps[i])) >= 0) {
                // If APP exists, then accumulate its values in
                accum_app(&app_list[index].signature, &apps[i].signature);
                samples_per_app[index]++;
            } else {
                write_app(&app_list[filtered_apps], &apps[i]);
                samples_per_app[filtered_apps] = 1;
                filtered_apps++;
            }
        }
    }

    // We will consider only applictions with f >= min_freq
    num_apps = filtered_apps;

    // We must compute the average per (app,f)
    for (i = 0; i < num_apps; i++) {
        average_list_samples(&app_list[i].signature, samples_per_app[i]);
    }

    verbose(0, "%s: %u total P_STATES (1: %u KHz), readed %d applications with f >= %u",
            nodename, num_node_p_states, nom_freq, num_apps, min_freq);

    // We maintain the name's of applications to generate graphs
    for (current_app = 0; current_app < num_apps; current_app++) {
        if (app_list[current_app].signature.def_f >= min_freq) {
            index = freq_to_p_state(app_list[current_app].signature.def_f);
            samples_f[index]++;
        }
    }

    // We group applications with frequencies
    MALLOC(sorted_app_list, application_t *, num_node_p_states);

    for (i = 0; i < num_node_p_states; i++) {
        MALLOC(sorted_app_list[i], application_t, samples_f[i]);
    }

    // Sorting applications by frequency
    for (current_app = 0; current_app < num_apps; current_app++) {
        f = app_list[current_app].signature.def_f;

        if (f >= min_freq) {
            i = freq_to_p_state(f);
            pos = current[i];

            write_app(&sorted_app_list[i][pos], &app_list[current_app]);
            current[i]++;
        }
    }

    // Computing coefficients
    MALLOC(coeffs_list, coefficient_t *, num_node_p_states);

    for (f = 0; f < num_node_p_states; f++) {
        MALLOC(coeffs_list[f], coefficient_t, num_node_p_states);

        for (i = 0; i < num_node_p_states; i++) {
			coeffs_list[f][i].pstate_ref=p_state_to_freq(f);
            coeffs_list[f][i].available = 0;
            coeffs_list[f][i].pstate = p_state_to_freq(i);
			coeffs_list[f][i].A=0;
			coeffs_list[f][i].B=0;
			coeffs_list[f][i].C=0;
			coeffs_list[f][i].D=0;
			coeffs_list[f][i].E=0;
			coeffs_list[f][i].F=0;
        }
    }

    init_list_coeffs(0, 0, nom_freq, 1, 0, 0, 1, 0, 0);
    double A, B, C, D, E, F;

    /* We compute regression */
	sprintf(path_coef_file,"%s/island%d",coeff_root,my_node->island);
	if (mkdir (path_coef_file,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IROTH)<0){
		if (errno!=EEXIST){
			verbose(0, "Error, path %s cannot be created (%s)", path_coef_file, strerror(errno)); //error
			exit(1);
		}
	}
    
    sprintf(coef_file, "%s/coeffs.%s", path_coef_file,nodename);
    OPEN(fd, coef_file, O_WRONLY | O_CREAT | O_TRUNC, CREATE_FLAGS);
	p_state_max=freq_to_p_state(min_freq);
    for (ref = 0; ref <= p_state_max; ref++)
    {
        if (samples_f[ref])
        {

            for (f = 0; f <= p_state_max; f++) // Coefficients per frequency
            {
                if (ref == f)
                {
			init_list_coeffs(ref, f, p_state_to_freq(f), 1, 0, 0, 1, 0, 0);
			verbose(0, "Writting coeffs for freq=%u projection %u", ref, f);
            		if (write(fd, &coeffs_list[ref][f], sizeof(coefficient_t) ) != sizeof(coefficient_t) ) {
                		perror("Error writting coefficients file\n");
                		exit(1);
            		}
                }
                else
                {
                    int n = samples_f[f];

                    if (n > 0)
                    {
                        #if CC_DEBUG
                        verbose(0, "Computing POWER regression for freq %u with %u samples (REF=%u)",
                                p_state_to_freq(f), samples_f[f], p_state_to_freq(ref));
                        #endif
                        gsl_matrix *SIGNATURE_POWER = gsl_matrix_calloc(n, 3);
                        gsl_vector *POWER = gsl_vector_alloc(n);
                        gsl_vector *COEFFS = gsl_vector_alloc(3);

                        for (i = 0; i < n; i++) {
                            // POWER
                            gsl_vector_set(POWER, i, sorted_app_list[f][i].signature.DC_power);

                            nominal_for_power(ref, sorted_app_list[f][i].job.app_id, &power, &tpi);

                            // SIGNATURE VALUES
                            gsl_matrix_set(SIGNATURE_POWER, i, 0, 1);
                            gsl_matrix_set(SIGNATURE_POWER, i, 1, power);
                            gsl_matrix_set(SIGNATURE_POWER, i, 2, tpi);
                        }

                        double chisq;
                        gsl_matrix *cov = gsl_matrix_alloc(3, 3);
                        gsl_multifit_linear_workspace *wspc = gsl_multifit_linear_alloc(n, 3);
                        gsl_multifit_linear(SIGNATURE_POWER, POWER, COEFFS, cov, &chisq, wspc);

                        #if CC_DEBUG
                        verbose(0, "Coefficient for power: %g*DC_power + %g*TPI_f0 + %g",
                                gsl_vector_get(COEFFS, 1), gsl_vector_get(COEFFS, 2), gsl_vector_get(COEFFS, 0));
                        #endif
                        A = gsl_vector_get(COEFFS, 1);
                        B = gsl_vector_get(COEFFS, 2);
                        C = gsl_vector_get(COEFFS, 0);
                        gsl_matrix_free(SIGNATURE_POWER);
                        gsl_matrix_free(cov);
                        gsl_vector_free(POWER);
                        gsl_vector_free(COEFFS);
                        gsl_multifit_linear_free(wspc);
                    }
                    if (n > 0)
                    {
                        #if CC_DEBUG
                        verbose(0, "Computing CPI regression for freq %u with %u samples (REF=%u)",
                                p_state_to_freq(f), samples_f[f], p_state_to_freq(ref));
                        #endif
                        gsl_matrix *SIGNATURE_CPI = gsl_matrix_calloc(n, 3);
                        gsl_vector *CPI = gsl_vector_alloc(n);
                        gsl_vector *COEFFS = gsl_vector_alloc(3);

                        for (i = 0; i < n; i++)
                        {
                            // CPI
                            gsl_vector_set(CPI, i, sorted_app_list[f][i].signature.CPI);

                            nominal_for_cpi(ref, sorted_app_list[f][i].job.app_id, &cpi, &tpi);

                            // SIGNATURE VALUES
                            gsl_matrix_set(SIGNATURE_CPI, i, 0, 1);
                            gsl_matrix_set(SIGNATURE_CPI, i, 1, cpi);
                            gsl_matrix_set(SIGNATURE_CPI, i, 2, tpi);
                        }

                        double chisq;
                        gsl_matrix *cov = gsl_matrix_alloc(3, 3);
                        gsl_multifit_linear_workspace *wspc = gsl_multifit_linear_alloc(n, 3);
                        gsl_multifit_linear(SIGNATURE_CPI, CPI, COEFFS, cov, &chisq, wspc);

                        #if CC_DEBUG
                        verbose(0, "Coefficient for cpi: %g*CPI_f0 + %g*TPI_f0 + %g",
                                gsl_vector_get(COEFFS, 1), gsl_vector_get(COEFFS, 2), gsl_vector_get(COEFFS, 0));
                        #endif
                        D = gsl_vector_get(COEFFS, 1);
                        E = gsl_vector_get(COEFFS, 2);
                        F = gsl_vector_get(COEFFS, 0);

                        gsl_matrix_free(SIGNATURE_CPI);
                        gsl_matrix_free(cov);
                        gsl_vector_free(CPI);
                        gsl_vector_free(COEFFS);
                        gsl_multifit_linear_free(wspc);

                        init_list_coeffs(ref, f, p_state_to_freq(f), A, B, C, D, E, F);
			verbose(0, "Writting coeffs for freq=%u projection %u", ref, f);
            		
			if (write(fd, &coeffs_list[ref][f], sizeof(coefficient_t) ) != sizeof(coefficient_t) ) {
                			perror("Error writting coefficients file\n");
                			exit(1);
            		}
                    }else{
                        verbose(0, "Writting NULL coeffs for freq=%u projection %u", ref, f);
                       
			 if (write(fd, &coeffs_list[ref][f], sizeof(coefficient_t) ) != sizeof(coefficient_t) ) {
                            perror("Error writting coefficients file\n");
                            exit(1);
                        }
					}
                }
            }


        }else{
			verbose(0, "Writting NULL coeffs for freq=%u projections %lu", ref, p_state_max);
            if (write(fd, &coeffs_list[ref][0], sizeof(coefficient_t)* p_state_max) != sizeof(coefficient_t)*p_state_max ) {
            	perror("Error writting coefficients file\n");
            	exit(1);
            }
		}
    }
    close(fd);

    verbose(0, " %s: computed coefficients", nodename);
    return 0;
}
