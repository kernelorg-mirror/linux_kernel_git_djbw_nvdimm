#include <linux/bio.h>
#include <linux/io.h>
#include <linux/export.h>
#include <xen/page.h>

bool xen_biovec_phys_mergeable(const struct bio_vec *vec1,
			       const struct bio_vec *vec2)
{
	unsigned long mfn1 = pfn_to_mfn(page_to_pfn(bvec_page(vec1)));
	unsigned long mfn2 = pfn_to_mfn(page_to_pfn(bvec_page(vec2)));

	return __BIOVEC_PHYS_MERGEABLE(vec1, vec2) &&
		((mfn1 == mfn2) || ((mfn1+1) == mfn2));
}
EXPORT_SYMBOL(xen_biovec_phys_mergeable);
