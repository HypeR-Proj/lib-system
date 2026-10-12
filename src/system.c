/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <hyper/system.h>
#include <hyper/syscall.h>
#include <stdatomic.h>
#include <stdint.h>

static atomic_size_t cached_page_size;

hyper_native_status_t hyper_page_size(size_t *result)
{
	if (!result)
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	size_t page = atomic_load_explicit(&cached_page_size, memory_order_relaxed);
	if (!page) {
		hyper_call_result_t query =
			hyper_system_config(HYPER_NATIVE_SYSTEM_CONFIG_PAGE_SIZE);
		if (query.status != HYPER_NATIVE_STATUS_OK)
			return query.status;
		/* Alignment arithmetic and two guard pages must remain representable.
		 * The allocator also requires a page to contain max_align_t alignment. */
		if (query.value0 < _Alignof(max_align_t) || query.value0 > SIZE_MAX / 2 ||
		    (query.value0 & (query.value0 - 1)) || query.value1)
			return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
		page = (size_t)query.value0;
		/* This immutable scalar publishes no other memory; racing initial
		 * queries return the same kernel property. No blocking startup lock. */
		atomic_store_explicit(&cached_page_size, page, memory_order_relaxed);
	}
	*result = page;
	return HYPER_NATIVE_STATUS_OK;
}
