/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <metrics/common/ipmi.h>
#include <metrics/common/ipmi_driver.h>
#include <pthread.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

state_t ipmi_open()
{
    state_t s;
    while (pthread_mutex_trylock(&lock))
        ;
    s = ipmi_driver_open();
    pthread_mutex_unlock(&lock);
    return s;
}

void ipmi_close()
{
    while (pthread_mutex_trylock(&lock))
        ;
    ipmi_driver_close();
    pthread_mutex_unlock(&lock);
}

int ipmi_devs_count()
{
    return ipmi_driver_devs_count();
}

int ipmi_has_hardware(char *string)
{
    return ipmi_driver_has_hardware(string);
}

int ipmi_sensors_find(char *string, ipmi_sensor_t **sensors, uint *sensors_count)
{
    return ipmi_driver_sensors_find(string, sensors, sensors_count);
}

state_t ipmi_sensors_read(ipmi_sensor_t *sensors, uint sensors_count, ipmi_reading_t **values)
{
    state_t s;
    while (pthread_mutex_trylock(&lock))
        ;
    s = ipmi_driver_sensors_read(sensors, sensors_count, values);
    pthread_mutex_unlock(&lock);
    return s;
}

// OEM
void ipmi_cmd_set_paddr(ipmi_cmd_t *pkg, ipmi_addr_t *addr)
{
    ipmi_driver_cmd_set_paddr(pkg, addr);
}

void ipmi_cmd_set_addr(ipmi_cmd_t *pkg, int type, short channel, uchar slave_addr, uchar lun)
{
    ipmi_driver_cmd_set_addr(pkg, type, channel, slave_addr, lun);
}

void ipmi_cmd_set_msg(ipmi_cmd_t *pkg, uchar netfn, uchar cmd, ushort data_len)
{
    ipmi_driver_cmd_set_msg(pkg, netfn, cmd, data_len);
}

state_t ipmi_cmd_send(int dev_no, ipmi_cmd_t *pkg)
{
    state_t s;
    while (pthread_mutex_trylock(&lock))
        ;
    s = ipmi_driver_cmd_send(dev_no, pkg);
    pthread_mutex_unlock(&lock);
    return s;
}
