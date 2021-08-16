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

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h> 
#include <pthread.h>
#include <common/output/verbose.h>
#include <common/states.h> 
#include <common/math_operations.h>
#include <metrics/energy/node/energy_sd650.h>
#include <metrics/energy/node/energy_node.h>



static pthread_mutex_t ompi_lock = PTHREAD_MUTEX_INITIALIZER;

int opendev(struct ipmi_intf *intf){
  intf->fd = open("/dev/ipmi0", O_RDWR);
  if (intf->fd < 0) {
  	intf->fd = open("/dev/ipmi/0", O_RDWR);
    if (intf->fd < 0) {
  	  intf->fd = open("/dev/ipmidev/0", O_RDWR);
    };
  };
	return intf->fd;
};

void closedev(struct ipmi_intf * intf){
  if (intf->fd > 0){
    close(intf->fd);
    intf->fd = -1 ;
  };
};

static struct ipmi_rs * sendcmd(struct ipmi_intf * intf, struct ipmi_rq * req)
{
	struct ipmi_req _req;
	struct ipmi_recv recv;
	struct ipmi_addr addr;
	struct ipmi_system_interface_addr bmc_addr = {
		addr_type:	IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
		channel:	IPMI_BMC_CHANNEL,
		};
	struct ipmi_ipmb_addr ipmb_addr = {
		addr_type:	IPMI_IPMB_ADDR_TYPE,
		channel:	intf->channel & 0x0f,
		};
	static struct ipmi_rs rsp;
	uint8_t * data = NULL;
	static int curr_seq = 0;
	fd_set rset;
	
	if (intf == NULL || req == NULL)
   	return NULL;
	memset(&_req, 0, sizeof(struct ipmi_req));
	
	if (intf->addr != 0){
		ipmb_addr.slave_addr = intf->addr;
		ipmb_addr.lun = req->msg.lun;
		_req.addr = (unsigned char *) &ipmb_addr;
		_req.addr_len = sizeof(ipmb_addr);
	
	}else {
	bmc_addr.lun = req->msg.lun;
	_req.addr = (unsigned char *) &bmc_addr;
	_req.addr_len = sizeof(bmc_addr);
	};
	_req.msgid = curr_seq++;
	
	_req.msg.data = req->msg.data;
	_req.msg.data_len = req->msg.data_len;
	_req.msg.netfn = req->msg.netfn;
	_req.msg.cmd = req->msg.cmd;
	
	if (ioctl(intf->fd, IPMICTL_SEND_COMMAND, &_req) < 0) {
  	debug("Unable to send command\n");
  	if (data != NULL)
     	free(data);
   	return NULL;
	};
	
	FD_ZERO(&rset);
	FD_SET(intf->fd, &rset);
	
	if (select(intf->fd+1, &rset, NULL, NULL, NULL) < 0) {
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


	/* save completion code */
	rsp.ccode = recv.msg.data[0];
	rsp.data_len = recv.msg.data_len - 1;
	
	if( recv.msg.data[0] == 0 ) {
	
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
};//sendcmd
	

// Robert Wolford provided command: ipmitool raw 0x3a 0x32 4 1 0 0 0 --> low frequency command
// Robert Wolford provided command: ipmitool raw 0x3a 0x32 4 2 0 0 0 --> High frequency command : Energy (J,mJ) and Time (sec,ms)
// lun=0x00 netfn=0x3a cmd=0x32    
// ipmitool raw 0x3a 0x32 4 2 0 0 0
// COMMENT: Add 2 bytes to this format
// the response format is (LSB first):
// Byte 0-1: the index/location of FPGA FIFO.
// Byte 2-5: energy reading(J)
// Byte 6-7: energy reading(mJ)
// Byte 8-11: the second info for timestamp
// Byte 12-13: the millisecond info for timestamp
//
state_t sd650_ene(struct ipmi_intf *intf,struct ipmi_data * out)
{
	struct ipmi_rs * rsp;
	struct ipmi_rq req;
	uint8_t msg_data[5];
	if (pthread_mutex_trylock(&ompi_lock)) {
    return EAR_BUSY;
  }

  memset(&req, 0, sizeof(req));
  req.msg.netfn = SD650_NETFN;
  req.msg.cmd = SD650_CMD;
  msg_data[0]=0x4;
  msg_data[1]=0x2;
  msg_data[2]=0x0;
  msg_data[3]=0x0;
  msg_data[4]=0x0;
  intf->addr=0x0;
  req.msg.data = msg_data;
  req.msg.data_len = sizeof(msg_data);

  rsp = sendcmd(intf, &req);
  if (rsp == NULL) {
        out->mode=-1;
				pthread_mutex_unlock(&ompi_lock);
        return EAR_ERROR;
  };
  if (rsp->ccode > 0) {
        out->mode=-1;
				pthread_mutex_unlock(&ompi_lock);
        return EAR_ERROR;
        };

  out->data_len=rsp->data_len;
	debug("data len %d ",out->data_len);
  int i;
  for (i=0;i<rsp->data_len; i++) {
  	out->data[i]=rsp->data[i];
		debug("data[%d]=%x ",i,out->data[i]);
  }
	debug("\n");
	pthread_mutex_unlock(&ompi_lock);
	return EAR_SUCCESS;
}; 

/*
 * MAIN FUNCTIONS
 */

state_t energy_init(void **c)
{
	int ret;
	if (c==NULL) return EAR_ERROR;
	*c=(struct ipmi_intf *)malloc(sizeof(struct ipmi_intf));
	if (*c==NULL) return EAR_ERROR;
	pthread_mutex_lock(&ompi_lock);
	ret= opendev((struct ipmi_intf *)*c);
	pthread_mutex_unlock(&ompi_lock);
	if (ret<0) return EAR_ERROR;
	return EAR_SUCCESS;
}
state_t energy_dispose(void **c)
{
	if ((c==NULL) || (*c==NULL)) return EAR_ERROR;
	pthread_mutex_lock(&ompi_lock);
	closedev((struct ipmi_intf *)*c);
	free(*c);
	pthread_mutex_unlock(&ompi_lock);
	return EAR_SUCCESS;
}
state_t energy_datasize(size_t *size)
{
	*size=sizeof(unsigned long);
	return EAR_SUCCESS;
}
state_t energy_frequency(ulong *freq_us)
{
	*freq_us=10000;	
	return EAR_SUCCESS;
}
#define FIRST_BYTE_TMS 12
#define FIRST_BYTE_TS 8
#define FIRST_BYTE_EJ	2
#define FIRST_BYTE_EMJ	6

state_t energy_dc_read(void *c, edata_t energy_mj)
{
	struct ipmi_data out;
	uint16_t aux_emj = 0;
	uint32_t aux_ej = 0;
	uint8_t *bytes_rs;
	state_t st;
	ulong *penergy_mj=(ulong *)energy_mj;

	*penergy_mj=0;
	st=sd650_ene((struct ipmi_intf *)c,&out);
	if (st!=EAR_SUCCESS) return st;
	bytes_rs=out.data;
	aux_ej=(bytes_rs[FIRST_BYTE_EJ+3] << 24) | (bytes_rs[FIRST_BYTE_EJ+2] << 16) | (bytes_rs[FIRST_BYTE_EJ+1] << 8) | (bytes_rs[FIRST_BYTE_EJ]);
	aux_emj=(bytes_rs[FIRST_BYTE_EMJ+1] << 8)  | (bytes_rs[FIRST_BYTE_EMJ]);
	*penergy_mj=	((ulong)aux_ej*1000)+(ulong)aux_emj;
	return EAR_SUCCESS;
}


state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms)
{
	struct ipmi_data out;
	uint16_t aux_emj = 0;
	uint16_t aux_tms = 0;
	uint32_t aux_ej = 0;
	uint32_t aux_ts = 0;
	uint8_t *bytes_rs;
	state_t st;
	ulong *penergy_mj = (ulong *)energy_mj;

	*penergy_mj = 0;
	*time_ms = 0;
	st = sd650_ene((struct ipmi_intf *)c,&out);
	if (st != EAR_SUCCESS) return st;
	bytes_rs = out.data;
	aux_ej = (bytes_rs[FIRST_BYTE_EJ+3] << 24) | (bytes_rs[FIRST_BYTE_EJ+2] << 16) | (bytes_rs[FIRST_BYTE_EJ+1] << 8) | (bytes_rs[FIRST_BYTE_EJ]);
	aux_emj = (bytes_rs[FIRST_BYTE_EMJ+1] << 8)  | (bytes_rs[FIRST_BYTE_EMJ]);
	*penergy_mj =	((ulong)aux_ej*1000)+(ulong)aux_emj;
	aux_tms = bytes_rs[FIRST_BYTE_TMS+1] <<  8 | bytes_rs[FIRST_BYTE_TMS];
	aux_ts = bytes_rs[FIRST_BYTE_TS+3] << 24 | bytes_rs[FIRST_BYTE_TS+2] << 16 | bytes_rs[FIRST_BYTE_TS+1] << 8 | bytes_rs[FIRST_BYTE_TS];
	*time_ms = ((ulong)aux_ts*1000)+(ulong)aux_tms;
	return EAR_SUCCESS;
}
state_t energy_ac_read(void *c, edata_t energy_mj)
{
	ulong *penergy_mj = (ulong *)energy_mj;
	*penergy_mj = 0;
	return EAR_SUCCESS;
}


unsigned long diff_node_energy(ulong init,ulong end)
{
  ulong ret = 0;
  if (end > init){
    ret = end - init;
  } else{
    ret = ulong_diff_overflow(init,end);
  }
  return ret;
}


state_t energy_units(uint *units)
{
  *units=1000;
  return EAR_SUCCESS;
}
state_t energy_accumulated(unsigned long *e,edata_t init,edata_t end)
{
	ulong *pinit = (ulong *)init,*pend=(ulong *)end;

  unsigned long total = diff_node_energy(*pinit,*pend);
  *e = total;
  return EAR_SUCCESS;
}
state_t energy_to_str(char *str,edata_t e)
{
  ulong *pe = (ulong *)e;
  sprintf(str,"%lu",*pe);
  return EAR_SUCCESS;
}

uint energy_data_is_null(edata_t e)  
{
  ulong *pe = (ulong *)e;
  return (*pe == 0);

}

