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

#define _GNU_SOURCE
#include <common/output/debug.h>
#include <common/string_enhanced.h>
#include <common/system/poll.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <metrics/common/ipmi_driver.h>
#include <metrics/common/ipmi_driver_frusdr.h>
#include <metrics/common/ipmi_driver_parsing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static ipmi_dev_t devs[IPMI_MAX_DEVS];
static int devs_count;
static int is_opened;
static long msg_counter;
static afd_set_t fds_group;
static int fds_status;

ipmi_addr_t *ipmi_has_addr(ipmi_dev_t *dev, int type, short channel, uchar slave_addr, uchar lun)
{
    struct ipmi_system_interface_addr *addr1;
    struct ipmi_ipmb_addr *addr2;
    int a;

    // Searching if the address is already added
    for (a = 0; a < dev->addrs_count; ++a) {
        addr1 = (struct ipmi_system_interface_addr *) &dev->addrs[a].addr;
        addr2 = (struct ipmi_ipmb_addr *) &dev->addrs[a].addr;

        if (type == IPMI_SYSTEM_INTERFACE_ADDR_TYPE) {
            if (addr1->addr_type == type && addr1->channel == channel && addr1->lun == lun) {
                return (ipmi_addr_t *) addr1;
            }
        } else if (type == IPMI_IPMB_ADDR_TYPE) {
            if (addr2->addr_type == type && addr2->channel == channel && addr2->lun == lun &&
                addr2->slave_addr == slave_addr) {
                return (ipmi_addr_t *) addr2;
            }
        }
    }
    return NULL;
}

ipmi_addr_t *ipmi_add_addr(ipmi_dev_t *dev, int type, short channel, uchar slave_addr, uchar lun)
{
    struct ipmi_system_interface_addr *addr1;
    struct ipmi_ipmb_addr *addr2;
    ipmi_addr_t *addr0;

    // This functions adds an address in a device address list. Its prepared for
    // the future, where there are multiple BMC's and devices attached to
    // private devices.
    if ((addr0 = ipmi_has_addr(dev, type, channel, slave_addr, lun)) != NULL) {
        return addr0;
    }
    // Adding the address
    addr1 = (struct ipmi_system_interface_addr *) &dev->addrs[dev->addrs_count].addr;
    addr2 = (struct ipmi_ipmb_addr *) &dev->addrs[dev->addrs_count].addr;
    memset(addr1, 0, sizeof(ipmi_addr_t));
    addr1->addr_type = type;
    addr1->channel   = channel;
    if (type == IPMI_SYSTEM_INTERFACE_ADDR_TYPE) {
        dev->addrs[dev->addrs_count].addr_len = sizeof(struct ipmi_system_interface_addr);
        addr1->lun                            = lun;
    } else if (type == IPMI_IPMB_ADDR_TYPE) {
        dev->addrs[dev->addrs_count].addr_len = sizeof(struct ipmi_ipmb_addr);
        addr2->slave_addr                     = slave_addr;
        addr2->lun                            = lun;
    }
    dev->addrs_count++;
    return (ipmi_addr_t *) addr1;
}

static state_t ipmi_open_dev_files()
{
    ipmi_addr_t *addr;
    ipmi_cmd_t pkg;
    int i;

    if (is_opened == 1) {
        return EAR_SUCCESS;
    }
    // Initializing devices
    memset(devs, 0, sizeof(ipmi_dev_t) * IPMI_MAX_DEVS);
    devs_count = 0;
    // Initializing aselect() struct
    afd_init(&fds_group);
    // Setting the Get Device Info message
    ipmi_driver_cmd_set_msg(&pkg, IPMI_NETFN_APP_REQUEST, IPMI_GET_DEVICE_ID_CMD, 0);
    // Opening devices through IPMI driver files
    for (i = 0; i < IPMI_MAX_DEVS; ++i) {
        devs[devs_count].fd = -1;
        devs[devs_count].no = devs_count;
        sprintf(devs[devs_count].path, "/dev/ipmi%d", i);
        if ((devs[devs_count].fd = open(devs[devs_count].path, O_RDWR)) < 0) {
            sprintf(devs[devs_count].path, "/dev/ipmi/%d", i);
            if ((devs[devs_count].fd = open(devs[devs_count].path, O_RDWR)) < 0) {
                sprintf(devs[devs_count].path, "/dev/ipmidev%d", i);
                if ((devs[devs_count].fd = open(devs[devs_count].path, O_RDWR)) < 0) {
                }
            }
        } // Ifs
        if (devs[devs_count].fd < 0) {
            debug("FD- Unopened IPMI device file '%s': %s", devs[devs_count].path, strerror(errno));
            if (!devs_count) {
                return 0;
            }
            break;
        }
        // Adding file descriptor to aselect FD group
        AFD_SET(devs[devs_count].fd, &fds_group);
        debug("FD%d Opened IPMI device file '%s'", devs[devs_count].fd, devs[devs_count].path);
        // IPMI Doc: 20.1 Get Device ID Command
        // Adding default address to the device.
        addr = ipmi_add_addr(&devs[i], IPMI_SYSTEM_INTERFACE_ADDR_TYPE, IPMI_BMC_CHANNEL, 0, 0);
        ipmi_driver_cmd_set_paddr(&pkg, addr);
        // Testing IPMI's BMCs by trying BMCs answer
        if (state_fail(ipmi_driver_cmd_send(devs_count, &pkg))) {
            close(devs[devs_count].fd);
            devs[devs_count].fd      = -1;
            devs[devs_count].path[0] = '\0';
            continue;
        }
        ipmi_driver_parse_device_id(pkg.msg_recv_data, &devs[devs_count]);
        ++devs_count;
        // Check the IPMI Doc: 20.9 Broadcast ‘Get Device ID’ to know more
        // about broadcasting Device ID Command through IPMB bus, which we are
        // not doing because the effort to build complex addresses.
    }
    //
    is_opened = 1;
    if (devs_count == 0) {
        return_msg(EAR_ERROR, "No IPMI devices found.");
    }
    return EAR_SUCCESS;
}

state_t ipmi_driver_open()
{
    state_t s;
    if (state_ok(s = ipmi_open_dev_files())) {
        ipmi_driver_sdrs_discover(devs, devs_count);
        ipmi_driver_frus_discover(devs, devs_count);
    }
    return s;
}

void ipmi_driver_close()
{
    // We can close the file descriptors, but we are not interested in traverse
    // the SDR and FRU inventories again.
}

int ipmi_driver_devs_count()
{
    return devs_count;
}

int ipmi_driver_has_hardware(char *string)
{
    char buffer[256] = {0};
    int d, b;

    strncpy(buffer, string, 255);
    strtolow(buffer);
    for (d = 0; d < devs_count; ++d) {
        for (b = 0; b < devs[d].boards_names_count; ++b) {
            if (strstr(devs[d].boards_names[b], buffer) != NULL) {
                return 1;
            }
        }
    }
    return 0;
}

static int is_sensor(char *string, ipmi_dev_t *devs, int d, int r, ipmi_sensor_t **alloc, int *s)
{
    // Comparing two lower case strings
    if (strstr(devs[d].records[r].name_lower, string) != NULL) {
        *alloc                 = realloc(*alloc, (*s + 1) * sizeof(ipmi_sensor_t));
        (*alloc)[*s].dev_no    = d;
        (*alloc)[*s].rec_no    = r;
        (*alloc)[*s].name      = devs[d].records[r].name;
        (*alloc)[*s].available = 1;
        (*alloc)[*s].units     = devs[d].records[r].formula_result_unit;
        (*alloc)[*s].units_str = devs[d].records[r].formula_result_unit_str;
        (*alloc)[*s].instance  = devs[d].records[r].entity_instance;
        debug("%d_%d %s (FOUND! %d)", d, r, (*alloc)[*s].name, *s);
        (*s)++;
        return 1;
    }
    // debug("%d_%d %s (!= %s)", d, r, devs[d].records[r].name_lower, string);
    return 0;
}

int ipmi_driver_sensors_find(char *string, ipmi_sensor_t **sensors, uint *sensors_count)
{
    ipmi_sensor_t *alloc = NULL;
    char string_tolow[64];
    uint list_count = 0;
    char **list     = NULL;
    int d, r, s, m, l;
    char buffer[64];

    if (string == NULL) {
        return 0;
    }
    // String to lower case
    strcpy(string_tolow, string);
    strtolow(string_tolow);
    // String low to list
    if (strtoa((const char *) string_tolow, ',', &list, &list_count) == NULL) {
        return 0;
    }
    for (d = s = 0; d < devs_count; ++d) {
        for (r = 0; r < devs[d].records_count; ++r) {
            // If not FULL sensor type (0x01)
            if (devs[d].records[r].type != 0x01) {
                continue;
            }
            for (l = 0; l < list_count; ++l) {
                // debug("%s == %s ?", devs[d].records[r].name, list[l]);
                // If string contains %d, then it has an index that grows
                if (strstr(list[l], "%d") == NULL) {
                    if (is_sensor(list[l], devs, d, r, &alloc, &s)) {
                        goto next_record;
                    }
                    continue;
                }
                // Doing the %d part (f of fail)
                m = 0;
                while (m < 16 && sprintf(buffer, list[l], m)) {
                    if (is_sensor(buffer, devs, d, r, &alloc, &s)) {
                        goto next_record;
                    }
                    m += 1;
                }
            }
        next_record:;
        }
    }
    strtoa_free(list);
    if (sensors != NULL)
        *sensors = alloc;
    if (sensors_count != NULL)
        *sensors_count = s;
    return s;
}

state_t ipmi_driver_sensors_read(ipmi_sensor_t *sensors, uint sensors_count, ipmi_reading_t **values)
{
    ipmi_addr_t *addr = NULL;
    sdr_t *rec        = NULL;
    ipmi_cmd_t pkg;
    state_t st;
    int s;

    if (*values == NULL) {
        *values = calloc(sensors_count, sizeof(ipmi_reading_t));
    }
    // IPMI Doc: 35.14 Get Sensor Reading Command, and Table G-1, Command Number
    // Assignments and Privilege Levels.
    ipmi_driver_cmd_set_msg(&pkg, IPMI_NETFN_SENSOR_EVENT_REQUEST, 0x2d, 1);
    //
    for (s = 0; s < sensors_count; ++s) {
        rec = &devs[sensors[s].dev_no].records[sensors[s].rec_no];
        if (addr != rec->addr) {
            ipmi_driver_cmd_set_paddr(&pkg, rec->addr);
            addr = rec->addr;
        }
        // Setting the sensor to read
        pkg.msg_send_data[0] = rec->sensor_no;
        // Sending message
        if (state_ok(st = ipmi_driver_cmd_send(sensors[s].dev_no, &pkg))) {
            // Getting the sensor formula variables
            ipmi_driver_parse_sensor_reading(pkg.msg_recv_data[1], rec, &(*values)[s]);
            debug("val[%02d]: %lf", s, (*values)[s].value);
        } else {
            debug("ipmi_driver_cmd_send failed: %s", state_msg);
        }
    }
    return EAR_SUCCESS;
}

// OEM
void ipmi_driver_cmd_set_paddr(ipmi_cmd_t *pkg, ipmi_addr_t *addr)
{
    memset(&pkg->addr, 0, sizeof(ipmi_addr_t));
    pkg->addr.addr.addr_type = addr->addr.addr_type;
    pkg->addr.addr.channel   = addr->addr.channel;
    if (addr->addr.addr_type == IPMI_SYSTEM_INTERFACE_ADDR_TYPE) {
        pkg->addr.addr_len                     = sizeof(struct ipmi_system_interface_addr);
        struct ipmi_system_interface_addr *dst = (struct ipmi_system_interface_addr *) &pkg->addr.addr;
        struct ipmi_system_interface_addr *src = (struct ipmi_system_interface_addr *) &addr->addr;
        dst->lun                               = src->lun;
    } else if (addr->addr.addr_type == IPMI_IPMB_ADDR_TYPE) {
        pkg->addr.addr_len         = sizeof(struct ipmi_ipmb_addr);
        struct ipmi_ipmb_addr *dst = (struct ipmi_ipmb_addr *) &pkg->addr.addr;
        struct ipmi_ipmb_addr *src = (struct ipmi_ipmb_addr *) &addr->addr;
        dst->slave_addr            = src->slave_addr;
        dst->lun                   = src->lun;
    }
}

void ipmi_driver_cmd_set_addr(ipmi_cmd_t *pkg, int type, short channel, uchar slave_addr, uchar lun)
{
    memset(&pkg->addr, 0, sizeof(ipmi_addr_t));
    pkg->addr.addr.addr_type = type;
    pkg->addr.addr.channel   = channel;
    if (type == IPMI_SYSTEM_INTERFACE_ADDR_TYPE) {
        pkg->addr.addr_len                     = sizeof(struct ipmi_system_interface_addr);
        struct ipmi_system_interface_addr *dst = (struct ipmi_system_interface_addr *) &pkg->addr.addr;
        dst->lun                               = lun;
    } else if (type == IPMI_IPMB_ADDR_TYPE) {
        pkg->addr.addr_len         = sizeof(struct ipmi_ipmb_addr);
        struct ipmi_ipmb_addr *dst = (struct ipmi_ipmb_addr *) &pkg->addr.addr;
        dst->slave_addr            = slave_addr;
        dst->lun                   = lun;
    }
}

void ipmi_driver_cmd_set_msg(ipmi_cmd_t *pkg, uchar netfn, uchar cmd, ushort data_len)
{
    ipmi_driver_cmd_null_msg_data(pkg);
    pkg->msg_send.netfn    = netfn;
    pkg->msg_send.cmd      = cmd;
    pkg->msg_send.data     = (data_len > 0) ? pkg->msg_send_data : NULL;
    pkg->msg_send.data_len = data_len;
}

void ipmi_driver_cmd_null_msg_data(ipmi_cmd_t *pkg)
{
    memset(pkg->msg_send_data, 0, sizeof(pkg->msg_send_data));
    memset(pkg->msg_recv_data, 0, sizeof(pkg->msg_recv_data));
}

#if SHOW_DEBUGS
static void debug_raw(struct ipmi_msg *msg)
{
    char buffer_debug[256 * 3];
    int i, c;

    c = sprintf(buffer_debug, "    ");
    if (msg->data_len == 0) {
        c += sprintf(&buffer_debug[c], "%02x ", 0);
    }
    for (i = 0; i < msg->data_len; i++) {
        if (i > 0 && (i % 32) == 0)
            c += sprintf(&buffer_debug[c], "\n    ");
        c += sprintf(&buffer_debug[c], "%02x ", (int) msg->data[i]);
    }
    debug("%s", buffer_debug);
}
#endif

state_t ipmi_driver_cmd_send(int dev_no, ipmi_cmd_t *pkg)
{
    static struct ipmi_recv recv;
    static struct ipmi_req reqs;

    memset(&recv, 0, sizeof(recv));
    memset(&reqs, 0, sizeof(reqs));
    // Addresses:
    // - IPMI_SYSTEM_INTERFACE_ADDR_TYPE: local BMC address. The driver uses
    //   KCS, BT or SMIC interfaces by PCI or motherboard's SMBus.
    // - IPMI_IPMB_ADDR_TYPE: to communicate to a device not directly connected
    //   to the main BMC, but connected by an i2c IMPB bus to the main BMC like
    //   a satellite controller (other BMC). In IPMB connections, multiple
    //   devices can be connected to the single bus, each of them with its own
    //   address.
    // Channels:
    //   - Are interfaces to group sets of devices (0-15, 4 bits).
    //   - There is a command to retrieve information of a channel.
    //   - Channels are predefined: 0 to communicate with KCS, BT or SMIC
    //      devices, 1 IPMB, 2-3 LAN and 4-15 hardware custom.
    // LUN (logical unit number):
    //   - Is used to identify logic subcomponents within a physical device.
    //     That allows multi-function devices to ask different messages
    //     depending on LUN.
    //   - Is a number of 2 bits, allowing 4 subcomponents (0-3).
    //   - In general LUN 0 is the default for most devices, and 1-3 are
    //     specialized.
    // NetFn:
    //   - Is a number to categorize commands (0-63, 6 bits)
    //   - 0 least significant bit means request, 1 is response.
    //   - Pre-definitions:
    //       - 0x00: chassis commands like turn on/off, reset...
    //       - 0x02: bridge between buses.
    //       - 0x04: sensors and events, like get sensor reading...
    //       - 0x06: general BMC (aka APP).
    //       - 0x08: firmware.
    //       - 0x0a: storage like FRU and SDR.
    //       - 0x0c: transport like LAN.
    //       - 0x30-03F: OEM (Original Equipment Manufacturer)
    //     We are particularly interested in 0x04, 0x06 and 0x0a.
    reqs.addr         = (unsigned char *) &pkg->addr.addr;
    reqs.addr_len     = pkg->addr.addr_len;
    reqs.msgid        = msg_counter++;
    reqs.msg.netfn    = pkg->msg_send.netfn;
    reqs.msg.cmd      = pkg->msg_send.cmd;
    reqs.msg.data     = pkg->msg_send.data;
    reqs.msg.data_len = pkg->msg_send.data_len;
#if 0
    debug("FD%d Sending %u bytes from IPMI [ADDR TYPE 0x%X (0x%X), CHAN 0x%X, NETFN 0x%X, CMD 0x%x]:", devs[dev_no].fd,
          reqs.msg.data_len, pkg->addr.addr.addr_type, pkg->addr.addr.data[0], pkg->addr.addr.channel, reqs.msg.netfn, reqs.msg.cmd);
    //debug_raw(&reqs.msg);
#endif
    if (ioctl(devs[dev_no].fd, IPMICTL_SEND_COMMAND, &reqs) < 0) {
        debug("IPMI command ioctl error: %s", strerror(errno));
        return_msg(EAR_ERROR, strerror(errno));
    }
    recv.addr         = (unsigned char *) &pkg->addr.addr;
    recv.addr_len     = pkg->addr.addr_len;
    recv.msg.data     = pkg->msg_recv_data;
    recv.msg.data_len = sizeof(pkg->msg_recv_data);

    // You can find a complete list of completion codes in the official
    // documentation: Table 5-2, Completion Codes
    if ((fds_status = aselect(&fds_group, 300, NULL)) <= 0) {
        if (fds_status < 0) {
            debug("IPMI waiting for message error: %s", strerror(errno));
            return_print(EAR_ERROR, "Error when receiving IPMI message: %s", strerror(errno));
        } else if (fds_status == 0) {
            debug("IPMI receive message ioctl error: timeout");
            return_msg(EAR_ERROR, "Timeout");
        }
    }
    if (ioctl(devs[dev_no].fd, IPMICTL_RECEIVE_MSG_TRUNC, &recv) < 0) {
        debug("IPMI receive message ioctl error: %s", strerror(errno));
        return_print(EAR_ERROR, "Error when receiving IPMI message: %s", strerror(errno));
    }
    return EAR_SUCCESS;
}
