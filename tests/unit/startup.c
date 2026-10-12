/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/launch.h>
#include <hyper/startup.h>

#include <assert.h>
#include <stdint.h>

static void parses_complete_initial_stack(void)
{
	static char name[] = "/init";
	static char environment[] = "TERM=hyper";
	hyper_native_startup_handle_t handles[] = {
		{
			.purpose = HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE,
			.flags = 0,
			.handle = 0x0000000200000001,
		},
	};
	uintptr_t stack[] = {
		1,
		(uintptr_t)name,
		0,
		(uintptr_t)environment,
		0,
		HYPER_AUXV_STARTUP_HANDLES,
		(uintptr_t)handles,
		HYPER_AUXV_STARTUP_HANDLE_COUNT,
		1,
		0,
		0,
	};
	hyper_startup_t startup;
	assert(hyper_startup_parse(stack, &startup) == HYPER_NATIVE_STATUS_OK);
	assert(startup.argument_count == 1);
	assert(startup.arguments[0] == name);
	assert(startup.environment_count == 1);
	assert(startup.environment[0] == environment);
	assert(startup.auxiliary_count == 2);
	assert(startup.handle_count == 1);

	hyper_native_handle_t console = 0;
	assert(hyper_startup_find_handle(&startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE,
					 &console) == HYPER_NATIVE_STATUS_OK);
	assert(console == handles[0].handle);
}

static void rejects_inconsistent_metadata(void)
{
	uintptr_t missing_argument_terminator[] = {1, 1, 1, 0, 0, 0};
	hyper_startup_t startup;
	assert(hyper_startup_parse(missing_argument_terminator, &startup) ==
	       HYPER_NATIVE_STATUS_INVALID_ARGUMENT);

	hyper_native_startup_handle_t duplicate[] = {
		{HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE, 0, 1},
		{HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE, 0, 2},
	};
	startup = (hyper_startup_t){
		.handle_count = 2,
		.handles = duplicate,
	};
	hyper_native_handle_t ignored;
	assert(hyper_startup_find_handle(&startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE,
					 &ignored) == HYPER_NATIVE_STATUS_BAD_STATE);

	hyper_native_startup_handle_t flagged = {
		HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE,
		1,
		1,
	};
	startup = (hyper_startup_t){
		.handle_count = 1,
		.handles = &flagged,
	};
	assert(hyper_startup_find_handle(&startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_CONSOLE,
					 &ignored) == HYPER_NATIVE_STATUS_INVALID_ARGUMENT);
}

int main(void)
{
	parses_complete_initial_stack();
	rejects_inconsistent_metadata();
	return 0;
}
