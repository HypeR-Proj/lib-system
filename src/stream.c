/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/stream.h>
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <string.h>
#include "mutex-internal.h"

#define MAGIC UINT32_C(0x5354524d)
#define READ 1
#define CANCEL 2
#define WRITE 3
#define FENCE 4
#define INFO 5
#define RESPONSE UINT16_C(0x8000)
#define HEADER 32
#define FRAME (HEADER + HYPER_STREAM_MAX_DATA)
#define SESSION_RIGHTS                                                                             \
	(HYPER_NATIVE_RIGHT_READ | HYPER_NATIVE_RIGHT_WRITE | HYPER_NATIVE_RIGHT_WAIT)

typedef struct {
	hyper_mutex_t lock;
	uint64_t handle, metadata, next_id, pending;
	size_t maximum;
	int native_reader;
} Stream;

static Stream streams[3];

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

static void header(unsigned char *p, uint16_t op, uint64_t id, uint64_t value, uint64_t extra)
{
	memset(p, 0, HEADER);
	store(p, MAGIC, 4);
	store(p + 4, op, 2);
	store(p + 8, id, 8);
	store(p + 16, value, 8);
	store(p + 24, extra, 8);
}

hyper_native_handle_t hyper_runtime_stream_connector(uint32_t stream)
{
	if (stream > 2)
		return 0;
	uint32_t purpose = HYPER_STREAM_INPUT + stream;
	uint64_t handle = hyper_runtime_capability(purpose);
	const hyper_startup_t *startup = hyper_runtime_startup();
	if (!handle && startup)
		(void)hyper_startup_find_handle(startup, purpose, &handle);
	return handle;
}

static int64_t wait_channel(uint64_t handle, uint64_t signal)
{
	hyper_call_result_t r =
		hyper_object_wait_one(handle, signal | HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_PEER_CLOSED,
				      HYPER_NATIVE_DEADLINE_INFINITE);
	return r.status;
}

static int64_t send_frame(uint64_t handle, const void *bytes, size_t size)
{
	for (;;) {
		int64_t status = hyper_byte_channel_write(handle, bytes, size);
		if (status != HYPER_NATIVE_STATUS_WOULD_BLOCK)
			return status;
		status = wait_channel(handle, HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_WRITABLE);
		if (status)
			return status;
	}
}

static hyper_call_result_t receive_frame(uint64_t handle, void *bytes, size_t size, int blocking)
{
	for (;;) {
		hyper_call_result_t r = hyper_byte_channel_read(handle, bytes, size);
		if (r.status != HYPER_NATIVE_STATUS_WOULD_BLOCK || !blocking)
			return r;
		int64_t status = wait_channel(handle, HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_READABLE);
		if (status)
			return (hyper_call_result_t){status, 0, 0};
	}
}

static int64_t validate(const unsigned char *bytes, size_t size, uint16_t op, uint64_t id)
{
	if (size < HEADER || load(bytes, 4) != MAGIC || load(bytes + 4, 2) != (op | RESPONSE) ||
	    load(bytes + 8, 8) != id)
		return HYPER_NATIVE_STATUS_INTERNAL;
	return (int64_t)load(bytes + 24, 8);
}

static int64_t connect_locked(Stream *s, uint32_t index)
{
	if (s->handle)
		return 0;
	uint64_t connector = hyper_runtime_stream_connector(index);
	if (!connector)
		return HYPER_NATIVE_STATUS_NOT_FOUND;
	hyper_call_result_t pair = hyper_byte_channel_create();
	if (pair.status)
		return pair.status;
	unsigned char open[16] = {'S', 'T', 'R', 'M', 1};
	store(open + 8, index ? 2 : 1, 8);
	hyper_native_capability_disposition_t offer = {
		.handle = pair.value1,
		.rights = SESSION_RIGHTS,
		.expected_kind = HYPER_NATIVE_OBJECT_BYTE_CHANNEL,
		.operation = HYPER_NATIVE_CAPABILITY_DISPOSITION_MOVE,
	};
	int64_t status;
	for (;;) {
		status =
			hyper_capability_channel_try_send(connector, open, sizeof(open), &offer, 1);
		if (status != HYPER_NATIVE_STATUS_WOULD_BLOCK)
			break;
		hyper_call_result_t wait = hyper_object_wait_one(
			connector,
			HYPER_NATIVE_SIGNAL_CAPABILITY_CHANNEL_PEER_RECEIVING |
				HYPER_NATIVE_SIGNAL_CAPABILITY_CHANNEL_PEER_CLOSED,
			HYPER_NATIVE_DEADLINE_INFINITE);
		if (wait.status ||
		    (wait.value0 & HYPER_NATIVE_SIGNAL_CAPABILITY_CHANNEL_PEER_CLOSED)) {
			status = wait.status ? wait.status : HYPER_NATIVE_STATUS_PEER_CLOSED;
			break;
		}
	}
	if (status) {
		(void)hyper_handle_close(pair.value0);
		(void)hyper_handle_close(pair.value1);
		return status;
	}
	unsigned char reply[HEADER];
	hyper_call_result_t r = receive_frame(pair.value0, reply, sizeof(reply), 1);
	status = r.status ? r.status : validate(reply, r.value0, INFO, 0);
	if (!status && r.value0 != HEADER)
		status = HYPER_NATIVE_STATUS_INTERNAL;
	if (status) {
		(void)hyper_handle_close(pair.value0);
		return status;
	}
	s->handle = pair.value0;
	s->metadata = load(reply + 16, 8);
	s->next_id = 1;
	return 0;
}

static int64_t begin_locked(Stream *s, size_t size, uint64_t deadline)
{
	if (!size || size > HYPER_STREAM_MAX_DATA || s->pending || s->next_id == UINT64_MAX)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	unsigned char request[HEADER];
	uint64_t id = s->next_id++;
	header(request, READ, id, size, deadline);
	int64_t status = send_frame(s->handle, request, sizeof(request));
	if (!status) {
		s->pending = id;
		s->maximum = size;
	}
	return status;
}

static int64_t poll_locked(Stream *s, void *buffer, size_t capacity, size_t *actual, uint32_t *kind,
			   int blocking)
{
	*actual = 0;
	*kind = UINT32_MAX;
	if (!s->pending || capacity < s->maximum)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	unsigned char frame[FRAME];
	hyper_call_result_t r = receive_frame(s->handle, frame, sizeof(frame), blocking);
	if (r.status)
		return r.status;
	uint64_t id = s->pending;
	s->pending = 0;
	*kind = 0; /* A response exists, including a completed READ error. */
	int64_t status = validate(frame, r.value0, READ, id);
	if (status)
		return status;
	uint64_t count = load(frame + 16, 8);
	uint32_t result = (uint32_t)load(frame + 6, 2);
	if (count > s->maximum || r.value0 != HEADER + count || result > 4 || (result && count) ||
	    (!result && !count))
		return HYPER_NATIVE_STATUS_INTERNAL;
	memcpy(buffer, frame + HEADER, count);
	*actual = count;
	*kind = result;
	return 0;
}

int64_t hyper_runtime_stream_read(void *buffer, size_t capacity, size_t *actual)
{
	*actual = 0;
	if (!capacity)
		return 0;
	Stream *s = &streams[0];
	hyper_mutex_lock(&s->lock);
	int64_t status = s->native_reader ? HYPER_NATIVE_STATUS_BUSY : connect_locked(s, 0);
	if (capacity > HYPER_STREAM_MAX_DATA)
		capacity = HYPER_STREAM_MAX_DATA;
	if (!status)
		status = begin_locked(s, capacity, HYPER_NATIVE_DEADLINE_INFINITE);
	uint32_t kind;
	if (!status) {
		status = poll_locked(s, buffer, capacity, actual, &kind, 1);
		if (!status && kind >= 2)
			status = kind == 3 ? HYPER_NATIVE_STATUS_TIMED_OUT
					   : HYPER_NATIVE_STATUS_CANCELLED;
	}
	hyper_mutex_unlock(&s->lock);
	return status;
}

static int64_t output_locked(Stream *s, uint16_t op, const void *buffer, size_t count,
			     size_t *actual)
{
	if (s->next_id == UINT64_MAX)
		return HYPER_NATIVE_STATUS_RESOURCE_LIMIT;
	unsigned char frame[FRAME];
	uint64_t id = s->next_id++;
	header(frame, op, id, count, 0);
	if (count)
		memcpy(frame + HEADER, buffer, count);
	int64_t status = send_frame(s->handle, frame, HEADER + count);
	if (status)
		return status;
	hyper_call_result_t r = receive_frame(s->handle, frame, HEADER, 1);
	status = r.status ? r.status : validate(frame, r.value0, op, id);
	if (!status) {
		uint64_t accepted = load(frame + 16, 8);
		if (r.value0 != HEADER || load(frame + 6, 2) != 0 || accepted > count ||
		    (count && !accepted))
			return HYPER_NATIVE_STATUS_INTERNAL;
		*actual = accepted;
	}
	return status;
}

int64_t hyper_runtime_stream_write(uint32_t stream, const void *buffer, size_t count,
				   size_t *actual)
{
	*actual = 0;
	if (stream == 0 || stream > 2)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (!count)
		return 0;
	if (count > HYPER_STREAM_MAX_DATA)
		count = HYPER_STREAM_MAX_DATA;
	Stream *s = &streams[stream];
	hyper_mutex_lock(&s->lock);
	int64_t status = connect_locked(s, stream);
	if (!status)
		status = output_locked(s, WRITE, buffer, count, actual);
	hyper_mutex_unlock(&s->lock);
	return status;
}

int64_t hyper_runtime_stream_fence(uint32_t stream)
{
	if (!stream || stream > 2)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (!hyper_runtime_stream_connector(stream))
		return 0;
	Stream *s = &streams[stream];
	hyper_mutex_lock(&s->lock);
	int64_t status = connect_locked(s, stream);
	size_t actual;
	if (!status)
		status = output_locked(s, FENCE, NULL, 0, &actual);
	hyper_mutex_unlock(&s->lock);
	return status;
}

int hyper_runtime_stream_is_terminal(uint32_t stream)
{
	if (stream > 2 || !hyper_runtime_stream_connector(stream))
		return 0;
	Stream *s = &streams[stream];
	hyper_mutex_lock(&s->lock);
	int result = !connect_locked(s, stream) && (s->metadata & 1);
	hyper_mutex_unlock(&s->lock);
	return result;
}

int hyper_runtime_stdin_exact(void)
{
	return hyper_runtime_stream_connector(0) != 0;
}

int64_t hyper_runtime_stdin_acquire(uint64_t *wait_handle)
{
	Stream *s = &streams[0];
	hyper_mutex_lock(&s->lock);
	int64_t status = s->native_reader ? HYPER_NATIVE_STATUS_BUSY : connect_locked(s, 0);
	if (!status) {
		s->native_reader = 1;
		*wait_handle = s->handle;
	}
	hyper_mutex_unlock(&s->lock);
	return status;
}

int64_t hyper_runtime_stdin_begin(size_t maximum, uint64_t deadline)
{
	Stream *s = &streams[0];
	hyper_mutex_lock(&s->lock);
	int64_t status = s->native_reader ? begin_locked(s, maximum, deadline)
					  : HYPER_NATIVE_STATUS_ACCESS_DENIED;
	hyper_mutex_unlock(&s->lock);
	return status;
}

int64_t hyper_runtime_stdin_poll(void *buffer, size_t capacity, size_t *actual, uint32_t *kind)
{
	Stream *s = &streams[0];
	hyper_mutex_lock(&s->lock);
	int64_t status = s->native_reader ? poll_locked(s, buffer, capacity, actual, kind, 0)
					  : HYPER_NATIVE_STATUS_ACCESS_DENIED;
	hyper_mutex_unlock(&s->lock);
	return status;
}

int64_t hyper_runtime_stdin_cancel(void)
{
	Stream *s = &streams[0];
	hyper_mutex_lock(&s->lock);
	unsigned char request[HEADER];
	header(request, CANCEL, s->pending, s->pending, 0);
	int64_t status = s->native_reader && s->pending
				 ? send_frame(s->handle, request, sizeof(request))
				 : HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	hyper_mutex_unlock(&s->lock);
	return status;
}

void hyper_runtime_stdin_release(void)
{
	Stream *s = &streams[0];
	hyper_mutex_lock(&s->lock);
	/* Abandoning an unfinished read closes only this process's private session.
	 * Explicit cancellation plus completion is required to preserve its result. */
	if (s->pending) {
		(void)hyper_handle_close(s->handle);
		s->handle = 0;
		s->pending = 0;
	}
	s->native_reader = 0;
	hyper_mutex_unlock(&s->lock);
}
