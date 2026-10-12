/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/heap.h>
#include <hyper/syscall.h>
#include <hyper/system.h>
#include <stdatomic.h>
#include "mutex-internal.h"
#include <stdbool.h>
#include <string.h>

/* One process heap. Region/block metadata lives in the mappings it describes;
 * allocator bookkeeping never allocates recursively. All topology is locked.
 * First-fit blocks split and coalesce; wholly free regions are unmapped. */
#define BLOCK_ALIGNMENT _Alignof(max_align_t)
#define REGION_GRANULE ((size_t)65536)
static size_t page_size;

typedef struct region region_t;
typedef struct block block_t;

struct block {
	size_t span;
	size_t requested;
	block_t *previous;
	block_t *next;
	region_t *region;
	bool available;
};

struct region {
	size_t size;
	hyper_native_handle_t vmar;
	region_t *next;
};

static hyper_mutex_t heap_lock;
static hyper_native_handle_t heap_vmar, root_vmar;
static uintptr_t heap_base;
static size_t heap_size;
static region_t *regions;

static void require_success(hyper_native_status_t status)
{
	if (status != HYPER_NATIVE_STATUS_OK) {
		hyper_process_exit(HYPER_NATIVE_STATUS_INTERNAL);
	}
}

/* All alignments supplied here are nonzero powers of two. */
static bool round_up(size_t value, size_t alignment, size_t *result)
{
	if (value > SIZE_MAX - (alignment - 1)) {
		return false;
	}
	*result = (value + alignment - 1) & ~(alignment - 1);
	return true;
}

static size_t region_header_size(void)
{
	return (sizeof(region_t) + BLOCK_ALIGNMENT - 1) & ~(BLOCK_ALIGNMENT - 1);
}

static block_t *first_block(region_t *region)
{
	return (block_t *)((unsigned char *)region + region_header_size());
}

hyper_native_status_t hyper_heap_initialize(const hyper_startup_t *startup)
{
	hyper_native_handle_t root;
	hyper_native_status_t status = hyper_startup_find_handle(
		startup, HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_VMAR, &root);
	if (status != HYPER_NATIVE_STATUS_OK) {
		return status;
	}
	hyper_mutex_lock(&heap_lock);
	if (heap_vmar != 0) {
		hyper_mutex_unlock(&heap_lock);
		return HYPER_NATIVE_STATUS_OK;
	}
	status = hyper_page_size(&page_size);
	if (status != HYPER_NATIVE_STATUS_OK) {
		hyper_mutex_unlock(&heap_lock);
		return status;
	}
	hyper_call_result_t configuration =
		hyper_system_config(HYPER_NATIVE_SYSTEM_CONFIG_APPLICATION_ADDRESS_LIMIT);
	if (configuration.status != HYPER_NATIVE_STATUS_OK || configuration.value1 ||
	    configuration.value0 / 8 < page_size || configuration.value0 > SIZE_MAX) {
		hyper_mutex_unlock(&heap_lock);
		return configuration.status ? configuration.status
					    : HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	}
	heap_size = (configuration.value0 / 8) & ~(page_size - 1);
	if (!heap_size || HYPER_HEAP_BASE % page_size) {
		hyper_mutex_unlock(&heap_lock);
		return HYPER_NATIVE_STATUS_INVALID_ARGUMENT;
	}
	hyper_call_result_t root_owner = hyper_handle_duplicate(root, HYPER_NATIVE_RIGHT_MAP);
	if (root_owner.status != HYPER_NATIVE_STATUS_OK) {
		hyper_mutex_unlock(&heap_lock);
		return root_owner.status;
	}
	hyper_call_result_t result =
		hyper_vmar_allocate(root_owner.value0, HYPER_HEAP_BASE, heap_size, 0);
	if (result.status == HYPER_NATIVE_STATUS_OK) {
		root_vmar = root_owner.value0;
		heap_vmar = result.value0;
		heap_base = result.value1;
	} else {
		require_success(hyper_handle_close(root_owner.value0));
	}
	hyper_mutex_unlock(&heap_lock);
	return result.status;
}

static region_t *grow(size_t minimum)
{
	size_t size;
	if (!round_up(minimum, page_size, &size)) {
		return NULL;
	}
	if (size < REGION_GRANULE && !round_up(REGION_GRANULE, page_size, &size))
		return NULL;
	uintptr_t address = heap_base;
	const uintptr_t end = heap_base + heap_size;
	for (region_t *region = regions; region; region = region->next) {
		if (region->vmar != heap_vmar)
			continue;
		if ((uintptr_t)region - address >= size)
			break;
		address = (uintptr_t)region + region->size;
	}
	hyper_native_handle_t vmar = heap_vmar;
	if (size > end - address) {
		/* Overflow allocations have their own reservation, with metadata in
		 * the mapping itself: extending the allocator never calls malloc. */
		hyper_call_result_t extra = hyper_vmar_allocate(root_vmar, end, size, 0);
		if (extra.status != HYPER_NATIVE_STATUS_OK)
			return NULL;
		vmar = extra.value0;
		address = extra.value1;
	}
	hyper_call_result_t backing = hyper_vmo_create(size);
	if (backing.status != HYPER_NATIVE_STATUS_OK) {
		if (vmar != heap_vmar)
			require_success(hyper_vmar_destroy(vmar));
		return NULL;
	}
	hyper_native_status_t status = hyper_vmar_map(vmar, backing.value0, 0, address, size,
						      HYPER_NATIVE_VMAR_PERMISSION_READ |
							      HYPER_NATIVE_VMAR_PERMISSION_WRITE);
	/* The successful mapping retains backing ownership. Failed maps leave no
	 * published region and must release any pages populated by the kernel. */
	require_success(hyper_handle_close(backing.value0));
	if (status != HYPER_NATIVE_STATUS_OK) {
		if (vmar != heap_vmar)
			require_success(hyper_vmar_destroy(vmar));
		return NULL;
	}
	region_t **link = &regions;
	while (*link && (uintptr_t)*link < address)
		link = &(*link)->next;
	region_t *region = (region_t *)address;
	*region = (region_t){.size = size, .vmar = vmar, .next = *link};
	*link = region;
	block_t *block = first_block(region);
	*block = (block_t){
		.span = size - region_header_size(),
		.region = region,
		.available = true,
	};
	return region;
}

static void split(block_t *block, size_t used)
{
	size_t remainder = block->span - used;
	if (remainder < sizeof(block_t) + sizeof(block_t *) + BLOCK_ALIGNMENT) {
		return;
	}
	block_t *tail = (block_t *)((unsigned char *)block + used);
	*tail = (block_t){
		.span = remainder,
		.previous = block,
		.next = block->next,
		.region = block->region,
		.available = true,
	};
	if (tail->next != NULL) {
		tail->next->previous = tail;
	}
	block->next = tail;
	block->span = used;
}

static void *take(block_t *block, size_t size, size_t alignment)
{
	size_t address;
	size_t used;
	if (!block->available ||
	    !round_up((uintptr_t)block + sizeof(*block) + sizeof(block_t *), alignment, &address) ||
	    size > SIZE_MAX - (address - (uintptr_t)block) ||
	    !round_up(address - (uintptr_t)block + size, BLOCK_ALIGNMENT, &used) ||
	    used > block->span) {
		return NULL;
	}
	split(block, used);
	block->available = false;
	block->requested = size;
	/* Alignment is at least pointer alignment, so the backlink is aligned. */
	((block_t **)address)[-1] = block;
	return (void *)address;
}

void *hyper_alloc(size_t size, size_t alignment)
{
	if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
		return NULL;
	}
	if (alignment < BLOCK_ALIGNMENT) {
		alignment = BLOCK_ALIGNMENT;
	}
	if (size == 0) {
		size = 1;
	}
	/* Bound every later metadata/padding addition before touching the heap. */
	size_t overhead =
		region_header_size() + sizeof(block_t) + sizeof(block_t *) + BLOCK_ALIGNMENT;
	if (alignment > SIZE_MAX - overhead || size > SIZE_MAX - alignment - overhead) {
		return NULL;
	}
	hyper_mutex_lock(&heap_lock);
	if (heap_vmar == 0) {
		hyper_mutex_unlock(&heap_lock);
		return NULL;
	}
	for (region_t *region = regions; region != NULL; region = region->next) {
		for (block_t *block = first_block(region); block != NULL; block = block->next) {
			void *pointer = take(block, size, alignment);
			if (pointer != NULL) {
				hyper_mutex_unlock(&heap_lock);
				return pointer;
			}
		}
	}
	region_t *region = grow(size + alignment + overhead);
	void *pointer = region == NULL ? NULL : take(first_block(region), size, alignment);
	hyper_mutex_unlock(&heap_lock);
	return pointer;
}

static void merge_next(block_t *block)
{
	block_t *next = block->next;
	block->span += next->span;
	block->next = next->next;
	if (block->next != NULL) {
		block->next->previous = block;
	}
}

void hyper_free(void *pointer)
{
	if (pointer == NULL) {
		return;
	}
	hyper_mutex_lock(&heap_lock);
	block_t *block = ((block_t **)pointer)[-1];
	block->available = true;
	if (block->next != NULL && block->next->available) {
		merge_next(block);
	}
	if (block->previous != NULL && block->previous->available) {
		block = block->previous;
		merge_next(block);
	}
	if (block->previous == NULL && block->next == NULL) {
		region_t *region = block->region;
		region_t **link = &regions;
		while (*link != region) {
			link = &(*link)->next;
		}
		const size_t size = region->size;
		const hyper_native_handle_t vmar = region->vmar;
		*link = region->next;
		require_success(hyper_vmar_unmap(vmar, (uintptr_t)region, size));
		if (vmar != heap_vmar)
			require_success(hyper_vmar_destroy(vmar));
	}
	hyper_mutex_unlock(&heap_lock);
}

void *hyper_realloc(void *pointer, size_t size, size_t alignment)
{
	if (pointer == NULL) {
		return hyper_alloc(size, alignment);
	}
	if (size == 0) {
		hyper_free(pointer);
		return NULL;
	}
	/* The caller exclusively owns this allocation, even while unrelated blocks
	 * are changed by other threads. requested is immutable until free. */
	block_t *block = ((block_t **)pointer)[-1];
	size_t previous = block->requested;
	void *replacement = hyper_alloc(size, alignment);
	if (replacement != NULL) {
		memcpy(replacement, pointer, size < previous ? size : previous);
		hyper_free(pointer);
	}
	return replacement;
}
