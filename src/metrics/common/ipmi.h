/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_IPMI_H
#define METRICS_COMMON_IPMI_H

#include <common/states.h>
#include <linux/ipmi.h>

//  _____ _____  __  __ _____
// |_   _|  __ \|  \/  |_   _|
//   | | | |__) | \  / | | |
//   | | |  ___/| |\/| | | |
//  _| |_| |    | |  | |_| |_
// |_____|_|    |_|  |_|_____|
//
// What we don't support by now?
//   - SDRs and FRUs in IMPB addresses (other than INTERFACE).
//   - Full sensors with linearization functions other than 0x00.
//   - Compact sensors.
// These missing features can be added as we require it.

#define IPMI_MAX_DEVS 4 // Max supported devices

typedef struct ipmi_addr_s {
    struct ipmi_addr addr;
    uint addr_len;
} ipmi_addr_t;

// This structure includes all the data required to send a message. It is also
// named as Command Package.
typedef struct ipmi_cmd_s {
    ipmi_addr_t addr;
    struct ipmi_msg msg_send;
    uchar msg_send_data[64];
    uchar msg_recv_data[256];
    uint msg_recv_data_len;
} ipmi_cmd_t;

typedef struct ipmi_reading_s {
    double value;
    uchar raw[5];
} ipmi_reading_t;

typedef struct ipmi_sensor_s {
    uchar dev_no; // device (/dev/ipmi#)
    uchar rec_no; // record
    const char *name;
    uchar available;
    uchar units;
    uchar rate;
    const char *units_str;
    uchar instance;
} ipmi_sensor_t;

state_t ipmi_open();

void ipmi_close();

// Returns the IPMI devices opened (normally /dev/impiN).
int ipmi_devs_count();

// Checks if IPMI has a board attached to its system.
int ipmi_has_hardware(char *board_name);

// The 'sensor_name' can be a comma separated list of sensor names and also can
// include %d to get numbered sensors.
int ipmi_sensors_find(char *sensor_name, ipmi_sensor_t **sensors, uint *sensors_count);

// Allocates values memory if the inner pointer is NULL.
state_t ipmi_sensors_read(ipmi_sensor_t *sensors, uint sensors_count, ipmi_reading_t **values);

// RAW/OEM:
// These functions are intended to contact to IPMI devices through raw messages.
// Before the main function ipmi_cmd_send(), you have to fill the request struct
// with address through ipmi_cmd_set_paddr() OR ipmi_cmd_set_addr(), and the
// message content (net function and command) by calling ipmi_cmd_set_msg().

// Sets an address in the request pack (req) based on an existing address.
void ipmi_cmd_set_paddr(ipmi_cmd_t *req, ipmi_addr_t *addr);

// Sets an address in the request (req) based on separated components.
void ipmi_cmd_set_addr(ipmi_cmd_t *req, int type, short channel, uchar slave_addr, uchar lun);

// Sets the message part in the request (req) based on separated components.
void ipmi_cmd_set_msg(ipmi_cmd_t *req, uchar netfn, uchar cmd, ushort data_len);

// Sends the request (req) to the device specified by its number.
state_t ipmi_cmd_send(int dev_no, ipmi_cmd_t *req);

#endif // METRICS_COMMON_IPMI_H
