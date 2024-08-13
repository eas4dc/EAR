/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

// #define SHOW_DEBUGS 1

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/output/debug.h>
#include <common/system/plugin_manager.h>
#include <common/config.h>
#include <common/system/popen.h>
#include <data_center_monitor/plugins/conf.h>
#include <data_center_monitor/plugins/nodesensors.h>

// Separator character used by nodesensors command
static char sep=',';

// EDCMON configuration : list of pdus, sensors, etc
static conf_t                   *conf;
static edcmon_t * edcmons = NULL;
static uint       num_edcmons = 0;
static uint*      enabled;
static uint       total_enabled = 0;

// Use the absolute path because otherwise popen fails
static char *command="/opt/confluent/bin/nodesensors -c ";
//
static char **sensor_name;

static char **nodelist;
static popen_t nodesensor_p;
static pdu_t *type;
static uint  *num_pdus;


#define QUERY_SIZE 4096
static char **query;
static char my_hostname[256];





static cluster_sensors_t edcmon_info;

declr_up_get_tag ()
{
    *tag = "nodesensors";
    *tags_deps = "!conf";
}

static void static_init(uint i)
{
    debug("Preparing query [%u]........", i);
    // Create command query
    snprintf(query[i],QUERY_SIZE,"%s %s %s", command, nodelist[i], sensor_name[i]);
    debug("Current Command to be executed: \'%s\'", query[i]);

}

declr_up_action_init(_nodesensors)
{
    // We expose this information
    *data_alloc = &edcmon_info;

    debug("Initializing nodesensors tag");

    for (uint i = 0; i < total_enabled; i++) static_init(i);
    return rsprintf("nodesensors action init(_nodesensors): received tag %s", tag);
}


static void compute_num_puds(char *src_list, uint *elems)
{
	char *next;
	char *alloc;
	char *list;
	*elems = 0;

	debug("Parsing list %s", src_list);
	if (src_list == NULL) 			return;
	if (strlen(src_list) == 0) 	return;
	list = calloc(strlen(src_list)+1, sizeof(char));
	strcpy(list, src_list);
	alloc = list;

	next = strtok(list, ",");
	while(next != NULL){
		*elems = *elems + 1;
		next = strtok(NULL, ",");
	}
	debug("%u pdus found in %s", *elems, src_list);
	free(alloc);
}

/* We use the configuration from conf plugin (ear.conf) */
declr_up_action_init(_conf)
{
    conf = (conf_t *) data;

    debug("Initializing conf tag");
		// We get my name to look for my queries
		if (gethostname(my_hostname, sizeof(my_hostname)) < 0) {
    	error("Error getting node name (%s)", strerror(errno));
  	}

  	strtok(my_hostname, ".");

	debug("Server name is %s", my_hostname);

		edcmons     = conf->cluster.edcmon_tags;
		num_edcmons = conf->cluster.num_edcmons_tags;	

	for (uint edc = 0; edc < num_edcmons; edc++)	print_edcmon_tags_conf(&edcmons[edc], edc);	
		// server is the edcmon hostname	
		strncpy(edcmon_info.server, my_hostname, sizeof(edcmon_info.server));

		if (num_edcmons == 0){
			return rsprintf("[D] No EDCMONS sensors configured, plugin disabled");
		}


		debug("Nodesensors: %u configurations detected", num_edcmons);
		enabled = calloc(num_edcmons, sizeof(uint));
		for (uint i = 0; i < num_edcmons; i++){
			// This version only support 1 tag
			enabled[i] = ((strcmp(edcmons[i].host, my_hostname) == 0) || (strcmp(edcmons[i].host, "any") == 0));
			debug("EDMON[%u] host %s my host %s, enabled %u", i, edcmons[i].host, my_hostname, enabled[i]);
			if (enabled[i]) total_enabled++;
		}

		// This version only supports 1 set of query per node
		if (!total_enabled){
			return rsprintf("[D] Number of actions 0 or not supported %u, plugin disabled", total_enabled);
		}

		query = (char **)calloc(total_enabled, sizeof(char *));
		if (!query) return rsprintf("[D] Not enough memory, disabled");
		num_pdus    = (uint *)calloc(total_enabled, sizeof(uint));
		if (!num_pdus) return rsprintf("[D] Not enough memory, disabled");


		for (uint i = 0; i < total_enabled; i++) query[i] = (char *)calloc(QUERY_SIZE, sizeof(char));
		// Error control

		sensor_name = (char **)calloc(total_enabled, sizeof(char *));
		nodelist    = (char **)calloc(total_enabled, sizeof(char *));
		type        = (pdu_t *)calloc(total_enabled, sizeof(pdu_t *));
		num_pdus    = (pdu_t *)calloc(total_enabled, sizeof(pdu_t *));
		// Error control
		uint idx = 0;
		for (uint i = 0; i < num_edcmons; i++){ 
			if (enabled[i]){ 
				sensor_name[idx] = edcmons[i].sensors_list;
				nodelist[idx]    = edcmons[i].pdu_list;
				type[idx]        = edcmons[i].type;
				compute_num_puds(nodelist[idx], &num_pdus[idx]);
				idx++;
			}
		}


		uint total_sources = 0;
	 	for (uint i = 0; i < total_enabled; i++)
		{
			total_sources += num_pdus[i];
			verbose(0,"[%u] Sensor list %s pdus %s type %u", i, sensor_name[i] , nodelist[i], type[i]);
		}	
		if (total_sources > MAX_SOURCES){
			verbose(0,"[D] ERROR, max sources exceeded (curr %u limit %u)", total_sources,MAX_SOURCES);		
		}


    return rsprintf("nodesensors action init(_nodesensors): received tag %s, %u groups enabled", tag, total_enabled);
}


static void convert_double(double *info, char *data_in)
{
	if (strcmp(data_in, "N/A") == 0){
		// 0 means not available. Should we use another value? Can be the same for all the sensors? 
		*info = 0;
	}else{
		*info = strtod(data_in, NULL);
	}
}

static state_t static_read_power(uint i, uint *num_rows, uint expected_items)
{
	// Execute the query
    char *node;
    char *power;
    uint it = 0;
    uint idx = edcmon_info.num_items;

    uint no_header = 1;
    uint one_shot  = 0;

    debug("EDCMON Item[%u] '%s'", i, query[i]);
    if (popen_open(query[i], no_header, one_shot, &nodesensor_p) != EAR_SUCCESS){
       debug("popen failed\n");
       return EAR_ERROR;
    }
    // Parse data: An external loop is needed since popen returns if there is no data, then
    // in case the command has not reported yet the information, the inner loop will finish with
    // no data. It is a busy waiting loop assuming the command will not last for too much time
    while(it < expected_items){
    while(popen_read2(&nodesensor_p, sep,  "ss", &node,&power)) {
            debug("RETURNED: node %s power %s\n", node, power);
	    strncpy(edcmon_info.edcmon_data[it+idx].nodename, node, sizeof(edcmon_info.edcmon_data[it+idx].nodename));

	    // Sensors units are still hardcoded. We could implement a discovery method
	    convert_double(&edcmon_info.edcmon_data[it+idx].power, power);
	    edcmon_info.edcmon_data[it+idx].timestamp  = time(NULL);
	    edcmon_info.edcmon_data[it+idx].type       = type[i];
	    debug("nodesensors[%u] time %u node %s power %lf W ", it+idx, (uint)edcmon_info.edcmon_data[it+idx].timestamp, edcmon_info.edcmon_data[it+idx].nodename, edcmon_info.edcmon_data[it+idx].power);
	    it++;
    }
    }
    *num_rows = it;
    // Close
    popen_close(&nodesensor_p);
		return EAR_SUCCESS;
}

declr_up_action_periodic(_nodesensors)
{
    	debug("nodesensors action");
			uint curr_items;

			edcmon_info.num_items = 0;
    	// Get instanteneous metrics
    	for (uint i = 0; i < total_enabled; i++){
    		static_read_power(i, &curr_items, num_pdus[i]);
    		debug("TYPE[%u] %u items detected", i, curr_items);
				edcmon_info.num_items += curr_items;
			}

    	return rsprintf("nodesensors action periodic(_nodesensors): received tag %s", tag);
}


#if TEST
void main(int argc, char *argv[])
{
	uint num_items;
	static_init();
	static_read_power(&num_items, NUM_PDUs);
	printf("%u items detected", num_items);
}
#endif
