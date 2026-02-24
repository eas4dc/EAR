/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
#include <stdlib.h>

#include <pthread.h>
#include <metrics/common/ipmi.h>
#include <metrics/common/ipmi_driver.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

#define TEST_PWR_READING 0

state_t ipmi_open()
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
	#if TEST_PWR_READING
	if (getenv("EAR_ROOT_MODE") != NULL) s = EAR_SUCCESS;
	if (getenv("EAR_USER_MODE") != NULL) s = EAR_ERROR;
	#else
    s = ipmi_driver_open();
	#endif
    pthread_mutex_unlock(&lock);
    return s;
}

void ipmi_close()
{
    while (pthread_mutex_trylock(&lock));
	#if TEST_PWR_READING
	#else
    ipmi_driver_close();
	#endif
    pthread_mutex_unlock(&lock);
}

int ipmi_devs_count()
{
	#if TEST_PWR_READING
	return 1;
	#else
    return ipmi_driver_devs_count();
	#endif
}

int ipmi_has_hardware(char *string)
{
	#if TEST_PWR_READING
	return 1;
	#else
    return ipmi_driver_has_hardware(string);
	#endif
}

int ipmi_sensors_find(char *string, ipmi_sensor_t **sensors, uint *sensors_count)
{
	#if TEST_PWR_READING

	if (sensors_count) *sensors_count = 1;
	if (sensors){
		ipmi_sensor_t *lsensor = calloc(1, sizeof(ipmi_sensor_t *));
		lsensor[0].dev_no = 0;
		lsensor[0].rec_no = 0;
		lsensor[0].name = calloc(1, strlen("dummy")+1);
		strcpy(lsensor[0].name,"dummy");
		lsensor[0].available = 1;
		lsensor[0].units_str = calloc(1, strlen("Watts")+1);
		strcpy(lsensor[0].units_str, "Watts");
		*sensors = lsensor;
	}
	return 1;
	#else
    return ipmi_driver_sensors_find(string, sensors, sensors_count);
	#endif
}

state_t ipmi_sensors_read(ipmi_sensor_t *sensors, uint sensors_count, ipmi_reading_t **values)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
	#if TEST_PWR_READING
	if (!values) return EAR_ERROR;
	ipmi_reading_t *lvalues;
    if (*values == NULL) {
		lvalues = calloc(sensors_count, sizeof(ipmi_reading_t));
		*values = lvalues;
    }else{
		lvalues = *values;
	}
	lvalues[0].value = 600;
	#else
    s = ipmi_driver_sensors_read(sensors, sensors_count, values);
	#endif
    pthread_mutex_unlock(&lock);
    return s;
}

// OEM
void ipmi_cmd_set_paddr(ipmi_cmd_t *pkg, ipmi_addr_t *addr)
{
	#if TEST_PWR_READING
	#else
    ipmi_driver_cmd_set_paddr(pkg, addr);
	#endif
}

void ipmi_cmd_set_addr(ipmi_cmd_t *pkg, int type, short channel, uchar slave_addr, uchar lun)
{
	#if TEST_PWR_READING
	#else
    ipmi_driver_cmd_set_addr(pkg, type, channel, slave_addr, lun);
	#endif
}

void ipmi_cmd_set_msg(ipmi_cmd_t *pkg, uchar netfn, uchar cmd, ushort data_len)
{
	#if TEST_PWR_READING
	#else
    ipmi_driver_cmd_set_msg(pkg, netfn, cmd, data_len);
	#endif
}

state_t ipmi_cmd_send(int dev_no, ipmi_cmd_t *pkg)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
	#if TEST_PWR_READING
	s = EAR_SUCCESS;
	#else
    s = ipmi_driver_cmd_send(dev_no, pkg);
	#endif
    pthread_mutex_unlock(&lock);
    return s;
}
