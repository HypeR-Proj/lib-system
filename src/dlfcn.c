/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Static-link fallback for the runtime-loader API. A dynamically interpreted
 * process resolves these names from its interpreter before libhyper. Keeping
 * the fallback in libhyper makes the link contract uniform while reporting a
 * deterministic failure when a static image has no runtime linker.
 */

#include <hyper/dlfcn.h>
#include <stddef.h>

static const char unavailable[] = "dynamic loader unavailable";
static unsigned char error_pending;

void *hyper_dlopen_at(hyper_native_handle_t directory, const char *name, uint32_t flags)
{
	(void)directory;
	(void)name;
	(void)flags;
	__atomic_store_n(&error_pending, 1, __ATOMIC_RELEASE);
	return NULL;
}

void *hyper_dlsym(void *object, const char *name)
{
	(void)object;
	(void)name;
	__atomic_store_n(&error_pending, 1, __ATOMIC_RELEASE);
	return NULL;
}

hyper_native_status_t hyper_dlclose(void *object)
{
	(void)object;
	__atomic_store_n(&error_pending, 1, __ATOMIC_RELEASE);
	return HYPER_NATIVE_STATUS_NOT_SUPPORTED;
}

const char *hyper_dlerror(void)
{
	return __atomic_exchange_n(&error_pending, 0, __ATOMIC_ACQ_REL) != 0 ? unavailable : NULL;
}
