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

#ifndef RISK_LEVEL_H
#define RISK_LEVEL_H

#include <common/states.h>
#define WARNING1    0x01
#define WARNING2    0x02
#define PANIC       0x04

#define ENERGY  0x01
#define POWER   0x02

typedef unsigned int risk_t;
state_t set_risk(risk_t *r,risk_t new_r);
int is_risk_set(risk_t r,risk_t value);
state_t add_risk(risk_t *r,risk_t value);
state_t del_risk(risk_t *r,risk_t value);

risk_t get_risk(char *risk);
unsigned int get_target(char *target);

#endif

