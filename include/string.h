/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HYPER_STRING_H
#define HYPER_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void *restrict destination, const void *restrict source, size_t count);
void *memmove(void *destination, const void *source, size_t count);
void *memset(void *destination, int value, size_t count);
int memcmp(const void *left, const void *right, size_t count);
void *memchr(const void *bytes, int value, size_t count);
char *strchr(const char *string, int value);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
size_t strlen(const char *string);

#ifdef __cplusplus
}
#endif

#endif /* HYPER_STRING_H */
