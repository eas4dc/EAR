/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <common/utils/strtable.h>
#include <metrics/common/hsmp.h>
#include <metrics/common/hsmp_driver.h>
#include <metrics/common/hsmp_esmi.h>
#include <metrics/common/hsmp_pci.h>
#include <pthread.h>
#include <sys/types.h>

#define HSMP_API_FAILED -1
#define HSMP_API_NONE   0
#define HSMP_API_DRIVER 1
#define HSMP_API_ESMI   2
#define HSMP_API_PCI    3

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int api;
static mode_t api_mode;

char *mode_str(mode_t mode)
{
    if (mode == O_RDWR)
        return "O_RDWR";
    return "O_RDONLY";
}

static state_t __hsmp_open(topology_t *tp, mode_t mode)
{
    state_t s;
    api_mode = mode;
    if ((mode == O_RDWR) && state_ok(s = hsmp_pci_open(tp, mode))) {
        debug("HSMP PCI loaded in %s mode", mode_str(mode));
        api = HSMP_API_PCI;
        return s;
    }
    debug("HSMP PCI failed in %s mode: %s", mode_str(mode), state_msg);
    if (state_ok(s = hsmp_esmi_open(tp, mode))) {
        debug("HSMP ESMI loaded in %s mode", mode_str(mode));
        api = HSMP_API_ESMI;
        return s;
    }
    debug("HSMP ESMI failed in %s mode: %s", mode_str(mode), state_msg);
    if (state_ok(s = hsmp_driver_open(tp, mode))) {
        debug("HSMP driver loaded in %s mode", mode_str(mode));
        api = HSMP_API_DRIVER;
        return s;
    }
    debug("HSMP driver failed in %s mode: %s", mode_str(mode), state_msg);
    return EAR_ERROR;
}

state_t hsmp_open(topology_t *tp, mode_t mode)
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (api == HSMP_API_NONE) {
        if (state_fail(__hsmp_open(tp, O_RDWR))) {
            if (state_fail(__hsmp_open(tp, O_RDONLY))) {
                debug("no HSMP API can be opened");
                api = HSMP_API_FAILED;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    // Once opened or failed
    if (api == HSMP_API_FAILED) {
        return_msg(EAR_ERROR, "API failed previously");
    } else if (mode == O_RDWR && api_mode != O_RDWR) {
        debug("The permissions request can be satisfied");
        return_msg(EAR_ERROR, Generr.no_permissions);
    }
    return EAR_SUCCESS;
}

state_t hsmp_close()
{
    state_t s = EAR_ERROR;
    state_msg = Generr.api_uninitialized;
    while (pthread_mutex_trylock(&lock))
        ;
    if (api == HSMP_API_DRIVER)
        s = hsmp_driver_close();
    if (api == HSMP_API_ESMI)
        s = hsmp_esmi_close();
    if (api == HSMP_API_PCI)
        s = hsmp_pci_close();
    api = HSMP_API_NONE;
    pthread_mutex_unlock(&lock);
    return s;
}

static state_t unlock(state_t s)
{
    pthread_mutex_unlock(&lock);
    return s;
}

state_t hsmp_send(int socket, uint function, uint *args, uint *reps)
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (api == HSMP_API_DRIVER)
        return unlock(hsmp_driver_send(socket, function, args, reps));
    if (api == HSMP_API_ESMI)
        return unlock(hsmp_esmi_send(socket, function, args, reps));
    if (api == HSMP_API_PCI)
        return unlock(hsmp_pci_send(socket, function, args, reps));
    pthread_mutex_unlock(&lock);
    return_msg(EAR_ERROR, Generr.api_uninitialized);
}

static void hsmp_test_print_functions(cpu_t *cpu, strtable_t *tb, char *api_str, char *perms_str)
{
    state_t s    = EAR_SUCCESS;
    uint args[2] = {-1, -1};
    uint reps[2] = {-1, -1};
    uint fAar    = cpu->apicid;
    uint f8ar    = 0; // Function 8 argument
    uint f5ar    = 0;

#define ht(dependent, a0, a1, r0, r1, function, line)                                                                  \
    args[0] = a0;                                                                                                      \
    args[1] = a1;                                                                                                      \
    reps[0] = r0;                                                                                                      \
    reps[1] = r1;                                                                                                      \
    if (!dependent || state_ok(s)) {                                                                                   \
        if (state_ok(s = hsmp_send(cpu->socket_id, function, args, reps))) {                                           \
            tprintf2(tb, "%s||%s||%s||%d||%d||%d||%s||%d||%d||%d||-", api_str, perms_str, #function, cpu->socket_id,   \
                     cpu->id, cpu->apicid, "OK", (int) args[0], (int) reps[0], (int) reps[1]);                         \
            line;                                                                                                      \
        } else {                                                                                                       \
            tprintf2(tb, "%s||%s||%s||%d||%d||%d||%s||%d||%d||%d||%s", api_str, perms_str, #function, cpu->socket_id,  \
                     cpu->id, cpu->apicid, "FAILED", (int) args[0], (int) reps[0], (int) reps[1], state_msg);          \
        }                                                                                                              \
    }
    ht(0, 68, -1, 0, -1, HSMP_TEST, );
    ht(0, -1, -1, 0, -1, HSMP_GET_SMU_VER, );
    ht(0, -1, -1, 0, -1, HSMP_GET_PROTO_VER, );
    ht(0, -1, -1, 0, -1, HSMP_GET_SOCKET_POWER, );
    ht(0, -1, -1, 0, -1, HSMP_GET_SOCKET_POWER_LIMIT, f5ar = reps[0]);
    ht(0, f5ar, -1, -1, -1, HSMP_SET_SOCKET_POWER_LIMIT, );
    ht(0, -1, -1, 0, -1, HSMP_GET_SOCKET_POWER_LIMIT_MAX, );
    ht(0, fAar, -1, 0, -1, HSMP_GET_BOOST_LIMIT, f8ar = (cpu->apicid << 16) | reps[0]);
    ht(1, f8ar, -1, -1, -1, HSMP_SET_BOOST_LIMIT, );
    // ht(1, -1, -1, -1, -1, HSMP_SET_BOOST_LIMIT_SOCKET,); // If pthe revious work...
    ht(0, -1, -1, 0, -1, HSMP_GET_PROC_HOT, );
    // ht(1, -1, -1, -1, -1, HSMP_SET_XGMI_LINK_WIDTH,); // We don't use this.
    ht(0, 0, -1, -1, -1, HSMP_SET_DF_PSTATE, );
    ht(0, -1, -1, -1, -1, HSMP_SET_AUTO_DF_PSTATE, );
    ht(0, -1, -1, 0, 0, HSMP_GET_FCLK_MCLK, );
    ht(0, -1, -1, 0, -1, HSMP_GET_CCLK_THROTTLE_LIMIT, );
    ht(0, -1, -1, 0, -1, HSMP_GET_C0_PERCENT, );
    // ht(0, -1, -1, -1, -1, HSMP_SET_NBIO_DPM_LEVEL,); // We don't use this. By now.
    ht(0, -1, -1, 0, -1, HSMP_GET_DDR_BANDWIDTH, );
    // More functions from family 1A
}

void hsmp_test_print()
{
    static topology_t tp_sockets = {0};
    static topology_t tp         = {0};
    static strtable_t tb         = {0};
    int cpu;

    // If more than one socket
    topology_init(&tp);
    topology_select(&tp, &tp_sockets, TPSelect.socket, TPGroup.merge, 0);

    // Initializing the output table
    tprintf_init2(&tb, fdout, STR_MODE_DEF, "8 9 35 4 4 8 8 10 10 10 20");
    tprintf2(&tb, "API||Perms||Function||S||CPU||APICID||State||args0||reps0||reps1||Error msg.");
    tprintf2(&tb, "---||-----||--------||-||---||------||-----||-----||-----||-----||----------");

#define ho(iapi, IAPI, PERMS)                                                                                          \
    if (state_fail(hsmp_##iapi##_open(&tp, PERMS))) {                                                                  \
        tprintf2(&tb, "%s||%s||HSMP_OPEN()||-||-||-||%s||-1||-1||-1||%s", #IAPI, #PERMS, "FAILED", state_msg);         \
    } else {                                                                                                           \
        api = HSMP_API_##IAPI;                                                                                         \
        for (cpu = 0; cpu < tp_sockets.cpu_count; ++cpu) {                                                             \
            hsmp_test_print_functions(&tp_sockets.cpus[cpu], &tb, #IAPI, #PERMS);                                      \
        }                                                                                                              \
        hsmp_##iapi##_close();                                                                                         \
    }
    ho(driver, DRIVER, HSMP_WR);
    ho(driver, DRIVER, HSMP_RD);
    ho(esmi, ESMI, HSMP_WR);
    ho(esmi, ESMI, HSMP_RD);
    ho(pci, PCI, HSMP_WR);
    ho(pci, PCI, HSMP_RD);
}

#if TESTM
int main(int argc, char *argv[])
{
    hsmp_test_print();
    return 0;
}
#endif
#if TEST
int main(int argc, char *argv[])
{
    static topology_t tp = {0};
    topology_init(&tp);
    if (state_ok(hsmp_open(&tp, HSMP_RD))) {
        printf("HSMP opened: API %d\n", api);
    } else {
        printf("HSMP failed: %s\n", state_msg);
    }
    return 0;
}
#endif
