/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H
/* clang-format off */

#include <common/types.h>
#include <common/system/poll.h>

// The Plugin Manager is a framework that loads dynamic libraries whose function
// names are known (declr_ macros), and calls them periodically. Also, it
// provides a simple API to interact with them.
//
// Example of running this simple binary:
//     int main(int argc, char *argv[]) {
//         plugin_manager_main(argc, argv);
//         plugin_manager_wait();
//         return 0;
//     }
//
// And can be launched with:
//     ./binary --paths=./path/to/plugins --plugins=metrics.so:4000+metrics_ui.so:4000
//
// Take a look to the EAR Wiki for more information about the defined functions
// of the plugins.


// Universal Plugin function declarations
#define declr_up_get_tag()               void up_get_tag(cchar **tag, cchar **tags_deps)
#define declr_up_action_init(suffix)     char *up_action_init##suffix(cchar *tag, void **data_alloc, void *data)
#define declr_up_action_periodic(suffix) char *up_action_periodic##suffix(cchar *tag, void *data)
#define declr_up_action_close()          char *up_action_close()
#define declr_up_message_receive(suffix) char *up_message_receive##suffix(cchar *tag_msg, void *data)
#define declr_up_poll_attend(suffix)     char *up_poll_attend##suffix(cchar *tag_fd, int fd)

// Helpers
#define is_tag(t)                        (strcmp(t, tag) == 0)
#define ARG0(var)                        (var != NULL)
#define ARG1(var)                        ARG0(var) && (var[1] != NULL)
#define ARG2(var)                        ARG1(var) && (var[2] != NULL)
#define ARGE(...)                        __VA_ARGS__
#define ARG(v, n)                        ARGE(ARG##n)(v)

// The main function of the Plugin Manager. It inits a thread.
int plugin_manager_main(int argc, char *argv[]);
// The main function but passing the array of files and paths.
int plugin_manager_main2(char *files, char *paths);
// Closes Plugin Manager main thread.
void plugin_manager_exit();
// Wait until Plugin Manager main thread exits.
void plugin_manager_wait();
// Asking for an action. Intended to be called from plugins.
void *plugin_manager_action_trigger(cchar *tag);
// Passing data to plugins. Intended to be called outside plugins.
void plugin_manager_message_send(cchar *tag_msg, void *data);
// Add a fd to the Plugin Manager internal poll/select(). Argument 'tag' is the
// own plugin tag and 'tag_fd' is the tag associated to the registered 'fd'.
int plugin_manager_poll_add(cchar *tag, cchar *tag_fd, int fd);
// Remove a file descriptor from the PM main poll/select().
void plugin_manager_poll_remove(int fd);

/* clang-format on */
#endif // PLUGIN_MANAGER_H
