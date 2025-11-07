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

#include <common/environment_common.h>
#include <common/hardware/bithack.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <metrics/common/hsmp_esmi.h>

#if defined __has_include
#if __has_include(<e_smi/e_smi.h>)
#define HAS_E_SMI_H 1
#include <e_smi/e_smi.h>
#endif
#endif

#ifndef HAS_E_SMI_H
#define ESMI_SUCCESS 0

typedef int esmi_status_t;

struct smu_fw_version {
    uchar debug;
    uchar minor;
    uchar major;
    uchar unused;
};

struct ddr_bw_metrics {
    uint max_bw;
    uint utilized_bw;
    uint utilized_pct;
};
#endif

typedef struct esmi_s {
    esmi_status_t (*init)(void);
    esmi_status_t (*exit)(void);
    char *(*get_err_msg)(esmi_status_t esmi_err);
    esmi_status_t (*smu_fw_version_get)(struct smu_fw_version *smu_fw);             // HSMP_GET_SMU_VER
    esmi_status_t (*proto_ver_get)(uint *proto_ver);                                // HSMP_GET_PROTO_VER
    esmi_status_t (*socket_power_get)(uint socket_idx, uint *ppower);               // HSMP_GET_SOCKET_POWER
    esmi_status_t (*socket_power_cap_set)(uint socket_idx, uint pcap);              // HSMP_SET_SOCKET_POWER_LIMIT
    esmi_status_t (*socket_power_cap_get)(uint socket_idx, uint *pcap);             // HSMP_GET_SOCKET_POWER_LIMIT
    esmi_status_t (*socket_power_cap_max_get)(uint socket_idx, uint *pmax);         // HSMP_GET_SOCKET_POWER_LIMIT_MAX
    esmi_status_t (*core_boostlimit_set)(uint cpu_ind, uint boostlimit);            // HSMP_SET_BOOST_LIMIT
    esmi_status_t (*socket_boostlimit_set)(uint socket_idx, uint boostlimit);       // HSMP_SET_BOOST_LIMIT_SOCKET
    esmi_status_t (*core_boostlimit_get)(uint cpu_ind, uint *pboostlimit);          // HSMP_GET_BOOST_LIMIT
    esmi_status_t (*prochot_status_get)(uint socket_idx, uint *prochot);            // HSMP_GET_PROC_HOT
    esmi_status_t (*xgmi_width_set)(uchar min, uchar max);                          // HSMP_SET_XGMI_LINK_WIDTH
    esmi_status_t (*apb_disable)(uint sock_ind, uchar pstate);                      // HSMP_SET_DF_PSTATE
    esmi_status_t (*apb_enable)(uint sock_ind);                                     // HSMP_SET_AUTO_DF_PSTATE
    esmi_status_t (*fclk_mclk_get)(uint socket_idx, uint *fclk, uint *mclk);        // HSMP_GET_FCLK_MCLK
    esmi_status_t (*cclk_limit_get)(uint socket_idx, uint *cclk);                   // HSMP_GET_CCLK_THROTTLE_LIMIT
    esmi_status_t (*socket_c0_residency_get)(uint socket_idx, uint *pc0_residency); // HSMP_GET_C0_PERCENT
    esmi_status_t (*socket_lclk_dpm_level_set)(uint sock_ind, uchar nbio_id, uchar min,
                                               uchar max);      // HSMP_SET_NBIO_DPM_LEVEL
    esmi_status_t (*ddr_bw_get)(struct ddr_bw_metrics *ddr_bw); // HSMP_GET_DDR_BANDWIDTH
} esmi_t;

static const char *esmi_names[] = {"esmi_init",
                                   "esmi_exit",
                                   "esmi_get_err_msg",
                                   "esmi_smu_fw_version_get",
                                   "esmi_hsmp_proto_ver_get",
                                   "esmi_socket_power_get",
                                   "esmi_socket_power_cap_set",
                                   "esmi_socket_power_cap_get",
                                   "esmi_socket_power_cap_max_get",
                                   "esmi_core_boostlimit_set",
                                   "esmi_socket_boostlimit_set",
                                   "esmi_core_boostlimit_get",
                                   "esmi_prochot_status_get",
                                   "esmi_xgmi_width_set",
                                   "esmi_apb_disable",
                                   "esmi_apb_enable",
                                   "esmi_fclk_mclk_get",
                                   "esmi_cclk_limit_get",
                                   "esmi_socket_c0_residency_get",
                                   "esmi_socket_lclk_dpm_level_set",
                                   "esmi_ddr_bw_get"};

#define ESMI_N 21

static esmi_t esmi;
static topology_t tp;

static state_t translate(esmi_status_t s)
{
    if (s == ESMI_SUCCESS) {
        return EAR_SUCCESS;
    }
    return_msg(EAR_ERROR, esmi.get_err_msg(s));
}

static state_t hsmp_load_library()
{
#define load_library(path)                                                                                             \
    debug("Openning %s", (char *) path);                                                                               \
    if (state_ok(plug_open(path, (void **) &esmi, esmi_names, ESMI_N, RTLD_NOW | RTLD_LOCAL))) {                       \
        return EAR_SUCCESS;                                                                                            \
    }

    load_library(ear_getenv("HACK_HSMP_LIBRARY"));
    load_library("/usr/lib64/libe_smi64.so");
    load_library("/usr/lib64/libe_smi64.so.1");
    load_library("/usr/lib/libe_smi64.so");
    load_library("/usr/lib/libe_smi64.so.1");

    return_msg(EAR_ERROR, "Can not load libe_smi.so");
}

state_t hsmp_esmi_open(topology_t *tp_in, mode_t mode)
{
    struct smu_fw_version smu;
    state_t s = EAR_SUCCESS;

    // This is because ESMI library can't perform writing actions without root
    // permissions. ESMI does not provide any function to perform that test,
    // then if write permissions are required we return error.
    if (mode == O_RDWR && (getuid() != 0)) {
        return_msg(EAR_ERROR, Generr.no_permissions);
    }
    if (state_fail(s = hsmp_load_library())) {
        return s;
    }
    if (state_ok(s = translate(esmi.init()))) {
        if (state_ok(s = translate(esmi.smu_fw_version_get(&smu)))) {
            topology_copy(&tp, tp_in);
        }
    }
    return s;
}

state_t hsmp_esmi_close()
{
    if (esmi.exit != NULL) {
        return translate(esmi.exit());
    }
    return EAR_SUCCESS;
}

static uint apicid_to_cpuid(uint apicid)
{
    uint c;
    for (c = 0; c < tp.cpu_count; ++c) {
        if (tp.cpus[c].apicid == apicid) {
            debug("Found APICID %u in CPU %u", apicid, c);
            return c;
        }
    }
    return 0;
}

state_t hsmp_esmi_send(int socket, uint function, uint *args, uint *reps)
{
    struct ddr_bw_metrics ddr;
    esmi_status_t s;

    if (esmi.smu_fw_version_get == NULL) {
        return_msg(EAR_ERROR, Generr.api_uninitialized);
    }
    // Switch case
    switch (function) {
        case HSMP_TEST:
            // If HSMP initialized then this test is correct
            reps[0] = args[0] + 1;
            return EAR_SUCCESS;
        case HSMP_GET_SMU_VER:
            return translate(esmi.smu_fw_version_get((struct smu_fw_version *) &reps[0]));
        case HSMP_GET_PROTO_VER:
            return translate(esmi.proto_ver_get(&reps[0]));
        case HSMP_GET_SOCKET_POWER:
            return translate(esmi.socket_power_get(socket, &reps[0]));
        case HSMP_SET_SOCKET_POWER_LIMIT:
            return translate(esmi.socket_power_cap_set(socket, args[0]));
        case HSMP_GET_SOCKET_POWER_LIMIT:
            return translate(esmi.socket_power_cap_get(socket, &reps[0]));
        case HSMP_GET_SOCKET_POWER_LIMIT_MAX:
            return translate(esmi.socket_power_cap_max_get(socket, &reps[0]));
        case HSMP_SET_BOOST_LIMIT:
            // Function core_boostlimit_set receives the CPU number, not the APICID
            return translate(
                esmi.core_boostlimit_set(apicid_to_cpuid(getbits32(args[0], 31, 16)), getbits32(args[0], 15, 0)));
        case HSMP_SET_BOOST_LIMIT_SOCKET:
            return translate(esmi.socket_boostlimit_set(socket, args[0]));
        case HSMP_GET_BOOST_LIMIT:
            reps[0] = 0U;
            // Function core_boostlimit_get receives the CPU number, not the APICID
            return translate(esmi.core_boostlimit_get(apicid_to_cpuid(args[0]), &reps[0]));
        case HSMP_GET_PROC_HOT:
            return translate(esmi.prochot_status_get(socket, &reps[0]));
        case HSMP_SET_XGMI_LINK_WIDTH:
            return translate(esmi.xgmi_width_set((uchar) getbits32(args[0], 15, 8), (uchar) getbits32(args[0], 7, 0)));
        case HSMP_SET_DF_PSTATE:
            return translate(esmi.apb_disable(socket, (uchar) args[0]));
        case HSMP_SET_AUTO_DF_PSTATE:
            return translate(esmi.apb_enable(socket));
        case HSMP_GET_FCLK_MCLK:
            return translate(esmi.fclk_mclk_get(socket, &reps[0], &reps[1]));
        case HSMP_GET_CCLK_THROTTLE_LIMIT:
            return translate(esmi.cclk_limit_get(socket, &reps[0]));
        case HSMP_GET_C0_PERCENT:
            return translate(esmi.socket_c0_residency_get(socket, &reps[0]));
        case HSMP_SET_NBIO_DPM_LEVEL:
            return translate(esmi.socket_lclk_dpm_level_set(socket, (uchar) getbits32(args[0], 24, 16),
                                                            (uchar) getbits32(args[0], 7, 0),
                                                            (uchar) getbits32(args[0], 15, 8)));
        case HSMP_GET_DDR_BANDWIDTH:
            memset(&ddr, 0, sizeof(struct ddr_bw_metrics));
            s       = esmi.ddr_bw_get(&ddr);
            reps[0] = setbits32(reps[0], ddr.max_bw, 31, 20);
            reps[0] = setbits32(reps[0], ddr.utilized_bw, 19, 8);
            reps[0] = setbits32(reps[0], ddr.utilized_pct, 7, 0);
            return translate(s);
    }
    return_msg(EAR_ERROR, "HSMP function not found");
}
