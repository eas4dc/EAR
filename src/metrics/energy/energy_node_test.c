/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <metrics/energy/energy_node.h>
#include <common/system/monitor.h>

//gcc -I ../../ -o test energy_node_test.c ../libmetrics.a ../../common/libcommon.a -ldl -lpthread -rdynamic
//sudo ./test /ear_install_path/lib/plugins energy_inm_power.so

#define SHOW_WARNINGS 1

int main(int argc, char *argv[])
{
	cluster_conf_t conf;
	edata_t energy_mj_init, energy_mj_end;
	ulong energy_mj_diff;
	ehandler_t eh;
	state_t s;
	size_t node_size;
	uint   node_units;
	uint wait_period = 5;

	char epath[256];

	if (argc < 3) {
		printf("usage:%s /ear_install_path/lib/plugins energy_plugin.so\n",argv[0]);
		return 0;
	}

	strcpy(conf.install.dir_plug, argv[1]);
	strcpy(conf.install.obj_ener, argv[2]);

	monitor_init();

	if(state_fail(energy_init(&conf, &eh))) {
		printf("energy_init failed: %s", state_msg);
		return EXIT_FAILURE;
	}

	energy_units(&eh, &node_units);
  energy_datasize(&eh, &node_size);

	printf("Energy units %u Energy size %u\n", node_units, node_size);
	
	energy_mj_init = malloc(node_size);
	energy_mj_end  = malloc(node_size);
	// energy_mj_diff = (edata_t *)malloc(node_size);

	if (state_fail(energy_dc_read(&eh, energy_mj_init))) {
		warning("First energy reading failed.");
	}
	char energy_str[32];
	energy_to_str(&eh, energy_str, energy_mj_init);
	printf("Energy init: %s\n", energy_str);

	uint i = 0;
	while (i < 10) {
		i++;
	  sleep(wait_period);

	  if (state_fail(energy_dc_read(&eh, energy_mj_end))) {
			warning("Energy reading failed.");
		}

		energy_to_str(&eh, energy_str, energy_mj_end);
		printf("Energy end: %s\n", energy_str);

	  energy_accumulated(NULL, &energy_mj_diff, energy_mj_init, energy_mj_end);
	  printf("Total energy %lu mj. Avg Power %.2lf W\n",
		       energy_mj_diff, ((double)energy_mj_diff/(double)(node_units*wait_period)));

		memcpy((char *)energy_mj_init, (char *)energy_mj_end, node_size);
	}

	if (state_fail(energy_dispose(&eh))) {
		warning("Energy dispose failed.");
	}

	return EXIT_SUCCESS;
}
