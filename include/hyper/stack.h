/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_STACK_H
#define HYPER_STACK_H

#include <hyper/startup.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hyper_stack hyper_stack_t;

typedef struct hyper_stack_info {
	uintptr_t base;	 /* First currently usable byte. */
	uintptr_t top;	 /* Fixed, exclusive top; initial SP for a new thread. */
	size_t size;	 /* Currently usable bytes, excluding guards. */
	size_t capacity; /* Maximum usable bytes in this reservation. */
} hyper_stack_info_t;

/* Reserve a dedicated VMAR with an unmapped page at each end. Size and
 * capacity round up to Native pages. Reservation capacity is at least 256 MiB
 * and at least the initial size. A nonzero capacity smaller than size is invalid.
 * Placement prefers the high application range, not a fixed SDK arena.
 * Only size bytes are initially mapped; reservation is not physical memory.
 * Capacity is fixed at creation and is the hard limit for in-place growth;
 * request a larger capacity here if needed. Guard pages are additional.
 * A successful create returns an owned descriptor, not a malloc pointer. */
hyper_native_status_t hyper_stack_create(size_t size, size_t capacity, hyper_stack_t **result);
hyper_native_status_t hyper_stack_get_info(hyper_stack_t *stack, hyper_stack_info_t *result);
/* Extend downwards without moving any existing byte or changing SP/top.
 * Size must not exceed the capacity reported by hyper_stack_get_info();
 * this operation cannot enlarge the reservation, even into adjacent free VA.
 * Failure leaves the old usable extent intact. No shrink or automatic fault
 * growth: call before exhausting the old stack, with sufficient call headroom.
 * Grow/query serialize internally; descriptor destruction requires exclusive
 * ownership and proof that no thread can execute on this stack again. */
hyper_native_status_t hyper_stack_grow(hyper_stack_t *stack, size_t size);
/* Runtime-owned/current stacks cannot be destroyed through this API.
 * Failure retains ownership for a later destroy retry; the payload may already
 * be unmapped and grow/query then return BAD_STATE. Never reuse that stack. */
hyper_native_status_t hyper_stack_destroy(hyper_stack_t *stack);
/* Borrowed descriptor: main and SDK-created threads use the same interface.
 * It is valid only while its owning thread remains alive; retaining this
 * pointer does not retain the stack or permit access after thread termination.
 * NULL for a raw Native thread which has not attached a runtime stack. */
hyper_stack_t *hyper_stack_current(void);

#ifdef __cplusplus
}
#endif
#endif
