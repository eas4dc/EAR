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

#ifndef COMMON_UTILS_KEEPER_SAVE_H
#define COMMON_UTILS_KEEPER_SAVE_H

#include <common/types.h>

void keeper_save_text(const char *id, char *value);
void keeper_save_int32(const char *id, int value);
void keeper_save_int64(const char *id, llong value);
void keeper_save_uint64(const char *id, ullong value);
void keeper_save_float32(const char *id, float value);
void keeper_save_float64(const char *id, double value);
int  keeper_load_text(const char *id, char *value);
int  keeper_load_int32(const char *id, int *value);
int  keeper_load_int64(const char *id, llong *value);
int  keeper_load_uint64(const char *id, ullong *value);
int  keeper_load_float32(const char *id, float *value);
int  keeper_load_float64(const char *id, double *value);

#endif //COMMON_UTILS_KEEPER_H
