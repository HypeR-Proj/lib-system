/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include "syscall-capture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint64_t expected_target;
static uint64_t expected_operation;
static uint64_t expected_arguments[6];
static hyper_native_status_t expected_status;
static int pending;
static size_t expected_record_argument = 6;
static size_t expected_record_size;
static unsigned char expected_record[64];

void expect_call(uint64_t target, uint64_t operation, const uint64_t arguments[6],
		 hyper_native_status_t status)
{
	assert(!pending);
	expected_record_argument = 6;
	expected_target = target;
	expected_operation = operation;
	for (size_t index = 0; index < 6; ++index) {
		expected_arguments[index] = arguments[index];
	}
	expected_status = status;
	pending = 1;
}

void expect_record_call(uint64_t target, uint64_t operation, const uint64_t arguments[6],
			size_t record_argument, const void *record, size_t record_size,
			hyper_native_status_t status)
{
	assert(record_argument < 5 && record_size <= sizeof(expected_record));
	expect_call(target, operation, arguments, status);
	expected_record_argument = record_argument;
	expected_record_size = record_size;
	memcpy(expected_record, record, record_size);
}

void check_consumed(void)
{
	assert(!pending);
}

hyper_call_result_t hyper_native_invoke6(uint64_t target, uint64_t operation, uint64_t a0,
					 uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
					 uint64_t a5)
{
	const uint64_t actual[] = {a0, a1, a2, a3, a4, a5};
	assert(pending);
	assert(target == expected_target);
	assert(operation == expected_operation);
	for (size_t index = 0; index < 6; ++index) {
		if (index == expected_record_argument) {
			assert(actual[index] != 0);
			assert(actual[index + 1] == expected_record_size);
			assert(memcmp((const void *)(uintptr_t)actual[index], expected_record,
				      expected_record_size) == 0);
			continue;
		}
		if (actual[index] != expected_arguments[index]) {
			fprintf(stderr, "operation %llu slot %zu: actual=%llx expected=%llx\n",
				(unsigned long long)operation, index,
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

hyper_native_handle_t hyper_native_execution_context(void)
{
	return UINT64_C(0xe000000000000001);
}

hyper_native_handle_t hyper_native_object_factory(void)
{
	return UINT64_C(0xe000000000000002);
}

hyper_native_handle_t hyper_native_system_info(void)
{
	return UINT64_C(0xe000000000000003);
}
