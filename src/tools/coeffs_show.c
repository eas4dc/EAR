/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#define SHOW_DEBUG 1
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/system/file.h>
#include <common/types/coefficient.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <library/models/cpu_power_model_default.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Reports coefficients in stdout and csv file
 **/

void print_basic_coefficients(coefficient_t *avg, int n_pstates, char *csv_output)
{
    int i;
    FILE *fd;
    coefficient_t *coeff;

    fd = fopen(csv_output, "w");

    fprintf(fd, "FROM\tTO\tA\tB\tC\tD\tE\tF\n");
    verbose(2, "FROM\tTO\tA\tB\tC\tD\tE\tF");
    for (i = 0; i < n_pstates; i++) {
        coeff = &avg[i];
        if (coeff->available) {
            verbose(2, "%lu\t%lu\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf", coeff->pstate_ref, coeff->pstate, coeff->A, coeff->B,
                    coeff->C, coeff->D, coeff->E, coeff->F);

            fprintf(fd, "%lu\t%lu\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", coeff->pstate_ref, coeff->pstate, coeff->A,
                    coeff->B, coeff->C, coeff->D, coeff->E, coeff->F);
        }
    }
    fclose(fd);
}

void print_cpu_model_coefficients(intel_skl_t *coeff, char *csv_output)
{
    FILE *fd;
    fd = fopen(csv_output, "w");

    verbose(2, "IPC\tGBS\tVPI\tF\tINTER");
    verbose(2, "%lf\t%lf\t%lf\t%lf\t%lf", coeff->ipc, coeff->gbs, coeff->vpi, coeff->f, coeff->inter);

    fprintf(fd, "IPC\tGBS\tVPI\tF\tINTER\n");
    fprintf(fd, "%lf\t%lf\t%lf\t%lf\t%lf\n", coeff->ipc, coeff->gbs, coeff->vpi, coeff->f, coeff->inter);

    fclose(fd);
}

void print_gpu_coefficients(coefficient_gpu_t *avg, int n_pstates)
{
    int i;
    coefficient_gpu_t *coeff;

    verbose(2, "FROM\tTO\tA0\tA1\tA2\tA3\tB0\tB1\tB2\tB3");
    for (i = 0; i < n_pstates; i++) {
        coeff = &avg[i];
        if (coeff->available) {
            verbose(2, "%lu\t\t%lu\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf", coeff->pstate_ref, coeff->pstate,
                    coeff->A0, coeff->A1, coeff->A2, coeff->A3, coeff->B0, coeff->B1, coeff->B2, coeff->B3);
        }
    }
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
        coeffs = (intel_skl_t *) calloc(size, 1);

        if (coeffs == NULL) {
            error("not enough memory");
            exit(1);
        }

        state = ear_file_read(argv[1], (char *) coeffs, size, 0);

        if (state_fail(state)) {
            error("state id: %d (%s)", state, state_msg);
            exit(1);
        }

        csv_name      = "coeffs.cpu_model";
        csv_name_full = malloc(strlen(csv_name) + strlen(tag) + 4);

        strcpy(csv_name_full, csv_name);
        strcat(csv_name_full, tag);
        strcat(csv_name_full, ext);

        print_cpu_model_coefficients(coeffs, csv_name_full);

    } else if (size % sizeof(coefficient_t) == 0) {
        // Basic model coeffs
        n_pstates = size / sizeof(coefficient_t);
        debug("Basic Model coefficients");
        debug("coefficients file size: %d", size);
        debug("number of P_STATES: %d", n_pstates);

        coefficient_t *coeffs;
        coeffs = (coefficient_t *) calloc(size, 1);

        if (coeffs == NULL) {
            error("not enough memory");
            exit(1);
        }

        state = ear_file_read(argv[1], (char *) coeffs, size, 0);

        if (state_fail(state)) {
            error("state id: %d (%s)", state, state_msg);
            exit(1);
        }

        csv_name      = "coeffs";
        csv_name_full = malloc(strlen(csv_name) + strlen(tag) + 4);
        strcpy(csv_name_full, csv_name);
        strcat(csv_name_full, tag);
        strcat(csv_name_full, ext);
        print_basic_coefficients(coeffs, n_pstates, csv_name_full);
    } else if (size % sizeof(coefficient_gpu_t) == 0) {
        // GPU model coeffs
        n_pstates = size / sizeof(coefficient_gpu_t);
        printf("GPU Model coefficients\n");
        printf("coefficients file size: %d\n", size);
        printf("number of P_STATES: %d\n", n_pstates);

        coefficient_gpu_t *coeffs;
        coeffs = (coefficient_gpu_t *) calloc(size, 1);

        if (coeffs == NULL) {
            error("not enough memory");
            exit(1);
        }

        state = ear_file_read(argv[1], (char *) coeffs, size, 0);

        if (state_fail(state)) {
            error("state id: %d (%s)", state, state_msg);
            exit(1);
        }

        print_gpu_coefficients(coeffs, n_pstates);
    }

    else {
        // More models: change the test by the size comparaison (cf. models-jla branch)
        error("Bad input file");
        exit(1);
    }

    return 0;
}
