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
