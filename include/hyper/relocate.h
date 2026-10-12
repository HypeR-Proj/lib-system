/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HYPER_RELOCATE_H
#define HYPER_RELOCATE_H

#include <hyper/elf.h>
#include <stddef.h>
#include <stdint.h>

/* Early-entry code must not access a GOT, initialized pointer, TLS or external
 * function before completing self relocation. Keep these helpers local and
 * freestanding. The mapped image and stack come from userspace-loader. */
static inline int hyper_elf_range(const Elf64_Phdr *phdr, size_t count, uintptr_t address,
				  size_t bytes, int writable)
{
	if (address > UINTPTR_MAX - bytes)
		return 0;
	for (size_t i = 0; i < count; ++i) {
		const Elf64_Phdr *p = &phdr[i];
		if (p->type == 1 && p->vaddr <= address && p->memsz <= UINTPTR_MAX - p->vaddr &&
		    address + bytes <= p->vaddr + p->memsz && (!writable || (p->flags & 3) == 2))
			return 1;
	}
	return 0;
}

static inline int hyper_elf_self_relocate(const Elf64_Ehdr *header)
{
	if (header->phnum > 32 || header->phentsize != sizeof(Elf64_Phdr) || header->phoff > 4096 ||
	    header->phnum * sizeof(Elf64_Phdr) > 4096 - header->phoff)
		return 0;
	const Elf64_Phdr *phdr = (const void *)((uintptr_t)header + header->phoff);
	uintptr_t base = 0;
	if (header->type == 3) {
		int found = 0;
		for (size_t i = 0; i < header->phnum; ++i) {
			if (phdr[i].type == 1 && phdr[i].offset == 0 &&
			    phdr[i].filesz >= sizeof(*header) &&
			    phdr[i].vaddr <= (uintptr_t)header) {
				base = (uintptr_t)header - phdr[i].vaddr;
				found = 1;
				break;
			}
		}
		if (!found)
			return 0;
	}
	const Elf64_Dyn *dynamic = NULL;
	size_t dynamic_count = 0;
	for (size_t i = 0; i < header->phnum; ++i) {
		if (phdr[i].type == 2) {
			if (dynamic || phdr[i].vaddr % 8 || phdr[i].filesz % sizeof(Elf64_Dyn) ||
			    !hyper_elf_range(phdr, header->phnum, phdr[i].vaddr, phdr[i].filesz, 0))
				return 0;
			dynamic = (const void *)(base + phdr[i].vaddr);
			dynamic_count = phdr[i].filesz / sizeof(Elf64_Dyn);
		}
	}
	/* Fixed static images can have no dynamic table. */
	if (!dynamic)
		return header->type == 2;
	uintptr_t rela = 0, relr = 0;
	size_t relasz = 0, relrsz = 0, relaent = 24, relrent = 8;
	int terminated = 0;
	unsigned seen = 0;
	for (size_t i = 0; i < dynamic_count; ++i) {
		uint64_t value = dynamic[i].value;
		unsigned bit = 0;
		if (dynamic[i].tag >= 7 && dynamic[i].tag <= 9)
			bit = 1u << (dynamic[i].tag - 7);
		if (dynamic[i].tag >= 35 && dynamic[i].tag <= 37)
			bit = 8u << (dynamic[i].tag - 35);
		if (seen & bit)
			return 0;
		seen |= bit;
		switch (dynamic[i].tag) {
		case 0:
			terminated = 1;
			break;
		case 7:
			rela = value;
			break;
		case 8:
			relasz = value;
			break;
		case 9:
			relaent = value;
			break;
		case 35:
			relrsz = value;
			break;
		case 36:
			relr = value;
			break;
		case 37:
			relrent = value;
			break;
		case 1:
		case 17:
		case 18:
		case 19:
		case 22:
			return 0;
		case 2:
			if (value)
				return 0;
			break;
		case 30:
			if (value & 4)
				return 0;
			break;
		default:
			break;
		}
		if (terminated)
			break;
	}
	if (!terminated || relaent != sizeof(Elf64_Rela) || relrent != 8 ||
	    relasz % sizeof(Elf64_Rela) || relrsz % 8 || rela % 8 || relr % 8 ||
	    (relasz && !hyper_elf_range(phdr, header->phnum, rela, relasz, 0)) ||
	    (relrsz && !hyper_elf_range(phdr, header->phnum, relr, relrsz, 0)))
		return 0;
	for (size_t i = 0; i < relasz / sizeof(Elf64_Rela); ++i) {
		const Elf64_Rela *r = (const void *)(base + rela + i * sizeof(Elf64_Rela));
#if defined(__aarch64__)
		const uint64_t relative = 1027;
#elif defined(__riscv) && __riscv_xlen == 64
		const uint64_t relative = 3;
#else
#error Unsupported Native self-relocation architecture
#endif
		if (r->info != relative || r->offset % 8 ||
		    !hyper_elf_range(phdr, header->phnum, r->offset, 8, 1))
			return 0;
		*(uintptr_t *)(base + r->offset) = base + (uint64_t)r->addend;
	}
	uintptr_t cursor = 0;
	for (size_t i = 0; i < relrsz / 8; ++i) {
		uint64_t word = *(const uint64_t *)(base + relr + i * 8);
		if (!(word & 1)) {
			if (word < cursor || word % 8 ||
			    !hyper_elf_range(phdr, header->phnum, word, 8, 1))
				return 0;
			*(uintptr_t *)(base + word) += base;
			cursor = word + 8;
		} else {
			if (!cursor || cursor > UINTPTR_MAX - 63 * 8)
				return 0;
			for (unsigned bit = 1; bit < 64; ++bit) {
				if (!(word & (UINT64_C(1) << bit)))
					continue;
				uintptr_t target = cursor + (bit - 1) * 8;
				if (!hyper_elf_range(phdr, header->phnum, target, 8, 1))
					return 0;
				*(uintptr_t *)(base + target) += base;
			}
			cursor += 63 * 8;
		}
	}
	return 1;
}

static inline uintptr_t hyper_elf_auxiliary(const uintptr_t *stack, uintptr_t key)
{
	const uintptr_t *cursor = stack + 1 + stack[0] + 1;
	while (*cursor)
		++cursor;
	++cursor;
	for (; cursor[0]; cursor += 2)
		if (cursor[0] == key)
			return cursor[1];
	return 0;
}
#endif
