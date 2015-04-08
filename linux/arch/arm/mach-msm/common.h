/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __MACH_COMMON_H
#define __MACH_COMMON_H

extern void msm7x01_timer_init(void);
extern void msm7x30_timer_init(void);
extern void qsd8x50_timer_init(void);

extern void msm_map_common_io(void);
extern void msm_map_msm7x30_io(void);
extern void msm_map_qsd8x50_io(void);

extern void __iomem *__msm_ioremap_caller(phys_addr_t phys_addr, size_t size,
					  unsigned int mtype, void *caller);

struct msm_mmc_platform_data;

extern void msm_add_devices(void);
extern void msm_init_irq(void);
extern void msm_init_gpio(void);
extern int msm_add_sdcc(unsigned int controller,
			struct msm_mmc_platform_data *plat,
			unsigned int stat_irq, unsigned long stat_irq_flags);

#if defined(CONFIG_MSM_SMD) && defined(CONFIG_DEBUG_FS)
extern int smd_debugfs_init(void);
#else
static inline int smd_debugfs_init(void) { return 0; }
#endif

#endif
