/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/invocation.h>
#include <hyper/syscall.h>

hyper_call_result_t hyper_abi_query(void)
{
	return hyper_native_invoke6(hyper_native_system_info(), HYPER_NATIVE_OP_ABI_QUERY, 0, 0, 0,
				    0, 0, 0);
}

hyper_call_result_t hyper_system_config(uint64_t key)
{
	return hyper_native_invoke6(hyper_native_system_info(), HYPER_NATIVE_OP_SYSTEM_CONFIG, key,
				    0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_clock_get_monotonic(void)
{
	return hyper_native_invoke6(hyper_native_system_info(), HYPER_NATIVE_OP_CLOCK_GET_MONOTONIC,
				    0, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_handle_close(hyper_native_handle_t handle)
{
	return hyper_native_invoke6(handle, HYPER_NATIVE_OP_HANDLE_CLOSE, 0, 0, 0, 0, 0, 0).status;
}

hyper_call_result_t hyper_handle_duplicate(hyper_native_handle_t source, uint64_t requested_rights)
{
	return hyper_native_invoke6(source, HYPER_NATIVE_OP_HANDLE_DUPLICATE, requested_rights, 0,
				    0, 0, 0, 0);
}

hyper_call_result_t hyper_handle_replace(hyper_native_handle_t source, uint64_t requested_rights)
{
	return hyper_native_invoke6(source, HYPER_NATIVE_OP_HANDLE_REPLACE, requested_rights, 0, 0,
				    0, 0, 0);
}

hyper_call_result_t hyper_handle_get_info(hyper_native_handle_t handle,
					  hyper_native_handle_info_t *info)
{
	return hyper_native_invoke6(handle, HYPER_NATIVE_OP_HANDLE_GET_INFO, (uintptr_t)info,
				    sizeof(*info), 0, 0, 0, 0);
}

hyper_call_result_t hyper_object_get_basic_info(hyper_native_handle_t handle,
						hyper_native_object_basic_info_t *info)
{
	return hyper_native_invoke6(handle, HYPER_NATIVE_OP_OBJECT_GET_BASIC_INFO, (uintptr_t)info,
				    sizeof(*info), 0, 0, 0, 0);
}

hyper_call_result_t hyper_object_wait_one(hyper_native_handle_t object, uint64_t signals,
					  uint64_t deadline)
{
	return hyper_native_invoke6(object, HYPER_NATIVE_OP_OBJECT_WAIT_ONE, signals, deadline, 0,
				    0, 0, 0);
}

hyper_call_result_t hyper_object_wait_many(const hyper_native_object_wait_item_t *items,
					   size_t item_count, uint64_t deadline)
{
	return hyper_native_invoke6(hyper_native_execution_context(),
				    HYPER_NATIVE_OP_OBJECT_WAIT_MANY, (uintptr_t)items, item_count,
				    deadline, 0, 0, 0);
}

hyper_native_status_t hyper_byte_channel_write(hyper_native_handle_t endpoint, const void *bytes,
					       size_t byte_count)
{
	return hyper_native_invoke6(endpoint, HYPER_NATIVE_OP_BYTE_CHANNEL_WRITE, 0,
				    (uintptr_t)bytes, byte_count, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_byte_channel_read(hyper_native_handle_t endpoint, void *bytes,
					    size_t byte_capacity)
{
	return hyper_native_invoke6(endpoint, HYPER_NATIVE_OP_BYTE_CHANNEL_READ, 0,
				    (uintptr_t)bytes, byte_capacity, 0, 0, 0);
}

hyper_call_result_t hyper_capability_channel_create(void)
{
	return hyper_native_invoke6(hyper_native_object_factory(),
				    HYPER_NATIVE_OP_CAPABILITY_CHANNEL_CREATE, 0, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_capability_channel_try_send(
	hyper_native_handle_t endpoint, const void *bytes, size_t byte_count,
	const hyper_native_capability_disposition_t *dispositions, size_t disposition_count)
{
	return hyper_native_invoke6(endpoint, HYPER_NATIVE_OP_CAPABILITY_CHANNEL_TRY_SEND, 0,
				    (uintptr_t)bytes, byte_count, (uintptr_t)dispositions,
				    disposition_count, 0)
		.status;
}

hyper_call_result_t hyper_capability_channel_receive(hyper_native_handle_t endpoint,
						     uint64_t deadline, void *bytes,
						     size_t byte_capacity,
						     hyper_native_capability_receive_slot_t *slots,
						     size_t slot_count)
{
	return hyper_native_invoke6(endpoint, HYPER_NATIVE_OP_CAPABILITY_CHANNEL_RECEIVE, deadline,
				    (uintptr_t)bytes, byte_capacity, (uintptr_t)slots, slot_count,
				    0);
}

hyper_call_result_t hyper_console_discard_input(hyper_native_handle_t console)
{
	return hyper_native_invoke6(console, HYPER_NATIVE_OP_CONSOLE_DISCARD_INPUT, 0, 0, 0, 0, 0,
				    0);
}

hyper_call_result_t hyper_console_read(hyper_native_handle_t console, void *bytes, size_t capacity)
{
	return hyper_native_invoke6(console, HYPER_NATIVE_OP_CONSOLE_READ, 0, (uintptr_t)bytes,
				    capacity, 0, 0, 0);
}

hyper_call_result_t hyper_console_write(hyper_native_handle_t console, const void *bytes,
					size_t count)
{
	return hyper_native_invoke6(console, HYPER_NATIVE_OP_CONSOLE_WRITE, 0, (uintptr_t)bytes,
				    count, 0, 0, 0);
}

hyper_call_result_t hyper_directory_open_file(hyper_native_handle_t directory, const void *path,
					      size_t path_size, uint64_t requested_rights)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_OPEN_FILE, (uintptr_t)path,
				    path_size, requested_rights, 0, 0, 0);
}

hyper_call_result_t hyper_directory_open_directory(hyper_native_handle_t directory,
						   const void *path, size_t path_size,
						   uint64_t requested_rights)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_OPEN_DIRECTORY,
				    (uintptr_t)path, path_size, requested_rights, 0, 0, 0);
}

hyper_call_result_t hyper_directory_begin_read(hyper_native_handle_t directory)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_BEGIN_READ, 0, 0, 0, 0, 0,
				    0);
}

hyper_call_result_t hyper_directory_read(hyper_native_handle_t stream, uint64_t cookie,
					 hyper_native_directory_entry_t *records, size_t capacity)
{
	return hyper_native_invoke6(stream, HYPER_NATIVE_OP_DIRECTORY_READ, cookie,
				    (uintptr_t)records, capacity, 0, 0, 0);
}

hyper_call_result_t hyper_directory_get_info(hyper_native_handle_t directory,
					     hyper_native_directory_info_t *info)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_GET_INFO, (uintptr_t)info,
				    sizeof(*info), 0, 0, 0, 0);
}

hyper_call_result_t hyper_file_read_at(hyper_native_handle_t file, uint64_t offset, void *output,
				       size_t output_capacity)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_READ_AT, 0, offset,
				    (uintptr_t)output, output_capacity, 0, 0);
}

hyper_call_result_t hyper_file_get_info(hyper_native_handle_t file, hyper_native_file_info_t *info)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_GET_INFO, (uintptr_t)info,
				    sizeof(*info), 0, 0, 0, 0);
}

hyper_call_result_t hyper_file_prefetch(hyper_native_handle_t file, uint64_t offset,
					uint64_t length)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_PREFETCH, offset, length, 0, 0, 0,
				    0);
}

hyper_call_result_t hyper_vmo_create_snapshot(hyper_native_handle_t vmo)
{
	return hyper_native_invoke6(vmo, HYPER_NATIVE_OP_VMO_CREATE_SNAPSHOT, 0, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_vmar_map_private(hyper_native_handle_t vmar,
					     hyper_native_handle_t snapshot,
					     const hyper_native_private_mapping_t *mapping,
					     size_t mapping_size)
{
	return hyper_native_invoke6(vmar, HYPER_NATIVE_OP_VMAR_MAP_PRIVATE, snapshot,
				    (uintptr_t)mapping, mapping_size, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_vmo_create(uint64_t size)
{
	return hyper_native_invoke6(hyper_native_object_factory(), HYPER_NATIVE_OP_VMO_CREATE, size,
				    0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_vmo_read(hyper_native_handle_t vmo, uint64_t offset, void *output,
				     size_t byte_count)
{
	return hyper_native_invoke6(vmo, HYPER_NATIVE_OP_VMO_READ, offset, (uintptr_t)output,
				    byte_count, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_vmo_write(hyper_native_handle_t vmo, uint64_t offset, const void *input,
				      size_t byte_count)
{
	return hyper_native_invoke6(vmo, HYPER_NATIVE_OP_VMO_WRITE, offset, (uintptr_t)input,
				    byte_count, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_vmar_allocate(hyper_native_handle_t parent, uintptr_t address,
					size_t size, uint64_t options)
{
	return hyper_native_invoke6(parent, HYPER_NATIVE_OP_VMAR_ALLOCATE, address, size, options,
				    0, 0, 0);
}

hyper_native_status_t hyper_vmar_map(hyper_native_handle_t vmar, hyper_native_handle_t vmo,
				     uint64_t vmo_offset, uintptr_t address, size_t size,
				     uint32_t permissions)
{
	return hyper_native_invoke6(vmar, HYPER_NATIVE_OP_VMAR_MAP, vmo, vmo_offset, address, size,
				    permissions, 0)
		.status;
}

hyper_native_status_t hyper_vmar_protect(hyper_native_handle_t vmar, uintptr_t address, size_t size,
					 uint32_t permissions)
{
	return hyper_native_invoke6(vmar, HYPER_NATIVE_OP_VMAR_PROTECT, address, size, permissions,
				    0, 0, 0)
		.status;
}

hyper_native_status_t hyper_vmar_unmap(hyper_native_handle_t vmar, uintptr_t address, size_t size)
{
	return hyper_native_invoke6(vmar, HYPER_NATIVE_OP_VMAR_UNMAP, address, size, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_vmar_destroy(hyper_native_handle_t vmar)
{
	return hyper_native_invoke6(vmar, HYPER_NATIVE_OP_VMAR_DESTROY, 0, 0, 0, 0, 0, 0).status;
}

hyper_call_result_t hyper_process_builder_create(hyper_native_handle_t factory,
						 hyper_native_handle_t group,
						 hyper_native_handle_t domain,
						 hyper_native_handle_t executable,
						 hyper_native_handle_t loader)
{
	return hyper_native_invoke6(factory, HYPER_NATIVE_OP_PROCESS_BUILDER_CREATE, group, domain,
				    executable, loader, 0, 0);
}

hyper_call_result_t hyper_resource_domain_create(hyper_native_handle_t parent,
						 const hyper_native_resource_limits_t *limits)
{
	return hyper_native_invoke6(parent, HYPER_NATIVE_OP_RESOURCE_DOMAIN_CREATE,
				    (uint64_t)(uintptr_t)limits, sizeof(*limits), 0, 0, 0, 0);
}

hyper_call_result_t hyper_task_group_create(hyper_native_handle_t factory,
					    hyper_native_handle_t resource_domain)
{
	return hyper_native_invoke6(factory, HYPER_NATIVE_OP_TASK_GROUP_CREATE, resource_domain, 0,
				    0, 0, 0, 0);
}

hyper_call_result_t
hyper_virtual_machine_creation_lease_create(hyper_native_handle_t authority,
					    hyper_native_handle_t resource_domain)
{
	return hyper_native_invoke6(authority,
				    HYPER_NATIVE_OP_VIRTUAL_MACHINE_CREATION_LEASE_CREATE,
				    resource_domain, 0, 0, 0, 0, 0);
}

hyper_call_result_t
hyper_virtual_machine_create(hyper_native_handle_t lease,
			     const hyper_native_virtual_machine_configuration_t *configuration)
{
	return hyper_native_invoke6(lease, HYPER_NATIVE_OP_VIRTUAL_MACHINE_CREATE,
				    (uintptr_t)configuration, sizeof(*configuration), 0, 0, 0, 0);
}

hyper_native_status_t hyper_pending_virtual_machine_set_memory(hyper_native_handle_t pending,
							       hyper_native_handle_t vmo)
{
	return hyper_native_invoke6(pending, HYPER_NATIVE_OP_PENDING_VIRTUAL_MACHINE_SET_MEMORY,
				    vmo, 0, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t
hyper_pending_virtual_machine_set_virtual_serial(hyper_native_handle_t pending,
						 hyper_native_handle_t virtual_serial)
{
	return hyper_native_invoke6(pending,
				    HYPER_NATIVE_OP_PENDING_VIRTUAL_MACHINE_SET_VIRTUAL_SERIAL,
				    virtual_serial, 0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_virtual_serial_create(void)
{
	return hyper_native_invoke6(hyper_native_object_factory(),
				    HYPER_NATIVE_OP_VIRTUAL_SERIAL_CREATE, 0, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_virtual_serial_register_output(hyper_native_handle_t serial,
							   hyper_native_handle_t buffer)
{
	return hyper_native_invoke6(serial, HYPER_NATIVE_OP_VIRTUAL_SERIAL_REGISTER_OUTPUT, buffer,
				    0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_virtual_serial_write(hyper_native_handle_t virtual_serial,
					       const void *bytes, size_t byte_count)
{
	return hyper_native_invoke6(virtual_serial, HYPER_NATIVE_OP_VIRTUAL_SERIAL_WRITE,
				    (uintptr_t)bytes, byte_count, 0, 0, 0, 0);
}

hyper_native_status_t
hyper_pending_virtual_machine_set_bootstrap(hyper_native_handle_t pending,
					    const hyper_native_virtual_cpu_bootstrap_t *bootstrap)
{
	return hyper_native_invoke6(pending, HYPER_NATIVE_OP_PENDING_VIRTUAL_MACHINE_SET_BOOTSTRAP,
				    (uintptr_t)bootstrap, sizeof(*bootstrap), 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_pending_virtual_machine_seal(hyper_native_handle_t pending)
{
	return hyper_native_invoke6(pending, HYPER_NATIVE_OP_PENDING_VIRTUAL_MACHINE_SEAL, 0, 0, 0,
				    0, 0, 0)
		.status;
}

hyper_call_result_t hyper_pending_virtual_machine_install(hyper_native_handle_t pending)
{
	return hyper_native_invoke6(pending, HYPER_NATIVE_OP_PENDING_VIRTUAL_MACHINE_INSTALL, 0, 0,
				    0, 0, 0, 0);
}

hyper_native_status_t hyper_virtual_cpu_start(hyper_native_handle_t virtual_cpu)
{
	return hyper_native_invoke6(virtual_cpu, HYPER_NATIVE_OP_VIRTUAL_CPU_START, 0, 0, 0, 0, 0,
				    0)
		.status;
}

hyper_native_status_t hyper_virtual_cpu_set_affinity(hyper_native_handle_t virtual_cpu,
						     const uint64_t *words, size_t word_count)
{
	return hyper_native_invoke6(virtual_cpu, HYPER_NATIVE_OP_VIRTUAL_CPU_SET_AFFINITY,
				    (uintptr_t)words, word_count, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_pending_virtual_machine_abort(hyper_native_handle_t pending)
{
	return hyper_native_invoke6(pending, HYPER_NATIVE_OP_PENDING_VIRTUAL_MACHINE_ABORT, 0, 0, 0,
				    0, 0, 0)
		.status;
}

hyper_native_status_t hyper_virtual_machine_request_stop(hyper_native_handle_t machine)
{
	return hyper_native_invoke6(machine, HYPER_NATIVE_OP_VIRTUAL_MACHINE_REQUEST_STOP, 0, 0, 0,
				    0, 0, 0)
		.status;
}

hyper_call_result_t hyper_virtual_machine_get_info(hyper_native_handle_t machine,
						   hyper_native_virtual_machine_info_t *info)
{
	return hyper_native_invoke6(machine, HYPER_NATIVE_OP_VIRTUAL_MACHINE_GET_INFO,
				    (uintptr_t)info, sizeof(*info), 0, 0, 0, 0);
}

hyper_call_result_t hyper_virtual_cpu_get_info(hyper_native_handle_t vcpu,
					       hyper_native_virtual_cpu_info_t *info)
{
	return hyper_native_invoke6(vcpu, HYPER_NATIVE_OP_VIRTUAL_CPU_GET_INFO, (uintptr_t)info,
				    sizeof(*info), 0, 0, 0, 0);
}

hyper_native_status_t hyper_process_builder_set_name(hyper_native_handle_t builder,
						     const void *name, size_t name_size)
{
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_SET_NAME,
				    (uintptr_t)name, name_size, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_process_builder_set_data(hyper_native_handle_t builder,
						     const void *data, size_t size)
{
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_SET_DATA,
				    (uintptr_t)data, size, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_process_builder_set_affinity(hyper_native_handle_t builder,
							 const uint64_t *words, size_t word_count)
{
	uint8_t encoded[HYPER_NATIVE_PROCESS_AFFINITY_MAX_WORDS * sizeof(uint64_t)] = {0};
	size_t word_index;

	if (word_count > HYPER_NATIVE_PROCESS_AFFINITY_MAX_WORDS ||
	    (word_count != 0 && words == NULL)) {
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	}
	for (word_index = 0; word_index < word_count; ++word_index) {
		size_t byte_index;

		for (byte_index = 0; byte_index < sizeof(uint64_t); ++byte_index) {
			encoded[word_index * sizeof(uint64_t) + byte_index] =
				(uint8_t)(words[word_index] >> (byte_index * 8));
		}
	}
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_SET_AFFINITY,
				    (uintptr_t)encoded, word_count, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_process_builder_add_handle(hyper_native_handle_t builder,
						       hyper_native_handle_t source,
						       uint32_t purpose, uint32_t expected_kind,
						       uint64_t rights, uint32_t operation)
{
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_ADD_HANDLE, source,
				    purpose, expected_kind, rights, operation, 0)
		.status;
}

hyper_native_status_t hyper_process_builder_seal(hyper_native_handle_t builder)
{
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_SEAL, 0, 0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_process_builder_start(hyper_native_handle_t builder)
{
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_START, 0, 0, 0, 0, 0,
				    0);
}

hyper_native_status_t hyper_process_builder_abort(hyper_native_handle_t builder)
{
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_ABORT, 0, 0, 0, 0, 0,
				    0)
		.status;
}

hyper_call_result_t hyper_process_derive_observer(hyper_native_handle_t process)
{
	return hyper_native_invoke6(process, HYPER_NATIVE_OP_PROCESS_DERIVE_OBSERVER, 0, 0, 0, 0, 0,
				    0);
}

hyper_native_status_t hyper_task_group_request_stop(hyper_native_handle_t group)
{
	return hyper_native_invoke6(group, HYPER_NATIVE_OP_TASK_GROUP_REQUEST_STOP, 0, 0, 0, 0, 0,
				    0)
		.status;
}

hyper_native_status_t hyper_process_request_stop(hyper_native_handle_t process)
{
	return hyper_native_invoke6(process, HYPER_NATIVE_OP_PROCESS_REQUEST_STOP, 0, 0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_process_builder_retain(hyper_native_handle_t builder, uint64_t max_bytes,
						 uint64_t timeout_ns)
{
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_RETAIN, max_bytes,
				    timeout_ns, 0, 0, 0, 0);
}

hyper_call_result_t hyper_process_retirement_open_view(hyper_native_handle_t control)
{
	return hyper_native_invoke6(control, HYPER_NATIVE_OP_PROCESS_RETIREMENT_OPEN_VIEW, 0, 0, 0,
				    0, 0, 0);
}

hyper_call_result_t hyper_process_retirement_release(hyper_native_handle_t control)
{
	return hyper_native_invoke6(control, HYPER_NATIVE_OP_PROCESS_RETIREMENT_RELEASE, 0, 0, 0, 0,
				    0, 0);
}

hyper_call_result_t hyper_crash_view_read(hyper_native_handle_t view, uintptr_t address,
					  void *output, size_t length)
{
	return hyper_native_invoke6(view, HYPER_NATIVE_OP_CRASH_VIEW_READ, address,
				    (uintptr_t)output, length, 0, 0, 0);
}

hyper_call_result_t
hyper_process_retirement_get_info(hyper_native_handle_t object,
				  hyper_native_process_retirement_info_t *output)
{
	return hyper_native_invoke6(object, HYPER_NATIVE_OP_PROCESS_RETIREMENT_GET_INFO,
				    (uintptr_t)output, sizeof(*output), 0, 0, 0, 0);
}

hyper_call_result_t hyper_crash_view_get_info(hyper_native_handle_t object,
					      hyper_native_process_retirement_info_t *output)
{
	return hyper_native_invoke6(object, HYPER_NATIVE_OP_CRASH_VIEW_GET_INFO, (uintptr_t)output,
				    sizeof(*output), 0, 0, 0, 0);
}

hyper_call_result_t hyper_crash_view_thread(hyper_native_handle_t object, uint64_t index,
					    hyper_native_crash_thread_t *output)
{
	return hyper_native_invoke6(object, HYPER_NATIVE_OP_CRASH_VIEW_THREAD, index,
				    (uintptr_t)output, sizeof(*output), 0, 0, 0);
}

hyper_call_result_t hyper_crash_view_mapping(hyper_native_handle_t object, uint64_t index,
					     hyper_native_crash_mapping_t *output)
{
	return hyper_native_invoke6(object, HYPER_NATIVE_OP_CRASH_VIEW_MAPPING, index,
				    (uintptr_t)output, sizeof(*output), 0, 0, 0);
}

hyper_call_result_t hyper_crash_view_handle(hyper_native_handle_t object, uint64_t index,
					    hyper_native_crash_handle_t *output)
{
	return hyper_native_invoke6(object, HYPER_NATIVE_OP_CRASH_VIEW_HANDLE, index,
				    (uintptr_t)output, sizeof(*output), 0, 0, 0);
}

hyper_call_result_t hyper_process_get_info(hyper_native_handle_t process,
					   hyper_native_process_info_t *info)
{
	return hyper_native_invoke6(process, HYPER_NATIVE_OP_PROCESS_GET_INFO, (uintptr_t)info,
				    sizeof(*info), 0, 0, 0, 0);
}

hyper_call_result_t hyper_task_inspector_scan_processes(hyper_native_handle_t inspector,
							uint64_t cursor,
							hyper_native_task_process_t *records,
							size_t capacity)
{
	return hyper_native_invoke6(inspector, HYPER_NATIVE_OP_TASK_INSPECTOR_SCAN_PROCESSES,
				    cursor, (uintptr_t)records, capacity, 0, 0, 0);
}

hyper_call_result_t hyper_task_inspector_scan_threads(hyper_native_handle_t inspector,
						      uint64_t cursor,
						      hyper_native_task_thread_t *records,
						      size_t capacity)
{
	return hyper_native_invoke6(inspector, HYPER_NATIVE_OP_TASK_INSPECTOR_SCAN_THREADS, cursor,
				    (uintptr_t)records, capacity, 0, 0, 0);
}

hyper_call_result_t hyper_object_inspector_read_details(hyper_native_handle_t inspector,
							uint64_t process, uint64_t target,
							uint64_t cursor,
							hyper_native_object_details_t *record)
{
	return hyper_native_invoke6(inspector, HYPER_NATIVE_OP_OBJECT_INSPECTOR_READ_DETAILS,
				    process, target, cursor, (uintptr_t)record, sizeof(*record), 0);
}

hyper_call_result_t hyper_object_inspector_scan_objects(hyper_native_handle_t inspector,
							uint64_t cursor,
							hyper_native_object_inspection_t *records,
							size_t capacity)
{
	return hyper_native_invoke6(inspector, HYPER_NATIVE_OP_OBJECT_INSPECTOR_SCAN_OBJECTS,
				    cursor, (uintptr_t)records, capacity, 0, 0, 0);
}

hyper_call_result_t hyper_object_inspector_scan_handles(hyper_native_handle_t inspector,
							uint64_t process_koid, uint64_t cursor,
							hyper_native_handle_inspection_t *records,
							size_t capacity)
{
	return hyper_native_invoke6(inspector, HYPER_NATIVE_OP_OBJECT_INSPECTOR_SCAN_HANDLES,
				    process_koid, cursor, (uintptr_t)records, capacity, 0, 0);
}

hyper_call_result_t hyper_memory_inspector_read(hyper_native_handle_t inspector,
						hyper_native_memory_observation_t *observation)
{
	return hyper_native_invoke6(inspector, HYPER_NATIVE_OP_MEMORY_INSPECTOR_READ,
				    (uintptr_t)observation, sizeof(*observation), 0, 0, 0, 0);
}

hyper_call_result_t hyper_cpu_inspector_read(hyper_native_handle_t inspector,
					     hyper_native_cpu_observation_t *observation)
{
	return hyper_native_invoke6(inspector, HYPER_NATIVE_OP_CPU_INSPECTOR_READ,
				    (uintptr_t)observation, sizeof(*observation), 0, 0, 0, 0);
}

static hyper_call_result_t inspector_derive(uint64_t operation, hyper_native_handle_t inspector,
					    hyper_native_handle_t scope)
{
	return hyper_native_invoke6(inspector, operation, scope, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_task_inspector_derive_process(hyper_native_handle_t inspector,
							hyper_native_handle_t process)
{
	return inspector_derive(HYPER_NATIVE_OP_TASK_INSPECTOR_DERIVE_PROCESS, inspector, process);
}

hyper_call_result_t hyper_task_inspector_derive_task_group(hyper_native_handle_t inspector,
							   hyper_native_handle_t task_group)
{
	return inspector_derive(HYPER_NATIVE_OP_TASK_INSPECTOR_DERIVE_TASK_GROUP, inspector,
				task_group);
}

hyper_call_result_t
hyper_task_inspector_derive_resource_domain(hyper_native_handle_t inspector,
					    hyper_native_handle_t resource_domain)
{
	return inspector_derive(HYPER_NATIVE_OP_TASK_INSPECTOR_DERIVE_RESOURCE_DOMAIN, inspector,
				resource_domain);
}

hyper_call_result_t hyper_object_inspector_derive_process(hyper_native_handle_t inspector,
							  hyper_native_handle_t process)
{
	return inspector_derive(HYPER_NATIVE_OP_OBJECT_INSPECTOR_DERIVE_PROCESS, inspector,
				process);
}

hyper_call_result_t hyper_object_inspector_derive_task_group(hyper_native_handle_t inspector,
							     hyper_native_handle_t task_group)
{
	return inspector_derive(HYPER_NATIVE_OP_OBJECT_INSPECTOR_DERIVE_TASK_GROUP, inspector,
				task_group);
}

hyper_call_result_t
hyper_object_inspector_derive_resource_domain(hyper_native_handle_t inspector,
					      hyper_native_handle_t resource_domain)
{
	return inspector_derive(HYPER_NATIVE_OP_OBJECT_INSPECTOR_DERIVE_RESOURCE_DOMAIN, inspector,
				resource_domain);
}

hyper_native_status_t hyper_thread_yield(void)
{
	return hyper_native_invoke6(hyper_native_execution_context(), HYPER_NATIVE_OP_THREAD_YIELD,
				    0, 0, 0, 0, 0, 0)
		.status;
}

_Noreturn void hyper_thread_exit(int64_t status)
{
	(void)hyper_native_invoke6(hyper_native_execution_context(), HYPER_NATIVE_OP_THREAD_EXIT,
				   (uint64_t)status, 0, 0, 0, 0, 0);
	__builtin_trap();
}

_Noreturn void hyper_process_exit(int64_t status)
{
	(void)hyper_native_invoke6(hyper_native_execution_context(), HYPER_NATIVE_OP_PROCESS_EXIT,
				   (uint64_t)status, 0, 0, 0, 0, 0);
	__builtin_trap();
}

hyper_call_result_t hyper_thread_create(uint64_t entry, uint64_t initial_sp, uint64_t tls,
					uint64_t argument, const uint64_t *affinity_words,
					size_t affinity_word_count)
{
	return hyper_native_invoke6(hyper_native_execution_context(), HYPER_NATIVE_OP_THREAD_CREATE,
				    entry, initial_sp, tls, argument, (uintptr_t)affinity_words,
				    affinity_word_count);
}

hyper_native_status_t hyper_thread_start(hyper_native_handle_t thread)
{
	return hyper_native_invoke6(thread, HYPER_NATIVE_OP_THREAD_START, 0, 0, 0, 0, 0, 0).status;
}

hyper_native_status_t hyper_thread_set_current_name(const void *name, size_t length)
{
	return hyper_native_invoke6(hyper_native_execution_context(),
				    HYPER_NATIVE_OP_THREAD_SET_CURRENT_NAME, (uintptr_t)name,
				    length, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_thread_request_stop(hyper_native_handle_t thread)
{
	return hyper_native_invoke6(thread, HYPER_NATIVE_OP_THREAD_REQUEST_STOP, 0, 0, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_atomic_wait(const uint32_t *address, uint32_t expected,
					uint64_t deadline)
{
	return hyper_native_invoke6(hyper_native_execution_context(), HYPER_NATIVE_OP_ATOMIC_WAIT,
				    (uintptr_t)address, expected, deadline, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_atomic_wake(const uint32_t *address, uint32_t count)
{
	return hyper_native_invoke6(hyper_native_execution_context(), HYPER_NATIVE_OP_ATOMIC_WAKE,
				    (uintptr_t)address, count, 0, 0, 0, 0);
}

hyper_native_status_t hyper_thread_sleep(uint64_t deadline)
{
	return hyper_native_invoke6(hyper_native_execution_context(), HYPER_NATIVE_OP_THREAD_SLEEP,
				    deadline, 0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_file_write_at(hyper_native_handle_t file, uint32_t options,
					uint64_t offset, const void *input, size_t size)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_WRITE_AT, options, offset,
				    (uintptr_t)input, size, 0, 0);
}

hyper_call_result_t hyper_file_resize(hyper_native_handle_t file, uint64_t size)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_RESIZE, size, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_create_file(hyper_native_handle_t directory, const void *path,
						size_t path_size, uint64_t rights, uint32_t mode)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_CREATE_FILE,
				    (uintptr_t)path, path_size, rights, mode, 0, 0);
}

hyper_call_result_t hyper_directory_create_directory(hyper_native_handle_t directory,
						     const void *path, size_t path_size,
						     uint32_t mode)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_CREATE_DIRECTORY,
				    (uintptr_t)path, path_size, mode, 0, 0, 0);
}

hyper_call_result_t hyper_directory_remove(hyper_native_handle_t directory, const void *path,
					   size_t path_size, uint32_t options)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_REMOVE, (uintptr_t)path,
				    path_size, options, 0, 0, 0);
}

hyper_call_result_t hyper_wait_set_create(size_t capacity)
{
	return hyper_native_invoke6(hyper_native_object_factory(), HYPER_NATIVE_OP_WAIT_SET_CREATE,
				    capacity, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_wait_set_add(hyper_native_handle_t set, hyper_native_handle_t source,
				       uint64_t signals)
{
	return hyper_native_invoke6(set, HYPER_NATIVE_OP_WAIT_SET_ADD, source, signals, 0, 0, 0, 0);
}

hyper_call_result_t hyper_wait_set_rearm(hyper_native_handle_t set, uint64_t registration)
{
	return hyper_native_invoke6(set, HYPER_NATIVE_OP_WAIT_SET_REARM, registration, 0, 0, 0, 0,
				    0);
}

hyper_call_result_t hyper_wait_set_remove(hyper_native_handle_t set, uint64_t registration)
{
	return hyper_native_invoke6(set, HYPER_NATIVE_OP_WAIT_SET_REMOVE, registration, 0, 0, 0, 0,
				    0);
}

hyper_call_result_t hyper_wait_set_wait(hyper_native_handle_t set, uint64_t deadline, void *output,
					size_t output_size)
{
	return hyper_native_invoke6(set, HYPER_NATIVE_OP_WAIT_SET_WAIT, deadline, (uintptr_t)output,
				    output_size, 0, 0, 0);
}

hyper_call_result_t hyper_process_get_current_id(void)
{
	return hyper_native_invoke6(hyper_native_execution_context(),
				    HYPER_NATIVE_OP_PROCESS_GET_CURRENT_ID, 0, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_byte_channel_create(void)
{
	return hyper_native_invoke6(hyper_native_object_factory(),
				    HYPER_NATIVE_OP_BYTE_CHANNEL_CREATE, 0, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_scope_create(hyper_native_handle_t root,
						 hyper_native_handle_t start, uint64_t rights)
{
	return hyper_native_invoke6(root, HYPER_NATIVE_OP_DIRECTORY_SCOPE_CREATE, start, rights, 0,
				    0, 0, 0);
}

hyper_call_result_t hyper_directory_get_metadata(hyper_native_handle_t directory, const void *path,
						 size_t path_length, uint32_t options,
						 hyper_native_file_metadata_t *output,
						 size_t output_size)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_GET_METADATA,
				    (uintptr_t)path, path_length, options, (uintptr_t)output,
				    output_size, 0);
}

hyper_call_result_t hyper_file_get_metadata(hyper_native_handle_t file,
					    hyper_native_file_metadata_t *output,
					    size_t output_size)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_GET_METADATA, (uintptr_t)output,
				    output_size, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_get_self_metadata(hyper_native_handle_t directory,
						      hyper_native_file_metadata_t *output,
						      size_t output_size)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_GET_SELF_METADATA,
				    (uintptr_t)output, output_size, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_set_metadata(hyper_native_handle_t directory, const void *path,
						 size_t path_length, uint32_t options,
						 const hyper_native_file_metadata_update_t *input,
						 size_t input_size)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_SET_METADATA,
				    (uintptr_t)path, path_length, options, (uintptr_t)input,
				    input_size, 0);
}

hyper_call_result_t hyper_file_set_metadata(hyper_native_handle_t file,
					    const hyper_native_file_metadata_update_t *input,
					    size_t input_size)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_SET_METADATA, (uintptr_t)input,
				    input_size, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_rename(hyper_native_handle_t source, const void *path,
					   size_t path_length, hyper_native_handle_t destination,
					   const void *new_path, size_t new_path_length)
{
	return hyper_native_invoke6(source, HYPER_NATIVE_OP_DIRECTORY_RENAME, (uintptr_t)path,
				    path_length, destination, (uintptr_t)new_path, new_path_length,
				    0);
}

hyper_call_result_t hyper_directory_link(hyper_native_handle_t source, const void *path,
					 size_t path_length, hyper_native_handle_t destination,
					 const void *new_path, size_t new_path_length)
{
	return hyper_native_invoke6(source, HYPER_NATIVE_OP_DIRECTORY_LINK, (uintptr_t)path,
				    path_length, destination, (uintptr_t)new_path, new_path_length,
				    0);
}

hyper_call_result_t hyper_directory_symlink(hyper_native_handle_t directory, const void *path,
					    size_t path_length, const void *target,
					    size_t target_length)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_SYMLINK, (uintptr_t)path,
				    path_length, (uintptr_t)target, target_length, 0, 0);
}

hyper_call_result_t hyper_directory_read_link(hyper_native_handle_t directory, const void *path,
					      size_t path_length, void *output, size_t capacity)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_READ_LINK, (uintptr_t)path,
				    path_length, (uintptr_t)output, capacity, 0, 0);
}

hyper_call_result_t hyper_directory_canonicalize(hyper_native_handle_t directory, const void *path,
						 size_t path_length, void *output, size_t capacity)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_CANONICALIZE,
				    (uintptr_t)path, path_length, (uintptr_t)output, capacity, 0,
				    0);
}

hyper_call_result_t hyper_directory_remove_if(hyper_native_handle_t directory, const void *path,
					      size_t path_length, uint32_t options,
					      uint64_t expected_node_id)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_REMOVE_IF, (uintptr_t)path,
				    path_length, options, expected_node_id, 0, 0);
}

hyper_call_result_t hyper_directory_open_directory_nofollow(hyper_native_handle_t directory,
							    const void *path, size_t path_length,
							    uint64_t rights)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_OPEN_DIRECTORY_NOFOLLOW,
				    (uintptr_t)path, path_length, rights, 0, 0, 0);
}

hyper_call_result_t hyper_file_sync(hyper_native_handle_t file, uint32_t scope)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_SYNC, scope, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_file_lock(hyper_native_handle_t file, uint32_t mode, uint64_t deadline)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_LOCK, mode, deadline, 0, 0, 0, 0);
}

hyper_call_result_t hyper_file_unlock(hyper_native_handle_t file)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_UNLOCK, 0, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_clock_get_realtime(void)
{
	return hyper_native_invoke6(hyper_native_system_info(), HYPER_NATIVE_OP_CLOCK_GET_REALTIME,
				    0, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_open_file_with_options(hyper_native_handle_t directory,
							   const void *path, size_t path_length,
							   uint64_t rights, uint32_t options,
							   uint32_t mode)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_DIRECTORY_OPEN_FILE_WITH_OPTIONS,
				    (uintptr_t)path, path_length, rights, options, mode, 0);
}

hyper_call_result_t hyper_virtual_machine_creation_lease_get_platform_info(
	hyper_native_handle_t lease, uint32_t profile,
	hyper_native_virtual_machine_platform_info_t *info)
{
	return hyper_native_invoke6(
		lease, HYPER_NATIVE_OP_VIRTUAL_MACHINE_CREATION_LEASE_GET_PLATFORM_INFO, profile,
		(uintptr_t)info, sizeof(*info), 0, 0, 0);
}

hyper_call_result_t hyper_filesystem_mount_ramfs(hyper_native_handle_t directory, const void *path,
						 size_t path_length)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_FILESYSTEM_MOUNT_RAMFS,
				    (uintptr_t)path, path_length, 0, 0, 0, 0);
}

hyper_call_result_t
hyper_filesystem_switch_root(hyper_native_handle_t directory,
			     const hyper_native_filesystem_root_switch_t *request)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_FILESYSTEM_SWITCH_ROOT,
				    (uintptr_t)request, 0, 0, 0, 0, 0);
}

/* Content protection is process-local; closing this token cannot release a
 * separate reference held by a live file-backed mapping. */
hyper_call_result_t hyper_file_protect(hyper_native_handle_t file)
{
	return hyper_native_invoke6(file, HYPER_NATIVE_OP_FILE_PROTECT, 0, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_vmar_map_file(hyper_native_handle_t vmar, hyper_native_handle_t file,
					  hyper_native_handle_t protection,
					  const hyper_native_private_mapping_t *mapping,
					  size_t mapping_size)
{
	return hyper_native_invoke6(vmar, HYPER_NATIVE_OP_VMAR_MAP_FILE, file, protection,
				    (uintptr_t)mapping, mapping_size, 0, 0)
		.status;
}

hyper_native_status_t hyper_process_builder_set_memory_lock(hyper_native_handle_t builder,
							    int enabled)
{
	return hyper_native_invoke6(builder, HYPER_NATIVE_OP_PROCESS_BUILDER_SET_MEMORY_LOCK,
				    enabled != 0, 0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t
hyper_filesystem_begin_root_switch(hyper_native_handle_t directory,
				   const hyper_native_filesystem_root_switch_t *request)
{
	return hyper_native_invoke6(directory, HYPER_NATIVE_OP_FILESYSTEM_BEGIN_ROOT_SWITCH,
				    (uintptr_t)request, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_filesystem_finish_root_switch(hyper_native_handle_t transition,
							  int commit)
{
	return hyper_native_invoke6(transition, HYPER_NATIVE_OP_FILESYSTEM_FINISH_ROOT_SWITCH,
				    commit != 0, 0, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_vmar_lock_memory(hyper_native_handle_t vmar, uintptr_t address,
					     size_t size, uint64_t flags)
{
	return hyper_native_invoke6(vmar, HYPER_NATIVE_OP_VMAR_LOCK_MEMORY, address, size, flags, 0,
				    0, 0)
		.status;
}

hyper_native_status_t hyper_vmar_unlock_memory(hyper_native_handle_t vmar, uintptr_t address,
					       size_t size, uint64_t flags)
{
	return hyper_native_invoke6(vmar, HYPER_NATIVE_OP_VMAR_UNLOCK_MEMORY, address, size, flags,
				    0, 0, 0)
		.status;
}
