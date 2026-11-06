/* SPDX-FileCopyrightText: 2026 roolrz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hyper/vdso.h>
#include <hyper/elf.h>
#include <string.h>

/* No filesystem, runtime globals or allocation: both Native loaders use this
 * helper before handing off to either a static CRT or the dynamic linker. */
uintptr_t hyper_vdso_map(hyper_native_handle_t root, hyper_native_handle_t image,
			 hyper_native_handle_t data)
{
	uintptr_t base = 0;
	hyper_native_handle_t region = 0;
	Elf64_Ehdr header;
	Elf64_Phdr phdr[8], code = {0}, observations = {0};
	if (!image || !data || hyper_vmo_read(image, 0, &header, sizeof(header)))
		goto done;
	if (memcmp(header.ident, "\177ELF\002\001\001", 7) || header.type != 3 ||
	    header.ehsize != sizeof(header) || header.phentsize != sizeof(Elf64_Phdr) ||
	    !header.phnum || header.phnum > 8 || (header.phoff & 7) ||
	    header.phoff > 4096 - sizeof(phdr) ||
	    hyper_vmo_read(image, header.phoff, phdr, header.phnum * sizeof(*phdr)))
		goto done;
	for (size_t i = 0; i < header.phnum; ++i) {
		const Elf64_Phdr *p = &phdr[i];
		if (p->type != 1)
			continue;
		if (p->flags == 5 && !p->vaddr && !p->offset && !code.memsz &&
		    p->filesz == p->memsz && p->memsz >= sizeof(header) && p->memsz <= 65536)
			code = *p;
		else if (p->flags == 4 && !p->filesz && !observations.memsz && p->memsz == 4096 &&
			 !(p->vaddr & 4095) && p->vaddr <= 16 * 1024 * 1024)
			observations = *p;
		else
			goto done;
	}
	size_t code_size = (code.memsz + 4095) & ~(size_t)4095;
	if (!code_size || !observations.memsz || observations.vaddr < code_size ||
	    header.phoff + header.phnum * sizeof(*phdr) > code.memsz)
		goto done;
	hyper_call_result_t allocation = hyper_vmar_allocate(
		root, 64 * 1024 * 1024, observations.vaddr + observations.memsz, 0);
	if (allocation.status)
		goto done;
	region = allocation.value0;
	if (hyper_vmar_map(region, image, 0, allocation.value1, code_size,
			   HYPER_NATIVE_VMAR_PERMISSION_READ |
				   HYPER_NATIVE_VMAR_PERMISSION_EXECUTE) ||
	    hyper_vmar_map(region, data, 0, allocation.value1 + observations.vaddr, 4096,
			   HYPER_NATIVE_VMAR_PERMISSION_READ))
		goto done;
	base = allocation.value1;
done:
	if (region) {
		if (!base)
			(void)hyper_vmar_destroy(region);
		(void)hyper_handle_close(region);
	}
	if (image)
		(void)hyper_handle_close(image);
	if (data)
		(void)hyper_handle_close(data);
	return base;
}
