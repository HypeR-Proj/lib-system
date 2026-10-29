/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include <hyper/syscall.h>

/* Independent test oracle: expectations are ABI slots, never read from veneers. */
void expect_call(uint64_t target, uint64_t operation, const uint64_t arguments[6],
		 hyper_native_status_t status);
void check_consumed(void);

/* Compare a copied request while its wrapper-owned storage is still alive. */
void expect_record_call(uint64_t target, uint64_t operation, const uint64_t arguments[6],
			size_t record_argument, const void *record, size_t record_size,
			hyper_native_status_t status);
