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
