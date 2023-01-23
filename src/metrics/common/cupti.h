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

#ifndef METRICS_COMMON_CUPTI_H
#define METRICS_COMMON_CUPTI_H

#ifdef CUPTI_BASE
#include <cupti.h>
#include <cupti_events.h>
#include <cupti_metrics.h>
#include <cupti_callbacks.h>
#include <cuda_runtime_api.h>
#endif
#include <common/types.h>
#include <common/states.h>

#ifndef CUPTI_BASE
#define CUDA_SUCCESS                                        0
#define CUPTI_SUCCESS                                       0
#define CUPTI_API_EXIT                                      0
#define CUPTI_EVENT_ATTR_NAME                               0
#define CUPTI_EVENT_READ_FLAG_NONE                          0
#define CUPTI_CB_DOMAIN_DRIVER_API                          0
#define CUPTI_EVENT_GROUP_ATTR_EVENTS                       0
#define CUPTI_EVENT_GROUP_ATTR_NUM_EVENTS                   0
#define CUPTI_EVENT_GROUP_ATTR_INSTANCE_COUNT               0
#define CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS              0
#define CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel              0
#define CUPTI_EVENT_GROUP_ATTR_PROFILE_ALL_DOMAIN_INSTANCES 0
#define CUPTI_METRIC_ATTR_VALUE_KIND                        0

typedef int CUresult;
typedef int CUdevice;
typedef int *CUcontext;
typedef int CUptiResult;
typedef int CUpti_CallbackId;
typedef void *CUpti_CallbackFunc;
typedef int CUpti_CallbackDomain;
typedef int CUpti_SubscriberHandle;
typedef int CUpti_EventID;
typedef int CUpti_MetricID;
typedef int CUpti_MetricValueKind;
typedef int CUpti_MetricAttribute;
typedef void *CUpti_EventGroup;
typedef int CUpti_EventCollectionMode;
typedef int CUpti_EventGroupAttribute;
typedef int CUpti_EventAttribute;
typedef int CUpti_ReadEventFlags;

typedef struct CUpti_EventGroupSet_s {
    CUpti_EventGroup *eventGroups;
    int numEventGroups;
} CUpti_EventGroupSet;

typedef struct CUpti_EventGroupSets_s {
    CUpti_EventGroupSet *sets;
    int numSets;
} CUpti_EventGroupSets;

typedef struct CUpti_CallbackData_s {
    char *functionName;
    CUcontext context;
    int callbackSite;
} CUpti_CallbackData;

typedef union {
  double metricValueDouble;
  ullong metricValueUint64;
  llong metricValueInt64;
  double metricValuePercent;
  ullong metricValueThroughput;
} CUpti_MetricValue;
#endif

typedef struct cuda_s {
    CUresult (*Init) (uint flags);
    CUresult (*CtxGetDevice) (CUdevice *device);
    CUresult (*DeviceGetCount) (int *count);
    CUresult (*GetErrorString) (CUresult error, const char **str);
} cuda_t;

typedef struct cupti_s {
    CUptiResult (*Subscribe) (CUpti_SubscriberHandle *sus, CUpti_CallbackFunc cb, void *data);
    CUptiResult (*EnableCallback) (uint enable, CUpti_SubscriberHandle sus, CUpti_CallbackDomain dom, CUpti_CallbackId id);
    CUptiResult (*EnableDomain) (uint enable, CUpti_SubscriberHandle subscriber, CUpti_CallbackDomain domain);
    CUptiResult (*MetricGetIdFromName) (CUdevice dev, const char *name, CUpti_MetricID *met);
    CUptiResult (*MetricGetNumEvents) (CUpti_MetricID met, uint *num);
    CUptiResult (*MetricCreateEventGroupSets) (CUcontext c, size_t size, CUpti_MetricID *ids, CUpti_EventGroupSets **passes);
    CUptiResult (*MetricGetAttribute) (CUpti_MetricID met, CUpti_MetricAttribute attrib, size_t *valueSize, void *value);
    CUptiResult (*MetricGetValue) (CUdevice dev, CUpti_MetricID met, size_t size1, CUpti_EventID* ids, size_t size2, uint64_t* values, uint64_t ns, CUpti_MetricValue* value);
    CUptiResult (*SetEventCollectionMode) (CUcontext context, CUpti_EventCollectionMode mode);
    CUptiResult (*EventGroupSetsDestroy) (CUpti_EventGroupSets *eventGroupSets);
    CUptiResult (*EventGroupSetAttribute) (CUpti_EventGroup group, CUpti_EventGroupAttribute attrib, size_t size, void *values);
    CUptiResult (*EventGroupGetAttribute) (CUpti_EventGroup group, CUpti_EventGroupAttribute attrib, size_t *size, void *values);
    CUptiResult (*EventGroupEnable) (CUpti_EventGroup group);
    CUptiResult (*EventGroupDisable) (CUpti_EventGroup group);
    CUptiResult (*EventGroupSetDisable) (CUpti_EventGroupSet *set);
    CUptiResult (*EventGroupReadAllEvents) (CUpti_EventGroup g, CUpti_ReadEventFlags f, size_t *b1, ullong *b2, size_t *sz2, CUpti_EventID *b3, size_t *n);
    CUptiResult (*EventGetAttribute) (CUpti_EventID event, CUpti_EventAttribute attrib, size_t *valueSize, void *value);
    CUptiResult (*EventGroupAddEvent) (CUpti_EventGroup g, CUpti_EventID e);
    CUptiResult (*EventGroupCreate) (CUcontext context, CUpti_EventGroup *g, uint32_t flags);
    CUptiResult (*GetResultString) (CUptiResult result, const char **str);
} cupti_t;

state_t cupti_open(cupti_t *cupti, cuda_t *cu);

state_t cupti_close();

void cupti_count_devices(uint *devs_count);

#endif //METRICS_COMMON_CUPTI_H
