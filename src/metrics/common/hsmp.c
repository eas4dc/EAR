/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <common/hardware/topology.h>
#include <metrics/common/pci.h>
#include <metrics/common/hsmp.h>

#define HSMP_NOT_READY   0x00
#define HSMP_OK          0x01
#define HSMP_INVALID_ID  0xFE
#define HSMP_INVALID_ARG 0xFF

struct smn_s {
	off_t index;
	off_t data;
} smn = { .index = 0xC4, .data = 0xC8 };

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
	.function.address = 0x3B10534, .function.size = 4, .function.name = "function",
	.response.address = 0x3B10980, .response.size = 1, .response.name = "response",
	    .arg0.address = 0x3B109E0,     .arg0.size = 4,     .arg0.name = "argument",
};

// lspci -d 0x1022:0x1480
static ushort  amd_vendor        = 0x1022; // AMD
static char   *zen2_pci_dfs[2]   = { "00.0", NULL };
static ushort  zen2_pci_ids[6]   = { 0x1450, 0x15d0, 0x1480, 0x1630, 0x14b5, 0x00 };
static ushort  zen3_pci_ids[7]   = { 0x14a4, 0x14b5, 0x14d8, 0x14e8, 0x1480, 0x153a, 0x1507, 0x00 };
static uint    zen2_nbios_count  = 4;
static char  **mist_pci_dfs;
static ushort *mist_pci_ids;
static uint    mist_nbios_count;

//
static pthread_mutex_t lock0 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock  = PTHREAD_MUTEX_INITIALIZER;
static mailbox_t *mail;
static uint       sockets_count;
static uint       pcis_count;
static pci_t     *pcis;
static uint       hsmp_failed;

static state_t unlock_msg0(state_t s, char *msg)
{
    debug("Exitting because: %s", msg);
    hsmp_failed = state_fail(s);
	pthread_mutex_unlock(&lock0);
	return_msg(s, msg);
}

static state_t unlock_msg(state_t s, char *msg)
{
    debug("Exitting because: %s", msg);
	pthread_mutex_unlock(&lock);
	return_msg(s, msg);
}

state_t hsmp_scan(topology_t *tp)
{
	state_t s;

	if (tp->vendor == VENDOR_INTEL || tp->family < FAMILY_ZEN) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	while (pthread_mutex_trylock(&lock0));
	if (pcis != NULL) {
        if (hsmp_failed) {
            return unlock_msg0(EAR_ERROR, "HSMP doesn't work");
        } else {
            return unlock_msg0(EAR_SUCCESS, "");
        }
	}
	sockets_count = tp->socket_count;
	// By now just ZEN2 and greater
	if (tp->family >= FAMILY_ZEN) {
        // ZEN selection
        if (tp->family >= FAMILY_ZEN3) {
            debug("ZEN3 detected");
            mist_pci_dfs     = zen2_pci_dfs;
            mist_pci_ids     = zen3_pci_ids;
            mist_nbios_count = zen2_nbios_count;
        } else {
            debug("ZEN2 detected");
            mist_pci_dfs     = zen2_pci_dfs;
            mist_pci_ids     = zen2_pci_ids;
            mist_nbios_count = zen2_nbios_count;
        }
        // Opening PCis
        if (state_fail(s = pci_scan(amd_vendor, mist_pci_ids, mist_pci_dfs, O_RDWR, &pcis, &pcis_count))) {
            return unlock_msg0(s, state_msg);
        }
		if (pcis_count != (sockets_count*mist_nbios_count)) {
			return unlock_msg0(EAR_ERROR, "Not enough NBIO devices detected");
		}
		mail = &zen2_mailbox;
		// TODO: test function
        uint args[2] = { 68, -1 };
        uint reps[2] = {  0, -1 };
        
        if (state_fail(s = hsmp_send(0, HSMP_TEST, args, reps))) {
            return unlock_msg0(s, "HSMP not enabled");
        }
        // Ping checking
        if (reps[0] != 69) {
            return unlock_msg0(EAR_ERROR, "HSMP not enabled (incorrect ping value)");
        }
        debug("Ping returned %d", reps[0]);
	}
	return unlock_msg0(EAR_SUCCESS, "OK");
}

state_t hsmp_close()
{
	// Nothing
	return EAR_SUCCESS;
}

static state_t smn_write(pci_t *pci, mailopt_t *opt, uint data)
{
	state_t s;
    //debug("smn_write");
	if (state_fail(s = pci_write(pci, (const void *) &opt->address, sizeof(uint), smn.index))) {
		debug("pci_write returned: %s (%d)", state_msg, s);
		return s;
	}
	if (state_fail(s = pci_write(pci, &data, opt->size, smn.data))) {
		debug("pci_write returned: %s (%d)", state_msg, s);
		return s;
	}
	debug("PCI%d: written %u (%lu) in %s (%lx)",
		pci->fd, data, opt->size, opt->name, opt->address);
	return EAR_SUCCESS;
}

static state_t smn_read(pci_t *pci, mailopt_t *opt, uint *data)
{
	state_t s;
	if (state_fail(s = pci_write(pci, (const void *) &opt->address, sizeof(uint), smn.index))) {
		debug("pci_write returned: %s (%d)", state_msg, s);
		return s;
	}
	*data = 0;
	if (state_fail(s = pci_read(pci, data, opt->size, smn.data))) {
		debug("pci_write returned: %s (%d)", state_msg, s);
		return s;
	}
	debug("PCI%d: Readed %u (%lu) from %s (%lx)",
		pci->fd, *data, opt->size, opt->name, opt->address);
	return EAR_SUCCESS;
}

state_t hsmp_send(int socket, uint function, uint *args, uint *reps)
{
	struct timespec one_ms = { 0, 1000 * 1000 };
	mailopt_t arg;
	uint response;
	uint timeout;
	state_t s;
	int a;

	socket   = socket * 4;
	arg.size = mail->arg0.size;
	arg.name = mail->arg0.name;
	response = HSMP_NOT_READY;
	timeout  = 500;
	uint intents  = 0;
	// Locking
	while (pthread_mutex_trylock(&lock));
	//
	if (state_fail(s = smn_write(&pcis[socket], &mail->response, response))) {
		return unlock_msg(s, state_msg);
	}
	for (a = 0; args[a] != -1; ++a) {
		arg.address = mail->arg0.address + (a<<2);
		// Getting argument address
		if (state_fail(s = smn_write(&pcis[socket], &arg, args[a]))) {
			return unlock_msg(s, state_msg);
		}
	}
	if (state_fail(s = smn_write(&pcis[socket], &mail->function, function))) {
		return unlock_msg(s, state_msg);
	}
retry:
	nanosleep(&one_ms, NULL);
	// Waiting the mail signal
	if (state_fail(s = smn_read(&pcis[socket], &mail->response, &response))) {
		return unlock_msg(s, state_msg);
	}
	if (response == HSMP_NOT_READY) {
		if (--timeout == 0) {
			return unlock_msg(EAR_ERROR, "Timeout, waiting too long for an HSMP response");
		}
		++intents;
		goto retry;
	}
	if (response == HSMP_INVALID_ID) {
		return unlock_msg(EAR_ERROR, "Invalid mail function id");
	}
	if (response == HSMP_INVALID_ARG) {
		return unlock_msg(EAR_ERROR, "Invalid mail argument");
	}
	debug("HSMP answered after %d intents", intents);
	// Reading the values
	for (a = 0; reps[a] != -1; ++a) {
		arg.address = mail->arg0.address + (a<<2);
		// Getting argument address
		if (state_fail(s = smn_read(&pcis[socket], &arg, &reps[a]))) {
			return unlock_msg(s, state_msg);
		}
	}
	return unlock_msg(EAR_SUCCESS, "");
}

#if TEST
int main(int argc, char *argv[])
{
	topology_t tp;
	state_t s;

	topology_init(&tp);

	if (state_fail(s = hsmp_scan(&tp))) {
        printf("HSMP: FAILED (%s)\n", state_msg);
	} else {
        printf("HSMP: SUCCESS\n");
    }
	return 0;
}
#endif
