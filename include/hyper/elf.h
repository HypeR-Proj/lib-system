/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_ELF_H
#define HYPER_ELF_H

#include <stdint.h>

/* ELF64 file layouts shared by Native userspace image consumers. */
typedef struct {
	unsigned char ident[16];
	uint16_t type, machine;
	uint32_t version;
	uint64_t entry, phoff, shoff;
	uint32_t flags;
	uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
} Elf64_Ehdr;

typedef struct {
	uint32_t type, flags;
	uint64_t offset, vaddr, paddr, filesz, memsz, align;
} Elf64_Phdr;

typedef struct {
	int64_t tag;
	uint64_t value;
} Elf64_Dyn;

typedef struct {
	uint32_t name;
	unsigned char info, other;
	uint16_t section;
	uint64_t value, size;
} Elf64_Sym;

typedef struct {
	uint64_t offset, info;
	int64_t addend;
} Elf64_Rela;

#endif
