/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <metrics/gpu/gpuproc.h>

static gpuproc_t *data1;
static gpuproc_t *data2;
static gpuproc_t *dataD;

static state_t update(void *x)
{
    static uint updated = 0;
    debug("update proc %u", getpid());

    gpuproc_read_diff(no_ctx, data2, data1, dataD); 

    gpuproc_data_print(dataD, fderr);

    if (updated == 4) {
        gpuproc_disable(no_ctx);
    }
    if (updated == 8) {
        gpuproc_enable(no_ctx);
    }
    updated++;
    debug("leaving proc %u", getpid());

    return EAR_SUCCESS;
}

void __main(int argc, char **argv, char **envp)
{
    static int protected = 0;
    uint api;
    
    if (protected) {
        return;
    }
    protected = 1;
    
    debug("__main");
    
    monitor_init();

    gpuproc_load(NO_EARD);
    gpuproc_get_api(&api);
    gpuproc_init(no_ctx);
    gpuproc_data_alloc(&data1);
    gpuproc_data_alloc(&data2);
    gpuproc_data_alloc(&dataD);
    gpuproc_read(no_ctx, data1); 
    apis_print(api, "API: ");
  
	suscription_t *sus = suscription();
	sus->call_init     = NULL;
	sus->call_main     = update;
	sus->time_relax    = 5000;
	sus->time_burst    = 5000;
	sus->suscribe(sus);
 
    return;
}

__attribute__((section(".init_array"))) typeof(__main) *__init = __main;
