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

//#define SHOW_DEBUGS 1

#include <math.h>
#include <stdlib.h>
#include <metrics/common/msr.h>
#include <common/output/debug.h>
#include <common/string_enhanced.h>
#include <common/hardware/bithack.h>
#include <common/utils/keeper.h>
#include <management/cpufreq/archs/amd17.h>
#include <metrics/common/apis.h>
#include <metrics/common/hsmp.h>

// In both modes, the P_STATE list is built using the P0 MSR register 0xc0010064.
// The list has P0 to PX, being X the last frequency found in the list given by
// the driver. From a P_STATE to next there are always a difference of 100 MHz.

typedef struct pss_s {
	ullong fid;
	ullong did;
	ullong cof;
} pss_t;

#define CPU				0
#define MAX_PSTATES		128
#define MAX_REGISTERS	8
#define REG_HWCONF		0xc0010015
#define REG_LIMITS		0xc0010061
#define REG_CONTROL		0xc0010062
#define REG_STATUS		0xc0010063
#define REG_P0			0xc0010064
#define REG_P1			0xc0010065

#define p0_khz(cof) ((cof * 1000LLU) + 1000LLU)
#define p1_khz(cof) ((cof * 1000LLU))


#define READ_BOOST_LIMIT	0x0A  //gets the frequency limit currently enforced or Fmax
#define WRITE_BOOST_LIMIT 	0x08  //sets the frequency limit 

static topology_t           tp;
static ctx_t                driver_c;
static mgt_ps_driver_ops_t *driver;
static ullong               regs[MAX_REGISTERS];
static pss_t                psss[MAX_PSTATES];
static uint                 boost_enabled;
static uint                 pss_nominal;
static uint                 pss_count;
static uint                 init;
static uint                 mode_virtual;

//
static state_t set_pstate(uint cpu, uint pst, uint test, uint set_driver);
static state_t set_pstate0(uint cpu);

// Modes:
//  - HSMP: using HSMP mailing system to change frequency. You can select whatever frequency,
//    even limit boost frequency, in the CPU you want. It has no limits.
//  - virtual: it changes P1 MSR register. You are limited to P0 and P1 per CPU, and P1 frequency
//    value is shared between all CPUs in a socket. It has a lot of limitations.

state_t mgt_cpufreq_amd17_load(topology_t *tp_in, mgt_ps_ops_t *ops, mgt_ps_driver_ops_t *ops_driver)
{
	state_t s;
    int cond1;
    int cond2;

	debug("testing AMD17 P_STATE control status");
	#if AMD_OSCPUFREQ
	return EAR_ERROR;
    #endif
	if (tp_in->vendor != VENDOR_AMD) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	if (tp_in->family < FAMILY_ZEN) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	// Driver is used to many things, without it this API can't do anything by now
	if (ops_driver->init == NULL) {
		return_msg(EAR_ERROR, "Driver is not available");
	}
	driver = ops_driver;
	// This is also an MSR class, without MSR this API can't do anything
	if (state_fail(s = msr_test(tp_in, MSR_WR))) {
		return s;
	}
	if (tp.cpu_count == 0) {
		if(state_fail(s = topology_copy(&tp, tp_in))) {
			return s;
		}
	}
    mode_virtual = (getenv("HACK_AMD_ZEN_VIRTUAL") != NULL);
    if (!mode_virtual) {
        if (state_fail(s = hsmp_scan(tp_in))) {
            return s;
        }
    }

    // Conditional 1, if can set frequencies
    cond1 = (!mode_virtual || driver->set_current_list != NULL);
    // Conditional 2, if can set governor
    cond2 = (driver->set_governor != NULL);
	// Setting references (if driver set is not available, this API neither)
	apis_put(ops->init,               mgt_cpufreq_amd17_init);
	apis_put(ops->dispose,            mgt_cpufreq_amd17_dispose);
	apis_put(ops->count_available,    mgt_cpufreq_amd17_count_available);
	apis_put(ops->get_available_list, mgt_cpufreq_amd17_get_available_list);
	apis_put(ops->get_current_list,   mgt_cpufreq_amd17_get_current_list);
	apis_put(ops->get_nominal,        mgt_cpufreq_amd17_get_nominal);
	apis_put(ops->get_index,          mgt_cpufreq_amd17_get_index);
    apis_pin(ops->set_current_list,   mgt_cpufreq_amd17_set_current_list, cond1);
    apis_pin(ops->set_current,        mgt_cpufreq_amd17_set_current, cond1);
    apis_pin(ops->reset,              mgt_cpufreq_amd17_reset, cond1);
	apis_put(ops->get_governor,       mgt_cpufreq_amd17_governor_get);
	apis_put(ops->get_governor_list,  mgt_cpufreq_amd17_governor_get_list);
	apis_pin(ops->set_governor,       mgt_cpufreq_amd17_governor_set, cond2);
	apis_pin(ops->set_governor_mask,  mgt_cpufreq_amd17_governor_set_mask, cond2);
	apis_pin(ops->set_governor_list,  mgt_cpufreq_amd17_governor_set_list, cond2);

	return EAR_SUCCESS;
}

static state_t static_dispose(uint close_msr_up_to, state_t s, char *msg)
{
	int cpu;
	// TODO: Recover MSR previous state
	for (cpu = 0; cpu < close_msr_up_to; ++cpu) {
		msr_close(tp.cpus[cpu].id);
	}
	if (driver != NULL) {
		driver->dispose(&driver_c);
	}
	init = 0;
	return_msg(s, msg);
}

state_t mgt_cpufreq_amd17_dispose(ctx_t *c)
{
	return static_dispose(tp.cpu_count, EAR_SUCCESS, NULL);
}

#if SHOW_DEBUGS
static void print_registers()
{
	// Look into 'PPR 17h, Models 31h.pdf' the MSRC001_006[4...B] register information.
	debug ("-----------------------------------------------------------------------------------");
	tprintf_init(STDERR_FILENO, STR_MODE_DEF, "4 18 4 8 8 11 8 7 8 10 10");
	tprintf("#||reg||en||IddDiv||IddVal||IddPerCore|||CpuVid|||CpuFid||  200||CpuDfsId||CoreCof");
	tprintf("-||---||--||------||------||----------|||------|||------||-----||--------||-------");

	int i;

	for (i = 0; i < MAX_REGISTERS; ++i)
	{
		ullong en        = getbits64(regs[i], 63, 63);
		ullong idd_div   = getbits64(regs[i], 31, 30);
		ullong idd_value = getbits64(regs[i], 29, 22);
		double idd_core  = ((double) idd_value) / pow(10.0, (double) idd_div);
		ullong vdd_cpu   = getbits64(regs[i], 21, 14);
		//double pow_core  = 1.350 - (((double) CpuVid) * 0.00625);
		ullong freq_fid  = getbits64(regs[i],  7,  0);
		ullong freq_did  = getbits64(regs[i], 13,  8);
		ullong freq_cof  = 0;

		if (freq_did > 0) {
			freq_cof = (freq_fid * 200LLU) / freq_did;
		}

		tprintf("%d||%LX||%llu||%llu||%llu||%0.2lf|||%llu|||%llu||* 200||/ %llu||= %llu",
				i, regs[i], en, idd_div, idd_value, idd_core, vdd_cpu, freq_fid, freq_did, freq_cof);
	}
	debug ("-----------------------------------------------------------------------------------");
}
#endif

static state_t build_pstate_fake_register(ullong freq_mhz, ullong *cof, ullong *fid, ullong *did)
{
	// List of predefined divisors.
	ullong _did[31] = {  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
					    18, 19, 20, 21, 22, 23, 24, 25, 26, 28,
					    30, 32, 34, 36, 38, 40, 42, 44 };
	ullong near;
	ullong diff;
	ullong m, d;
	//	
	near = UINT_MAX;
	// Maximum multipier (255) and minimum (16)
	for (m = 255; m >= 16; --m) {
		// Divisors
		for (d = 0; d < 31; ++d) {
			
			// A frequency is composed by a multiplicator and a divisor, which are set
			// in the MSR register. This function tests by brute-force, all possible
			// combinations and selects the smallest multiplicator plus divisor add. 
			// 
			// 		formula = (multiplicator / divisor) * 200
			// 		        = (fid / did) * 200
			if (((m * 1000LLU) * 200LLU) == ((freq_mhz * 1000LLU) * _did[d]))
			{
				diff = _did[d] + m;
				diff = diff * diff;

				if (diff < near) {
					*cof = freq_mhz;
					*fid = m;
					*did = _did[d];
					near = diff;
				}
			}
		}
	}
	if (near == UINT_MAX) {
		return_msg(EAR_ERROR, "impossible to build PSS");
	}
	return EAR_SUCCESS;
}

static int clean_pstate_list(uint i)
{
	int how_many;
	// If reached the deepest point then return
	if (i == pss_count) {
		return 0;
	}
	//
	how_many = clean_pstate_list(i+1);
	// If we are OK then return
	if (state_ok(set_pstate(0, i, 1, 0))) {
		return how_many+1;
	}
	// We are not OK
	debug("frequency %llu can't be written", psss[i].cof);
	// If we are not OK and the next is OK
	int h;
	for (h = 0; h < how_many; ++h) {
		memcpy(&psss[i+h], &psss[i+h+1], sizeof(pss_t));
	}
	// If we are not OK and the next is not OK
	return how_many;
}

static void print_pstate_list()
{
	int i;
	debug("Final PSS object list: ");
	for (i = 0; i < pss_count; ++i) {
		debug("P%d: %llum * 200 / %llud = %llu MHz", i,
			  psss[i].fid, psss[i].did, psss[i].cof);
	}
}

static state_t build_pstate_list()
{
	const ullong *freqs_available;
	uint freq_count;
	ullong min_mhz;
	state_t s;
	ullong p;
	int i;

	// Getting the minimum frequency specified by the driver. In the past it was 1000.
	if (state_fail(s = driver->get_available_list(&driver_c, &freqs_available, &freq_count))) {
		return static_dispose(0, s, state_msg);
	}
	min_mhz = freqs_available[freq_count-1] / 1000LLU;
	// Getting P0(b) information from the first of a list of registers. Registers are read
	// during init phase.
	psss[0].fid = getbits64(regs[0],  7,  0);
	psss[0].did = getbits64(regs[0], 13,  8);
	if (psss[0].did == 0) {
		return_msg(EAR_ERROR, "the frequency divisor is 0");
	}
	// Getting the P0(b) Core Frequency
	psss[0].cof = (psss[0].fid * 200LLU) / psss[0].did;
	// If boost is enabled, P0 is copied in P1. P1 then will have the same COF.
	i = 1;
	if (boost_enabled) {
		psss[1].cof = psss[0].cof;
		psss[1].fid = psss[0].fid;
		psss[1].did = psss[0].did;
		i = 2;
	}
	// Getting P2 (or P1 if boost is not enabled) Core Frequency (intervals of 100 MHz).
	if ((psss[0].cof % 100LLU) != 0) {
		p = psss[0].cof + (100LLU - (psss[0].cof % 100LLU)) - 100LLU;
	} else {
		p = psss[0].cof - 100LLU;
	}
	// Getting PSS objects from P1 to P7
	for (; p >= min_mhz && i < MAX_PSTATES; p -= 100, ++i) {
		if (state_fail(s = build_pstate_fake_register(p, &psss[i].cof, &psss[i].fid, &psss[i].did))) {
			return s;
		}
		debug("BUILT PS%d: %llum * 200 / %llud = %llu MHz", i, psss[i].fid, psss[i].did, psss[i].cof);
	}
	pss_count = i;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_amd17_init(ctx_t *c)
{
	ullong data;
	state_t s;
	//uint aux;
	int cpu;
	int i;

	debug("initializing AMD17 P_STATE control");
	if (state_fail(s = driver->init(&driver_c))) {
		return static_dispose(tp.cpu_count, s, state_msg);
	}
	// Opening MSRs (switch to user mode if permission denied)
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (state_fail(s = msr_open(tp.cpus[cpu].id, MSR_WR))) {
			return static_dispose(cpu, s, state_msg);
		}
	}
	// Reading min and max P_STATEs (by MSR, not by CPUFREQ, but it is supposed to be the same).
	if (state_fail(s = msr_read(tp.cpus[0].id, &data, sizeof(ullong), REG_LIMITS))) {
		return static_dispose(tp.cpu_count, s, state_msg);
	}
	// Find maximum and minimum P_STATE
    #if EXCESIVE_ROBUSTNESS
	ullong pstate_max = getbits64(data, 2, 0); // P_STATE max ex: P0
	ullong pstate_min = getbits64(data, 6, 4); // P_STATE min ex: P7
    debug("PSTATE_MAX: %llu", pstate_max);
    debug("PSTATE_MIN: %llu", pstate_min);
	if (pstate_max > 1LLU || pstate_min < 1LLU) {
		return static_dispose(tp.cpu_count, EAR_ERROR, "Incorrect P_STATE limits");
	}
    #endif
	// Save the 8 registers
	for (data = 0LLU, i = MAX_REGISTERS - 1; i >= 0; --i) {
		if (state_fail(s = msr_read(0, &regs[i], sizeof(ullong), REG_P0+((ullong) i)))) {
			return static_dispose(tp.cpu_count, s, state_msg);
		}
        debug("Read P%d register: %llu", i, regs[i]);
		if (getbits64(regs[i], 63, 63) == 1LLU) {
			data = regs[i];
		}
	}
    // Keeper will keep any messed configuration
    if (!keeper_load_uint64("Amd17PstateReg0", &regs[0])) {
        keeper_save_uint64("Amd17PstateReg0", regs[0]);
    } else{
        debug("Read P0 from keeper: %llu", regs[0]);
    }
    if (!keeper_load_uint64("Amd17PstateReg1", &regs[1])) {
        keeper_save_uint64("Amd17PstateReg1", regs[1]);
    } else{
        debug("Read P1 from keeper: %llu", regs[1]);
    }
	#if SHOW_DEBUGS
	print_registers();
	#endif
	// Boost enabled checked by MSR. 
	if (state_fail(s = msr_read(0, &data, sizeof(ullong), REG_HWCONF))) {
		return static_dispose(tp.cpu_count, s, state_msg);
	}
	boost_enabled = (uint) !getbits64(data, 25, 25);
	pss_nominal   = boost_enabled;
	// Building PSS object list. Is built through the P0 MSR register. But
    // it can be done using the frequency returned by the driver.
	if (state_fail(s = build_pstate_list())) {
		return static_dispose(tp.cpu_count, s, state_msg);
	}
	// Cleaning (Governor userspace required). Cleaning means setting
	// the frequency to test it. If it is not accepted, then its P_STATE
	// is removed from the list. In case is not virtual, is not required.
	if (mode_virtual) {
	    driver->set_governor(&driver_c, Governor.userspace);
	    pss_count = clean_pstate_list(1) + 1;
	    driver->set_governor(&driver_c, Governor.last);
    }
	//
	print_pstate_list();
	debug("num P_STATEs: %d", pss_count);
	debug("nominal P_STATE: P%d", pss_nominal);
	debug("boost enabled: %d", boost_enabled);
	init = 1;

	return EAR_SUCCESS;
}

/** Getters */
state_t mgt_cpufreq_amd17_count_available(ctx_t *c, uint *pstate_count)
{
	*pstate_count = pss_count;
	return EAR_SUCCESS;
}

static state_t search_pstate_list(ullong freq_khz, uint *pstate_index, uint closest)
{
	ullong cof_khz;
	ullong aux_khz;
    int p;

	// Boost test
	if (boost_enabled && freq_khz >= p0_khz(psss[0].cof)) {
		*pstate_index = 0;
		return EAR_SUCCESS;
	}
	// Closest test
	if (closest && freq_khz > p1_khz(psss[pss_nominal].cof)) {
		*pstate_index = pss_nominal;
		return EAR_SUCCESS;
	}
    //
	for (p = pss_nominal; p < pss_count; ++p)
	{
		cof_khz = p1_khz(psss[p].cof);
		if (freq_khz == cof_khz) {
            *pstate_index = p;
            return EAR_SUCCESS;
		}
		if (closest && freq_khz > cof_khz) {
			if (p > pss_nominal) {
				aux_khz = p1_khz(psss[p-1].cof) - freq_khz;
				cof_khz = freq_khz - cof_khz;
				p = p - (aux_khz < cof_khz);
			}
			*pstate_index = p;
            return EAR_SUCCESS;
		}
	}
	if (closest) {
		*pstate_index = pss_count-1;
		return EAR_SUCCESS;
	}
    return_msg(EAR_ERROR, "P_STATE not found");
}

state_t mgt_cpufreq_amd17_get_available_list(ctx_t *c, pstate_t *pstate_list)
{
	int i;

	if (pss_nominal) {
		pstate_list[0].idx = 0LLU;
		pstate_list[0].khz = p0_khz(psss[0].cof);
	}
	for (i = pss_nominal; i < pss_count; ++i) {
		pstate_list[i].idx = (ullong) i;
		pstate_list[i].khz = p1_khz(psss[i].cof);
	}
	return EAR_SUCCESS;
}


state_t get_current_hsmp(ctx_t *c, pstate_t *pstate_list)
{
    uint args[2] = {  0, -1 };
    uint reps[2] = {  0, -1 };
    uint cpu;
    uint idx;

    for (cpu = 0; cpu < tp.cpu_count; ++cpu)
    {
        // Preparing HSMP arguments and response
        args[0] = tp.cpus[cpu].apicid;
        reps[0] = 0;
        // Calling HSMP
        hsmp_send(tp.cpus[cpu].socket_id, READ_BOOST_LIMIT, args, reps); //response is freq (MHz)
        debug("HSMP read %u", reps[0]);

        pstate_list[cpu].idx = pss_nominal;
        pstate_list[cpu].khz = p1_khz(psss[pss_nominal].cof);

        if (state_ok(search_pstate_list(((ullong) reps[0])*1000LLU, &idx, 1))) {
            pstate_list[cpu].idx = idx;
            if (idx == 0) {
                pstate_list[cpu].khz = p0_khz(psss[idx].cof);
            } else {
                pstate_list[cpu].khz = p1_khz(psss[idx].cof);
            }
        }
    }
    return EAR_SUCCESS;
}

state_t get_current_virtual(ctx_t *c, pstate_t *pstate_list)
{
	const ullong *freq_list;
	ullong reg, fid, did, cof;
	uint governor;
	uint cpu, pst;
	state_t s;

	if (state_fail(s = driver->get_governor(&driver_c, &governor))) {
		return s;
	}
	if (state_fail(s = driver->get_current_list(&driver_c, &freq_list))) {
		return s;
	}
	for (cpu = 0; cpu < tp.cpu_count; ++cpu)
	{
		pstate_list[cpu].idx = pss_nominal;
		pstate_list[cpu].khz = p1_khz(psss[pss_nominal].cof);

		if (governor == Governor.userspace) {
			// If is P0 and boost enabled
			if (boost_enabled && freq_list[cpu] == p1_khz(psss[0].cof)) {
				pstate_list[cpu].idx = 0;
				pstate_list[cpu].khz = p0_khz(psss[0].cof);
			}
			// If is in P1
			else if (state_ok(msr_read(cpu, &reg, sizeof(ullong), REG_P1))) {
				fid = getbits64(reg, 7, 0);
				did = getbits64(reg, 13, 8);
				cof = 0LLU;

				if (did != 0) {
					cof = p1_khz((fid * 200LLU) / did);

					if (state_ok(search_pstate_list(cof, &pst, 0))) {
						pstate_list[cpu].idx = pst;
						pstate_list[cpu].khz = p1_khz(psss[pst].cof);
					}
				}
			}
		} // Governor

		#if SHOW_DEBUGS
		ullong data[4];
		data[0] = 0;
		data[1] = 0;
		data[2] = 0;
		data[3] = 0;
		
		msr_read(cpu, &data[0], sizeof(ullong), 0xc0010061);
		msr_read(cpu, &data[1], sizeof(ullong), 0xc0010062);
		msr_read(cpu, &data[2], sizeof(ullong), 0xc0010063);
		msr_read(cpu, &data[3], sizeof(ullong), 0xc0010015);

		debug("CPU%d PMAX: %llu, PLIM: %llu, PCOM: %llu, PCUR: %llu, REG: %llu, COF: %llu", cpu,
			getbits64(data[0],  6,  4), getbits64(data[0],  2,  0),
			getbits64(data[1],  2,  0), getbits64(data[2],  2,  0),
			reg, pstate_list[cpu].khz
			);
		#endif
	}

	return EAR_SUCCESS;
}

state_t mgt_cpufreq_amd17_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
    if (mode_virtual) {
        return get_current_virtual(c, pstate_list);
    }
    return get_current_hsmp(c, pstate_list);
}

state_t mgt_cpufreq_amd17_get_nominal(ctx_t *c, uint *pstate_index)
{
	*pstate_index = boost_enabled;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_amd17_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
    return search_pstate_list(freq_khz, pstate_index, closest);
}

/** Setters */
static state_t set_pstate_hsmp(uint cpu, uint pst, uint test)
{
    uint args[2] = {  0, -1 };
    uint reps[1] = { -1, };

    args[0] = setbits32(0, tp.cpus[cpu].apicid, 31, 16) | psss[pst].cof; //arguments are apicid and max_freq
    debug("setting PS%d: %u MHz in cpu %d (apic %d) [%u]",
        pst, getbits32(args[0], 15, 0), cpu, getbits32(args[0], 31, 16), args[0]);

    return hsmp_send(tp.cpus[cpu].socket_id, WRITE_BOOST_LIMIT, args, reps);
}

// Sets the current P_STATE to MSR P1 adding new frequency.
static state_t set_pstate_virtual(uint cpu, uint pst, uint test, uint set_driver)
{
	ullong aux;
	ullong reg;
	state_t s;

	if (cpu >= tp.cpu_count) {
		return_msg(EAR_ERROR, Generr.cpu_invalid);
	}
	// Setting the P_STATE 'pst' multiplicator and divider
	reg = regs[0];
	reg = setbits64(reg, psss[pst].fid,  7,  0);
	reg = setbits64(reg, psss[pst].did, 13,  8);
	reg = setbits64(reg,          1LLU, 63, 63);
	// Writing its value in MSR
	if (state_fail(s = msr_write(cpu, &reg, sizeof(ullong), REG_P1))) {
		return s;
	}
	// Calling the driver to set P1 in all CPUs (edit, why?)
	if (set_driver) {
		if (state_fail(s = driver->set_current(&driver_c, 1, cpu))) {
			return s;
		}
	}
	debug("edited CPU%d P1 MSR to (%llum * 200 / %llud) = %llu MHz",
		  cpu, psss[pst].fid, psss[pst].did, psss[pst].cof);

	// Test in case the frequency is not written.
	if (test) {
		if (state_fail(s = msr_read(cpu, &aux, sizeof(ullong), REG_P1))) {
			return s;
		}
		if (aux != reg) {
			return_msg(EAR_ERROR, "frequency not written");
		}
	}

	return EAR_SUCCESS;
}

static state_t set_pstate(uint cpu, uint pst, uint test, uint set_driver)
{
    if (mode_virtual) {
        return set_pstate_virtual(cpu, pst, test, set_driver);
    }
    return set_pstate_hsmp(cpu, pst, test);
}

static state_t set_pstate0_hsmp(uint cpu)
{
    uint args[2] = { 0, -1 };
    uint reps[1] = { -1, };
       
    // Setting 16 bit MAX (it is converted by the system later) 
    args[0] = setbits32(0, tp.cpus[cpu].apicid, 31, 16) | 65535;
    debug("setting PS0: %u MHz in cpu %d (apic %d) [%u]",
        getbits32(args[0], 15, 0), cpu, getbits32(args[0], 31, 16), args[0]);

    return hsmp_send(tp.cpus[cpu].socket_id, WRITE_BOOST_LIMIT, args, reps);
}

// Sets the current P_STATE to MSR P0
static state_t set_pstate0_virtual(uint cpu)
{
	state_t s;
	#if EXCESIVE_ROBUSTNESS
	// Recovering all MSR P1s to its original state (excess of robustness)
	if (state_fail(s = msr_write(cpu, &regs[1], sizeof(ullong), REG_P1))) {
		return s;
	}
	#endif
	// Calling the driver to set P0 in specific CPU
	if (state_fail(s = driver->set_current(&driver_c, 0, cpu))) {
		return s;
	}
	debug("edited CPU%d to MSR to P0", cpu);
	return EAR_SUCCESS;
}

static state_t set_pstate0(uint cpu)
{
    if (mode_virtual) {
        return set_pstate0_virtual(cpu);
    }
    return set_pstate0_hsmp(cpu);
}

state_t mgt_cpufreq_amd17_set_current_list(ctx_t *c, uint *pstate_index)
{
	state_t s2 = EAR_SUCCESS;
	state_t s1 = EAR_SUCCESS;
	uint cpu;

	#if EXCESIVE_ROBUSTNESS
	// Step 1 (excesss of robustness, better rely on external functions to change governor).
	if (state_fail(s1 = driver->set_governor(&driver_c, Governor.userspace))) {
		return s1;
	}
	#endif
	// Step 2
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (pstate_index[cpu] == ps_nothing) {
			continue;
		}
		// If P_STATE boost
		if (pstate_index[cpu] == 0 && pss_nominal) {
			if (state_fail(s1 = set_pstate0(cpu))) {
				s2 = s1;
			}
		// Else
		} else if (state_fail(s1 = set_pstate(cpu, pstate_index[cpu], 0, 1))) {
			s2 = s1;
		}
	}
	return s2;
}

state_t mgt_cpufreq_amd17_set_current(ctx_t *_c, uint pstate_index, int cpu)
{
	state_t s1 = EAR_SUCCESS;
	state_t s2 = EAR_SUCCESS;
	uint c;

    if (cpu >= tp.cpu_count) {
        return_msg(EAR_ERROR, Generr.cpu_invalid);
    }
    if (pstate_index >= pss_count) {
        return_msg(EAR_ERROR, Generr.arg_outbounds);
    } 
	#if EXCESIVE_ROBUSTNESS
	// Step 1 (excesss of robustness, better rely on external functions to change governor).
	if (state_fail(s1 = driver->set_governor(&driver_c, Governor.userspace))) {
		return s1;
	}
	#endif
	debug("setting P_STATE %d (cof: %llu) in cpu %d", pstate_index, psss[pstate_index].cof, cpu);
	// Step 2 (single CPU)
	if (cpu != all_cpus) {
		// If P_STATE boost
		if (pstate_index == 0 && pss_nominal) {
			if (state_fail(s1 = set_pstate0(cpu))) {
				return s1;
			}
			return set_pstate0((uint) tp.cpus[cpu].sibling_id);
		// Else
		} else {
			if (state_fail(s1 = set_pstate((uint) cpu, pstate_index, 0, 1))) {
				return s1;
			}
			return set_pstate((uint) tp.cpus[cpu].sibling_id, pstate_index, 0, 1);
		}
	}
	// Step 2 (all CPUs to P0)
	if (pstate_index == 0 && pss_nominal) {
        // Here in the past, when in mode virtual the P1 registers were
        // recovered to their initial value.
        if (mode_virtual) {
		    // Calling the driver to set P0 in all CPUs
		    return driver->set_current(&driver_c, 0, all_cpus);
        }
	}
	// Step 2 (all CPUs to PX)
	for (c = 0; c < tp.cpu_count; ++c) {
    	if (pstate_index == 0 && pss_nominal) {
		    if (state_fail(s1 = set_pstate0(c))) {
			    s2 = s1;
		    }
        } else {
		    if (state_fail(s1 = set_pstate(c, pstate_index, 0, 1))) {
			    s2 = s1;
		    }
        }
	}
	return s2;
}

state_t mgt_cpufreq_amd17_reset(ctx_t *c)
{
    int cpu;
    for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
        msr_write(cpu, &regs[0], sizeof(ullong), REG_P0);
        msr_write(cpu, &regs[1], sizeof(ullong), REG_P1);
    }
    return EAR_SUCCESS;
}

// Governors
state_t mgt_cpufreq_amd17_governor_get(ctx_t *c, uint *governor)
{
    return driver->get_governor(&driver_c, governor);
}

state_t mgt_cpufreq_amd17_governor_get_list(ctx_t *c, uint *governors)
{
    return driver->get_governor_list(&driver_c, governors);
}

static state_t recover()
{
	state_t s;
	int cpu;
    // Recovering CPUs to its original frecuencies
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (state_fail(s = msr_write(cpu, &regs[1], sizeof(ullong), REG_P1))) {
			return s;
		}
	}
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_amd17_governor_set(ctx_t *c, uint governor)
{
    recover();
	return driver->set_governor(&driver_c, governor);
}

state_t mgt_cpufreq_amd17_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask)
{
    recover();
    return driver->set_governor_mask(&driver_c, governor, mask);
}

state_t mgt_cpufreq_amd17_governor_set_list(ctx_t *c, uint *governors)
{
    recover();
    return driver->set_governor_list(&driver_c, governors);
}

