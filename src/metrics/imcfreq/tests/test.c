#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/types.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/hardware/topology.h>
#include <daemon/local_api/eard_api.h>
#include <metrics/imcfreq/imcfreq.h>

state_t eards_connection()
{
	application_t my_dummy_app;
	my_dummy_app.job.id = 0;
	my_dummy_app.job.step_id = 0;
	return eards_connect(&my_dummy_app);
}

int main(int argc, char *argv[])
{
	topology_t tp;
	state_t    s;
	imcfreq_t *samp2;
	imcfreq_t *samp1;
	ulong     *freq_list;
	ulong      freq_avg;
	uint       dev_count;
	uint       api;
	int        i = 0;

	eards_connection();

	topology_init(&tp);
    
	imcfreq_load(&tp, EARD);
	imcfreq_get_api(&api);
	apis_print(api, "MET_IMFREQ api: ");

	state_assert(s, imcfreq_init(no_ctx), return 0);
	
	state_assert(s, imcfreq_count_devices(no_ctx, &dev_count), return 0);
	printf("MET_IMCFREQ devices: %u\n", dev_count);
	
	state_assert(s, imcfreq_data_alloc(&samp2, empty),      return 0);
	state_assert(s, imcfreq_data_alloc(&samp1, &freq_list), return 0);

	// Main loop	
	state_assert(s, imcfreq_read(no_ctx, samp1), return 0);

	while (i < 5) {
		sleep(1);
		if (state_ok(imcfreq_read_copy(no_ctx, samp2, samp1, freq_list, &freq_avg))) {
			imcfreq_data_print(freq_list, &freq_avg, fderr);
		}
		++i;
	}

	eards_disconnect();

	return 0;
}
