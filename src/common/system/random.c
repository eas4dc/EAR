/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/random.h>
#include <common/output/debug.h>
#include <common/system/random.h>

// Compatibility for MSVC
#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

#define RND_BUFFER_SIZE 4096
static THREAD_LOCAL uint8_t buffer[RND_BUFFER_SIZE];
static THREAD_LOCAL size_t offset = RND_BUFFER_SIZE;

static int refill_buffer(void)
{
    size_t filled = 0;
    ssize_t ret = 0;

    while (filled < RND_BUFFER_SIZE) {
        if ((ret = getrandom(buffer + filled, RND_BUFFER_SIZE - filled, 0)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }
        filled += (size_t) ret;
    }
    offset = 0;
    return 1;
}

static uint64_t random_get()
{
    uint64_t value = 0U;
    if (offset >= RND_BUFFER_SIZE) {
        if (!refill_buffer()) {
            return (uint64_t) clock();
        }
    }
    memcpy(&value, buffer + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    return value;
}

uint64_t random_getrank64(uint64_t max, uint64_t min)
{
    uint64_t value = random_get();
    return min + (value % (max-min));
}