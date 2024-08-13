/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_EARDBD_STORAGE_H
#define EAR_EARDBD_STORAGE_H

/* Functions */
void reset_all();

void reset_indexes();

void insert_hub(uint option, uint reason);

void storage_sample_add(char *buf, ulong len, ulong *idx, char *cnt, size_t siz, uint opt);

void storage_sample_receive(int fd, packet_header_t *header, char *content);

#endif //EAR_EARDBD_STORAGE_H
