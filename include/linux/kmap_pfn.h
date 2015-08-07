#ifndef _LINUX_KMAP_PFN_H
#define _LINUX_KMAP_PFN_H 1

#include <linux/highmem.h>

struct device;
struct resource;
#ifdef CONFIG_KMAP_PFN
extern void *kmap_atomic_pfn_t(__pfn_t pfn);
extern void kunmap_atomic_pfn_t(void *addr);
extern int devm_register_kmap_pfn_range(struct device *dev,
		struct resource *res, void *base);
#else
static inline void *kmap_atomic_pfn_t(__pfn_t pfn)
{
	return kmap_atomic(__pfn_t_to_page(pfn));
}

static inline void kunmap_atomic_pfn_t(void *addr)
{
	__kunmap_atomic(addr);
}

static inline int devm_register_kmap_pfn_range(struct device *dev,
		struct resource *res, void *base)
{
	return 0;
}
#endif /* CONFIG_KMAP_PFN */

#endif /* _LINUX_KMAP_PFN_H */
