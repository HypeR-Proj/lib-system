/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_LAUNCH_H
#define HYPER_LAUNCH_H

#include <hyper/native.h>
#include <stddef.h>
#include <stdint.h>

/* Userspace launch payload. Offsets are relative to the start of the payload;
 * each points to a NUL-terminated string. The kernel treats it as opaque bytes. */
typedef struct {
	uint32_t argument_count;
	uint32_t environment_count;
	/* uint32_t offsets[argument_count + environment_count], then strings. */
} hyper_launch_data_t;

/* SDK startup role for an explicitly delegated child loader File. */
#define HYPER_LAUNCH_CHILD_LOADER UINT32_C(0x80040003)

#define HYPER_LAUNCH_MAX_ARGUMENTS 64
#define HYPER_LAUNCH_MAX_ENVIRONMENT 64
#define HYPER_LAUNCH_MAX_STRING_BYTES 4096

/* Runtime-private auxv. These values describe loader-to-CRT handoff only. */
#define HYPER_AUXV_STARTUP_HANDLES UINT64_C(0x48590001)
#define HYPER_AUXV_STARTUP_HANDLE_COUNT UINT64_C(0x48590002)
#define HYPER_AUXV_INITIAL_STACK_BASE UINT64_C(0x48590003)
#define HYPER_AUXV_INITIAL_STACK_CAPACITY UINT64_C(0x48590004)
#define HYPER_AUXV_INITIAL_STACK_SIZE UINT64_C(0x48590005)
#define HYPER_AUXV_MAIN_STACK_SIZE UINT64_C(0x48590006)
#define HYPER_AUXV_LOADER_BASE UINT64_C(0x48590007)
#define HYPER_AUXV_LOADER_SIZE UINT64_C(0x48590008)
#define HYPER_AUXV_STARTUP_CHANNEL UINT64_C(0x48590009)

/* Exactly one result precedes constructors. A failure before this message is
 * also observable through the supervisor Process termination signal. */
typedef struct {
	int64_t status;
} hyper_launch_result_t;

/* Borrows both handles. On failure requests child stop and waits for exit. */
#ifdef __cplusplus
extern "C" {
#endif
hyper_native_status_t hyper_launch_wait(hyper_native_handle_t process,
					hyper_native_handle_t channel);
#ifdef __cplusplus
}
#endif

#endif
