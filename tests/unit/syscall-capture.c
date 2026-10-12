/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include "syscall-capture.h"
#include <assert.h>
#include <stdio.h>

static uint64_t expected_number;
static uint64_t expected_arguments[6];
static hyper_native_status_t expected_status;
static int pending;

void expect_call(uint64_t number, const uint64_t arguments[6], hyper_native_status_t status)
{
	assert(!pending);
	expected_number = number;
	for (size_t index = 0; index < 6; ++index) {
		expected_arguments[index] = arguments[index];
	}
	expected_status = status;
	pending = 1;
}

void check_consumed(void)
{
	assert(!pending);
}

hyper_call_result_t hyper_native_call6(uint64_t number, uint64_t a0, uint64_t a1, uint64_t a2,
				       uint64_t a3, uint64_t a4, uint64_t a5)
{
	const uint64_t actual[] = {a0, a1, a2, a3, a4, a5};
	assert(pending);
	assert(number == expected_number);
	for (size_t index = 0; index < 6; ++index) {
		if (actual[index] != expected_arguments[index]) {
			fprintf(stderr, "syscall %llu slot %zu: actual=%llx expected=%llx\n",
				(unsigned long long)number, index,
				(unsigned long long)actual[index],
				(unsigned long long)expected_arguments[index]);
		}
		assert(actual[index] == expected_arguments[index]);
	}
	pending = 0;
	/* Failure statuses may also carry auxiliary results; do not erase them. */
	return (hyper_call_result_t){expected_status, UINT64_C(0xfedcba9876543210),
				     UINT64_C(0x89abcdef01234567)};
}
