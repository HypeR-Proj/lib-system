/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/stream.h>
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <assert.h>
#include <string.h>

static unsigned char replies[16][4128];
static size_t reply_size[16];
static uint64_t created, pending_id, pending_handle;
static unsigned read_requests, fences, closes;
static int deferred;
static unsigned completion_kind;
static int64_t completion_status;

static uint64_t get(const unsigned char *p, size_t n)
{
	uint64_t value = 0;
	for (size_t i = 0; i < n; ++i)
		value |= (uint64_t)p[i] << (8 * i);
	return value;
}

static void put(unsigned char *p, uint64_t value, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		p[i] = value >> (8 * i);
}

static void respond(uint64_t handle, unsigned op, uint64_t id, unsigned flags, const char *bytes,
		    size_t length)
{
	assert(handle < 16 && !reply_size[handle]);
	unsigned char *p = replies[handle];
	memset(p, 0, 32);
	put(p, 0x5354524d, 4);
	put(p + 4, op | 0x8000, 2);
	put(p + 6, flags, 2);
	put(p + 8, id, 8);
	put(p + 16, length, 8);
	if (bytes)
		memcpy(p + 32, bytes, length);
	reply_size[handle] = 32 + (bytes ? length : 0);
}

const hyper_startup_t *hyper_runtime_startup(void)
{
	return NULL;
}

hyper_native_status_t hyper_startup_find_handle(const hyper_startup_t *s, uint32_t p, uint64_t *h)
{
	(void)s;
	(void)p;
	(void)h;
	return HYPER_NATIVE_STATUS_NOT_FOUND;
}

uint64_t hyper_runtime_capability(uint32_t purpose)
{
	return purpose >= HYPER_STREAM_INPUT && purpose <= HYPER_STREAM_ERROR
		       ? 100 + purpose - HYPER_STREAM_INPUT
		       : 0;
}

hyper_call_result_t hyper_byte_channel_create(void)
{
	created += 2;
	return (hyper_call_result_t){0, created, created + 1};
}

int64_t hyper_handle_close(uint64_t handle)
{
	assert(handle < 16);
	reply_size[handle] = 0;
	++closes;
	return 0;
}

int64_t hyper_capability_channel_try_send(uint64_t connector, const void *bytes, size_t size,
					  const hyper_native_capability_disposition_t *offer,
					  size_t count)
{
	const unsigned char *p = bytes;
	assert(connector >= 100 && connector <= 102 && size == 16 && count == 1);
	assert(!memcmp(p, "STRM\1\0\0\0", 8));
	assert(get(p + 8, 8) == (connector == 100 ? 1 : 2));
	assert(offer->operation == HYPER_NATIVE_CAPABILITY_DISPOSITION_MOVE);
	respond(offer->handle - 1, 5, 0, 0, NULL, 1);
	return 0;
}

int64_t hyper_byte_channel_write(uint64_t handle, const void *bytes, size_t length)
{
	const unsigned char *p = bytes;
	assert(length >= 32 && get(p, 4) == 0x5354524d);
	uint64_t id = get(p + 8, 8), size = get(p + 16, 8);
	switch (get(p + 4, 2)) {
	case 1:
		++read_requests;
		assert(size == 1); /* A one-byte Read must never prefetch another command. */
		pending_id = id;
		pending_handle = handle;
		if (!deferred)
			respond(handle, 1, id, completion_kind, completion_kind ? NULL : "x",
				completion_kind ? 0 : 1);
		if (!deferred)
			put(replies[handle] + 24, (uint64_t)completion_status, 8);
		break;
	case 2:
		assert(id == pending_id && handle == pending_handle);
		if (!reply_size[handle])
			respond(handle, 1, id, 2, NULL, 0);
		break;
	case 3:
		assert(size == length - 32 && size > 0);
		respond(handle, 3, id, 0, NULL, size > 1 ? size - 1 : size);
		break;
	case 4:
		++fences;
		respond(handle, 4, id, 0, NULL, 0);
		break;
	default:
		assert(0);
	}
	return 0;
}

hyper_call_result_t hyper_byte_channel_read(uint64_t handle, void *buffer, size_t capacity)
{
	assert(handle < 16);
	if (!reply_size[handle])
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_WOULD_BLOCK, 0, 0};
	size_t size = reply_size[handle];
	assert(size <= capacity);
	memcpy(buffer, replies[handle], size);
	reply_size[handle] = 0;
	return (hyper_call_result_t){0, size, 0};
}

hyper_call_result_t hyper_object_wait_one(uint64_t handle, uint64_t signals, uint64_t deadline)
{
	(void)handle;
	(void)signals;
	(void)deadline;
	assert(0);
	return (hyper_call_result_t){0};
}

int main(void)
{
	char byte = 0;
	size_t actual = 99;
	uint32_t kind = 99;
	uint64_t owner = 0;
	assert(hyper_runtime_stdin_exact());
	assert(hyper_runtime_stream_is_terminal(0));
	assert(hyper_runtime_stream_read(&byte, 1, &actual) == 0 && actual == 1 && byte == 'x');
	assert(read_requests == 1 && created == 2);
	assert(hyper_runtime_stdin_acquire(&owner) == 0 && owner == 2);
	assert(hyper_runtime_stream_read(&byte, 1, &actual) == HYPER_NATIVE_STATUS_BUSY);
	deferred = 1;
	assert(hyper_runtime_stdin_begin(1, UINT64_MAX) == 0);
	assert(hyper_runtime_stdin_poll(&byte, 1, &actual, &kind) ==
	       HYPER_NATIVE_STATUS_WOULD_BLOCK);
	assert(hyper_runtime_stdin_cancel() == 0);
	assert(hyper_runtime_stdin_poll(&byte, 1, &actual, &kind) == 0 && kind == 2 && actual == 0);
	deferred = 0;
	assert(hyper_runtime_stdin_begin(1, UINT64_MAX) == 0);
	assert(hyper_runtime_stdin_cancel() ==
	       0); /* Completion already won: no duplicate response. */
	assert(hyper_runtime_stdin_poll(&byte, 1, &actual, &kind) == 0 && kind == 0 && actual == 1);
	hyper_runtime_stdin_release();
	assert(hyper_runtime_stream_read(&byte, 1, &actual) == 0 && created == 2);
	assert(hyper_runtime_stdin_acquire(&owner) == 0);
	deferred = 1;
	assert(hyper_runtime_stdin_begin(1, UINT64_MAX) == 0);
	hyper_runtime_stdin_release();
	assert(closes == 1); /* Abandoned private connection cannot leak a stale reply. */
	deferred = 0;
	assert(hyper_runtime_stream_read(&byte, 1, &actual) == 0 && created == 4);
	assert(hyper_runtime_stream_write(1, "abc", 3, &actual) == 0 && actual == 2);
	assert(hyper_runtime_stream_fence(1) == 0 && fences == 1);
	completion_kind = 4;
	assert(hyper_runtime_stdin_acquire(&owner) == 0);
	assert(hyper_runtime_stdin_begin(1, UINT64_MAX) == 0);
	assert(hyper_runtime_stdin_poll(&byte, 1, &actual, &kind) == 0 && kind == 4 && actual == 0);
	hyper_runtime_stdin_release();
	assert(hyper_runtime_stream_read(&byte, 1, &actual) == HYPER_NATIVE_STATUS_CANCELLED);

	completion_kind = 0;
	completion_status = HYPER_NATIVE_STATUS_WOULD_BLOCK;
	assert(hyper_runtime_stdin_acquire(&owner) == 0);
	assert(hyper_runtime_stdin_begin(1, 0) == 0);
	assert(hyper_runtime_stdin_poll(&byte, 1, &actual, &kind) ==
		       HYPER_NATIVE_STATUS_WOULD_BLOCK &&
	       kind == 0);
	completion_status = 0;
	assert(hyper_runtime_stdin_begin(1, UINT64_MAX) == 0);
	assert(hyper_runtime_stdin_poll(&byte, 1, &actual, &kind) == 0 && actual == 1);
	hyper_runtime_stdin_release();
	return 0;
}
