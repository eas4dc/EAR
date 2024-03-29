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

#include <stdlib.h>
#include <metrics/common/popen.h>
#include <metrics/gpu/archs/dcgmi.h>

static char command[1024];
static uint events_count  = 0;
static uint gpus_count    = 0;
//static uint *ev_list;

typedef struct dcgmi_ev_conf{
  char group;
  uint field;
  }dcgmi_ev_conf_t;
 
/* These data could be gather running dcgmi profile -l */ 
/* ordered based on ev number */
/* From DCGMI doc: A metric from each letter group can be collected without multiplexing. 
 * A metric from A.1 can be collected with another metric from A.1 without multiplexing. 
 * A metric from A.1 will be multiplexed with another metric from A.2 or A.3. 
 * Metrics from different letter groups can be combined for concurrent collection (without multiplexing) */

/*
 * dcgmi profile -l
 * +----------------+----------+------------------------------------------------------+
| Group.Subgroup | Field ID | Field Tag                                            |
+----------------+----------+------------------------------------------------------+
| A.1            | 1002     | sm_active                                            |
| A.1            | 1003     | sm_occupancy                                         |
| A.1            | 1004     | tensor_active                                        |
| A.1            | 1006     | fp64_active                                          |
| A.2            | 1008     | fp16_active                                          |
| A.2            | 1013     | tensor_imma_active                                   |
| A.2            | 1014     | tensor_hmma_active                                   |
| A.3            | 1007     | fp32_active                                          |
| B.0            | 1005     | dram_active                                          |
| C.0            | 1009     | pcie_tx_bytes                                        |
| C.0            | 1010     | pcie_rx_bytes                                        |
| D.0            | 1001     | gr_engine_active                                     |
| E.0            | 1011     | nvlink_tx_bytes                                      |
| E.0            | 1012     | nvlink_rx_bytes                                      |
+----------------+----------+------------------------------------------------------+
*/

#if 0
static dcgmi_ev_conf_t dcgmi_ev_confs[dcgmi_num_events]={
  {'D',0},
  {'A',1},
  {'A',1},
  {'A',1},
  {'B',0},
  {'A',1},
  {'A',3},
  {'A',2},
  {'C',0},
  {'C',0},
  {'E',0},
  {'E',0},
  {'A',2},
  {'A',2}
};
#endif

static uint dcgmi_ev_type[dcgmi_num_events]={RATIO,RATIO,RATIO,RATIO,RATIO,RATIO,RATIO,RATIO,ACCUM, ACCUM,ACCUM,ACCUM,RATIO,RATIO};

static char *dcgmi_ev_names[dcgmi_num_events]={
  "gr_engine_active", /* 1001 */
  "sm_active",
  "sm_occupancy",
  "tensor_active",
  "dram_active",      /* 1005 */
  "fp64_active",
  "fp32_active",
  "fp16_active",
  "pcie_tx_bytes",
  "pcie_rx_bytes",    /* 1010 */
  "nvlink_tx_bytes",
  "nvlink_rx_bytes",
  "tensor_imma_active",
  "tensor_hmma_active" /* 1014 */
};

state_t gpu_dcgmi_init(char *events, uint events_count_in)
{
    popen_t p;
    state_t s;

    if (events == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    }
    if (state_fail(s = popen_test("dcgmi"))) {
        debug("Failed to test: %s", state_msg);
        return s;
    }
    // Getting the number of GPUS
    if (state_fail(s = popen_open("dcgmi discovery -l", 0, 1, &p))) {
        debug("Failed to open: %s", state_msg);
        return s;
    }
    popen_read(&p, "ia", &gpus_count);
    popen_close(&p);
    debug("gpus_count: %u", gpus_count);
    if (gpus_count == 0) {
        return_msg(EAR_ERROR, "No GPUs detected");
    }
    // Formatting command
    sprintf(command, "dcgmi dmon -e %s -c 1", events);
    events_count = events_count_in;
    return EAR_SUCCESS;
}

state_t gpu_dcgmi_read(dcgmi_t *d)
{
    double values[15];
    popen_t p;
    state_t s;
    int gpu;

    if ((gpus_count == 0) || (events_count == 0) || (d == NULL)) {
        return_msg(EAR_ERROR, Generr.input_null);
    }
    if (state_fail(s = popen_open(command, 2, 1, &p))) {
        return s;
    }
    while(popen_read(&p, "aiD", &gpu, values)) {
        memcpy(d[gpu].values, values, sizeof(double)*events_count);
        if (events_count >= 2){ 
          debug("GPU%d: %0.2lf %0.2lf\n", gpu, values[0], values[1]);
        }
    }
    popen_close(&p);
    return EAR_SUCCESS;
}

void gpu_dcgmi_data_alloc(dcgmi_t **d)
{
    debug("gpu_dcgmi_data_alloc");
    if (d == NULL) {
        return;
    }
    *d = NULL;
    if (gpus_count){
        *d = calloc(gpus_count, sizeof(dcgmi_t));
    }
}

void gpu_dcgmi_set_events(char *events, uint events_count_in)
{
    debug("gpu_dcgmi_set_events");
    // Formatting command
    sprintf(command, "dcgmi dmon -e %s -c 1", events);
    events_count = events_count_in;
}

void gpu_dcgmi_data_copy(dcgmi_t *dest, dcgmi_t *src)
{
  if ((dest == NULL) || (src == NULL)) return;
  memcpy(dest, src, sizeof(dcgmi_t)*gpus_count);
}

void gpu_dcgmi_data_str(dcgmi_t *d, char *d_str, uint *ev_list, uint num_ev)
{
    uint g, e;
    char ev_str[64];

    debug("gpu_dcgmi_data_str");
    if ((d == NULL) || (d_str == NULL) || (num_ev == 0)) return;

  d_str[0] = '\0';
  for (g = 0; g < gpus_count; g++){
    snprintf(ev_str, sizeof(ev_str), "\nGPU[%u] ", g);
    strcat(d_str, ev_str);
    for (e = 0; e < num_ev; e++){
      if (e) strcat(d_str,",");
      snprintf(ev_str, sizeof(ev_str), "ev[%s] = %lf " , dcgmi_ev_names[ev_list[e] - dcgmi_first_event], d[g].values[e]);
      strcat(d_str, ev_str);
    }
  }
}

void gpu_dcgmi_events_to_str(uint *dcgmi_events, uint num_ev,char * dcgmi_events_str)
{
  uint i;
  char ev_str[64];
  debug("gpu_dcgmi_events_to_str");
  if ((dcgmi_events == NULL) || (dcgmi_events_str == NULL)) return;

  dcgmi_events_str[0] = '\0';
  for (i = 0; i < num_ev; i++){
    if (i) strcat(dcgmi_events_str, ",");
    snprintf(ev_str, 64, "%u", dcgmi_events[i]);
    strcat(dcgmi_events_str,ev_str);
  }
}

void gpu_dcgmi_accum_data(dcgmi_t *accum, dcgmi_t *d, double elapsed, uint *ev_list, uint num_ev)
{
  debug("gpu_dcgmi_accum_data");
  if ((accum == NULL) || (d == NULL) || (events_count == 0) || (gpus_count == 0)) return;
  for (uint g = 0; g < gpus_count; g++){
    for (uint e = 0; e < num_ev; e++){
      if (dcgmi_ev_type[ev_list[e] - dcgmi_first_event] == RATIO){ 
        if (g == 0){ 
          debug("Event %s type RATIO", dcgmi_ev_names[ev_list[e] - dcgmi_first_event]);
        }
        accum[g].values[e] += d[g].values[e] * elapsed;
      }else{
        if (g == 0){ 
          debug("Event %s type ACUM", dcgmi_ev_names[ev_list[e] - dcgmi_first_event]);
        }
        accum[g].values[e] += d[g].values[e];
      }
    }
  }
}

char *gpu_dcgmi_event_to_name(uint ev)
{
  return dcgmi_ev_names[ev -dcgmi_first_event];
}

uint gpu_dcgmi_event_type(uint event)
{
  return dcgmi_ev_type[event - dcgmi_first_event];
}

uint gpu_dcgmi_gpus_detected()
{
  return gpus_count;
}

void gpu_dcgmi_data_avg(dcgmi_t *accum, double elapsed, uint *ev_list, uint num_ev )
{
  debug("gpu_dcgmi_data_avg elapsed %.3lf", elapsed);
  if ((accum == NULL) || (elapsed == 0) || (events_count == 0) || (gpus_count == 0)) return;
  for (uint g = 0; g < gpus_count; g++){
    for (uint e = 0; e < num_ev; e++){
      if (accum[g].values[e]){
        accum[g].values[e] = accum[g].values[e] / elapsed;
      }
    }
  }
}

#if TEST
int main(int argc, char *argv[])
{
    dcgmi_t *d;
    gpu_dcgmi_init("1009,1010", 2);
    gpu_dcgmi_data_alloc(&d);
    gpu_dcgmi_read(d);
    return 0;
}
#endif
