/*
 * r8a7790 Power management support
 *
 * Copyright (C) 2013  Renesas Electronics Corporation
 * Copyright (C) 2011  Renesas Solutions Corp.
 * Copyright (C) 2011  Magnus Damm
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/kernel.h>
#include <linux/smp.h>
#include <asm/io.h>
#include "common.h"
#include "pm-rcar.h"
#include "r8a7790.h"

/* RST */
#define RST		0xe6160000
#define CA15BAR		0x0020
#define CA7BAR		0x0030
#define CA15RESCNT	0x0040
#define CA7RESCNT	0x0044

/* On-chip RAM */
#define MERAM          0xe8080000

/* SYSC */
#define SYSCIER 0x0c
#define SYSCIMR 0x10

#if defined(CONFIG_SMP)

static void __init r8a7790_sysc_init(void)
{
	void __iomem *base = rcar_sysc_init(0xe6180000);

	/* enable all interrupt sources, but do not use interrupt handler */
	iowrite32(0x0131000e, base + SYSCIER);
	iowrite32(0, base + SYSCIMR);
}

#else /* CONFIG_SMP */

static inline void r8a7790_sysc_init(void) {}

#endif /* CONFIG_SMP */

void __init r8a7790_pm_init(void)
{
	void __iomem *p;
	u32 bar;
	static int once;

	if (once++)
		return;

	/* MERAM for jump stub, because BAR requires 256KB aligned address */
	p = ioremap_nocache(MERAM, shmobile_boot_size);
	memcpy_toio(p, shmobile_boot_vector, shmobile_boot_size);
	iounmap(p);

	/* setup reset vectors */
	p = ioremap_nocache(RST, 0x63);
	bar = (MERAM >> 8) & 0xfffffc00;
	writel_relaxed(bar, p + CA15BAR);
	writel_relaxed(bar, p + CA7BAR);
	writel_relaxed(bar | 0x10, p + CA15BAR);
	writel_relaxed(bar | 0x10, p + CA7BAR);

	/* de-assert reset for all CPUs */
	writel_relaxed((readl_relaxed(p + CA15RESCNT) & ~0x0f) | 0xa5a50000,
		       p + CA15RESCNT);
	writel_relaxed((readl_relaxed(p + CA7RESCNT) & ~0x0f) | 0x5a5a0000,
		       p + CA7RESCNT);
	iounmap(p);

	r8a7790_sysc_init();
	shmobile_smp_apmu_suspend_init();
}
