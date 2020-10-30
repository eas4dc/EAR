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

#include <metrics/energy/energy_node.h>

//gcc -I ../../ -o test energy_node_test.c ../libmetrics.a ../../common/libcommon.a -ldl && sudo ./test $PWD/.. energy_nm.so

int main(int argc, char *argv[])
{
	cluster_conf_t conf;
	edata_t energy_mj;
	ehandler_t eh;
	state_t s;

	if (argc < 3) {
		return 0;
	}

	strcpy(conf.install.dir_plug, argv[1]);
	strcpy(conf.install.obj_ener, argv[2]);

	//
	s = energy_init(&conf, &eh);
	printf("energy_init %d\n", s);

	//
	s = energy_dc_read(&eh, energy_mj);
	printf("energy_dc_read %d\n", s);

	//
	s = energy_dispose(&eh);
	printf("energy_dispose %d\n", s);

	return 0;
}
