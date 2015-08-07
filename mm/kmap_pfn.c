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
#include <linux/rcupdate.h>
#include <linux/rculist.h>
#include <linux/highmem.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mm.h>

static LIST_HEAD(ranges);
static DEFINE_MUTEX(register_lock);

struct kmap {
	struct list_head list;
	struct resource *res;
	struct device *dev;
	void *base;
};

static void teardown_kmap(void *data)
{
	struct kmap *kmap = data;

	dev_dbg(kmap->dev, "kmap unregister %pr\n", kmap->res);
	mutex_lock(&register_lock);
	list_del_rcu(&kmap->list);
	mutex_unlock(&register_lock);
	synchronize_rcu();
	kfree(kmap);
}

int devm_register_kmap_pfn_range(struct device *dev, struct resource *res,
		void *base)
{
	struct kmap *kmap = kzalloc(sizeof(*kmap), GFP_KERNEL);
	int rc;

	if (!kmap)
		return -ENOMEM;

	INIT_LIST_HEAD(&kmap->list);
	kmap->res = res;
	kmap->base = base;
	kmap->dev = dev;
	rc = devm_add_action(dev, teardown_kmap, kmap);
	if (rc) {
		kfree(kmap);
		return rc;
	}
	dev_dbg(kmap->dev, "kmap register %pr\n", kmap->res);

	mutex_lock(&register_lock);
	list_add_rcu(&kmap->list, &ranges);
	mutex_unlock(&register_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(devm_register_kmap_pfn_range);

void *kmap_atomic_pfn_t(__pfn_t pfn)
{
	struct page *page = __pfn_t_to_page(pfn);
	resource_size_t addr;
	struct kmap *kmap;

	rcu_read_lock();
	if (page)
		return kmap_atomic(page);
	addr = __pfn_t_to_phys(pfn);
	list_for_each_entry_rcu(kmap, &ranges, list)
		if (addr >= kmap->res->start && addr <= kmap->res->end)
			return kmap->base + addr - kmap->res->start;

	/* only unlock in the error case */
	rcu_read_unlock();
	return NULL;
}
EXPORT_SYMBOL(kmap_atomic_pfn_t);

void kunmap_atomic_pfn_t(void *addr)
{
	struct kmap *kmap;
	bool dev_pfn = false;

	if (!addr)
		return;

	/*
	 * If the original __pfn_t had an entry in the memmap (i.e.
	 * !PFN_DEV) then 'addr' will be outside of the registered
	 * ranges and we'll need to kunmap_atomic() it.
	 */
	list_for_each_entry_rcu(kmap, &ranges, list)
		if (addr < kmap->base + resource_size(kmap->res)
				&& addr >= kmap->base) {
			dev_pfn = true;
			break;
		}

	if (!dev_pfn)
		kunmap_atomic(addr);

	/* signal that we are done with the range */
	rcu_read_unlock();
}
EXPORT_SYMBOL(kunmap_atomic_pfn_t);
