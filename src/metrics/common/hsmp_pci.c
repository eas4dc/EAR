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
#include <metrics/common/hsmp_pci.h>
#include <metrics/common/pci.h>
#include <pthread.h>

#define HSMP_NOT_READY    0x00
#define HSMP_OK           0x01
#define HSMP_INVALID_ID   0xFE
#define HSMP_INVALID_ARG  0xFF
#define HSMP_ERR_SMU_BUSY 0xFC // Taken from amd_hsmp.c github driver.

struct smn_s {
    off_t index;
    off_t data;
} smn = {.index = 0xC4, .data = 0xC8};

typedef struct mailopt_s {
    off_t address;
    size_t size;
    char *name;
} mailopt_t;

typedef struct mailbox_s {
    mailopt_t function;
    mailopt_t response;
    mailopt_t arg0; // +4 per arg
} mailbox_t;

static mailbox_t zen2_mailbox = {
    .function.address = 0x3B10534, .function.size    = 4, .function.name    = "function",
    .response.address = 0x3B10980, .response.size    = 1, .response.name    = "response",
    .arg0.address     = 0x3B109E0, .arg0.size        = 4, .arg0.name        = "argument",
};

static mailbox_t zen5_mailbox = {
    .function.address = 0x3B10934, .function.size    = 4, .function.name    = "function",
    .response.address = 0x3B10980, .response.size    = 1, .response.name    = "response",
    .arg0.address     = 0x3B109E0, .arg0.size        = 4, .arg0.name        = "argument",
};

// lspci -d 0x1022:0x1480 (more information on issue 'How to find HSMP device`)
static ushort  amd_vendor       = 0x1022; // AMD
static char   *zen2_pci_dfs[2]  = { "00.0", NULL };
static ushort  zen5_pci_ids[12] = { 0x1450, 0x15d0, 0x1480, 0x1630, 0x14b5, // 17H
                                    0x14a4, 0x14d8, 0x14e8,                 // 19H
                                    0x153a, 0x1507, 0x1122,                 // 1AH
                                    0x00 };
static char  **mist_pci_dfs;
static ushort *mist_pci_ids;
static uint    mist_nbios_count;
//
static mailbox_t *mail;
static uint       sockets_count;
static uint       pcis_count;
static pci_t     *pcis;

state_t hsmp_pci_open(topology_t *tp, mode_t mode)
{
    state_t s;

    if (tp->vendor != VENDOR_AMD || tp->family < FAMILY_ZEN) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
    if (pcis != NULL) {
        return_msg(EAR_SUCCESS, "already loaded");
    }
    sockets_count = tp->socket_count;
    if (tp->family >= FAMILY_ZEN5) {
        debug("ZEN5 detected");
        mist_pci_dfs =  zen2_pci_dfs;
        mist_pci_ids =  zen5_pci_ids;
        mail         = &zen5_mailbox;
    } else {
        debug("ZEN2/3/4 detected");
        mist_pci_dfs =  zen2_pci_dfs;
        mist_pci_ids =  zen5_pci_ids;
        mail         = &zen2_mailbox;
    }
    // Opening PCis
    if (state_fail(s = pci_scan(amd_vendor, mist_pci_ids, mist_pci_dfs, mode, &pcis, &pcis_count))) {
        return_reprint(s, "HSMP failed while scanning PCI: %s", state_msg);
    }
    // Calculating Root Complex per DataFabric. We will need it to access
    // to different sockets Roots.
    mist_nbios_count = pcis_count / sockets_count;
    // Test ping
    uint args[2] = { 68, -1 };
    uint reps[2] = {  0, -1 };
    if (state_fail(s = hsmp_pci_send(0, HSMP_TEST, args, reps))) {
        pci_scan_close(&pcis, &pcis_count);
        return_reprint(EAR_ERROR, "HSMP test function failed: %s", state_msg);
    }
    if (reps[0] != 69) {
        pci_scan_close(&pcis, &pcis_count);
        return_print(EAR_ERROR, "HSMP not enabled (incorrect ping value, expected 69, received %u)", reps[0]);
    }
    debug("Ping returned %d", reps[0]);
    return EAR_SUCCESS;
}

state_t hsmp_pci_close()
{
    return pci_scan_close(&pcis, &pcis_count);
}

static state_t smn_write(pci_t *pci, mailopt_t *opt, uint data)
{
    state_t s;
    if (state_fail(s = pci_write(pci, (const void *) &opt->address, sizeof(uint), smn.index))) {
        debug("pci_write returned: %s", state_msg);
        return s;
    }
    if (state_fail(s = pci_write(pci, &data, opt->size, smn.data))) {
        debug("pci_write returned: %s", state_msg);
        return s;
    }
    debug("PCI FD%d: written value %u (%lu bytes) in %s regiser (0x%lx address)", pci->fd, data, opt->size, opt->name,
          opt->address);
    return EAR_SUCCESS;
}

static state_t smn_read(pci_t *pci, mailopt_t *opt, uint *data)
{
    state_t s;
    if (state_fail(s = pci_write(pci, (const void *) &opt->address, sizeof(uint), smn.index))) {
        debug("pci_write returned: %s", state_msg);
        return s;
    }
    *data = 0;
    if (state_fail(s = pci_read(pci, data, opt->size, smn.data))) {
        debug("pci_write returned: %s", state_msg);
        return s;
    }
    debug("PCI FD%d: readed value %u (%lu bytes) from %s register (0x%lx address)", pci->fd, *data, opt->size,
          opt->name, opt->address);
    return EAR_SUCCESS;
}

state_t hsmp_pci_send(int socket, uint function, uint *args, uint *reps)
{
    struct timespec one_ms = {0, 1000 * 1000};
    mailopt_t arg;
    uint response;
    uint timeout;
    uint intents;
    state_t s;
    int a;

    if (pcis == NULL) {
        return_msg(EAR_ERROR, Generr.api_uninitialized);
    }
    socket   = socket * mist_nbios_count;
    arg.size = mail->arg0.size;
    arg.name = mail->arg0.name;
    response = HSMP_NOT_READY;
    timeout  = 500;
    intents  = 0;
    //
    if (state_fail(s = smn_write(&pcis[socket], &mail->response, response))) {
        return s;
    }
    for (a = 0; args[a] != -1; ++a) {
        arg.address = mail->arg0.address + (a << 2);
        // Getting argument address
        if (state_fail(s = smn_write(&pcis[socket], &arg, args[a]))) {
            return s;
        }
    }
    if (state_fail(s = smn_write(&pcis[socket], &mail->function, function))) {
        return s;
    }
retry:
    nanosleep(&one_ms, NULL);
    // Waiting the mail signal
    if (state_fail(s = smn_read(&pcis[socket], &mail->response, &response))) {
        return s;
    }
    if (response == HSMP_NOT_READY || response == HSMP_ERR_SMU_BUSY) {
        if (--timeout == 0) {
            debug("HSMP didn't answer because TIMEOUT");
            return_msg(EAR_ERROR, "Timeout, waiting too long for an HSMP response");
        }
        ++intents;
        goto retry;
    }
    if (response == HSMP_INVALID_ID) {
        debug("HSMP answered with INVALID_MAIL_FUNCTION");
        return_msg(EAR_ERROR, "Invalid mail function id");
    }
    if (response == HSMP_INVALID_ARG) {
        debug("HSMP answered with INVALID_ARGUMENT");
        return_msg(EAR_ERROR, "Invalid mail argument");
    }
    debug("HSMP answered after %d intents", intents);
    // Reading the values
    for (a = 0; reps[a] != -1; ++a) {
        arg.address = mail->arg0.address + (a << 2);
        // Getting argument address
        if (state_fail(s = smn_read(&pcis[socket], &arg, &reps[a]))) {
            return s;
        }
    }
    return EAR_SUCCESS;
}