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
    *tag       = "sockets_receiver";
    *tags_deps = NULL;
}

#define macro_assert(f)      \
    if (state_fail(f)) {     \
        return "[X] Failed"; \
    }

declr_up_action_init(_sockets_receiver)
{
    macro_assert(sockets_init(&sock, NULL, 8686, TCP));
    macro_assert(sockets_socket(&sock));
    macro_assert(sockets_bind(&sock, 2));
    macro_assert(sockets_listen(&sock));
    plugin_manager_poll_add("sockets_receiver", "accept", sock.fd);
    return rsprintf("Listening socket %d", sock.fd);
}

declr_up_poll_attend(_accept)
{
    int fd_data;
    macro_assert(sockets_accept(fd, &fd_data, NULL));
    plugin_manager_poll_add("sockets_receiver", "data", fd_data);
    return rsprintf("Accepted socket %d from socket %d", fd_data, fd);
}

declr_up_poll_attend(_data)
{
    char message[128];
    size_t data_size;
    uint data_type;

    macro_assert(sockets_recv_header(fd, &data_type, &data_size, NULL, 1));
    macro_assert(sockets_recv(fd, message, data_size, 1));
    return rsprintf("Received message from socket %d '%s'", fd, message);
}
