/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/vdso.h>
#include <hyper/elf.h>
#include <stdatomic.h>
#include <string.h>

typedef hyper_call_result_t (*Entry)(void);
static _Atomic(Entry) monotonic_entry, realtime_entry, ipc_entry;

static int within(uint64_t offset, uint64_t size, uint64_t extent)
{
	return offset <= extent && size <= extent - offset;
}

/* Kernel-generated images have no relocations. Resolve the standard ELF dynamic
 * symbol table, rather than publishing private data offsets as an app ABI. */
static Entry resolve(uintptr_t base, const char *name)
{
	const Elf64_Ehdr *header = (const void *)base;
	if (!base || base & 4095 || memcmp(header->ident, "\177ELF\002\001\001", 7) ||
	    header->type != 3 || !header->phnum || header->phnum > 8 ||
	    header->phentsize != sizeof(Elf64_Phdr) || (header->phoff & 7) ||
	    !within(header->phoff, header->phnum * sizeof(Elf64_Phdr), 4096))
		return NULL;
	const Elf64_Phdr *phdr = (const void *)(base + header->phoff);
	uint64_t extent = 0, dynamic = 0, dynamic_size = 0;
	for (size_t i = 0; i < header->phnum; ++i) {
		if (phdr[i].type == 1 && phdr[i].flags == 5 && !phdr[i].vaddr)
			extent = phdr[i].filesz;
		if (phdr[i].type == 2) {
			dynamic = phdr[i].vaddr;
			dynamic_size = phdr[i].filesz;
		}
	}
	if (!extent || extent > 65536 || base > UINTPTR_MAX - extent || !dynamic_size ||
	    (dynamic & 7) || dynamic_size % sizeof(Elf64_Dyn) ||
	    !within(dynamic, dynamic_size, extent))
		return NULL;
	const Elf64_Dyn *entries = (const void *)(base + dynamic);
	uint64_t hash = 0, symbols = 0, strings = 0, string_size = 0, symbol_size = 0;
	for (size_t i = 0; i < dynamic_size / sizeof(*entries); ++i) {
		if (!entries[i].tag)
			break;
		switch (entries[i].tag) {
		case 4:
			hash = entries[i].value;
			break;
		case 5:
			strings = entries[i].value;
			break;
		case 6:
			symbols = entries[i].value;
			break;
		case 10:
			string_size = entries[i].value;
			break;
		case 11:
			symbol_size = entries[i].value;
			break;
		}
	}
	if (symbol_size != sizeof(Elf64_Sym) || !hash || (hash & 3) || (symbols & 7) || !symbols ||
	    !strings || !within(hash, 8, extent) || !within(strings, string_size, extent))
		return NULL;
	const uint32_t *table = (const void *)(base + hash);
	uint32_t count = table[1];
	if (!count || count > 1024 || !within(symbols, (uint64_t)count * symbol_size, extent))
		return NULL;
	const Elf64_Sym *symbol = (const void *)(base + symbols);
	size_t wanted = strlen(name) + 1;
	for (uint32_t i = 0; i < count; ++i) {
		if (symbol[i].info != 0x12 || !symbol[i].section || !symbol[i].size ||
		    !within(symbol[i].value, symbol[i].size, extent) ||
		    !within(symbol[i].name, wanted, string_size))
			continue;
		if (!memcmp((const void *)(base + strings + symbol[i].name), name, wanted))
			return (Entry)(base + symbol[i].value);
	}
	return NULL;
}

void hyper_vdso_initialize(const hyper_startup_t *startup)
{
	uintptr_t base = 0;
	for (size_t i = 0; i < startup->auxiliary_count; ++i)
		if (startup->auxiliary[i].key == HYPER_NATIVE_VDSO_AUXV_BASE)
			base = startup->auxiliary[i].value;
	atomic_store_explicit(&monotonic_entry, resolve(base, "__hyper_vdso_monotonic_v1"),
			      memory_order_release);
	atomic_store_explicit(&realtime_entry, resolve(base, "__hyper_vdso_realtime_v1"),
			      memory_order_release);
	atomic_store_explicit(&ipc_entry, resolve(base, "__hyper_vdso_ipc_active_v1"),
			      memory_order_release);
}

static hyper_call_result_t invoke(_Atomic(Entry) *slot)
{
	Entry entry = atomic_load_explicit(slot, memory_order_acquire);
	return entry ? entry() : (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_NOT_SUPPORTED};
}

hyper_call_result_t hyper_vdso_monotonic(void)
{
	return invoke(&monotonic_entry);
}

hyper_call_result_t hyper_vdso_realtime(void)
{
	return invoke(&realtime_entry);
}

int hyper_trace_ipc_active(void)
{
	hyper_call_result_t hint = invoke(&ipc_entry);
	return hint.status || hint.value0 != 0;
}
