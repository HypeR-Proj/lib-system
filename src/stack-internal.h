/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_STACK_INTERNAL_H
#define HYPER_STACK_INTERNAL_H
#include <hyper/stack.h>
hyper_native_status_t hyper_stack_initialize(const hyper_startup_t *startup,
					     hyper_stack_t **initial);
void hyper_stack_claim_runtime(hyper_stack_t *stack);
void hyper_stack_release_runtime(hyper_stack_t *stack);
hyper_native_status_t hyper_runtime_thread_attach_stack(hyper_stack_t *stack);
#endif
