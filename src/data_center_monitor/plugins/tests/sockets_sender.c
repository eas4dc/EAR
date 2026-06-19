/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
// #define SHOW_DEBUGS 1
#include <stdio.h>
#include <common/output/debug.h>
#include <common/system/sockets.h>
#include <common/system/plugin_manager.h>

static socket_t sock;

declr_up_get_tag()
{
    *tag       = "sockets_sender";
    *tags_deps = "!sockets_receiver";
}

#define macro_assert(f)      \
    if (state_fail(f)) {     \
        return "[X] Failed"; \
    }

declr_up_action_init(_sockets_sender)
{
    macro_assert(sockets_init(&sock, "localhost", 8686, TCP));
    macro_assert(sockets_socket(&sock));
    macro_assert(sockets_connect(&sock));
    return rsprintf("Ready to send from socket %d", sock.fd);
}

declr_up_action_periodic(_sockets_sender)
{
    static char message[128] = {0};
    static int num = 0;
    sprintf(message, "message %d", num++);
    macro_assert(sockets_send(sock.fd, 0, message, strlen(message)+1, 0LLU));
    return rsprintf("Sent message from socket %d '%s'", sock.fd, message);
}
