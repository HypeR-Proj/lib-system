/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/random.h>
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <string.h>

#define MAGIC UINT32_C(0x474e5248)
#define HEADER 32
#define RETRY_NS UINT64_C(10000000)

static uint64_t load(const unsigned char *p, size_t n)
{
	uint64_t value = 0;
	for (size_t i = 0; i < n; ++i)
		value |= (uint64_t)p[i] << (i * 8);
	return value;
}

static void store(unsigned char *p, uint64_t value, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		p[i] = (unsigned char)(value >> (i * 8));
}

static void clear(void *buffer, size_t length)
{
	volatile unsigned char *p = buffer;
	while (length--)
		*p++ = 0;
}

static int64_t check_deadline(uint64_t deadline)
{
	if (deadline == HYPER_NATIVE_DEADLINE_INFINITE)
		return 0;
	hyper_call_result_t now = hyper_clock_get_monotonic();
	if (now.status)
		return now.status;
	return now.value0 >= deadline ? HYPER_NATIVE_STATUS_TIMED_OUT : 0;
}

static int64_t wait(uint64_t handle, uint64_t signals, uint64_t deadline)
{
	return hyper_object_wait_one(handle, signals, deadline).status;
}

static int64_t validate(const unsigned char *reply, size_t size, size_t requested)
{
	if (size < HEADER || load(reply, 4) != MAGIC || load(reply + 4, 2) != 1 ||
	    load(reply + 6, 2) != 0x8001 || load(reply + 12, 4) || load(reply + 24, 8))
		return HYPER_NATIVE_STATUS_INTERNAL;
	uint64_t wire_status = load(reply + 16, 8);
	int64_t status;
	memcpy(&status, &wire_status, sizeof(status));
	size_t count = load(reply + 8, 4);
	if (!status)
		return count == requested && size == HEADER + count ? 0
								    : HYPER_NATIVE_STATUS_INTERNAL;
	if (count || size != HEADER)
		return HYPER_NATIVE_STATUS_INTERNAL;
	switch (status) {
	case HYPER_NATIVE_STATUS_INVALID_ARGUMENT:
	case HYPER_NATIVE_STATUS_NOT_SUPPORTED:
	case HYPER_NATIVE_STATUS_TIMED_OUT:
	case HYPER_NATIVE_STATUS_WOULD_BLOCK:
	case HYPER_NATIVE_STATUS_IO_ERROR:
		return status;
	default:
		return HYPER_NATIVE_STATUS_INTERNAL;
	}
}

static int64_t draw(uint64_t connector, void *buffer, size_t length, uint64_t deadline)
{
	int64_t status = check_deadline(deadline);
	if (status)
		return status;
	hyper_call_result_t pair = hyper_byte_channel_create();
	if (pair.status)
		return pair.status;
	unsigned char request[HEADER] = {0};
	store(request, MAGIC, 4);
	store(request + 4, 1, 2);
	store(request + 6, 1, 2);
	store(request + 12, length, 4);
	store(request + 16, deadline, 8);
	hyper_native_capability_disposition_t offer = {
		.handle = pair.value1,
		.rights = HYPER_NATIVE_RIGHT_WRITE | HYPER_NATIVE_RIGHT_WAIT,
		.expected_kind = HYPER_NATIVE_OBJECT_BYTE_CHANNEL,
		.operation = HYPER_NATIVE_CAPABILITY_DISPOSITION_MOVE,
	};
	for (;;) {
		status = check_deadline(deadline);
		if (status)
			break;
		status = hyper_capability_channel_try_send(connector, request, sizeof(request),
							   &offer, 1);
		if (status != HYPER_NATIVE_STATUS_WOULD_BLOCK)
			break;
		hyper_call_result_t ready = hyper_object_wait_one(
			connector,
			HYPER_NATIVE_SIGNAL_CAPABILITY_CHANNEL_PEER_RECEIVING |
				HYPER_NATIVE_SIGNAL_CAPABILITY_CHANNEL_PEER_CLOSED,
			deadline);
		status = ready.status;
		if (!status && (ready.value0 & HYPER_NATIVE_SIGNAL_CAPABILITY_CHANNEL_PEER_CLOSED))
			status = HYPER_NATIVE_STATUS_PEER_CLOSED;
		if (status)
			break;
	}
	if (status) {
		(void)hyper_handle_close(pair.value1);
		(void)hyper_handle_close(pair.value0);
		return status;
	}
	/* The sender owner moved on successful rendezvous. Only the receiver
	 * remains ours, including timeout/cancellation before a late response. */
	unsigned char reply[HEADER + HYPER_RANDOM_MAX_REQUEST] = {0};
	for (;;) {
		status = check_deadline(deadline);
		if (status)
			break;
		hyper_call_result_t response =
			hyper_byte_channel_read(pair.value0, reply, sizeof(reply));
		status = response.status;
		if (status != HYPER_NATIVE_STATUS_WOULD_BLOCK) {
			if (!status) {
				status = response.value0 > sizeof(reply) || response.value1
						 ? HYPER_NATIVE_STATUS_INTERNAL
						 : validate(reply, response.value0, length);
				if (!status)
					memcpy(buffer, reply + HEADER, length);
			} else if (status == HYPER_NATIVE_STATUS_BUFFER_TOO_SMALL) {
				status = HYPER_NATIVE_STATUS_INTERNAL;
			}
			break;
		}
		status = wait(pair.value0,
			      HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_READABLE |
				      HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_PEER_CLOSED,
			      deadline);
		if (status)
			break;
	}
	clear(reply, sizeof(reply));
	(void)hyper_handle_close(pair.value0);
	return status;
}

hyper_call_result_t hyper_random_read(void *buffer, size_t length, uint32_t flags,
				      uint64_t deadline)
{
	if ((flags & ~HYPER_RANDOM_NONBLOCK) || (length && !buffer))
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_INVALID_ARGUMENT, 0, 0};
	if (!length)
		return (hyper_call_result_t){0, 0, 0};
	uint64_t connector = hyper_runtime_capability(HYPER_RANDOM_CLIENT);
	if (!connector)
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_ACCESS_DENIED, 0, 0};
	size_t completed = 0;
	int64_t status = 0;
	while (completed < length) {
		size_t chunk = length - completed;
		if (chunk > HYPER_RANDOM_MAX_REQUEST)
			chunk = HYPER_RANDOM_MAX_REQUEST;
		status = draw(connector, (unsigned char *)buffer + completed, chunk, deadline);
		if (status == HYPER_NATIVE_STATUS_WOULD_BLOCK && !(flags & HYPER_RANDOM_NONBLOCK)) {
			hyper_call_result_t now = hyper_clock_get_monotonic();
			if (now.status) {
				status = now.status;
				break;
			}
			if (now.value0 >= deadline) {
				status = HYPER_NATIVE_STATUS_TIMED_OUT;
				break;
			}
			uint64_t retry = now.value0 >= UINT64_MAX - RETRY_NS
						 ? UINT64_MAX - 1
						 : now.value0 + RETRY_NS;
			if (retry > deadline)
				retry = deadline;
			status = hyper_thread_sleep(retry);
			if (status)
				break;
			continue;
		}
		if (status)
			break;
		completed += chunk;
	}
	return (hyper_call_result_t){completed ? 0 : status, completed, 0};
}

hyper_native_status_t hyper_random_fill(void *buffer, size_t length, uint64_t deadline)
{
	size_t completed = 0;
	do {
		hyper_call_result_t result =
			hyper_random_read(buffer ? (unsigned char *)buffer + completed : NULL,
					  length - completed, 0, deadline);
		if (result.status)
			return result.status;
		if (result.value0 > length - completed || (!result.value0 && length != completed))
			return HYPER_NATIVE_STATUS_INTERNAL;
		completed += result.value0;
	} while (completed < length);
	return 0;
}
