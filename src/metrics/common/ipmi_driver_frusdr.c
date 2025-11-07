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
#include <metrics/common/ipmi_driver.h>
#include <metrics/common/ipmi_driver_frusdr.h>
#include <metrics/common/ipmi_driver_parsing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uchar fru_buffer[1048576];

static void ipmi_driver_sdrs_traversal(ipmi_cmd_t *pkg, ipmi_dev_t *dev, ipmi_addr_t *addr, ushort next_record_id)
{
    ipmi_addr_t *new_addr = NULL;
    sdr_t *rec            = NULL;

    if (next_record_id == 0xFFFF) {
        return;
    }
    // Preparing the package message
    pkg->msg_send.data[2] = (next_record_id) & 0xFF;
    pkg->msg_send.data[3] = (next_record_id >> 8) & 0xFF;
    pkg->msg_send.data[4] = 0x00; // Offset
    pkg->msg_send.data[5] = 0xFF; // Length
    // And the package address
    ipmi_driver_cmd_set_paddr(pkg, addr);

    if (state_fail(ipmi_driver_cmd_send(dev->no, pkg))) {
        return;
    }
    // Response:
    //    0: Completion Code
    //    1: Record ID for next record, LS Byte
    //    2: Record ID for next record, MS Byte
    //    3: Record Data (the 5 first bytes are the header):
    //          0-1: Record ID
    //          2  : SDR Version (0x51 is IPMI 2.0)
    //          3  : Record Type (0x1 full, 0x2 compact, 0x8 FRU, 0x11 other BMC...)
    //          4  : Record Length
    //          5-x: Device information.
    if (pkg->msg_recv_data[0] != 0x00) {
        return;
    }
    // Allocating new space if required (in blocks of 64 records)
    if ((dev->records_count % 64) == 0) {
        dev->records = realloc(dev->records, (dev->records_count + 64) * sizeof(sdr_t));
        memset(&dev->records[dev->records_count], 0, 64 * sizeof(sdr_t));
    }
    // Parsing, in case is BMC-Locator, get the address
    rec = &dev->records[dev->records_count];

    switch (ipmi_driver_parse_sdr(pkg->msg_recv_data, rec, addr)) {
        case 0x11: // New FRU detected
            // Nothing by now because we can't build complex IPMB addresses.
            break;
        case 0x12: // New BMC detected
            // If no SDR or it's the main BMC (chan 0x00), no contact required.
            if (!rec->bmc_sdr || rec->new_addr_channel == 0x00) {
                break;
            }
            if (!ipmi_has_addr(dev, IPMI_IPMB_ADDR_TYPE, rec->new_addr_channel, rec->new_addr_slave,
                               rec->new_addr_lun)) {
                new_addr = ipmi_add_addr(dev, IPMI_IPMB_ADDR_TYPE, rec->new_addr_channel, rec->new_addr_slave,
                                         rec->new_addr_lun);
            }
            break;
    }
    dev->records_count++;
    // Getting next ID
    next_record_id = pkg->msg_recv_data[1] | (pkg->msg_recv_data[2] << 8);
    // Following the traversal
    ipmi_driver_sdrs_traversal(pkg, dev, addr, next_record_id);
    // Following the traversal on new address
    if (new_addr != NULL) {
        ipmi_driver_sdrs_traversal(pkg, dev, new_addr, 0x0000);
    }
}

void ipmi_driver_sdrs_discover(ipmi_dev_t *devs, uint devs_count)
{
    ipmi_cmd_t pkg;
    int d;
    // The SDR reservation allows the SDR doesn't change while reading
    // registers. But we are going to assume it won't happen.
    ipmi_driver_cmd_set_msg(&pkg, IPMI_NETFN_STORAGE_REQUEST, 0x23, 6);

    for (d = 0; d < devs_count; ++d) {
        // Discover the first device in the default address
        ipmi_driver_sdrs_traversal(&pkg, &devs[d], &devs[d].addrs[0], 0x0000);
    }
}

__attribute__((unused)) static void debug_fru_bytes(char *title, uchar *data, uint length)
{
    char buffer[1024];
    int l, c;
    c = sprintf(buffer, "%s ", title);
    for (l = 0; l < length; ++l) {
        c += sprintf(&buffer[c], "%02x ", data[l]);
    }
    debug("%s", buffer);
}

static int ipmi_driver_frus_read_offset(ipmi_dev_t *dev, ipmi_cmd_t *pkg, uchar f, uint offset)
{
    uint org_offset = offset;
    uint max_length = 256;
    uint fru_length = 0;
    uint length     = 0;
    uint finish     = 0;
    uint marker     = 0;

    // Reading length of the string
    ipmi_driver_cmd_set_msg(pkg, IPMI_NETFN_STORAGE_REQUEST, 0x11, 4);
    pkg->msg_send.data[0] = f;
    pkg->msg_send.data[1] = offset & 0xFF;
    pkg->msg_send.data[2] = offset >> 8;
    pkg->msg_send.data[3] = 2;

    if (state_fail(ipmi_driver_cmd_send(dev->no, pkg))) {
        return 0;
    }
    if (pkg->msg_recv_data[0] != 0x00) {
        return 0;
    }
    // DATA [0:0] Completion Code
    // DATA [1:1] Bytes returned
    // DATA [2:2] FRU version
    // DATA [3:3] FRU length
    fru_length = pkg->msg_recv_data[3] * 8;
    if (fru_length == 0) {
        debug("FRU%d error: length is 0", f);
        return 0;
    }
calc_length:
    offset = org_offset;
    length = fru_length;
    finish = offset + length;

    if (length > max_length) {
        // According to IPMI documentation:
        //   - The IPMB standard overall message length for ‘non-bridging’ messages
        //     is specified as 32 bytes, maximum, including slave address.
        // Take this into the account in the future.
        length = max_length;
    }
    do {
        // Reading length of the string
        ipmi_driver_cmd_set_msg(pkg, IPMI_NETFN_STORAGE_REQUEST, 0x11, 4);
        pkg->msg_send.data[0] = f;
        pkg->msg_send.data[1] = offset & 0xFF;
        pkg->msg_send.data[2] = offset >> 8;
        pkg->msg_send.data[3] = length;
        if (state_fail(ipmi_driver_cmd_send(dev->no, pkg))) {
            debug("FRU%d (len %u) reading command failed: %s", f, length, state_msg);
            return 0;
        }
        if (pkg->msg_recv_data[0] != 0x00) {
            debug("FRU%d (len %u) reading state failed: 0x%d", f, length, pkg->msg_recv_data[0]);
            // This meands that the requested data is too big so we have to reduce the requested size.
            if (pkg->msg_recv_data[0] == 0xc7 || pkg->msg_recv_data[0] == 0xc8 || pkg->msg_recv_data[0] == 0xca) {
                if (max_length > 32) {
                    max_length = 32;
                    goto calc_length;
                }
            }
            return 0;
        }
        // Copying TODO: failing, marker is overflowing
        memcpy(&fru_buffer[marker], &pkg->msg_recv_data[2], pkg->msg_recv_data[1]);
        // debug_fru_bytes("FRU BUFFER    DATA ", &fru_buffer[00], 32);
        // debug_fru_bytes("FRU BUFFER    DATA ", &fru_buffer[32], 32);
        // debug_fru_bytes("FRU BUFFER    DATA ", &fru_buffer[64], 32);
        //  Recalculating the batch pointers
        // debug("offset/marker/length: %u/%u/%u (before)", offset, marker, length);
        offset = offset + pkg->msg_recv_data[1];
        marker = marker + pkg->msg_recv_data[1];
        length = ((finish - offset) < max_length) ? finish - offset : max_length;
        // debug("offset/marker/length: %u/%u/%u (after)", offset, marker, length);
    } while (offset < finish);
    return 1;
}

static int ipmi_driver_frus_read_board(ipmi_dev_t *dev, ipmi_cmd_t *pkg, uchar f, uint offset)
{
    if (!ipmi_driver_frus_read_offset(dev, pkg, f, offset)) {
        return 0;
    }
    ipmi_driver_parse_fru_board(fru_buffer, dev);
    return 1;
}

static int ipmi_driver_frus_read_product(ipmi_dev_t *dev, ipmi_cmd_t *pkg, uchar f, uint offset)
{
    if (!ipmi_driver_frus_read_offset(dev, pkg, f, offset)) {
        return 0;
    }
    ipmi_driver_parse_fru_product(fru_buffer, dev);
    return 1;
}

static int ipmi_driver_frus_read_chassis(ipmi_dev_t *dev, ipmi_cmd_t *pkg, uchar f, uint offset)
{
    if (!ipmi_driver_frus_read_offset(dev, pkg, f, offset)) {
        return 0;
    }
    ipmi_driver_parse_fru_chassis(fru_buffer, dev);
    return 1;
}

void ipmi_driver_frus_discover(ipmi_dev_t *devs, uint devs_count)
{
    struct fru_header {
        uchar version;

        union {
            struct {
                uchar internal;
                uchar chassis;
                uchar board;
                uchar product;
                uchar multi;
            } offset;

            uchar offsets[5];
        };

        uchar pad;
        uchar checksum;
    } header;

    ipmi_cmd_t pkg;
    uint holes = 0;
    uchar f;
    int d;

    // By now we are just reading the FRU in the INTERFACE address (or main address).
    // But in the future maybe we have to process the SDR types 10h, 11h and 12h and
    // find these devices in other addresses that includes FRU information. You can
    // see how ipmitool does it in function ipmi_fru_print().
    for (d = 0; d < devs_count; ++d) {
        ipmi_driver_cmd_set_paddr(&pkg, &devs[d].addrs[0]);
        for (f = 0x00; f < 0xff; ++f) {
            ipmi_driver_cmd_set_msg(&pkg, IPMI_NETFN_STORAGE_REQUEST, 0x10, 1);
            pkg.msg_send.data[0] = f;
            // Sending the message
            if (state_fail(ipmi_driver_cmd_send(devs[d].no, &pkg))) {
                break;
            }
            // If the ID fails, we will try 10 additional times
            if (pkg.msg_recv_data[0] != 0x00) {
                if (holes == 3)
                    break;
                else
                    continue;
            }
            holes = 0;
            // Parsing the FRU inventory if it is correct
            if (!ipmi_driver_parse_fru_inventory(pkg.msg_recv_data)) {
                continue;
            }
            // IPMI Doc: 34.2 Read FRU Data Command
            ipmi_driver_cmd_set_msg(&pkg, IPMI_NETFN_STORAGE_REQUEST, 0x11, 4);
            pkg.msg_send.data[0] = f;    // FRU id
            pkg.msg_send.data[1] = 0x00; // LS offset
            pkg.msg_send.data[2] = 0x00; // MS offset
            pkg.msg_send.data[3] = 0x08; // Getting 8 bytes
            // Sending the message
            if (state_fail(ipmi_driver_cmd_send(devs[d].no, &pkg))) {
                break;
            }
            if (pkg.msg_recv_data[0] != 0x00) {
                break;
            }
            memcpy(&header, pkg.msg_recv_data + 2, 8);
#if 0
            debug("fru.header.version:         0x%x", header.version);
            debug("fru.header.offset.internal: 0x%x", header.offset.internal * 8);
            debug("fru.header.offset.chassis:  0x%x", header.offset.chassis  * 8);
            debug("fru.header.offset.board:    0x%x", header.offset.board    * 8);
            debug("fru.header.offset.product:  0x%x", header.offset.product  * 8);
            debug("fru.header.offset.multi:    0x%x", header.offset.multi    * 8);
#endif
            // We found that if a FRU reading fails for a type offset, it will also
            // fail for the other types, so we continue to save execution time.
            if (header.offset.chassis != 0) {
                if (!ipmi_driver_frus_read_chassis(&devs[d], &pkg, f, ((uint) header.offset.chassis) * 8)) {
                    continue;
                }
            }
            if (header.offset.board != 0) {
                if (!ipmi_driver_frus_read_board(&devs[d], &pkg, f, ((uint) header.offset.board) * 8)) {
                    continue;
                }
            }
            if (header.offset.product != 0) {
                if (!ipmi_driver_frus_read_product(&devs[d], &pkg, f, ((uint) header.offset.product) * 8)) {
                    continue;
                }
            }
        }
    }
}
