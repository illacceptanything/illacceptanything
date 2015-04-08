/* linux/arch/arm/mach-msm/board-halibut.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/smc91x.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/setup.h>

#include <mach/irqs.h>
#include <mach/msm_iomap.h>

#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include "devices.h"
#include "common.h"

static struct resource smc91x_resources[] = {
	[0] = {
		.start	= 0x9C004300,
		.end	= 0x9C004400,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= MSM_GPIO_TO_INT(49),
		.end	= MSM_GPIO_TO_INT(49),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct smc91x_platdata smc91x_platdata = {
	.flags = SMC91X_USE_16BIT | SMC91X_NOWAIT,
};

static struct platform_device smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smc91x_resources),
	.resource	= smc91x_resources,
	.dev.platform_data = &smc91x_platdata,
};

static struct platform_device *devices[] __initdata = {
	&msm_clock_7x01a,
	&msm_device_gpio_7201,
	&msm_device_uart3,
	&msm_device_smd,
	&msm_device_nand,
	&msm_device_hsusb,
	&msm_device_i2c,
	&smc91x_device,
};

static void __init halibut_init_early(void)
{
	arch_ioremap_caller = __msm_ioremap_caller;
}

static void __init halibut_init_irq(void)
{
	msm_init_irq();
}

static void __init halibut_init(void)
{
	platform_add_devices(devices, ARRAY_SIZE(devices));
}

static void __init halibut_map_io(void)
{
	msm_map_common_io();
}

static void __init halibut_init_late(void)
{
	smd_debugfs_init();
}

MACHINE_START(HALIBUT, "Halibut Board (QCT SURF7200A)")
	.atag_offset	= 0x100,
	.map_io		= halibut_map_io,
	.init_early	= halibut_init_early,
	.init_irq	= halibut_init_irq,
	.init_machine	= halibut_init,
	.init_late	= halibut_init_late,
	.init_time	= msm7x01_timer_init,
MACHINE_END
