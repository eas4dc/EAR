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

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/math_operations.h>
#include <common/hardware/bithack.h>
#include <metrics/common/msr.h>
#include <metrics/bandwidth/archs/amd17df.h>

// In a 32 cores and 64 threads CPU (ZEN3@7773X):
// 007:047 are excited by CPUs 00-15
// 087:0c7 are excited by CPUs 16-31
// 107:147 are excited by CPUs 32-47
// 187:1c7 are excited by CPUs 48-63
static topology_t      tp;
static uint            devs_count;
static bwidth_t       *pool;
static suscription_t  *sus;
static uint            imcs[8];
static off_t           ctls[4] = { 0xc0010240, 0xc0010242, 0xc0010244, 0xc0010246 };
static off_t           ctrs[4] = { 0xc0010241, 0xc0010243, 0xc0010245, 0xc0010247 };
static ullong          cmds[8] = { 0x0000000000403807, 0x0000000000403887, 0x0000000100403807, 0x0000000100403887,
                                   0x0000000000403847, 0x00000000004038c7, 0x0000000100403847, 0x00000001004038c7 };

BWIDTH_F_LOAD(bwidth_amd17df_load)
{
    int i;
    if (tpo->vendor != VENDOR_AMD || tpo->family < FAMILY_ZEN) {
	    debug("%s", Generr.api_incompatible);
        return_msg(, Generr.api_incompatible);
    }
    if (state_fail(msr_test(tpo, MSR_WR))) {
	    debug("msr_test(): %s", state_msg);
        return;
    }
    // Getting the L3 groups
    if (state_fail(topology_select(tpo, &tp, TPSelect.socket, TPGroup.merge, 0))) {
        return;
    }
    debug("CPUS %d", tp.cpu_count);
    // Saving numbers and allocating
    for (i = 0; i < 8; ++i) {
        imcs[i] = (getbits64(cmds[i], 60, 59) <<  6) |
                  (getbits64(cmds[i], 35, 32) <<  2) |
                  (getbits64(cmds[i],  7,  6)      );
    }
    devs_count = tp.cpu_count * 4;
    pool = calloc(devs_count+1, sizeof(bwidth_t));
    //
    apis_put(ops->get_info, bwidth_amd17df_get_info);
    apis_put(ops->init,     bwidth_amd17df_init);
    apis_put(ops->dispose,  bwidth_amd17df_dispose);
    apis_put(ops->read,     bwidth_amd17df_read);
    debug("Loaded AMD17DF");
}

BWIDTH_F_GET_INFO(bwidth_amd17df_get_info)
{
    info->api         = API_AMD17;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    info->devs_count  = devs_count+1;
}

static state_t multiplex(void *something)
{
    static int flip = 0; // New commands
    int sock;
    int i, j;

    debug("flipflop");
    // Flip-flopping the controllers and cleaning the counters
    for (sock = 0; sock < tp.cpu_count; ++sock) {
        for (i = 0, j = flip * 4; i < 4; ++i, ++j) {
            debug("SOCK%d_REG%d: multiplexing to IMC%d 0x%09llx)",
                sock, i, imcs[j], cmds[j]);
            msr_write(tp.cpus[sock].id, (void *) &cmds[j], sizeof(ullong), ctls[i]);
        }
    }
    flip = !flip;
    return EAR_SUCCESS;
}

BWIDTH_F_INIT(bwidth_amd17df_init)
{
    ullong zero = 0LLU;
    int i, sock;
    state_t s;

    for (sock = 0; sock < tp.cpu_count; ++sock) {
        if (state_fail(s = msr_open(tp.cpus[sock].id, MSR_WR))) {
            return s;
        }
    }
    // Cleaning counters and controllers
    for (sock = 0; sock < tp.cpu_count; ++sock) {
        for (i = 0; i < 4; ++i) {
            msr_write(tp.cpus[sock].id, (void *) &zero, sizeof(ullong), ctls[i]);
            msr_write(tp.cpus[sock].id, (void *) &zero, sizeof(ullong), ctrs[i]);
        }
    }
    // First multiplex;
    multiplex(NULL);
    // If monitor is running the multiplexing can be done.
    // If not, we are not flipfloping. But we can already read.
    // Multiplexing suscription
    sus             = suscription();
    sus->call_init  = NULL;
    sus->call_main  = multiplex;
    sus->time_relax = 5000;
    sus->time_burst = 5000;
    return sus->suscribe(sus);
}

BWIDTH_F_DISPOSE(bwidth_amd17df_dispose)
{
    return EAR_SUCCESS;
}

BWIDTH_F_READ(bwidth_amd17df_read)
{
    ullong value;
    int sock;
    int i;
    // Pooling
    timestamp_get(&bws[devs_count].time);
    for (sock = 0; sock < tp.cpu_count; ++sock) {
        for (i = 0; i < 4; ++i) {
            msr_read(tp.cpus[sock].id, &value, sizeof(ullong), ctrs[i]);
            // 48 bits wide registers. Multipied by two because there are 8
            // channels but just 4 registers. The ideal thing would be postpone
            // this multiplication until the GB/s are computed, but this class
            // don't have a diff function yet.
            value = (value & MAXBITS48) * 2LLU;
            // We are not cleaning the register, because although we are
            // setting another event when multiplexing, the value remains
            // in the counter, then the following = is the correct operator.
            bws[(sock*4)+i].cas = value;
        }
    }
    return EAR_SUCCESS;
}
