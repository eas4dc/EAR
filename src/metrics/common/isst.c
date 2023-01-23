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

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <common/utils/keeper.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <common/hardware/bithack.h>
#include <metrics/common/apis.h>
#include <metrics/common/isst.h>

//
// Find the references in the commentaries in the intel-speed-select application source code.
// 
#define BC(b)                         (~(0x01 << b)) //BIT CLEAN
#define PM_GET_CONFIG                  0x094,0x03,0x00,0x00 //READ_PM_CONFIG,PM_FEATURE
#define PM_SET_CONFIG(o,e)             0x095,0x03,0x00,((o & BC(16)) | (e << 16))  //WRITE_PM_CONFIG,PM_FEATURE
#define PM_DEF_CONFIG(o)               0x095,0x03,0x00,o //WRITE_PM_CONFIG,PM_FEATURE
#define CP_GET_CONFIG                  0x0D0,0x02,0x00,0x00 //CONFIG_CLOS,CLOS_PM_QOS_CONFIG
#define CP_SET_CONFIG(o,e,p)           0x0D0,0x02,(0x01 << 0x08),((o & BC(2) & BC(1)) | (p << 2) | (e << 1)) //CONFIG_CLOS,CLOS_PM_QOS_CONFIG,MBOX_CMD_WRITE_BIT
#define CP_DEF_CONFIG(o)               0x0D0,0x02,(0x01 << 0x08),o //CONFIG_CLOS,CLOS_PM_QOS_CONFIG,MBOX_CMD_WRITE_BIT
#define TF_GET_CONFIG                  0x07F,0x01,0x00,0x00 //CONFIG_TDP,CONFIG_TDP_GET_TDP_CONTROL
#define TF_SET_CONFIG(o,e)             0x07F,0x02,0x00,((o & BC(16)) | (e << 16)) //CONFIG_TDP,CONFIG_TDP_GET_TDP_CONTROL
#define TF_DEF_CONFIG(o)               0x07F,0x02,0x00,o //CONFIG_TDP,CONFIG_TDP_GET_TDP_CONTROL
#define TR_GET_CONFIG                  0x1AD,0x00,0x00 //Turbo Ratio Limit
#define TR_SET_CONFIG(o,e)             0x1AD,(o | ((~(e) + 0x01) & 0xFFFFFFFFFFFFFFFFULL)),0x01 //Turbo Ratio Limit
#define TR_DEF_CONFIG(o)               0x1AD,o,0x01 //Turbo Ratio Limit
#define CP_GET_CLOS(i)                (0x08 + ((i & 0x03) * 0x04)),0x00,0x00 //PM_CLOS_OFFSET
#define CP_SET_CLOS(i, c)             (0x08 + ((i & 0x03) * 0x04)),   c,0x01 //PM_CLOS_OFFSET
#define CP_GET_ASSOC(c)               (0x20 + ((c & 0xFF) * 0x04)),0x00,0x00 //PQR_ASSOC_OFFSET
#define CP_SET_ASSOC(c,a)             (0x20 + ((c & 0xFF) * 0x04)),((a & 0x03) << 16),0x01 //PQR_ASSOC_OFFSET
#define ISST_IF_MAGIC                  0xFE
#define ISST_IF_PHYS_CMD              _IOWR(ISST_IF_MAGIC, 1, struct isst_if_cpu_map *)
#define ISST_IF_INFO_CMD              _IOR (ISST_IF_MAGIC, 0, struct isst_if_platform_info *)
#define ISST_IF_MMIO_CMD              _IOW (ISST_IF_MAGIC, 2, struct isst_if_io_regs *)
#define ISST_IF_MBOX_CMD              _IOWR(ISST_IF_MAGIC, 3, struct isst_if_mbox_cmds *)
#define ISST_IF_MSR_CMD               _IOWR(ISST_IF_MAGIC, 4, struct isst_if_msr_cmds *)
#define CMDS_MAX                       64 //ISST_IF_CMD_LIMIT
#define CMDS_SPLIT(i, total)          (((i + CMDS_MAX) <= total) * CMDS_MAX) + (((i + CMDS_MAX) > total) * (total % CMDS_MAX))
#define CLOS_COUNT                     4

struct isst_if_msr_cmd {
    uint read_write;
    uint logical_cpu;
    ullong msr;
    ullong data;
};

struct isst_if_msr_cmds {
    uint cmd_count;
    struct isst_if_msr_cmd msr_cmd[1];
};

struct isst_if_cpu_map {
    uint logical_cpu;
    uint physical_cpu;
};

struct isst_if_cpu_maps {
    uint cmd_count;
    struct isst_if_cpu_map cpu_map[CMDS_MAX];
};

struct isst_if_platform_info {
    ushort api_version;
    ushort driver_version;
    ushort max_cmds_per_ioctl;
    uchar  mbox_supported;
    uchar  mmio_supported;
};

struct isst_if_io_reg {
    uint read_write;
    uint logical_cpu;
    uint reg;
    uint value;
};

struct isst_if_io_regs {
    uint req_count;
    struct isst_if_io_reg io_reg[CMDS_MAX];
};

struct isst_if_mbox_cmd {
    uint   logical_cpu;
    uint   parameter;
    uint   req_data;
    uint   resp_data;
    ushort command;
    ushort sub_command;
    uint   reserved;
};

struct isst_if_mbox_cmds {
    uint cmd_count;
    struct isst_if_mbox_cmd mbox_cmd[CMDS_MAX];
};

/*
 *
 * My types
 *
 */

typedef struct isst_if_io_regs   cmds_mmio_t;
typedef struct isst_if_mbox_cmds cmds_mbox_t;

typedef struct envals_s {
    uint    pm_reg;
    uint    cp_reg;
    uint    tf_reg;
    ullong  tr_reg;
    uint    cp_support;
    uint    tf_support;
    uint    bf_support;
    uint    pm_enabled;
    uint    cp_enabled;
    uint    tf_enabled;
    uint    bf_enabled;
    clos_t  clos[CLOS_COUNT];
    uint   *assocs;
} envals_t;

__attribute__((unused)) static char *strtype[2]  = { "proportional", "ordered" };
static char      *strprio[2]  = { "high", "low" };
static char      *driver_path = "/dev/isst_interface";
static int        driver_fd   = -1;
static uint       cpus_count; // Total CPUs count
static uint      *punit_cpus;
static envals_t   ev;
static topology_t tp; // Socketed topology

static state_t commands_mmsr_send_single(uint cpu, ullong msr, ullong data, int write, ullong *answer)
{
    struct isst_if_msr_cmds msr_cmds;
    int read = !write;

    // MSR does not have fill function because is barely used.
    if (answer != NULL) {
        *answer = 0;
    }

    msr_cmds.cmd_count              = 1;
    msr_cmds.msr_cmd[0].logical_cpu = cpu;
    msr_cmds.msr_cmd[0].msr         = msr;
    msr_cmds.msr_cmd[0].data        = data * write;
    msr_cmds.msr_cmd[0].read_write  = write;

    // Sending MSR type command through IOCTL. Its ID is
    // copied from ISST application and driver.
    if (ioctl(driver_fd, ISST_IF_MSR_CMD, &msr_cmds) == -1) {
            debug("ioctl failed for MSR command 0x%llX (%d, %s)",
                msr, errno, strerror(errno));
    } else {
        if (read) {
            *answer = msr_cmds.msr_cmd[0].data;
            debug("Answer for MMSR command 0x%llX: 0x%llX", msr, *answer);
        }
    }
    return EAR_SUCCESS;
}

static state_t commands_mmio_send(cmds_mmio_t *cmds, uint *answers)
{
    int i;

    if (cmds->req_count == 0) {
        return EAR_SUCCESS;
    }
    #if 0
    // Extreme debugging
    for (i = 0; i < cmds->req_count; ++i) {
        debug("MMIO%d: cpu %d, reg 0x%02X, value 0x%02X, read_write %d", i,
            cmds->io_reg[i].logical_cpu, cmds->io_reg[i].reg, cmds->io_reg[i].value, cmds->io_reg[i].read_write);
    }
    #endif 
    // Cleaning answers
    for(i = 0; answers != NULL && i < cmds->req_count; ++i) {
        answers[i] = 0;
    }
    // Sending MMIO type command through IOCTL. Its ID is
    // copied from ISST application and driver.
    if (ioctl(driver_fd, ISST_IF_MMIO_CMD, cmds) == -1) {
        debug("ioctl failed to send %d MMIO commands 0x%X: %d, %s",
            cmds->req_count, cmds->io_reg[0].reg, errno, strerror(errno));
        return_msg(EAR_ERROR, strerror(errno));
    }
    // Saving answers 
    for(i = 0; answers != NULL && i < cmds->req_count; ++i) {
        answers[i] = cmds->io_reg[i].value;
    }
    debug("Answer for the %d MMIO commands 0x%02X: 0x%02X",
        cmds->req_count, cmds->io_reg[0].reg, cmds->io_reg[0].value);

    return EAR_SUCCESS;
}

static int commands_mmio_fill(cmds_mmio_t *cmds, uint cpu, uint reg, uint value, int write, uint *answers, int send)
{
    // Fills an array of MMIO commands. If send is 1 or if the array is full
    // of commands, the bulk is passed to the send function.
    cmds->io_reg[cmds->req_count].logical_cpu = cpu;
    cmds->io_reg[cmds->req_count].reg         = reg;
    cmds->io_reg[cmds->req_count].value       = value;
    cmds->io_reg[cmds->req_count].read_write  = write;

    if (++(cmds->req_count) == CMDS_MAX || send) {
        // Sending
        commands_mmio_send(cmds, answers);
        // Cleaning command array
        memset(cmds, 0, sizeof(cmds_mmio_t));
        return 1;
    }
    return 0;
}

static state_t commands_mmio_send_single(uint cpu, uint reg, uint value, int write, uint *answer)
{
    cmds_mmio_t cmds = { 0 };
    commands_mmio_fill(&cmds, cpu, reg, value, write, NULL, 0); 
    return commands_mmio_send(&cmds, answer);
}

static state_t commands_mbox_send(cmds_mbox_t *cmds, uint *answers)
{
    uint retrys = 3;
    int i;  
 
    if (cmds->cmd_count == 0) {
        return EAR_SUCCESS;
    } 
    // Cleaning answers
    for(i = 0; answers != NULL && i < cmds->cmd_count; ++i) {
        answers[i] = 0;
    }
    // Sending MBOX type command through IOCTL. Its ID is
    // copied from ISST application and driver.
    do {
        if (ioctl(driver_fd, ISST_IF_MBOX_CMD, cmds) == -1) {
            debug("ioctl failed to send %d MBOX commands 0x%X_0x%X: %u retrys remaining (%d, %s)",
                cmds->cmd_count, cmds->mbox_cmd[0].command, cmds->mbox_cmd[0].sub_command, retrys, errno, strerror(errno));
            --retrys;
        } else {
            break;
        }
    } while (retrys);
    //
    if (!retrys) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    // Saving answers 
    for(i = 0; answers != NULL && i < cmds->cmd_count; ++i) {
        answers[i] = cmds->mbox_cmd[i].resp_data;
    }
    debug("Answer for MBOX command 0x%X_0x%X: 0x%X",
        cmds->mbox_cmd[0].command, cmds->mbox_cmd[0].sub_command, cmds->mbox_cmd[0].resp_data);
    return EAR_SUCCESS;
}

static int commands_mbox_fill(cmds_mbox_t *cmds, uint cpu, uint cmd1, uint cmd2, uint param, uint req_data, uint *answers, int send)
{
    // Fills an array of MBOX commands. If send is 1 or if the array is full
    // of commands, the bulk is passed to the send function.
    cmds->mbox_cmd[cmds->cmd_count].logical_cpu = cpu;
    cmds->mbox_cmd[cmds->cmd_count].command     = cmd1;
    cmds->mbox_cmd[cmds->cmd_count].sub_command = cmd2;
    cmds->mbox_cmd[cmds->cmd_count].parameter   = param;
    cmds->mbox_cmd[cmds->cmd_count].req_data    = req_data;

    if (++cmds->cmd_count == CMDS_MAX || send) {
        // Sending
        commands_mbox_send(cmds, answers);
        // Cleaning command array
        memset(&cmds, 0, sizeof(cmds_mbox_t));
        return 1;
    }
    return 0;
}

static state_t commands_mbox_send_single(uint cpu, uint cmd1, uint cmd2, uint param, uint req_data, uint *answer)
{
    cmds_mbox_t cmds = { 0 };
    commands_mbox_fill(&cmds, cpu, cmd1, cmd2, param, req_data, NULL, 0);
    return commands_mbox_send(&cmds, answer);
}

/*
 *
 * Not commands
 *
 */

__attribute__((unused)) static void clos_print(clos_t *clos, int fd)
{
    int c;

    for (c = 0; c < CLOS_COUNT; ++c)
    {
        if (clos[c].max_khz == HHZ_TO_KHZ(255)) {
            dprintf(fd, "CLOS%d: MAX GHZ - %.1lf GHz (%s)\n", c,
               ((double) clos[c].min_khz) / 1000000.0, strprio[clos[c].low]);
        } else {
            dprintf(fd, "CLOS%d: %.1lf GHz - %.1lf GHz (%s)\n", c,
               KHZ_TO_GHZ((double) clos[c].max_khz), KHZ_TO_GHZ((double) clos[c].min_khz), strprio[clos[c].low]);
        }
    }
}

#define UINT_TO_CLOS(u, c) \
    c.max_khz = (((u >> 16) & 0xFF) * 100000); \
    c.min_khz = (((u >>  8) & 0xFF) * 100000);
#define CLOS_TO_UINT(c, u) \
    u  = ((KHZ_TO_HHZ(c.max_khz) & 0xFF) << 16); \
    u |= ((KHZ_TO_HHZ(c.min_khz) & 0xFF) <<  8);

static state_t clos_get(clos_t *clos)
{
    cmds_mmio_t cmds = { 0 };
    uint answers[CLOS_COUNT];
    int c;

    for (c = 0; c < CLOS_COUNT; ++c) {
        commands_mmio_fill(&cmds, 0, CP_GET_CLOS(c), answers, c == 3);
    }
    for (c = 0; c < CLOS_COUNT; ++c) {
        clos[c].idx     = c;
        clos[c].low     = c > 1;
        clos[c].high    = c < 2;
        UINT_TO_CLOS(answers[c], clos[c]);
    }
    #if SHOW_DEBUGS
    debug("CLOS GET:");
    clos_print(clos, debug_channel);
    #endif
    return EAR_SUCCESS; 
}

static state_t clos_set(clos_t *clos)
{
    cmds_mmio_t cmds = { 0 };
    uint value;
    int s, c;

    for (s = 0; s < tp.cpu_count; ++s) {
        for (c = 0; c < CLOS_COUNT; ++c) {
            CLOS_TO_UINT(clos[c], value);
            commands_mmio_fill(&cmds, tp.cpus[s].id, CP_SET_CLOS(c, value), NULL, c == 3);
        }
    }
    #if SHOW_DEBUGS
    debug("CLOS SET:");
    clos_get(clos);
    #endif
    return EAR_SUCCESS; 
}

__attribute__((unused)) static void assoc_print(uint *clos)
{
    int cpu;
    for (cpu = 0; cpu < cpus_count; ++cpu) {
        if ((cpu != 0) && (cpu % 8) == 0) dprintf(debug_channel, "\n");
        dprintf(debug_channel, "[%03d,%d] ", cpu, clos[cpu]);
    }
    dprintf(debug_channel, "\n");
}

static state_t assoc_get(uint *clos)
{
    cmds_mmio_t cmds = { 0 };
    int cpu, last;

    for (cpu = last = 0; cpu < cpus_count; ++cpu) {
        if (commands_mmio_fill(&cmds, cpu, CP_GET_ASSOC(punit_cpus[cpu]), &clos[last], cpu == (cpus_count - 1))) {
            last = cpu + 1;
        }
    }
    for (cpu = 0; cpu < cpus_count; ++cpu) {
        clos[cpu] = (clos[cpu] >> 16) & 0x03;
    }
    #if SHOW_DEBUGS
    debug("ASSOC GET:"); 
    assoc_print(clos);
    #endif

    return EAR_SUCCESS;
}

static state_t assoc_set_single(uint clos, int cpu)
{
    debug("associating CPU%d to CLOS%u", cpu, clos);
    return commands_mmio_send_single(cpu, CP_SET_ASSOC(punit_cpus[cpu], clos), NULL);
}

static state_t assoc_set(uint *clos)
{
    cmds_mmio_t cmds = { 0 };
    int cpu;
    for (cpu = 0; cpu < cpus_count; ++cpu) {
        if (clos[cpu] != ISST_SAME_PRIO) {
            commands_mmio_fill(&cmds, cpu, CP_SET_ASSOC(punit_cpus[cpu], clos[cpu]), NULL, cpu == (cpus_count - 1));
        }
    }
    // SAME_PRIO avoided sending flag. If there are pending commands, its time to send.
    if (cmds.req_count > 0) {
        commands_mmio_send(&cmds, NULL);
    }
    #if SHOW_DEBUGS
    debug("ASSOC SET:"); 
    assoc_get(clos);
    #endif
    
    return EAR_SUCCESS;
}

void bprintf(const char *title, void *p, size_t size)
{
    uchar *b = (uchar*) p;
    uchar byte;
    int i, j;
  
    printf("%s: ", title);
    for (i = 7; i >= size; i--) {
        for (j = 7; j >= 0; j--) {
            printf("_");
        }
        printf(" ");
    }
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
        printf(" ");
    }
    printf("\n");
}

static state_t status_get(envals_t *en)
{
    state_t s;
    uint i;

    // 1 Socket is enough
    for (i = 0; i < 1; ++i) {
        // Enabling PM (¿What is PM?) Don't kwnow but the ISST application does this thing.
        if (state_fail(s = commands_mbox_send_single(tp.cpus[i].id, PM_GET_CONFIG, &en->pm_reg))) {
             return s;
        }
        // Get current core-power status (prio 0: proportional, prio 1: ordered)
        if (state_fail(s = commands_mbox_send_single(tp.cpus[i].id, CP_GET_CONFIG, &en->cp_reg))) {
            return s;
        }
        // Get current turbo-freq status
        if (state_fail(s = commands_mbox_send_single(tp.cpus[i].id, TF_GET_CONFIG, &en->tf_reg))) {
            return s;
        }
        // Get current turbo-ratio-limit status
        if (state_fail(s = commands_mmsr_send_single(tp.cpus[i].id, TR_GET_CONFIG, &en->tr_reg))) {
            return s;
        }
        #if SHOW_DEBUGS
        #if 0
         printf("             63    56 55    48 47    40 39    32 31    24 23    16 15     8 7      0\n");
        bprintf("pm register", &en->pm_reg, sizeof(uint)); 
        bprintf("cp register", &en->cp_reg, sizeof(uint)); 
        bprintf("tf register", &en->tf_reg, sizeof(uint)); 
        bprintf("tr register", &en->tr_reg, sizeof(ullong));
        #else
        debug("PM register: 0x%X", en->pm_reg)
        debug("CP register: 0x%X", en->cp_reg)
        debug("TF register: 0x%X", en->tf_reg)
        debug("TR register: 0x%llX", en->tr_reg)
        #endif
        #endif
        en->cp_support = getbits32(en->cp_reg,  0,  0);
        en->tf_support = getbits32(en->tf_reg,  0,  0);
        en->bf_support = getbits32(en->tf_reg,  1,  1);
        en->pm_enabled = getbits32(en->pm_reg, 16, 16);
        en->cp_enabled = getbits32(en->cp_reg,  1,  1);
        en->tf_enabled = getbits32(en->tf_reg, 16, 16);
        en->bf_enabled = getbits32(en->tf_reg, 17, 17);
        // Printing details
        debug("CP support : %d", en->cp_support);
        debug("TF support : %d", en->tf_support);
        debug("BF support : %d", en->bf_support);
        debug("PM enabled : %d", en->pm_enabled);
        debug("CP enabled : %d (%s)", en->cp_enabled, strtype[getbits32(en->tf_reg, 2, 2)]);
        debug("TF enabled : %d", en->tf_enabled);
        debug("BF enabled : %d", en->cp_enabled);
        // Failing
        if (!en->cp_support || !en->tf_support) {
            return_msg(EAR_ERROR, "CP/TF not supported");
        }
    }
    // Overwriting read values by the registers original values
    if (!keeper_load_uint32("IsstPmGetConfigReg", &en->pm_reg)) {
        keeper_save_uint32("IsstPmGetConfigReg", en->pm_reg);
    }
    if (!keeper_load_uint32("IsstCpGetConfigReg", &en->cp_reg)) {
        keeper_save_uint32("IsstCpGetConfigReg", en->cp_reg);
    }
    if (!keeper_load_uint32("IsstTfGetConfigReg", &en->tf_reg)) {
        keeper_save_uint32("IsstTfGetConfigReg", en->tf_reg);
    }
    if (!keeper_load_uint64("IsstTrGetConfigReg", &en->tr_reg)) {
        keeper_save_uint64("IsstTrGetConfigReg", en->tr_reg);
    }
    #if SHOW_DEBUGS
    debug("Keeper values:");
    debug("PM register: 0x%X", en->pm_reg)
    debug("CP register: 0x%X", en->cp_reg)
    debug("TF register: 0x%X", en->tf_reg)
    debug("TR register: 0x%llX", en->tr_reg)
    #endif

    return EAR_SUCCESS;
}

static state_t cpus_map()
{
    struct isst_if_cpu_maps map;
    int i, j;

    // Mapping is required because the driver has its own
    // CPU mapping procedure.
    for (i = 0; i < cpus_count; i += CMDS_MAX) {
        // THe CMDS_SPLIT macros is legacy code. It creates bulks of
        // up to CMDS_MAX commands, and takes into the account the rest
        // using a logic formula. The commands "fill style" functions
        // are the new approach.
        map.cmd_count = CMDS_SPLIT(i, cpus_count);
        for (j = 0; j < map.cmd_count; ++j) {
            map.cpu_map[j].logical_cpu = i+j;
        }
        debug("Mapping [%3d - %3d] (#%d)", i, i+CMDS_MAX, map.cmd_count);
        if (ioctl(driver_fd, ISST_IF_PHYS_CMD, &map) == -1) {
            debug("ioctl failed in MAP CMD: %s", strerror(errno));
            return_msg(EAR_ERROR, strerror(errno));
        }
        for (j = 0; j < map.cmd_count; ++j) {
            punit_cpus[i+j] = (map.cpu_map[j].physical_cpu >> 1);
        }
    }
    #if SHOW_DEBUGS
    for (i = 0; i < cpus_count; ++i) {
        if ((i != 0) && (i % 8) == 0) dprintf(debug_channel, "\n");
        dprintf(debug_channel, "[%03d,%03d] ", i, punit_cpus[i]);
    }
    dprintf(debug_channel, "\n");
    #endif
    return EAR_SUCCESS;
}

static state_t init()
{
    struct isst_if_platform_info info = { 0 };
    state_t s;
    uint aux;

    // Revealing the first information about ISST system.
    if (ioctl(driver_fd, ISST_IF_INFO_CMD, &info) == -1) {
        debug("ioctl failed: %s", strerror(errno));
        return 0;
    }
    debug("API version   : %d", info.api_version);
    debug("Driver version: %d", info.driver_version);
    debug("MBOX supported: %d", info.mbox_supported);
    debug("MMIO supported: %d", info.mmio_supported);

    if (!info.mbox_supported || !info.mmio_supported) {
        return_msg(EAR_ERROR, "MBOX/MMIO not supported");
    }
    // Mapping CPUs
    if (state_fail(s = cpus_map())) {
        return s;
    }
    // Saving initial status
    if (state_fail(s = status_get(&ev))) {
        return s;
    }
    // Saving initial CLOS
    if (state_fail(s = clos_get(ev.clos))) {
        return s;
    }
    // Saving initial ASSOCs
    if (!keeper_load_auint32("IsstAssocs", &ev.assocs, &aux)) {
        ev.assocs = calloc(cpus_count, sizeof(uint));
        if (state_fail(s = assoc_get(ev.assocs))) {
            return s;
        }
        keeper_save_auint32("IsstAssocs", ev.assocs, cpus_count);
    } else {
        #if SHOW_DEBUGS
        debug("ASSOC READ FROM KEEPER:"); 
        assoc_print(ev.assocs);
        #endif
    }

    return EAR_SUCCESS;
}

state_t isst_init(topology_t *_tp)
{
    // Opening driver
    if ((driver_fd = open(driver_path, O_RDWR)) < 0) {
        return EAR_ERROR;
    }
    debug("Opened %s with FD %d", driver_path, driver_fd);
    // Counting CPUs 
    cpus_count = _tp->cpu_count;
    // Allocating SST CPU mapping
    punit_cpus = calloc(cpus_count, sizeof(uint));
    // Getting the list of sockets
    topology_select(_tp, &tp, TPSelect.socket, TPGroup.merge, 0);
    // Mapping the CPUs SST style
    return init();
}

static state_t reset()
{
    state_t s;
    int i;

    for (i = 0; i < tp.cpu_count; ++i) {
        if (state_fail(s = commands_mbox_send_single(tp.cpus[i].id, PM_DEF_CONFIG(ev.pm_reg), NULL))) {
            return s;
        }
        if (state_fail(s = commands_mbox_send_single(tp.cpus[i].id, TF_DEF_CONFIG(ev.tf_reg), NULL))) {
            return s;
        }
        if (state_fail(s = commands_mmsr_send_single(tp.cpus[i].id, TR_DEF_CONFIG(ev.tr_reg), NULL))) {
            return s;
        }
        if (state_fail(s = commands_mbox_send_single(tp.cpus[i].id, CP_DEF_CONFIG(ev.cp_reg), NULL))) {
            return s;
        }
    }
    return assoc_set(ev.assocs);
}

state_t isst_dispose()
{
    return reset();
}

static state_t enable(int e)
{
    state_t s;
    int i;

    // E is 1 for enabled and 0 for disable. As can be seen in the following code, e changes
    // the order of commands. The sequence is the following:
    //     0: PM -> CP -> TF -> TR
    //     1: TF -> TR -> CP -> PM
    // It is not clear if there is any difference when switching between TR and TF.
    for (i = 0; i < tp.cpu_count; ++i)
    {
        #if 1
        if (e && state_fail(s = commands_mbox_send_single(tp.cpus[i].id, PM_SET_CONFIG(ev.pm_reg, e), NULL))) {
            return s;
        }
        if (e && state_fail(s = commands_mbox_send_single(tp.cpus[i].id, CP_SET_CONFIG(ev.cp_reg, e, e), NULL))) {
            return s;
        }
        if (state_fail(s = commands_mbox_send_single(tp.cpus[i].id, TF_SET_CONFIG(ev.tf_reg, e), NULL))) {
            return s;
        }
        if (state_fail(s = commands_mmsr_send_single(tp.cpus[i].id, TR_SET_CONFIG(ev.tr_reg, e), NULL))) {
            return s;
        }
        if (!e && state_fail(s = commands_mbox_send_single(tp.cpus[i].id, CP_SET_CONFIG(ev.cp_reg, e, e), NULL))) {
            return s;
        }
        if (!e && state_fail(s = commands_mbox_send_single(tp.cpus[i].id, PM_SET_CONFIG(ev.pm_reg, e), NULL))) {
            return s;
        }
        #endif
    }
    return EAR_SUCCESS;
}

state_t isst_enable()
{
    return enable(1);
}

state_t isst_disable()
{
    return enable(0);
}

int isst_is_enabled()
{
    envals_t en;
    status_get(&en);
    return en.cp_enabled && en.tf_enabled;
}

state_t isst_reset()
{
    return reset();
}

state_t isst_clos_count(uint *count)
{
    if (count != NULL) {
        *count = CLOS_COUNT;
    }
    return EAR_SUCCESS;
}

state_t isst_clos_alloc(clos_t **clos_list)
{
    *clos_list = calloc(CLOS_COUNT, sizeof(clos_t));
    return EAR_SUCCESS;
}

state_t isst_clos_get_list(clos_t *clos_list)
{
    return clos_get(clos_list);
}

state_t isst_clos_set_list(clos_t *clos_list)
{
    return clos_set(clos_list);
}

state_t isst_assoc_count(uint *count)
{
    if (count != NULL) {
        *count = cpus_count;
    }
    return EAR_SUCCESS;
}

state_t isst_assoc_alloc_list(uint **idx_list)
{
    *idx_list = calloc(cpus_count, sizeof(uint));
    return EAR_SUCCESS;
}

state_t isst_assoc_get_list(uint *idx_list)
{
    return assoc_get(idx_list); 
}

state_t isst_assoc_set_list(uint *idx_list)
{
    return assoc_set(idx_list); 
}

state_t isst_assoc_set(uint idx, int cpu)
{
    uint fake_list[2048];
    int i;

    if (cpu == all_cpus) {
        for (i = 0; i < cpus_count; ++i) {
            fake_list[i] = idx;
        }
        return assoc_set(fake_list);
    }
    return assoc_set_single(idx, cpu);
}
