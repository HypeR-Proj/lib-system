/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/std.h>
#include <hyper/stream.h>
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <stdatomic.h>
#include "mutex-internal.h"
#include <string.h>

/* Matches hyper-service::stdio startup purposes. A byte channel transports
 * whole messages, whereas Read consumes arbitrary prefixes: retain the rest
 * in one process-wide buffer, including across separately linked std shims. */
#define STDIO_INPUT UINT32_C(0x80030001)
static hyper_mutex_t input_lock;
static unsigned char pending[HYPER_NATIVE_BYTE_CHANNEL_MAX_MESSAGE_BYTES];
static size_t pending_start;
static size_t pending_end;

static hyper_native_handle_t stream_handle(uint32_t stream, int *console)
{
	hyper_native_handle_t handle = hyper_runtime_capability(STDIO_INPUT + stream);
	const hyper_startup_t *startup = hyper_runtime_startup();
	*console = 0;
	if (handle)
		return handle;
	if (startup == NULL)
		return 0;
	if (hyper_startup_find_handle(startup, STDIO_INPUT + stream, &handle) ==
	    HYPER_NATIVE_STATUS_OK)
		return handle;
	*console = 1;
	if (hyper_startup_find_handle(startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE,
				      &handle) == HYPER_NATIVE_STATUS_OK)
		return handle;
	return 0;
}

int64_t hyper_runtime_stdio_write(uint32_t stream, const void *buffer, size_t count, size_t *actual)
{
	*actual = 0;
	if (stream != 1 && stream != 2)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (count == 0)
		return HYPER_NATIVE_STATUS_OK;
	if (hyper_runtime_stream_connector(stream))
		return hyper_runtime_stream_write(stream, buffer, count, actual);
	int console;
	hyper_native_handle_t handle = stream_handle(stream, &console);
	if (!handle)
		return HYPER_NATIVE_STATUS_NOT_FOUND;
	size_t limit = console ? HYPER_NATIVE_CONSOLE_MAX_TRANSFER_BYTES
			       : HYPER_NATIVE_BYTE_CHANNEL_MAX_MESSAGE_BYTES;
	if (count > limit)
		count = limit;
	for (;;) {
		hyper_native_status_t status;
		if (console) {
			hyper_call_result_t result = hyper_console_write(handle, buffer, count);
			status = result.status;
			if ((status == HYPER_NATIVE_STATUS_OK ||
			     status == HYPER_NATIVE_STATUS_WOULD_BLOCK) &&
			    result.value0) {
				if (result.value0 > count)
					return HYPER_NATIVE_STATUS_INTERNAL;
				*actual = result.value0;
				return HYPER_NATIVE_STATUS_OK;
			}
			if (status == HYPER_NATIVE_STATUS_OK)
				return HYPER_NATIVE_STATUS_INTERNAL;
		} else {
			status = hyper_byte_channel_write(handle, buffer, count);
			if (status == HYPER_NATIVE_STATUS_OK) {
				*actual = count;
				return status;
			}
		}
		if (status != HYPER_NATIVE_STATUS_WOULD_BLOCK)
			return status;
		uint64_t signals = console ? HYPER_NATIVE_SIGNAL_CONSOLE_WRITABLE
					   : HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_WRITABLE |
						     HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_PEER_CLOSED;
		hyper_call_result_t wait =
			hyper_object_wait_one(handle, signals, HYPER_NATIVE_DEADLINE_INFINITE);
		if (wait.status != HYPER_NATIVE_STATUS_OK)
			return wait.status;
		if (!console && (wait.value0 & HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_PEER_CLOSED))
			return HYPER_NATIVE_STATUS_PEER_CLOSED;
	}
}

static int64_t read_locked(void *buffer, size_t capacity, size_t *actual)
{
	if (capacity == 0)
		return HYPER_NATIVE_STATUS_OK;
	int console;
	hyper_native_handle_t handle = stream_handle(0, &console);
	if (!handle)
		return HYPER_NATIVE_STATUS_NOT_FOUND;
	for (;;) {
		if (pending_start != pending_end) {
			size_t count = pending_end - pending_start;
			if (count > capacity)
				count = capacity;
			memcpy(buffer, pending + pending_start, count);
			pending_start += count;
			*actual = count;
			return HYPER_NATIVE_STATUS_OK;
		}
		size_t limit = console ? HYPER_NATIVE_CONSOLE_MAX_TRANSFER_BYTES : sizeof(pending);
		hyper_call_result_t result =
			console ? hyper_console_read(handle, pending, limit)
				: hyper_byte_channel_read(handle, pending, limit);
		if (result.status == HYPER_NATIVE_STATUS_PEER_CLOSED)
			return HYPER_NATIVE_STATUS_OK;
		if ((result.status == HYPER_NATIVE_STATUS_OK ||
		     (console && result.status == HYPER_NATIVE_STATUS_WOULD_BLOCK)) &&
		    result.value0 != 0) {
			if (result.value0 > limit)
				return HYPER_NATIVE_STATUS_INTERNAL;
			pending_start = 0;
			pending_end = result.value0;
			continue;
		}
		if (result.status != HYPER_NATIVE_STATUS_OK &&
		    result.status != HYPER_NATIVE_STATUS_WOULD_BLOCK)
			return result.status;
		/* Empty channel messages aren't EOF. Drain subsequent messages. */
		if (!console && result.status == HYPER_NATIVE_STATUS_OK)
			continue;
		uint64_t signals = console ? HYPER_NATIVE_SIGNAL_CONSOLE_READABLE
					   : HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_READABLE |
						     HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_PEER_CLOSED;
		hyper_call_result_t wait =
			hyper_object_wait_one(handle, signals, HYPER_NATIVE_DEADLINE_INFINITE);
		if (wait.status != HYPER_NATIVE_STATUS_OK)
			return wait.status;
		/* Always attempt another read: closure can coexist with queued data. */
	}
}

int64_t hyper_runtime_stdio_read(void *buffer, size_t capacity, size_t *actual)
{
	*actual = 0;
	if (hyper_runtime_stream_connector(0))
		return hyper_runtime_stream_read(buffer, capacity, actual);
	hyper_mutex_lock(&input_lock);
	int64_t result = read_locked(buffer, capacity, actual);
	hyper_mutex_unlock(&input_lock);
	return result;
}
