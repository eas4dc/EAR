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

#ifndef METRICS_BANDWIDTH_INTEL63_H
#define METRICS_BANDWIDTH_INTEL63_H

#include <metrics/bandwidth/bandwidth.h>

// HASWELL_X & BROADWELL_X
static short HASWELL_X_IDS[]      = { 0x2FB4, 0x2FB5, 0x2FB0, 0x2FB1, 0x2FD4, 0x2FD5, 0x2FD0, 0x2FD1 };
static char HASWELL_X_STA_CTL[]   = { 0xF4 };
static int  HASWELL_X_STA_CMD[]   = { 0x30000 };
static char HASWELL_X_STO_CTL[]   = { 0xF4 };
static int  HASWELL_X_STO_CMD[]   = { 0x30100 };
static char HASWELL_X_RST_CTL[]   = { 0xF4, 0xD8, 0xDC };
static int  HASWELL_X_RST_CMD[]   = { 0x30102, 0x420304, 0x420C04 };
static char HASWELL_X_RED_CTR[]   = { 0xA0, 0xA8 };
static char HASWELL_X_DEVICES[]   = { 0x14, 0x15, 0x17, 0x18 };
static char HASWELL_X_FUNCTIONS[] = { 0x00, 0x01 };
static int  HASWELL_X_N_FUNCTIONS = 8;
// SKYLAKE_X
static short SKYLAKE_X_IDS[]      = { 0x2042, 0x2046, 0x204A };
static char SKYLAKE_X_DEVICES[]   = { 0x0A, 0x0B, 0x0C, 0x0D };
static char SKYLAKE_X_REGISTERS[] = { 0xF8, 0xF4, 0xD8, 0xA0 };
static char SKYLAKE_X_FUNCTIONS[] = { 0x02, 0x06 };
static int  SKYLAKE_X_N_FUNCTIONS = 6;

state_t bwidth_intel63_status(topology_t *tp);

state_t bwidth_intel63_init(ctx_t *c, topology_t *tp);

state_t bwidth_intel63_dispose(ctx_t *c);

state_t bwidth_intel63_count(ctx_t *c, uint *count);

state_t bwidth_intel63_start(ctx_t *c);

state_t bwidth_intel63_reset(ctx_t *c);

state_t bwidth_intel63_stop(ctx_t *c, ullong *cas);

state_t bwidth_intel63_read(ctx_t *c, ullong *cas);

#endif
