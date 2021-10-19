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

#define FIRST_SIGNIFICANT_BYTE 3
#define POWER_PERIOD 5

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <common/states.h> 
#include <common/types/generic.h> 
#include <common/math_operations.h>
#include <common/output/verbose.h>
#include <metrics/energy/node/energy_inm_power.h>
#include <metrics/energy/node/energy_node.h>
#include <common/system/monitor.h>

static pthread_mutex_t ompi_lock = PTHREAD_MUTEX_INITIALIZER;
static suscription_t *inm_power_sus;
static ulong inm_power_already_loaded=0;
static ulong inm_power_timeframe;
static ulong inm_power_accumulated_energy=0;
static inm_power_data_t inm_current_power_reading,inm_last_power_reading;
static struct ipmi_intf inm_context_for_pool;

static int opendev(struct ipmi_intf *intf)
{
	intf->fd = open("/dev/ipmi0", O_RDWR);
	if (intf->fd < 0) {
		intf->fd = open("/dev/ipmi/0", O_RDWR);
		if (intf->fd < 0) {
			intf->fd = open("/dev/ipmidev/0", O_RDWR);
		};
	};
	return intf->fd;
};

static void closedev(struct ipmi_intf *intf)
{
	if (intf->fd > 0) {
		close(intf->fd);
		intf->fd = -1;
	};
};

static struct ipmi_rs *sendcmd(struct ipmi_intf *intf, struct ipmi_rq *req)
{
	struct ipmi_req _req;
	struct ipmi_recv recv;
	struct ipmi_addr addr;
	struct ipmi_system_interface_addr bmc_addr = {
			addr_type:    IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
			channel:    IPMI_BMC_CHANNEL,
	};
	struct ipmi_ipmb_addr ipmb_addr = {
			addr_type:    IPMI_IPMB_ADDR_TYPE,
			channel:    intf->channel & 0x0f,
	};
	static struct ipmi_rs rsp;
	uint8_t *data = NULL;
	static int curr_seq = 0;
	fd_set rset;

	if (intf == NULL || req == NULL)
		return NULL;
	memset(&_req, 0, sizeof(struct ipmi_req));

	if (intf->addr != 0) {
		ipmb_addr.slave_addr = 0x2c;
		ipmb_addr.lun = req->msg.lun;
		_req.addr = (unsigned char *) &ipmb_addr;
		_req.addr_len = sizeof(ipmb_addr);
		//debug("IPMB channel %x addr %x", ipmb_addr.channel, ipmb_addr.slave_addr);

	} else {
		bmc_addr.lun = req->msg.lun;
		_req.addr = (unsigned char *) &bmc_addr;
		_req.addr_len = sizeof(bmc_addr);
		//debug("BMC channel %x", bmc_addr.channel);
	};
	_req.msgid = curr_seq++;

	_req.msg.data = req->msg.data;
	_req.msg.data_len = req->msg.data_len;
	_req.msg.netfn = req->msg.netfn;
	_req.msg.cmd = req->msg.cmd;


	if (ioctl(intf->fd, IPMICTL_SEND_COMMAND, &_req) < 0) {
		debug("ioctl Unable to send command %s\n", strerror(errno));
		if (data != NULL)
			free(data);
		return NULL;
	};

	FD_ZERO(&rset);
	FD_SET(intf->fd, &rset);

	if (select(intf->fd + 1, &rset, NULL, NULL, NULL) < 0) {
		debug("I/O Error\n");
		if (data != NULL)
			free(data);
		return NULL;
	};
	if (FD_ISSET(intf->fd, &rset) == 0) {
		debug("No data available\n");
		if (data != NULL)
			free(data);
		return NULL;
	};

	recv.addr = (unsigned char *) &addr;
	recv.addr_len = sizeof(addr);
	recv.msg.data = rsp.data;
	recv.msg.data_len = sizeof(rsp.data);
	if (ioctl(intf->fd, IPMICTL_RECEIVE_MSG_TRUNC, &recv) < 0) {
		debug("Error receiving message\n");
		if (errno != EMSGSIZE) {
			if (data != NULL)
				free(data);
			return NULL;
		};
	};

	// debug("ioctl completed with code 0x%02x len 0x%02x  ", recv.msg.data[0], recv.msg.data_len - 1);
	/* save completion code */
	rsp.ccode = recv.msg.data[0];
	rsp.data_len = recv.msg.data_len - 1;



	if (recv.msg.data[0] == 0) {

		/* save response data for caller */
		if (rsp.ccode == 0 && rsp.data_len > 0) {
			memmove(rsp.data, rsp.data + 1, rsp.data_len);
			rsp.data[recv.msg.data_len] = 0;
		};

		if (data != NULL)
			free(data);
		return &rsp;
	};
	rsp.ccode = recv.msg.data[0];
	rsp.data_len = recv.msg.data_len - 1;
	return &rsp;
} //sendcmd




state_t inm_power_reading(struct ipmi_intf *intf, struct ipmi_data *out,inm_power_data_t *cpower)
{
	struct ipmi_rs *rsp;
	struct ipmi_rq req;
	uint8_t msg_data[6];
	uint16_t current_power, min_power,max_power,avg_power;
	uint16_t *current_powerp, *min_powerp,*max_powerp,*avg_powerp;
	uint32_t timeframe,timestamp;
	uint32_t *timeframep,*timestampp;
	int i;
/*
 * ipmitool -b 0 -t 0x2C raw 0x2E 0xC8 0x57 0x01 0x00 0x01 0x00 0x00

In the returned byte string, power is in bytes 4-5.  4th byte is LSB.*/

/*
 * NET_FN = 2E
 * LUN    = 0b
 * CMD    = C8
 * Request
Byte 1:3 – Intel Manufacturer ID – 000157h, LS byte first.
Byte 4 – Mode [0:4] – Mode =01h – Global power statistics in [Watts].
							[5:7] – Reserved. Write as 000b.
Byte 5 – Domain ID [0:3] – Domain ID (Identifies the domain that this Intel® Node Manager policy applies to) =00h – Entire platform
									 [4:7] – Reserved. Write as 0000b.
Byte 6 – Policy ID (ignored)
* Answer
* Byte 1 – Completion code
* Byte 2:4 – Intel Manufacturer ID – 000157h, LS byte first.
* Byte 5:6 – Current Value (unsigned integer) see Section 3.1.1
Byte 7:8 – Minimum Value (unsigned integer) see Section 3.1.2
Byte 9:10 – Maximum Value (unsigned integer) see Section 3.1.3
Byte 11:12 – Average Value (unsigned integer) see Section 3.1.4
Byte 13:16 – Timestamp as defined by the IPMI v2.0 specification indicating when the response message was sent. If Intel® NM cannot obtain valid time, the timestamp is set to FFFFFFFFh as defined in the IPMI v2.0 specification.
Byte 17:20 – Statistics Reporting Period (the timeframe in seconds, over which the firmware collects statistics). This is unsigned integer value. For all global statistics this field contains the time after the last statistics reset.
 *
 */
  memset(&req, 0, sizeof(req));
  req.msg.netfn = INM_POWER_NETFN;
  req.msg.cmd = INM_POWER_CMD;
	/* Intel Manufacturer ID – 000157h, LS byte first */
  msg_data[0]=(uint8_t)0x57;
  msg_data[1]=(uint8_t)0x01;
  msg_data[2]=(uint8_t)0x00;
	/* Reserved + Mode = 01 */
  msg_data[3]=(uint8_t)0x01;
	/* Domain ID = 0 */
  msg_data[4]=(uint8_t)0x00;
	/* Policy ID */
  msg_data[5]=(uint8_t)0x00;

  intf->addr    = 0x20;
	intf->channel = 0x0;

  req.msg.data = msg_data;
  req.msg.data_len = sizeof(msg_data);

  rsp = sendcmd(intf, &req);
  if (rsp == NULL) {
        out->mode=-1;
				debug("sendcmd returns NULL");
        return EAR_ERROR;
  };
  if (rsp->ccode > 0) {
				debug("Power reading command returned with error 0x%02x",(int)rsp->ccode);
        out->mode=-1;
        return EAR_ERROR;
        };
	debug("Command returns with code %dii len %d", rsp->ccode, rsp->data_len);
  out->data_len = rsp->data_len;
  for (i=0;i<rsp->data_len; i++) {
  	out->data[i]=rsp->data[i];
		debug("Byte %d: 0x%02x\n",i,rsp->data[i]);
  }
	/*
 *   uint16_t current_power, min_power,max_power,avg_power;
 *     uint32_t timeframe;
 */
	/*
 * * Byte 1 – Completion code
* Byte 2:4 – Intel Manufacturer ID – 000157h, LS byte first.  3b
* Byte 5:6 – Current Value (unsigned integer) see Section 3.1.1 2b
Byte 7:8 – Minimum Value (unsigned integer) see Section 3.1.2 2b
Byte 9:10 – Maximum Value (unsigned integer) see Section 3.1.3 2b
Byte 11:12 – Average Value (unsigned integer) see Section 3.1.4 2b
Byte 13:16 – Timestamp as defined by the IPMI v2.0 specification indicating when the response message was sent. If Intel® NM cannot obtain valid time, the timestamp is set to FFFFFFFFh as defined in the IPMI v2.0 specification. 4b
Byte 17:20 – Statistics Reporting Period (the timeframe in seconds, over which the firmware collects statistics). This is unsigned integer value. For all global statistics this field contains the time after the last statistics reset. 4b
 */
	current_powerp=(uint16_t *)&rsp->data[FIRST_SIGNIFICANT_BYTE]; // 3-4
	current_power=*current_powerp;
	min_powerp=(uint16_t *)&rsp->data[FIRST_SIGNIFICANT_BYTE+2]; // 5-6
	max_powerp=(uint16_t *)&rsp->data[FIRST_SIGNIFICANT_BYTE+4]; // 7-8
	avg_powerp=(uint16_t *)&rsp->data[FIRST_SIGNIFICANT_BYTE+6]; // 9-10
	timestampp=(uint32_t *)&rsp->data[FIRST_SIGNIFICANT_BYTE+8]; // 11-14
	timeframep=(uint32_t *)&rsp->data[FIRST_SIGNIFICANT_BYTE+12]; // 15-18
	min_power=*min_powerp;
	max_power=*max_powerp;
	avg_power=*avg_powerp;
	timeframe=*timeframep;
	timestamp=*timestampp;
	#if SHOW_DEBUGS
	//debug("current power 0x%04x min power 0x%04x max power 0x%04x avg power 0x%04x\n",current_power,min_power,max_power,avg_power);
	debug("current power %u min power %u max power %u avg power %u\n",(uint)current_power,(uint)min_power,(uint)max_power,(uint)avg_power);
	debug("timeframe %u timestamp %u\n",(uint)timeframe,(uint)timestamp);
	#endif
	cpower->current_power=(ulong)current_power;
	cpower->min_power=(ulong)min_power;
	cpower->max_power=(ulong)max_power;
	cpower->avg_power=(ulong)avg_power;
	cpower->timeframe=(ulong)timeframe;
	cpower->timestamp=(ulong)timestamp;
	
	return EAR_SUCCESS;
}

/**** These functions are used to accumulate the eneergy */
state_t inm_power_thread_main(void *p)
{
	struct ipmi_data out;
	ulong current_elapsed,current_energy;
	inm_power_reading(&inm_context_for_pool, &out,&inm_current_power_reading);
	if (inm_current_power_reading.current_power > 0){
		//current_elapsed = inm_current_power_reading.timeframe - inm_last_power_reading.timeframe;	
		current_elapsed = POWER_PERIOD;
		current_energy = current_elapsed * inm_current_power_reading.avg_power;
		/* Energy is reported in MJ */
		inm_power_accumulated_energy += (current_energy*1000);
		debug("AVG power in last %lu sec is %u Total energy %lu\n",current_elapsed,inm_current_power_reading.avg_power, inm_power_accumulated_energy);		
	}else{
		debug("Current power is 0 in inm power reading pool reading");
		return EAR_ERROR;
	}
  memcpy(&inm_last_power_reading,&inm_current_power_reading,sizeof(inm_power_data_t));
	return EAR_SUCCESS;
}

state_t inm_power_thread_init(void *p)
{
	int ret;
	ret	= opendev(&inm_context_for_pool);
 	if (ret < 0){
		debug("opendev fails in inm energy plugin when initializing pool");
		return EAR_ERROR;
	} 
	debug("thread_init for inm_power OK");
	return EAR_SUCCESS;
}

/*
 * MAIN FUNCTIONS
 */

state_t energy_init(void **c)
{
	struct ipmi_data out;
	state_t st;
	inm_power_data_t my_power;
	int ret;

	if (c == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	*c = (struct ipmi_intf *) malloc(sizeof(struct ipmi_intf));
	if (*c == NULL) {
		return EAR_ERROR;
	}
	//
	pthread_mutex_lock(&ompi_lock);
	debug("trying opendev\n");
	ret = opendev((struct ipmi_intf *)*c);
	if (ret<0){ 
		pthread_mutex_unlock(&ompi_lock);
		return_print(EAR_ERROR, "error opening IPMI device (%s)", strerror(errno));
	}

	if (inm_power_already_loaded == 0){
		inm_power_already_loaded = 1;
		st = inm_power_reading(*c, &out,&my_power);
		if (st != EAR_SUCCESS){
			debug("inm_power_reading fails");
		}else{
			memcpy(&inm_last_power_reading,&my_power,sizeof(inm_power_data_t));
			inm_power_timeframe = my_power.timeframe;
			inm_power_sus = suscription();
			inm_power_sus->call_main = inm_power_thread_main;
			inm_power_sus->call_init = inm_power_thread_init;
			inm_power_sus->time_relax = POWER_PERIOD*1000;
			inm_power_sus->time_burst = POWER_PERIOD*1000;
			inm_power_sus->suscribe(inm_power_sus);
			//debug("inm energy plugin suscription initialized with timeframe %lu ms",inm_power_timeframe);
		}
	}
	pthread_mutex_unlock(&ompi_lock);

	return EAR_SUCCESS;
}

state_t energy_dispose(void **c) {
	if ((c == NULL) || (*c == NULL)) return EAR_ERROR;
	pthread_mutex_lock(&ompi_lock);
	closedev((struct ipmi_intf *) *c);
	free(*c);
	pthread_mutex_unlock(&ompi_lock);
	return EAR_SUCCESS;
}
state_t energy_datasize(size_t *size)
{
	debug("energy_datasize %lu\n",sizeof(unsigned long));
	*size=sizeof(unsigned long);
	return EAR_SUCCESS;
}

state_t energy_frequency(ulong *freq_us) {
	*freq_us = inm_power_timeframe;
	return EAR_SUCCESS;
}

state_t energy_to_str(char *str, edata_t e) {
        ulong *pe = (ulong *) e;
        sprintf(str, "%lu", *pe);
        return EAR_SUCCESS;
}

unsigned long diff_node_energy(ulong init,ulong end)
{
  ulong ret=0;
  if (end>=init){
    ret=end-init;
  } else{
    ret=ulong_diff_overflow(init,end);
  }
  return ret;
}
state_t energy_units(uint *units) {
  *units = 1000;
  return EAR_SUCCESS;
}

state_t energy_accumulated(unsigned long *e, edata_t init, edata_t end) {
  ulong *pinit = (ulong *) init, *pend = (ulong *) end;

  unsigned long total = diff_node_energy(*pinit, *pend);
  *e = total;
  return EAR_SUCCESS;
}



state_t energy_dc_read(void *c, edata_t energy_mj) {
	ulong *penergy_mj=(ulong *)energy_mj;

  //debug("energy_dc_read\n");

  *penergy_mj=inm_power_accumulated_energy;

	return EAR_SUCCESS;
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms) 
{
  ulong *penergy_mj=(ulong *)energy_mj;

  //debug("energy_dc_read\n");

  *penergy_mj = inm_power_accumulated_energy;
	*time_ms = inm_last_power_reading.timeframe;

  return EAR_SUCCESS;

}
uint energy_data_is_null(edata_t e)  
{
  ulong *pe=(ulong *)e;
  return (*pe == 0);

}

