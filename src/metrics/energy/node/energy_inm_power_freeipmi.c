/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <pthread.h>
#include <limits.h> 
#include <pwd.h>
#include <freeipmi/freeipmi.h>

// #define SHOW_DEBUGS 1

#include <common/config.h>
#include <common/states.h>
#include <common/system/execute.h>
#include <common/math_operations.h>
#include <metrics/common/apis.h>
#include <metrics/accumulators/types.h>
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <common/hardware/hardware_info.h>
#include <common/system/monitor.h>

/* For error control */
#define MAX_TIMES_POWER_SUPPORTED 10
#define VPOWER_ERROR 0
static        unsigned int     nm_free_last_power_measurement = 0;

#define PACKAGE_NAME  "freeipmi"
#define FREEIPMI_CONFIG_DIRECTORY_MODE    0700
#define SDR_CACHE_DIR         "sdr-cache"
#define MAXHOSTNAMELEN 64
#define MAXPATHLEN 4096
#define POWER_PERIOD 2

#define RESTART_TH 1800
#define MAX_LOCK_TRIES 1000000000

static pthread_mutex_t node_energy_lock_nm = PTHREAD_MUTEX_INITIALIZER;
static ipmi_sdr_ctx_t  sdr_ctx;
static uint8_t         channel;
static uint8_t         slave;
static uint8_t         lun;
static uint            found = 0;
static suscription_t *inm_sus;
static ipmi_ctx_t     ipmi_ctx;
static time_t         last_timestamp;
static time_t         first_timestamp;
static uint           inm_last_power_reading;
static uint			  inm_timeframe;
static ulong 		  inm_accumulated_energy;
static uint			  inm_initialized = 0;
static unsigned int   workaround_flags = 0;
static int        sdr_done = 0;
static int 				monitor_done = 0;

state_t energy_dispose(void **c);
static state_t inm_power_reading(ipmi_ctx_t ipmi_ctx, uint *maxp, uint *minp, uint *avgp, uint *currentp, uint *periodp);
static state_t inm_thread_main(void *p);
static state_t inm_thread_init(void *p);


#define SDR_CACHE_FILENAME_PREFIX   "sdr-cache"

/* ipmi_sdr_cache_iterate
 *  * - iterate through all SDR records calling callback for each one.
 *   * - if callback returns < 0, that will break iteration and return
 *    *   value is returned here.
 *     */

static int intelnm_sdr_callback (ipmi_sdr_ctx_t sdr_ctx, uint8_t record_type,
								const void *sdr_record, unsigned int sdr_record_len, void *arg)
{ 
				int ret; 
				debug("Record type %d OEMO RECORD %d\n", record_type, IPMI_SDR_FORMAT_OEM_RECORD);
				if (record_type != IPMI_SDR_FORMAT_OEM_RECORD)
								return (0);

				debug("ipmi_sdr_oem_parse_intel_node_manager\n");
				/* return (1) - is oem intel node manager, fully parsed
				 *  * return (0) - is not oem intel node manager
				 *   * return (-1) - error
				 *    */

				if ((ret = ipmi_sdr_oem_parse_intel_node_manager (sdr_ctx,
																				sdr_record,
																				sdr_record_len,
																				&slave,
																				&lun,
																				&channel,
																				NULL,
																				NULL,
																				NULL,
																				NULL)) < 0)
				{ 
								return 0;
				}

				if (ret){
								debug("Intel NM record found\n");
								found = 1;
				}
				if (found) return -1;
				return 0;
}

static int sdr_get_home_directory ( char *buf, unsigned int buflen)
{
				uid_t user_id;
				struct passwd pwd,*pwdptr;
				long int tbuf_len;
				char *tbuf = NULL;


				tbuf_len = 4096;      /* XXX */

				if (!(tbuf = malloc (tbuf_len)))
				{
								debug("malloc error\n");
								return -1;
				}

				user_id = getuid ();
				debug("User id %d\n", user_id);
				memset (&pwd, '\0', sizeof (struct passwd));
				if (getpwuid_r (user_id, &pwd, tbuf, tbuf_len, &(pwdptr)) != 0)
				{
								free(tbuf);
								debug("getpwuid_r error\n");
								return -1;
				}
				debug("pw_dir %s\n", pwd.pw_dir); 

				// The string fields pointed to by the members of the passwd structure are stored in the buffer buf of size buflen
        //free(tbuf);

				if (pwd.pw_dir)
				{
								if (!access (pwd.pw_dir, R_OK|W_OK|X_OK))
								{
												if (strlen (pwd.pw_dir) > (buflen - 1))
												{
																debug("internal overflow error\n");
																free(tbuf);
																return -1;
												}

												strcpy (buf, pwd.pw_dir);
												free(tbuf);
												return 0;
								}
				}

				snprintf (buf, buflen, "/tmp/.%s-%s", PACKAGE_NAME, pwd.pw_name);
				// Releasing now
				free(tbuf);
				if (access (buf, R_OK|W_OK|X_OK) < 0)
				{
								if (errno == ENOENT)
								{
												if (mkdir (buf, FREEIPMI_CONFIG_DIRECTORY_MODE) < 0)
												{
																debug("Cannot make cache directory: %s: %s\n", buf, strerror (errno));
																return -1;
												}
								}
								else
								{
												debug(    "Cannot access cache directory: %s\n", buf);
												return -1;
								}
				}
				debug("Home dir is %s\n", buf);

				return (0);
}

static int sdr_get_config_directory ( const char *cache_dir, char *buf, unsigned int buflen)
{
				char tbuf[MAXPATHLEN+1];

				memset (tbuf, '\0', MAXPATHLEN+1);
				if (!cache_dir)
				{
								if (sdr_get_home_directory ( tbuf, MAXPATHLEN) < 0){
												debug("home directory error\n");
												return (-1);
								}
				}
				else
				{
								if (strlen (cache_dir) > (MAXPATHLEN - 1))
								{
												debug("internal overflow error\n");
												return (-1);
								}
								strcpy (tbuf, cache_dir);

								if (access (tbuf, R_OK|W_OK|X_OK) < 0)
								{
												if (errno == ENOENT)
												{
																if (mkdir (tbuf, FREEIPMI_CONFIG_DIRECTORY_MODE) < 0)
																{
																				debug("Cannot make cache directory: %s: %s\n", tbuf, strerror (errno));
																				return (-1);
																}
												}
												else
												{
																debug("Cannot access cache directory: %s\n", tbuf);
																return (-1);
												}
								}
				}
				debug("home directory is %s\n", tbuf);
				snprintf(buf, buflen, "%s/.%s", tbuf, PACKAGE_NAME);
				debug("sdr_get_config_directory success %s\n", buf);
				return (0);
}

static int sdr_cache_get_cache_directory ( const char *cache_dir, char *buf, unsigned int buflen)
{
    char tbuf[MAXPATHLEN+1];


    memset (tbuf, '\0', MAXPATHLEN+1);
    if (sdr_get_config_directory ( cache_dir,
                tbuf,
                MAXPATHLEN) < 0)
        return (-1);

    snprintf (buf, buflen, "%s/%s", tbuf, SDR_CACHE_DIR);
    debug("sdr_cache_get_cache_directory success %s\n", buf);

    return (0);
}

static int sdr_get_cache_filename ( const char *hostname, char *buf, unsigned int buflen)
{
    char sdrcachebuf[MAXPATHLEN+1];
    char *cache_directory  = 0;
    char *ptr;


    char hostnamebuf[MAXHOSTNAMELEN+1];

    memset (hostnamebuf, '\0', MAXHOSTNAMELEN+1);
    if (gethostname (hostnamebuf, MAXHOSTNAMELEN) < 0){
        snprintf (hostnamebuf, MAXHOSTNAMELEN, "localhost");
    }

    /* shorten hostname if necessary */
    if ((ptr = strchr (hostnamebuf, '.')))
        *ptr = '\0';

    if (sdr_cache_get_cache_directory (cache_directory,
                sdrcachebuf,
                MAXPATHLEN) < 0)
        return (-1);

    snprintf (buf, buflen, "%s/%s-%s.%s", sdrcachebuf, SDR_CACHE_FILENAME_PREFIX,
            hostnamebuf, hostname ? hostname : "localhost");

    debug("sdr name success %s\n", buf);
    return (0);
}

state_t energy_init(void **c)
{
    uid_t uid;
    int ret=0;
    char cachefilenamebuf[MAXPATHLEN+1];
    char *hostname = 0;
	int etries = 0, lret;

    if (inm_initialized) return EAR_SUCCESS;

    //
    // Creating the context
    //

    while((lret = pthread_mutex_trylock(&node_energy_lock_nm)) && (etries < MAX_LOCK_TRIES)) etries++;
		if ((etries == MAX_LOCK_TRIES) && lret){
			return EAR_ERROR;
		}
    debug("Create context\n");
    if (!( ipmi_ctx = ipmi_ctx_create ())) {
        pthread_mutex_unlock(&node_energy_lock_nm);
        error("Error in ipmi_ctx_create %s",strerror(errno));
        return EAR_ERROR;
    }
    // Checking for root
    uid = getuid ();
    if (uid != 0){ 
        pthread_mutex_unlock(&node_energy_lock_nm);
        energy_dispose(no_ctx);
        error("No root permissions");
        return EAR_ERROR;
    }
    // inband context
    debug("Inband context\n");
    if ((ret = ipmi_ctx_find_inband (ipmi_ctx,
                    NULL, // driver_type
                    0, //disable_auto_probe
                    0, // driver_address
                    0, // register_spacing
                    NULL, // driver_device
                    workaround_flags,
                    IPMI_FLAGS_DEFAULT)) < 0) {
        error("%s",ipmi_ctx_errormsg(ipmi_ctx));
        pthread_mutex_unlock(&node_energy_lock_nm);
        energy_dispose(no_ctx);
				
        return EAR_ERROR;    
    }
    if (ret==0) {
        error("Not inband device found %s",ipmi_ctx_errormsg(ipmi_ctx));
        pthread_mutex_unlock(&node_energy_lock_nm);
        energy_dispose(no_ctx);
        return EAR_ERROR;    
    }

		if (!sdr_done){

    	/* SDR context */
    	debug( "ipmi_sdr_ctx_create");
    	if (!(sdr_ctx = ipmi_sdr_ctx_create ()) ) {
        	error("INM SDR ctx create");
        	pthread_mutex_unlock(&node_energy_lock_nm);
        	energy_dispose(no_ctx);
        	return EAR_ERROR;
    	}
    	debug("Callback and sdr process\n");
	
    	/* Cache file name generation */
	
    	if (sdr_get_cache_filename(hostname, cachefilenamebuf, MAXPATHLEN) < 0)
        	debug("Error in sdr_get_cache_filename\n");
	
	
    	if (ipmi_sdr_cache_open(sdr_ctx, ipmi_ctx, cachefilenamebuf) < 0) {
		debug("Error ipmi_sdr_cache_open, creating it %s\n", cachefilenamebuf);
        	char buff[4096];
        	snprintf(buff,sizeof(buff), "%s/sbin/ipmi-oem intelnm get-node-manager-statistics mode=globalpower", FREEIPMI_BASE);
        	if (execute(buff) != EAR_SUCCESS){
							verbose(0,"SDR cache not initialized");
            	pthread_mutex_unlock(&node_energy_lock_nm);
            	energy_dispose(no_ctx);
            	return EAR_ERROR;
        	}
	
    	}

			debug("Opening again sdr ctx");
			if (ipmi_sdr_cache_open(sdr_ctx, ipmi_ctx, cachefilenamebuf) < 0) {
				debug("Error ipmi_sdr_cache_open, Warning , it will probbaly not work", ipmi_sdr_ctx_errormsg(sdr_ctx));
			}
	
	
    	debug("ipmi_sdr_cache_iterate\n");
    	if (ipmi_sdr_cache_iterate (sdr_ctx,
                	intelnm_sdr_callback,
                	NULL) < 0)
    	{
        	if (!found) {
		debug("Error ipmi_sdr_cache_iterate %s\n", ipmi_sdr_ctx_errormsg(sdr_ctx));
            	pthread_mutex_unlock(&node_energy_lock_nm);
            	energy_dispose(no_ctx);
            	return EAR_ERROR;
        	}
    	}
	

    	if (!found)
    	{  
        	debug("ARG not fount! using default\n");
        	slave = 0x2c;
        	lun = 0x00;
        	channel = 0x06;
        	pthread_mutex_unlock(&node_energy_lock_nm);
        	return EAR_SUCCESS;
	
    	}

    	/* From IPMI documentation */
    	slave <<= 1;

    	debug("after init channel %x slave %x lun %x \n", channel,  slave, lun);

    	ipmi_sdr_cache_close(sdr_ctx);
    	ipmi_sdr_ctx_destroy(sdr_ctx);
    	sdr_ctx = NULL;
			sdr_done = 1;
		}
    uint maxp, minp, avgp, currentp, periodp;
    state_t st;
    inm_initialized = 1;
    st = inm_power_reading(ipmi_ctx, &maxp, &minp, &avgp, &currentp, &periodp);
    pthread_mutex_unlock(&node_energy_lock_nm);
    if (st != EAR_SUCCESS) {
        debug("_power_reading fails");
        energy_dispose(no_ctx);
        return EAR_ERROR;
    } else {
        // Read the first read time
        first_timestamp = time(NULL);
        inm_last_power_reading = currentp;
				if (!monitor_done){
        	inm_sus = suscription();
        	inm_timeframe = POWER_PERIOD * 1000;
        	inm_sus->call_main = inm_thread_main;
        	inm_sus->call_init = inm_thread_init;
        	inm_sus->time_relax = inm_timeframe;
        	inm_sus->time_burst = inm_timeframe;
        	inm_sus->suscribe(inm_sus);
        	debug("inm energy plugin suscription initialized with timeframe %u ms",inm_timeframe);
					monitor_done = 1;
				}

    }
    return EAR_SUCCESS;    

}

static state_t inm_power_reading(ipmi_ctx_t ipmi_ctx, uint *maxp, uint *minp,
        uint *avgp, uint *currentp, uint *periodp)
{
#define destroy_and_return(st) \
  fiid_obj_destroy(obj_cmd_rs);\
  return st;

    uint8_t mode = IPMI_OEM_INTEL_NODE_MANAGER_STATISTICS_MODE_GLOBAL_POWER_STATISTICS;
    uint8_t domainid = IPMI_OEM_INTEL_NODE_MANAGER_DOMAIN_ID_ENTIRE_PLATFORM;
    uint8_t policyid = 0;
    uint64_t val;

    fiid_obj_t obj_cmd_rs = NULL;

		*currentp = 0;
		if (!inm_initialized)
		{
			debug("INM not initialized");
			return EAR_ERROR;
		}

    if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_oem_intel_node_manager_get_node_manager_statistics_rs)))
    {
        debug("fiid_obj_create: %s\n", strerror (errno));
        return EAR_ERROR;
    }

		debug("executing ipmi_cmd_oem_intel_node_manager_get_node_manager_statistics");
    if (ipmi_cmd_oem_intel_node_manager_get_node_manager_statistics (ipmi_ctx,
                channel,
                slave,
                lun,
                mode,
                domainid,
                policyid,
                obj_cmd_rs) < 0){
        debug("Error ipmi_cmd_oem_intel_node_manager_get_node_manager_statistics\n");
        destroy_and_return(EAR_ERROR);
    }
		debug("parsing information from ipmi_cmd_oem_intel_node_manager_get_node_manager_statistics");

    if (FIID_OBJ_GET (obj_cmd_rs,
                "average",
                &val) < 0)
    {
        debug("FIID_OBJ_GET: 'average': %s\n",
                fiid_obj_errormsg (obj_cmd_rs));
        destroy_and_return(EAR_ERROR);
    }

    *avgp = val; 
    //debug("Average power %d\n", val);
    if (FIID_OBJ_GET (obj_cmd_rs,
                "maximum",
                &val) < 0)
    {
        debug("FIID_OBJ_GET: 'maximum': %s\n",
                fiid_obj_errormsg (obj_cmd_rs));
        destroy_and_return(EAR_ERROR);
    }
    *maxp = val;
    //debug("Maximum power %d\n", val);
    if (FIID_OBJ_GET (obj_cmd_rs,
                "minimum",
                &val) < 0)
    {
        debug("FIID_OBJ_GET: 'minimum': %s\n",
                fiid_obj_errormsg (obj_cmd_rs));
        destroy_and_return(EAR_ERROR);
    }
    *minp = val;
    //debug("Minimum power %d\n", val);

    if (FIID_OBJ_GET (obj_cmd_rs,
                "current",
                &val) < 0)
    {
        debug("FIID_OBJ_GET: 'current power': %s\n",
                fiid_obj_errormsg (obj_cmd_rs));
        destroy_and_return(EAR_ERROR);
    }
    *currentp = val;
    if (FIID_OBJ_GET (obj_cmd_rs,
                "statistics_reporting_period",
                &val) < 0)
    {
        debug("FIID_OBJ_GET: 'statistics_reporting_period': %s\n",
                fiid_obj_errormsg (obj_cmd_rs));
        destroy_and_return(EAR_ERROR);
    }
    *periodp = val;
    //debug("Statistics reporting period %d\n", val);
    verbose(2,"power: Max power %u Min power %u Avg power %u Current power %u Period %u\n", *maxp, *minp, *avgp, *currentp, *periodp);

    /* For error control */
    if (*currentp > *maxp){
      verbose(VPOWER_ERROR,"enery_nm_freimpi: New max power reading current:%u max %u", *currentp, *maxp);
    }
    if ((*currentp < (*maxp * MAX_TIMES_POWER_SUPPORTED)) && ( *currentp > 0)){
      nm_free_last_power_measurement = *currentp;
    }else{
      verbose(VPOWER_ERROR, "enery_nm_freeipmi:warning, invalid power detected. current %u, corrected to last valid power %u", *currentp, nm_free_last_power_measurement);
       *currentp = nm_free_last_power_measurement;
    }

		destroy_and_return(EAR_SUCCESS);
}

/**** These functions are used to accumulate the energy */
static state_t inm_thread_main(void *p)
{
    uint maxp, minp, avgp, currentp, periodp;
    ulong curr_time_elapsed;
    ulong total_time_elapsed;
    ulong current_energy;
    state_t st;
		int etries = 0,lret;

		debug("inm_thread_main");
    //pthread_mutex_lock(&node_energy_lock_nm);
		while((lret = pthread_mutex_trylock(&node_energy_lock_nm)) && (etries < MAX_LOCK_TRIES)) etries++;
		if ((etries == MAX_LOCK_TRIES) && lret){
			return EAR_ERROR;
		}

    time_t curr_time = time(NULL);
    st = inm_power_reading(ipmi_ctx, &maxp, &minp, &avgp, &currentp, &periodp);
		if (st == EAR_ERROR){
			debug("inm_power_reading fails in inm_thread_main");
			pthread_mutex_unlock(&node_energy_lock_nm);
			return EAR_ERROR;
		}

    if (currentp > 0) {
        curr_time_elapsed = curr_time - last_timestamp;
        last_timestamp = curr_time;

        current_energy = curr_time_elapsed * currentp;
        /* Energy is reported in MJ */
        inm_accumulated_energy += (current_energy*1000);
    		pthread_mutex_unlock(&node_energy_lock_nm);

        debug("AVG power in last %lu s is %u", curr_time_elapsed, currentp);

        /*  We check if we must restart the connection */
        total_time_elapsed = curr_time - first_timestamp;
        debug("%sTotal time elapsed: %lu s%s", COL_BLU, total_time_elapsed, COL_CLR);
        if (total_time_elapsed > RESTART_TH) {
            debug("%sATTENTION%s A restart must be performed.", COL_RED, COL_CLR);

            /* ************************************************** */

            //pthread_mutex_lock(&node_energy_lock_nm);
            etries = 0;
						while((lret = pthread_mutex_trylock(&node_energy_lock_nm)) && (etries < MAX_LOCK_TRIES)) etries++;
						if ((etries == MAX_LOCK_TRIES) && lret){
							return EAR_ERROR;
						}
						
#if SHOW_DEBUGS
            time_t t_init = time(NULL); // Debug only purposes
#endif
            int ret = 0;
            // Close context
            ipmi_ctx_close (ipmi_ctx);
            // delete context
            ipmi_ctx_destroy (ipmi_ctx);
            ipmi_ctx = NULL;

            //
            // Creating the context
            //

            debug("Create context\n");
            if (!( ipmi_ctx = ipmi_ctx_create ())) {
								inm_initialized = 0;
                pthread_mutex_unlock(&node_energy_lock_nm);
                error("Error in ipmi_ctx_create %s",strerror(errno));
                return EAR_ERROR;
            }
            // inband context
            debug("Inband context\n");
            if ((ret = ipmi_ctx_find_inband (ipmi_ctx,
                            NULL, // driver_type
                            0, //disable_auto_probe
                            0, // driver_address
                            0, // register_spacing
                            NULL, // driver_device
                            workaround_flags,
                            IPMI_FLAGS_DEFAULT)) < 0) {
                error("%s",ipmi_ctx_errormsg(ipmi_ctx));
                pthread_mutex_unlock(&node_energy_lock_nm);
                energy_dispose(no_ctx);

                return EAR_ERROR;    
            }
            if (ret==0) {
                error("Not inband device found %s",ipmi_ctx_errormsg(ipmi_ctx));
                pthread_mutex_unlock(&node_energy_lock_nm);
                energy_dispose(no_ctx);
                return EAR_ERROR;    
            }

            pthread_mutex_unlock(&node_energy_lock_nm);
            first_timestamp = time(NULL);
#if SHOW_DEBUGS
            /*  Debug only */
            ulong overhead = first_timestamp - t_init;
            debug("%sNew overhead generated: %lu%s", COL_YLW, overhead, COL_CLR);
#endif
        }

    } else {
    		pthread_mutex_unlock(&node_energy_lock_nm);
        debug("Current power is 0 in inm power reading ");
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}

static state_t inm_thread_init(void *p)
{
    last_timestamp = time(NULL);
    return EAR_SUCCESS;
    }

/* Release access to ipmi device */
state_t energy_dispose(void **c) {
		int etries = 0, lret;
    if (!inm_initialized) return EAR_ERROR;

		while((lret = pthread_mutex_trylock(&node_energy_lock_nm)) && (etries < MAX_LOCK_TRIES)) etries++;
		if ((etries == MAX_LOCK_TRIES) && lret){
			return EAR_ERROR;
		}

		

    // // Close context
    ipmi_ctx_close (ipmi_ctx);
    // delete context
    ipmi_ctx_destroy (ipmi_ctx);
    ipmi_ctx = NULL;
		inm_initialized = 0;

    if (sdr_ctx == NULL){
				pthread_mutex_unlock(&node_energy_lock_nm);
        return EAR_SUCCESS;
    }
    ipmi_sdr_cache_close(sdr_ctx);
    ipmi_sdr_ctx_destroy(sdr_ctx);
    sdr_ctx = NULL;
		pthread_mutex_unlock(&node_energy_lock_nm);
    return EAR_SUCCESS;
}

state_t energy_datasize(size_t *size)
{ 
				debug("energy_datasize %lu\n",sizeof(unsigned long));
				*size=sizeof(unsigned long);
				return EAR_SUCCESS;
}

state_t energy_frequency(ulong *freq_us) {
				*freq_us = inm_timeframe;
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
				if (end >= init){
								ret = end-init;
				} else{
								ret = ulong_diff_overflow(init,end);
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

				debug("energy_dc_read\n");
				/* Given we are reading every 2 secs, we compute the current power */
    		uint maxp, minp, avgp, currentp, periodp;
    		ulong curr_time_elapsed;
    		ulong current_energy;
    		state_t st;
				int etries = 0, lret;

				*penergy_mj = inm_accumulated_energy;
    		//pthread_mutex_lock(&node_energy_lock_nm);
				while((lret = pthread_mutex_trylock(&node_energy_lock_nm)) && (etries < MAX_LOCK_TRIES)) etries++;
				if ((etries == MAX_LOCK_TRIES) && lret){
					return EAR_ERROR;
				}
				
    		time_t curr_time = time(NULL);
    		st = inm_power_reading(ipmi_ctx, &maxp, &minp, &avgp, &currentp, &periodp);

				if (st == EAR_ERROR){
					pthread_mutex_unlock(&node_energy_lock_nm);
					return EAR_ERROR;
				}
    		if (currentp > 0) {
        		curr_time_elapsed = curr_time - last_timestamp;
        		last_timestamp = curr_time;
        		current_energy = curr_time_elapsed * currentp;
        		/* Energy is reported in MJ */
        		inm_accumulated_energy += (current_energy*1000);
				}
        pthread_mutex_unlock(&node_energy_lock_nm);


				*penergy_mj= inm_accumulated_energy;

				return EAR_SUCCESS;
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms)
{
				ulong *penergy_mj=(ulong *)energy_mj;

				debug("energy_dc_read\n");
        /* Given we are reading every 2 secs, we compute the current power */
        uint maxp, minp, avgp, currentp, periodp;
        ulong curr_time_elapsed;
        ulong current_energy;
        state_t st;
				int etries = 0, lret;

				*penergy_mj = inm_accumulated_energy;
        //pthread_mutex_lock(&node_energy_lock_nm);
				while((lret = pthread_mutex_trylock(&node_energy_lock_nm)) && (etries < MAX_LOCK_TRIES)) etries++;
				if ((etries == MAX_LOCK_TRIES) && lret){
					return EAR_ERROR;
				}
        time_t curr_time = time(NULL);
        st = inm_power_reading(ipmi_ctx, &maxp, &minp, &avgp, &currentp, &periodp);

				if (st == EAR_ERROR){
					pthread_mutex_unlock(&node_energy_lock_nm);
          return EAR_ERROR;
        }

		
        if (currentp > 0) {
            curr_time_elapsed = curr_time - last_timestamp;
            last_timestamp = curr_time;
            current_energy = curr_time_elapsed * currentp;
            /* Energy is reported in MJ */
            inm_accumulated_energy += (current_energy*1000);
        }
        pthread_mutex_unlock(&node_energy_lock_nm);


				*penergy_mj = inm_accumulated_energy;
				*time_ms = curr_time*1000;

				return EAR_SUCCESS;

}
uint energy_data_is_null(edata_t e)
{
				ulong *pe=(ulong *)e;
				return (*pe == 0);

}


#if 0
void main(int argc , char *argv[])
{
				uint iters = 0;
				uint maxp, minp, avgp, periodp, currentp;
				node_energy_init();
				do{
								node_power_reading(&maxp, &minp, &avgp, &currentp, &periodp);
								debug("POWER READING: Max power %u Min power %u Avg power %u Current power %u Period %u\n", maxp, minp, avgp, currentp, periodp);
								sleep(10);
								iters++;
				}while(iters < 10);
				node_energy_dispose();

}
#endif

