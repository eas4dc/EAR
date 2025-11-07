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

#include <common/hardware/bithack.h>
#include <common/output/debug.h>
#include <common/string_enhanced.h>
#include <ctype.h>
#include <math.h>
#include <metrics/common/ipmi_driver_parsing.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *unit_str[] = {"unspecified",
                                 "degrees C",
                                 "degrees F",
                                 "degrees K",
                                 "Volts",
                                 "Amps",
                                 "Watts",
                                 "Joules",
                                 "Coulombs",
                                 "VA",
                                 "Nits",
                                 "lumen",
                                 "lux",
                                 "Candela",
                                 "kPa",
                                 "PSI",
                                 "Newton",
                                 "CFM",
                                 "RPM",
                                 "Hz",
                                 "microsecond",
                                 "millisecond",
                                 "second",
                                 "minute",
                                 "hour",
                                 "day",
                                 "week",
                                 "mil",
                                 "inches",
                                 "feet",
                                 "cu in",
                                 "cu feet",
                                 "mm",
                                 "cm",
                                 "m",
                                 "cu cm",
                                 "cu m",
                                 "liters",
                                 "fluid ounce",
                                 "radians",
                                 "steradians",
                                 "revolutions",
                                 "cycles",
                                 "gravities",
                                 "ounce",
                                 "pound",
                                 "ft-lb",
                                 "oz-in",
                                 "gauss",
                                 "gilberts",
                                 "henry",
                                 "millihenry",
                                 "farad",
                                 "microfarad",
                                 "ohms",
                                 "siemens",
                                 "mole",
                                 "becquerel",
                                 "PPM",
                                 "reserved",
                                 "Decibels",
                                 "DbA",
                                 "DbC",
                                 "gray",
                                 "sievert",
                                 "color temp deg K",
                                 "bit",
                                 "kilobit",
                                 "megabit",
                                 "gigabit",
                                 "byte",
                                 "kilobyte",
                                 "megabyte",
                                 "gigabyte",
                                 "word",
                                 "dword",
                                 "qword",
                                 "line",
                                 "hit",
                                 "miss",
                                 "retry",
                                 "reset",
                                 "overflow",
                                 "underrun",
                                 "collision",
                                 "packets",
                                 "messages",
                                 "characters",
                                 "error",
                                 "correctable error",
                                 "uncorrectable error",
                                 "fatal error",
                                 "grams",
                                 NULL};
const char *sensor_type_str[] = {

    "reserved",
    "Temperature",
    "Voltage",
    "Current",
    "Fan",
    "Physical Security",
    "Platform Security",
    "Processor",
    "Power Supply",
    "Power Unit",
    "Cooling Device",
    "Other",
    "Memory",
    "Drive Slot / Bay",
    "POST Memory Resize",
    "System Firmwares",
    "Event Logging Disabled",
    "Watchdog1",
    "System Event",
    "Critical Interrupt",
    "Button",
    "Module / Board",
    "Microcontroller",
    "Add-in Card",
    "Chassis",
    "Chip Set",
    "Other FRU",
    "Cable / Interconnect",
    "Terminator",
    "System Boot Initiated",
    "Boot Error",
    "OS Boot",
    "OS Critical Stop",
    "Slot / Connector",
    "System ACPI Power State",
    "Watchdog2",
    "Platform Alert",
    "Entity Presence",
    "Monitor ASIC",
    "LAN",
    "Management Subsys Health",
    "Battery",
    "Session Audit",
    "Version Change",
    "FRU State",
    NULL};

void ipmi_driver_parse_device_id(uchar *data, ipmi_dev_t *dev)
{
    // IPMI Doc: 20.1 Get Device ID Command, getting the BMC data
    dev->id      = data[1];
    dev->has_sdr = getbits32((uint) data[2], 7, 7);
    dev->has_fru = getbits32((uint) data[6], 3, 3);
    debug("FD%d Detected BMC [ ID0x%X %s%s]", dev->fd, dev->id, (dev->has_sdr) ? "SDR " : "",
          (dev->has_fru) ? "FRU " : "");
}

uchar ipmi_driver_parse_sdr(uchar *data, sdr_t *rec, ipmi_addr_t *addr)
{
#define B(n) 3 + n - 1

    // IPMI Doc: 43. Sensor Data Record Formats
    switch ((rec->type = data[B(4)])) {
        case 0x01: // FULL
            // IPMI Doc: 43.1 SDR Type 01h, Full Sensor Record
            rec->entity_instance         = getbits8(data[B(10)], 6, 0);
            rec->entity_logical          = getbits8(data[B(10)], 7, 7);
            rec->sensor_no               = data[B(8)];
            rec->sensor_type             = data[B(13)];
            rec->sensor_type_str         = (char *) sensor_type_str[rec->sensor_type];
            rec->formula_complement      = getbits8(data[B(21)], 7, 6);
            rec->formula_rate_unit       = getbits8(data[B(21)], 5, 3);
            rec->formula_modifier        = getbits8(data[B(21)], 2, 1);
            rec->formula_percentage      = getbits8(data[B(21)], 0, 0);
            rec->formula_result_unit     = data[B(22)];
            rec->formula_result_unit_str = (char *) unit_str[rec->formula_result_unit];
            rec->formula_modifier_unit   = data[B(23)];
            rec->formula_lin_function    = getbits8(data[B(24)], 6, 0);
            rec->formula_m               = getbits8(data[B(26)], 7, 6) << 8 | data[B(25)];
            rec->formula_b               = getbits8(data[B(28)], 7, 6) << 8 | data[B(27)];
            rec->formula_m =
                getbits32(rec->formula_m, 9, 9) ? 0xfffffc00 | rec->formula_m : rec->formula_m; // Converting to ca2
            rec->formula_b =
                getbits32(rec->formula_b, 9, 9) ? 0xfffffc00 | rec->formula_b : rec->formula_b; // Converting to ca2
            rec->formula_r_exp = getbits8(data[B(30)], 7, 4);
            rec->formula_b_exp = getbits8(data[B(30)], 3, 0);
            rec->formula_r_exp = getbits32(rec->formula_r_exp, 3, 3) ? 0xfffffff0 | rec->formula_r_exp
                                                                     : rec->formula_r_exp; // Converting to ca2
            rec->formula_b_exp = getbits32(rec->formula_b_exp, 3, 3) ? 0xfffffff0 | rec->formula_b_exp
                                                                     : rec->formula_b_exp; // Converting to ca2
            strncpy(rec->name, (const char *) &data[B(49)], getbits8(data[B(48)], 4, 0));
            strncpy(rec->name_lower, (const char *) &data[B(49)], getbits8(data[B(48)], 4, 0));
            strtolow(rec->name_lower);
            debug("FULL    SEN#%03d INS#%02d [%s %s] "
                  "(%uca2 %ur %um %upcnt) (%uru %umu) %ulin (%dm %db) (%dre %dbe)",
                  rec->sensor_no, rec->entity_instance, rec->name, sensor_type_str[rec->sensor_type],
                  rec->formula_complement, rec->formula_rate_unit, rec->formula_modifier, rec->formula_percentage,
                  rec->formula_result_unit, rec->formula_modifier_unit, rec->formula_lin_function, rec->formula_m,
                  rec->formula_b, rec->formula_r_exp, rec->formula_b_exp);
            break;
        case 0x02: // COMPACT
            rec->entity_instance       = getbits8(data[B(10)], 6, 0);
            rec->entity_logical        = getbits8(data[B(10)], 7, 7);
            rec->sensor_no             = data[B(8)];
            rec->sensor_type           = data[B(13)];
            rec->formula_rate_unit     = getbits8(data[B(21)], 5, 3);
            rec->formula_modifier      = getbits8(data[B(21)], 2, 1);
            rec->formula_percentage    = getbits8(data[B(21)], 0, 0);
            rec->formula_result_unit   = data[B(22)];
            rec->formula_modifier_unit = data[B(23)];
            strncpy(rec->name, (const char *) &data[B(33)], getbits8(data[B(32)], 4, 0));
            strncpy(rec->name_lower, (const char *) &data[B(33)], getbits8(data[B(32)], 4, 0));
            strtolow(rec->name_lower);
            debug("COMPACT SEN#%03d INS#%02d [%s %s] "
                  "(%ur %um %upcnt) (%uru %umu)",
                  rec->sensor_no, rec->entity_instance, rec->name, sensor_type_str[rec->sensor_type],
                  rec->formula_rate_unit, rec->formula_modifier, rec->formula_percentage, rec->formula_result_unit,
                  rec->formula_modifier_unit);
            break;
        case 0x03: // EVENT-BASED
            debug("EVENT-BASED");
            break;
        case 0x08: // ENTITY-ASSOC
            debug("ENTITY-ASSOC");
            break;
        case 0x09: // DEV-RELATIVE
            debug("DEV-RELATIVE");
            break;
        case 0x10: // DEV-LOCATOR
            // This record is used to store the location and type information
            // for devices on the IPMB or management controller private busses
            // that are neither IPMI FRU devices nor IPMI management
            // controllers.
            debug("DEV-LOCATOR");
            break;
        case 0x11: // FRU-LOCATOR
            // This record is used for locating FRU information that is on the
            // IPMB, on a Private Bus behind or management controller, or
            // accessed via FRU commands to a management controller. This
            // excludes any FRU Information that is accessed via FRU commands
            // at LUN 00b of a management controller. The presence or absence
            // of that FRU Information is indicated using the Management Device
            // Locator record (0x12).
            //
            // The generation of an IPMB address (IPMI_IPMB_ADDR_TYPE) requires
            // more knowledge about IMPI and is beyond the scope of this source
            // code. By now que use IPMI_SYSTEM_INTERFACE_ADDR_TYPE type
            // addresses that are managed by the BMC by its KCS, SMIC, etc
            // interfaces and saves us from writing channels, slave addresses,
            // private bus addresses, etc. Maybe one day we will be forced to
            // increase the complexity of this code and build IMPB addresses.
            rec->new_addr_type    = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
            rec->new_addr_channel = IPMI_BMC_CHANNEL;
            rec->entity_logical   = getbits8(data[B(8)], 7, 7);
            rec->entity_instance  = (rec->entity_logical) ? getbits8(data[B(7)], 7, 0) : 0;
            rec->length           = getbits8(data[B(16)], 4, 0);
            if (rec->length != 0x00 && rec->length != 0x1F) {
                strncpy(rec->name, (const char *) &data[B(17)], rec->length);
            } else {
                sprintf(rec->name, "no-name");
            }
            debug("FRU-LOCATOR ADDR_0x%X CHAN_0x%x LUN_0x%X BUS_0x%X ID_0x%X %s %s", getbits8(data[B(6)], 7, 1),
                  getbits8(data[B(9)], 7, 4), getbits8(data[B(8)], 4, 3), getbits8(data[B(8)], 2, 0),
                  rec->entity_instance, rec->name, (rec->entity_logical) ? "LOGICAL" : "PHYSICAL");
            break;
        case 0x12: // BMC-LOCATOR
            rec->new_addr_slave   = getbits8(data[B(6)], 7, 1);
            rec->new_addr_channel = getbits8(data[B(7)], 3, 0);
            rec->bmc_sdr          = getbits8(data[B(9)], 1, 1);
            rec->bmc_fru          = getbits8(data[B(9)], 3, 3);
            strncpy(rec->name, (const char *) &data[B(17)], getbits8(data[B(16)], 4, 0));
            debug("BMC-LOCATOR ADDR_0x%X CHAN_0x%x %s %s%s", rec->new_addr_slave, rec->new_addr_channel, rec->name,
                  (rec->bmc_sdr) ? "SDR " : "", (rec->bmc_fru) ? "FRU" : "");
            break;
        case 0x13: // MCC-RECORD
            debug("MCC-RECORD");
            break;
        case 0x14: // MSG-CHANNEL
            debug("MSG-CHANNEL");
            break;
        default:
            if (rec->type >= 0x0a && rec->type <= 0x0f) {
            } // RESERVED
            if (rec->type >= 0xc0) {
            } // Original Equipment Manufacturer (OEM)
    }
    rec->addr = addr;
    return rec->type;
}

int ipmi_driver_parse_fru_inventory(uchar *data)
{
    // Check the function __ipmi_fru_print() of ipmitool source code for more
    // information regarding FRU and its processing.
    int words = getbits8(data[3], 0, 0);
#if SHOW_DEBUGS && 0
    ushort inventory_size = (data[2] << 8) | data[1];
    debug("Detected FRU inventory of size %u accesed by %s", inventory_size, (!words) ? "BYTES" : "WORDS");
#endif
    if (words) {
        // TODO: support it
        debug("Readig FRU word by word is not supported.");
        return 0;
    }
    return 1;
}

static void ipmi_driver_parse_fru_string(uchar *data, uint *index, ipmi_dev_t *dev, int add_it, char *str_type)
{
    int i = 1;
    // Check the function get_fru_area_str() of ipmitool's source code.
    uchar typecode  = (data[*index] & 0xC0) >> 6;
    uchar length    = (data[*index] & 0x3f);
    uchar not_null  = strncmp((const char *) &data[(*index) + 1], "NULL", length) != 0;
    uchar not_empty = strnlen((const char *) &data[(*index) + 1], length) != 0;
    // Allocation. TODO: adapt the string extraction to other codifications
    if (add_it && typecode == 3 && not_null && not_empty) {
        dev->boards_names = realloc(dev->boards_names, sizeof(char *) * dev->boards_names_count + 1);
        dev->boards_names[dev->boards_names_count] = calloc(length + 1, sizeof(uchar));
        while (data[(*index) + i] == ' ') {
            ++i;
        }
        memcpy(dev->boards_names[dev->boards_names_count], &data[(*index) + i], length);
        debug("FRU_STRING%02d saved (%s): '%s'", dev->boards_names_count, str_type,
              dev->boards_names[dev->boards_names_count]);
        // TODO: remove also special characters
        strtolow(dev->boards_names[dev->boards_names_count]);
        ++dev->boards_names_count;
    }
    *index = *index + 1 + length;
}

void ipmi_driver_parse_fru_board(uchar *data, ipmi_dev_t *dev)
{
    char *str = "board  ";
    // Check the function fru_area_print_board() of ipmitool's source code.
    // DATA [0:0] FRU version
    // DATA [1:1] FRU length
    // DATA [2:2] FRU board language
    // DATA [3:5] Board manufacturing date
    // DATA [6:?] Board manufacturer
    // DATA [?:?] etc
    uint i = 6;
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Board manufacturer
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Board product
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Board serial
    ipmi_driver_parse_fru_string(data, &i, dev, 0, str); // Board part-number
    ipmi_driver_parse_fru_string(data, &i, dev, 0, str); // Board FRU-id
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Board extra0
}

void ipmi_driver_parse_fru_product(uchar *data, ipmi_dev_t *dev)
{
    char *str = "product";
    // Check the function fru_area_print_product() of ipmitool's source code.
    // DATA [0:0] FRU version
    // DATA [1:1] FRU length
    // DATA [2:2] FRU board language
    // DATA [3:?] Product manufacturer
    // DATA [?:?] ...
    uint i = 3;
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Product manufacturer
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Product name
    ipmi_driver_parse_fru_string(data, &i, dev, 0, str); // Product part number
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Product version
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Product serial
}

void ipmi_driver_parse_fru_chassis(uchar *data, ipmi_dev_t *dev)
{
    char *str = "chassis";
    // Check the function fru_area_print_chassis() of ipmitool's source code.
    // DATA [0:0] FRU version
    // DATA [1:1] FRU length
    // DATA [?:?] ...
    uint i = 2;
    ipmi_driver_parse_fru_string(data, &i, dev, 0, str); // Chassis type
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Chassis part number
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Chassis serial
    ipmi_driver_parse_fru_string(data, &i, dev, 1, str); // Chassis extra0
}

void ipmi_driver_parse_sensor_reading(uchar raw, sdr_t *rec, ipmi_reading_t *val)
{
    // IPMI Doc: 36.3 Sensor Reading Conversion Formula
    //
    // Linearization formula:
    //   y = lf((mx + (b * 10^(b_exp))) * 10^(r_exp)) units
    //
    //   where:
    //   - x Raw reading
    //   - y Converted reading
    //   - lf() Linearization function specified by ‘lin_type’.
    //     This function is ‘null’ (y = f(x) = x) if the sensor is linear.
    //   - m Signed integer constant multiplier
    //   - b Signed additive ‘offset’
    //   - b_exp Signed Exponent. Sets ‘decimal point’ location for b.
    //   - r_exp Signed Result Exponent. Sets ‘decimal point’ location for
    //     the result before the linearization function is applied.
    double b_exp = (double) rec->formula_b_exp;
    double r_exp = (double) rec->formula_r_exp;
    double m     = (double) rec->formula_m;
    double b     = (double) rec->formula_b;
    double y     = 0.0;
    long x       = raw; // Unsigned

    // Complement processing
    if (rec->formula_complement == 0x02) { // Two's complement
        x = (long) ((char) raw);
    } else { // Ones' complement
        x = (((char) raw) >= 0) ? (long) ((uchar) raw) : (long) ((char) raw + 1);
    }
    // Operations
    b = b * pow(10.0, b_exp);
    y = (m * ((double) x)) + b;
    y = y * pow(10.0, r_exp);
    debug("%s #%u => %lfy = lf((%lfm*%ldx + (%lfb * 10^(%lfb_exp))) * 10^(%lfr_exp))", rec->name, rec->sensor_no, y, m,
          x, b, b_exp, r_exp);
    val->value = y;
}
