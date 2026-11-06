/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <stdio.h>
#include "../../src/vdso.c"
#include "../../src/vdso-map.c"

static _Alignas(4096) unsigned char image[4096];
static unsigned maps, closes, destroys, fail_map;

hyper_native_status_t hyper_vmo_read(hyper_native_handle_t vmo, uint64_t offset, void *bytes,
				     size_t length)
{
	assert(vmo == 10 && within(offset, length, sizeof(image)));
	memcpy(bytes, image + offset, length);
	return 0;
}

hyper_call_result_t hyper_vmar_allocate(hyper_native_handle_t root, uintptr_t hint, size_t length,
					uint64_t options)
{
	assert(root == 9 && hint && length == 65536 + 4096 && !options);
	return (hyper_call_result_t){.value0 = 12, .value1 = 0x4000000};
}

hyper_native_status_t hyper_vmar_map(hyper_native_handle_t region, hyper_native_handle_t vmo,
				     uint64_t offset, uintptr_t address, size_t size,
				     uint32_t permissions)
{
	assert(region == 12 && !offset && size == 4096);
	++maps;
	if (maps == 1) {
		assert(vmo == 10 && address == 0x4000000);
		assert(permissions ==
		       (HYPER_NATIVE_VMAR_PERMISSION_READ | HYPER_NATIVE_VMAR_PERMISSION_EXECUTE));
	} else {
		assert(vmo == 11 && address == 0x4010000);
		assert(permissions == HYPER_NATIVE_VMAR_PERMISSION_READ);
	}
	return maps == fail_map ? HYPER_NATIVE_STATUS_NO_MEMORY : 0;
}

hyper_native_status_t hyper_vmar_destroy(hyper_native_handle_t region)
{
	assert(region == 12);
	++destroys;
	return 0;
}

hyper_native_status_t hyper_handle_close(hyper_native_handle_t handle)
{
	assert(handle >= 10 && handle <= 12);
	++closes;
	return 0;
}

static hyper_call_result_t inactive(void)
{
	return (hyper_call_result_t){0};
}

static hyper_call_result_t active(void)
{
	return (hyper_call_result_t){.value0 = 1};
}

static hyper_call_result_t unsupported(void)
{
	return (hyper_call_result_t){.status = HYPER_NATIVE_STATUS_NOT_SUPPORTED};
}

int main(void)
{
	hyper_startup_t startup = {0};
	hyper_vdso_initialize(&startup);
	assert(hyper_trace_ipc_active());
	assert(hyper_vdso_monotonic().status == HYPER_NATIVE_STATUS_NOT_SUPPORTED);
	assert(hyper_vdso_realtime().status == HYPER_NATIVE_STATUS_NOT_SUPPORTED);
	atomic_store(&ipc_entry, inactive);
	assert(!hyper_trace_ipc_active());
	atomic_store(&ipc_entry, active);
	assert(hyper_trace_ipc_active());
	atomic_store(&ipc_entry, unsupported);
	assert(hyper_trace_ipc_active());

	Elf64_Ehdr *header = (void *)image;
	*header = (Elf64_Ehdr){.type = 3,
			       .ehsize = sizeof(*header),
			       .phoff = 64,
			       .phentsize = sizeof(Elf64_Phdr),
			       .phnum = 3};
	memcpy(header->ident, "\177ELF\002\001\001", 7);
	Elf64_Phdr *phdr = (void *)(image + 64);
	phdr[0] = (Elf64_Phdr){.type = 1, .flags = 5, .filesz = 4096, .memsz = 4096};
	phdr[1] = (Elf64_Phdr){.type = 1, .flags = 4, .vaddr = 65536, .memsz = 4096};
	phdr[2] = (Elf64_Phdr){.type = 2, .vaddr = 256, .filesz = 6 * sizeof(Elf64_Dyn)};
	Elf64_Dyn *dynamic = (void *)(image + 256);
	dynamic[0] = (Elf64_Dyn){4, 512};
	dynamic[1] = (Elf64_Dyn){5, 768};
	dynamic[2] = (Elf64_Dyn){6, 1024};
	dynamic[3] = (Elf64_Dyn){10, 64};
	dynamic[4] = (Elf64_Dyn){11, sizeof(Elf64_Sym)};
	uint32_t *hash = (void *)(image + 512);
	hash[1] = 2;
	memcpy(image + 768, "__hyper_vdso_ipc_active_v1", sizeof("__hyper_vdso_ipc_active_v1"));
	Elf64_Sym *symbol = (void *)(image + 1024);
	symbol[1] = (Elf64_Sym){.info = 0x12, .section = 1, .value = 2048, .size = 16};
	assert(resolve((uintptr_t)image, "__hyper_vdso_ipc_active_v1") == (Entry)(image + 2048));
	assert(!resolve((uintptr_t)image, "__hyper_vdso_monotonic_v1"));
	hash[1] = UINT32_MAX;
	assert(!resolve((uintptr_t)image, "__hyper_vdso_ipc_active_v1"));
	hash[1] = 2;
	symbol[1].value = UINT64_MAX;
	assert(!resolve((uintptr_t)image, "__hyper_vdso_ipc_active_v1"));

	assert(hyper_vdso_map(9, 10, 11) == 0x4000000);
	assert(maps == 2 && closes == 3 && !destroys);
	maps = closes = destroys = 0;
	fail_map = 2;
	assert(!hyper_vdso_map(9, 10, 11));
	assert(maps == 2 && closes == 3 && destroys == 1);
	maps = closes = destroys = 0;
	phdr[1].flags = 6;
	assert(!hyper_vdso_map(9, 10, 11));
	assert(!maps && closes == 2 && !destroys);
	puts("optional vDSO discovery, fallback, permissions and rollback passed");
	return 0;
}
