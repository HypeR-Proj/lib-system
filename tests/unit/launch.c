/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <hyper/launch.h>
#include <hyper/syscall.h>

static unsigned reads, channel_waits, stops, joins;
static int pending, closed;
static hyper_native_status_t reply, wait_error;
static size_t reply_size;

hyper_call_result_t hyper_byte_channel_read(hyper_native_handle_t channel, void *output,
					    size_t capacity)
{
	assert(channel == 21 && capacity == sizeof(hyper_launch_result_t));
	++reads;
	if (pending) {
		pending = 0;
		return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_WOULD_BLOCK};
	}
	if (closed)
		return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_PEER_CLOSED};
	*(hyper_launch_result_t *)output = (hyper_launch_result_t){.status = reply};
	/* The child may exit immediately after queuing its ready message. */
	closed = 1;
	return (hyper_call_result_t){.value0 = reply_size};
}

hyper_call_result_t hyper_object_wait_one(hyper_native_handle_t handle, uint64_t signals,
					  uint64_t deadline)
{
	assert(deadline == HYPER_NATIVE_DEADLINE_INFINITE);
	if (handle == 21) {
		assert(signals == (HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_READABLE |
				   HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_PEER_CLOSED));
		++channel_waits;
		return (hyper_call_result_t){.status = wait_error};
	}
	assert(handle == 20 && signals == HYPER_NATIVE_SIGNAL_PROCESS_TERMINATED && stops == 1);
	++joins;
	return (hyper_call_result_t){0};
}

hyper_native_status_t hyper_process_request_stop(hyper_native_handle_t process)
{
	assert(process == 20);
	++stops;
	return 0;
}

static void reset(void)
{
	reads = channel_waits = stops = joins = 0;
	pending = closed = 0;
	reply = wait_error = 0;
	reply_size = sizeof(hyper_launch_result_t);
}

int main(void)
{
	reset();
	assert(hyper_launch_wait(20, 21) == 0);
	assert(reads == 1 && !stops && !joins);
	reset();
	pending = 1;
	assert(hyper_launch_wait(20, 21) == 0);
	assert(reads == 2 && channel_waits == 1 && !stops);
	reset();
	reply = HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	assert(hyper_launch_wait(20, 21) == reply && stops == 1 && joins == 1);
	reset();
	closed = 1;
	assert(hyper_launch_wait(20, 21) == HYPER_NATIVE_STATUS_PEER_CLOSED);
	assert(stops == 1 && joins == 1);
	reset();
	reply_size = 1;
	assert(hyper_launch_wait(20, 21) == HYPER_NATIVE_STATUS_BAD_STATE);
	assert(stops == 1 && joins == 1);
	reset();
	reply = 1;
	assert(hyper_launch_wait(20, 21) == HYPER_NATIVE_STATUS_BAD_STATE);
	assert(stops == 1 && joins == 1);
	reset();
	pending = 1;
	wait_error = HYPER_NATIVE_STATUS_CANCELLED;
	assert(hyper_launch_wait(20, 21) == wait_error);
	assert(stops == 1 && joins == 1);
	return 0;
}
