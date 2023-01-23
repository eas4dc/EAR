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

#ifndef EAR_PCI_H
#define EAR_PCI_H

#include <unistd.h>
#include <common/types.h>
#include <common/states.h>

#define PAGE_SIZE        sysconf(_SC_PAGE_SIZE)
#define PAGE_MASK(addr)  (addr & ~(PAGE_SIZE - 1))

typedef struct pci_s {
	off_t  map_addrs[32];
	void  *map_ptxs[32];
	char   path[128];
	mode_t mode;
	int fd;
} pci_t;

state_t pci_scan(ushort vendor, ushort *ids, char **dfs, mode_t mode, pci_t **pcis, uint *pcis_count);

state_t pci_scan_close(pci_t **pcis, uint *pcis_count);

state_t pci_read(pci_t *pci, void *buffer, size_t size, off_t addr);

state_t pci_write(pci_t *pci, const void *buffer, size_t size, off_t addr);

state_t pci_mwrite32(pci_t *pcis, uint pcis_count, const uint *buffer, off_t *addrs, uint addrs_count);

/* Maps a physical address to the void pointer p. The size is a PAGE. */
state_t pci_mmio_map(addr_t addr, void **p);

state_t pci_mmio_unmap(void *p);

#endif //EAR_PCI_H
