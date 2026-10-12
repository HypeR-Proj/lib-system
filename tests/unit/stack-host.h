/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_STACK_HOST_H
#define HYPER_STACK_HOST_H
#include <stdint.h>
#include <unistd.h>
#include <hyper/native.h>
#undef HYPER_NATIVE_PAGE_SIZE
#define HYPER_NATIVE_PAGE_SIZE ((size_t)sysconf(_SC_PAGESIZE))
#endif
