// /hpc/base/ctt/packages/compiler/gnu/9.1.0/bin/gcc -I ../../ -o temp temperature.c ../libmetrics.a ../../common/libcommon.a -lpthread

#include <metrics/temperature/temperature.h>

int main()
{
	topology_t  tp;
	llong      *temp_list;
	llong       temp_average;
	uint        temp_count;
	state_t     s;
	ctx_t       c;
	int         i;

	state_assert(s, topology_init(&tp),                           return 0);
	state_assert(s, temp_load(&tp),                               return 0);
	state_assert(s, temp_init(&c),                                return 0);
	state_assert(s, temp_data_alloc(&c, &temp_list, &temp_count), return 0);
	state_assert(s, temp_read(&c, temp_list, &temp_average),      return 0);

	for (i = 0; i < temp_count; ++i) {
		printf("S%d: %lld\n", i, temp_list[i]);
	}
	printf("AVG: %lld\n", temp_average);

	state_assert(s, temp_data_free(&c, &temp_list),               return 0);
	state_assert(s, temp_dispose(&c),                             return 0);

	return 0;
}
