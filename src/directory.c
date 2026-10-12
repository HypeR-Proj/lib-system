/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/startup.h>
#include <hyper/syscall.h>
#include <hyper/thread.h>
#include <stdatomic.h>
#include "mutex-internal.h"

/* Root and cwd owners live in libhyper, shared by all language shims/DSOs.
 * Acquirers duplicate under the mutex; replace releases the old owner after
 * unlocking. No borrowed handle can race a cwd change and close. */
static hyper_mutex_t cwd_lock;
static hyper_native_handle_t normalized_root;
static hyper_native_handle_t current;

static hyper_native_status_t scope_from(hyper_native_handle_t root, hyper_native_handle_t start,
					hyper_native_handle_t *output)
{
	*output = 0;
	hyper_native_handle_info_t root_info = {0}, start_info = {0};
	int64_t status = hyper_handle_get_info(root, &root_info).status;
	if (status)
		return status;
	status = hyper_handle_get_info(start, &start_info).status;
	if (status)
		return status;
	hyper_call_result_t result =
		hyper_directory_scope_create(root, start, root_info.rights & start_info.rights);
	if (!result.status)
		*output = result.value0;
	return result.status;
}

static hyper_native_status_t duplicate(hyper_native_handle_t source, hyper_native_handle_t *output)
{
	*output = 0;
	hyper_native_handle_info_t info = {0};
	int64_t status = hyper_handle_get_info(source, &info).status;
	if (status)
		return status;
	hyper_call_result_t copy = hyper_handle_duplicate(source, info.rights);
	if (!copy.status)
		*output = copy.value0;
	return copy.status;
}

/* Called under cwd_lock. A startup Directory may itself be a rooted cursor.
 * Select its current node once as the std process root; otherwise relative
 * resolution and absolute resolution could select different namespaces. */
static hyper_native_status_t prepare_root_locked(void)
{
	if (normalized_root)
		return 0;
	uint64_t supplied =
		hyper_runtime_capability(HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_DIRECTORY);
	if (!supplied)
		return HYPER_NATIVE_STATUS_ACCESS_DENIED;
	return scope_from(supplied, supplied, &normalized_root);
}

hyper_native_status_t hyper_runtime_directory_root(hyper_native_handle_t *output)
{
	*output = 0;
	hyper_mutex_lock(&cwd_lock);
	int64_t status = prepare_root_locked();
	if (!status)
		status = duplicate(normalized_root, output);
	hyper_mutex_unlock(&cwd_lock);
	return status;
}

hyper_native_status_t hyper_runtime_directory_scope(hyper_native_handle_t start,
						    hyper_native_handle_t *output)
{
	*output = 0;
	uint64_t root = 0;
	int64_t status = hyper_runtime_directory_root(&root);
	if (status)
		return status;
	status = scope_from(root, start, output);
	(void)hyper_handle_close(root);
	return status;
}

hyper_native_status_t hyper_runtime_directory_acquire(const char *path, size_t size,
						      hyper_native_handle_t *output)
{
	*output = 0;
	if (!size || size > HYPER_NATIVE_DIRECTORY_MAX_PATH_BYTES)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (path[0] == '/')
		return hyper_runtime_directory_root(output);
	hyper_mutex_lock(&cwd_lock);
	int64_t status = prepare_root_locked();
	if (!status && !current) {
		uint64_t start = hyper_runtime_capability(UINT32_C(0x80040001));
		if (!start)
			start = normalized_root;
		status = scope_from(normalized_root, start, &current);
	}
	if (!status)
		status = duplicate(current, output);
	hyper_mutex_unlock(&cwd_lock);
	return status;
}

hyper_native_status_t hyper_runtime_directory_change(const char *path, size_t size)
{
	uint64_t base = 0;
	int64_t status = hyper_runtime_directory_acquire(path, size, &base);
	if (status)
		return status;
	hyper_native_handle_info_t info = {0};
	status = hyper_handle_get_info(base, &info).status;
	hyper_call_result_t opened = {0};
	if (!status) {
		opened = hyper_directory_open_directory(base, path, size, info.rights);
		status = opened.status;
	}
	(void)hyper_handle_close(base);
	if (status)
		return status;
	uint64_t replacement = 0;
	status = hyper_runtime_directory_scope(opened.value0, &replacement);
	(void)hyper_handle_close(opened.value0);
	if (status)
		return status;
	hyper_mutex_lock(&cwd_lock);
	uint64_t previous = current;
	current = replacement;
	hyper_mutex_unlock(&cwd_lock);
	if (previous)
		(void)hyper_handle_close(previous);
	return 0;
}
