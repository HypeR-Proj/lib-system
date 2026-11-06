/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_VDSO_H
#define HYPER_VDSO_H
#include <hyper/syscall.h>
#include <hyper/startup.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Loader helper: consumes the optional paired handles, maps RX code and RO/NX
 * observations, and returns the ELF base or zero for syscall fallback. */
uintptr_t hyper_vdso_map(hyper_native_handle_t root, hyper_native_handle_t image,
			 hyper_native_handle_t data);
/* Runtime initialization before constructors; each missing symbol falls back. */
void hyper_vdso_initialize(const hyper_startup_t *startup);
hyper_call_result_t hyper_vdso_monotonic(void);
hyper_call_result_t hyper_vdso_realtime(void);
/* Advisory only. Missing vDSO returns true to retain authoritative annotations. */
int hyper_trace_ipc_active(void);
#ifdef __cplusplus
}
#endif
#endif
