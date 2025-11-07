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
#include <fcntl.h>
#include <metrics/common/hwmon.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static state_t static_read(int fd, char *buffer, size_t size)
{
    int i, r, p;
    size -= 1;
    for (i = 0, r = 1, p = size; i < size && r > 0;) {
        r = pread(fd, (void *) &buffer[i], p, i);
        i = i + r;
        p = size - i;
    }
    buffer[i] = '\0';
    if (r == -1) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

static uint test_device_name(int fd, const char *name)
{
    char data[SZ_PATH];
    uint correct = 0;
    memset(data, 0, SZ_PATH);
    if (state_ok(static_read(fd, data, SZ_PATH))) {
        int len = strlen(data);
        if (data[len - 1] == '\n') {
            data[len - 1] = '\0';
        }
#if SHOW_DEBUGS
        debug("comparing the driver name '%s' with '%s'", name, data);
#endif
        if (strstr(data, name) != NULL)
            correct = 1;
    }
    close(fd);
    return correct;
}

state_t hwmon_find_drivers(const char *name, uint **ids, uint *n)
{
    char data[SZ_PATH];
    int i = 0; // index
    int c = 0; // count
    int m = 0; // mode
    int fd;
    uint def_path = 0, dev_path = 0, dev_oem = 0;
    uint fd_found = 0, dev_found = 0;

    while (1) {
        debug("Testing %d.........mode %d", i, m);
        do {
            def_path = 0;
            dev_path = 0;
            dev_oem  = 0;
            sprintf(data, "/sys/class/hwmon/hwmon%d/name", i);
            debug("opening file '%s'", data);
            //
            if ((fd = open(data, O_RDONLY)) >= 0) {
                def_path = 1;
                if ((dev_found = test_device_name(fd, name)))
                    goto detected;
            }
            fd_found = def_path || dev_path || dev_oem;
            debug("File %s open returns %d", data, fd);
            if (!fd_found || !dev_found) {
                sprintf(data, "/sys/class/hwmon/hwmon%d/device/name", i);
                debug("opening device path name file '%s'", data);
                if ((fd = open(data, O_RDONLY)) >= 0) {
                    dev_path = 1;
                    if ((dev_found = test_device_name(fd, name)))
                        goto detected;
                }
            }
            fd_found = def_path || dev_path || dev_oem;
            if (!fd_found || !dev_found) {
                sprintf(data, "/sys/class/hwmon/hwmon%d/device/power1_oem_info", i);
                debug("opening device path name file '%s'", data);
                if ((fd = open(data, O_RDONLY)) >= 0) {
                    dev_oem = 1;
                    if ((dev_found = test_device_name(fd, name)))
                        goto detected;
                }
            }
            fd_found = def_path || dev_path || dev_oem;
            debug("paths %u / %u / %u", def_path, dev_path, dev_oem);
            /* If one of the paths is found we check the name */
            if (!fd_found)
                break;        /* File not found */
            if (!dev_found) { /* File is found but name doesn't corresponds with */
                i++;
                continue;
            }
        /* At this point we have detected one device */
        detected:
            debug("hwmon found in pos %d", i);
            if (m == 1) {
                (*ids)[c] = i;
            }
            c += 1;
            ++i;
        } while (1);

        if (c == 0) {
            return_msg(EAR_ERROR, "no drivers found");
        }
        if (ids == NULL) {
            return EAR_SUCCESS;
        }
        if (m == 1) {
            return EAR_SUCCESS;
        }

        *ids = (uint *) calloc(c, sizeof(uint));
        *n   = c;
        // Resetting for next iteration
        i = 0;
        c = 0;
        m = 1;
    }

    return EAR_SUCCESS;
}

state_t hwmon_open_files(uint id, char *files, int **fds, uint *n, int i_start)
{
    char data[SZ_PATH];
    int i = i_start; // index
    int c = 0;       // count
    int m = 0;       // mode
    int fd;

    debug("hwmon_open_files................");
    while (1) {
        do {
            sprintf(data, (char *) files, id, i);
            debug("opening file '%s'", data);
            //
            if ((fd = open(data, O_RDONLY)) < 0) {
                break;
            }
            if (m == 0) {
                close(fd);
            } else {
                debug("copying fd %d", fd);
                (*fds)[c] = fd;
            }
            i += 1;
            c += 1;
        } while (1);

        if (c == 0) {
            debug("no files found");
            return_msg(EAR_ERROR, "no files found");
        }
        if (m == 1) {
            return EAR_SUCCESS;
        }
        //
        debug("Allocating %d fds", c);
        if ((*fds = calloc(c, sizeof(int))) == NULL) {
            return_msg(EAR_ERROR, strerror(errno));
        }
        *fds = (int *) memset(*fds, -1, c * sizeof(int));
        *n   = c;
        //
        i = i_start;
        m = 1;
        c = 0;
    }

    return EAR_SUCCESS;
}

state_t hwmon_close_files(int *fds, uint n)
{
    int i;
    if (fds == NULL || n == 0) {
        return EAR_SUCCESS;
    }
    for (i = 0; i < n; ++i) {
        if (fds[i] != -1) {
            debug("closing fd %d", fds[i]);
            close(fds[i]);
        }
    }
    free(fds);
    return EAR_SUCCESS;
}

state_t hwmon_read_files(int fd, char *buffer, size_t size)
{
    return static_read(fd, buffer, size);
}
