/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdlib.h>
#include <metrics/gpu/archs/gpuprof_dummy.h>

static gpuprof_evs_t events;

GPUPROF_F_LOAD(gpuprof_dummy_load)
{
    sprintf(events.name, "dummy");
    events.id = 1;

    apis_put(ops->init,             gpuprof_dummy_init);
    apis_put(ops->dispose,          gpuprof_dummy_dispose);
    apis_put(ops->get_info,         gpuprof_dummy_get_info);
    apis_put(ops->events_get,       gpuprof_dummy_events_get);
    apis_put(ops->events_set,       gpuprof_dummy_events_set);
    apis_put(ops->events_unset,     gpuprof_dummy_events_unset);
    apis_put(ops->read,             gpuprof_dummy_read);
    apis_put(ops->read_raw,         gpuprof_dummy_read_raw);
    apis_put(ops->data_diff,        gpuprof_dummy_data_diff);
    apis_put(ops->data_alloc,       gpuprof_dummy_data_alloc);
    apis_put(ops->data_copy,        gpuprof_dummy_data_copy);
}

GPUPROF_F_INIT(gpuprof_dummy_init)
{
    return EAR_SUCCESS;
}

GPUPROF_F_DISPOSE(gpuprof_dummy_dispose)
{
}

GPUPROF_F_GET_INFO(gpuprof_dummy_get_info)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_PERIPHERAL;
    info->devs_count  = 1;
}

GPUPROF_F_EVENTS_GET(gpuprof_dummy_events_get)
{
    *evs = &events;
    *evs_count = 1;
}

GPUPROF_F_EVENTS_SET(gpuprof_dummy_events_set)
{

}

GPUPROF_F_EVENTS_UNSET(gpuprof_dummy_events_unset)
{

}


GPUPROF_F_READ(gpuprof_dummy_read)
{
    return EAR_SUCCESS;
}

GPUPROF_F_READ_RAW(gpuprof_dummy_read_raw)
{
    return EAR_SUCCESS;
}

GPUPROF_F_DATA_DIFF(gpuprof_dummy_data_diff)
{

}

GPUPROF_F_DATA_ALLOC(gpuprof_dummy_data_alloc)
{
    gpuprof_t *aux;
    aux = calloc(1, sizeof(gpuprof_t));
    // We allocate all values contiguously
    aux[0].values = calloc(1, sizeof(double));
    // Setting pointers
    aux[0].samples_count = 1;
    aux[0].values_count = 1;
    *data = aux;
}

GPUPROF_F_DATA_COPY(gpuprof_dummy_data_copy)
{

}
