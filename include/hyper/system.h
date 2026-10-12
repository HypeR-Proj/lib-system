/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_SYSTEM_H
#define HYPER_SYSTEM_H
#include <hyper/native.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Queries and caches the immutable Native page size. Safe before heap/TLS
 * initialization. Failure leaves the output untouched and is not cached. */
hyper_native_status_t hyper_page_size(size_t *result);
#ifdef __cplusplus
}
#endif
#endif
