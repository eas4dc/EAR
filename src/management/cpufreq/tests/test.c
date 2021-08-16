#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/types.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/hardware/topology.h>
#include <management/cpufreq/cpufreq.h>

int main(int argc, char *argv[])
{
	topology_t tp;
	state_t    s;
	pstate_t  *available_list;
	pstate_t  *current_list;
	uint      *index_list;
	uint       available_count;
	uint       current_count;
	ctx_t      c;
	int        i;

	state_assert(s, topology_init(&tp),    return 0);
	state_assert(s, mgt_cpufreq_load(&tp), return 0);
	state_assert(s, mgt_cpufreq_init(&c),  return 0);
	state_assert(s, mgt_cpufreq_alloc_available(&c, &available_list, &available_count),        return 0);
	state_assert(s, mgt_cpufreq_alloc_current(&c, &current_list, &index_list, &current_count), return 0);
	state_assert(s, mgt_cpufreq_get_available_list(&c, available_list),                        return 0);
	state_assert(s, mgt_cpufreq_get_current_list(&c, current_list),                            return 0);
	state_assert(s, mgt_cpufreq_dispose(&c),                                                   return 0);

	printf("---------------------------\n");
	printf("-- Available frequencies --\n");
	printf("---------------------------\n");
	for (i = 0; i < available_count; ++i) {
		printf("P%u:%llu\n", available_list[i].idx, available_list[i].khz);
	}
	printf("---------------------------\n");
	printf("-- Current frequencies ----\n");
	printf("---------------------------\n");
	for (i = 0; i < tp.cpu_count; ++i) {
		printf("CPU%u:%llu\n", i, current_list[i].khz);
	}

	return 0;
}
