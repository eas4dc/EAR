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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1
#define VGPU 2

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
#include <common/config.h>
#include <common/states.h> //clean
#include <common/system/poll.h>
#include <common/math_operations.h>
#include <common/output/verbose.h>
#include <metrics/energy/node/energy_nm.h>
#include <metrics/energy/node/energy_node.h>

#include <metrics/gpu/gpu.h>

static uint    gpu_sel_model;
static uint    gpu_num;

typedef struct sd650N_v2{
	ulong ac_energy;
	ulong dc_energy;
	gpu_t gpus_energy[MAX_GPUS_SUPPORTED];
	uint  gpu_num;
}sd650N_v2_t;


static pthread_mutex_t ompi_lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t cmd_arg;

int opendev(struct ipmi_intf *intf)
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

void closedev(struct ipmi_intf *intf)
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
	afd_set_t rset;

	if (intf == NULL || req == NULL)
		return NULL;
	memset(&_req, 0, sizeof(struct ipmi_req));

	if (intf->addr != 0) {
		ipmb_addr.slave_addr = intf->addr;
		ipmb_addr.lun = req->msg.lun;
		_req.addr = (unsigned char *) &ipmb_addr;
		_req.addr_len = sizeof(ipmb_addr);
		debug("IPMB channel %x", ipmb_addr.channel);

	} else {
		bmc_addr.lun = req->msg.lun;
		_req.addr = (unsigned char *) &bmc_addr;
		_req.addr_len = sizeof(bmc_addr);
		debug("BMC channel %x", bmc_addr.channel);
	};
	_req.msgid = curr_seq++;

	_req.msg.data = req->msg.data;
	_req.msg.data_len = req->msg.data_len;
	_req.msg.netfn = req->msg.netfn;
	_req.msg.cmd = req->msg.cmd;

	debug("Channel %p, bcm %p ipmb %p", _req.addr, &bmc_addr, &ipmb_addr);


	if (ioctl(intf->fd, IPMICTL_SEND_COMMAND, &_req) < 0) {
		debug("Unable to send command\n");
		if (data != NULL)
			free(data);
		return NULL;
	};

	AFD_ZERO(&rset);
	AFD_SET(intf->fd, &rset);

	if (aselectv(&rset, NULL) < 0) {
		debug("I/O Error\n");
		if (data != NULL)
			free(data);
		return NULL;
	};
	if (AFD_ISSET(intf->fd, &rset) == 0) {
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


state_t nm_arg(struct ipmi_intf *intf, struct ipmi_data *out)
{
  struct ipmi_rs * rsp;
  struct ipmi_rq req;
  uint8_t msg_data[6];
	debug("getting nm_arg\n");
// ipmitool raw 0x2e 0x82 0x66 0x4a 0 0 0 1 --> Command to get the parameter (0x20 in Lenovo) bytes_rq[6]
// // sudo ./ipmi-raw 0x0 0x2e 0x82 0x66 0x4a 0 0 0 1 
// // byte number 8 with ipmi-raw command

//// bytes_rq[3]=(uint8_t)0x66;
//// bytes_rq[4]=(uint8_t)0x4a;
//// bytes_rq[5]=(uint8_t)0x00;
//// bytes_rq[6]=(uint8_t)0x00;
//// bytes_rq[7]=(uint8_t)0x00;
//// bytes_rq[8]=(uint8_t)0x01;
//
  memset(&req, 0, sizeof(req));
  req.msg.netfn = NM_NETFN;
  req.msg.cmd = NM_CMD_GET_ARG;
  msg_data[0]=(uint8_t)0x66;
  msg_data[1]=(uint8_t)0x4a;
  msg_data[2]=(uint8_t)0x00;
  msg_data[3]=(uint8_t)0x00;
  msg_data[4]=(uint8_t)0x00;
  msg_data[5]=(uint8_t)0x01;
  intf->addr=0x0;
  req.msg.data = msg_data;
  req.msg.data_len = sizeof(msg_data);
	debug("sending command\n");
  rsp = sendcmd(intf, &req);
  if (rsp == NULL) {
        out->mode=-1;
				debug("Error rsp null\n");
        return EAR_ERROR;
  };

#if 1
	if (rsp->ccode > 0) {
		out->mode=-1;
		debug("error code greater than zero (%d)", rsp->ccode);
		return EAR_ERROR;
	}
#endif

  out->data_len=rsp->data_len;
  int i;
  for (i=0;i<rsp->data_len; i++) {
    out->data[i]=rsp->data[i];
		debug("cmd arg byte %d is %hu\n",i,out->data[i]);
  }
	debug("nm_arg ok\n");
  return EAR_SUCCESS;
}

state_t nm_ene(struct ipmi_intf *intf, struct ipmi_data *out)
{
	struct ipmi_rs *rsp;
	struct ipmi_rq req;
	uint8_t msg_data[8];
	int i;

	if (pthread_mutex_trylock(&ompi_lock)) {
    return EAR_BUSY;
  }

// ipmitool raw 0x2E 0x81 0x66 0x4A 0x00 0x20 0x01 0x82 0x00 0x08
//// bytes_rq[3]=(uint8_t)0x66;
//// bytes_rq[4]=(uint8_t)0x4A;
//// bytes_rq[5]=(uint8_t)0x00;
//// bytes_rq[6]=bytes_rs[8];
//// bytes_rq[7]=(uint8_t)0x01;
//// bytes_rq[8]=(uint8_t)0x82;
//// bytes_rq[9]=(uint8_t)0x00;
//// bytes_rq[10]=(uint8_t)0x08;

  memset(&req, 0, sizeof(req));
  req.msg.netfn = NM_NETFN;
  req.msg.cmd = NM_CMD_ENERGY;
  msg_data[0]=(uint8_t)0x66;
  msg_data[1]=(uint8_t)0x4A;
  msg_data[2]=(uint8_t)0x00;
  msg_data[3]=cmd_arg;
  msg_data[4]=(uint8_t)0x01;
  msg_data[5]=(uint8_t)0x82;
  msg_data[6]=(uint8_t)0x00;
  msg_data[7]=(uint8_t)0x08;
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
  for (i=0;i<rsp->data_len; i++) {
  	out->data[i]=rsp->data[i];
  }
	pthread_mutex_unlock(&ompi_lock);
	return EAR_SUCCESS;
}

state_t nm_power_limit(struct ipmi_intf *intf, unsigned long limit,uint8_t target)
{
#if 0
	/* sudo ipmitool -b 0 -t 0x2c raw 0x2E 0xD0 0x57 0x01 0x00 TARGET LIMIT_LSB LIMIT_HSB */
	/* -b channel     Set destination channel for bridged request
 *   -t address     Bridge request to remote target address*/
	uint16_t limit2b;
	uint8_t *limitb;
	struct ipmi_rs *rsp;
  struct ipmi_rq req;
  uint8_t msg_data[8];
  int i;
  int s;
	limit2b=(uint16_t)limit;
	limitb=(uint8_t*)limit2b;

  if (pthread_mutex_trylock(&ompi_lock)) {
    return EAR_BUSY;
  }
	memset(&req, 0, sizeof(req));
  req.msg.netfn = NM_NETFN;
  req.msg.cmd = NM_CMD_ENERGY;
  msg_data[0]=(uint8_t)0x66;
  msg_data[1]=(uint8_t)0x4A;
  msg_data[2]=(uint8_t)0x00;
  msg_data[3]=cmd_arg;
  msg_data[4]=(uint8_t)0x01;
  msg_data[5]=(uint8_t)0x82;
  msg_data[6]=(uint8_t)0x00;
  msg_data[7]=(uint8_t)0x08;
  intf->addr=0x0;
  req.msg.data = msg_data;
  req.msg.data_len = sizeof(msg_data);

  rsp = send_power_cmd(intf, &req);
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
  for (i=0;i<rsp->data_len; i++) {
    out->data[i]=rsp->data[i];
  }
  pthread_mutex_unlock(&ompi_lock);
  return EAR_SUCCESS;
#endif

	return EAR_SUCCESS;

}


/*
 * MAIN FUNCTIONS
 */
#define CMD_ARG_BYTE	6

state_t energy_init(void **c)
{
	struct ipmi_data out;
	state_t st;
	int ret;

	if (c == NULL) {
		return EAR_ERROR;
	}

	*c = (struct ipmi_intf *) malloc(sizeof(struct ipmi_intf));
	if (*c == NULL) {
		return EAR_ERROR;
	}

	pthread_mutex_lock(&ompi_lock);
	debug("trying opendev\n");
	ret= opendev((struct ipmi_intf *)*c);
	if (ret<0){ 
		debug("opendev fails\n");
		pthread_mutex_unlock(&ompi_lock);
		return EAR_ERROR;
	}
	st=nm_arg((struct ipmi_intf *)*c,&out);
	if (st!=EAR_SUCCESS){ 
		debug("nm fails\n");
		pthread_mutex_unlock(&ompi_lock);
		return st;
	}
	cmd_arg=out.data[CMD_ARG_BYTE];
	debug("cmd arg is %hu\n",cmd_arg);
	pthread_mutex_unlock(&ompi_lock);

	verbose(VGPU,"GPU part");

    state_t s;
    // Loading GPU
    gpu_load(EARD);
    // Getting the loaded internal API
    gpu_get_api(&gpu_sel_model); 

    if (state_fail(s = gpu_init(no_ctx))) {
        verbose(VGPU, "gpu_init returned %d (%s)", s, state_msg);
        gpu_sel_model = API_DUMMY;
    }
    if (gpu_sel_model == API_DUMMY) {
        gpu_num = 0;
    }else{
        if (state_fail(s = gpu_count_devices(no_ctx, &gpu_num))) {
            verbose(VGPU, "gpu_count returned %d (%s)", s, state_msg);
        }
	}
	verbose(VGPU, "GPU model %d DUMMY %d GPUS detected %d", gpu_sel_model, gpu_sel_model == API_DUMMY, gpu_num);
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
	*size = sizeof(sd650N_v2_t);
	return EAR_SUCCESS;
}

state_t energy_frequency(ulong *freq_us) {
	*freq_us = 1000000;
	return EAR_SUCCESS;
}

state_t energy_dc_read(void *c, edata_t energy_mj) {
	struct ipmi_data out;
	uint8_t *bytes_rs;
	int FIRST_BYTE_EMJ;
	state_t st;
	ulong * energyp;
	sd650N_v2_t *penergy_mj = (sd650N_v2_t *)energy_mj;
	
	debug("energy_dc_read\n");

	penergy_mj->dc_energy = 0;
	st = nm_ene((struct ipmi_intf *)c,&out);
	if (st != EAR_SUCCESS) return st;
	FIRST_BYTE_EMJ = out.data_len-8;
	bytes_rs = out.data;
	energyp = (unsigned long *)&bytes_rs[FIRST_BYTE_EMJ];
	penergy_mj->dc_energy = (unsigned long)be64toh(*energyp);
	penergy_mj->gpu_num   = (uint)gpu_num;
	if (gpu_num){
		state_t s;
		if (state_fail(s = gpu_read(no_ctx, penergy_mj->gpus_energy))) {
    	verbose(VGPU,"gpu_read returned %d (%s)", s, state_msg);
  	}
		verbose(VGPU, "GPU read done");
	}
	return EAR_SUCCESS;
}

state_t energy_power_limit(void *c, unsigned long limit,unsigned long target) {
  state_t st;

  debug("energy_power_limit limit=%luW target=%lu\n",limit,target);
  st = nm_power_limit((struct ipmi_intf *)c,limit,(uint8_t)target);
  return st;
}


state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms) {
	struct ipmi_data out;
	unsigned long  *energyp;
	int FIRST_BYTE_EMJ;
	uint8_t *bytes_rs;
	state_t st;
	struct timeval t;
	sd650N_v2_t *penergy_mj = (sd650N_v2_t *) energy_mj;

	penergy_mj->dc_energy = 0;
	*time_ms = 0;
	st = nm_ene((struct ipmi_intf *) c, &out);
	if (st != EAR_SUCCESS) return st;
	bytes_rs = out.data;
	FIRST_BYTE_EMJ = out.data_len - 8;
	energyp = (unsigned long *) &bytes_rs[FIRST_BYTE_EMJ];
	penergy_mj->dc_energy = (unsigned long) be64toh(*energyp);
	penergy_mj->gpu_num   = (uint)gpu_num;
	if (gpu_num){
        state_t s;
        if (state_fail(s = gpu_read(no_ctx, penergy_mj->gpus_energy))) {
            verbose(VGPU, "gpu_read returned %d (%s)", s, state_msg);
        }
		verbose(VGPU, "GPU read done");
    }

	gettimeofday(&t, NULL);
	*time_ms = t.tv_sec * 1000 + t.tv_usec / 1000;
	return EAR_SUCCESS;
}

state_t energy_ac_read(void *c, edata_t energy_mj) {
	sd650N_v2_t *penergy_mj = (sd650N_v2_t *) energy_mj;
	penergy_mj->ac_energy = 0;
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
	sd650N_v2_t *pinit = (sd650N_v2_t *) init;
	sd650N_v2_t *pend  = (sd650N_v2_t *) end;
	gpu_t gpu_diff[MAX_GPUS_SUPPORTED];
	ulong total_gpu = 0;

	unsigned long total = diff_node_energy(pinit->dc_energy, pend->dc_energy);
	verbose(0, "Enode init %lu Enode end %lu Ediff %lu", pinit->dc_energy,pend->dc_energy, total);
	if (pinit->gpu_num) {
			verbose(VGPU, "Computing energy per GPU");
            gpu_data_diff(pend->gpus_energy, pinit->gpus_energy, gpu_diff);
			for (uint i = 0; i < pinit->gpu_num; i++){ 
				verbose(0, "GPU[%d]=%lu acum %lu", i, (unsigned long)gpu_diff[i].energy_j, total_gpu);
				total_gpu += (unsigned long)gpu_diff[i].energy_j;
			}
	}
	verbose(0, "Energy node %lu (%lu-%lu) GPU energy %lu", total, pinit->dc_energy, pend->dc_energy, total_gpu*1000);
	*e = total + total_gpu*1000;
	return EAR_SUCCESS;
}

state_t energy_to_str(char *str, edata_t e) {
        sd650N_v2_t *pe = (sd650N_v2_t *) e;
        sprintf(str, "dc_energy %lu " , pe->dc_energy);
				if (pe->gpu_num){
					char gpu_power_buffer[128];
					for (uint i = 0; i < pe->gpu_num; i++){
						snprintf(gpu_power_buffer, sizeof(gpu_power_buffer), "gpu_power[%d] = %.1lf ", i, pe->gpus_energy[i].power_w);
						strcat(str, gpu_power_buffer);	
					}
				}
				verbose(VGPU,"energy_to_str:%s" , str);
        return EAR_SUCCESS;
}

state_t power_limit(ulong limit)
{
	verbose(1,"Energy Intel Node Manager setting limit to %lu",limit);
	return EAR_SUCCESS;
}
uint energy_data_is_null(edata_t e)  
{
  sd650N_v2_t *pe=(sd650N_v2_t *)e;
  return (pe->dc_energy == 0 && pe->gpus_energy[0].power_w == 0);

}

