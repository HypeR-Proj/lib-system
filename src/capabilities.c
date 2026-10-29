/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/startup.h>
#include <hyper/launch.h>
#include <hyper/syscall.h>

/* These process-lifetime owners isolate language-runtime use from handles
 * explicitly taken and closed by Native application code. No ambient
 * authority is created: an input must authorize duplication. */
static const uint32_t purposes[] = {
	HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_DIRECTORY,
	UINT32_C(0x80040001), /* conventional working directory */
	HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_TASK_FACTORY,
	HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_TASK_GROUP,
	HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_RESOURCE_DOMAIN,
	HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_DYNAMIC_LIBRARY_DIRECTORY,
	UINT32_C(0x80040002), /* child library directory */
	HYPER_LAUNCH_CHILD_LOADER,
	UINT32_C(0x80030001),
	UINT32_C(0x80030002),
	UINT32_C(0x80030003),
	UINT32_C(0x80030011), /* generic input connector */
	UINT32_C(0x80030012), /* generic output connector */
	UINT32_C(0x80030013), /* generic error connector */
};
static hyper_native_handle_t retained[sizeof(purposes) / sizeof(purposes[0])];

hyper_native_status_t hyper_runtime_capabilities_initialize(const hyper_startup_t *startup)
{
	for (size_t i = 0; i < sizeof(purposes) / sizeof(purposes[0]); ++i) {
		hyper_native_handle_t source;
		if (hyper_startup_find_handle(startup, purposes[i], &source) !=
		    HYPER_NATIVE_STATUS_OK)
			continue;
		hyper_native_handle_info_t info = {0};
		if (hyper_handle_get_info(source, &info, sizeof(hyper_native_handle_info_t))
			    .status != HYPER_NATIVE_STATUS_OK)
			continue;
		if (!(info.rights & HYPER_NATIVE_RIGHT_DUPLICATE))
			continue;
		hyper_call_result_t copy = hyper_handle_duplicate(source, info.rights);
		if (copy.status != HYPER_NATIVE_STATUS_OK) {
			for (size_t j = 0; j < i; ++j) {
				if (retained[j])
					(void)hyper_handle_close(retained[j]);
				retained[j] = 0;
			}
			return copy.status;
		}
		retained[i] = copy.value0;
	}
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_handle_t hyper_runtime_capability(uint32_t purpose)
{
	for (size_t i = 0; i < sizeof(purposes) / sizeof(purposes[0]); ++i)
		if (purpose == purposes[i])
			return retained[i];
	return 0;
}
