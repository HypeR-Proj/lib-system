/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/syscall.h>

static hyper_call_result_t call0(uint64_t number)
{
	return hyper_native_call6(number, 0, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_abi_query(void)
{
	return call0(HYPER_NATIVE_SYS_ABI_QUERY);
}

hyper_call_result_t hyper_system_config(uint64_t key)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_SYSTEM_CONFIG, key, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_clock_get_monotonic(void)
{
	return call0(HYPER_NATIVE_SYS_CLOCK_GET_MONOTONIC);
}

hyper_native_status_t hyper_handle_close(hyper_native_handle_t handle)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_HANDLE_CLOSE, handle, 0, 0, 0, 0, 0).status;
}

hyper_call_result_t hyper_handle_duplicate(hyper_native_handle_t source, uint64_t requested_rights)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_HANDLE_DUPLICATE, source, requested_rights, 0, 0,
				  0, 0);
}

hyper_call_result_t hyper_handle_replace(hyper_native_handle_t source, uint64_t requested_rights)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_HANDLE_REPLACE, source, requested_rights, 0, 0,
				  0, 0);
}

hyper_call_result_t hyper_handle_get_info(hyper_native_handle_t handle,
					  hyper_native_handle_info_t *info)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_HANDLE_GET_INFO, handle, (uintptr_t)info,
				  sizeof(*info), 0, 0, 0);
}

hyper_call_result_t hyper_object_get_basic_info(hyper_native_handle_t handle,
						hyper_native_object_basic_info_t *info)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_OBJECT_GET_BASIC_INFO, handle, (uintptr_t)info,
				  sizeof(*info), 0, 0, 0);
}

hyper_call_result_t hyper_object_wait_one(hyper_native_handle_t object, uint64_t signals,
					  uint64_t deadline)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_OBJECT_WAIT_ONE, object, signals, deadline, 0, 0,
				  0);
}

hyper_call_result_t hyper_object_wait_many(const hyper_native_object_wait_item_t *items,
					   size_t item_count, uint64_t deadline)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_OBJECT_WAIT_MANY, (uintptr_t)items, item_count,
				  deadline, 0, 0, 0);
}

hyper_native_status_t hyper_byte_channel_write(hyper_native_handle_t endpoint, const void *bytes,
					       size_t byte_count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_BYTE_CHANNEL_WRITE, endpoint, 0,
				  (uintptr_t)bytes, byte_count, 0, 0)
		.status;
}

hyper_call_result_t hyper_byte_channel_read(hyper_native_handle_t endpoint, void *bytes,
					    size_t byte_capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_BYTE_CHANNEL_READ, endpoint, 0, (uintptr_t)bytes,
				  byte_capacity, 0, 0);
}

hyper_call_result_t hyper_capability_channel_create(void)
{
	return call0(HYPER_NATIVE_SYS_CAPABILITY_CHANNEL_CREATE);
}

hyper_native_status_t hyper_capability_channel_try_send(
	hyper_native_handle_t endpoint, const void *bytes, size_t byte_count,
	const hyper_native_capability_disposition_t *dispositions, size_t disposition_count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_CAPABILITY_CHANNEL_TRY_SEND, endpoint, 0,
				  (uintptr_t)bytes, byte_count, (uintptr_t)dispositions,
				  disposition_count)
		.status;
}

hyper_call_result_t hyper_capability_channel_receive(hyper_native_handle_t endpoint,
						     uint64_t deadline, void *bytes,
						     size_t byte_capacity,
						     hyper_native_capability_receive_slot_t *slots,
						     size_t slot_count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_CAPABILITY_CHANNEL_RECEIVE, endpoint, deadline,
				  (uintptr_t)bytes, byte_capacity, (uintptr_t)slots, slot_count);
}

hyper_call_result_t hyper_console_discard_input(hyper_native_handle_t console)
{
    return hyper_native_call6(HYPER_NATIVE_SYS_CONSOLE_DISCARD_INPUT, console, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_console_read(hyper_native_handle_t console, void *bytes, size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_CONSOLE_READ, console, 0, (uintptr_t)bytes,
				  capacity, 0, 0);
}

hyper_call_result_t hyper_console_write(hyper_native_handle_t console, const void *bytes,
					size_t count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_CONSOLE_WRITE, console, 0, (uintptr_t)bytes,
				  count, 0, 0);
}

hyper_call_result_t hyper_directory_open_file(hyper_native_handle_t directory, const void *path,
					      size_t path_size, uint64_t requested_rights)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_OPEN_FILE, directory, (uintptr_t)path,
				  path_size, requested_rights, 0, 0);
}

hyper_call_result_t hyper_directory_open_directory(hyper_native_handle_t directory,
						   const void *path, size_t path_size,
						   uint64_t requested_rights)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_OPEN_DIRECTORY, directory,
				  (uintptr_t)path, path_size, requested_rights, 0, 0);
}

hyper_call_result_t hyper_directory_begin_read(hyper_native_handle_t directory)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_BEGIN_READ, directory, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_read(hyper_native_handle_t stream, uint64_t cookie,
					 hyper_native_directory_entry_t *records, size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_READ, stream, cookie,
				  (uintptr_t)records, capacity, 0, 0);
}

hyper_call_result_t hyper_directory_get_info(hyper_native_handle_t directory,
					     hyper_native_directory_info_t *info)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_GET_INFO, directory, (uintptr_t)info,
				  sizeof(*info), 0, 0, 0);
}

hyper_call_result_t hyper_file_read_at(hyper_native_handle_t file, uint64_t offset, void *output,
				       size_t output_capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_READ_AT, file, 0, offset, (uintptr_t)output,
				  output_capacity, 0);
}

hyper_call_result_t hyper_file_get_info(hyper_native_handle_t file, hyper_native_file_info_t *info)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_GET_INFO, file, (uintptr_t)info,
				  sizeof(*info), 0, 0, 0);
}

hyper_call_result_t hyper_vmo_create_snapshot(hyper_native_handle_t vmo)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMO_CREATE_SNAPSHOT, vmo, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_vmar_map_private(hyper_native_handle_t vmar,
					     hyper_native_handle_t snapshot,
					     const hyper_native_private_mapping_t *mapping,
					     size_t mapping_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMAR_MAP_PRIVATE, vmar, snapshot,
				  (uintptr_t)mapping, mapping_size, 0, 0)
		.status;
}

hyper_call_result_t hyper_vmo_create(uint64_t size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMO_CREATE, size, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_vmo_read(hyper_native_handle_t vmo, uint64_t offset, void *output,
				     size_t byte_count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMO_READ, vmo, offset, (uintptr_t)output,
				  byte_count, 0, 0)
		.status;
}

hyper_native_status_t hyper_vmo_write(hyper_native_handle_t vmo, uint64_t offset, const void *input,
				      size_t byte_count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMO_WRITE, vmo, offset, (uintptr_t)input,
				  byte_count, 0, 0)
		.status;
}

hyper_call_result_t hyper_vmar_allocate(hyper_native_handle_t parent, uintptr_t address,
					size_t size, uint64_t options)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMAR_ALLOCATE, parent, address, size, options, 0,
				  0);
}

hyper_native_status_t hyper_vmar_map(hyper_native_handle_t vmar, hyper_native_handle_t vmo,
				     uint64_t vmo_offset, uintptr_t address, size_t size,
				     uint32_t permissions)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMAR_MAP, vmar, vmo, vmo_offset, address, size,
				  permissions)
		.status;
}

hyper_native_status_t hyper_vmar_protect(hyper_native_handle_t vmar, uintptr_t address, size_t size,
					 uint32_t permissions)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMAR_PROTECT, vmar, address, size, permissions,
				  0, 0)
		.status;
}

hyper_native_status_t hyper_vmar_unmap(hyper_native_handle_t vmar, uintptr_t address, size_t size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMAR_UNMAP, vmar, address, size, 0, 0, 0).status;
}

hyper_native_status_t hyper_vmar_destroy(hyper_native_handle_t vmar)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMAR_DESTROY, vmar, 0, 0, 0, 0, 0).status;
}

hyper_call_result_t hyper_process_builder_create(hyper_native_handle_t factory,
						 hyper_native_handle_t group,
						 hyper_native_handle_t domain,
						 hyper_native_handle_t executable,
						 hyper_native_handle_t loader)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_BUILDER_CREATE, factory, group, domain,
				  executable, loader, 0);
}

hyper_call_result_t hyper_resource_domain_create(hyper_native_handle_t parent,
						 const hyper_native_resource_limits_t *limits)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_RESOURCE_DOMAIN_CREATE, parent,
				  (uint64_t)(uintptr_t)limits, sizeof(*limits), 0, 0, 0);
}

hyper_call_result_t hyper_task_group_create(hyper_native_handle_t factory,
					    hyper_native_handle_t resource_domain)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_TASK_GROUP_CREATE, factory, resource_domain, 0,
				  0, 0, 0);
}

hyper_call_result_t
hyper_virtual_machine_creation_lease_create(hyper_native_handle_t authority,
					    hyper_native_handle_t resource_domain)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_MACHINE_CREATION_LEASE_CREATE, authority,
				  resource_domain, 0, 0, 0, 0);
}

hyper_call_result_t
hyper_virtual_machine_create(hyper_native_handle_t lease,
			     const hyper_native_virtual_machine_configuration_t *configuration)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_MACHINE_CREATE, lease,
				  (uintptr_t)configuration, sizeof(*configuration), 0, 0, 0);
}

hyper_native_status_t hyper_pending_virtual_machine_set_memory(hyper_native_handle_t pending,
							       hyper_native_handle_t vmo)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PENDING_VIRTUAL_MACHINE_SET_MEMORY, pending, vmo,
				  0, 0, 0, 0)
		.status;
}

hyper_native_status_t
hyper_pending_virtual_machine_set_virtual_serial(hyper_native_handle_t pending,
						 hyper_native_handle_t virtual_serial)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PENDING_VIRTUAL_MACHINE_SET_VIRTUAL_SERIAL,
				  pending, virtual_serial, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_virtual_serial_create(void)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_SERIAL_CREATE, 0, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_virtual_serial_register_output(hyper_native_handle_t serial,
							   hyper_native_handle_t buffer)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_SERIAL_REGISTER_OUTPUT, serial, buffer,
				  0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_virtual_serial_write(hyper_native_handle_t virtual_serial,
					       const void *bytes, size_t byte_count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_SERIAL_WRITE, virtual_serial,
				  (uintptr_t)bytes, byte_count, 0, 0, 0);
}

hyper_native_status_t
hyper_pending_virtual_machine_set_bootstrap(hyper_native_handle_t pending,
					    const hyper_native_virtual_cpu_bootstrap_t *bootstrap)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PENDING_VIRTUAL_MACHINE_SET_BOOTSTRAP, pending,
				  (uintptr_t)bootstrap, sizeof(*bootstrap), 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_pending_virtual_machine_seal(hyper_native_handle_t pending)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PENDING_VIRTUAL_MACHINE_SEAL, pending, 0, 0, 0,
				  0, 0)
		.status;
}

hyper_call_result_t hyper_pending_virtual_machine_install(hyper_native_handle_t pending)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PENDING_VIRTUAL_MACHINE_INSTALL, pending, 0, 0,
				  0, 0, 0);
}

hyper_native_status_t hyper_virtual_cpu_start(hyper_native_handle_t virtual_cpu)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_CPU_START, virtual_cpu, 0, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_virtual_cpu_set_affinity(hyper_native_handle_t virtual_cpu,
						     const uint64_t *words, size_t word_count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_CPU_SET_AFFINITY, virtual_cpu,
				  (uintptr_t)words, word_count, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_pending_virtual_machine_abort(hyper_native_handle_t pending)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PENDING_VIRTUAL_MACHINE_ABORT, pending, 0, 0, 0,
				  0, 0)
		.status;
}

hyper_native_status_t hyper_virtual_machine_request_stop(hyper_native_handle_t machine)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_MACHINE_REQUEST_STOP, machine, 0, 0, 0,
				  0, 0)
		.status;
}

hyper_call_result_t hyper_virtual_machine_get_info(hyper_native_handle_t machine,
						   hyper_native_virtual_machine_info_t *info)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_MACHINE_GET_INFO, machine,
				  (uintptr_t)info, sizeof(*info), 0, 0, 0);
}

hyper_call_result_t hyper_virtual_cpu_get_info(hyper_native_handle_t vcpu,
					       hyper_native_virtual_cpu_info_t *info)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_CPU_GET_INFO, vcpu, (uintptr_t)info,
				  sizeof(*info), 0, 0, 0);
}

hyper_native_status_t hyper_process_builder_set_name(hyper_native_handle_t builder,
						     const void *name, size_t name_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_BUILDER_SET_NAME, builder,
				  (uintptr_t)name, name_size, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_process_builder_set_data(hyper_native_handle_t builder,
						     const void *data, size_t size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_BUILDER_SET_DATA, builder,
				  (uintptr_t)data, size, 0, 0, 0)
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
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_BUILDER_SET_AFFINITY, builder,
				  (uintptr_t)encoded, word_count, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_process_builder_add_handle(hyper_native_handle_t builder,
						       hyper_native_handle_t source,
						       uint32_t purpose, uint32_t expected_kind,
						       uint64_t rights, uint32_t operation)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_BUILDER_ADD_HANDLE, builder, source,
				  purpose, expected_kind, rights, operation)
		.status;
}

hyper_native_status_t hyper_process_builder_seal(hyper_native_handle_t builder)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_BUILDER_SEAL, builder, 0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_process_builder_start(hyper_native_handle_t builder)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_BUILDER_START, builder, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_process_builder_abort(hyper_native_handle_t builder)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_BUILDER_ABORT, builder, 0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_process_derive_observer(hyper_native_handle_t process)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_DERIVE_OBSERVER, process, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_task_group_request_stop(hyper_native_handle_t group)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_TASK_GROUP_REQUEST_STOP, group, 0, 0, 0, 0, 0).status;
}

hyper_native_status_t hyper_process_request_stop(hyper_native_handle_t process)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_REQUEST_STOP, process, 0, 0, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_process_get_info(hyper_native_handle_t process,
					   hyper_native_process_info_t *info)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_GET_INFO, process, (uintptr_t)info,
				  sizeof(*info), 0, 0, 0);
}

hyper_call_result_t hyper_task_inspector_scan_processes(hyper_native_handle_t inspector,
							uint64_t cursor,
							hyper_native_task_process_t *records,
							size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_TASK_INSPECTOR_SCAN_PROCESSES, inspector, cursor,
				  (uintptr_t)records, capacity, 0, 0);
}

hyper_call_result_t hyper_task_inspector_scan_threads(hyper_native_handle_t inspector,
						      uint64_t cursor,
						      hyper_native_task_thread_t *records,
						      size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_TASK_INSPECTOR_SCAN_THREADS, inspector, cursor,
				  (uintptr_t)records, capacity, 0, 0);
}

hyper_call_result_t hyper_object_inspector_read_details(hyper_native_handle_t inspector,
							uint64_t process, uint64_t target,
							uint64_t cursor,
							hyper_native_object_details_t *record)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_OBJECT_INSPECTOR_READ_DETAILS, inspector,
				  process, target, cursor, (uintptr_t)record, sizeof(*record));
}

hyper_call_result_t hyper_object_inspector_scan_objects(hyper_native_handle_t inspector,
							uint64_t cursor,
							hyper_native_object_inspection_t *records,
							size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_OBJECT_INSPECTOR_SCAN_OBJECTS, inspector, cursor,
				  (uintptr_t)records, capacity, 0, 0);
}

hyper_call_result_t hyper_object_inspector_scan_handles(hyper_native_handle_t inspector,
							uint64_t process_koid, uint64_t cursor,
							hyper_native_handle_inspection_t *records,
							size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_OBJECT_INSPECTOR_SCAN_HANDLES, inspector,
				  process_koid, cursor, (uintptr_t)records, capacity, 0);
}

hyper_call_result_t hyper_memory_inspector_read(hyper_native_handle_t inspector,
						hyper_native_memory_observation_t *observation)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_MEMORY_INSPECTOR_READ, inspector,
				  (uintptr_t)observation, sizeof(*observation), 0, 0, 0);
}

hyper_call_result_t hyper_cpu_inspector_read(hyper_native_handle_t inspector,
					     hyper_native_cpu_observation_t *observation)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_CPU_INSPECTOR_READ, inspector,
				  (uintptr_t)observation, sizeof(*observation), 0, 0, 0);
}

static hyper_call_result_t inspector_derive(uint64_t number, hyper_native_handle_t inspector,
					    hyper_native_handle_t scope)
{
	return hyper_native_call6(number, inspector, scope, 0, 0, 0, 0);
}

hyper_call_result_t hyper_task_inspector_derive_process(hyper_native_handle_t inspector,
							hyper_native_handle_t process)
{
	return inspector_derive(HYPER_NATIVE_SYS_TASK_INSPECTOR_DERIVE_PROCESS, inspector, process);
}

hyper_call_result_t hyper_task_inspector_derive_task_group(hyper_native_handle_t inspector,
							   hyper_native_handle_t task_group)
{
	return inspector_derive(HYPER_NATIVE_SYS_TASK_INSPECTOR_DERIVE_TASK_GROUP, inspector,
				task_group);
}

hyper_call_result_t
hyper_task_inspector_derive_resource_domain(hyper_native_handle_t inspector,
					    hyper_native_handle_t resource_domain)
{
	return inspector_derive(HYPER_NATIVE_SYS_TASK_INSPECTOR_DERIVE_RESOURCE_DOMAIN, inspector,
				resource_domain);
}

hyper_call_result_t hyper_object_inspector_derive_process(hyper_native_handle_t inspector,
							  hyper_native_handle_t process)
{
	return inspector_derive(HYPER_NATIVE_SYS_OBJECT_INSPECTOR_DERIVE_PROCESS, inspector,
				process);
}

hyper_call_result_t hyper_object_inspector_derive_task_group(hyper_native_handle_t inspector,
							     hyper_native_handle_t task_group)
{
	return inspector_derive(HYPER_NATIVE_SYS_OBJECT_INSPECTOR_DERIVE_TASK_GROUP, inspector,
				task_group);
}

hyper_call_result_t
hyper_object_inspector_derive_resource_domain(hyper_native_handle_t inspector,
					      hyper_native_handle_t resource_domain)
{
	return inspector_derive(HYPER_NATIVE_SYS_OBJECT_INSPECTOR_DERIVE_RESOURCE_DOMAIN, inspector,
				resource_domain);
}

hyper_native_status_t hyper_thread_yield(void)
{
	return call0(HYPER_NATIVE_SYS_THREAD_YIELD).status;
}

_Noreturn void hyper_thread_exit(int64_t status)
{
	(void)hyper_native_call6(HYPER_NATIVE_SYS_THREAD_EXIT, (uint64_t)status, 0, 0, 0, 0, 0);
	__builtin_trap();
}

_Noreturn void hyper_process_exit(int64_t status)
{
	(void)hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_EXIT, (uint64_t)status, 0, 0, 0, 0, 0);
	__builtin_trap();
}

hyper_call_result_t hyper_thread_create(uint64_t entry, uint64_t initial_sp, uint64_t tls,
					uint64_t argument, const uint64_t *affinity_words,
					size_t affinity_word_count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_THREAD_CREATE, entry, initial_sp, tls, argument,
				  (uintptr_t)affinity_words, affinity_word_count);
}

hyper_native_status_t hyper_thread_start(hyper_native_handle_t thread)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_THREAD_START, thread, 0, 0, 0, 0, 0).status;
}

hyper_native_status_t hyper_thread_request_stop(hyper_native_handle_t thread)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_THREAD_REQUEST_STOP, thread, 0, 0, 0, 0, 0)
		.status;
}

hyper_native_status_t hyper_atomic_wait(const uint32_t *address, uint32_t expected,
					uint64_t deadline)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_ATOMIC_WAIT, (uintptr_t)address, expected,
				  deadline, 0, 0, 0)
		.status;
}

hyper_call_result_t hyper_atomic_wake(const uint32_t *address, uint32_t count)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_ATOMIC_WAKE, (uintptr_t)address, count, 0, 0, 0,
				  0);
}

hyper_native_status_t hyper_thread_sleep(uint64_t deadline)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_THREAD_SLEEP, deadline, 0, 0, 0, 0, 0).status;
}

hyper_call_result_t hyper_file_write_at(hyper_native_handle_t file, uint32_t options,
					uint64_t offset, const void *input, size_t size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_WRITE_AT, file, options, offset,
				  (uintptr_t)input, size, 0);
}

hyper_call_result_t hyper_file_resize(hyper_native_handle_t file, uint64_t size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_RESIZE, file, size, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_create_file(hyper_native_handle_t directory, const void *path,
						size_t path_size, uint64_t rights, uint32_t mode)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_CREATE_FILE, directory,
				  (uintptr_t)path, path_size, rights, mode, 0);
}

hyper_call_result_t hyper_directory_create_directory(hyper_native_handle_t directory,
						     const void *path, size_t path_size,
						     uint32_t mode)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_CREATE_DIRECTORY, directory,
				  (uintptr_t)path, path_size, mode, 0, 0);
}

hyper_call_result_t hyper_directory_remove(hyper_native_handle_t directory, const void *path,
					   size_t path_size, uint32_t options)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_REMOVE, directory, (uintptr_t)path,
				  path_size, options, 0, 0);
}

hyper_call_result_t hyper_wait_set_create(size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_WAIT_SET_CREATE, capacity, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_wait_set_add(hyper_native_handle_t set, hyper_native_handle_t source,
				       uint64_t signals)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_WAIT_SET_ADD, set, source, signals, 0, 0, 0);
}

hyper_call_result_t hyper_wait_set_rearm(hyper_native_handle_t set, uint64_t registration)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_WAIT_SET_REARM, set, registration, 0, 0, 0, 0);
}

hyper_call_result_t hyper_wait_set_remove(hyper_native_handle_t set, uint64_t registration)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_WAIT_SET_REMOVE, set, registration, 0, 0, 0, 0);
}

hyper_call_result_t hyper_wait_set_wait(hyper_native_handle_t set, uint64_t deadline, void *output,
					size_t output_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_WAIT_SET_WAIT, set, deadline, (uintptr_t)output,
				  output_size, 0, 0);
}

hyper_call_result_t hyper_process_get_current_id(void)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_PROCESS_GET_CURRENT_ID, 0, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_byte_channel_create(void)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_BYTE_CHANNEL_CREATE, 0, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_scope_create(hyper_native_handle_t root,
						 hyper_native_handle_t start, uint64_t rights)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_SCOPE_CREATE, root, start, rights, 0,
				  0, 0);
}

hyper_call_result_t hyper_directory_get_metadata(hyper_native_handle_t directory, const void *path,
						 size_t path_length, uint32_t options,
						 hyper_native_file_metadata_t *output,
						 size_t output_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_GET_METADATA, directory,
				  (uintptr_t)path, path_length, options, (uintptr_t)output,
				  output_size);
}

hyper_call_result_t hyper_file_get_metadata(hyper_native_handle_t file,
					    hyper_native_file_metadata_t *output,
					    size_t output_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_GET_METADATA, file, (uintptr_t)output,
				  output_size, 0, 0, 0);
}

hyper_call_result_t hyper_directory_get_self_metadata(hyper_native_handle_t directory,
						      hyper_native_file_metadata_t *output,
						      size_t output_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_GET_SELF_METADATA, directory,
				  (uintptr_t)output, output_size, 0, 0, 0);
}

hyper_call_result_t hyper_directory_set_metadata(hyper_native_handle_t directory, const void *path,
						 size_t path_length, uint32_t options,
						 const hyper_native_file_metadata_update_t *input,
						 size_t input_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_SET_METADATA, directory,
				  (uintptr_t)path, path_length, options, (uintptr_t)input,
				  input_size);
}

hyper_call_result_t hyper_file_set_metadata(hyper_native_handle_t file,
					    const hyper_native_file_metadata_update_t *input,
					    size_t input_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_SET_METADATA, file, (uintptr_t)input,
				  input_size, 0, 0, 0);
}

hyper_call_result_t hyper_directory_rename(hyper_native_handle_t source, const void *path,
					   size_t path_length, hyper_native_handle_t destination,
					   const void *new_path, size_t new_path_length)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_RENAME, source, (uintptr_t)path,
				  path_length, destination, (uintptr_t)new_path, new_path_length);
}

hyper_call_result_t hyper_directory_link(hyper_native_handle_t source, const void *path,
					 size_t path_length, hyper_native_handle_t destination,
					 const void *new_path, size_t new_path_length)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_LINK, source, (uintptr_t)path,
				  path_length, destination, (uintptr_t)new_path, new_path_length);
}

hyper_call_result_t hyper_directory_symlink(hyper_native_handle_t directory, const void *path,
					    size_t path_length, const void *target,
					    size_t target_length)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_SYMLINK, directory, (uintptr_t)path,
				  path_length, (uintptr_t)target, target_length, 0);
}

hyper_call_result_t hyper_directory_read_link(hyper_native_handle_t directory, const void *path,
					      size_t path_length, void *output, size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_READ_LINK, directory, (uintptr_t)path,
				  path_length, (uintptr_t)output, capacity, 0);
}

hyper_call_result_t hyper_directory_canonicalize(hyper_native_handle_t directory, const void *path,
						 size_t path_length, void *output, size_t capacity)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_CANONICALIZE, directory,
				  (uintptr_t)path, path_length, (uintptr_t)output, capacity, 0);
}

hyper_call_result_t hyper_directory_remove_if(hyper_native_handle_t directory, const void *path,
					      size_t path_length, uint32_t options,
					      uint64_t expected_node_id)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_REMOVE_IF, directory, (uintptr_t)path,
				  path_length, options, expected_node_id, 0);
}

hyper_call_result_t hyper_directory_open_directory_nofollow(hyper_native_handle_t directory,
							    const void *path, size_t path_length,
							    uint64_t rights)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_OPEN_DIRECTORY_NOFOLLOW, directory,
				  (uintptr_t)path, path_length, rights, 0, 0);
}

hyper_call_result_t hyper_file_sync(hyper_native_handle_t file, uint32_t scope)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_SYNC, file, scope, 0, 0, 0, 0);
}

hyper_call_result_t hyper_file_lock(hyper_native_handle_t file, uint32_t mode, uint64_t deadline)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_LOCK, file, mode, deadline, 0, 0, 0);
}

hyper_call_result_t hyper_file_unlock(hyper_native_handle_t file)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_UNLOCK, file, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_clock_get_realtime(void)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_CLOCK_GET_REALTIME, 0, 0, 0, 0, 0, 0);
}

hyper_call_result_t hyper_directory_open_file_with_options(hyper_native_handle_t directory,
							   const void *path, size_t path_length,
							   uint64_t rights, uint32_t options,
							   uint32_t mode)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_DIRECTORY_OPEN_FILE_WITH_OPTIONS, directory,
				  (uintptr_t)path, path_length, rights, options, mode);
}

hyper_call_result_t hyper_virtual_machine_creation_lease_get_platform_info(
	hyper_native_handle_t lease, uint32_t profile,
	hyper_native_virtual_machine_platform_info_t *info)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VIRTUAL_MACHINE_CREATION_LEASE_GET_PLATFORM_INFO,
				  lease, profile, (uintptr_t)info, sizeof(*info), 0, 0);
}

hyper_call_result_t hyper_filesystem_mount_ramfs(hyper_native_handle_t directory, const void *path,
						 size_t path_length)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILESYSTEM_MOUNT_RAMFS, directory,
				  (uintptr_t)path, path_length, 0, 0, 0);
}

hyper_call_result_t
hyper_filesystem_switch_root(hyper_native_handle_t directory,
			     const hyper_native_filesystem_root_switch_t *request)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILESYSTEM_SWITCH_ROOT, directory,
				  (uintptr_t)request, 0, 0, 0, 0);
}

/* Content protection is process-local; closing this token cannot release a
 * separate reference held by a live file-backed mapping. */
hyper_call_result_t hyper_file_protect(hyper_native_handle_t file)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_FILE_PROTECT, file, 0, 0, 0, 0, 0);
}

hyper_native_status_t hyper_vmar_map_file(hyper_native_handle_t vmar, hyper_native_handle_t file,
					  hyper_native_handle_t protection,
					  const hyper_native_private_mapping_t *mapping,
					  size_t mapping_size)
{
	return hyper_native_call6(HYPER_NATIVE_SYS_VMAR_MAP_FILE, vmar, file, protection,
				  (uintptr_t)mapping, mapping_size, 0)
		.status;
}
