/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_UTILS_KEEPER_SAVE_H
#define COMMON_UTILS_KEEPER_SAVE_H

#include <common/types.h>

void keeper_save_text(const char *id, char *value);
void keeper_save_int32(const char *id, int value);
void keeper_save_int64(const char *id, llong value);
void keeper_save_uint32(const char *id, uint value);
void keeper_save_uint64(const char *id, ullong value);
void keeper_save_float32(const char *id, float value);
void keeper_save_float64(const char *id, double value);
void keeper_save_auint32(const char *id, uint *list, uint list_length);
void keeper_save_auint64(const char *id, ullong *list, uint list_length);
int keeper_load_text(const char *id, char *value);
int keeper_load_int32(const char *id, int *value);
int keeper_load_int64(const char *id, llong *value);
int keeper_load_uint32(const char *id, uint *value);
int keeper_load_uint64(const char *id, ullong *value);
int keeper_load_float32(const char *id, float *value);
int keeper_load_float64(const char *id, double *value);
int keeper_load_auint32(const char *id, uint **list, uint *list_length);
int keeper_load_auint64(const char *id, ullong **list, uint *list_length);

#define keeper_macro(suffix, id, value)                                                                                \
    if (!keeper_load_##suffix(id, &value)) {                                                                           \
        keeper_save_##suffix(id, value);                                                                               \
    }

#endif // COMMON_UTILS_KEEPER_H