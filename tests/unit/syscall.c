/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include "syscall-capture.h"
#include <hyper/invocation.h>
#include <assert.h>

#define EXPECT(name, target, ...)                                                                  \
	expect_call(target, HYPER_NATIVE_OP_##name, (uint64_t[6]){__VA_ARGS__}, status)
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
	EXPECT(FILESYSTEM_MOUNT_RAMFS, handle, (uintptr_t)bytes, sizeof(bytes), 0, 0, 0, 0);
	RESULT(hyper_filesystem_mount_ramfs(handle, bytes, sizeof(bytes)));
	EXPECT(DIRECTORY_BEGIN_READ, handle, 0, 0, 0, 0, 0, 0);
	RESULT(hyper_directory_begin_read(handle));
	EXPECT(FILE_PROTECT, handle, 0, 0, 0, 0, 0, 0);
	RESULT(hyper_file_protect(handle));
	EXPECT(FILE_PREFETCH, handle, offset, 65536, 0, 0, 0, 0);
	RESULT(hyper_file_prefetch(handle, offset, 65536));
	hyper_native_private_mapping_t mapping = {0};
	EXPECT(VMAR_MAP_FILE, handle, other, loader, (uintptr_t)&mapping, sizeof(mapping), 0, 0);
	STATUS(hyper_vmar_map_file(handle, other, loader, &mapping, sizeof(mapping)));
	EXPECT(FILESYSTEM_SWITCH_ROOT, handle, (uintptr_t)&root_switch, 0, 0, 0, 0, 0);
	RESULT(hyper_filesystem_switch_root(handle, &root_switch));
	EXPECT(FILESYSTEM_BEGIN_ROOT_SWITCH, handle, (uintptr_t)&root_switch, 0, 0, 0, 0, 0);
	RESULT(hyper_filesystem_begin_root_switch(handle, &root_switch));
	EXPECT(FILESYSTEM_FINISH_ROOT_SWITCH, handle, 1, 0, 0, 0, 0, 0);
	STATUS(hyper_filesystem_finish_root_switch(handle, 1));
	EXPECT(VMAR_LOCK_MEMORY, handle, offset, deadline, options, 0, 0, 0);
	STATUS(hyper_vmar_lock_memory(handle, offset, deadline, options));
	EXPECT(VMAR_UNLOCK_MEMORY, handle, 0, 0, HYPER_NATIVE_VMAR_MEMORY_CURRENT, 0, 0, 0);
	STATUS(hyper_vmar_unlock_memory(handle, 0, 0, HYPER_NATIVE_VMAR_MEMORY_CURRENT));
	EXPECT(PROCESS_BUILDER_SET_MEMORY_LOCK, handle, 1, 0, 0, 0, 0, 0);
	STATUS(hyper_process_builder_set_memory_lock(handle, 1));
	EXPECT(SYSTEM_CONFIG, hyper_native_system_info(), other, 0, 0, 0, 0, 0);
	RESULT(hyper_system_config(other));
	EXPECT(ABI_QUERY, hyper_native_system_info(), 0, 0, 0, 0, 0, 0);
	RESULT(hyper_abi_query());
	EXPECT(CLOCK_GET_MONOTONIC, hyper_native_system_info(), 0, 0, 0, 0, 0, 0);
	RESULT(hyper_clock_get_monotonic());
	EXPECT(CLOCK_GET_REALTIME, hyper_native_system_info(), 0, 0, 0, 0, 0, 0);
	RESULT(hyper_clock_get_realtime());
	EXPECT(HANDLE_CLOSE, handle, 0, 0, 0, 0, 0, 0);
	STATUS(hyper_handle_close(handle));
	EXPECT(HANDLE_DUPLICATE, handle, other, 0, 0, 0, 0, 0);
	RESULT(hyper_handle_duplicate(handle, other));
	EXPECT(HANDLE_GET_INFO, handle, (uintptr_t)&info, sizeof(info), 0, 0, 0, 0);
	RESULT(hyper_handle_get_info(handle, &info));
	EXPECT(OBJECT_WAIT_ONE, handle, other, deadline, 0, 0, 0, 0);
	RESULT(hyper_object_wait_one(handle, other, deadline));
	EXPECT(BYTE_CHANNEL_WRITE, handle, 0, (uintptr_t)bytes, sizeof(bytes), 0, 0, 0);
	STATUS(hyper_byte_channel_write(handle, bytes, sizeof(bytes)));
	EXPECT(BYTE_CHANNEL_READ, handle, 0, (uintptr_t)bytes, sizeof(bytes), 0, 0, 0);
	RESULT(hyper_byte_channel_read(handle, bytes, sizeof(bytes)));
	EXPECT(CAPABILITY_CHANNEL_TRY_SEND, handle, 0, (uintptr_t)bytes, sizeof(bytes),
	       (uintptr_t)dispositions, 3, 0);
	STATUS(hyper_capability_channel_try_send(handle, bytes, sizeof(bytes), dispositions, 3));
	EXPECT(CAPABILITY_CHANNEL_RECEIVE, handle, deadline, (uintptr_t)bytes, sizeof(bytes),
	       (uintptr_t)slots, 2, 0);
	RESULT(hyper_capability_channel_receive(handle, deadline, bytes, sizeof(bytes), slots, 2));
	EXPECT(FILE_WRITE_AT, handle, options, offset, (uintptr_t)bytes, sizeof(bytes), 0, 0);
	RESULT(hyper_file_write_at(handle, options, offset, bytes, sizeof(bytes)));
	EXPECT(DIRECTORY_RENAME, handle, (uintptr_t)bytes, sizeof(bytes), other,
	       (uintptr_t)destination, sizeof(destination), 0);
	RESULT(hyper_directory_rename(handle, bytes, sizeof(bytes), other, destination,
				      sizeof(destination)));
	EXPECT(DIRECTORY_OPEN_FILE_WITH_OPTIONS, handle, (uintptr_t)bytes, sizeof(bytes), other,
	       options, 0754, 0);
	RESULT(hyper_directory_open_file_with_options(handle, bytes, sizeof(bytes), other, options,
						      0754));
	EXPECT(VMAR_MAP, handle, other, offset, deadline, 4096, options, 0);
	STATUS(hyper_vmar_map(handle, other, offset, deadline, 4096, options));
	uint64_t affinity_words[2] = {3, UINT64_C(0x8000000000000000)};
	EXPECT(VIRTUAL_CPU_SET_AFFINITY, handle, (uintptr_t)affinity_words, 2, 0, 0, 0, 0);
	STATUS(hyper_virtual_cpu_set_affinity(handle, affinity_words, 2));
	EXPECT(THREAD_CREATE, hyper_native_execution_context(), handle, other, offset, deadline, 0,
	       0);
	RESULT(hyper_thread_create(handle, other, offset, deadline, NULL, 0));
	EXPECT(THREAD_SET_CURRENT_NAME, hyper_native_execution_context(), (uintptr_t)bytes,
	       sizeof(bytes), 0, 0, 0, 0);
	STATUS(hyper_thread_set_current_name(bytes, sizeof(bytes)));
	EXPECT(THREAD_CREATE, hyper_native_execution_context(), handle, other, offset, deadline,
	       (uintptr_t)affinity_words, 2);
	RESULT(hyper_thread_create(handle, other, offset, deadline, affinity_words, 2));
	EXPECT(PROCESS_BUILDER_CREATE, handle, other, offset, deadline, loader, 0, 0);
	RESULT(hyper_process_builder_create(handle, other, offset, deadline, loader));
	EXPECT(PROCESS_BUILDER_ADD_HANDLE, handle, other, options, 27, offset, 1, 0);
	STATUS(hyper_process_builder_add_handle(handle, other, options, 27, offset, 1));
	EXPECT(WAIT_SET_WAIT, handle, deadline, (uintptr_t)bytes, sizeof(bytes), 0, 0, 0);
	RESULT(hyper_wait_set_wait(handle, deadline, bytes, sizeof(bytes)));
	uint32_t word = 1;
	EXPECT(ATOMIC_WAIT, hyper_native_execution_context(), (uintptr_t)&word, options, deadline,
	       0, 0, 0);
	STATUS(hyper_atomic_wait(&word, options, deadline));
	EXPECT(ATOMIC_WAKE, hyper_native_execution_context(), (uintptr_t)&word, options, 0, 0, 0,
	       0);
	RESULT(hyper_atomic_wake(&word, options));
	EXPECT(BYTE_CHANNEL_WRITE, handle, 0, 0, 0, 0, 0, 0);
	STATUS(hyper_byte_channel_write(handle, NULL, 0));
}

int main(void)
{
	transport(HYPER_NATIVE_STATUS_OK);
	transport(HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
	transport(HYPER_NATIVE_STATUS_BUFFER_TOO_SMALL);
	return 0;
}
