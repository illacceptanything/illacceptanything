/*
 *  Copyright (C) 1999,2000 Arm Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *    - add MX31 specific definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/mm.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/pinctrl/machine.h>

#include <asm/pgtable.h>
#include <asm/mach/map.h>

#include "common.h"
#include "devices/devices-common.h"
#include "hardware.h"
#include "iomux-v3.h"
#include "mx25.h"

/*
 * This table defines static virtual address mappings for I/O regions.
 * These are the mappings common across all MX25 boards.
 */
static struct map_desc mx25_io_desc[] __initdata = {
	imx_map_entry(MX25, AVIC, MT_DEVICE_NONSHARED),
	imx_map_entry(MX25, AIPS1, MT_DEVICE_NONSHARED),
	imx_map_entry(MX25, AIPS2, MT_DEVICE_NONSHARED),
};

/*
 * This function initializes the memory map. It is called during the
 * system startup to create static physical to virtual memory mappings
 * for the IO modules.
 */
void __init mx25_map_io(void)
{
	iotable_init(mx25_io_desc, ARRAY_SIZE(mx25_io_desc));
}

void __init imx25_init_early(void)
{
	mxc_set_cpu_type(MXC_CPU_MX25);
	mxc_iomux_v3_init(MX25_IO_ADDRESS(MX25_IOMUXC_BASE_ADDR));
}

void __init mx25_init_irq(void)
{
	mxc_init_irq(MX25_IO_ADDRESS(MX25_AVIC_BASE_ADDR));
}

static struct sdma_platform_data imx25_sdma_pdata __initdata = {
	.fw_name = "sdma-imx25.bin",
};

static const struct resource imx25_audmux_res[] __initconst = {
	DEFINE_RES_MEM(MX25_AUDMUX_BASE_ADDR, SZ_16K),
};

void __init imx25_soc_init(void)
{
	mxc_arch_reset_init(MX25_IO_ADDRESS(MX25_WDOG_BASE_ADDR));
	mxc_device_init();

	/* i.mx25 has the i.mx35 type gpio */
	mxc_register_gpio("imx35-gpio", 0, MX25_GPIO1_BASE_ADDR, SZ_16K, MX25_INT_GPIO1, 0);
	mxc_register_gpio("imx35-gpio", 1, MX25_GPIO2_BASE_ADDR, SZ_16K, MX25_INT_GPIO2, 0);
	mxc_register_gpio("imx35-gpio", 2, MX25_GPIO3_BASE_ADDR, SZ_16K, MX25_INT_GPIO3, 0);
	mxc_register_gpio("imx35-gpio", 3, MX25_GPIO4_BASE_ADDR, SZ_16K, MX25_INT_GPIO4, 0);

	pinctrl_provide_dummies();
	/* i.mx25 has the i.mx35 type sdma */
	imx_add_imx_sdma("imx35-sdma", MX25_SDMA_BASE_ADDR, MX25_INT_SDMA, &imx25_sdma_pdata);
	/* i.mx25 has the i.mx31 type audmux */
	platform_device_register_simple("imx31-audmux", 0, imx25_audmux_res,
					ARRAY_SIZE(imx25_audmux_res));
}
