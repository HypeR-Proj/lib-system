/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPER_DLFCN_H
#define HYPER_DLFCN_H

#include <hyper/native.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HYPER_RTLD_NOW UINT32_C(0x00000001)
#define HYPER_RTLD_LOCAL UINT32_C(0x00000002)
#define HYPER_RTLD_GLOBAL UINT32_C(0x00000004)

void *hyper_dlopen_at(hyper_native_handle_t directory, const char *name, uint32_t flags);
void *hyper_dlsym(void *object, const char *name);
hyper_native_status_t hyper_dlclose(void *object);
const char *hyper_dlerror(void);

#ifdef __cplusplus
}
#endif

#endif
