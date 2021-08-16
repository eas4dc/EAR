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
#include <common/sizes.h>
#include <common/output/debug.h>
#include <common/system/folder.h>
#include <metrics/common/pci.h>

static char *strdfs(uint dfs, char *string)
{
	uint device = dfs / 8;
	uint function = dfs % 8;
	sprintf(string, "%.2x.%u", device, function);
	return string;
}

state_t pci_scan(ushort vendor, ushort *ids, char **dfs, mode_t mode, pci_t **pcis, uint *pcis_count)
{
	char path[SZ_PATH];
	folder_t folder;
	int b, d, i, fd;
	ushort rid, vid;
	uint ids_count;
	uint dfs_count;
	char aux[8];

	// Counting
	*pcis_count = 0;
	for (ids_count = i = 0; ids[i] != 0x00; ++i) {
		ids_count++;
	}
	for (dfs_count = i = 0; dfs != NULL && dfs[i] != NULL; ++i) {
		dfs_count++;
	}
	if (dfs_count == 0) {
		dfs_count = 32 * 8;
	}
	*pcis = calloc(32, sizeof(pci_t));
	// Travelling
	for (b = 0; b < 256; ++b) {
		sprintf(path, "/proc/bus/pci/%.2x", b);
		// Discarding bus
		if (state_fail(folder_open(&folder, path))) {
			continue;
		}
		folder_close(&folder);
		// Entering bus 
		for (d = 0; d < dfs_count; ++d) {
			// Opening a PCI device
			if (dfs != NULL) {
				sprintf(path, "/proc/bus/pci/%.2x/%s", b, dfs[d]);
			} else {
				sprintf(path, "/proc/bus/pci/%.2x/%s", b, strdfs(d, aux));
			}
			//debug("Trying to open PCI %s", path);
			if ((fd = open(path, mode)) < 0) {
				if (errno == EACCES) {
					debug("Exists '%s', but no permissions", path);
				}
				continue;
			}
			// Taking and comparing vendor
			pread(fd, &vid, sizeof(vid), 0);
			if (vid != vendor) {
				continue;
			}
			// Taking id
			pread(fd, &rid, sizeof(rid), 2);
			// Looking for IDs
			for (i = 0; i < ids_count; ++i) {
				if (rid == ids[i]) {
					break;
				}
			}
			//
			if (i < ids_count) {
				debug("Found PCI '%s' for vendor %xh and id %xh (fd %d)",
					path, (uint) vendor, (uint) rid, fd);
				(*pcis)[*pcis_count].mode = mode;
				(*pcis)[*pcis_count].fd   = fd;
				*pcis_count += 1;
			} else {
				close(fd);
			}
		}}
	if (*pcis_count == 0) {
		return_msg(EAR_ERROR, "No PCIs found");
	}
	return EAR_SUCCESS;
}

state_t pci_scan_close(pci_t **pcis, uint *pcis_count)
{
	int p;
	for (p = 0; p < *pcis_count; ++p) {
		close((*pcis)[p].fd);
		(*pcis)[p].mode = 0;
		(*pcis)[p].fd = -1;
	}
	*pcis_count = 0;
	return EAR_SUCCESS;
}

state_t pci_read(pci_t *pci, void *buffer, size_t size, off_t addr)
{
	if (pread(pci->fd, buffer, size, addr) != size) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t pci_write(pci_t *pci, const void *buffer, size_t size, off_t addr)
{
	if (pwrite(pci->fd, buffer, size, addr) != size) {
		debug("PCI FAILED");
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t pci_mwrite32(pci_t *pcis, uint pcis_count, const uint *buffer, off_t *addrs, uint addrs_count)
{
	ssize_t written = 0;
	int p, a;
	for (p = 0; p < pcis_count; ++p) {
		for (a = 0; a < addrs_count; ++a) {
			written = pwrite(pcis[p].fd, &buffer[a], sizeof(uint), addrs[a]);
			debug("In PCI%d@%lX written 0x%X (%llu bytes)", p, addrs[a], buffer[a], written);
		}
	}
	// Remove unused
	(void) written;
	return EAR_SUCCESS;
}

