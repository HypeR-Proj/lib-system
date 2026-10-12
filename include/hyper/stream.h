/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_STREAM_H
#define HYPER_STREAM_H
#include <hyper/native.h>
#include <stddef.h>
#include <stdint.h>

#define HYPER_STREAM_INPUT UINT32_C(0x80030011)
#define HYPER_STREAM_OUTPUT UINT32_C(0x80030012)
#define HYPER_STREAM_ERROR UINT32_C(0x80030013)
#define HYPER_STREAM_MAX_DATA 4096

hyper_native_handle_t hyper_runtime_stream_connector(uint32_t stream);
int64_t hyper_runtime_stream_read(void *, size_t, size_t *);
int64_t hyper_runtime_stream_write(uint32_t, const void *, size_t, size_t *);
int64_t hyper_runtime_stream_fence(uint32_t);
int hyper_runtime_stream_is_terminal(uint32_t);
int hyper_runtime_stdin_exact(void);
/* A single optional Native reader borrows the same process input connection
 * used by stdio. Ordinary reads fail BUSY until release; there is no second
 * input consumer or private prefetch queue. The wait handle is borrowed. */
int64_t hyper_runtime_stdin_acquire(uint64_t *);
int64_t hyper_runtime_stdin_begin(size_t, uint64_t);
/* kind=UINT32_MAX means no response arrived; completed READ errors set kind=0. */
int64_t hyper_runtime_stdin_poll(void *, size_t, size_t *, uint32_t *);
int64_t hyper_runtime_stdin_cancel(void);
void hyper_runtime_stdin_release(void);
#endif
