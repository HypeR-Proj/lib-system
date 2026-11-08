/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/random.h>
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

static unsigned live[256], next_handle, accepted, draws, sleeps, waits, closes;
static uint64_t connector = 1, now, expected_deadline, wire_deadline;
static size_t requested;
static int64_t send_error, receive_error, wait_error, provider_error, clock_error;
static unsigned not_ready, send_block, receive_block, fail_after, malformed;
static unsigned malformed_after;
static const unsigned char golden_request[] = {0x48, 0x52, 0x4e, 0x47, 1, 0, 1, 0, 0, 0, 0,
					       0,    3,	   0,	 0,    0, 8, 7, 6, 5, 4, 3,
					       2,    1,	   0,	 0,    0, 0, 0, 0, 0, 0};
static const unsigned char golden_reply[] = {
	0x48, 0x52, 0x4e, 0x47, 1, 0, 1, 0x80, 3, 0, 0, 0, 0, 0, 0,    0,    0,	  0,
	0,    0,    0,	  0,	0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0xaa, 0xbb, 0xcc};

static uint64_t load(const unsigned char *p, size_t n)
{
	uint64_t v = 0;
	for (size_t i = 0; i < n; i++)
		v |= (uint64_t)p[i] << (i * 8);
	return v;
}

static void store(unsigned char *p, uint64_t v, size_t n)
{
	for (size_t i = 0; i < n; i++)
		p[i] = (unsigned char)(v >> (i * 8));
}

static void reset(void)
{
	for (unsigned i = 2; i < 256; i++)
		assert(!live[i]);
	next_handle = 2;
	accepted = draws = sleeps = waits = closes = 0;
	connector = 1;
	now = 0;
	expected_deadline = UINT64_MAX;
	wire_deadline = 0;
	send_error = receive_error = wait_error = provider_error = clock_error = 0;
	not_ready = send_block = receive_block = fail_after = malformed = 0;
	malformed_after = 0;
}

hyper_native_handle_t hyper_runtime_capability(uint32_t purpose)
{
	assert(purpose == HYPER_RANDOM_CLIENT);
	return connector;
}

hyper_call_result_t hyper_clock_get_monotonic(void)
{
	return (hyper_call_result_t){clock_error, now, 0};
}

hyper_native_status_t hyper_thread_sleep(uint64_t until)
{
	assert(until <= expected_deadline && until > now);
	now = until;
	sleeps++;
	return 0;
}

hyper_call_result_t hyper_byte_channel_create(void)
{
	assert(next_handle + 1 < 256);
	unsigned a = next_handle++;
	unsigned b = next_handle++;
	live[a] = live[b] = 1;
	return (hyper_call_result_t){0, a, b};
}

hyper_native_status_t hyper_handle_close(uint64_t h)
{
	assert(h >= 2 && h < 256 && live[h]);
	live[h] = 0;
	closes++;
	return 0;
}

hyper_native_status_t
hyper_capability_channel_try_send(uint64_t h, const void *p, size_t n,
				  const hyper_native_capability_disposition_t *offers, size_t count)
{
	assert(h == 1 && n == 32 && count == 1);
	assert(offers[0].rights == (HYPER_NATIVE_RIGHT_WRITE | HYPER_NATIVE_RIGHT_WAIT));
	assert(offers[0].expected_kind == HYPER_NATIVE_OBJECT_BYTE_CHANNEL);
	assert(offers[0].operation == HYPER_NATIVE_CAPABILITY_DISPOSITION_MOVE);
	const unsigned char *b = p;
	assert(load(b, 4) == 0x474e5248 && load(b + 4, 2) == 1 && load(b + 6, 2) == 1);
	assert(!load(b + 8, 4) && !load(b + 24, 8));
	requested = load(b + 12, 4);
	wire_deadline = load(b + 16, 8);
	assert(requested && requested <= 192 && wire_deadline == expected_deadline);
	if (wire_deadline == 0x0102030405060708ULL)
		assert(!memcmp(p, golden_request, 32));
	if (send_block) {
		send_block--;
		return HYPER_NATIVE_STATUS_WOULD_BLOCK;
	}
	if (send_error)
		return send_error;
	assert(live[offers[0].handle]);
	live[offers[0].handle] = 0;
	accepted++;
	return 0;
}

hyper_call_result_t hyper_object_wait_one(uint64_t h, uint64_t signals, uint64_t deadline)
{
	assert(h && signals && deadline == expected_deadline);
	waits++;
	return (hyper_call_result_t){wait_error, 0, 0};
}

hyper_call_result_t hyper_byte_channel_read(uint64_t h, void *p, size_t n)
{
	assert(live[h] && n == 224);
	if (receive_block) {
		receive_block--;
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_WOULD_BLOCK, 0, 0};
	}
	if (receive_error)
		return (hyper_call_result_t){receive_error, 0, 0};
	draws++;
	int64_t status = provider_error;
	if (not_ready) {
		not_ready--;
		status = HYPER_NATIVE_STATUS_WOULD_BLOCK;
	}
	if (fail_after && draws >= fail_after)
		status = HYPER_NATIVE_STATUS_IO_ERROR;
	unsigned char *b = p;
	memset(b, 0, n);
	store(b, 0x474e5248, 4);
	store(b + 4, 1, 2);
	store(b + 6, 0x8001, 2);
	store(b + 16, (uint64_t)status, 8);
	size_t size = 32;
	if (!status) {
		store(b + 8, requested, 4);
		memset(b + 32, 0xa5, requested);
		size += requested;
		if (wire_deadline == 0x0102030405060708ULL)
			memcpy(b, golden_reply, sizeof(golden_reply));
	}
	switch (!malformed_after || draws >= malformed_after ? malformed : 0) {
	case 1:
		b[0] = 0;
		break;
	case 2:
		b[4] = 2;
		break;
	case 3:
		b[6] = 0;
		break;
	case 4:
		b[12] = 1;
		break;
	case 5:
		b[24] = 1;
		break;
	case 6:
		store(b + 8, 0, 4);
		break;
	case 7:
		size--;
		break;
	case 8:
		store(b + 16, 12345, 8);
		store(b + 8, 0, 4);
		size = 32;
		break;
	case 9:
		size = 31;
		break;
	case 10:
		/* A successful syscall must not claim bytes outside its buffer. */
		size = n + 1;
		break;
	case 11:
		return (hyper_call_result_t){0, size, 1};
	case 12:
		store(b + 16, (uint64_t)HYPER_NATIVE_STATUS_IO_ERROR, 8);
		break;
	case 13:
		store(b + 16, (uint64_t)HYPER_NATIVE_STATUS_IO_ERROR, 8);
		store(b + 8, 0, 4);
		size = 33;
		break;
	case 14:
		size++;
		break;
	}
	return (hyper_call_result_t){0, size, 0};
}

int main(void)
{
	unsigned char out[400];
	hyper_call_result_t r;
	reset();
	connector = 0;
	assert(!hyper_random_fill(NULL, 0, 0));
	assert(hyper_random_read(NULL, 0, 2, 0).status == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	assert(hyper_random_read(NULL, 1, 0, 0).status == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	assert(hyper_random_read(out, 1, 0, 0).status == HYPER_NATIVE_STATUS_ACCESS_DENIED);
	reset();
	expected_deadline = 0x0102030405060708ULL;
	r = hyper_random_read(out, 3, 0, expected_deadline);
	assert(!r.status && r.value0 == 3 && !memcmp(out, "\xaa\xbb\xcc", 3) && closes == 1);
	reset();
	r = hyper_random_read(out, 400, 0, expected_deadline);
	assert(!r.status && r.value0 == 400 && accepted == 3 && closes == 3);
	reset();
	fail_after = 2;
	memset(out, 0, sizeof(out));
	r = hyper_random_read(out, 400, 0, expected_deadline);
	assert(!r.status && r.value0 == 192 && out[191] == 0xa5 && !out[192]);
	reset();
	fail_after = 2;
	assert(hyper_random_fill(out, 400, expected_deadline) == HYPER_NATIVE_STATUS_IO_ERROR);
	reset();
	not_ready = 1;
	r = hyper_random_read(out, 1, HYPER_RANDOM_NONBLOCK, expected_deadline);
	assert(r.status == HYPER_NATIVE_STATUS_WOULD_BLOCK && !sleeps && closes == 1);
	reset();
	not_ready = 1;
	assert(!hyper_random_fill(out, 1, expected_deadline) && sleeps == 1 && accepted == 2);
	reset();
	not_ready = 10;
	expected_deadline = 15000000;
	r = hyper_random_read(out, 1, 0, expected_deadline);
	assert(r.status == HYPER_NATIVE_STATUS_TIMED_OUT && now == expected_deadline &&
	       sleeps == 2);
	reset();
	send_block = receive_block = 1;
	assert(!hyper_random_fill(out, 1, expected_deadline) && waits == 2 && closes == 1);
	reset();
	send_error = HYPER_NATIVE_STATUS_CANCELLED;
	assert(hyper_random_fill(out, 1, expected_deadline) == send_error && closes == 2);
	reset();
	receive_error = HYPER_NATIVE_STATUS_PEER_CLOSED;
	assert(hyper_random_fill(out, 1, expected_deadline) == receive_error && closes == 1);
	reset();
	send_block = 1;
	wait_error = HYPER_NATIVE_STATUS_TIMED_OUT;
	expected_deadline = 100;
	assert(hyper_random_fill(out, 1, expected_deadline) == wait_error && closes == 2);
	reset();
	receive_block = 1;
	wait_error = HYPER_NATIVE_STATUS_CANCELLED;
	assert(hyper_random_fill(out, 1, expected_deadline) == wait_error && closes == 1);
	for (unsigned bad = 1; bad <= 14; bad++) {
		reset();
		malformed = bad;
		out[0] = 0;
		assert(hyper_random_fill(out, 1, expected_deadline) ==
			       HYPER_NATIVE_STATUS_INTERNAL &&
		       !out[0] && closes == 1);
	}
	reset();
	receive_error = HYPER_NATIVE_STATUS_BUFFER_TOO_SMALL;
	out[0] = 0;
	assert(hyper_random_fill(out, 1, expected_deadline) == HYPER_NATIVE_STATUS_INTERNAL &&
	       !out[0] && closes == 1);
	reset();
	provider_error = HYPER_NATIVE_STATUS_IO_ERROR;
	assert(hyper_random_fill(out, 1, expected_deadline) == HYPER_NATIVE_STATUS_IO_ERROR);
	reset();
	malformed = 11;
	malformed_after = 2;
	memset(out, 0, sizeof(out));
	r = hyper_random_read(out, 400, 0, expected_deadline);
	assert(!r.status && r.value0 == 192 && !r.value1 && out[191] == 0xa5 && !out[192] &&
	       closes == 2);
	reset();
	malformed = 11;
	malformed_after = 2;
	assert(hyper_random_fill(out, 400, expected_deadline) == HYPER_NATIVE_STATUS_INTERNAL);
	reset();
	now = 1;
	expected_deadline = 1;
	assert(hyper_random_fill(out, 1, expected_deadline) == HYPER_NATIVE_STATUS_TIMED_OUT &&
	       !accepted);
	reset();
	provider_error = HYPER_NATIVE_STATUS_NOT_SUPPORTED;
	assert(hyper_random_fill(out, 1, expected_deadline) == provider_error && !sleeps);
	reset();
	puts("Random wire, ownership, partial progress and deadlines: PASS");
}
