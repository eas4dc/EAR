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

//#define SHOW_DEBUGS 1

#include <math.h>
#include <stdlib.h>
#include <metrics/common/msr.h>
#include <common/output/debug.h>
#include <common/string_enhanced.h>
#include <common/hardware/bithack.h>
#include <management/cpufreq/archs/amd17.h>

typedef struct pss_s {
	ullong fid;
	ullong did;
	ullong cof;
} pss_t;

#define CPU             0
#define MAX_PSTATES     128
#define MAX_REGISTERS   8
#define REG_HWCONF      0xc0010015 //HWCR
#define REG_LIMITS      0xc0010061
#define REG_CONTROL     0xc0010062
#define REG_STATUS      0xc0010063
#define REG_P0          0xc0010064
#define REG_P1          0xc0010065

#define p0_khz(cof) ((cof * 1000LLU) + 1000LLU)
#define p1_khz(cof) ((cof * 1000LLU))

static topology_t tp;

typedef struct amd17_ctx_s {
	ctx_t                driver_c;
	mgt_ps_driver_ops_t *driver;
	ullong               regs[MAX_REGISTERS];
	pss_t                psss[MAX_PSTATES];
	uint                 boost_enabled;
	uint                 pss_nominal;
	uint                 pss_count;
	uint                 user_mode;
	uint                 init;
} amd17_ctx_t;

state_t cpufreq_amd17_status(topology_t *_tp)
{
	state_t s;
	debug("testing AMD17 P_STATE control status");
	#if AMD_OSCPUFREQ
	return EAR_ERROR;
	#endif
	if (_tp->vendor != VENDOR_AMD) {
		return EAR_ERROR;
	}
	if (_tp->family < FAMILY_ZEN) {
		return EAR_ERROR;
	}
	// Thread control required in this small section
	if (tp.cpu_count == 0) {
		if(xtate_fail(s, topology_copy(&tp, _tp))) {
			return EAR_ERROR;
		}
	}
	return EAR_SUCCESS;
}

static state_t static_init_test(ctx_t *c, amd17_ctx_t **f)
{
	// This function converts a context into an specific AMD17 context
	if (c == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	if (((*f = (amd17_ctx_t *) c->context) == NULL) || !(*f)->init) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	return EAR_SUCCESS;
}

static state_t static_dispose(amd17_ctx_t *f, uint close_msr_up_to, state_t s, char *msg)
{
	int cpu;

	// TODO: Recover MSR previous state
	// Closing just the opened MSRs
	for (cpu = 0; cpu < close_msr_up_to; ++cpu) {
		msr_close(tp.cpus[cpu].id);
	}
	if (f == NULL) {
		return_msg(s, msg);
	}
	//
	if (f->driver != NULL) {
		f->driver->dispose(&f->driver_c);
	}
	//
	f->init = 0;
	return_msg(s, msg);
}

state_t cpufreq_amd17_dispose(ctx_t *c)
{
	amd17_ctx_t *f;
	state_t s;
	if (xtate_fail(s, static_init_test(c, &f))) {
		return s;
	}
	if (f->user_mode) {
		return static_dispose(f, 0, EAR_SUCCESS, NULL);
	}
	return static_dispose(f, tp.cpu_count, EAR_SUCCESS, NULL);
}

#if SHOW_DEBUGS
static void amd17_pstate_print(amd17_ctx_t *f)
{
	// Look into 'PPR 17h, Models 31h.pdf' the MSRC001_006[4...B] register information.
	debug ("-----------------------------------------------------------------------------------");
	tprintf_init(STDERR_FILENO, STR_MODE_DEF, "4 18 4 8 8 11 8 7 8 10 10");
	tprintf("#||reg||en||IddDiv||IddVal||IddPerCore|||CpuVid|||CpuFid||  200||CpuDfsId||CoreCof");
	tprintf("-||---||--||------||------||----------|||------|||------||-----||--------||-------");

	int i;

	for (i = 0; i < MAX_REGISTERS; ++i)
	{
		ullong en        = getbits64(f->regs[i], 63, 63);
		ullong idd_div   = getbits64(f->regs[i], 31, 30);
		ullong idd_value = getbits64(f->regs[i], 29, 22);
		double idd_core  = ((double) idd_value) / pow(10.0, (double) idd_div);
		ullong vdd_cpu   = getbits64(f->regs[i], 21, 14);
		//double pow_core  = 1.350 - (((double) CpuVid) * 0.00625);
		ullong freq_fid  = getbits64(f->regs[i],  7,  0);
		ullong freq_did  = getbits64(f->regs[i], 13,  8);
		ullong freq_cof  = 0;

		if (freq_did > 0) {
			freq_cof = (freq_fid * 200LLU) / freq_did;
		}

		tprintf("%d||%LX||%llu||%llu||%llu||%0.2lf|||%llu|||%llu||* 200||/ %llu||= %llu",
				i, f->regs[i], en, idd_div, idd_value, idd_core, vdd_cpu, freq_fid, freq_did, freq_cof);
	}
	debug ("-----------------------------------------------------------------------------------");
}
#endif

static state_t pstate_build_psss_single(ullong freq_mhz, ullong *cof, ullong *fid, ullong *did)
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
			//
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

static state_t set_frequency_p1(amd17_ctx_t *f, uint cpu, uint pst, uint test, uint set_driver);

static int pstate_clean_psss(amd17_ctx_t *f, uint i)
{
	int how_many;
	// If reached the deepest point then return
	if (i == f->pss_count) {
		return 0;
	}
	//
	how_many = pstate_clean_psss(f, i+1);
	// If we are OK then return
	if (state_ok(set_frequency_p1(f, 0, i, 1, 0))) {
		return how_many+1;
	}
	// We are not OK
	debug("frequency %llu can't be written (%s)", f->psss[i].cof, state_msg);
	// If we are not OK and the next is OK
	int h;
	for (h = 0; h < how_many; ++h) {
		memcpy(&f->psss[i+h], &f->psss[i+h+1], sizeof(pss_t));
	}
	// If we are not OK and the next is not OK
	return how_many;
}

static void amd17_pstate_print_psss(amd17_ctx_t *f)
{
	int i;
	debug("PSS object list: ");
	for (i = 0; i < f->pss_count; ++i) {
		debug("P%d: %llum * 200 / %llud = %llu MHz", i,
			  f->psss[i].fid, f->psss[i].did, f->psss[i].cof);
	}
}

static state_t pstate_build_psss(amd17_ctx_t *f)
{
	const ullong *freqs_available;
	uint freq_count;
	ullong min_mhz;
	state_t s;
	ullong p;
	int i;

	// Getting the minimum frequency specified by the driver
	if (xtate_fail(s, f->driver->get_available_list(&f->driver_c, &freqs_available, &freq_count))) {
		return static_dispose(f, 0, s, state_msg);
	}
	min_mhz = freqs_available[freq_count-1] / 1000LLU;
	// Getting P0(b) information from the first of a list of registers. In case of user mode,
	// this register is filled virtually, because its impossible to read the MSR.
	f->psss[0].fid = getbits64(f->regs[0],  7,  0);
	f->psss[0].did = getbits64(f->regs[0], 13,  8);
	if (f->psss[0].did == 0) {
		return_msg(EAR_ERROR, "the frequency divisor is 0");
	}
	// Getting the P0(b) Core Frequency
	f->psss[0].cof = (f->psss[0].fid * 200LLU) / f->psss[0].did;
	// Getting P0 in case boost is not enabled
	i = 1;
	// If boost is enabled, the non-boosted P0 is defined, if not P0(b) becomes P0.
	if (f->boost_enabled) {
		f->psss[1].cof = f->psss[0].cof;
		f->psss[1].fid = f->psss[0].fid;
		f->psss[1].did = f->psss[0].did;
		i = 2;
	}
	// Getting P1 Core Frequency (intervals of 100 MHz)
	if ((f->psss[0].cof % 100LLU) != 0) {
		p = f->psss[0].cof + (100LLU - (f->psss[0].cof % 100LLU)) - 100LLU;
	} else {
		p = f->psss[0].cof - 100LLU;
	}
	// Getting PSS objects from P1 to P7
	//min_mhz = 1000;
	for (; p >= min_mhz && i < MAX_PSTATES; p -= 100, ++i) {
		if (xtate_fail(s, pstate_build_psss_single(p, &f->psss[i].cof, &f->psss[i].fid, &f->psss[i].did))) {
			return s;	
		}
	}
	f->pss_count = i;
	return EAR_SUCCESS;
}

static state_t pstate_build_psss_user(amd17_ctx_t *f)
{
	const ullong *freqs_available;
	ullong cof, fid, did;
	state_t s;

	// In user mode the driver is the main actor
	if (state_fail(s = f->driver->get_available_list(&f->driver_c, &freqs_available, &f->pss_count))) {
		return static_dispose(f, 0, s, state_msg);
	}
	// Initializing a virtual (because MSR can't be read) register PSS0 to initialize the list.
	cof = freqs_available[0] / 1000LLU;
	if (state_fail(s = pstate_build_psss_single(cof, &cof, &fid, &did))) {
		return s;
	}
	f->regs[0] = setbits64(      0LLU,  fid,  7,  0);
	f->regs[0] = setbits64(f->regs[0],  did, 13,  8);
	f->regs[0] = setbits64(f->regs[0], 1LLU, 63, 63);
	#if SHOW_DEBUGS
	amd17_pstate_print(f);
	#endif
	// Building PSS object list
	if (state_fail(s = pstate_build_psss(f))) {
		return static_dispose(f, 0, s, state_msg);
	}
	
	return EAR_SUCCESS;
}

state_t cpufreq_amd17_init_user(ctx_t *c, mgt_ps_driver_ops_t *ops_driver, const ullong *freq_list, uint freq_count)
{
	amd17_ctx_t *f;
	state_t s;
	int i;

	debug("initializing AMD17 P_STATE control");
	//
	if (state_ok(static_init_test(c, &f))) {
		return_msg(EAR_ERROR, Generr.api_initialized);
	}
	// Context
    if ((c->context = calloc(1, sizeof(amd17_ctx_t))) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    f = (amd17_ctx_t *) c->context;
	// User mode please.
	f->user_mode = 1;
	// Inititializing the driver before (because if doesn't work we can't do anything).
	f->driver = ops_driver;
	if (xtate_fail(s, f->driver->init(&f->driver_c))) {
		return static_dispose(f, 0, s, state_msg);
	}
	// Boost enabled checked by the driver.
	if (xtate_ok(s, f->driver->get_boost(&f->driver_c, &f->boost_enabled))) {
		f->pss_nominal = f->boost_enabled;
	}
	// Inititializing the driver before (because if doesn't work we can't do anything).
	if (freq_list == NULL) {
		if (xtate_fail(s, pstate_build_psss_user(f))) {
			return static_dispose(f, 0, s, state_msg);
		}
	} else {
		f->pss_count = freq_count;
		for (i = 0; i < f->pss_count; ++i) {
			f->psss[i].cof = freq_list[i] / 1000LLU;
			// If someone sends the list, the additional 1000 KHz is substracted.
			if (f->boost_enabled && (i == 0)) {
				if ((freq_list[0] % 10000LLU) == 1000LLU) {
					f->psss[0].cof = (freq_list[0] - 1000LLU) / 1000LLU;
				}
			}
		}
	} 
	// Initializing in user mode
	amd17_pstate_print_psss(f);
	debug("num P_STATEs: %d", f->pss_count);
	debug("nominal P_STATE: P%d", f->pss_nominal);
	debug("boost enabled: %d", f->boost_enabled);
	f->init = 1;
	
	return EAR_SUCCESS;
}

state_t cpufreq_amd17_init(ctx_t *c, mgt_ps_driver_ops_t *ops_driver)
{
	amd17_ctx_t *f;
	ullong data;
	state_t s;
	int cpu;
	int i;

	debug("initializing AMD17 P_STATE control");
	//
	if (state_ok(static_init_test(c, &f))) {
		return_msg(EAR_ERROR, Generr.api_initialized);
	}
	// Context
    if ((c->context = calloc(1, sizeof(amd17_ctx_t))) == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    f = (amd17_ctx_t *) c->context;
	// Inititializing the driver before (because if doesn't work we can't do anything).
	f->driver = ops_driver;
	if (xtate_fail(s, f->driver->init(&f->driver_c))) {
		return static_dispose(f, tp.cpu_count, s, state_msg);
	}
	// Opening MSRs (switch to user mode if permission denied)
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (xtate_fail(s, msr_open(tp.cpus[cpu].id))) {
			#if 0
			if (state_is(s, EAR_NO_PERMISSIONS)) {
				return cpufreq_amd17_init_user(f);
			}
			#endif
			return static_dispose(f, cpu, s, state_msg);
		}
	}
	// Reading min and max P_STATEs (by MSR, not by CPUFREQ, but it is supposed to be the same).
	if (xtate_fail(s, msr_read(tp.cpus[0].id, &data, sizeof(ullong), REG_LIMITS))) {
		return static_dispose(f, tp.cpu_count, s, state_msg);
	}
	// Find maximum and minimum P_STATE
	ullong pstate_max = getbits64(data, 2, 0); // P_STATE max ex: P0
	ullong pstate_min = getbits64(data, 6, 4); // P_STATE min ex: P7
	if (pstate_max > 1LLU || pstate_min < 1LLU) {
		return static_dispose(f, tp.cpu_count, s, "Incorrect P_STATE limits");
	}
	// Save registers configuration
	for (data = 0LLU, i = MAX_REGISTERS - 1; i >= 0; --i) {
		if (xtate_fail(s, msr_read(0, &f->regs[i], sizeof(ullong), REG_P0+((ullong) i)))) {
			return static_dispose(f, tp.cpu_count, s, state_msg);
		}
		if (getbits64(f->regs[i], 63, 63) == 1LLU) {
			data = f->regs[i];
		}
	}
	#if SHOW_DEBUGS
	amd17_pstate_print(f);
	#endif
	// Boost enabled checked by MSR. 
	if (state_fail(s = msr_read(0, &data, sizeof(ullong), REG_HWCONF))) {
		return static_dispose(f, tp.cpu_count, s, state_msg);
	}
	debug("AMD HWCONF %llu (bit 25: %llu)", data, getbits64(data, 25, 25));
	f->boost_enabled = (uint) !getbits64(data, 25, 25);
	f->pss_nominal   = f->boost_enabled;
	// Building PSS object list.
	if (state_fail(s = pstate_build_psss(f))) {
		return static_dispose(f, tp.cpu_count, s, state_msg);
	}
	// Clean sets individual P_STATEs, and if any doen't work is removed.
	f->pss_count = pstate_clean_psss(f, 1) + 1;
	//
	amd17_pstate_print_psss(f);
	debug("num P_STATEs: %d", f->pss_count);
	debug("nominal P_STATE: P%d", f->pss_nominal);
	debug("boost enabled: %d", f->boost_enabled);
	f->init = 1;

	return EAR_SUCCESS;
}

/** Getters */
state_t cpufreq_amd17_count_available(ctx_t *c, uint *pstate_count)
{
	amd17_ctx_t *f;
	state_t s;
	if (xtate_fail(s, static_init_test(c, &f))) {
		return s;
	}
	*pstate_count = f->pss_count;
	return EAR_SUCCESS;
}

static state_t static_get_index(amd17_ctx_t *f, ullong freq_khz, uint *pstate_index, uint closest)
{
	ullong cof_khz;
	ullong aux_khz;
    int pst;

	// Boost test
	if (f->boost_enabled && p0_khz(f->psss[0].cof) == freq_khz) {
		*pstate_index = 0;
		return EAR_SUCCESS;
	}
	// Closest test
	if (closest && freq_khz > p1_khz(f->psss[f->pss_nominal].cof)) {
		*pstate_index = f->pss_nominal;
		return EAR_SUCCESS;
	}
	// The searching. The closest frequency in case of PST-1 and PST0 differences are
	// equal, it will be PST0. 
	for (pst = f->pss_nominal; pst < f->pss_count; ++pst)
	{
		cof_khz = p1_khz(f->psss[pst].cof);
		if (freq_khz == cof_khz) {
            *pstate_index = pst;
            return EAR_SUCCESS;
		}
		if (closest && freq_khz > cof_khz) {
			if (pst > f->pss_nominal) {
				aux_khz = p1_khz(f->psss[pst-1].cof) - freq_khz;
				cof_khz = freq_khz - cof_khz;
				pst = pst - (aux_khz < cof_khz);
			}
			*pstate_index = pst;
            return EAR_SUCCESS;
		}
	}
	if (closest) {
		*pstate_index = f->pss_count-1;
		return EAR_SUCCESS;
	}
    return_msg(EAR_ERROR, "P_STATE not found");
}

state_t cpufreq_amd17_get_available_list(ctx_t *c, pstate_t *pstate_list)
{
	amd17_ctx_t *f;
	state_t s;
	int i;

	if (xtate_fail(s, static_init_test(c, &f))) {
		return s;
	}
	if (f->pss_nominal) {
		pstate_list[0].idx = 0LLU;
		pstate_list[0].khz = p0_khz(f->psss[0].cof);
	}
	for (i = f->pss_nominal; i < f->pss_count; ++i) {
		pstate_list[i].idx = (ullong) i;
		pstate_list[i].khz = p1_khz(f->psss[i].cof);
	}
	return EAR_SUCCESS;
}

state_t cpufreq_amd17_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	const ullong *freq_list;
	ullong reg, fid, did, cof;
	amd17_ctx_t *f;
	uint governor;
	uint cpu, pst;
	state_t s;

	if (xtate_fail(s, static_init_test(c, &f))) {
		return s;
	}
	if (xtate_fail(s, f->driver->get_governor(&f->driver_c, &governor))) {
		return s;
	}
	if (xtate_fail(s, f->driver->get_current_list(&f->driver_c, &freq_list))) {
		return s;
	}
	for (cpu = 0; cpu < tp.cpu_count; ++cpu)
	{
		pstate_list[cpu].idx = f->pss_nominal;
		pstate_list[cpu].khz = p1_khz(f->psss[f->pss_nominal].cof);

		if (governor == Governor.userspace)
		{
			// If is P0 and boost enabled
			if (f->boost_enabled && freq_list[cpu] == p1_khz(f->psss[0].cof)) {
				pstate_list[cpu].idx = 0;
				pstate_list[cpu].khz = p0_khz(f->psss[0].cof);
			}
			// If is in P1
			else if (state_ok(msr_read(cpu, &reg, sizeof(ullong), REG_P1)))
			{
				fid = getbits64(reg, 7, 0);
				did = getbits64(reg, 13, 8);
				cof = 0LLU;

				if (did != 0)
				{
					cof = p1_khz((fid * 200LLU) / did);

					if (state_ok(static_get_index(f, cof, &pst, 0))) {
						pstate_list[cpu].idx = pst;
						pstate_list[cpu].khz = p1_khz(f->psss[pst].cof);
					}
				}
			}
		}

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

state_t cpufreq_amd17_get_nominal(ctx_t *c, uint *pstate_index)
{
	amd17_ctx_t *f;
	state_t s;
	if (xtate_fail(s, static_init_test(c, &f))) {
		return s;
	}
	*pstate_index = f->boost_enabled;
	return EAR_SUCCESS;
}

state_t cpufreq_amd17_get_governor(ctx_t *c, uint *governor)
{
	amd17_ctx_t *f;
	state_t s;
	if (xtate_fail(s, static_init_test(c, &f))) {
		return s;
	}
	return f->driver->get_governor(&f->driver_c, governor);
}

state_t cpufreq_amd17_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
    amd17_ctx_t *f;
    state_t s;

    if (xtate_fail(s, static_init_test(c, &f))) {
        return s;
    }
    return static_get_index(f, freq_khz, pstate_index, closest);
}

/** Setters */
// Sets the current P_STATE to MSR P1 adding new frequency.
static state_t set_frequency_p1(amd17_ctx_t *f, uint cpu, uint pst, uint test, uint set_driver)
{
	ullong aux;
	ullong reg;
	state_t s;

	if (cpu >= tp.cpu_count) {
		return_msg(EAR_ERROR, Generr.cpu_invalid);
	}
	// Setting the P_STATE 'pst' multiplicator and divider
	reg = f->regs[0];
	reg = setbits64(reg, f->psss[pst].fid,  7,  0);
	reg = setbits64(reg, f->psss[pst].did, 13,  8);
	reg = setbits64(reg,             1LLU, 63, 63);
	// Writing its value in MSR
	if (xtate_fail(s, msr_write(cpu, &reg, sizeof(ullong), REG_P1))) {
		return s;
	}
	// Calling the driver to set P1 in all CPUs
	if (set_driver) {
		if (xtate_fail(s, f->driver->set_current(&f->driver_c, 1, all_cpus))) {
			return s;
		}
	}

	debug("edited CPU%d P1 MSR to (%llum * 200 / %llud) = %llu MHz",
		  cpu, f->psss[pst].fid, f->psss[pst].did, f->psss[pst].cof);

	// Test in case the frequency is not written
	if (test) {
		if (xtate_fail(s, msr_read(cpu, &aux, sizeof(ullong), REG_P1))) {
			return s;
		}
		if (aux != reg) {
			debug("REG_P1 %llu is different from the written %llu", aux, reg);
			return_msg(EAR_ERROR, "frequency not written");
		}
	}

	return EAR_SUCCESS;
}

// Sets the current P_STATE to MSR P0
static state_t set_frequency_p0(amd17_ctx_t *f, uint cpu)
{
	state_t s;
	#if 0
	// Recovering all MSR P1s to its original state (excess of robustness)
	if (xtate_fail(s, msr_write(cpu, &f->regs[1], sizeof(ullong), REG_P1))) {
		return s;
	}
	#endif
	// Calling the driver to set P0 in specific CPU
	if (xtate_fail(s, f->driver->set_current(&f->driver_c, 0, cpu))) {
		return s;
	}
	debug("edited CPU%d to MSR to P0", cpu);
	return EAR_SUCCESS;
}

state_t cpufreq_amd17_set_current_list(ctx_t *c, uint *pstate_index)
{
	state_t s2 = EAR_SUCCESS;
	state_t s1 = EAR_SUCCESS;
	amd17_ctx_t *f;
	uint cpu;

	if (xtate_fail(s1, static_init_test(c, &f))) {
		return s1;
	}
	#if 0
	// Step 1 (excesss of robustness, better rely on external functions to change governor).
	if (xtate_fail(s1, f->driver->set_governor(&f->driver_c, Governor.userspace))) {
		return s1;
	}
	#endif
	// Step 2
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		// If P_STATE boost
		if (pstate_index[cpu] == 0 && f->pss_nominal) {
			if (xtate_fail(s1, set_frequency_p0(f, cpu))) {
				s2 = s1;
			}
		// Else
		} else if (xtate_fail(s1, set_frequency_p1(f, cpu, pstate_index[cpu], 0, 1))) {
			s2 = s1;
		}
	}
	return s2;
}

state_t cpufreq_amd17_set_current(ctx_t *c, uint pstate_index, int _cpu)
{
	state_t s1 = EAR_SUCCESS;
	state_t s2 = EAR_SUCCESS;
	amd17_ctx_t *f;
	uint cpu;

	if (xtate_fail(s1, static_init_test(c, &f))) {
		return s1;
	}
	#if 0
	// Step 1 (excesss of robustness, better rely on external functions to change governor).
	if (xtate_fail(s1, f->driver->set_governor(&f->driver_c, Governor.userspace))) {
		return s1;
	}
	#endif
	debug("setting P_STATE %d (cof: %llu) in cpu %d", pstate_index, f->psss[pstate_index].cof, _cpu);
	// Step 2 (single CPU)
	if (_cpu != all_cpus) {
		// If P_STATE boost
		if (pstate_index == 0 && f->pss_nominal) {
			if (xtate_fail(s1, set_frequency_p0(f, _cpu))) {
				return s1;
			}
			return set_frequency_p0(f, (uint) tp.cpus[_cpu].sibling_id);
		// Else
		} else {
			if (xtate_fail(s1, set_frequency_p1(f, (uint) _cpu, pstate_index, 0, 1))) {
				return s1;
			}
			return set_frequency_p1(f, (uint) tp.cpus[_cpu].sibling_id, pstate_index, 0, 1);
		}
	}
	// Step 2 (all CPUs to P0)
	if (pstate_index == 0 && f->pss_nominal) {
		#if 0
		// Recovering all P1s to its original state
		for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
			if (xtate_fail(s1, msr_write(cpu, &f->regs[1], sizeof(ullong), REG_P1))) {
				s2 = s1;
			}
		}
		#endif
		// Calling the driver to set P0 in all CPUs
		if (xtate_fail(s1, f->driver->set_current(&f->driver_c, 0, all_cpus))) {
			return s1;
		}
		return s2;
	}
	// Step 2 (all CPUs to P1)
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (xtate_fail(s1, set_frequency_p1(f, cpu, pstate_index, 0, 1))) {
			s2 = s1;
		}
	}
	return s2;
}

state_t cpufreq_amd17_set_governor(ctx_t *c, uint governor)
{
	amd17_ctx_t *f;
	state_t s;
	int cpu;

	if (xtate_fail(s, static_init_test(c, &f))) {
		return s;
	}
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		if (xtate_fail(s, msr_write(cpu, &f->regs[1], sizeof(ullong), REG_P1))) {
			return s;
		}
	}
	if (xtate_fail(s, f->driver->set_governor(&f->driver_c, governor))) {
		return s;
	}

	return EAR_SUCCESS;
}
