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
#include <sys/ioctl.h>
#include <common/states.h>
#include <common/system/poll.h>
#include <common/math_operations.h>
#include <common/types/generic.h> 
#include <common/output/verbose.h>
#include <common/system/monitor.h>
#include <metrics/energy/node/energy_dcmi.h>
#include <metrics/energy/node/energy_node.h>

#define MAX_LOCK_TRIES  1000000000
#define MIN_READ_PERIOD 2000       /*!< The minimum period time to for the periodic thread to read the power. */

/* For error control */
#define MAX_TIMES_POWER_SUPPORTED 10
#define VPOWER_ERROR 0
static        unsigned long     dcmi_last_power_measurement = 0;


static struct ipmi_intf         dcmi_context_for_pool;
static        dcmi_power_data_t dcmi_current_power_reading;
static        dcmi_power_data_t dcmi_last_power_reading;
static        ulong             dcmi_timeframe;
static        suscription_t    *dcmi_sus;

static int    monitor_done                            = 0;
static        ulong           dcmi_accumulated_energy = 0;
static        pthread_mutex_t ompi_lock               = PTHREAD_MUTEX_INITIALIZER;

static state_t dcmi_thread_init(void *p);
static        timestamp_t     last_timestamp;

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
	afd_set_t rset;

	if (intf == NULL || req == NULL)
		return NULL;
	memset(&_req, 0, sizeof(struct ipmi_req));

	if (intf->addr != 0) {
		ipmb_addr.slave_addr = intf->addr;
		ipmb_addr.lun = req->msg.lun;
		_req.addr = (unsigned char *) &ipmb_addr;
		_req.addr_len = sizeof(ipmb_addr);

	} else {
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


state_t dcmi_get_capabilities_enh_power(struct ipmi_intf *intf, struct ipmi_data *out)
{
	struct ipmi_rs *rsp;
	struct ipmi_rq req;
	uint8_t msg_data[2];

	
	int i;

	if (pthread_mutex_trylock(&ompi_lock)) {
    return EAR_BUSY;
  }
  memset(&req, 0, sizeof(req));
  req.msg.netfn = DCMI_NETFN;
  req.msg.cmd = DCMI_CMD_GET_CAP;
  msg_data[0]=(uint8_t)DCMI_GROUP_EXT;
  msg_data[1]=(uint8_t)DCMI_PARAM_ENH_POWER;
  intf->addr=0x0;
  req.msg.data = msg_data;
  req.msg.data_len = sizeof(msg_data);

  rsp = sendcmd(intf, &req);
  if (rsp == NULL) {
        out->mode=-1;
				error("sendcmd returns NULL");
				pthread_mutex_unlock(&ompi_lock);
        return EAR_ERROR;
  };
  if (rsp->ccode > 0) {
				error("Get capability command returned with error 0x%02x",(int)rsp->ccode);
        out->mode=-1;
				pthread_mutex_unlock(&ompi_lock);
        return EAR_ERROR;
        };

  out->data_len=rsp->data_len;
  for (i=0;i<rsp->data_len; i++) {
  	out->data[i]=rsp->data[i];
		#if SHOW_DEBUGS
		debug("Byte %d: 0x%02x\n",i,rsp->data[i]);
		#endif
  }
	/* 0: Group extension
 *  * 1:2 DCMI Specification Conformance (Major/Minor)
 *   * 3 Parameter revisions
 *    * 6:N parameter data
 *     */
	/* Parameter data for capabilities
 *  * 0 -> 6 number of supported ra_time_periods 
 *  2:n ra_time_periods [7:6] duration unit [5:0] duration
 *    */
	#if SHOW_DEBUGS
	uint8_t num_rolling_avg, ra_time_period;
	num_rolling_avg = rsp->data[6];
	debug("There are %u rolling averag periods\n",num_rolling_avg);

	for (i=0;i<num_rolling_avg;i++){
		ra_time_period = rsp->data[7+i];
		debug("Units %u Time %u\n",ra_time_period&0xC0,ra_time_period&0x3F);
	}		
	#endif
	
	pthread_mutex_unlock(&ompi_lock);
	return EAR_SUCCESS;

}
state_t dcmi_get_capabilities(struct ipmi_intf *intf, struct ipmi_data *out)
{
	struct ipmi_rs *rsp;
	struct ipmi_rq req;
	uint8_t msg_data[2];

	
	int i;

	if (pthread_mutex_trylock(&ompi_lock)) {
    return EAR_BUSY;
  }
  memset(&req, 0, sizeof(req));
  req.msg.netfn = DCMI_NETFN;
  req.msg.cmd = DCMI_CMD_GET_CAP;
  msg_data[0]=(uint8_t)DCMI_GROUP_EXT;
  msg_data[1]=(uint8_t)DCMI_PARAM_SUP_CAP;
  intf->addr=0x0;
  req.msg.data = msg_data;
  req.msg.data_len = sizeof(msg_data);

  rsp = sendcmd(intf, &req);
  if (rsp == NULL) {
        out->mode=-1;
				error("sendcmd returns NULL");
				pthread_mutex_unlock(&ompi_lock);
        return EAR_ERROR;
  };
  if (rsp->ccode > 0) {
				error("Get capability command returned with error 0x%02x",(int)rsp->ccode);
        out->mode=-1;
				pthread_mutex_unlock(&ompi_lock);
        return EAR_ERROR;
        };

  out->data_len=rsp->data_len;
  for (i=0;i<rsp->data_len; i++) {
  	out->data[i]=rsp->data[i];
		#if 0
		debug("Byte %d: 0x%02x\n",i,rsp->data[i]);
		#endif
  }
	/* 0: Group extension
 * 1:2 DCMI Specification Conformance (Major/Minor)
 * 3 Parameter revisions
 * 6:N parameter data
 */
	/* Parameter data for capabilities
 * 0 -> 6 reserved 
 * 1 -> Platform capabilities. Bit 0 refers to Power management
 */
		
	#if SHOW_DEBUGS
  uint8_t major,minor,revision,power_enabled;
	major=rsp->data[1];
	minor=rsp->data[2];
	revision=rsp->data[3];
	power_enabled=rsp->data[5]&(uint8_t)0x01;
	debug("Major %u minor %u revision %u power management enabled %u \n",(uint)major,(uint)minor,(uint)revision,(uint)power_enabled);
	#endif
	
	pthread_mutex_unlock(&ompi_lock);
	return EAR_SUCCESS;

}


state_t dcmi_power_reading(struct ipmi_intf *intf, struct ipmi_data *out,dcmi_power_data_t *cpower)
{
    struct ipmi_rs *rsp;
    struct ipmi_rq req;
    uint8_t msg_data[4];
    uint16_t current_power, min_power,max_power,avg_power;
    uint16_t *current_powerp, *min_powerp,*max_powerp,*avg_powerp;
    uint32_t timeframe,timestamp;
    uint32_t *timeframep,*timestampp;
    int i;

    /*
     * NETFN=0x2c 
     * COMMAND=0x02 
     * Group EXTENSSION=0xdc 
     * 0x01=SYTEMPOWER STATSTIC 
     * 0x00 0x00 = RESERVED
     *
     * OUTPUT
     * 1 -> COMP CODE
     * 2 -> GROUP ID =0xDC
     * 3:4 -> POwer in watts
     * 5:6 -> min power
     * 7:8 -> max power
     * 9:10 --> average power (*)
     * 11:14 IPMI Specification based Time Stamp
     * 15:18 Timeframe in milliseconds
     * 19 power reading state
     */
    memset(&req, 0, sizeof(req));
    req.msg.netfn = DCMI_NETFN;
    req.msg.cmd = DCMI_CMD_GET_POWER;
    msg_data[0] = (uint8_t)DCMI_GROUP_EXT;
    msg_data[1] = (uint8_t)0x01;
    msg_data[2] = (uint8_t)0x00;
    msg_data[3] = (uint8_t)0x00;
    intf->addr = 0x0;
    req.msg.data = msg_data;
    req.msg.data_len = sizeof(msg_data);

    rsp = sendcmd(intf, &req);
    if (rsp == NULL) {
        out->mode = -1;
        debug("sendcmd returns NULL");
        return EAR_ERROR;
    };
    if (rsp->ccode > 0) {
        debug("Power reading command returned with error 0x%02x",(int)rsp->ccode);
        out->mode = -1;
        return EAR_ERROR;
    };

    out->data_len=rsp->data_len;
    for (i=0;i<rsp->data_len; i++) {
        out->data[i] = rsp->data[i];
#if 0
        debug("Byte %d: 0x%02x\n",i,rsp->data[i]);
#endif
    }
    /*
     *   uint16_t current_power, min_power,max_power,avg_power;
     *     uint32_t timeframe;
     */
    /*
     *3:4 -> POwer in watts
     *5:6 -> min power
     *7:8 -> max power
     *9:10 --> average power (*)
     *11:14 IPMI Specification based Time Stamp
     *15:18 Timeframe in milliseconds
     */
    current_powerp = (uint16_t *)&rsp->data[1];
    current_power  = *current_powerp;
    min_powerp = (uint16_t *)&rsp->data[3];
    max_powerp = (uint16_t *)&rsp->data[5];
    avg_powerp = (uint16_t *)&rsp->data[7];
    timestampp = (uint32_t *)&rsp->data[9];
    timeframep = (uint32_t *)&rsp->data[13];
    min_power = *min_powerp;
    max_power = *max_powerp;
    avg_power = *avg_powerp;
    timeframe = *timeframep;
    timestamp = *timestampp;
#if SHOW_DEBUGS
    uint8_t power_state,*power_statep;
    power_statep = (uint8_t *)&rsp->data[17];
    power_state  = ((*power_statep&0x40) > 0);
    debug("current power 0x%04x min power 0x%04x max power 0x%04x avg power 0x%04x\n",current_power,min_power,max_power,avg_power);
    debug("current power %u min power %u max power %u avg power %u\n",(uint)current_power,(uint)min_power,(uint)max_power,(uint)avg_power);
    debug("timeframe %u timestamp %u\n",timeframe,timestamp);
    debug("power state %u\n",(uint)power_state);
#endif
    cpower->current_power = (ulong)current_power;
    cpower->min_power     = (ulong)min_power;
    cpower->max_power     = (ulong)max_power;
    cpower->avg_power     = (ulong)avg_power;
    cpower->timeframe     = (ulong)timeframe;
    cpower->timestamp     = (ulong)timestamp;

    /* For error control */
    if (cpower->current_power > cpower->max_power){
      verbose(VPOWER_ERROR,"enery_dcmi: New max power reading current:%lu max %lu", cpower->current_power, cpower->max_power);
    }
    if ((cpower->current_power < (cpower->max_power * MAX_TIMES_POWER_SUPPORTED)) && ( cpower->current_power > 0)){
      dcmi_last_power_measurement = cpower->current_power;
    }else{
      verbose(VPOWER_ERROR, "enery_dcmi:warning, invalid power detected. current %lu, corrected to last valid power %lu", cpower->current_power, dcmi_last_power_measurement);
       cpower->current_power = dcmi_last_power_measurement;
    }


    return EAR_SUCCESS;
}

/** These functions are used to accumulate the eneergy. */
state_t dcmi_thread_main(void *p)
{
    struct ipmi_data out;
    state_t st;
    int etries = 0, lret;

    debug("dcmi_thread_main");

    ulong current_elapsed, current_energy;
		timestamp_t   curr_time;
    ullong        dcmi_elapsed;

    while ((lret = pthread_mutex_trylock(&ompi_lock)) && (etries < MAX_LOCK_TRIES)) {
        etries++;
    }
    if ((etries == MAX_LOCK_TRIES) && lret) {
        return EAR_ERROR;
    }

    st = dcmi_power_reading(&dcmi_context_for_pool, &out, &dcmi_current_power_reading);
    if (st == EAR_ERROR) {
        debug("dcmi_power_reading fails in dcmi_thread_main");
        pthread_mutex_unlock(&ompi_lock);
        return st;
    }

    /* We use this timestamp because DCMI fails */
    timestamp_get(&curr_time);
    dcmi_elapsed = timestamp_diff(&curr_time, &last_timestamp, TIME_SECS);
    last_timestamp = curr_time;


    if (dcmi_current_power_reading.current_power > 0) {
        //current_elapsed = dcmi_current_power_reading.timestamp - dcmi_last_power_reading.timestamp;	
        current_elapsed = (ulong) dcmi_elapsed;
        current_energy = current_elapsed * dcmi_current_power_reading.avg_power;

        /* Energy is reported in MJ */
        dcmi_accumulated_energy += (current_energy * 1000);
        verbose(2,"DCMI AVG power in last %lu sec is %lu", current_elapsed, dcmi_current_power_reading.avg_power);		
    } else {
        debug("Current power is 0 in dcmi power reading pool reading, Resetting the context");
				closedev(&dcmi_context_for_pool);
				st = dcmi_thread_init(NULL);
    }

    memcpy(&dcmi_last_power_reading, &dcmi_current_power_reading, sizeof(dcmi_power_data_t));
    pthread_mutex_unlock(&ompi_lock);
    return EAR_SUCCESS;
}

state_t dcmi_thread_init(void *p)
{
    int ret;
    ret	= opendev(&dcmi_context_for_pool);
    if (ret < 0){
        debug("opendev fails in dcmi energy plugin when initializing pool");
        return EAR_ERROR;
    } 

    timestamp_get(&last_timestamp);


    debug("thread_init for dcmi_power OK");
    return EAR_SUCCESS;
}

/*
 * MAIN FUNCTIONS
 */

state_t energy_init(void **c)
{
    struct ipmi_data out;
    state_t st;
    dcmi_power_data_t my_power;
    int ret;
    int etries = 0, lret;


    if (c == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    }
    *c = (struct ipmi_intf *) malloc(sizeof(struct ipmi_intf));
    if (*c == NULL) {
        return EAR_ERROR;
    }
    //
    while ((lret = pthread_mutex_trylock(&ompi_lock)) && (etries < MAX_LOCK_TRIES)) {
        etries++;
    }
    if ((etries == MAX_LOCK_TRIES) && lret) {
        return EAR_ERROR;
    }

    debug("trying opendev\n");
    ret = opendev((struct ipmi_intf *)(*c));
    if (ret < 0) {
        pthread_mutex_unlock(&ompi_lock);
        return_print(EAR_ERROR, "error opening IPMI device (%s)", strerror(errno));
    }

    // TODO: Check for root?

    st = dcmi_power_reading(*c, &out, &my_power);
    memcpy(&dcmi_last_power_reading, &my_power, sizeof(dcmi_power_data_t));
    pthread_mutex_unlock(&ompi_lock);
    if (st != EAR_SUCCESS) {
        debug("dcmi_power_reading fails");
    } else {
        if (!monitor_done) {
        		dcmi_timeframe = my_power.timeframe;
            dcmi_sus = suscription();
            dcmi_sus->call_main = dcmi_thread_main;
            dcmi_sus->call_init = dcmi_thread_init;
            dcmi_sus->time_relax = ear_max(dcmi_timeframe, MIN_READ_PERIOD);
            dcmi_sus->time_burst = ear_max(dcmi_timeframe, MIN_READ_PERIOD);
            dcmi_sus->suscribe(dcmi_sus);
            debug("dcmi energy plugin suscription initialized with timeframe %lu ms", dcmi_timeframe);
            monitor_done = 1;
        }
    }
		verbose(2, "DCMI Init ok");

    return EAR_SUCCESS;
}

state_t energy_dispose(void **c) {
    if ((c == NULL) || (*c == NULL)){ 
			verbose(2, "DCMI context is NULL in energy dispose");
			return EAR_ERROR;
		}


    int etries = 0;
    int lret;

    while((lret = pthread_mutex_trylock(&ompi_lock)) && (etries < MAX_LOCK_TRIES)) etries++;
    if ((etries == MAX_LOCK_TRIES) && lret) {
        return EAR_ERROR;
    }
    closedev((struct ipmi_intf *) *c);
    free(*c);

    pthread_mutex_unlock(&ompi_lock);
		verbose(2, "DCMI Dispose ok");
    return EAR_SUCCESS;
}

state_t energy_datasize(size_t *size)
{
	debug("energy_datasize %lu\n", sizeof(ulong));

	*size = sizeof(ulong);

	return EAR_SUCCESS;
}

state_t energy_frequency(ulong *freq_us) {
	*freq_us = dcmi_timeframe;
	return EAR_SUCCESS;
}

state_t energy_to_str(char *str, edata_t e) {
        ulong *pe = (ulong *) e;
        sprintf(str, "%lu", *pe);
        return EAR_SUCCESS;
}

unsigned long diff_node_energy(ulong init,ulong end)
{
  ulong ret = 0;
  if (end >= init) {
    ret = end-init;
  } else {
    ret = ulong_diff_overflow(init, end);
  }
  return ret;
}

state_t energy_units(uint *units) {
  *units = 1000;
  return EAR_SUCCESS;
}

state_t energy_accumulated(unsigned long *e, edata_t init, edata_t end) {
  ulong *pinit = (ulong *) init, *pend = (ulong *) end;

  ulong total = diff_node_energy(*pinit, *pend);
  *e = total;
  return EAR_SUCCESS;
}

// TODO: re-factor the code along with thread_main and energy_dc_time_read
state_t energy_dc_read(void *c, edata_t energy_mj)
{
    ulong *penergy_mj = (ulong *) energy_mj;
    timestamp_t   curr_time;
    ullong        dcmi_elapsed;


    debug("energy_dc_read");

    // As we are reading the power every 2 seconds, we first read the current power.
    struct ipmi_data out;
    state_t st = EAR_SUCCESS;
    int etries = 0, lret;

    ulong current_elapsed, current_energy;

    while ((lret = pthread_mutex_trylock(&ompi_lock)) && (etries < MAX_LOCK_TRIES)) {
        etries++;
    }
    if ((etries == MAX_LOCK_TRIES) && lret) {
        return EAR_ERROR;
    }

    st = dcmi_power_reading((struct ipmi_intf *)c, &out, &dcmi_current_power_reading);
    if (st == EAR_ERROR) {
        debug("dcmi_power_reading fails in energy_dc_read");
        pthread_mutex_unlock(&ompi_lock);
        return st;
    }

    /* We use this timestamp because DCMI fails */
    timestamp_get(&curr_time);
    dcmi_elapsed = timestamp_diff(&curr_time, &last_timestamp, TIME_SECS);
    last_timestamp = curr_time;


    if (dcmi_current_power_reading.current_power > 0) {
        //current_elapsed = dcmi_current_power_reading.timestamp - dcmi_last_power_reading.timestamp;	
        current_elapsed = (ulong) dcmi_elapsed;
        current_energy = current_elapsed * dcmi_current_power_reading.avg_power;

        /* Energy is reported in MJ */
        dcmi_accumulated_energy += (current_energy * 1000);
    		memcpy(&dcmi_last_power_reading, &dcmi_current_power_reading, sizeof(dcmi_power_data_t));

        debug("AVG power in last %lu ms is %lu", current_elapsed, dcmi_current_power_reading.avg_power);		
    } else {
        debug("Current power is 0 in dcmi power reading pool reading");
        st = EAR_ERROR;
    }
    *penergy_mj = dcmi_accumulated_energy;
    pthread_mutex_unlock(&ompi_lock);

    return st;
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms) 
{
    ulong *penergy_mj=(ulong *)energy_mj;
		state_t st = EAR_SUCCESS;

    debug("energy_dc_read\n");

    // As we are reading the power every 2 seconds, we first read the current power.
    struct ipmi_data out;
    int etries = 0, lret;

    ulong current_elapsed, current_energy;

    while ((lret = pthread_mutex_trylock(&ompi_lock)) && (etries < MAX_LOCK_TRIES)) {
        etries++;
    }
    if ((etries == MAX_LOCK_TRIES) && lret) {
        return EAR_ERROR;
    }

    st = dcmi_power_reading((struct ipmi_intf *)c, &out, &dcmi_current_power_reading);
    if (st == EAR_ERROR) {
        debug("dcmi_power_reading fails in energy_dc_read");
        pthread_mutex_unlock(&ompi_lock);
        return st;
    }

    if (dcmi_current_power_reading.current_power > 0) {
        current_elapsed = dcmi_current_power_reading.timestamp - dcmi_last_power_reading.timestamp;	
        current_energy = current_elapsed * dcmi_current_power_reading.avg_power;

        /* Energy is reported in MJ */
        dcmi_accumulated_energy += (current_energy * 1000);

        debug("AVG power in last %lu ms is %lu", current_elapsed, dcmi_current_power_reading.avg_power);		
    } else {
        debug("Current power is 0 in dcmi power reading pool reading");
        st = EAR_ERROR;
    }

    memcpy(&dcmi_last_power_reading, &dcmi_current_power_reading, sizeof(dcmi_power_data_t));

    *penergy_mj = dcmi_accumulated_energy;
    *time_ms = dcmi_last_power_reading.timestamp*1000;
    pthread_mutex_unlock(&ompi_lock);

    return st;

}
uint energy_data_is_null(edata_t e)  
{
    ulong *pe=(ulong *)e;
    return (*pe == 0);

}

