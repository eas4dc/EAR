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
#include <fcntl.h>
#include <metrics/common/hsmp_driver.h>
#include <sys/ioctl.h>
#if !HAS_AMD_HSMP_H
#include <asm-generic/types.h>
#endif

#define HSMP_BY_NOTHING 0
#define HSMP_BY_DRIVER  1
#define HSMP_BY_ESMI    2
#define HSMP_BY_PCI     3
#if !HAS_AMD_HSMP_H
#define HSMP_MAX_MSG_LEN   8
#define HSMP_BASE_IOCTL_NR 0xF8
#define HSMP_IOCTL_CMD     _IOWR(HSMP_BASE_IOCTL_NR, 0, struct hsmp_message)

struct hsmp_message {
    __u32 msg_id;                 /* Message ID */
    __u16 num_args;               /* Number of input argument words in message */
    __u16 response_sz;            /* Number of expected output/response words */
    __u32 args[HSMP_MAX_MSG_LEN]; /* argument/response buffer */
    __u16 sock_ind;               /* socket number */
};
#endif

static int fd = -1;

state_t hsmp_driver_send(int socket, uint function, uint *args, uint *reps)
{
    struct hsmp_message msg;
    int ret;

    if (fd < 0) {
        return_msg(EAR_ERROR, Generr.api_uninitialized);
    }
    memset(&msg, 0, sizeof(struct hsmp_message));
    msg.msg_id   = (__u32) function;
    msg.sock_ind = (__u16) socket;
    debug("msg.msg_id     : 0x%X", msg.msg_id);
    debug("msg.sock_ind   : %u", msg.sock_ind);
    // Counting args and reps
    while (args[msg.num_args] != -1) {
        msg.args[msg.num_args] = (__u32) args[msg.num_args];
        debug("msg.args[%d]    : %u", msg.num_args, msg.args[msg.num_args]);
        msg.num_args++;
    }
    while (reps[msg.response_sz] != -1) {
        msg.response_sz++;
    }
    debug("msg.num_args   : %u", msg.num_args);
    debug("msg.response_sz: %u", msg.response_sz);
    // Contacting the driver
    ret = ioctl(fd, HSMP_IOCTL_CMD, &msg);
    debug("ret = %d, errno = %d", ret, errno);
    if (ret == -1) {
        debug("ioctl failed: %s", strerror(errno));
        return_print(EAR_ERROR, "Error while sending a command to HSMP driver: %s", strerror(errno));
    }
    // Recovering responses
    while (msg.response_sz > 0) {
        reps[msg.response_sz - 1] = (uint) msg.args[msg.response_sz - 1];
        msg.response_sz--;
    }
    return EAR_SUCCESS;
}

state_t hsmp_driver_close()
{
    if (fd != -1) {
        close(fd);
    }
    fd = -1;
    return EAR_SUCCESS;
}

state_t hsmp_driver_open(topology_t *tp, mode_t mode)
{
    state_t s = EAR_SUCCESS;
    if (tp->vendor == VENDOR_INTEL || tp->family < FAMILY_ZEN) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
    if ((fd = open("/dev/hsmp", mode)) < 0) {
        return_print(EAR_ERROR, "Error while opening /dev/hsmp driver file: %s", strerror(errno))
    }
    if (mode == O_RDONLY) {
        debug("HSMP is in user mode");
    }
    // Test ping
    uint args[2] = {68, -1};
    uint reps[2] = {0, -1};
    if (state_fail(s = hsmp_driver_send(0, HSMP_TEST, args, reps))) {
        hsmp_driver_close();
        return_print(EAR_ERROR, "HSMP test function failed: %s", state_msg);
    }
    if (reps[0] != 69) {
        hsmp_driver_close();
        return_print(EAR_ERROR, "HSMP not enabled (incorrect ping value, expected 69, received %u)", reps[0]);
    }
    return EAR_SUCCESS;
}
