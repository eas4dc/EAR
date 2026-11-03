/*
 * fake_powermon.h -- a controllable stand-in for daemon/power_monitor.c.
 *
 * The powercap code reads four things from the power monitor: the ear.conf
 * default and maximum node powercap, the current node power, and the running
 * application contexts (to push pc_opt into their settings and to read the
 * frequencies they asked for). It also reports events. This fake exposes each
 * of those as plain globals the tests set and inspect.
 */
#ifndef FAKE_POWERMON_H
#define FAKE_POWERMON_H

#include <common/config.h>
#include <daemon/power_monitor.h>
#include <daemon/shared_configuration.h>

#define FAKE_MAX_CTX    8
#define FAKE_MAX_EVENTS 256

typedef struct fake_event {
    uint type;
    llong value;
} fake_event_t;

typedef struct fake_ctx {
    int active;
    powermon_app_t app;
    settings_conf_t settings;
    resched_t resched;
    app_mgt_t app_info;
    pc_app_info_t pc_app_info;
} fake_ctx_t;

/* Knobs */
extern uint fake_pc_def;       /* powermon_get_baseline_powercap()      (ear.conf node powercap) */
extern uint fake_min_working;
extern uint fake_pc_max;       /* powermon_get_max_powercap_def()  (ear.conf max powercap)  */
extern uint fake_node_power;   /* powermon_current_power()                                  */
extern fake_ctx_t fake_ctx[FAKE_MAX_CTX];

/* Observations */
extern fake_event_t fake_events[FAKE_MAX_EVENTS];
extern int fake_num_events;

void fake_powermon_reset(void);
/* Activates context slot cc as an application requesting cpu_khz on every CPU
 * and gpu_khz on the first num_gpus GPUs. */
void fake_ctx_start(int cc, ulong cpu_khz, ulong gpu_mhz, int num_gpus);
void fake_ctx_stop(int cc);
/* Last event of a given type, or NULL */
fake_event_t *fake_last_event(uint type);
int fake_count_events(uint type);

#endif
