/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/std.h>
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <assert.h>
#include <string.h>

static const hyper_native_startup_handle_t handles[] = {
	{0x80030001, 0, 10},
	{0x80030002, 0, 11},
	{0x80030003, 0, 12},
};
static const hyper_startup_t startup = {.handle_count = 3, .handles = handles};
static const hyper_native_startup_handle_t console_handle = {
	HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE,
	0,
	13,
};
static const hyper_startup_t console_startup = {.handle_count = 1, .handles = &console_handle};
static const hyper_native_startup_handle_t terminal_handles[] = {
	{0x80030001, 0, 10},
	{0x80030004, 0, 14},
};
static const hyper_startup_t terminal_startup = {.handle_count = 2, .handles = terminal_handles};
static int terminal_mode;
static int mismatched_alias;
static int packet_mode;
static int console_mode;
static int invalid_count;
static unsigned reads;
static unsigned writes;
static unsigned waits;
static int missing;
static int broken;

const hyper_startup_t *hyper_runtime_startup(void)
{
	return missing ? NULL
		       : (terminal_mode ? &terminal_startup
					: (console_mode ? &console_startup : &startup));
}

hyper_native_handle_t hyper_runtime_capability(uint32_t purpose)
{
	(void)purpose;
	return 0;
}

hyper_call_result_t hyper_object_get_basic_info(hyper_native_handle_t h,
						hyper_native_object_basic_info_t *info)
{
	assert(h == 10 || h == 14);
	info->koid = h == 14 && mismatched_alias ? 2 : 1;
	info->object_kind = HYPER_NATIVE_OBJECT_BYTE_CHANNEL;
	return (hyper_call_result_t){0};
}

hyper_native_status_t hyper_thread_yield(void)
{
	return 0;
}

hyper_call_result_t hyper_console_read(hyper_native_handle_t h, void *p, size_t n)
{
	assert(h == 13 && n == HYPER_NATIVE_CONSOLE_MAX_TRANSFER_BYTES);
	memcpy(p, "abcdef", 6);
	return (hyper_call_result_t){HYPER_NATIVE_STATUS_WOULD_BLOCK, invalid_count ? n + 1 : 6, 0};
}

hyper_call_result_t hyper_console_write(hyper_native_handle_t h, const void *p, size_t n)
{
	(void)h;
	(void)p;
	(void)n;
	assert(0);
	return (hyper_call_result_t){0};
}

hyper_call_result_t hyper_object_wait_one(hyper_native_handle_t h, uint64_t signals,
					  uint64_t deadline)
{
	assert(h == 10 || h == 11);
	assert(deadline == HYPER_NATIVE_DEADLINE_INFINITE);
	++waits;
	(void)signals;
	return (hyper_call_result_t){0,
				     h == 10 ? HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_READABLE
					     : HYPER_NATIVE_SIGNAL_BYTE_CHANNEL_WRITABLE,
				     0};
}

hyper_call_result_t hyper_byte_channel_read(hyper_native_handle_t h, void *buffer, size_t capacity)
{
	assert(h == 10);
	assert(capacity == HYPER_NATIVE_BYTE_CHANNEL_MAX_MESSAGE_BYTES);
	if (packet_mode) {
		((unsigned char *)buffer)[0] = packet_mode == 2 ? '\r' : (reads++ == 0 ? 4 : 'z');
		return (hyper_call_result_t){0, 1, 0};
	}
	switch (reads++) {
	case 0:
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_WOULD_BLOCK, 0, 0};
	case 1:
		return (hyper_call_result_t){0, 0, 0}; /* Empty message. */
	case 2:
		memcpy(buffer, "abcdef", 6);
		return (hyper_call_result_t){0, 6, 0};
	default:
		return (hyper_call_result_t){HYPER_NATIVE_STATUS_PEER_CLOSED, 0, 0};
	}
}

hyper_native_status_t hyper_byte_channel_write(hyper_native_handle_t h, const void *buffer,
					       size_t count)
{
	assert(h == 11 || h == 12);
	assert(count <= HYPER_NATIVE_BYTE_CHANNEL_MAX_MESSAGE_BYTES);
	assert(buffer);
	if (broken)
		return HYPER_NATIVE_STATUS_PEER_CLOSED;
	if (++writes == 1)
		return HYPER_NATIVE_STATUS_WOULD_BLOCK;
	return 0;
}

int main(void)
{
	char buffer[3];
	size_t actual = 99;
	assert(hyper_runtime_stdio_read(buffer, 0, &actual) == 0 && actual == 0);
	assert(reads == 0);
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 3);
	assert(memcmp(buffer, "abc", 3) == 0);
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 3);
	assert(memcmp(buffer, "def", 3) == 0);
	assert(reads == 3); /* Remainder came from process-wide buffering. */
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 0);
	assert(hyper_runtime_stdio_write(1, buffer, 3, &actual) == 0 && actual == 3);
	assert(waits == 2);
	assert(hyper_runtime_stdio_write(2, buffer, 3, &actual) == 0 && actual == 3);
	broken = 1;
	assert(hyper_runtime_stdio_write(1, buffer, 3, &actual) ==
		       HYPER_NATIVE_STATUS_PEER_CLOSED &&
	       actual == 0);
	missing = 1;
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == HYPER_NATIVE_STATUS_NOT_FOUND);
	assert(hyper_runtime_stdio_write(1, buffer, 3, &actual) == HYPER_NATIVE_STATUS_NOT_FOUND);
	missing = 0;
	console_mode = 1;
	/* Console WOULD_BLOCK can still return bytes. Preserve their remainder. */
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 3);
	assert(memcmp(buffer, "abc", 3) == 0);
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 3);
	assert(memcmp(buffer, "def", 3) == 0);
	assert(waits == 2);
	invalid_count = 1;
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == HYPER_NATIVE_STATUS_INTERNAL &&
	       actual == 0);
	console_mode = 0;
	invalid_count = 0;
	packet_mode = 1;
	reads = 0;
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 1 && buffer[0] == 4);
	terminal_mode = 1;
	reads = 0;
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 0);
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 1 &&
	       buffer[0] == 'z');
	packet_mode = 2;
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 1 &&
	       buffer[0] == '\n');
	terminal_mode = 0;
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) == 0 && actual == 1 &&
	       buffer[0] == '\r');
	terminal_mode = 1;
	mismatched_alias = 1;
	assert(hyper_runtime_stdio_read(buffer, 3, &actual) ==
		       HYPER_NATIVE_STATUS_INVALID_ARGUMENT &&
	       actual == 0);
	return 0;
}
