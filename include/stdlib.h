/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPER_STDLIB_H
#define HYPER_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Native allocation subset; failures return NULL without an errno facility. */
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *pointer, size_t size);
void free(void *pointer);
void *aligned_alloc(size_t alignment, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* HYPER_STDLIB_H */
