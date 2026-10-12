/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/heap.h>
#include <hyper/syscall.h>
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

hyper_call_result_t hyper_system_config(uint64_t key)
{
	if (key == HYPER_NATIVE_SYSTEM_CONFIG_APPLICATION_ADDRESS_LIMIT)
		return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_OK,
					     .value0 = 128 * 1024 * 1024};
	assert(key == HYPER_NATIVE_SYSTEM_CONFIG_PAGE_SIZE);
	return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_OK, .value0 = 16384};
}

#define HYPER_HEAP_SIZE (16 * 1024 * 1024)
uintptr_t hyper_heap_test_base;
static uintptr_t extra_base;
static size_t extra_size;

hyper_call_result_t hyper_handle_duplicate(uint64_t source, uint64_t rights)
{
	assert(source == 1 && rights == HYPER_NATIVE_RIGHT_MAP);
	return (hyper_call_result_t){.status = 0, .value0 = 4};
}

hyper_native_status_t hyper_vmar_destroy(uint64_t handle)
{
	assert(handle == 5 && extra_size);
	assert(munmap((void *)extra_base, extra_size) == 0);
	extra_base = extra_size = 0;
	return 0;
}

static size_t mapped_bytes;
static size_t maps;
static size_t closes;
static size_t backing_size;
static int fail_create;
static int fail_map;
static int fail_reserve;
static int backing_live;

static struct {
	uintptr_t address;
	size_t size;
} live_mappings[128];

hyper_call_result_t hyper_vmar_allocate(hyper_native_handle_t parent, uintptr_t address,
					size_t size, uint64_t options)
{
	assert(parent == 4 && options == 0);
	if (fail_reserve)
		return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_NO_MEMORY};
	if (address == HYPER_HEAP_BASE) {
		assert(size == HYPER_HEAP_SIZE);
		return (hyper_call_result_t){.status = 0, .value0 = 2, .value1 = address + 16384};
	}
	assert(!extra_size);
	void *area = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (area == MAP_FAILED)
		return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_NO_MEMORY};
	extra_base = (uintptr_t)area;
	extra_size = size;
	return (hyper_call_result_t){.status = 0, .value0 = 5, .value1 = extra_base};
}

hyper_call_result_t hyper_vmo_create(uint64_t size)
{
	assert(size && size % 16384 == 0);
	assert(!backing_live);
	if (fail_create) {
		return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_NO_MEMORY};
	}
	backing_live = 1;
	backing_size = size;
	return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_OK, .value0 = 3};
}

hyper_native_status_t hyper_vmar_map(hyper_native_handle_t vmar, hyper_native_handle_t vmo,
				     uint64_t offset, uintptr_t address, size_t size,
				     uint32_t permissions)
{
	assert((vmar == 2 || vmar == 5) && vmo == 3 && offset == 0 && size == backing_size);
	assert(backing_live && permissions == (HYPER_NATIVE_VMAR_PERMISSION_READ |
					       HYPER_NATIVE_VMAR_PERMISSION_WRITE));
	assert((vmar == 2 && address >= HYPER_HEAP_BASE + 16384 &&
		address + size <= HYPER_HEAP_BASE + 16384 + HYPER_HEAP_SIZE) ||
	       (vmar == 5 && address == extra_base && size == extra_size));
	if (fail_map) {
		return HYPER_NATIVE_STATUS_NO_MEMORY;
	}
	size_t slot = 128;
	for (size_t i = 0; i < 128; ++i) {
		if (live_mappings[i].size == 0) {
			slot = i;
		} else {
			assert(address + size <= live_mappings[i].address ||
			       live_mappings[i].address + live_mappings[i].size <= address);
		}
	}
	assert(slot < 128);
	live_mappings[slot].address = address;
	live_mappings[slot].size = size;
	memset((void *)address, 0, size);
	mapped_bytes += size;
	++maps;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_handle_close(hyper_native_handle_t handle)
{
	if (handle == 4)
		return 0;
	assert(handle == 3 && backing_live);
	backing_live = 0;
	++closes;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_vmar_unmap(hyper_native_handle_t vmar, uintptr_t address, size_t size)
{
	assert((vmar == 2 || vmar == 5) && size <= mapped_bytes);
	size_t slot = 128;
	for (size_t i = 0; i < 128; ++i) {
		if (live_mappings[i].address == address && live_mappings[i].size == size) {
			slot = i;
			break;
		}
	}
	assert(slot < 128);
	live_mappings[slot].size = 0;
	memset((void *)address, 0xdd, size);
	mapped_bytes -= size;
	return HYPER_NATIVE_STATUS_OK;
}

hyper_native_status_t hyper_thread_yield(void)
{
	sched_yield();
	return HYPER_NATIVE_STATUS_OK;
}

_Noreturn void hyper_process_exit(int64_t status)
{
	(void)status;
	assert(!"unexpected heap invariant failure");
	__builtin_trap();
}

static void *worker(void *argument)
{
	uintptr_t seed = (uintptr_t)argument;
	for (size_t iteration = 0; iteration < 500; ++iteration) {
		size_t size = 1 + (iteration * 137 + seed) % 4096;
		unsigned char *p = hyper_alloc(size, 64);
		assert(p != NULL && (uintptr_t)p % 64 == 0);
		memset(p, (int)seed, size);
		unsigned char *q = hyper_realloc(p, size * 2, 64);
		assert(q != NULL && (uintptr_t)q % 64 == 0);
		for (size_t i = 0; i < size; ++i) {
			assert(q[i] == seed);
		}
		hyper_free(q);
	}
	return NULL;
}

int main(void)
{
	void *reservation = mmap(NULL, HYPER_HEAP_SIZE + 16384, PROT_READ | PROT_WRITE,
				 MAP_PRIVATE | MAP_ANON, -1, 0);
	assert(reservation != MAP_FAILED);
	/* The emulated Native page may exceed the host mmap granule. */
	hyper_heap_test_base = ((uintptr_t)reservation + 16383) & ~(uintptr_t)16383;
	assert(hyper_alloc(16, 16) == NULL);
	hyper_native_startup_handle_t root = {
		.purpose = HYPER_NATIVE_STARTUP_HANDLE_PURPOSE_ROOT_VMAR,
		.handle = 1,
	};
	hyper_startup_t startup = {.handles = &root, .handle_count = 1};
	fail_reserve = 1;
	assert(hyper_heap_initialize(&startup) == HYPER_NATIVE_STATUS_NO_MEMORY);
	fail_reserve = 0;
	assert(hyper_heap_initialize(&startup) == HYPER_NATIVE_STATUS_OK);
	assert(hyper_heap_initialize(&startup) == HYPER_NATIVE_STATUS_OK);
	assert(mapped_bytes == 0);
	assert(hyper_alloc(SIZE_MAX, 16) == NULL);
	assert(hyper_alloc(16, 0) == NULL);
	assert(hyper_alloc(16, 3) == NULL);
	assert(hyper_alloc(16, (SIZE_MAX / 2) + 1) == NULL);
	assert(calloc(SIZE_MAX, 2) == NULL);
	assert(aligned_alloc(64, 65) == NULL);
	free(NULL);
	free(malloc(0));
	unsigned char *dirty = malloc(1024);
	void *guard = malloc(32);
	assert(dirty && guard);
	memset(dirty, 0xa5, 1024);
	free(dirty);
	unsigned char *zero = calloc(1024, 1);
	assert(zero == dirty);
	assert(zero != NULL);
	for (size_t i = 0; i < 1024; ++i) {
		assert(zero[i] == 0);
	}
	free(zero);
	free(guard);
	for (size_t alignment = 1; alignment <= 131072; alignment *= 2) {
		void *p = hyper_alloc(123, alignment);
		assert(p != NULL && (uintptr_t)p % alignment == 0);
		memset(p, 0xab, 123);
		hyper_free(p);
		assert(mapped_bytes == 0);
	}
	/* Keep a neighbor alive so free/reuse and coalescing happen inside a region. */
	void *a = malloc(1024), *b = malloc(1024), *c = malloc(1024);
	assert(a && b && c);
	size_t before = maps;
	free(b);
	void *reuse = malloc(1024);
	assert(reuse == b && maps == before);
	free(reuse);
	free(a);
	void *merged = malloc(1800);
	assert(merged == a && maps == before);
	free(merged);
	free(c);
	assert(mapped_bytes == 0);
	unsigned char *p = malloc(32);
	assert(p != NULL);
	memset(p, 0x5a, 32);
	fail_create = 1;
	assert(realloc(p, 100000) == NULL);
	fail_create = 0;
	fail_map = 1;
	size_t old_closes = closes;
	assert(realloc(p, 100000) == NULL);
	assert(closes == old_closes + 1 && !backing_live);
	fail_map = 0;
	for (size_t i = 0; i < 32; ++i) {
		assert(p[i] == 0x5a);
	}
	unsigned char *grown = realloc(p, 100000);
	assert(grown != NULL);
	for (size_t i = 0; i < 32; ++i) {
		assert(grown[i] == 0x5a);
	}
	unsigned char *shrunk = realloc(grown, 16);
	assert(shrunk != NULL);
	for (size_t i = 0; i < 16; ++i) {
		assert(shrunk[i] == 0x5a);
	}
	assert(realloc(shrunk, 0) == NULL);
	assert(mapped_bytes == 0);
	void *large = hyper_alloc(HYPER_HEAP_SIZE + 1, 16);
	assert(large && extra_size);
	hyper_free(large);
	assert(!extra_size && !mapped_bytes);
	fail_map = 1;
	assert(!hyper_alloc(HYPER_HEAP_SIZE + 1, 16));
	fail_map = 0;
	assert(!extra_size && !backing_live);
	pthread_t workers[4];
	for (uintptr_t i = 0; i < 4; ++i) {
		assert(pthread_create(&workers[i], NULL, worker, (void *)(i + 1)) == 0);
	}
	for (size_t i = 0; i < 4; ++i) {
		assert(pthread_join(workers[i], NULL) == 0);
	}
	assert(mapped_bytes == 0 && !backing_live);
	assert(munmap(reservation, HYPER_HEAP_SIZE + 16384) == 0);
	return 0;
}
