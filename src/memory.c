/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>

void *memcpy(void *restrict destination, const void *restrict source, size_t count)
{
	unsigned char *output = destination;
	const unsigned char *input = source;

	for (size_t index = 0; index < count; ++index) {
		output[index] = input[index];
	}
	return destination;
}

void *memmove(void *destination, const void *source, size_t count)
{
	unsigned char *output = destination;
	const unsigned char *input = source;

	uintptr_t distance = (uintptr_t)output - (uintptr_t)input;
	if (distance >= count) {
		for (size_t index = 0; index < count; ++index) {
			output[index] = input[index];
		}
	} else {
		for (size_t index = count; index != 0; --index) {
			output[index - 1] = input[index - 1];
		}
	}
	return destination;
}

void *memset(void *destination, int value, size_t count)
{
	unsigned char *output = destination;

	for (size_t index = 0; index < count; ++index) {
		output[index] = (unsigned char)value;
	}
	return destination;
}

int memcmp(const void *left, const void *right, size_t count)
{
	const unsigned char *left_bytes = left;
	const unsigned char *right_bytes = right;

	for (size_t index = 0; index < count; ++index) {
		if (left_bytes[index] != right_bytes[index]) {
			return (int)left_bytes[index] - (int)right_bytes[index];
		}
	}
	return 0;
}
