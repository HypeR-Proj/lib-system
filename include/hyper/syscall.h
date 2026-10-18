/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPER_SYSCALL_H
#define HYPER_SYSCALL_H

#include <hyper/native.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hyper_call_result {
	hyper_native_status_t status;
	uint64_t value0;
	uint64_t value1;
} hyper_call_result_t;

hyper_call_result_t hyper_native_call6(uint64_t number, uint64_t argument0, uint64_t argument1,
				       uint64_t argument2, uint64_t argument3, uint64_t argument4,
				       uint64_t argument5);

hyper_call_result_t hyper_abi_query(void);
/* Public scalar configuration; unknown keys return NOT_SUPPORTED. */
hyper_call_result_t hyper_system_config(uint64_t key);
hyper_call_result_t hyper_process_get_current_id(void);
/* Returns absolute nanoseconds from the kernel monotonic clock domain. */
hyper_call_result_t hyper_clock_get_monotonic(void);
hyper_native_status_t hyper_handle_close(hyper_native_handle_t handle);
hyper_call_result_t hyper_handle_duplicate(hyper_native_handle_t source, uint64_t requested_rights);
hyper_call_result_t hyper_handle_replace(hyper_native_handle_t source, uint64_t requested_rights);
hyper_call_result_t hyper_handle_get_info(hyper_native_handle_t handle,
					  hyper_native_handle_info_t *info);
hyper_call_result_t hyper_object_get_basic_info(hyper_native_handle_t handle,
						hyper_native_object_basic_info_t *info);
hyper_call_result_t hyper_object_wait_one(hyper_native_handle_t object, uint64_t signals,
					  uint64_t deadline);
hyper_call_result_t hyper_object_wait_many(const hyper_native_object_wait_item_t *items,
					   size_t item_count, uint64_t deadline);
hyper_native_status_t hyper_byte_channel_write(hyper_native_handle_t endpoint, const void *bytes,
					       size_t byte_count);
hyper_call_result_t hyper_byte_channel_read(hyper_native_handle_t endpoint, void *bytes,
					    size_t byte_capacity);
hyper_call_result_t hyper_byte_channel_create(void);
hyper_call_result_t hyper_capability_channel_create(void);
hyper_native_status_t hyper_capability_channel_try_send(
	hyper_native_handle_t endpoint, const void *bytes, size_t byte_count,
	const hyper_native_capability_disposition_t *dispositions, size_t disposition_count);
hyper_call_result_t hyper_capability_channel_receive(hyper_native_handle_t endpoint,
						     uint64_t deadline, void *bytes,
						     size_t byte_capacity,
						     hyper_native_capability_receive_slot_t *slots,
						     size_t slot_count);
hyper_call_result_t hyper_console_discard_input(hyper_native_handle_t console);
hyper_call_result_t hyper_console_read(hyper_native_handle_t console, void *bytes, size_t capacity);
hyper_call_result_t hyper_console_write(hyper_native_handle_t console, const void *bytes,
					size_t count);
hyper_call_result_t hyper_directory_open_file(hyper_native_handle_t directory, const void *path,
					      size_t path_size, uint64_t requested_rights);
hyper_call_result_t hyper_directory_open_directory(hyper_native_handle_t directory,
						   const void *path, size_t path_size,
						   uint64_t requested_rights);
hyper_call_result_t hyper_directory_begin_read(hyper_native_handle_t directory);
/* Reads a DirectoryStream; zero starts, subsequent cookies are stream-issued. */
hyper_call_result_t hyper_directory_read(hyper_native_handle_t stream, uint64_t cookie,
					 hyper_native_directory_entry_t *records, size_t capacity);
hyper_call_result_t hyper_directory_get_info(hyper_native_handle_t directory,
					     hyper_native_directory_info_t *info);
hyper_call_result_t hyper_file_write_at(hyper_native_handle_t file, uint32_t options,
					uint64_t offset, const void *input, size_t size);
hyper_call_result_t hyper_file_resize(hyper_native_handle_t file, uint64_t size);
hyper_call_result_t hyper_directory_create_file(hyper_native_handle_t directory, const void *path,
						size_t path_size, uint64_t rights, uint32_t mode);
hyper_call_result_t hyper_directory_create_directory(hyper_native_handle_t directory,
						     const void *path, size_t path_size,
						     uint32_t mode);
hyper_call_result_t hyper_directory_remove(hyper_native_handle_t directory, const void *path,
					   size_t path_size, uint32_t options);

hyper_call_result_t hyper_wait_set_create(size_t capacity);
hyper_call_result_t hyper_wait_set_add(hyper_native_handle_t set, hyper_native_handle_t source,
				       uint64_t signals);
hyper_call_result_t hyper_wait_set_rearm(hyper_native_handle_t set, uint64_t registration);
hyper_call_result_t hyper_wait_set_remove(hyper_native_handle_t set, uint64_t registration);
hyper_call_result_t hyper_wait_set_wait(hyper_native_handle_t set, uint64_t deadline, void *output,
					size_t output_size);

hyper_call_result_t hyper_file_read_at(hyper_native_handle_t file, uint64_t offset, void *output,
				       size_t output_capacity);
hyper_call_result_t hyper_file_get_info(hyper_native_handle_t file, hyper_native_file_info_t *info);
hyper_call_result_t hyper_resource_domain_create(hyper_native_handle_t parent,
						 const hyper_native_resource_limits_t *limits);
hyper_call_result_t hyper_task_group_create(hyper_native_handle_t factory,
					    hyper_native_handle_t resource_domain);
hyper_call_result_t
hyper_virtual_machine_creation_lease_create(hyper_native_handle_t authority,
					    hyper_native_handle_t resource_domain);
hyper_call_result_t
hyper_virtual_machine_create(hyper_native_handle_t lease,
			     const hyper_native_virtual_machine_configuration_t *configuration);
hyper_native_status_t hyper_pending_virtual_machine_set_memory(hyper_native_handle_t pending,
							       hyper_native_handle_t vmo);
/* Consumes virtual_serial only when the binding succeeds. */
hyper_native_status_t
hyper_pending_virtual_machine_set_virtual_serial(hyper_native_handle_t pending,
						 hyper_native_handle_t virtual_serial);
hyper_call_result_t hyper_virtual_serial_create(void);
hyper_native_status_t hyper_virtual_serial_register_output(hyper_native_handle_t serial,
							   hyper_native_handle_t buffer);
hyper_call_result_t hyper_virtual_serial_write(hyper_native_handle_t virtual_serial,
					       const void *bytes, size_t byte_count);
hyper_native_status_t
hyper_pending_virtual_machine_set_bootstrap(hyper_native_handle_t pending,
					    const hyper_native_virtual_cpu_bootstrap_t *bootstrap);
hyper_native_status_t hyper_pending_virtual_machine_seal(hyper_native_handle_t pending);
hyper_call_result_t hyper_pending_virtual_machine_install(hyper_native_handle_t pending);
hyper_native_status_t hyper_virtual_cpu_start(hyper_native_handle_t virtual_cpu);
/* Success accepts the mask update; a required source handoff may still be pending. */
hyper_native_status_t hyper_virtual_cpu_set_affinity(hyper_native_handle_t virtual_cpu,
						     const uint64_t *words, size_t word_count);
hyper_native_status_t hyper_pending_virtual_machine_abort(hyper_native_handle_t pending);
hyper_native_status_t hyper_virtual_machine_request_stop(hyper_native_handle_t machine);
hyper_call_result_t hyper_virtual_machine_creation_lease_get_platform_info(
	hyper_native_handle_t lease, uint32_t profile,
	hyper_native_virtual_machine_platform_info_t *info);
hyper_call_result_t hyper_virtual_machine_get_info(hyper_native_handle_t machine,
						   hyper_native_virtual_machine_info_t *info);
hyper_call_result_t hyper_virtual_cpu_get_info(hyper_native_handle_t vcpu,
					       hyper_native_virtual_cpu_info_t *info);
hyper_call_result_t hyper_vmo_create(uint64_t size);
/** Acquire PROTECT; value0 is a process-local token and value1 the stable size.
 * Closing the token releases its reference. Existing mappings hold independent
 * references until unmap or process teardown. This operation retains no pages.
 */
hyper_call_result_t hyper_file_protect(hyper_native_handle_t file);
/** Privately map protected File bytes into the caller's own VMAR.
 * Requires File READ (also EXECUTE for executable mappings), MAP on both VMAR
 * and a matching protection token, and COPY_ON_WRITE mode in the request.
 */
hyper_native_status_t hyper_vmar_map_file(hyper_native_handle_t vmar, hyper_native_handle_t file,
					  hyper_native_handle_t protection,
					  const hyper_native_private_mapping_t *mapping,
					  size_t mapping_size);
hyper_call_result_t hyper_vmo_create_snapshot(hyper_native_handle_t vmo);
hyper_native_status_t hyper_vmar_map_private(hyper_native_handle_t vmar,
					     hyper_native_handle_t snapshot,
					     const hyper_native_private_mapping_t *mapping,
					     size_t mapping_size);
hyper_native_status_t hyper_vmo_read(hyper_native_handle_t vmo, uint64_t offset, void *output,
				     size_t byte_count);
hyper_native_status_t hyper_vmo_write(hyper_native_handle_t vmo, uint64_t offset, const void *input,
				      size_t byte_count);
/* With options zero, address is a low-end hint (zero: lowest free range).
 * EXACT requires that address, including zero. Unknown flags are invalid.
 * Success returns child in value0 and actual base in value1. */
hyper_call_result_t hyper_vmar_allocate(hyper_native_handle_t parent, uintptr_t address,
					size_t size, uint64_t options);
hyper_native_status_t hyper_vmar_map(hyper_native_handle_t vmar, hyper_native_handle_t vmo,
				     uint64_t vmo_offset, uintptr_t address, size_t size,
				     uint32_t permissions);
hyper_native_status_t hyper_vmar_protect(hyper_native_handle_t vmar, uintptr_t address, size_t size,
					 uint32_t permissions);
hyper_native_status_t hyper_vmar_unmap(hyper_native_handle_t vmar, uintptr_t address, size_t size);
hyper_native_status_t hyper_vmar_destroy(hyper_native_handle_t vmar);
hyper_call_result_t hyper_process_builder_create(hyper_native_handle_t factory,
						 hyper_native_handle_t group,
						 hyper_native_handle_t domain,
						 hyper_native_handle_t executable,
						 hyper_native_handle_t loader);
hyper_native_status_t hyper_process_builder_set_name(hyper_native_handle_t builder,
						     const void *name, size_t name_size);
hyper_native_status_t hyper_process_builder_set_data(hyper_native_handle_t builder,
						     const void *data, size_t size);
hyper_native_status_t hyper_process_builder_set_affinity(hyper_native_handle_t builder,
							 const uint64_t *words, size_t word_count);
hyper_native_status_t hyper_process_builder_add_handle(hyper_native_handle_t builder,
						       hyper_native_handle_t source,
						       uint32_t purpose, uint32_t expected_kind,
						       uint64_t rights, uint32_t operation);
hyper_native_status_t hyper_process_builder_seal(hyper_native_handle_t builder);
hyper_call_result_t hyper_process_builder_start(hyper_native_handle_t builder);
hyper_native_status_t hyper_process_builder_abort(hyper_native_handle_t builder);
hyper_call_result_t hyper_process_derive_observer(hyper_native_handle_t process);
hyper_native_status_t hyper_task_group_request_stop(hyper_native_handle_t group);
hyper_native_status_t hyper_process_request_stop(hyper_native_handle_t process);

/* Supervisor-controlled retention. Views are revoked by explicit release,
 * controller loss or the configured deadline; no target page-in occurs. */
hyper_call_result_t hyper_process_builder_retain(hyper_native_handle_t builder, uint64_t max_bytes, uint64_t timeout_ns);
hyper_call_result_t hyper_process_retirement_open_view(hyper_native_handle_t control);
hyper_call_result_t hyper_process_retirement_release(hyper_native_handle_t control);
hyper_call_result_t hyper_crash_view_read(hyper_native_handle_t view, uintptr_t address, void *output, size_t length);
hyper_call_result_t hyper_process_retirement_get_info(hyper_native_handle_t object, hyper_native_process_retirement_info_t *output);
hyper_call_result_t hyper_crash_view_get_info(hyper_native_handle_t object, hyper_native_process_retirement_info_t *output);
hyper_call_result_t hyper_crash_view_thread(hyper_native_handle_t object, uint64_t index, hyper_native_crash_thread_t *output);
hyper_call_result_t hyper_crash_view_mapping(hyper_native_handle_t object, uint64_t index, hyper_native_crash_mapping_t *output);
hyper_call_result_t hyper_crash_view_handle(hyper_native_handle_t object, uint64_t index, hyper_native_crash_handle_t *output);

hyper_call_result_t hyper_process_get_info(hyper_native_handle_t process,
					   hyper_native_process_info_t *info);
hyper_call_result_t hyper_task_inspector_scan_processes(hyper_native_handle_t inspector,
							uint64_t cursor,
							hyper_native_task_process_t *records,
							size_t capacity);
hyper_call_result_t hyper_task_inspector_scan_threads(hyper_native_handle_t inspector,
						      uint64_t cursor,
						      hyper_native_task_thread_t *records,
						      size_t capacity);
/* Requires INSPECT | INSPECT_DETAILS. process == 0 selects a global KOID;
 * otherwise target is a full process-local handle. cursor starts at zero. */
hyper_call_result_t hyper_object_inspector_read_details(hyper_native_handle_t inspector,
							uint64_t process, uint64_t target,
							uint64_t cursor,
							hyper_native_object_details_t *record);
hyper_call_result_t hyper_object_inspector_scan_objects(hyper_native_handle_t inspector,
							uint64_t cursor,
							hyper_native_object_inspection_t *records,
							size_t capacity);
hyper_call_result_t hyper_object_inspector_scan_handles(hyper_native_handle_t inspector,
							uint64_t process_koid, uint64_t cursor,
							hyper_native_handle_inspection_t *records,
							size_t capacity);
hyper_call_result_t hyper_memory_inspector_read(hyper_native_handle_t inspector,
						hyper_native_memory_observation_t *observation);
hyper_call_result_t hyper_cpu_inspector_read(hyper_native_handle_t inspector,
					     hyper_native_cpu_observation_t *observation);
hyper_call_result_t hyper_task_inspector_derive_process(hyper_native_handle_t inspector,
							hyper_native_handle_t process);
hyper_call_result_t hyper_task_inspector_derive_task_group(hyper_native_handle_t inspector,
							   hyper_native_handle_t task_group);
hyper_call_result_t
hyper_task_inspector_derive_resource_domain(hyper_native_handle_t inspector,
					    hyper_native_handle_t resource_domain);
hyper_call_result_t hyper_object_inspector_derive_process(hyper_native_handle_t inspector,
							  hyper_native_handle_t process);
hyper_call_result_t hyper_object_inspector_derive_task_group(hyper_native_handle_t inspector,
							     hyper_native_handle_t task_group);
hyper_call_result_t
hyper_object_inspector_derive_resource_domain(hyper_native_handle_t inspector,
					      hyper_native_handle_t resource_domain);
/* initial_sp is the initial user stack pointer for a downward-growing stack, normally
 * its exclusive upper bound; it must satisfy the architecture's SP alignment.
 * No stack size is supplied; the caller owns the mappings and guard pages.
 * affinity_words is a little-endian CPU-mask array. NULL, 0 inherits the
 * calling thread's affinity; an explicit mask must be nonempty and nonzero. */
hyper_call_result_t hyper_thread_create(uint64_t entry, uint64_t initial_sp, uint64_t tls,
					uint64_t argument, const uint64_t *affinity_words,
					size_t affinity_word_count);
hyper_native_status_t hyper_thread_start(hyper_native_handle_t thread);
/* Diagnostic name of the caller only: 1..64 UTF-8 bytes, no control characters
 * or trailing NUL. Invalid input leaves the old name unchanged. */
hyper_native_status_t hyper_thread_set_current_name(const void *name, size_t length);
hyper_native_status_t hyper_thread_request_stop(hyper_native_handle_t thread);
hyper_native_status_t hyper_atomic_wait(const uint32_t *address, uint32_t expected,
					uint64_t deadline);
hyper_call_result_t hyper_atomic_wake(const uint32_t *address, uint32_t count);
hyper_native_status_t hyper_thread_sleep(uint64_t deadline);
hyper_native_status_t hyper_thread_yield(void);
_Noreturn void hyper_thread_exit(int64_t status);
_Noreturn void hyper_process_exit(int64_t status);

/* Namespace topology requires MANAGE_NAMESPACE | WRITE | EXECUTE on a live
 * root Directory anchor. All borrowed request memory remains valid until return. */
hyper_call_result_t hyper_filesystem_mount_ramfs(hyper_native_handle_t directory, const void *path,
						 size_t path_length);
hyper_call_result_t
hyper_filesystem_switch_root(hyper_native_handle_t directory,
			     const hyper_native_filesystem_root_switch_t *request);

hyper_call_result_t hyper_directory_scope_create(hyper_native_handle_t root,
						 hyper_native_handle_t start, uint64_t rights);
hyper_call_result_t hyper_directory_get_metadata(hyper_native_handle_t directory, const void *path,
						 size_t path_length, uint32_t options,
						 hyper_native_file_metadata_t *output,
						 size_t output_size);
hyper_call_result_t hyper_file_get_metadata(hyper_native_handle_t file,
					    hyper_native_file_metadata_t *output,
					    size_t output_size);
hyper_call_result_t hyper_directory_get_self_metadata(hyper_native_handle_t directory,
						      hyper_native_file_metadata_t *output,
						      size_t output_size);
hyper_call_result_t hyper_directory_set_metadata(hyper_native_handle_t directory, const void *path,
						 size_t path_length, uint32_t options,
						 const hyper_native_file_metadata_update_t *input,
						 size_t input_size);
hyper_call_result_t hyper_file_set_metadata(hyper_native_handle_t file,
					    const hyper_native_file_metadata_update_t *input,
					    size_t input_size);
hyper_call_result_t hyper_directory_rename(hyper_native_handle_t source, const void *path,
					   size_t path_length, hyper_native_handle_t destination,
					   const void *new_path, size_t new_path_length);
hyper_call_result_t hyper_directory_link(hyper_native_handle_t source, const void *path,
					 size_t path_length, hyper_native_handle_t destination,
					 const void *new_path, size_t new_path_length);
hyper_call_result_t hyper_directory_symlink(hyper_native_handle_t directory, const void *path,
					    size_t path_length, const void *target,
					    size_t target_length);
hyper_call_result_t hyper_directory_read_link(hyper_native_handle_t directory, const void *path,
					      size_t path_length, void *output, size_t capacity);
hyper_call_result_t hyper_directory_canonicalize(hyper_native_handle_t directory, const void *path,
						 size_t path_length, void *output, size_t capacity);
hyper_call_result_t hyper_directory_remove_if(hyper_native_handle_t directory, const void *path,
					      size_t path_length, uint32_t options,
					      uint64_t expected_node_id);
hyper_call_result_t hyper_directory_open_directory_nofollow(hyper_native_handle_t directory,
							    const void *path, size_t path_length,
							    uint64_t rights);
hyper_call_result_t hyper_file_sync(hyper_native_handle_t file, uint32_t scope);
hyper_call_result_t hyper_file_lock(hyper_native_handle_t file, uint32_t mode, uint64_t deadline);
hyper_call_result_t hyper_file_unlock(hyper_native_handle_t file);
hyper_call_result_t hyper_clock_get_realtime(void);
hyper_call_result_t hyper_directory_open_file_with_options(hyper_native_handle_t directory,
							   const void *path, size_t path_length,
							   uint64_t rights, uint32_t options,
							   uint32_t mode);
/* Residency is independent of DMA pinning. CURRENT selects all existing
 * mappings of this VMAR, with address=size=0; future mappings are unaffected. */
hyper_native_status_t hyper_vmar_lock_memory(hyper_native_handle_t vmar, uintptr_t address,
					     size_t size, uint64_t flags);
hyper_native_status_t hyper_vmar_unlock_memory(hyper_native_handle_t vmar, uintptr_t address,
					       size_t size, uint64_t flags);
hyper_native_status_t hyper_process_builder_set_memory_lock(hyper_native_handle_t builder,
							    int enabled);
hyper_call_result_t
hyper_filesystem_begin_root_switch(hyper_native_handle_t directory,
				   const hyper_native_filesystem_root_switch_t *request);
hyper_native_status_t hyper_filesystem_finish_root_switch(hyper_native_handle_t transition,
							  int commit);

#ifdef __cplusplus
}
#endif
#endif /* HYPER_SYSCALL_H */
