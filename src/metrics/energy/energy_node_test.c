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

#include <metrics/energy/energy_node.h>
#include <common/system/monitor.h>

//gcc -I ../../ -o test energy_node_test.c ../libmetrics.a ../../common/libcommon.a -ldl -lpthread -rdynamic
//sudo ./test /ear_install_path/lib/plugins energy_inm_power.so

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

	s = energy_init(&conf, &eh);

	energy_units(&eh, &node_units);
  energy_datasize(&eh, &node_size);

	printf("Energy units %u Energy size %u\n", node_units, node_size);
	
	energy_mj_init = (edata_t *) malloc(node_size);
	energy_mj_end  = (edata_t *) malloc(node_size);
	energy_mj_diff = (edata_t *) malloc(node_size);

	//
	s = energy_dc_read(&eh, energy_mj_init);
	uint i = 0;
	while(i<10){
		i++;
	  sleep(wait_period);

	  s = energy_dc_read(&eh, energy_mj_end);
	  energy_accumulated(NULL, &energy_mj_diff, energy_mj_init, energy_mj_end);
	  printf("Total energy %lu mj. Avg Power %.2lf W\n", energy_mj_diff, ((double)energy_mj_diff/(double)(node_units*wait_period)));

		memcpy((char *)energy_mj_init, (char *)energy_mj_end, node_size);
	}

	//
	s = energy_dispose(&eh);
	printf("energy_dispose %d\n", s);

	return 0;
}
