/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2012 MIPS Technologies, Inc.  All rights reserved.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/irqchip/mips-gic.h>
#include <linux/serial_8250.h>

#include <asm/mips-boards/sead3int.h>

#define UART(base)							\
{									\
	.mapbase	= base,						\
	.irq		= -1,						\
	.uartclk	= 14745600,					\
	.iotype		= UPIO_MEM32,					\
	.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_IOREMAP, \
	.regshift	= 2,						\
}

static struct plat_serial8250_port uart8250_data[] = {
	UART(0x1f000900),   /* ttyS0 = USB   */
	UART(0x1f000800),   /* ttyS1 = RS232 */
	{ },
};

static struct platform_device uart8250_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM2,
	.dev			= {
		.platform_data	= uart8250_data,
	},
};

static int __init uart8250_init(void)
{
	if (gic_present) {
		uart8250_data[0].irq = MIPS_GIC_IRQ_BASE + GIC_INT_UART0;
		uart8250_data[1].irq = MIPS_GIC_IRQ_BASE + GIC_INT_UART1;
	} else {
		uart8250_data[0].irq = MIPS_CPU_IRQ_BASE + CPU_INT_UART0;
		uart8250_data[1].irq = MIPS_CPU_IRQ_BASE + CPU_INT_UART1;
	}
	return platform_device_register(&uart8250_device);
}

module_init(uart8250_init);

MODULE_AUTHOR("Chris Dearman <chris@mips.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("8250 UART probe driver for SEAD3");
