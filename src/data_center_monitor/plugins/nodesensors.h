/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _NODESENSORS_H_
#define _NODESENSORS_H_

#include <time.h>
#include <sys/types.h>
#include <common/types/configuration/cluster_conf_edcmon_tag.h>

typedef enum {
  POWER_W      = 0,
  HUMIDITY_PERC,
  TEMPERATURE_F,
}esensor_t;

// MAX_SOURCES must be the maximum number of PDUs, IPs etc.
#define MAX_SOURCES 128


typedef struct nodesesor{
        time_t    timestamp;
        char      nodename[MAX_PATH_SIZE]; // PDU IP
				pdu_t     type;
        double    power;
#if 0
	double    humidity;
	double    temperature;
#endif
}nodesensor_t;

// This is the aggregated information. for if type , one nodesensor_t per pdu
typedef struct cluster_sensors{
	nodesensor_t    edcmon_data[MAX_SOURCES];
	char    				server[MAX_PATH_SIZE];
	uint 						num_items;
}cluster_sensors_t;

#endif
