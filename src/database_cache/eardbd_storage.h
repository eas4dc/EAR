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

#ifndef EAR_EARDBD_STORAGE_H
#define EAR_EARDBD_STORAGE_H

/* Functions */
void reset_all();

void reset_indexes();

void insert_hub(uint option, uint reason);

void storage_sample_add(char *buf, ulong len, ulong *idx, char *cnt, size_t siz, uint opt);

void storage_sample_receive(int fd, packet_header_t *header, char *content);

#endif //EAR_EARDBD_STORAGE_H
