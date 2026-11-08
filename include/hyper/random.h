/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_RANDOM_H
#define HYPER_RANDOM_H
#include <hyper/native.h>
#include <hyper/syscall.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define HYPER_RANDOM_CLIENT UINT32_C(0x800d0001)
#define HYPER_RANDOM_NONBLOCK UINT32_C(1)
#define HYPER_RANDOM_MAX_REQUEST 192

/* Uses an optional startup capability; no kernel random syscall or weak
 * fallback. Deadline is absolute monotonic nanoseconds, UINT64_MAX is infinite.
 * NONBLOCK skips entropy-readiness retries, but permits IPC transport waits.
 * Success value0 is the completed prefix, value1 is zero. An error after a
 * completed chunk returns successful short progress. Zero length needs no grant.
 */
hyper_call_result_t hyper_random_read(void *buffer, size_t length, uint32_t flags,
				      uint64_t deadline);
/* Success fills the whole buffer. Failure may leave a completed prefix. */
hyper_native_status_t hyper_random_fill(void *buffer, size_t length, uint64_t deadline);
#ifdef __cplusplus
}
#endif
#endif
