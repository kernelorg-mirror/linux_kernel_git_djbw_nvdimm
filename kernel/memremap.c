/*
 * Copyright(c) 2015 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include <linux/types.h>
#include <linux/io.h>
#include <linux/mm.h>

__weak void *arch_memremap(resource_size_t offset, size_t size,
		unsigned long flags)
{
	if (!IS_ENABLED(CONFIG_MMU))
		return (void *) (unsigned long) offset;
	WARN_ONCE(1, "%s in %s should only be called in NOMMU configurations\n",
			__func__, __FILE__);
	return NULL;
}

__weak void arch_memunmap(void *addr)
{
	WARN_ONCE(IS_ENABLED(CONFIG_MMU),
		"%s in %s should only be called in NOMMU configurations\n",
		__func__, __FILE__);
}

/**
 * memremap() - remap an iomem_resource as cacheable memory
 * @offset: iomem resource start address
 * @size: size of remap
 * @flags: either MEMREMAP_WB or MEMREMAP_WT
 *
 * memremap() is "ioremap" for cases where it is known that the resource
 * being mapped does not have i/o side effects and the __iomem
 * annotation is not applicable.
 *
 * MEMREMAP_WB - matches the default mapping for "System RAM" on
 * the architecture.  This is usually a read-allocate write-back cache.
 * Morever, if MEMREMAP_WB is specified and the requested remap region is RAM
 * memremap() will bypass establishing a new mapping and instead return
 * a pointer into the direct map.
 *
 * MEMREMAP_WT - establish a mapping whereby writes either bypass the
 * cache or are written through to memory and never exist in a
 * cache-dirty state with respect to program visibility.  Attempts to
 * map "System RAM" with this mapping type will fail.
 *
 * Note, that overlapping mappings can be established provided they are
 * all of the same mapping type.
 */
void *memremap(resource_size_t offset, size_t size, unsigned long flags)
{
	int is_ram = region_intersects(offset, size, "System RAM");
	void *addr = NULL;

	if (is_ram == REGION_MIXED) {
		WARN_ONCE(1, "memremap attempted on mixed range %pa size: %zu\n",
				&offset, size);
		return NULL;
	}

	/* Try all mapping types requested until one returns non-NULL */
	if (flags & MEMREMAP_WB) {
		flags &= ~MEMREMAP_WB;
		/*
		 * MEMREMAP_WB is special in that it can be satisifed
		 * from the direct map.  Some archs depend on the
		 * capability of memremap() to autodetect cases where
		 * the requested range is potentially in "System RAM"
		 */
		if (is_ram == REGION_INTERSECTS)
			addr = __va(offset);
		else
			addr = arch_memremap(offset, size, MEMREMAP_WB);
	}

	/*
	 * If we don't have a mapping yet and more request flags are
	 * pending then we will be attempting to establish a new virtual
	 * address mapping.  Enforce that this mapping is not aliasing
	 * "System RAM"
	 */
	if (!addr && is_ram == REGION_INTERSECTS && flags) {
		WARN_ONCE(1, "memremap attempted on ram %pa size: %zu\n",
				&offset, size);
		return NULL;
	}

	if (!addr && (flags & MEMREMAP_WT)) {
		flags &= ~MEMREMAP_WT;
		addr = arch_memremap(offset, size, MEMREMAP_WT);
	}

	return addr;
}
EXPORT_SYMBOL(memremap);

void memunmap(void *addr)
{
	if (is_vmalloc_addr(addr))
		arch_memunmap(addr);
}
EXPORT_SYMBOL(memunmap);
