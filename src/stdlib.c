/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/heap.h>
#include <stdlib.h>
#include <string.h>

void *malloc(size_t size)
{
	return hyper_alloc(size, _Alignof(max_align_t));
}

void *calloc(size_t count, size_t size)
{
	if (size != 0 && count > SIZE_MAX / size) {
		return NULL;
	}
	size_t bytes = count * size;
	void *pointer = malloc(bytes);
	if (pointer != NULL) {
		memset(pointer, 0, bytes);
	}
	return pointer;
}

void *realloc(void *pointer, size_t size)
{
	return hyper_realloc(pointer, size, _Alignof(max_align_t));
}

void free(void *pointer)
{
	hyper_free(pointer);
}

void *aligned_alloc(size_t alignment, size_t size)
{
	if (alignment == 0 || (alignment & (alignment - 1)) != 0 || size % alignment != 0) {
		return NULL;
	}
	return hyper_alloc(size, alignment);
}
