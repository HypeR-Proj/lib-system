/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_INVOCATION_H
#define HYPER_INVOCATION_H

#include <hyper/startup.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Borrowed process-lifetime capabilities owned by the runtime. Do not close
 * or replace these values: invalidating their generation makes subsequent
 * context-dependent SDK calls fail, without automatic rebinding or recreation.
 * No-return exit wrappers trap on failure. For reduced rights, duplicate a
 * separate attenuated handle and pass it explicitly to hyper_native_invoke6.
 * Operations use ordinary handle lookup and rights checks; there is no ambient
 * or zero-handle fallback before initialization. */
hyper_native_handle_t hyper_native_execution_context(void);
hyper_native_handle_t hyper_native_object_factory(void);
hyper_native_handle_t hyper_native_system_info(void);

/* Startup only, before constructors or secondary threads exist. Binding the
 * same tuple again is a no-op; replacing an established binding is rejected. */
hyper_native_status_t hyper_native_bind_invocation_handles(hyper_native_handle_t execution,
							   hyper_native_handle_t factory,
							   hyper_native_handle_t system);
hyper_native_status_t hyper_native_initialize_invocation(const hyper_startup_t *startup);

#ifdef __cplusplus
}
#endif
#endif /* HYPER_INVOCATION_H */
