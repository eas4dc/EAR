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
#include <common/sizes.h>
#include <common/system/folder.h>
#include <fcntl.h>
#include <metrics/common/pci.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static char *strdfs(uint dfs, char *string)
{
    uint device   = dfs / 8;
    uint function = dfs % 8;
    sprintf(string, "%.2x.%u", device, function);
    return string;
}

state_t pci_scan(ushort vendor, ushort *ids, char **dfs, mode_t mode, pci_t **pcis, uint *pcis_count)
{
    uint ret_n_errs_perms = 0;
    uint ret_n_errs_other = 0;
    uint ret_n_opened     = 0;
    char *ret_err_other   = NULL;
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
                // sprintf(path, "/sys/bus/pci/devices/0000:%.2x:%s/config", b, dfs[d]);
                sprintf(path, "/proc/bus/pci/%.2x/%s", b, dfs[d]);
            } else {
                // sprintf(path, "/sys/bus/pci/devices/0000:%.2x:%s/config", b, strdfs(d, aux));
                sprintf(path, "/proc/bus/pci/%.2x/%s", b, strdfs(d, aux));
            }
            debug("Trying to open PCI %s", path);
            if ((fd = open(path, mode)) < 0) {
                if (errno == EACCES) {
                    debug("Exists '%s', but no permissions", path);
                    ret_n_errs_perms++;
                } else {
                    debug("Exists '%s', but error %s", path, strerror(errno));
                    ret_err_other = strerror(errno);
                    ret_n_errs_other++;
                }
                continue;
            }
            ret_n_opened++;
            // Taking and comparing vendor
            pread(fd, &vid, sizeof(vid), 0);
            if (vid != vendor) {
                close(fd);
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
                debug("Found PCI '%s' for vendor %xh and id %xh (fd %d)", path, (uint) vendor, (uint) rid, fd);
                strcpy((*pcis)[*pcis_count].path, path);
                (*pcis)[*pcis_count].mode = mode;
                (*pcis)[*pcis_count].fd   = fd;
                *pcis_count += 1;
            } else {
                close(fd);
            }
        }
    }
    if (*pcis_count == 0) {
        free(*pcis);
        *pcis = NULL;
        debug("No PCIs found (%u perms, %u others [%s], %u wrong IDs)", ret_n_errs_perms, ret_n_errs_other,
              ret_err_other, ret_n_opened);
        return_print(EAR_ERROR, "No PCIs found (%u perms, %u others [%s], %u wrong IDs)", ret_n_errs_perms,
                     ret_n_errs_other, ret_err_other, ret_n_opened);
    }
    return EAR_SUCCESS;
}

state_t pci_scan_close(pci_t **pcis, uint *pcis_count)
{
    int p;
    if (pcis == NULL || *pcis == NULL) {
        return EAR_ERROR;
    }
    for (p = 0; p < *pcis_count; ++p) {
        close((*pcis)[p].fd);
    }
    free(*pcis);
    *pcis       = NULL;
    *pcis_count = 0;
    return EAR_SUCCESS;
}

state_t pci_read(pci_t *pci, void *buffer, size_t size, off_t addr)
{
    ssize_t psize;
    errno = 0;
    if ((psize = pread(pci->fd, buffer, size, addr)) != size) {
        debug("PCI %s FD%d read %ld bytes but expected %lu: %s (errno %d)", pci->path, pci->fd, size, psize,
              strerror(errno), errno);
        if (errno == 0) {
            return_msg(EAR_ERROR, "undefined error");
        } else {
            return_msg(EAR_ERROR, strerror(errno));
        }
    }
    return EAR_SUCCESS;
}

state_t pci_write(pci_t *pci, const void *buffer, size_t size, off_t addr)
{
    if (pwrite(pci->fd, buffer, size, addr) != size) {
        debug("PCI %s FD%d failed to write %ld bytes: %s (errno %d)", pci->path, pci->fd, size, strerror(errno), errno);
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

state_t pci_mwrite32(pci_t *pcis, uint pcis_count, const uint *buffer, off_t *addrs, uint addrs_count)
{
    int p, a;
    for (p = 0; p < pcis_count; ++p) {
        for (a = 0; a < addrs_count; ++a) {
#if SHOW_DEBUGS
            uint written = pwrite(pcis[p].fd, &buffer[a], sizeof(uint), addrs[a]);
            debug("PCI%d (fd %d), in address %lx written value 0x%x (%u bytes)", p, pcis[p].fd, addrs[a], buffer[a],
                  written);
#else
            pwrite(pcis[p].fd, &buffer[a], sizeof(uint), addrs[a]);
#endif
        }
    }
    return EAR_SUCCESS;
}

state_t pci_mmio_map(addr_t addr, void **p)
{
    void *maddr;
    int fd;

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
        return_print(EAR_ERROR, "while opening /dev/mem: %s", strerror(errno));
    }
    if ((maddr = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PAGE_MASK(addr))) == MAP_FAILED) {
        return_print(EAR_ERROR, "while mapping /dev/mem: %s", strerror(errno));
    }
    *p = (void *) (((addr_t) maddr) + (addr - PAGE_MASK(addr)));
    debug("addr           : %llx", addr);
    debug("PAGE_MASK(addr): %llx", PAGE_MASK(addr));
    debug("PAGE_SIZE      : %llx", (ullong) PAGE_SIZE);
    debug("maddr          : %llx", (ullong) maddr);
    debug("p              : %llx", (ullong) *p);
#if 0
	if ((*p = ioremap(addr, size)) == NULL) {
		return_msg(EAR_ERROR, strerror(errno));
	}
#endif
    return EAR_SUCCESS;
}

state_t pci_mmio_unmap(void *p)
{
    // iounmap(p);
    return EAR_SUCCESS;
}
