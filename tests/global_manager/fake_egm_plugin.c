/*
 * fake_egm_plugin.c -- an EARGM powercap plugin that records its calls in the
 * test binary (plugin_record, resolved at load time). Built twice (PLUG_ID 0/1).
 */
#include <global_manager/plugins/powercap_plugin.h>

void plugin_record(int plugin, int fn);

state_t plugin_init(cluster_conf_t *conf)
{
    plugin_record(PLUG_ID, 0);
    return EAR_SUCCESS;
}

state_t plugin_hard_powercap_decision(powercap_status_t *status, powercap_opt_t *opt, bool *send_options,
                                      cluster_powercap_ctx_t *ctx)
{
    plugin_record(PLUG_ID, 1);
    /* the second plugin asks for idle power to be released */
    return PLUG_ID == 1 ? EAR_WARNING : EAR_SUCCESS;
}

state_t plugin_soft_powercap_decision(power_check_t *power)
{
    plugin_record(PLUG_ID, 2);
    return EAR_SUCCESS;
}
