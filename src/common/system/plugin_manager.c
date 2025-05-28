/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <common/utils/args.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/utils/strscreen.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>
#include <common/system/plugin_manager.h>

#define MAX_SYMBOLS 4
#define MAX_PLUGINS 128
#define PM_DEFAULT_VERB 2

typedef void  (get_tag_f)         (cchar **tag, cchar **tags_deps);
typedef char *(action_init_f)     (cchar *tag, void **data_alloc, void *data);
typedef char *(action_periodic_f) (cchar *tag, void *data);
typedef char *(post_data_f)       (cchar *msg, void *data);
typedef char *(fds_set_f)         (afd_set_t *set);

static ulong monitor_period = 100;
static ulong relax_period = 100;

// Example of dependency table:
// plugin0 [0] [1] [0]: It means that for plugin0, the plugin1 is mandatory
// plugin1 [0] [0] [1]: It means that for plugin1, the plugin2 is mandatory
// plugin2 [0] [0] [0]

typedef struct plugin_s {
    char               path[4096];
    char              *file_name;
    void              *handler;
    cchar             *tag;
    cchar             *tags_deps;
    char               conf[512];
    uint               deps_table[MAX_PLUGINS]; // Not sorted, table of dependecy modes
    action_init_f     *action_init[MAX_PLUGINS];
    action_periodic_f *action_periodic[MAX_PLUGINS];
    fds_set_f         *fds_register;
    fds_set_f         *fds_attend;
    post_data_f       *post_data;
    void              *data;
    void              *data_conf;
    ullong             time_accum;
    ullong             time_lapse; // In ms
    timestamp_t        time_origin;
    uint               time_param; // Time in parameter
    uint               one_time; // Obsolete?
    uint               priority;
    uint               is_opened;
    uint               is_enabled;
    uint               is_paused;
    uint               is_silenced;
} plugin_t;

typedef struct plugins_s {
    plugin_t **plugins_sorted;
    plugin_t  *plugins;
    uint       max_priority;
    uint       count;
} plugins_t;

static plugins_t   p;
static char      **priority_paths;
static char        date[128];
static uint        exit_called;
// static int         fds_status;
static afd_set_t   fds_active;
// static ullong      timeout;
// static ullong      timeout_remaining;

static int plugins_dependencies_read(cchar *string, ullong father_time_lapse, uint priority, uint *father_deps_table)
{
    uint   list1_count;
    uint   list2_count;
    char **list1;
    char **list2;
    int    priority_plus;
    int    time_inherit;
    int    mandatory;
    int    l, j;
    char  *c;

    // Get list of plugins to load and times
    debug("plugin string: %s", string);
    if (strtoa(string, '+', &list1, &list1_count) == NULL) {
        return 0;
    }
    // Allocating p
    debug("detected plugins: #%d", list1_count);
    // Filling p (i string index, j plugins index)
    for (l = 0; l < list1_count; ++l) {
        debug("list1[%d]: %s", l, list1[l]);
        // Cleaning
        memset(&p.plugins[p.count], 0, sizeof(plugin_t));
        list2         = NULL;
        list2_count   = 0;
        priority_plus = 0;
        time_inherit  = 0;
        mandatory     = 0;
        // Configuration = file:time:conf1:conf2
        if (strtoa(list1[l], ':', &list2, &list2_count) != NULL) {
            for (j = 1; list2[j] != NULL; ++j) {
                adebug("[Debug] %s: arg[%d] %s", list2[0], j, list2[j]);
            }
            // If time is valid
            if (list2_count > 1 && strlen(list2[1]) > 0 && strisnum(list2[1])) {
                p.plugins[p.count].time_lapse = (ullong) atoi(list2[1]);
                p.plugins[p.count].time_param = p.plugins[p.count].time_lapse > 0;
                // If configuration exists
                if (list2_count > 2) {
                    //strncpy(p.plugins[p.count].conf, list2[2], sizeof(p.plugins[p.count].conf));
                    //p.plugins[p.count].data_conf = (void *) p.plugins[p.count].conf;
                    p.plugins[p.count].data_conf = (void *) &list2[2];
                }
            } else {
                // Ok, time is not valid, so this is configuration then
                if (list2_count > 1 && strlen(list2[1]) > 0) {
                    p.plugins[p.count].data_conf = (void *) &list2[1];
                } else {
                    // Or maybe not, if list2[1] is empty then the configuration is this
                    if (list2_count > 2) {
                        p.plugins[p.count].data_conf = (void *) &list2[2];
                    }
                }
            }
            c = list2[0];
        } else {
            c = list1[l];
        }
        // Parsing the file
        while (*c == '!' || *c == '<' || *c == '^' || *c == '-') {
            if (*c == '!') {
                mandatory = 1;
            }
            if (*c == '<') {
                time_inherit = 1;
            }
            if (*c == '^') {
               priority_plus = MAX_PLUGINS;
            }
            if (*c == '-') {
                p.plugins[p.count].is_silenced = 1;
            }
            ++c;
        }
        // Checking if the file have the .so extension
        if (strcmp(&c[strlen(c)-3], ".so")) {
            sprintf(p.plugins[p.count].path, "%s.so", c);
        } else {
            sprintf(p.plugins[p.count].path, "%s", c);
        }
        // If you free this list, the plugin can't get the configuration
        //if (list2 != NULL) {
            //strtoa_free(list2);
        //}
        // Getting the plugin file_name for printing and compare purposes
        for (j = strlen(p.plugins[p.count].path)-2; j >= 0; --j) {
            p.plugins[p.count].file_name = &p.plugins[p.count].path[j];
            if (p.plugins[p.count].path[j] == '/') {
                p.plugins[p.count].file_name = &p.plugins[p.count].path[j+1];
                break;
            }
        }
        // Priority
        p.plugins[p.count].priority = priority+(list1_count-l)+priority_plus;
        if (p.plugins[p.count].priority > p.max_priority) {
            p.max_priority = p.plugins[p.count].priority;
        }
        // Time lapse
        if (!p.plugins[p.count].time_param && time_inherit && father_time_lapse > 0) {
            p.plugins[p.count].time_lapse = father_time_lapse;
        }
        // Looking this new plugin in the already processed plugins
        for (j = 0; j < p.count; ++j) {
            // If found
            if (strcmp(p.plugins[p.count].file_name, p.plugins[j].file_name) == 0) {
                // Replacing priority if has increased its value
                if (p.plugins[j].priority < p.plugins[p.count].priority) {
                    p.plugins[j].priority = p.plugins[p.count].priority;
                }
                // Replacing timelapse
                if (!p.plugins[j].time_param && time_inherit && p.plugins[j].time_lapse > father_time_lapse && father_time_lapse != 0) {
                    p.plugins[j].time_lapse = father_time_lapse;
                }
                // Replacing if the plugin is silenced
                if (p.plugins[p.count].is_silenced) {
                    p.plugins[j].is_silenced = 1;
                }
                // Replacing dependency type
                if (father_deps_table != NULL) {
                    father_deps_table[j] = (uint) mandatory;
                }
                break;
            }
        }
        // If found, passing to next dependency
        if (j < p.count) {
            continue;
        }
        // If not found, update the father dependency table
        if (father_deps_table != NULL) {
            father_deps_table[p.count] = mandatory;
        }
        // Not found, so this is a new plugin
        p.count += 1;
    }
    strtoa_free(list1);

    return 1;
}

static void plugins_open_sort(uint priority, uint index)
{
    int i;
    // Base case
    if (priority == 0) {
        return;
    }
    for (i = 0; i < p.count; ++i) {
        if (priority == p.plugins[i].priority) {
            adebug("[Debug] %s: time lapse '%4llu', time_param %u, priority '%u'",
                    p.plugins[i].path, p.plugins[i].time_lapse, p.plugins[i].time_param,
                    p.plugins[i].priority);
            p.plugins_sorted[index++] = &p.plugins[i];
        }
    }
    plugins_open_sort(priority-1, index);
}

static state_t plugins_open_so(char *path, void **handler)
{
    if (access(path, X_OK) != 0) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    // Lazy binding allows to load plugins thar require symbols of other plugins
    // that haven't been loaded yet.
    debug("Loding %s", path);
    if ((*handler = dlopen(path, RTLD_LAZY | RTLD_GLOBAL)) == NULL) {
        return_msg(EAR_ERROR, dlerror());
    }
    return EAR_SUCCESS;
}

static state_t plugins_open_file(int i)
{
    char new_path[8192];
    char *e = NULL;
    int j = 0;

    // Priority paths
    while(priority_paths != NULL && priority_paths[j] != NULL) {
        sprintf(new_path, "%s/%s", priority_paths[j], p.plugins[i].path);
        if (state_ok(plugins_open_so(new_path, (void **) &p.plugins[i].handler))) {
            verbose(4, "[Configuration] Trying to open: %s... OK", new_path);
            return EAR_SUCCESS;
        }
        verbose(4, "[Configuration] Trying to open: %s... FAILED (%s)", new_path, state_msg);
        ++j;
    }
    // Written paths
    if (state_ok(plugins_open_so(p.plugins[i].path, (void **) &p.plugins[i].handler))) {
        verbose(4, "[Configuration] Trying to open: %s... OK", p.plugins[i].path);
        return EAR_SUCCESS;
    }
    verbose(4, "[Configuration] Trying to open: %s... FAILED", p.plugins[i].path);
    // Environment paths
    if((e = ear_getenv(ENV_PATH_EAR)) != NULL) {
        sprintf(new_path, "%s/lib/plugins/monitoring/%s", e, p.plugins[i].path);
        if (state_ok(plugins_open_so(new_path, (void **) &p.plugins[i].handler))) {
            verbose(4, "[Configuration] Trying to open: %s... OK", new_path);
            return EAR_SUCCESS;
        }
        verbose(4, "[Configuration] Trying to open: %s... FAILED", new_path);
    }
    return EAR_ERROR;
}

static int plugins_open()
{
    plugin_t *plug;
    get_tag_f *get_tag;
    int enabled_count = 0;
    int i, j;

    for (i = 0; i < p.count; ++i) {
        if (state_fail(plugins_open_file(i))) {
            continue;
        }
        plug = &p.plugins[i];
        if ((get_tag = dlsym(plug->handler, "up_get_tag")) == NULL) {
            debug("[DEBUG] %s get_tag() not detected", plug->path);
            continue;
        }
        // Maybe we can explore in the future a multi tag system
        get_tag(&plug->tag, &plug->tags_deps);
        debug("[DEBUG] %s after call get_tag(): tag '%s', tags_deps '%s'",
            plug->path, (char *) plug->tag, (char *) plug->tags_deps);
        plug->is_opened = 1;
        // Loading dependencies
        if (plug->tags_deps != NULL) {
            plugins_dependencies_read(plug->tags_deps, plug->time_lapse, plug->priority, plug->deps_table);
        }
    }
    // Testing if dependencies are at least opened.
    for (i = 0; i < p.count; ++i) {
        plug = &p.plugins[i];
        for (j = 0; j < p.count; ++j) {
            // If mandatory but the dependency wasn't opened
            if (plug->deps_table[j] && !p.plugins[j].is_opened) {
                break;
            }
        }
        // Enabling if all the dependenciesare opened.
        plug->is_enabled = plug->is_opened && (j == p.count);
        verbose(4, "[Summary] %s %s: called every %06llu ms with priority %u%s",
                (plug->is_opened)? "Opened": "Not opened", plug->file_name, plug->time_lapse,
                plug->priority, (plug->is_enabled)? "": ", but disabled");
        enabled_count += plug->is_enabled;
    }
    return enabled_count;
}

static void output_print_format()
{
    time_t ti = time(NULL);
    struct tm tm = *localtime(&ti);
    sprintf(date, "%d/%d %d:%d:%d", tm.tm_mday, tm.tm_mon+1, tm.tm_hour, tm.tm_min, tm.tm_sec);
    date[strlen(date)] = '\0';
}

static int output_print(int i, int d, char *prefix, char *output)
{
    if (output == NULL) {
        return 1;
    }
    if (p.plugins_sorted[i]->is_silenced) {
        return 1;
    }
    // Returning instructions:
    //  [=] Pause
    //  [D] Disable (Pending dependency test)
    //  [X] Exit (Pending)
    if (output != NULL) {
        if (strstr(output, "[=]") != NULL) {
            p.plugins_sorted[i]->is_paused = 1;
            output += 3;
        } else if (strstr(output, "[D]") != NULL) {
            p.plugins_sorted[i]->is_enabled = 0;
            output += 3;
        } else if (strstr(output, "[X]") != NULL) {
            exit_called = 1;
        }
    }
    // Cleaning spaces
    while (*output == ' ') {
        ++output;
    }
    if (strlen(output) > 0) {
        verbose(0, "[ %s, %21s ] %s%s", date, p.plugins_sorted[i]->file_name, prefix, output);
    }
    if (exit_called) {
        return 0;
    }
    return 1;
}

static int plugins_action_periodic_call(int i, int d, void *data)
{
    char buffer[256];
    char *sout;

    if (p.plugins_sorted[i]->action_periodic[d] == (void *) UINT64_MAX) {
        return 0;
    }
    if (p.plugins_sorted[i]->action_periodic[d] == NULL) {
        sprintf(buffer, "up_action_periodic_%s", p.plugins_sorted[d]->tag);
        p.plugins_sorted[i]->action_periodic[d] = dlsym(p.plugins_sorted[i]->handler, buffer);
        // If NULL persist
        if (p.plugins_sorted[i]->action_periodic[d] == NULL) {
            p.plugins_sorted[i]->action_periodic[d] = dlsym(p.plugins_sorted[i]->handler, "up_action_periodic");
        }
        // If is still NULL, this address is unreachable
        if (p.plugins_sorted[i]->action_periodic[d] == NULL) {
            p.plugins_sorted[i]->action_periodic[d] = (void *) UINT64_MAX;
            return 0;
        }
    }
    sout = p.plugins_sorted[i]->action_periodic[d](p.plugins_sorted[d]->tag, data);
    return output_print(i, d, "", sout);
}

static void plugins_action_periodic(int i, timestamp_t *time_now, ullong time_passed)
{
    uint time_isup;
    int j;

    // Base case
    if (i == p.count) {
        return;
    }
    if (exit_called) {
        return;
    }
    // If is disabled, next
    if (!p.plugins_sorted[i]->is_enabled) {
        goto next_ap;
    }
    if (p.plugins_sorted[i]->is_paused) {
        goto next_ap;
    }
    // Time calculation
    p.plugins_sorted[i]->time_accum += time_passed;
    time_isup = p.plugins_sorted[i]->time_accum >= p.plugins_sorted[i]->time_lapse;

    if (time_isup) {
        p.plugins_sorted[i]->time_origin = *time_now;
        p.plugins_sorted[i]->time_accum  = p.plugins_sorted[i]->time_accum - p.plugins_sorted[i]->time_lapse;
    }
    debug("[DEBUG] %19s P%d-%u: lapse %llu ms, passed %llu ms, accum %llu ms, time's up %d, data %d",
          p.plugins_sorted[i]->file_name, i, p.plugins_sorted[i]->priority,
          p.plugins_sorted[i]->time_lapse, time_passed,
          p.plugins_sorted[i]->time_accum, time_isup,
          p.plugins_sorted[i]->data != NULL);
    // If the plugin has data allocated to work with
    if (time_isup) {
        if (!plugins_action_periodic_call(i, i, p.plugins_sorted[i]->data)) {
            goto next_ap;
        }
        // Feeding others with own data
        for (j = 0; p.plugins_sorted[i]->data != NULL && j < p.count; ++j) {
            if (j == i || !p.plugins_sorted[j]->is_enabled) {
                continue;
            }
            plugins_action_periodic_call(j, i, p.plugins_sorted[i]->data);
        }
    }
next_ap:
    // I think this is OK because the priority system moves the dependencies calls
    // before the own call. I can interpret that a data from a dependency coming
    // before means that data is required to perform the dependent main function,
    // which makes sense.
    plugins_action_periodic(i+1, time_now, time_passed);
}

#if 0
static void plugins_fds_attend(int i)
{
    char *sout;
    // Base case
    if (i == p.count) {
        return;
    }
    if (p.plugins_sorted[i]->fds_attend == (void *) UINT64_MAX) {
        goto next_fa;
    }
    p.plugins_sorted[i]->fds_attend = dlsym(p.plugins_sorted[i]->handler, "up_fds_attend");
    if (p.plugins_sorted[i]->fds_attend != NULL) {
        sout = p.plugins_sorted[i]->fds_attend(&fds_active);
        output_print(i, i, "FDs attend status: ", sout);
    } else {
        p.plugins_sorted[i]->fds_attend = (void *) UINT64_MAX;
    }
next_fa:
    plugins_fds_attend(i+1);
}
#endif

static state_t plugins_main(void *whatever)
{
    static timestamp_t time_now;
    static timestamp_t time_last;
    static ullong time_passed = ULLONG_MAX;
    // First time, just to start counting time
    if (time_passed == ULLONG_MAX) {
        timestamp_get(&time_last);
        time_passed = 0;
        return EAR_SUCCESS;
    }
    // Formatting the date
    output_print_format();
    // Time calculation
    timestamp_get(&time_now);
    time_passed = timestamp_diff(&time_now, &time_last, TIME_MSECS);
    // Main call
    plugins_action_periodic(0, &time_now, time_passed);
    if (exit_called) {
        monitor_dispose();
    }
    //
    time_last = time_now;
    return EAR_SUCCESS;
}

static int plugins_action_init_call(int i, int d, void **data_alloc, void *data)
{
    char buffer[256];
    char *sout;

    if (p.plugins_sorted[i]->action_init[d] == (void *) UINT64_MAX) {
        return 0;
    }
    if (p.plugins_sorted[i]->action_init[d] == NULL) {
        sprintf(buffer, "up_action_init_%s", p.plugins_sorted[d]->tag);
        p.plugins_sorted[i]->action_init[d] = dlsym(p.plugins_sorted[i]->handler, buffer);
        // If NULL persist
        if (p.plugins_sorted[i]->action_init[d] == NULL) {
            debug("[DEBUG] %s: function '%s'... NOT DETECTED", p.plugins_sorted[i]->file_name, buffer)
            sprintf(buffer, "up_action_init");
            p.plugins_sorted[i]->action_init[d] = dlsym(p.plugins_sorted[i]->handler, buffer);
        }
        // If is still NULL, this address is unreachable
        if (p.plugins_sorted[i]->action_init[d] == NULL) {
            debug("[DEBUG] %s: function '%s'... NOT DETECTED", p.plugins_sorted[i]->file_name, buffer)
            p.plugins_sorted[i]->action_init[d] = (void *) UINT64_MAX;
            return 0;
        }
        debug("[DEBUG] %s: function '%s' DETECTED", p.plugins_sorted[i]->file_name, buffer)
    }
    debug("[DEBUG] %s: called 'action_init' with tag/data '%s'", p.plugins_sorted[i]->file_name, p.plugins_sorted[d]->tag)
    sout = p.plugins_sorted[i]->action_init[d](p.plugins_sorted[d]->tag, data_alloc, data);
    return output_print(i, d, "Init status: ", sout);
}

static void plugins_action_init(int i)
{
    int j;
    // Base case
    if (i == p.count) {
        return;
    }
    // For the first plugin, the output have to be formatted
    if (i == 0) {
        output_print_format();
    }
    // If is disabled, next
    if (!p.plugins_sorted[i]->is_enabled) {
        goto next_ai;
    }
    // If don't have time, then is paused (just inits, nothing more)
    if (p.plugins[i].time_lapse == 0LLU) {
        p.plugins[i].is_paused = 1;
    }
    p.plugins_sorted[i]->data = NULL;
    if (!plugins_action_init_call(i, i, &p.plugins_sorted[i]->data, p.plugins_sorted[i]->data_conf)) {
        goto next_ai;
    }
    // Feeding others with own data (if it has)
    for (j = 0; p.plugins_sorted[i]->data != NULL && j < p.count; ++j) {
        if (j == i || !p.plugins_sorted[j]->is_enabled) {
            continue;
        }
        // Calling plugin J with plugin I tag/data
        plugins_action_init_call(j, i, NULL, p.plugins_sorted[i]->data);
    }
next_ai:
    plugins_action_init(i+1);
}

static void plugins_fds_register(int i)
{
    char *sout;
    // Base case
    if (i == p.count) {
        return;
    }
    p.plugins_sorted[i]->fds_register = dlsym(p.plugins_sorted[i]->handler, "up_fds_register");
    if (p.plugins_sorted[i]->fds_register != NULL) {
        sout = p.plugins_sorted[i]->fds_register(&fds_active);
        output_print(i, i, "FDs register status: ", sout);
    }
    plugins_fds_register(i+1);
}

static state_t plugins_init(void *whatever)
{
    int enabled = 0;
    if ((enabled = plugins_open())) {
        plugins_open_sort(p.max_priority, 0);
        plugins_action_init(0);
        plugins_fds_register(0);
    }
    if (!enabled) {
        monitor_dispose();
    }
    return EAR_SUCCESS;
}

static int plugin_manager_configure(int argc, char *argv[])
{
    char buffer[8192];
    char params[128];
    char verb_lvl[2] = "\0";

    if (argc == 1 || args_get(argc, argv, "help", buffer)) {
        verbose(0, "Usage: %s [OPTIONS]\n", argv[0]);
        verbose(0, "Options:");
        verbose(0, "    --plugins    List of plus sign separated plugins to load.");
        verbose(0, "    --paths      List of colon separated priority paths to search plugins.");
        verbose(0, "    --verbose    Show how the things are going internally.");
        verbose(0, "    --silence    Hide messages returned by plugins.");
        verbose(0, "    --monitor    Period at which the plugin wake ups for monitoring. Def=100 ms  ");
        verbose(0, "    --relax      Period to be used during low monitoring periods. Def=100 ms  ");
        verbose(0, "    --help       If you see it you already typed --help.");
        return 0;
    }

    // Processing list of priority paths
    if (args_get(argc, argv, "paths", buffer)) {
        // NUll terminated list, count is not required
        strtoa(buffer, ':', &priority_paths, NULL);
    }
    // Processing list of plugins (by now is hardcoded)
    if (!args_get(argc, argv, "plugins", buffer)) {
        return 0;
    }
    if (args_get(argc, argv, "verbose", verb_lvl)) {
        if (verb_lvl[0] == '\0')
            VERB_SET_LV(PM_DEFAULT_VERB)
        else
            VERB_SET_LV(atoi(verb_lvl))
    }
    if (args_get(argc, argv, "silence", NULL)) {
        VERB_SET_EN(0);
    }
    // Buffer cannot be used becasue it has to be parsed later
    if (args_get(argc, argv, "monitor", params)) {
	    monitor_period = strtoul(params, NULL, 10);
    }
    // Buffer cannot be used becasue it has to be parsed later
    if (args_get(argc, argv, "relax", params)) {
	    relax_period = strtoul(params, NULL, 10);
    }
    verbose(0,"UPM configuration: plugins \'%s\' verbose %u monitor periods[burst %lu, relax %lu]", buffer, VERB_GET_LV(), monitor_period, relax_period);
    if (args_get(argc, argv, "debug", NULL)) {
        ADEBUG_SET_EN(1);
    }
    p.plugins = calloc(MAX_PLUGINS, sizeof(plugin_t));
    p.plugins_sorted = calloc(MAX_PLUGINS, sizeof(plugin_t *));
    return plugins_dependencies_read(buffer, 0LLU, 0, NULL);
}

// Header functions
int plugin_manager_main(int argc, char *argv[])
{
    // Read configuration
    if (!plugin_manager_configure(argc, argv)) {
        verbose(0, "[ERROR] No plugins to load.")
        return 0;
    }
    #if 0
    afd_init(&fds_active);
    plugins_init(NULL);
    // 100 milliseconds
    timeout = 100LLU;
    while (!exit_called) {
        if ((fds_status = aselect(&fds_active, timeout, &timeout_remaining)) != -1) {
            if (fds_status > 0) {
                printf("fds_status > 0\n");
                plugins_fds_attend(0);
                timeout = timeout_remaining;
            } else if (fds_status == 0) {
                printf("fds_status == 0\n");
                plugins_main(NULL);
                timeout = 100LLU;
            }
        }
    }
    #else
    // Suscriptions
    suscription_t *sus = suscription();
    sus->call_init     = plugins_init;
    sus->call_main     = plugins_main;
    sus->time_relax    = relax_period;
    sus->time_burst    = monitor_period;
    sus->suscribe(sus);
    // Monitoring metrics
    debug("Initializing monitor");
    if (state_fail(monitor_init())) {
        verbose(0, "Monitor failed: %s", state_msg);
    }
    #endif

    return 1;
}

int plugin_manager_init(char *plugins, char *paths)
{
    char p0[1024];
    char p1[1024];
    char p2[1024];
    char *argv[3];
    argv[0] = p0;
    argv[1] = p1;
    argv[2] = p2;
    sprintf(p0, "./bin");
    sprintf(p1, "--plugins=%s", plugins);
    sprintf(p2, "--paths=%s", paths);

    verbose(0,"UPM main: plugin list \'%s\' paths \'%s\'", plugins, paths);
    return plugin_manager_main((paths != NULL) ? 3 : 2, argv);
}

void plugin_manager_close()
{
    exit_called = 1;
}

void plugin_manager_wait()
{
    monitor_wait();
}

void *plugin_manager_action(cchar *tag)
{
    int i;
    for (i = 0; i < p.count; ++i) {
        if (is_tag(p.plugins_sorted[i]->tag)) {
            if (plugins_action_periodic_call(i, i, p.plugins_sorted[i]->data)) {
                return p.plugins_sorted[i]->data;
            }
        }
    }
    return NULL;
}

void plugin_mananger_post(cchar *msg, void *data)
{
    int i;
    for (i = 0; i < p.count; ++i) {
        if (p.plugins_sorted[i]->post_data == (void *) UINT64_MAX) {
            continue;
        }
        if (p.plugins_sorted[i]->post_data == NULL) {
            if ((p.plugins_sorted[i]->post_data = dlsym(p.plugins_sorted[i]->handler, "up_post_data")) == NULL) {
                p.plugins_sorted[i]->post_data = (void *) UINT64_MAX;
                continue;
            }
        }
        p.plugins_sorted[i]->post_data(msg, data);
    }
}
