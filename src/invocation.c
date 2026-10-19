/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/invocation.h>

/* Written only by loader/CRT initialization, before constructors or threads. */
static hyper_native_handle_t execution_context;
static hyper_native_handle_t object_factory;
static hyper_native_handle_t system_info;

hyper_native_handle_t hyper_native_execution_context(void)
{
	return execution_context;
}

hyper_native_handle_t hyper_native_object_factory(void)
{
	return object_factory;
}

hyper_native_handle_t hyper_native_system_info(void)
{
	return system_info;
}

hyper_native_status_t hyper_native_bind_invocation_handles(hyper_native_handle_t execution,
							   hyper_native_handle_t factory,
							   hyper_native_handle_t system)
{
	if (!execution || !factory || !system || execution == factory || execution == system ||
	    factory == system)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	if (execution_context) {
		if (execution_context != execution || object_factory != factory || system_info != system)
			return HYPER_NATIVE_STATUS_BAD_STATE;
		return HYPER_NATIVE_STATUS_OK;
	}
	execution_context = execution;
	object_factory = factory;
	system_info = system;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_native_initialize_invocation(const hyper_startup_t *startup)
{
	hyper_native_handle_t execution, factory, system;
	hyper_native_status_t status = hyper_startup_find_handle(
		startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_EXECUTION_CONTEXT, &execution);
	if (status)
		return status;
	status = hyper_startup_find_handle(
		startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_OBJECT_FACTORY, &factory);
	if (status)
		return status;
	status = hyper_startup_find_handle(startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_SYSTEM_INFO,
					   &system);
	if (status)
		return status;
	return hyper_native_bind_invocation_handles(execution, factory, system);
}
