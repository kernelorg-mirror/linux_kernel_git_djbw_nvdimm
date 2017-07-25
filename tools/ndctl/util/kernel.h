#ifndef __UTIL_KERNEL_H__
#define __UTIL_KERNEL_H__
/*
 * Disable the dummy definition of cpu_to_le64 since we have one
 * locally from ccan. TODO: uplevel endian helpers to top-level tools/
 */
#define cpu_to_le64 cpu_to_le64
#include <linux/kernel.h>
#endif /* __UTIL_KERNEL_H__ */
