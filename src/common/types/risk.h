/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef RISK_LEVEL_H
#define RISK_LEVEL_H

#include <common/states.h>
#define WARNING1 0x01
#define WARNING2 0x02
#define PANIC    0x04

#define ENERGY   0x01
#define POWER    0x02

typedef unsigned int risk_t;
state_t set_risk(risk_t *r, risk_t new_r);
int is_risk_set(risk_t r, risk_t value);
state_t add_risk(risk_t *r, risk_t value);
state_t del_risk(risk_t *r, risk_t value);

risk_t get_risk(char *risk);
unsigned int get_target(char *target);

#endif
