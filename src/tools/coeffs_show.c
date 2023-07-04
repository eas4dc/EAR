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

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/system/file.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/coefficient.h>
#include <library/models/cpu_power_model_default.h>
#include <common/output/debug.h>

/*
 * Reports coefficients in stdout and csv file 
 **/

void print_basic_coefficients(coefficient_t *avg, int n_pstates, char *csv_output)
{
	int i;
  FILE *fd;
  coefficient_t *coeff;
  
  fd = fopen(csv_output,"w");	

  fprintf(fd, "FROM\tTO\tA\tB\tC\tD\tE\tF\n"); 
  verbose(2, "FROM\tTO\tA\tB\tC\tD\tE\tF");
	for (i=0; i<n_pstates; i++){
    coeff = &avg[i];
    if(coeff->available){
      verbose(2,"%lu\t%lu\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf",
                 coeff->pstate_ref, coeff->pstate, 
                 coeff->A, coeff->B, coeff->C, coeff->D, coeff->E, coeff->F);

      fprintf(fd,"%lu\t%lu\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", 
                 coeff->pstate_ref, coeff->pstate, 
                 coeff->A, coeff->B, coeff->C, coeff->D, coeff->E, coeff->F);  
    }
	}
  fclose(fd);
}

void print_cpu_model_coefficients(intel_skl_t *coeff, char *csv_output)
{
  FILE *fd;
  fd = fopen(csv_output,"w");	
	
  verbose(2, "IPC\tGBS\tVPI\tF\tINTER");
  verbose(2, "%lf\t%lf\t%lf\t%lf\t%lf",
              coeff->ipc, coeff->gbs, coeff->vpi, coeff->f, coeff->inter);

	fprintf(fd, "IPC\tGBS\tVPI\tF\tINTER\n");
  fprintf(fd,"%lf\t%lf\t%lf\t%lf\t%lf\n",
              coeff->ipc, coeff->gbs, coeff->vpi, coeff->f, coeff->inter);

  fclose(fd);
}

int main(int argc, char *argv[])
{
  state_t state;
	int n_pstates;
	int size;
  // for the csv output
  char *tag;
  char *ext = ".csv";
  char *csv_name;
  char *csv_name_full;
  
  if (argc < 2) {
      verbose(0, "Usage: %s coeffs_file", argv[0]);
      exit(1);
  }

	VERB_SET_LV(5);
	size = ear_file_size(argv[1]);
  
  tag = strrchr(argv[1], '.');
  if (!tag) {
    debug("coeffs. file: no tag extension");
    tag = "";
  } 

	if (size < 0) {
		error("invalid coeffs path %s (%s)", argv[1], intern_error_str);
		exit(1);
	}
  
  if (size == sizeof(intel_skl_t)) {
    // CPU Power model coeffs 
    debug("CPU Power Model coefficients");
    debug("coefficients file size: %d", size);
    
	  intel_skl_t *coeffs;
    coeffs = (intel_skl_t*) calloc(size, 1);

    if (coeffs == NULL) {
    error("not enough memory");
    exit(1);
    }

    state = ear_file_read(argv[1], (char *) coeffs, size);

    if (state_fail(state)) {
      error("state id: %d (%s)", state, state_msg);
      exit(1);
    }

    csv_name = "coeffs.cpu_model";
    csv_name_full = malloc(strlen(csv_name)+strlen(tag)+4);

    strcpy(csv_name_full, csv_name);
    strcat(csv_name_full, tag);
    strcat(csv_name_full, ext);
    
    print_cpu_model_coefficients(coeffs, csv_name_full);

  }
  else if (size % sizeof(coefficient_t) == 0) {
    // Basic model coeffs
    n_pstates = size / sizeof(coefficient_t);
    debug("Basic Model coefficients");
    debug("coefficients file size: %d", size);
    debug("number of P_STATES: %d", n_pstates);
    
    coefficient_t *coeffs;
    coeffs = (coefficient_t*) calloc(size, 1);

    if (coeffs == NULL) {
    error("not enough memory");
    exit(1);
    }

    state = ear_file_read(argv[1], (char *) coeffs, size);

    if (state_fail(state)) {
      error("state id: %d (%s)", state, state_msg);
      exit(1);
    }

    csv_name = "coeffs";
    csv_name_full = malloc(strlen(csv_name)+strlen(tag)+4);
    strcpy(csv_name_full, csv_name);
    strcat(csv_name_full, tag);
    strcat(csv_name_full, ext);
    print_basic_coefficients(coeffs, n_pstates, csv_name_full); 
  }
  else {
    // More models: change the test by the size comparaison (cf. models-jla branch)
    error("Bad input file");
    exit(1);
  }

  return 0;
}
