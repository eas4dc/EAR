/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/coefficient.h>

#define PERMISSION  S_IRUSR  | S_IWUSR | S_IRGRP | S_IROTH
#define OPTIONS     O_WRONLY | O_CREAT | O_TRUNC | O_APPEND

int coeff_file_size(char *path)
{
    int ret, fd;

    if ((fd = open(path, O_RDONLY)) < 0) {
        return EAR_OPEN_ERROR;
    }

    ret = lseek(fd, 0, SEEK_END);
	close(fd);

	return ret;
}

int coeff_file_read_no_alloc(char *path, coefficient_t *coeffs, int size)
{
    int ret, fd;

    if ((fd = open(path, O_RDONLY)) < 0) {
        return EAR_OPEN_ERROR;
    }

    if ((ret = read(fd, coeffs, size)) != size)
    {
        close(fd);
        return EAR_READ_ERROR;
    }
    close(fd);

    return (size / sizeof(coefficient_t));
}

int coeff_gpu_file_read_no_alloc(char *path, coefficient_gpu_t *coeffs, int size)
{
    int ret, fd;

    if ((fd = open(path, O_RDONLY)) < 0) {
        return EAR_OPEN_ERROR;
    }

    if ((ret = read(fd, coeffs, size)) != size)
    {
        close(fd);
        return EAR_READ_ERROR;
    }
    close(fd);

    return (size / sizeof(coefficient_t));
}

void coeff_print(coefficient_t *coeff)
{
    verbose(VTYPE,"ref %lu pstate %lu avail %u A %lf B %lf C %lf D %lf E %lf F %lf",
    coeff->pstate_ref,coeff->pstate,coeff->available,coeff->A,coeff->B,coeff->C,coeff->D,coeff->E,coeff->F);
}

void coeff_reset(coefficient_t *coeff)
{
	coeff->available=0;
}

int coeff_file_read(char *path, coefficient_t **coeffs)
{
    coefficient_t *coeffs_aux;
    int size, ret, fd;

    if ((fd = open(path, O_RDONLY)) < 0) {
        return EAR_OPEN_ERROR;
    }

    size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    /* Allocating memory*/
    coeffs_aux = (coefficient_t *) malloc(size);

    if (coeffs_aux == NULL)
    {
        close(fd);
        return EAR_ALLOC_ERROR;
    }

    /* Reset the memory to zeroes*/
    memset(coeffs_aux, 0, sizeof(coefficient_t));

    if ((ret = read(fd, coeffs_aux, size)) != size)
    {
        close(fd);
        free(coeffs_aux);
        return EAR_READ_ERROR;
    }
    close(fd);
    
	*coeffs = coeffs_aux;
    return (size / sizeof(coefficient_t));
}
