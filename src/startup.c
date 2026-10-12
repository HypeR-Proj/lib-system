/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/launch.h>
#include <hyper/startup.h>
#include <stdbool.h>

enum {
	MAXIMUM_ARGUMENTS = 4096,
	MAXIMUM_ENVIRONMENT = 4096,
	MAXIMUM_AUXILIARY_ENTRIES = 256,
	MAXIMUM_STARTUP_HANDLES = 256,
};

static bool bounded_string(const char *string, size_t maximum_bytes)
{
	if (string == NULL) {
		return false;
	}
	for (size_t index = 0; index <= maximum_bytes; ++index) {
		if (string[index] == '\0') {
			return true;
		}
	}
	return false;
}

static hyper_native_status_t parse_auxiliary(const hyper_auxiliary_entry_t *auxiliary,
					     hyper_startup_t *startup)
{
	uintptr_t handles = 0;
	uintptr_t handle_count = 0;
	bool saw_handles = false;
	bool saw_handle_count = false;

	for (size_t index = 0; index < MAXIMUM_AUXILIARY_ENTRIES; ++index) {
		const hyper_auxiliary_entry_t entry = auxiliary[index];
		if (entry.key == 0) {
			startup->auxiliary = auxiliary;
			startup->auxiliary_count = index;
			if (saw_handles != saw_handle_count ||
			    handle_count > MAXIMUM_STARTUP_HANDLES) {
				return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
			}
			if (!saw_handles) {
				return HYPER_NATIVE_STATUS_OK;
			}
			if ((handles == 0 && handle_count != 0) ||
			    handles % _Alignof(hyper_native_startup_handle_t) != 0) {
				return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
			}
			startup->handles = (const hyper_native_startup_handle_t *)handles;
			startup->handle_count = (size_t)handle_count;
			return HYPER_NATIVE_STATUS_OK;
		}
		if (entry.key == HYPER_AUXV_STARTUP_HANDLES) {
			if (saw_handles) {
				return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
			}
			handles = entry.value;
			saw_handles = true;
		} else if (entry.key == HYPER_AUXV_STARTUP_HANDLE_COUNT) {
			if (saw_handle_count) {
				return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
			}
			handle_count = entry.value;
			saw_handle_count = true;
		}
	}
	return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
}

hyper_native_status_t hyper_startup_parse(const uintptr_t *initial_stack, hyper_startup_t *startup)
{
	if (initial_stack == NULL || startup == NULL) {
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	}
	*startup = (hyper_startup_t){0};
	const uintptr_t raw_argument_count = initial_stack[0];
	if (raw_argument_count > MAXIMUM_ARGUMENTS) {
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	}
	const size_t argument_count = (size_t)raw_argument_count;
	char *const *arguments = (char *const *)(initial_stack + 1);
	if (arguments[argument_count] != NULL) {
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	}
	for (size_t index = 0; index < argument_count; ++index) {
		if (!bounded_string(arguments[index], HYPER_LAUNCH_MAX_STRING_BYTES)) {
			return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
		}
	}

	char *const *environment = arguments + argument_count + 1;
	size_t environment_count = 0;
	while (environment_count < MAXIMUM_ENVIRONMENT && environment[environment_count] != NULL) {
		++environment_count;
	}
	if (environment_count == MAXIMUM_ENVIRONMENT) {
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	}
	for (size_t index = 0; index < environment_count; ++index) {
		if (!bounded_string(environment[index],
				    HYPER_LAUNCH_MAX_STRING_BYTES)) {
			return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
		}
	}

	startup->argument_count = argument_count;
	startup->arguments = arguments;
	startup->environment_count = environment_count;
	startup->environment = environment;
	const hyper_auxiliary_entry_t *auxiliary =
		(const hyper_auxiliary_entry_t *)(environment + environment_count + 1);
	return parse_auxiliary(auxiliary, startup);
}

hyper_native_status_t hyper_startup_find_handle(const hyper_startup_t *startup, uint32_t purpose,
						hyper_native_handle_t *handle)
{
	if (startup == NULL || handle == NULL || purpose == 0 ||
	    (startup->handle_count != 0 && startup->handles == NULL)) {
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	}
	bool found = false;
	hyper_native_handle_t value = 0;
	for (size_t index = 0; index < startup->handle_count; ++index) {
		const hyper_native_startup_handle_t record = startup->handles[index];
		if (record.flags != 0 || record.handle == 0) {
			return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
		}
		if (record.purpose != purpose) {
			continue;
		}
		if (found) {
			return HYPER_NATIVE_STATUS_BAD_STATE;
		}
		value = record.handle;
		found = true;
	}
	if (!found) {
		return HYPER_NATIVE_STATUS_BAD_HANDLE;
	}
	*handle = value;
	return HYPER_NATIVE_STATUS_OK;
}
