/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include "syscall-capture.h"
#include <assert.h>

#define EXPECT(name, ...) expect_call(HYPER_NATIVE_SYS_##name, (uint64_t[6]){__VA_ARGS__}, status)
#define RESULT(expression)                                                                         \
	do {                                                                                       \
		hyper_call_result_t result = (expression);                                         \
		check_consumed();                                                                  \
		assert(result.status == status);                                                   \
		assert(result.value0 == UINT64_C(0xfedcba9876543210));                             \
		assert(result.value1 == UINT64_C(0x89abcdef01234567));                             \
	} while (0)
#define STATUS(expression)                                                                         \
	do {                                                                                       \
		assert((expression) == status);                                                    \
		check_consumed();                                                                  \
	} while (0)

/* Distinct, high-bit scalar values catch truncation and adjacent-slot swaps.
 * Pointer buffers are real objects, even though capture never dereferences them.
 * The suite covers each transport shape, not each syscall's kernel semantics. */
static void transport(hyper_native_status_t status)
{
	const uint64_t handle = UINT64_C(0x8123456789abcdef);
	const uint64_t other = UINT64_C(0x923456789abcdef0);
	const uint64_t offset = UINT64_C(0xa3456789abcdef01);
	const uint64_t deadline = UINT64_C(0xb456789abcdef012);
	const uint64_t loader = UINT64_C(0xc56789abcdef0123);
	const uint32_t options = UINT32_C(0xf1234567);
	uint8_t bytes[17] = {0};
	uint8_t destination[23] = {0};
	hyper_native_handle_info_t info = {0};
	hyper_native_capability_receive_slot_t slots[2] = {0};
	hyper_native_capability_disposition_t dispositions[3] = {0};

	hyper_native_filesystem_root_switch_t root_switch = {0};
	EXPECT(FILESYSTEM_MOUNT_RAMFS, handle, (uintptr_t)bytes, sizeof(bytes));
	RESULT(hyper_filesystem_mount_ramfs(handle, bytes, sizeof(bytes)));
	EXPECT(DIRECTORY_BEGIN_READ, handle);
	RESULT(hyper_directory_begin_read(handle));
	EXPECT(FILE_PROTECT, handle);
	RESULT(hyper_file_protect(handle));
	hyper_native_private_mapping_t mapping = {0};
	EXPECT(VMAR_MAP_FILE, handle, other, loader, (uintptr_t)&mapping, sizeof(mapping));
	STATUS(hyper_vmar_map_file(handle, other, loader, &mapping, sizeof(mapping)));
	EXPECT(FILESYSTEM_SWITCH_ROOT, handle, (uintptr_t)&root_switch);
	RESULT(hyper_filesystem_switch_root(handle, &root_switch));
	EXPECT(SYSTEM_CONFIG, other);
	RESULT(hyper_system_config(other));
	EXPECT(ABI_QUERY, 0);
	RESULT(hyper_abi_query());
	EXPECT(CLOCK_GET_MONOTONIC, 0);
	RESULT(hyper_clock_get_monotonic());
	EXPECT(CLOCK_GET_REALTIME, 0);
	RESULT(hyper_clock_get_realtime());
	EXPECT(HANDLE_CLOSE, handle);
	STATUS(hyper_handle_close(handle));
	EXPECT(HANDLE_DUPLICATE, handle, other);
	RESULT(hyper_handle_duplicate(handle, other));
	EXPECT(HANDLE_GET_INFO, handle, (uintptr_t)&info, sizeof(info));
	RESULT(hyper_handle_get_info(handle, &info));
	EXPECT(OBJECT_WAIT_ONE, handle, other, deadline);
	RESULT(hyper_object_wait_one(handle, other, deadline));
	EXPECT(BYTE_CHANNEL_WRITE, handle, 0, (uintptr_t)bytes, sizeof(bytes));
	STATUS(hyper_byte_channel_write(handle, bytes, sizeof(bytes)));
	EXPECT(BYTE_CHANNEL_READ, handle, 0, (uintptr_t)bytes, sizeof(bytes));
	RESULT(hyper_byte_channel_read(handle, bytes, sizeof(bytes)));
	EXPECT(CAPABILITY_CHANNEL_TRY_SEND, handle, 0, (uintptr_t)bytes, sizeof(bytes),
	       (uintptr_t)dispositions, 3);
	STATUS(hyper_capability_channel_try_send(handle, bytes, sizeof(bytes), dispositions, 3));
	EXPECT(CAPABILITY_CHANNEL_RECEIVE, handle, deadline, (uintptr_t)bytes, sizeof(bytes),
	       (uintptr_t)slots, 2);
	RESULT(hyper_capability_channel_receive(handle, deadline, bytes, sizeof(bytes), slots, 2));
	EXPECT(FILE_WRITE_AT, handle, options, offset, (uintptr_t)bytes, sizeof(bytes));
	RESULT(hyper_file_write_at(handle, options, offset, bytes, sizeof(bytes)));
	EXPECT(DIRECTORY_RENAME, handle, (uintptr_t)bytes, sizeof(bytes), other,
	       (uintptr_t)destination, sizeof(destination));
	RESULT(hyper_directory_rename(handle, bytes, sizeof(bytes), other, destination,
				      sizeof(destination)));
	EXPECT(DIRECTORY_OPEN_FILE_WITH_OPTIONS, handle, (uintptr_t)bytes, sizeof(bytes), other,
	       options, 0754);
	RESULT(hyper_directory_open_file_with_options(handle, bytes, sizeof(bytes), other, options,
						      0754));
	EXPECT(VMAR_MAP, handle, other, offset, deadline, 4096, options);
	STATUS(hyper_vmar_map(handle, other, offset, deadline, 4096, options));
	uint64_t affinity_words[2] = {3, UINT64_C(0x8000000000000000)};
	EXPECT(VIRTUAL_CPU_SET_AFFINITY, handle, (uintptr_t)affinity_words, 2);
	STATUS(hyper_virtual_cpu_set_affinity(handle, affinity_words, 2));
	EXPECT(THREAD_CREATE, handle, other, offset, deadline);
	RESULT(hyper_thread_create(handle, other, offset, deadline, NULL, 0));
	EXPECT(THREAD_CREATE, handle, other, offset, deadline, (uintptr_t)affinity_words, 2);
	RESULT(hyper_thread_create(handle, other, offset, deadline, affinity_words, 2));
	EXPECT(PROCESS_BUILDER_CREATE, handle, other, offset, deadline, loader);
	RESULT(hyper_process_builder_create(handle, other, offset, deadline, loader));
	EXPECT(PROCESS_BUILDER_ADD_HANDLE, handle, other, options, 27, offset, 1);
	STATUS(hyper_process_builder_add_handle(handle, other, options, 27, offset, 1));
	EXPECT(WAIT_SET_WAIT, handle, deadline, (uintptr_t)bytes, sizeof(bytes));
	RESULT(hyper_wait_set_wait(handle, deadline, bytes, sizeof(bytes)));
	uint32_t word = 1;
	EXPECT(ATOMIC_WAIT, (uintptr_t)&word, options, deadline);
	STATUS(hyper_atomic_wait(&word, options, deadline));
	EXPECT(ATOMIC_WAKE, (uintptr_t)&word, options);
	RESULT(hyper_atomic_wake(&word, options));
	EXPECT(BYTE_CHANNEL_WRITE, handle, 0, 0, 0);
	STATUS(hyper_byte_channel_write(handle, NULL, 0));
}

int main(void)
{
	transport(HYPER_NATIVE_STATUS_OK);
	transport(HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	transport(HYPER_NATIVE_STATUS_BUFFER_TOO_SMALL);
	return 0;
}
