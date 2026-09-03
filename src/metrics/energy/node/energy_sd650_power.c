/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

// #define SHOW_DEBUGS 1
#include <common/config.h>

#include <common/colors.h>
#include <common/math_operations.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/lock.h>
#include <common/system/monitor.h>
#include <common/system/poll.h>
#include <errno.h>
#include <fcntl.h>
#include <metrics/energy/node/energy_node.h>
#include <metrics/energy/node/energy_sd650.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static struct sd650_node_power last_power_value;
static ulong  accum_sd650_energy = 0;
static ulong  last_time = 0;
static uint sd650_power_initialized = 0;
static uint monitor_done = 0;

static pthread_mutex_t sd650_power_lock         = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sd650_power_reading_lock = PTHREAD_MUTEX_INITIALIZER;
static struct ipmi_intf sd650_context_for_pool;
static struct ipmi_intf sd650_context_for_main;
static suscription_t *sd650_power_sus;
static state_t sd650_power_thread_main(void *p);
static state_t sd650_power_thread_init(void *p);

static uint valid_power = 0;
#if FAKE_PLUGIN_POWER_READING
static atomic_int reading = 0;
#endif

static int opendev(struct ipmi_intf *intf)
{
#if FAKE_PLUGIN_POWER_READING
    intf->fd = 100;
    debug("No open");
#else
    intf->fd = open("/dev/ipmi0", O_RDWR);
    if (intf->fd < 0) {
        intf->fd = open("/dev/ipmi/0", O_RDWR);
        if (intf->fd < 0) {
            intf->fd = open("/dev/ipmidev/0", O_RDWR);
        };
    };
    debug("SD650_power: DEV open FD=%d", intf->fd);
#endif
    return intf->fd;
};

static void closedev(struct ipmi_intf *intf)
{
#if FAKE_PLUGIN_POWER_READING
    debug("NO close");
    return;
#else
    if (intf->fd >= 0) {
        close(intf->fd);
        intf->fd = -1;
    };
    debug("SD650_power: DEV closed");
#endif
};

static struct ipmi_rs *sendcmd(struct ipmi_intf *intf, struct ipmi_rq *req, struct ipmi_rs *rsp)
{
    struct ipmi_req _req;
#if !defined(FAKE_PLUGIN_POWER_READING) || (FAKE_PLUGIN_POWER_READING == 0)
    struct ipmi_recv recv;
    struct ipmi_addr addr;
#endif
    struct ipmi_system_interface_addr bmc_addr = {
        .addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
        .channel   = IPMI_BMC_CHANNEL,
    };
    struct ipmi_ipmb_addr ipmb_addr = {
        .addr_type = IPMI_IPMB_ADDR_TYPE,
        .channel   = intf->channel & 0x0f,
    };
    // Moved as argument
    // static struct ipmi_rs rsp;
    uint8_t *data              = NULL;
    static atomic_int curr_seq = 0;
    afd_set_t rset;

    debug("SD650_power: sendcmd %p %p", intf, req);

    if (intf == NULL || req == NULL)
        return NULL;

    if (ear_trylock(&sd650_power_reading_lock) != EAR_SUCCESS) {
        debug("SD650_power: power_reading Lock not available");
        return NULL;
    }

    memset(&_req, 0, sizeof(struct ipmi_req));

    if (intf->addr != 0) {
        ipmb_addr.slave_addr = intf->addr;
        ipmb_addr.lun        = req->msg.lun;
        _req.addr            = (unsigned char *) &ipmb_addr;
        _req.addr_len        = sizeof(ipmb_addr);

    } else {
        bmc_addr.lun  = req->msg.lun;
        _req.addr     = (unsigned char *) &bmc_addr;
        _req.addr_len = sizeof(bmc_addr);
    };
    _req.msgid = atomic_fetch_add(&curr_seq, 1);

    _req.msg.data     = req->msg.data;
    _req.msg.data_len = req->msg.data_len;
    _req.msg.netfn    = req->msg.netfn;
    _req.msg.cmd      = req->msg.cmd;

    debug("IOCTL FD=%d", intf->fd);
#if FAKE_PLUGIN_POWER_READING
#else
    if (ioctl(intf->fd, IPMICTL_SEND_COMMAND, &_req) < 0) {
        debug("Unable to send command\n");
        if (data != NULL)
            free(data);
        ear_unlock(&sd650_power_reading_lock);
        return NULL;
    };
#endif

    AFD_ZERO(&rset);
    AFD_SET(intf->fd, &rset);

    debug("SD650_power waiting for data to be available");

    struct timeval tout;
    tout.tv_sec  = MAX_TIMEOUT_ENERGY_READING;
    tout.tv_usec = 0;

#if FAKE_PLUGIN_POWER_READING
    atomic_fetch_add(&reading, 1);
    if ((reading % 10) == 0) {
        debug("Item %d: Delay %ld secs !!!!", reading, tout.tv_sec);
        sleep(tout.tv_sec);
        debug("End Delay!!!!");
        if (data != NULL)
            free(data);
        ear_unlock(&sd650_power_reading_lock);
        return NULL;
    }
    rsp->ccode = 0;
#else
    if (aselectv(&rset, &tout) <= 0) {
        debug("I/O Error\n");
        if (data != NULL)
            free(data);
        ear_unlock(&sd650_power_reading_lock);
        return NULL;
    };

    debug("SD650_power aselectv returns timeout ", );

    if (AFD_ISSET(intf->fd, &rset) == 0) {
        debug("No data available\n");
        if (data != NULL)
            free(data);
        ear_unlock(&sd650_power_reading_lock);
        return NULL;
    };

    recv.addr         = (unsigned char *) &addr;
    recv.addr_len     = sizeof(addr);
    recv.msg.data     = rsp->data;
    recv.msg.data_len = sizeof(rsp->data);

    debug("IOCTL FD=%d", intf->fd);
    if (ioctl(intf->fd, IPMICTL_RECEIVE_MSG_TRUNC, &recv) < 0) {
        debug("Error receiving message\n");
        if (errno != EMSGSIZE) {
            if (data != NULL)
                free(data);
            ear_unlock(&sd650_power_reading_lock);
            return NULL;
        };
    };

    /* save completion code */
    rsp->ccode    = recv.msg.data[0];
    rsp->data_len = recv.msg.data_len - 1;

    if (recv.msg.data[0] == 0) {

        /* save response data for caller */
        if (rsp->ccode == 0 && rsp->data_len > 0) {
            memmove(rsp->data, rsp->data + 1, rsp->data_len);
            rsp->data[recv.msg.data_len] = 0;
        };

        if (data != NULL)
            free(data);
        ear_unlock(&sd650_power_reading_lock);
        return rsp;
    };
    rsp->ccode    = recv.msg.data[0];
    rsp->data_len = recv.msg.data_len - 1;
#endif
    ear_unlock(&sd650_power_reading_lock);
    debug("Send command finished");
    return rsp;
}; // sendcmd

/* Information gathered from Lenovo document:
 *
 * Single Power Command
 * --------------------
 * Command: Get individual device power (watt) reading
 *	On SD650 V3 the power reading does not include GPU.
 * Raw Command String: 0x3A 0x32 4 8 0 0 0
 *	Response (LSB is first for each field):
 *		1f 19 9a 60 49 01 b5 00 05 01 19 00 00 00
 *	Response Decode:
 *		Timestamp associated with power reading:
 *			Bytes 0-3 =seconds (e.g. 1f 19 9a 60 =101,325,087 sec –Unix epoch time)
 *			Bytes 4-5 =mS (e.g. 49 01 =329 mS)
 *				Total time = 101,325,087.329 seconds
 *			Byte 6-7 =Decimal (LSB first), Node power:
 *				b5 00 = 00b5h = 181w
 *			Byte 8-9 =Decimal (LSB first), GPU power:
 *				05 01 = 0105h = 261w
 *			Byte 10-11 ==Decimal (LSB first), Riser1 power:
 *				19 00 = 0019h = 25w
 *	Node power includes the x86 compute portion of the tray+riser1+riser2 power.
 *	Total tray power for SD650N V2 is Node power+GPU power.
 *		In the example above it is 181W + 261W=442W.
 */

#define FIRST_BYTE_POWER_SEC		0
#define FIRST_BYTE_POWER_MSEC		4
#define FIRST_BYTE_NODE_POWER		6
#define FIRST_BYTE_GPU_POWER		8

#define SD650_POWER_PERIOD 2000

/* The lock must be gathered outside the function */
static state_t sd650_power_reading(struct ipmi_intf *intf,struct sd650_node_power * power_data)
{
    struct ipmi_rs *rsp, rsp_in;
    struct ipmi_rq req;
    uint8_t msg_data[5];
#if !defined(FAKE_PLUGIN_POWER_READING) || (FAKE_PLUGIN_POWER_READING == 0)
    uint8_t *bytes_rs;
#endif
    struct ipmi_data out;

    (void) out;

    debug("SD650_power: sd650_power_reading starts");

  memset(&req, 0, sizeof(req));
  req.msg.netfn = SD650_NETFN;
  req.msg.cmd = SD650_CMD;
  msg_data[0]=0x4;
  msg_data[1]=0x8;
  msg_data[2]=0x0;
  msg_data[3]=0x0;
  msg_data[4]=0x0;
  intf->addr=0x0;
  req.msg.data = msg_data;
  req.msg.data_len = sizeof(msg_data);

  memset(power_data, 0 , sizeof(struct sd650_node_power));

    rsp = sendcmd(intf, &req, &rsp_in);
    if (rsp == NULL) {
        debug("sendcmd returns NULL");
        out.mode = -1;
        return EAR_ERROR;
  };
  if (rsp->ccode > 0) {
	debug("ccode error %d", rsp->ccode);
        out.mode=-1;
        return EAR_ERROR;
    };
#if FAKE_PLUGIN_POWER_READING
    memset(power_data, 0, sizeof(struct sd650_node_power));
    power_data->seconds    = 0;
    power_data->mseconds   = 0;
    power_data->node_power = 500;
    power_data->gpu_power  = 1000;
#else

    out.data_len = rsp->data_len;
    debug("data len %d ", out.data_len);
    int i;
    for (i = 0; i < rsp->data_len; i++) {
        out.data[i] = rsp->data[i];
        debug("data[%d]=%x ", i, out.data[i]);
    }
    debug("\n");

    /* Processing answer */
    bytes_rs = out.data;
    memset(power_data, 0, sizeof(struct sd650_node_power));
    power_data->seconds = (bytes_rs[FIRST_BYTE_POWER_SEC + 3] << 24) | (bytes_rs[FIRST_BYTE_POWER_SEC + 2] << 16) |
                          (bytes_rs[FIRST_BYTE_POWER_SEC + 1] << 8) | (bytes_rs[FIRST_BYTE_POWER_SEC]);
    power_data->mseconds   = (bytes_rs[FIRST_BYTE_POWER_MSEC + 1] << 8) | (bytes_rs[FIRST_BYTE_POWER_MSEC]);
    power_data->node_power = (bytes_rs[FIRST_BYTE_NODE_POWER + 1] << 8) | (bytes_rs[FIRST_BYTE_NODE_POWER]);
    power_data->gpu_power  = (bytes_rs[FIRST_BYTE_GPU_POWER + 1] << 8) | (bytes_rs[FIRST_BYTE_GPU_POWER]);

#endif
    valid_power = 1;

    debug("SD650_power: Secs %lu msecs %lu Node power %lu GPU power %lu", power_data->seconds, power_data->mseconds,
          power_data->node_power, power_data->gpu_power);
    return EAR_SUCCESS;
};

/* Used by monitor */
static state_t sd650_power_thread_main(void *p)
{
	struct sd650_node_power out;
	ulong current_elapsed,current_energy;
	ulong curr_time;
	state_t st;

	if (!sd650_power_initialized) return EAR_ERROR;

	debug("%sSD650_power sd650_power_thread_main%s", COL_RED, COL_CLR);

    st = sd650_power_reading(&sd650_context_for_pool, &out);
    if (ear_trylock(&sd650_power_lock) != EAR_SUCCESS) {
        debug("SD650_power: Lock not available");
        return EAR_ERROR;
    }
    if (st == EAR_SUCCESS) {
        debug("SD650_power sd650_power_reading success");

        curr_time       = (ulong) timestamp_getconvert(TIME_MSECS);
        current_elapsed = overflow_zeros_u64(curr_time, last_time);
        // current_elapsed = SD650_POWER_PERIOD;
        // current_elapsed is msec
        current_energy = current_elapsed * (out.node_power + out.gpu_power);
        /* Energy is reported in MJ */
        accum_sd650_energy += current_energy;
        memcpy(&last_power_value, &out, sizeof(struct sd650_node_power));

        debug("AVG power in elapsed %lu msec (curr %lu last %lu) is %lu Current energy %lu Accumulated energy %lu\n",
              current_elapsed, curr_time, last_time, out.node_power + out.gpu_power, current_energy,
              accum_sd650_energy);
        last_time = curr_time;
        // last_time is msec
        ear_unlock(&sd650_power_lock);

    } else {
        if (valid_power) {
            debug("Error but using estimation");
            /* Using last value */
            curr_time       = (ulong) timestamp_getconvert(TIME_MSECS);
            current_elapsed = overflow_zeros_u64(curr_time, last_time);
            // current_elapsed is msec
            current_energy = current_elapsed * (last_power_value.node_power + last_power_value.gpu_power);
            accum_sd650_energy += current_energy;
            // last_time is msec
            debug("Estimated: AVG power in elapsed %lu msec (curr %lu last %lu) is %lu Curr energy %lu Total energy "
                  "%lu\n",
                  current_elapsed, curr_time, last_time, out.node_power + out.gpu_power, current_energy,
                  accum_sd650_energy);
            last_time = curr_time;
        }
        ear_unlock(&sd650_power_lock);
        debug("SD650_power: Error ");
        return (valid_power ? EAR_SUCCESS : EAR_ERROR);
    }
    return EAR_SUCCESS;
}

static state_t sd650_power_thread_init(void *p)
{
	int ret;
	state_t st;
	struct sd650_node_power first_power;

	debug("%sSD650_power sd650_power_thread_init%s", COL_GRE, COL_CLR);

    ret = opendev(&sd650_context_for_pool);
    if (ret < 0) {
        debug("SD650_poer: opendev failed");
        return EAR_ERROR;
    }

    st = sd650_power_reading(&sd650_context_for_pool, &first_power);
    if (ear_trylock(&sd650_power_lock) != EAR_SUCCESS) {
        debug("SD650_power: Lock not available");
        return EAR_ERROR;
    }

    if (st != EAR_SUCCESS) {
        debug("SD650_power: energy_init fails");
        ear_unlock(&sd650_power_lock);
        return EAR_ERROR;
    }
    last_time = (ulong) timestamp_getconvert(TIME_MSECS);
    memcpy(&last_power_value, &first_power, sizeof(struct sd650_node_power));
    debug("Init: SD650_power Init AVG power in last %lu sec is %lu Total energy %lu\n", last_time,
          first_power.node_power + first_power.gpu_power, accum_sd650_energy);

	ear_unlock(&sd650_power_lock);

	return EAR_SUCCESS;
}

/*
 * MAIN FUNCTIONS
 */

state_t energy_init(void **c)
{
	int ret;

	if (c == NULL) return EAR_ERROR;
        *c = (struct ipmi_intf *)calloc(1,sizeof(struct ipmi_intf));
        if (*c == NULL) return EAR_ERROR;

	debug("SD650_power context allocated %p", (struct ipmi_intf *)*c);

    ear_lock(&sd650_power_lock);
    if (sd650_power_initialized) {
        memcpy((struct ipmi_intf *) *c, &sd650_context_for_main, sizeof(struct ipmi_intf));
        ear_unlock(&sd650_power_lock);
        return EAR_SUCCESS;
    }

    /* Open a static dev for main queries */
    ret = opendev((struct ipmi_intf *) &sd650_context_for_main);
    if (ret < 0) {
        debug("SD650_poer: opendev failed");
        ear_unlock(&sd650_power_lock);
        return EAR_ERROR;
    }
    memcpy((struct ipmi_intf *) *c, &sd650_context_for_main, sizeof(struct ipmi_intf));
	
	if (!monitor_is_initialized()){
		debug("SD650_power initializing monitor");
		monitor_init();
	}
	if (!monitor_done){
		sd650_power_sus = suscription();
		sd650_power_sus->call_main = sd650_power_thread_main;
		sd650_power_sus->call_init = sd650_power_thread_init;
		sd650_power_sus->time_relax = SD650_POWER_PERIOD;
		sd650_power_sus->time_burst = SD650_POWER_PERIOD;
		sd650_power_initialized = 1;
		monitor_done = 1;
		ear_unlock(&sd650_power_lock);
		//sd650_power_sus->suscribe(sd650_power_sus);
		monitor_register(sd650_power_sus);
		debug("SD650_power: energy plugin suscription initialized with timeframe %u ms",SD650_POWER_PERIOD);

	}else ear_unlock(&sd650_power_lock);
		

	return EAR_SUCCESS;
}
state_t energy_dispose(void **c)
{
	if ((c==NULL) || (*c==NULL)) 	return EAR_ERROR;
	if (!sd650_power_initialized) 	return EAR_ERROR;

	ear_lock(&sd650_power_lock);
	closedev((struct ipmi_intf *)*c);
	ear_unlock(&sd650_power_lock);
	return EAR_SUCCESS;
}
state_t energy_datasize(size_t *size)
{
    if (!size)
        return EAR_ERROR;
    *size = sizeof(unsigned long);
    return EAR_SUCCESS;
}
state_t energy_frequency(ulong *freq_us)
{
    if (!freq_us)
        return EAR_ERROR;
    *freq_us = 10000;
    return EAR_SUCCESS;
}
#define FIRST_BYTE_TMS 12
#define FIRST_BYTE_TS 8
#define FIRST_BYTE_EJ	2
#define FIRST_BYTE_EMJ	6



state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms)
{
	struct sd650_node_power out;
	ulong  current_elapsed;

    ulong *penergy_mj = (ulong *) energy_mj;

    if (!penergy_mj || !time_ms)
        return EAR_ERROR;
    debug("SD650_power: energy_dc_time_read\n");
    if (!sd650_power_initialized)
        return EAR_ERROR;


	/* Given we are reading every 2 secs, we compute the current power */
	ulong curr_time;
	ulong current_energy;
	state_t st;

    opendev(&sd650_context_for_main);
    st = sd650_power_reading(&sd650_context_for_main, &out);
    closedev(&sd650_context_for_main);
    if (ear_trylock(&sd650_power_lock) != EAR_SUCCESS) {
        debug("Error because of lock");
        return EAR_ERROR;
    }
    *penergy_mj = accum_sd650_energy;
    *time_ms    = last_time;

    if (st == EAR_ERROR) {
        if (valid_power) {
            /* Using last value */
            curr_time       = (ulong) timestamp_getconvert(TIME_MSECS);
            current_elapsed = overflow_zeros_u64(curr_time, last_time);
            current_energy  = current_elapsed * (last_power_value.node_power + last_power_value.gpu_power);
            accum_sd650_energy += current_energy;
            *penergy_mj = accum_sd650_energy;
            *time_ms    = curr_time;
            debug("Estimated: AVG power in elapsed %lu msec (curr %lu last %lu) is %lu Curr energy %lu Total energy "
                  "%lu\n",
                  current_elapsed, curr_time, last_time, out.node_power + out.gpu_power, current_energy,
                  accum_sd650_energy);
            last_time = curr_time;
        }

        ear_unlock(&sd650_power_lock);
        return (valid_power ? EAR_SUCCESS : EAR_ERROR);
    }

    /* At this point the power is supposed to be valid */
    curr_time       = (ulong) timestamp_getconvert(TIME_MSECS);
    current_elapsed = overflow_zeros_u64(curr_time, last_time);
    if (current_elapsed == 0) {
        ear_unlock(&sd650_power_lock);
        return EAR_SUCCESS;
    }

    current_energy = current_elapsed * (out.node_power + out.gpu_power);
    /* Energy is reported in MJ */
    accum_sd650_energy += current_energy;
    debug("Estimated: AVG power in elapsed %lu msec (curr %lu last %lu) is %lu Curr energy %lu Total energy %lu\n",
          current_elapsed, curr_time, last_time, out.node_power + out.gpu_power, current_energy, accum_sd650_energy);
    last_time = curr_time;

    memcpy(&last_power_value, &out, sizeof(struct sd650_node_power));

        current_energy = (current_elapsed/1000) * (out.node_power +  out.gpu_power);
        /* Energy is reported in MJ */
        accum_sd650_energy += (current_energy*1000);
        last_time = curr_time;
        memcpy(&last_power_value,&out,sizeof(struct sd650_node_power));

	*penergy_mj = accum_sd650_energy;
	*time_ms    = curr_time;

	ear_unlock(&sd650_power_lock);

	debug("SD650_power: energy_read energy %lu time %lu", *penergy_mj, *time_ms);



	return EAR_SUCCESS;

}

state_t energy_dc_read(void *c, edata_t energy_mj)
{
    ulong my_time;
    debug("energy_dc_read");
    return energy_dc_time_read(c, energy_mj, &my_time);
}

state_t energy_ac_read(void *c, edata_t energy_mj)
{
    ulong *penergy_mj = (ulong *) energy_mj;

    if (!penergy_mj)
        return EAR_ERROR;
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
    if (!units)
        return EAR_ERROR;

    *units = 1000;
    return EAR_SUCCESS;
}
state_t energy_accumulated(unsigned long *e,edata_t init,edata_t end)
{
    ulong *pinit = (ulong *) init, *pend = (ulong *) end;
    if (!pinit || !pend || !e)
        return EAR_ERROR;

  unsigned long total = diff_node_energy(*pinit,*pend);
  *e = total;
  return EAR_SUCCESS;
}
state_t energy_to_str(char *str,edata_t e)
{
    ulong *pe = (ulong *) e;
    if (!str || !pe)
        return EAR_ERROR;
    sprintf(str, "%lu", *pe);
    return EAR_SUCCESS;
}

uint energy_data_is_null(edata_t e)  
{
    ulong *pe = (ulong *) e;
    if (!pe)
        return 1;
    return (*pe == 0);
}
