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
static char  *zen2_pci_dfs[2]   = { "00.0", NULL };
static ushort zen2_pci_ids[2]   = { 0x1480, 0x00 };
static ushort zen2_vendor       = 0x1022; // AMD
static uint   zen2_nbios_count  = 4;
//
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static mailbox_t *mail;
static uint       sockets_count;
static uint       pcis_count;
static pci_t     *pcis;

static state_t unlock_msg(state_t s, char *msg)
{
	pthread_mutex_unlock(&lock);
	debug("unloking with message %s", msg);
	return_msg(s, msg);
}

state_t hsmp_scan(topology_t *tp)
{
	state_t s;

	while (pthread_mutex_trylock(&lock));
	if (pcis != NULL) {
		return unlock_msg(EAR_SUCCESS, "");
	}
	if (tp->vendor == VENDOR_INTEL || tp->family < FAMILY_ZEN) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	sockets_count = tp->socket_count;
	// By now just ZEN2 and greater
	if (tp->family >= FAMILY_ZEN) {
		if (state_fail(s = pci_scan(zen2_vendor, zen2_pci_ids, zen2_pci_dfs, O_RDWR, &pcis, &pcis_count))) {
			return unlock_msg(s, state_msg);
		}
		if (pcis_count != (sockets_count*zen2_nbios_count)) {
			return unlock_msg(EAR_ERROR, "Not enough NBIO devices detected");
		}
		mail = &zen2_mailbox;
		// TODO: test function
	}
	return unlock_msg(EAR_SUCCESS, "");
}

state_t hsmp_close()
{
	// Nothing
	return EAR_SUCCESS;
}

static state_t smn_write(pci_t *pci, mailopt_t *opt, uint data)
{
	state_t s;
	if (state_fail(s = pci_write(pci, (const void *) &opt->address, sizeof(uint), smn.index))) {
		debug("pci_write returned: %s (%d)", state_msg, s);
		return s;
	}
	if (state_fail(s = pci_write(pci, &data, opt->size, smn.data))) {
		debug("pci_write returned: %s (%d)", state_msg, s);
		return s;
	}
	debug("PCI%d: written %u (%lu) in %s (%x)", pci->fd, data, opt->size, opt->name, opt->address);
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
	debug("PCI%d: readed %u (%lu) from %s (%x)", pci->fd, *data, opt->size, opt->name, opt->address);
	return EAR_SUCCESS;
}

state_t hsmp_send(int socket, uint function, uint *args, uint *reps)
{
	struct timespec one_ms = { 0, 1000 * 1000};
	mailopt_t arg;
	uint response;
	uint timeout;
	uint intents;
	state_t s;
	int a;

	socket   = socket * 4;
	arg.size = mail->arg0.size;
	arg.name = mail->arg0.name;
	response = HSMP_NOT_READY;
	timeout  = 500;
	intents  = 0;
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
			return unlock_msg(s, "Timeout, waiting too long for an HSMP response");
		}
		++intents;
		goto retry;
	}
	if (response == HSMP_INVALID_ID) {
		return unlock_msg(s, "Invalid mail function id");
	}
	if (response == HSMP_INVALID_ARG) {
		return unlock_msg(s, "Invalid mail argument");
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

#if 0
int main(int argc, char *arg[])
{
	uint args0[1] = { -1 };
	uint reps0[1] = { -1 };
	uint args1[2] = {  0, -1 };
	uint reps1[2] = {  0, -1 };
	uint args2[3] = {  0, 0, -1 };
	uint reps2[3] = {  0, 0, -1 };

	state_t s;

	topology_t tp;

	topology_init(&tp);

	if (state_fail(s = hsmp_open(&tp))) {
		serror("Failed during HSMP open");
		return 0;
	}

	#if 0
	// Testing sockets
	if (state_fail(s = hsmp_send(0, 0x01, args5, reps5))) {
		serror("Failed during HSMP test");
		return 0;
	}
	printf("Ping0 returned %d\n", reps5[0]);
	
	
	hsmp_send(1, 0x01, args5, reps5);
	printf("Ping1 returned %d\n", reps5[0]);
	
	// Getting firmware version
	hsmp_send(0, 0x02, args0, reps5);
	printf("Firmware returned %d\n", reps5[0]);
	
	// Getting interface version
	hsmp_send(0, 0x03, args0, reps5);
	printf("Interface returned %d\n", reps5[0]);

	// Enabling ABP
	hsmp_send(0, 0x0e, args0, reps0);
	printf("0x0e answered\n");
	#endif	

	// Asking frequency	
	ullong freqs[4];
	memset(freqs, 0, sizeof(ullong)*4);

	uint i;
	uint j;
	for (i = j = 0; i < 4; ++i, ++j) {
		args1[0] = i;
		hsmp_send(0, 0x0d, args1, reps0);
		hsmp_send(0, 0x0f, args0, reps2);
		if (i > 0 && reps2[0] == (uint) freqs[i-1]) {
			--i;
		} else {
			printf("PS%d: answered %u/%u (%u intents)\n", i, reps2[0], reps2[1], j);
			freqs[i] = (ullong) reps2[0];
			j = 0;
		}
	}

	return 0;
}
#endif
