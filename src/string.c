/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

void *memchr(const void *bytes, int value, size_t count)
{
	const unsigned char *cursor = bytes;
	for (size_t i = 0; i < count; ++i)
		if (cursor[i] == (unsigned char)value)
			return (void *)(cursor + i);
	return NULL;
}

char *strchr(const char *string, int value)
{
	for (;; ++string) {
		if ((unsigned char)*string == (unsigned char)value)
			return (char *)string;
		if (!*string)
			return NULL;
	}
}

int strcmp(const char *left, const char *right)
{
	while (*left && *left == *right) {
		++left;
		++right;
	}
	return (unsigned char)*left - (unsigned char)*right;
}

int strncmp(const char *left, const char *right, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		if (!left[i] || left[i] != right[i])
			return (unsigned char)left[i] - (unsigned char)right[i];
	}
	return 0;
}

size_t strlen(const char *string)
{
	size_t length = 0;

	while (string[length] != '\0') {
		++length;
	}
	return length;
}
