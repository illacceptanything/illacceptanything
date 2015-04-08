/*
 * Copyright (C) 2010 Pengutronix
 * Uwe Kleine-Koenig <u.kleine-koenig@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */
#include <asm/sizes.h>
#include <mach/hardware.h>
#include <mach/devices-common.h>

#define imx_fec_data_entry_single(soc)					\
	{								\
		.iobase = soc ## _FEC_BASE_ADDR,			\
		.irq = soc ## _INT_FEC,					\
	}

#ifdef CONFIG_ARCH_MX25
const struct imx_fec_data imx25_fec_data __initconst =
	imx_fec_data_entry_single(MX25);
#endif /* ifdef CONFIG_ARCH_MX25 */

#ifdef CONFIG_SOC_IMX27
const struct imx_fec_data imx27_fec_data __initconst =
	imx_fec_data_entry_single(MX27);
#endif /* ifdef CONFIG_SOC_IMX27 */

#ifdef CONFIG_ARCH_MX35
const struct imx_fec_data imx35_fec_data __initconst =
	imx_fec_data_entry_single(MX35);
#endif

#ifdef CONFIG_ARCH_MX51
const struct imx_fec_data imx51_fec_data __initconst =
	imx_fec_data_entry_single(MX51);
#endif

struct platform_device *__init imx_add_fec(
		const struct imx_fec_data *data,
		const struct fec_platform_data *pdata)
{
	struct resource res[] = {
		{
			.start = data->iobase,
			.end = data->iobase + SZ_4K,
			.flags = IORESOURCE_MEM,
		}, {
			.start = data->irq,
			.end = data->irq,
			.flags = IORESOURCE_IRQ,
		},
	};

	return imx_add_platform_device("fec", 0 /* -1? */,
			res, ARRAY_SIZE(res),
			pdata, sizeof(*pdata));
}
