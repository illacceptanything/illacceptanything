/*
 * ALSA PCM interface for the TI DAVINCI processor
 *
 * Author:      Vladimir Barinov, <vbarinov@embeddedalley.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DAVINCI_PCM_H
#define _DAVINCI_PCM_H

#include <linux/genalloc.h>
#include <linux/platform_data/davinci_asp.h>
#include <linux/platform_data/edma.h>

struct davinci_pcm_dma_params {
	int channel;			/* sync dma channel ID */
	unsigned short acnt;
	dma_addr_t dma_addr;		/* device physical address for DMA */
	unsigned sram_size;
	struct gen_pool *sram_pool;	/* SRAM gen_pool for ping pong */
	enum dma_event_q asp_chan_q;	/* event queue number for ASP channel */
	enum dma_event_q ram_chan_q;	/* event queue number for RAM channel */
	unsigned char data_type;	/* xfer data type */
	unsigned char convert_mono_stereo;
	unsigned int fifo_level;
};

#if IS_ENABLED(CONFIG_SND_DAVINCI_SOC)
int davinci_soc_platform_register(struct device *dev);
#else
static inline int davinci_soc_platform_register(struct device *dev)
{
	return 0;
}
#endif /* CONFIG_SND_DAVINCI_SOC */

#endif
