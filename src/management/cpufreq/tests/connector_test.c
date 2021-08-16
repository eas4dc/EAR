//#define SHOW_DEBUGS 1
#define EAR_CPUPOWER 1
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/types.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/hardware/topology.h>
#if CONNECTOR
#include <management/cpufreq/frequency.h>
#else
#include <management/cpufreq/frequency.h>
#endif
#include <common/output/debug.h>

#if CONNECTOR
#define CALL(f) \
	debug("calling " #f); \
	f
//	_ ## f
#define RCALL(r, f) \
	debug("calling " #f); \
	r = f
//	r = _ ## f
#else
#define CALL(f) \
	debug("calling " #f); \
	f
#define RCALL(r, f) \
	debug("calling " #f); \
	r = f
#endif

static governor_t govt;
static char opt1[SZ_PATH];
static char cmnd[SZ_PATH];
static topology_t topo;
static ulong retlonglist[256];
static ulong *retplong;
static ulong retlong;
static cpu_set_t mask;
static int retint;
static state_t s;
static int i;

int is(char *buf, char *string)
{
    return strcmp(buf, string) == 0;
}

int main(int argc, char *argv[])
{
	state_assert(s, topology_init(&topo), );

	while (1)
	{
		debug("command: ");
		scanf("%s", cmnd);

		if (is(cmnd, "init")) {
			RCALL(retint, frequency_init(topo.cpu_count));
			debug("frequency_init returned %d (%s)", retint, state_msg);
		} else if (is(cmnd, "disp")) {
			CALL(frequency_dispose());
		} else if (is(cmnd, "1")) {
			RCALL(retint, frequency_get_num_pstates());
			debug("frequency_get_num_pstates returned %d", retint);
		} else if (is(cmnd, "2")) { 
			scanf("%s", opt1);
			RCALL(retlong, frequency_get_cpu_freq((uint) atoi(opt1)));
			debug("frequency_get_cpu_freq returned %lu", retlong);
		} else if (is(cmnd, "3")) {
			RCALL(retlong, frequency_get_nominal_freq());
			debug("frequency_get_nominal_freq returned %lu", retlong);
		} else if (is(cmnd, "4")) {
			RCALL(retlong, frequency_get_nominal_pstate());
			debug("frequency_get_nominal_pstate returned %lu", retlong);
		} else if (is(cmnd, "5")) {
			RCALL(retint, frequency_get_num_pstates());
			debug("frequency_get_num_pstates returned %d", retint);
			RCALL(retplong, frequency_get_freq_rank_list()); 
			for (i = 0; i < retint; ++i) {
				debug("frequency_get_freq_rank_list %d:%lu", i, retplong[i]);
			}
		} else if (is(cmnd, "6")) { 
			scanf("%s", opt1);
			RCALL(retlong, frequency_pstate_to_freq((uint) atoi(opt1)));
			debug("frequency_pstate_to_freq returned %lu", retlong);
		} else if (is(cmnd, "7")) { 
			scanf("%s", opt1);
			RCALL(retint, frequency_freq_to_pstate((ulong) atol(opt1)));
			debug("frequency_freq_to_pstate returned %d", retint);
		} else if (is(cmnd, "8")) {
			RCALL(retint, frequency_get_num_pstates());
			debug("frequency_get_num_pstates returned %d", retint);
			RCALL(retplong, frequency_get_freq_rank_list());
			scanf("%s", opt1);
			RCALL(retlong, frequency_pstate_to_freq_list((uint) atoi(opt1), retplong, retint));
			debug("frequency_pstate_to_freq_list returned %lu", retlong);
		} else if (is(cmnd, "9")) {
			RCALL(retint, frequency_get_num_pstates());
			debug("frequency_get_num_pstates returned %d", retint);
			RCALL(retplong, frequency_get_freq_rank_list());
			scanf("%s", opt1);
			RCALL(retlong, frequency_freq_to_pstate_list((ulong) atol(opt1), retplong, retint));
			debug("frequency_freq_to_pstate_list returned %lu", retlong);
		} else if (is(cmnd, "10")) {
			scanf("%s", opt1);
			RCALL(retint, frequency_is_valid_frequency((ulong) atol(opt1)));
			debug("frequency_is_valid_frequency returned %d", retint);
		} else if (is(cmnd, "11")) {
			scanf("%s", opt1);
			RCALL(retint, frequency_is_valid_pstate((uint) atoi(opt1)));
			debug("frequency_is_valid_pstate returned %d", retint);
		} else if (is(cmnd, "12")) {
			scanf("%s", opt1);
			RCALL(retint, frequency_closest_pstate((ulong) atol(opt1)));
			debug("frequency_closest_pstate returned %d", retint);
		} else if (is(cmnd, "13")) { 
			scanf("%s", opt1);
			RCALL(retlong, frequency_closest_frequency((ulong) atol(opt1)));
			debug("frequency_closest_frequency returned %lu", retlong);
		} else if (is(cmnd, "14")) {
			CALL(get_governor(&govt));
			debug("get_governor returned %s, %lu, %lu", govt.name, govt.max_f, govt.min_f);
		} else if (is(cmnd, "15")) {
			CALL(frequency_set_performance_governor_all_cpus());
			CALL(get_governor(&govt));
			debug("frequency_set_performance_governor_all_cpus returned %s, %lu, %lu", govt.name, govt.max_f, govt.min_f);
		} else if (is(cmnd, "16")) {
			CALL(frequency_set_userspace_governor_all_cpus());
			CALL(get_governor(&govt));
			debug("frequency_set_userspace_governor_all_cpus returned %s, %lu, %lu", govt.name, govt.max_f, govt.min_f);
		} else if (is(cmnd, "17")) {
			CALL(get_governor(&govt));
			scanf("%s", opt1);
			strcpy(govt.name, opt1);
			CALL(set_governor(&govt));
		} else if (is(cmnd, "18")) {
			scanf("%s", opt1);
			RCALL(retlong, frequency_set_all_cpus((ulong) atol(opt1)));
			debug("frequency_set_all_cpus returned %lu", retlong);
		} else if (is(cmnd, "19")) {
			scanf("%s", opt1);
			CPU_ZERO(&mask);
			CPU_SET(atoi(opt1), &mask);
			scanf("%s", opt1);
			RCALL(retlong, frequency_set_with_mask(&mask, (ulong) atol(opt1)));
			debug("frequency_set_with_mask returned %lu", retlong);
		} else if (is(cmnd, "20")) {
			scanf("%s", opt1);
			RCALL(retlong, frequency_closest_high_freq((ulong) atol(opt1), 1));
			debug("frequency_closest_high_freq returned %lu", retlong);
		} else if (is(cmnd, "21")) {
			RCALL(retplong, frequency_get_freq_rank_list()); 
			for (i = 0; i < 20; ++i) {
				retplong[i]    = 2400000LU;
				retplong[i+20] = 1000000LU;
				retplong[i+40] = 2400000LU;
				retplong[i+60] = 1000000LU;
			}
			RCALL(retlong, frequency_set_with_list(0, retplong));
			debug("frequency_set_with_list returned %lu", retlong);
		} else if (is(cmnd, "22")) {
			RCALL(retlong, frequency_get_cpufreq_list(topo.cpu_count, retlonglist));
			debug("frequency_get_cpufreq_list returned %lu", retlong);
			for (i = 0; i < topo.cpu_count; ++i) {
				debug("frequency_get_cpufreq_list %d:%lu", i, retlonglist[i]);
			}
		} else if (is(cmnd, "23")) {
			scanf("%s", opt1);
			retint = atoi(opt1);
			scanf("%s", opt1);
			RCALL(retlong, frequency_set_cpu((ulong) atol(opt1), retint));
			debug("frequency_set_cpu returned %lu", retlong);
		}
	}

	// src/tools/coeffs_compute.c
	// src/common/types/configuration/policy_conf.c
	// src/common/hardware/architecture.c

	return 0;
}
