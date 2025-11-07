/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EARL_VERBOSE_H
#define _EARL_VERBOSE_H

#include <common/output/debug.h>
#include <common/output/error.h>
#include <common/output/verbose.h>
#include <library/common/externs.h>

// #define VEARL_FATAL 0
#define VEARL_ERROR 1
#define VEARL_WARN  2
#define VEARL_INFO  1
#define VEARL_INFO2 2
#define VEARL_DEBUG 3

#define verbose_master(...)                                                                                            \
    if (masters_info.my_master_rank >= 0)                                                                              \
    verbose(__VA_ARGS__)
#define verbosen_master(...)                                                                                           \
    if (masters_info.my_master_rank >= 0)                                                                              \
    verbosen(__VA_ARGS__)

#if HIDE_ERRORS_LIB
#define error_lib(...)
#else
#define error_lib(...) error(__VA_ARGS__)
#endif // HIDE_ERRORS_LIB

#define verbose_info(msg, ...)                                                                                         \
    do {                                                                                                               \
        verbose(VEARL_INFO, COL_BLU "[%s][%d][INFO] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);              \
    } while (0);

#define verbose_info2(msg, ...)                                                                                        \
    do {                                                                                                               \
        verbose(VEARL_INFO2, COL_BLU "[%s][%d][INFO2] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);            \
    } while (0);

#define verbose_info3(msg, ...)                                                                                        \
    do {                                                                                                               \
        verbose(VEARL_DEBUG, COL_BLU "[%s][%d][DEBUG] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);            \
    } while (0);

#define verbose_error(msg, ...)                                                                                        \
    do {                                                                                                               \
        verbose(VEARL_ERROR, COL_RED "[%s][%d][ERROR] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);            \
    } while (0);

#define verbose_warning(msg, ...)                                                                                      \
    do {                                                                                                               \
        verbose(VEARL_WARN, COL_YLW "[%s][%d][WARNING] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);           \
    } while (0);

#define verbose_info_master(msg, ...)                                                                                  \
    do {                                                                                                               \
        verbose_master(VEARL_INFO, COL_BLU "[%s][%d][INFO] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);       \
    } while (0);

#define verbosen_info_master(msg, ...)                                                                                 \
    do {                                                                                                               \
        verbosen_master(VEARL_INFO, COL_BLU "[%s][%d][INFO] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);      \
    } while (0);

#define verbose_info2_master(msg, ...)                                                                                 \
    do {                                                                                                               \
        verbose_master(VEARL_INFO2, COL_BLU "[%s][%d][INFO2] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);     \
    } while (0);

#define verbose_info3_master(msg, ...)                                                                                 \
    do {                                                                                                               \
        verbose_master(VEARL_DEBUG, COL_BLU "[%s][%d][DEBUG] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);     \
    } while (0);

#define verbose_error_master(msg, ...)                                                                                 \
    do {                                                                                                               \
        verbose_master(VEARL_ERROR, COL_RED "[%s][%d][ERROR] " COL_CLR msg, node_name, my_node_id, ##__VA_ARGS__);     \
    } while (0);

#define verbose_warning_master(msg, ...)                                                                               \
    do {                                                                                                               \
        verbose_master(VEARL_WARN, COL_YLW "[%s][%d][%d][WARNING] " COL_CLR msg, node_name, my_node_id, getpid(),      \
                       ##__VA_ARGS__);                                                                                 \
    } while (0);

#endif // _EARL_VERBOSE_H
