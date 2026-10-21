/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_HEAP_INTERNAL_H
#define HYPER_HEAP_INTERNAL_H

#define HYPER_HEAP_ARENAS 8
/* Does not attach TLS or allocate. Unattached bootstrap callers use arena 0. */
unsigned hyper_heap_arena_index(void);

#endif
