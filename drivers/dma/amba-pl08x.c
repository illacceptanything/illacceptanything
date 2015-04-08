/*
 * Copyright (c) 2006 ARM Ltd.
 * Copyright (c) 2010 ST-Ericsson SA
 *
 * Author: Peter Pearse <peter.pearse@arm.com>
 * Author: Linus Walleij <linus.walleij@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The full GNU General Public License is iin this distribution in the
 * file called COPYING.
 *
 * Documentation: ARM DDI 0196G == PL080
 * Documentation: ARM DDI 0218E	== PL081
 *
 * PL080 & PL081 both have 16 sets of DMA signals that can be routed to
 * any channel.
 *
 * The PL080 has 8 channels available for simultaneous use, and the PL081
 * has only two channels. So on these DMA controllers the number of channels
 * and the number of incoming DMA signals are two totally different things.
 * It is usually not possible to theoretically handle all physical signals,
 * so a multiplexing scheme with possible denial of use is necessary.
 *
 * The PL080 has a dual bus master, PL081 has a single master.
 *
 * Memory to peripheral transfer may be visualized as
 *	Get data from memory to DMAC
 *	Until no data left
 *		On burst request from peripheral
 *			Destination burst from DMAC to peripheral
 *			Clear burst request
 *	Raise terminal count interrupt
 *
 * For peripherals with a FIFO:
 * Source      burst size == half the depth of the peripheral FIFO
 * Destination burst size == the depth of the peripheral FIFO
 *
 * (Bursts are irrelevant for mem to mem transfers - there are no burst
 * signals, the DMA controller will simply facilitate its AHB master.)
 *
 * ASSUMES default (little) endianness for DMA transfers
 *
 * Only DMAC flow control is implemented
 *
 * Global TODO:
 * - Break out common code from arch/arm/mach-s3c64xx and share
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/dmapool.h>
#include <linux/amba/bus.h>
#include <linux/dmaengine.h>
#include <linux/amba/pl08x.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/hardware/pl080.h>
#include <asm/dma.h>
#include <asm/mach/dma.h>
#include <asm/atomic.h>
#include <asm/processor.h>
#include <asm/cacheflush.h>

#define DRIVER_NAME	"pl08xdmac"

/**
 * struct vendor_data - vendor-specific config parameters
 * for PL08x derivates
 * @name: the name of this specific variant
 * @channels: the number of channels available in this variant
 * @dualmaster: whether this version supports dual AHB masters
 * or not.
 */
struct vendor_data {
	char *name;
	u8 channels;
	bool dualmaster;
};

/*
 * PL08X private data structures
 * An LLI struct - see pl08x TRM
 * Note that next uses bit[0] as a bus bit,
 * start & end do not - their bus bit info
 * is in cctl
 */
struct lli {
	dma_addr_t src;
	dma_addr_t dst;
	dma_addr_t next;
	u32 cctl;
};

/**
 * struct pl08x_driver_data - the local state holder for the PL08x
 * @slave: slave engine for this instance
 * @memcpy: memcpy engine for this instance
 * @base: virtual memory base (remapped) for the PL08x
 * @adev: the corresponding AMBA (PrimeCell) bus entry
 * @vd: vendor data for this PL08x variant
 * @pd: platform data passed in from the platform/machine
 * @phy_chans: array of data for the physical channels
 * @pool: a pool for the LLI descriptors
 * @pool_ctr: counter of LLIs in the pool
 * @lock: a spinlock for this struct
 */
struct pl08x_driver_data {
	struct dma_device slave;
	struct dma_device memcpy;
	void __iomem *base;
	struct amba_device *adev;
	struct vendor_data *vd;
	struct pl08x_platform_data *pd;
	struct pl08x_phy_chan *phy_chans;
	struct dma_pool *pool;
	int pool_ctr;
	spinlock_t lock;
};

/*
 * PL08X specific defines
 */

/*
 * Memory boundaries: the manual for PL08x says that the controller
 * cannot read past a 1KiB boundary, so these defines are used to
 * create transfer LLIs that do not cross such boundaries.
 */
#define PL08X_BOUNDARY_SHIFT		(10)	/* 1KB 0x400 */
#define PL08X_BOUNDARY_SIZE		(1 << PL08X_BOUNDARY_SHIFT)

/* Minimum period between work queue runs */
#define PL08X_WQ_PERIODMIN	20

/* Size (bytes) of each LLI buffer allocated for one transfer */
# define PL08X_LLI_TSFR_SIZE	0x2000

/* Maximimum times we call dma_pool_alloc on this pool without freeing */
#define PL08X_MAX_ALLOCS	0x40
#define MAX_NUM_TSFR_LLIS	(PL08X_LLI_TSFR_SIZE/sizeof(struct lli))
#define PL08X_ALIGN		8

static inline struct pl08x_dma_chan *to_pl08x_chan(struct dma_chan *chan)
{
	return container_of(chan, struct pl08x_dma_chan, chan);
}

/*
 * Physical channel handling
 */

/* Whether a certain channel is busy or not */
static int pl08x_phy_channel_busy(struct pl08x_phy_chan *ch)
{
	unsigned int val;

	val = readl(ch->base + PL080_CH_CONFIG);
	return val & PL080_CONFIG_ACTIVE;
}

/*
 * Set the initial DMA register values i.e. those for the first LLI
 * The next lli pointer and the configuration interrupt bit have
 * been set when the LLIs were constructed
 */
static void pl08x_set_cregs(struct pl08x_driver_data *pl08x,
			    struct pl08x_phy_chan *ch)
{
	/* Wait for channel inactive */
	while (pl08x_phy_channel_busy(ch))
		;

	dev_vdbg(&pl08x->adev->dev,
		"WRITE channel %d: csrc=%08x, cdst=%08x, "
		 "cctl=%08x, clli=%08x, ccfg=%08x\n",
		ch->id,
		ch->csrc,
		ch->cdst,
		ch->cctl,
		ch->clli,
		ch->ccfg);

	writel(ch->csrc, ch->base + PL080_CH_SRC_ADDR);
	writel(ch->cdst, ch->base + PL080_CH_DST_ADDR);
	writel(ch->clli, ch->base + PL080_CH_LLI);
	writel(ch->cctl, ch->base + PL080_CH_CONTROL);
	writel(ch->ccfg, ch->base + PL080_CH_CONFIG);
}

static inline void pl08x_config_phychan_for_txd(struct pl08x_dma_chan *plchan)
{
	struct pl08x_channel_data *cd = plchan->cd;
	struct pl08x_phy_chan *phychan = plchan->phychan;
	struct pl08x_txd *txd = plchan->at;

	/* Copy the basic control register calculated at transfer config */
	phychan->csrc = txd->csrc;
	phychan->cdst = txd->cdst;
	phychan->clli = txd->clli;
	phychan->cctl = txd->cctl;

	/* Assign the signal to the proper control registers */
	phychan->ccfg = cd->ccfg;
	phychan->ccfg &= ~PL080_CONFIG_SRC_SEL_MASK;
	phychan->ccfg &= ~PL080_CONFIG_DST_SEL_MASK;
	/* If it wasn't set from AMBA, ignore it */
	if (txd->direction == DMA_TO_DEVICE)
		/* Select signal as destination */
		phychan->ccfg |=
			(phychan->signal << PL080_CONFIG_DST_SEL_SHIFT);
	else if (txd->direction == DMA_FROM_DEVICE)
		/* Select signal as source */
		phychan->ccfg |=
			(phychan->signal << PL080_CONFIG_SRC_SEL_SHIFT);
	/* Always enable error interrupts */
	phychan->ccfg |= PL080_CONFIG_ERR_IRQ_MASK;
	/* Always enable terminal interrupts */
	phychan->ccfg |= PL080_CONFIG_TC_IRQ_MASK;
}

/*
 * Enable the DMA channel
 * Assumes all other configuration bits have been set
 * as desired before this code is called
 */
static void pl08x_enable_phy_chan(struct pl08x_driver_data *pl08x,
				  struct pl08x_phy_chan *ch)
{
	u32 val;

	/*
	 * Do not access config register until channel shows as disabled
	 */
	while (readl(pl08x->base + PL080_EN_CHAN) & (1 << ch->id))
		;

	/*
	 * Do not access config register until channel shows as inactive
	 */
	val = readl(ch->base + PL080_CH_CONFIG);
	while ((val & PL080_CONFIG_ACTIVE) || (val & PL080_CONFIG_ENABLE))
		val = readl(ch->base + PL080_CH_CONFIG);

	writel(val | PL080_CONFIG_ENABLE, ch->base + PL080_CH_CONFIG);
}

/*
 * Overall DMAC remains enabled always.
 *
 * Disabling individual channels could lose data.
 *
 * Disable the peripheral DMA after disabling the DMAC
 * in order to allow the DMAC FIFO to drain, and
 * hence allow the channel to show inactive
 *
 */
static void pl08x_pause_phy_chan(struct pl08x_phy_chan *ch)
{
	u32 val;

	/* Set the HALT bit and wait for the FIFO to drain */
	val = readl(ch->base + PL080_CH_CONFIG);
	val |= PL080_CONFIG_HALT;
	writel(val, ch->base + PL080_CH_CONFIG);

	/* Wait for channel inactive */
	while (pl08x_phy_channel_busy(ch))
		;
}

static void pl08x_resume_phy_chan(struct pl08x_phy_chan *ch)
{
	u32 val;

	/* Clear the HALT bit */
	val = readl(ch->base + PL080_CH_CONFIG);
	val &= ~PL080_CONFIG_HALT;
	writel(val, ch->base + PL080_CH_CONFIG);
}


/* Stops the channel */
static void pl08x_stop_phy_chan(struct pl08x_phy_chan *ch)
{
	u32 val;

	pl08x_pause_phy_chan(ch);

	/* Disable channel */
	val = readl(ch->base + PL080_CH_CONFIG);
	val &= ~PL080_CONFIG_ENABLE;
	val &= ~PL080_CONFIG_ERR_IRQ_MASK;
	val &= ~PL080_CONFIG_TC_IRQ_MASK;
	writel(val, ch->base + PL080_CH_CONFIG);
}

static inline u32 get_bytes_in_cctl(u32 cctl)
{
	/* The source width defines the number of bytes */
	u32 bytes = cctl & PL080_CONTROL_TRANSFER_SIZE_MASK;

	switch (cctl >> PL080_CONTROL_SWIDTH_SHIFT) {
	case PL080_WIDTH_8BIT:
		break;
	case PL080_WIDTH_16BIT:
		bytes *= 2;
		break;
	case PL080_WIDTH_32BIT:
		bytes *= 4;
		break;
	}
	return bytes;
}

/* The channel should be paused when calling this */
static u32 pl08x_getbytes_chan(struct pl08x_dma_chan *plchan)
{
	struct pl08x_phy_chan *ch;
	struct pl08x_txd *txdi = NULL;
	struct pl08x_txd *txd;
	unsigned long flags;
	u32 bytes = 0;

	spin_lock_irqsave(&plchan->lock, flags);

	ch = plchan->phychan;
	txd = plchan->at;

	/*
	 * Next follow the LLIs to get the number of pending bytes in the
	 * currently active transaction.
	 */
	if (ch && txd) {
		struct lli *llis_va = txd->llis_va;
		struct lli *llis_bus = (struct lli *) txd->llis_bus;
		u32 clli = readl(ch->base + PL080_CH_LLI);

		/* First get the bytes in the current active LLI */
		bytes = get_bytes_in_cctl(readl(ch->base + PL080_CH_CONTROL));

		if (clli) {
			int i = 0;

			/* Forward to the LLI pointed to by clli */
			while ((clli != (u32) &(llis_bus[i])) &&
			       (i < MAX_NUM_TSFR_LLIS))
				i++;

			while (clli) {
				bytes += get_bytes_in_cctl(llis_va[i].cctl);
				/*
				 * A clli of 0x00000000 will terminate the
				 * LLI list
				 */
				clli = llis_va[i].next;
				i++;
			}
		}
	}

	/* Sum up all queued transactions */
	if (!list_empty(&plchan->desc_list)) {
		list_for_each_entry(txdi, &plchan->desc_list, node) {
			bytes += txdi->len;
		}

	}

	spin_unlock_irqrestore(&plchan->lock, flags);

	return bytes;
}

/*
 * Allocate a physical channel for a virtual channel
 */
static struct pl08x_phy_chan *
pl08x_get_phy_channel(struct pl08x_driver_data *pl08x,
		      struct pl08x_dma_chan *virt_chan)
{
	struct pl08x_phy_chan *ch = NULL;
	unsigned long flags;
	int i;

	/*
	 * Try to locate a physical channel to be used for
	 * this transfer. If all are taken return NULL and
	 * the requester will have to cope by using some fallback
	 * PIO mode or retrying later.
	 */
	for (i = 0; i < pl08x->vd->channels; i++) {
		ch = &pl08x->phy_chans[i];

		spin_lock_irqsave(&ch->lock, flags);

		if (!ch->serving) {
			ch->serving = virt_chan;
			ch->signal = -1;
			spin_unlock_irqrestore(&ch->lock, flags);
			break;
		}

		spin_unlock_irqrestore(&ch->lock, flags);
	}

	if (i == pl08x->vd->channels) {
		/* No physical channel available, cope with it */
		return NULL;
	}

	return ch;
}

static inline void pl08x_put_phy_channel(struct pl08x_driver_data *pl08x,
					 struct pl08x_phy_chan *ch)
{
	unsigned long flags;

	/* Stop the channel and clear its interrupts */
	pl08x_stop_phy_chan(ch);
	writel((1 << ch->id), pl08x->base + PL080_ERR_CLEAR);
	writel((1 << ch->id), pl08x->base + PL080_TC_CLEAR);

	/* Mark it as free */
	spin_lock_irqsave(&ch->lock, flags);
	ch->serving = NULL;
	spin_unlock_irqrestore(&ch->lock, flags);
}

/*
 * LLI handling
 */

static inline unsigned int pl08x_get_bytes_for_cctl(unsigned int coded)
{
	switch (coded) {
	case PL080_WIDTH_8BIT:
		return 1;
	case PL080_WIDTH_16BIT:
		return 2;
	case PL080_WIDTH_32BIT:
		return 4;
	default:
		break;
	}
	BUG();
	return 0;
}

static inline u32 pl08x_cctl_bits(u32 cctl, u8 srcwidth, u8 dstwidth,
				  u32 tsize)
{
	u32 retbits = cctl;

	/* Remove all src, dst and transfersize bits */
	retbits &= ~PL080_CONTROL_DWIDTH_MASK;
	retbits &= ~PL080_CONTROL_SWIDTH_MASK;
	retbits &= ~PL080_CONTROL_TRANSFER_SIZE_MASK;

	/* Then set the bits according to the parameters */
	switch (srcwidth) {
	case 1:
		retbits |= PL080_WIDTH_8BIT << PL080_CONTROL_SWIDTH_SHIFT;
		break;
	case 2:
		retbits |= PL080_WIDTH_16BIT << PL080_CONTROL_SWIDTH_SHIFT;
		break;
	case 4:
		retbits |= PL080_WIDTH_32BIT << PL080_CONTROL_SWIDTH_SHIFT;
		break;
	default:
		BUG();
		break;
	}

	switch (dstwidth) {
	case 1:
		retbits |= PL080_WIDTH_8BIT << PL080_CONTROL_DWIDTH_SHIFT;
		break;
	case 2:
		retbits |= PL080_WIDTH_16BIT << PL080_CONTROL_DWIDTH_SHIFT;
		break;
	case 4:
		retbits |= PL080_WIDTH_32BIT << PL080_CONTROL_DWIDTH_SHIFT;
		break;
	default:
		BUG();
		break;
	}

	retbits |= tsize << PL080_CONTROL_TRANSFER_SIZE_SHIFT;
	return retbits;
}

/*
 * Autoselect a master bus to use for the transfer
 * this prefers the destination bus if both available
 * if fixed address on one bus the other will be chosen
 */
void pl08x_choose_master_bus(struct pl08x_bus_data *src_bus,
	struct pl08x_bus_data *dst_bus, struct pl08x_bus_data **mbus,
	struct pl08x_bus_data **sbus, u32 cctl)
{
	if (!(cctl & PL080_CONTROL_DST_INCR)) {
		*mbus = src_bus;
		*sbus = dst_bus;
	} else if (!(cctl & PL080_CONTROL_SRC_INCR)) {
		*mbus = dst_bus;
		*sbus = src_bus;
	} else {
		if (dst_bus->buswidth == 4) {
			*mbus = dst_bus;
			*sbus = src_bus;
		} else if (src_bus->buswidth == 4) {
			*mbus = src_bus;
			*sbus = dst_bus;
		} else if (dst_bus->buswidth == 2) {
			*mbus = dst_bus;
			*sbus = src_bus;
		} else if (src_bus->buswidth == 2) {
			*mbus = src_bus;
			*sbus = dst_bus;
		} else {
			/* src_bus->buswidth == 1 */
			*mbus = dst_bus;
			*sbus = src_bus;
		}
	}
}

/*
 * Fills in one LLI for a certain transfer descriptor
 * and advance the counter
 */
int pl08x_fill_lli_for_desc(struct pl08x_driver_data *pl08x,
			    struct pl08x_txd *txd, int num_llis, int len,
			    u32 cctl, u32 *remainder)
{
	struct lli *llis_va = txd->llis_va;
	struct lli *llis_bus = (struct lli *) txd->llis_bus;

	BUG_ON(num_llis >= MAX_NUM_TSFR_LLIS);

	llis_va[num_llis].cctl		= cctl;
	llis_va[num_llis].src		= txd->srcbus.addr;
	llis_va[num_llis].dst		= txd->dstbus.addr;

	/*
	 * On versions with dual masters, you can optionally AND on
	 * PL080_LLI_LM_AHB2 to the LLI to tell the hardware to read
	 * in new LLIs with that controller, but we always try to
	 * choose AHB1 to point into memory. The idea is to have AHB2
	 * fixed on the peripheral and AHB1 messing around in the
	 * memory. So we don't manipulate this bit currently.
	 */

	llis_va[num_llis].next =
		(dma_addr_t)((u32) &(llis_bus[num_llis + 1]));

	if (cctl & PL080_CONTROL_SRC_INCR)
		txd->srcbus.addr += len;
	if (cctl & PL080_CONTROL_DST_INCR)
		txd->dstbus.addr += len;

	*remainder -= len;

	return num_llis + 1;
}

/*
 * Return number of bytes to fill to boundary, or len
 */
static inline u32 pl08x_pre_boundary(u32 addr, u32 len)
{
	u32 boundary;

	boundary = ((addr >> PL08X_BOUNDARY_SHIFT) + 1)
		<< PL08X_BOUNDARY_SHIFT;

	if (boundary < addr + len)
		return boundary - addr;
	else
		return len;
}

/*
 * This fills in the table of LLIs for the transfer descriptor
 * Note that we assume we never have to change the burst sizes
 * Return 0 for error
 */
static int pl08x_fill_llis_for_desc(struct pl08x_driver_data *pl08x,
			      struct pl08x_txd *txd)
{
	struct pl08x_channel_data *cd = txd->cd;
	struct pl08x_bus_data *mbus, *sbus;
	u32 remainder;
	int num_llis = 0;
	u32 cctl;
	int max_bytes_per_lli;
	int total_bytes = 0;
	struct lli *llis_va;
	struct lli *llis_bus;

	if (!txd) {
		dev_err(&pl08x->adev->dev, "%s no descriptor\n", __func__);
		return 0;
	}

	txd->llis_va = dma_pool_alloc(pl08x->pool, GFP_NOWAIT,
				      &txd->llis_bus);
	if (!txd->llis_va) {
		dev_err(&pl08x->adev->dev, "%s no memory for llis\n", __func__);
		return 0;
	}

	pl08x->pool_ctr++;

	/*
	 * Initialize bus values for this transfer
	 * from the passed optimal values
	 */
	if (!cd) {
		dev_err(&pl08x->adev->dev, "%s no channel data\n", __func__);
		return 0;
	}

	/* Get the default CCTL from the platform data */
	cctl = cd->cctl;

	/*
	 * On the PL080 we have two bus masters and we
	 * should select one for source and one for
	 * destination. We try to use AHB2 for the
	 * bus which does not increment (typically the
	 * peripheral) else we just choose something.
	 */
	cctl &= ~(PL080_CONTROL_DST_AHB2 | PL080_CONTROL_SRC_AHB2);
	if (pl08x->vd->dualmaster) {
		if (cctl & PL080_CONTROL_SRC_INCR)
			/* Source increments, use AHB2 for destination */
			cctl |= PL080_CONTROL_DST_AHB2;
		else if (cctl & PL080_CONTROL_DST_INCR)
			/* Destination increments, use AHB2 for source */
			cctl |= PL080_CONTROL_SRC_AHB2;
		else
			/* Just pick something, source AHB1 dest AHB2 */
			cctl |= PL080_CONTROL_DST_AHB2;
	}

	/* Find maximum width of the source bus */
	txd->srcbus.maxwidth =
		pl08x_get_bytes_for_cctl((cctl & PL080_CONTROL_SWIDTH_MASK) >>
				       PL080_CONTROL_SWIDTH_SHIFT);

	/* Find maximum width of the destination bus */
	txd->dstbus.maxwidth =
		pl08x_get_bytes_for_cctl((cctl & PL080_CONTROL_DWIDTH_MASK) >>
				       PL080_CONTROL_DWIDTH_SHIFT);

	/* Set up the bus widths to the maximum */
	txd->srcbus.buswidth = txd->srcbus.maxwidth;
	txd->dstbus.buswidth = txd->dstbus.maxwidth;
	dev_vdbg(&pl08x->adev->dev,
		 "%s source bus is %d bytes wide, dest bus is %d bytes wide\n",
		 __func__, txd->srcbus.buswidth, txd->dstbus.buswidth);


	/*
	 * Bytes transferred == tsize * MIN(buswidths), not max(buswidths)
	 */
	max_bytes_per_lli = min(txd->srcbus.buswidth, txd->dstbus.buswidth) *
		PL080_CONTROL_TRANSFER_SIZE_MASK;
	dev_vdbg(&pl08x->adev->dev,
		 "%s max bytes per lli = %d\n",
		 __func__, max_bytes_per_lli);

	/* We need to count this down to zero */
	remainder = txd->len;
	dev_vdbg(&pl08x->adev->dev,
		 "%s remainder = %d\n",
		 __func__, remainder);

	/*
	 * Choose bus to align to
	 * - prefers destination bus if both available
	 * - if fixed address on one bus chooses other
	 * - modifies cctl to choose an apropriate master
	 */
	pl08x_choose_master_bus(&txd->srcbus, &txd->dstbus,
				&mbus, &sbus, cctl);


	/*
	 * The lowest bit of the LLI register
	 * is also used to indicate which master to
	 * use for reading the LLIs.
	 */

	if (txd->len < mbus->buswidth) {
		/*
		 * Less than a bus width available
		 * - send as single bytes
		 */
		while (remainder) {
			dev_vdbg(&pl08x->adev->dev,
				 "%s single byte LLIs for a transfer of "
				 "less than a bus width (remain %08x)\n",
				 __func__, remainder);
			cctl = pl08x_cctl_bits(cctl, 1, 1, 1);
			num_llis =
				pl08x_fill_lli_for_desc(pl08x, txd, num_llis, 1,
					cctl, &remainder);
			total_bytes++;
		}
	} else {
		/*
		 *  Make one byte LLIs until master bus is aligned
		 *  - slave will then be aligned also
		 */
		while ((mbus->addr) % (mbus->buswidth)) {
			dev_vdbg(&pl08x->adev->dev,
				"%s adjustment lli for less than bus width "
				 "(remain %08x)\n",
				 __func__, remainder);
			cctl = pl08x_cctl_bits(cctl, 1, 1, 1);
			num_llis = pl08x_fill_lli_for_desc
				(pl08x, txd, num_llis, 1, cctl, &remainder);
			total_bytes++;
		}

		/*
		 *  Master now aligned
		 * - if slave is not then we must set its width down
		 */
		if (sbus->addr % sbus->buswidth) {
			dev_dbg(&pl08x->adev->dev,
				"%s set down bus width to one byte\n",
				 __func__);

			sbus->buswidth = 1;
		}

		/*
		 * Make largest possible LLIs until less than one bus
		 * width left
		 */
		while (remainder > (mbus->buswidth - 1)) {
			int lli_len, target_len;
			int tsize;
			int odd_bytes;

			/*
			 * If enough left try to send max possible,
			 * otherwise try to send the remainder
			 */
			target_len = remainder;
			if (remainder > max_bytes_per_lli)
				target_len = max_bytes_per_lli;

			/*
			 * Set bus lengths for incrementing busses
			 * to number of bytes which fill to next memory
			 * boundary
			 */
			if (cctl & PL080_CONTROL_SRC_INCR)
				txd->srcbus.fill_bytes =
					pl08x_pre_boundary(
						txd->srcbus.addr,
						remainder);
			else
				txd->srcbus.fill_bytes =
					max_bytes_per_lli;

			if (cctl & PL080_CONTROL_DST_INCR)
				txd->dstbus.fill_bytes =
					pl08x_pre_boundary(
						txd->dstbus.addr,
						remainder);
			else
				txd->dstbus.fill_bytes =
						max_bytes_per_lli;

			/*
			 *  Find the nearest
			 */
			lli_len	= min(txd->srcbus.fill_bytes,
				txd->dstbus.fill_bytes);

			BUG_ON(lli_len > remainder);

			if (lli_len <= 0) {
				dev_err(&pl08x->adev->dev,
					"%s lli_len is %d, <= 0\n",
						__func__, lli_len);
				return 0;
			}

			if (lli_len == target_len) {
				/*
				 * Can send what we wanted
				 */
				/*
				 *  Maintain alignment
				 */
				lli_len	= (lli_len/mbus->buswidth) *
							mbus->buswidth;
				odd_bytes = 0;
			} else {
				/*
				 * So now we know how many bytes to transfer
				 * to get to the nearest boundary
				 * The next lli will past the boundary
				 * - however we may be working to a boundary
				 *   on the slave bus
				 *   We need to ensure the master stays aligned
				 */
				odd_bytes = lli_len % mbus->buswidth;
				/*
				 * - and that we are working in multiples
				 *   of the bus widths
				 */
				lli_len -= odd_bytes;

			}

			if (lli_len) {
				/*
				 * Check against minimum bus alignment:
				 * Calculate actual transfer size in relation
				 * to bus width an get a maximum remainder of
				 * the smallest bus width - 1
				 */
				/* FIXME: use round_down()? */
				tsize = lli_len / min(mbus->buswidth,
						      sbus->buswidth);
				lli_len	= tsize * min(mbus->buswidth,
						      sbus->buswidth);

				if (target_len != lli_len) {
					dev_vdbg(&pl08x->adev->dev,
					"%s can't send what we want. Desired %08x, lli of %08x bytes in txd of %08x\n",
					__func__, target_len, lli_len, txd->len);
				}

				cctl = pl08x_cctl_bits(cctl,
						       txd->srcbus.buswidth,
						       txd->dstbus.buswidth,
						       tsize);

				dev_vdbg(&pl08x->adev->dev,
					"%s fill lli with single lli chunk of size %08x (remainder %08x)\n",
					__func__, lli_len, remainder);
				num_llis = pl08x_fill_lli_for_desc(pl08x, txd,
						num_llis, lli_len, cctl,
						&remainder);
				total_bytes += lli_len;
			}


			if (odd_bytes) {
				/*
				 * Creep past the boundary,
				 * maintaining master alignment
				 */
				int j;
				for (j = 0; (j < mbus->buswidth)
						&& (remainder); j++) {
					cctl = pl08x_cctl_bits(cctl, 1, 1, 1);
					dev_vdbg(&pl08x->adev->dev,
						"%s align with boundardy, single byte (remain %08x)\n",
						__func__, remainder);
					num_llis =
						pl08x_fill_lli_for_desc(pl08x,
							txd, num_llis, 1,
							cctl, &remainder);
					total_bytes++;
				}
			}
		}

		/*
		 * Send any odd bytes
		 */
		if (remainder < 0) {
			dev_err(&pl08x->adev->dev, "%s remainder not fitted 0x%08x bytes\n",
					__func__, remainder);
			return 0;
		}

		while (remainder) {
			cctl = pl08x_cctl_bits(cctl, 1, 1, 1);
			dev_vdbg(&pl08x->adev->dev,
				"%s align with boundardy, single odd byte (remain %d)\n",
				__func__, remainder);
			num_llis = pl08x_fill_lli_for_desc(pl08x, txd, num_llis,
					1, cctl, &remainder);
			total_bytes++;
		}
	}
	if (total_bytes != txd->len) {
		dev_err(&pl08x->adev->dev,
			"%s size of encoded lli:s don't match total txd, transferred 0x%08x from size 0x%08x\n",
			__func__, total_bytes, txd->len);
		return 0;
	}

	if (num_llis >= MAX_NUM_TSFR_LLIS) {
		dev_err(&pl08x->adev->dev,
			"%s need to increase MAX_NUM_TSFR_LLIS from 0x%08x\n",
			__func__, (u32) MAX_NUM_TSFR_LLIS);
		return 0;
	}
	/*
	 * Decide whether this is a loop or a terminated transfer
	 */
	llis_va = txd->llis_va;
	llis_bus = (struct lli *) txd->llis_bus;

	if (cd->circular_buffer) {
		/*
		 * Loop the circular buffer so that the next element
		 * points back to the beginning of the LLI.
		 */
		llis_va[num_llis - 1].next =
			(dma_addr_t)((unsigned int)&(llis_bus[0]));
	} else {
		/*
		 * On non-circular buffers, the final LLI terminates
		 * the LLI.
		 */
		llis_va[num_llis - 1].next = 0;
		/*
		 * The final LLI element shall also fire an interrupt
		 */
		llis_va[num_llis - 1].cctl |= PL080_CONTROL_TC_IRQ_EN;
	}

	/* Now store the channel register values */
	txd->csrc = llis_va[0].src;
	txd->cdst = llis_va[0].dst;
	if (num_llis > 1)
		txd->clli = llis_va[0].next;
	else
		txd->clli = 0;

	txd->cctl = llis_va[0].cctl;
	/* ccfg will be set at physical channel allocation time */

#ifdef VERBOSE_DEBUG
	{
		int i;

		for (i = 0; i < num_llis; i++) {
			dev_vdbg(&pl08x->adev->dev,
				 "lli %d @%p: csrc=%08x, cdst=%08x, cctl=%08x, clli=%08x\n",
				 i,
				 &llis_va[i],
				 llis_va[i].src,
				 llis_va[i].dst,
				 llis_va[i].cctl,
				 llis_va[i].next
				);
		}
	}
#endif

	return num_llis;
}

/* You should call this with the struct pl08x lock held */
static void pl08x_free_txd(struct pl08x_driver_data *pl08x,
			   struct pl08x_txd *txd)
{
	if (!txd)
		dev_err(&pl08x->adev->dev,
			"%s no descriptor to free\n",
			__func__);

	/* Free the LLI */
	dma_pool_free(pl08x->pool, txd->llis_va,
		      txd->llis_bus);

	pl08x->pool_ctr--;

	kfree(txd);
}

static void pl08x_free_txd_list(struct pl08x_driver_data *pl08x,
				struct pl08x_dma_chan *plchan)
{
	struct pl08x_txd *txdi = NULL;
	struct pl08x_txd *next;

	if (!list_empty(&plchan->desc_list)) {
		list_for_each_entry_safe(txdi,
					 next, &plchan->desc_list, node) {
			list_del(&txdi->node);
			pl08x_free_txd(pl08x, txdi);
		}

	}
}

/*
 * The DMA ENGINE API
 */
static int pl08x_alloc_chan_resources(struct dma_chan *chan)
{
	return 0;
}

static void pl08x_free_chan_resources(struct dma_chan *chan)
{
}

/*
 * This should be called with the channel plchan->lock held
 */
static int prep_phy_channel(struct pl08x_dma_chan *plchan,
			    struct pl08x_txd *txd)
{
	struct pl08x_driver_data *pl08x = plchan->host;
	struct pl08x_phy_chan *ch;
	int ret;

	/* Check if we already have a channel */
	if (plchan->phychan)
		return 0;

	ch = pl08x_get_phy_channel(pl08x, plchan);
	if (!ch) {
		/* No physical channel available, cope with it */
		dev_dbg(&pl08x->adev->dev, "no physical channel available for xfer on %s\n", plchan->name);
		return -EBUSY;
	}

	/*
	 * OK we have a physical channel: for memcpy() this is all we
	 * need, but for slaves the physical signals may be muxed!
	 * Can the platform allow us to use this channel?
	 */
	if (plchan->slave &&
	    ch->signal < 0 &&
	    pl08x->pd->get_signal) {
		ret = pl08x->pd->get_signal(plchan);
		if (ret < 0) {
			dev_dbg(&pl08x->adev->dev,
				"unable to use physical channel %d for transfer on %s due to platform restrictions\n",
				ch->id, plchan->name);
			/* Release physical channel & return */
			pl08x_put_phy_channel(pl08x, ch);
			return -EBUSY;
		}
		ch->signal = ret;
	}

	dev_dbg(&pl08x->adev->dev, "allocated physical channel %d and signal %d for xfer on %s\n",
		 ch->id,
		 ch->signal,
		 plchan->name);

	plchan->phychan = ch;

	return 0;
}

static dma_cookie_t pl08x_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct pl08x_dma_chan *plchan = to_pl08x_chan(tx->chan);

	atomic_inc(&plchan->last_issued);
	tx->cookie = atomic_read(&plchan->last_issued);
	/* This unlock follows the lock in the prep() function */
	spin_unlock_irqrestore(&plchan->lock, plchan->lockflags);

	return tx->cookie;
}

static struct dma_async_tx_descriptor *pl08x_prep_dma_interrupt(
		struct dma_chan *chan, unsigned long flags)
{
	struct dma_async_tx_descriptor *retval = NULL;

	return retval;
}

/*
 * Code accessing dma_async_is_complete() in a tight loop
 * may give problems - could schedule where indicated.
 * If slaves are relying on interrupts to signal completion this
 * function must not be called with interrupts disabled
 */
static enum dma_status
pl08x_dma_tx_status(struct dma_chan *chan,
		    dma_cookie_t cookie,
		    struct dma_tx_state *txstate)
{
	struct pl08x_dma_chan *plchan = to_pl08x_chan(chan);
	dma_cookie_t last_used;
	dma_cookie_t last_complete;
	enum dma_status ret;
	u32 bytesleft = 0;

	last_used = atomic_read(&plchan->last_issued);
	last_complete = plchan->lc;

	ret = dma_async_is_complete(cookie, last_complete, last_used);
	if (ret == DMA_SUCCESS) {
		dma_set_tx_state(txstate, last_complete, last_used, 0);
		return ret;
	}

	/*
	 * schedule(); could be inserted here
	 */

	/*
	 * This cookie not complete yet
	 */
	last_used = atomic_read(&plchan->last_issued);
	last_complete = plchan->lc;

	/* Get number of bytes left in the active transactions and queue */
	bytesleft = pl08x_getbytes_chan(plchan);

	dma_set_tx_state(txstate, last_complete, last_used,
			 bytesleft);

	if (plchan->state == PL08X_CHAN_PAUSED)
		return DMA_PAUSED;

	/* Whether waiting or running, we're in progress */
	return DMA_IN_PROGRESS;
}

/* PrimeCell DMA extension */
struct burst_table {
	int burstwords;
	u32 reg;
};

static const struct burst_table burst_sizes[] = {
	{
		.burstwords = 256,
		.reg = (PL080_BSIZE_256 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_256 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 128,
		.reg = (PL080_BSIZE_128 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_128 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 64,
		.reg = (PL080_BSIZE_64 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_64 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 32,
		.reg = (PL080_BSIZE_32 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_32 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 16,
		.reg = (PL080_BSIZE_16 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_16 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 8,
		.reg = (PL080_BSIZE_8 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_8 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 4,
		.reg = (PL080_BSIZE_4 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_4 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
	{
		.burstwords = 1,
		.reg = (PL080_BSIZE_1 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_1 << PL080_CONTROL_DB_SIZE_SHIFT),
	},
};

static void dma_set_runtime_config(struct dma_chan *chan,
			       struct dma_slave_config *config)
{
	struct pl08x_dma_chan *plchan = to_pl08x_chan(chan);
	struct pl08x_driver_data *pl08x = plchan->host;
	struct pl08x_channel_data *cd = plchan->cd;
	enum dma_slave_buswidth addr_width;
	u32 maxburst;
	u32 cctl = 0;
	/* Mask out all except src and dst channel */
	u32 ccfg = cd->ccfg & 0x000003DEU;
	int i = 0;

	/* Transfer direction */
	plchan->runtime_direction = config->direction;
	if (config->direction == DMA_TO_DEVICE) {
		plchan->runtime_addr = config->dst_addr;
		cctl |= PL080_CONTROL_SRC_INCR;
		ccfg |= PL080_FLOW_MEM2PER << PL080_CONFIG_FLOW_CONTROL_SHIFT;
		addr_width = config->dst_addr_width;
		maxburst = config->dst_maxburst;
	} else if (config->direction == DMA_FROM_DEVICE) {
		plchan->runtime_addr = config->src_addr;
		cctl |= PL080_CONTROL_DST_INCR;
		ccfg |= PL080_FLOW_PER2MEM << PL080_CONFIG_FLOW_CONTROL_SHIFT;
		addr_width = config->src_addr_width;
		maxburst = config->src_maxburst;
	} else {
		dev_err(&pl08x->adev->dev,
			"bad runtime_config: alien transfer direction\n");
		return;
	}

	switch (addr_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		cctl |= (PL080_WIDTH_8BIT << PL080_CONTROL_SWIDTH_SHIFT) |
			(PL080_WIDTH_8BIT << PL080_CONTROL_DWIDTH_SHIFT);
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		cctl |= (PL080_WIDTH_16BIT << PL080_CONTROL_SWIDTH_SHIFT) |
			(PL080_WIDTH_16BIT << PL080_CONTROL_DWIDTH_SHIFT);
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		cctl |= (PL080_WIDTH_32BIT << PL080_CONTROL_SWIDTH_SHIFT) |
			(PL080_WIDTH_32BIT << PL080_CONTROL_DWIDTH_SHIFT);
		break;
	default:
		dev_err(&pl08x->adev->dev,
			"bad runtime_config: alien address width\n");
		return;
	}

	/*
	 * Now decide on a maxburst:
	 * If this channel will only request single transfers, set
	 * this down to ONE element.
	 */
	if (plchan->cd->single) {
		cctl |= (PL080_BSIZE_1 << PL080_CONTROL_SB_SIZE_SHIFT) |
			(PL080_BSIZE_1 << PL080_CONTROL_DB_SIZE_SHIFT);
	} else {
		while (i < ARRAY_SIZE(burst_sizes)) {
			if (burst_sizes[i].burstwords <= maxburst)
				break;
			i++;
		}
		cctl |= burst_sizes[i].reg;
	}

	/* Access the cell in privileged mode, non-bufferable, non-cacheable */
	cctl &= ~PL080_CONTROL_PROT_MASK;
	cctl |= PL080_CONTROL_PROT_SYS;

	/* Modify the default channel data to fit PrimeCell request */
	cd->cctl = cctl;
	cd->ccfg = ccfg;

	dev_dbg(&pl08x->adev->dev,
		"configured channel %s (%s) for %s, data width %d, "
		"maxburst %d words, LE, CCTL=%08x, CCFG=%08x\n",
		dma_chan_name(chan), plchan->name,
		(config->direction == DMA_FROM_DEVICE) ? "RX" : "TX",
		addr_width,
		maxburst,
		cctl, ccfg);
}

/*
 * Slave transactions callback to the slave device to allow
 * synchronization of slave DMA signals with the DMAC enable
 */
static void pl08x_issue_pending(struct dma_chan *chan)
{
	struct pl08x_dma_chan *plchan = to_pl08x_chan(chan);
	struct pl08x_driver_data *pl08x = plchan->host;
	unsigned long flags;

	spin_lock_irqsave(&plchan->lock, flags);
	/* Something is already active */
	if (plchan->at) {
			spin_unlock_irqrestore(&plchan->lock, flags);
			return;
	}

	/* Didn't get a physical channel so waiting for it ... */
	if (plchan->state == PL08X_CHAN_WAITING)
		return;

	/* Take the first element in the queue and execute it */
	if (!list_empty(&plchan->desc_list)) {
		struct pl08x_txd *next;

		next = list_first_entry(&plchan->desc_list,
					struct pl08x_txd,
					node);
		list_del(&next->node);
		plchan->at = next;
		plchan->state = PL08X_CHAN_RUNNING;

		/* Configure the physical channel for the active txd */
		pl08x_config_phychan_for_txd(plchan);
		pl08x_set_cregs(pl08x, plchan->phychan);
		pl08x_enable_phy_chan(pl08x, plchan->phychan);
	}

	spin_unlock_irqrestore(&plchan->lock, flags);
}

static int pl08x_prep_channel_resources(struct pl08x_dma_chan *plchan,
					struct pl08x_txd *txd)
{
	int num_llis;
	struct pl08x_driver_data *pl08x = plchan->host;
	int ret;

	num_llis = pl08x_fill_llis_for_desc(pl08x, txd);

	if (!num_llis)
		return -EINVAL;

	spin_lock_irqsave(&plchan->lock, plchan->lockflags);

	/*
	 * If this device is not using a circular buffer then
	 * queue this new descriptor for transfer.
	 * The descriptor for a circular buffer continues
	 * to be used until the channel is freed.
	 */
	if (txd->cd->circular_buffer)
		dev_err(&pl08x->adev->dev,
			"%s attempting to queue a circular buffer\n",
			__func__);
	else
		list_add_tail(&txd->node,
			      &plchan->desc_list);

	/*
	 * See if we already have a physical channel allocated,
	 * else this is the time to try to get one.
	 */
	ret = prep_phy_channel(plchan, txd);
	if (ret) {
		/*
		 * No physical channel available, we will
		 * stack up the memcpy channels until there is a channel
		 * available to handle it whereas slave transfers may
		 * have been denied due to platform channel muxing restrictions
		 * and since there is no guarantee that this will ever be
		 * resolved, and since the signal must be aquired AFTER
		 * aquiring the physical channel, we will let them be NACK:ed
		 * with -EBUSY here. The drivers can alway retry the prep()
		 * call if they are eager on doing this using DMA.
		 */
		if (plchan->slave) {
			pl08x_free_txd_list(pl08x, plchan);
			spin_unlock_irqrestore(&plchan->lock, plchan->lockflags);
			return -EBUSY;
		}
		/* Do this memcpy whenever there is a channel ready */
		plchan->state = PL08X_CHAN_WAITING;
		plchan->waiting = txd;
	} else
		/*
		 * Else we're all set, paused and ready to roll,
		 * status will switch to PL08X_CHAN_RUNNING when
		 * we call issue_pending(). If there is something
		 * running on the channel already we don't change
		 * its state.
		 */
		if (plchan->state == PL08X_CHAN_IDLE)
			plchan->state = PL08X_CHAN_PAUSED;

	/*
	 * Notice that we leave plchan->lock locked on purpose:
	 * it will be unlocked in the subsequent tx_submit()
	 * call. This is a consequence of the current API.
	 */

	return 0;
}

/*
 * Initialize a descriptor to be used by memcpy submit
 */
static struct dma_async_tx_descriptor *pl08x_prep_dma_memcpy(
		struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
		size_t len, unsigned long flags)
{
	struct pl08x_dma_chan *plchan = to_pl08x_chan(chan);
	struct pl08x_driver_data *pl08x = plchan->host;
	struct pl08x_txd *txd;
	int ret;

	txd = kzalloc(sizeof(struct pl08x_txd), GFP_NOWAIT);
	if (!txd) {
		dev_err(&pl08x->adev->dev,
			"%s no memory for descriptor\n", __func__);
		return NULL;
	}

	dma_async_tx_descriptor_init(&txd->tx, chan);
	txd->direction = DMA_NONE;
	txd->srcbus.addr = src;
	txd->dstbus.addr = dest;

	/* Set platform data for m2m */
	txd->cd = &pl08x->pd->memcpy_channel;
	/* Both to be incremented or the code will break */
	txd->cd->cctl |= PL080_CONTROL_SRC_INCR | PL080_CONTROL_DST_INCR;
	txd->tx.tx_submit = pl08x_tx_submit;
	txd->tx.callback = NULL;
	txd->tx.callback_param = NULL;
	txd->len = len;

	INIT_LIST_HEAD(&txd->node);
	ret = pl08x_prep_channel_resources(plchan, txd);
	if (ret)
		return NULL;
	/*
	 * NB: the channel lock is held at this point so tx_submit()
	 * must be called in direct succession.
	 */

	return &txd->tx;
}

struct dma_async_tx_descriptor *pl08x_prep_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_data_direction direction,
		unsigned long flags)
{
	struct pl08x_dma_chan *plchan = to_pl08x_chan(chan);
	struct pl08x_driver_data *pl08x = plchan->host;
	struct pl08x_txd *txd;
	int ret;

	/*
	 * Current implementation ASSUMES only one sg
	 */
	if (sg_len != 1) {
		dev_err(&pl08x->adev->dev, "%s prepared too long sglist\n",
			__func__);
		BUG();
	}

	dev_dbg(&pl08x->adev->dev, "%s prepare transaction of %d bytes from %s\n",
		__func__, sgl->length, plchan->name);

	txd = kzalloc(sizeof(struct pl08x_txd), GFP_NOWAIT);
	if (!txd) {
		dev_err(&pl08x->adev->dev, "%s no txd\n", __func__);
		return NULL;
	}

	dma_async_tx_descriptor_init(&txd->tx, chan);

	if (direction != plchan->runtime_direction)
		dev_err(&pl08x->adev->dev, "%s DMA setup does not match "
			"the direction configured for the PrimeCell\n",
			__func__);

	/*
	 * Set up addresses, the PrimeCell configured address
	 * will take precedence since this may configure the
	 * channel target address dynamically at runtime.
	 */
	txd->direction = direction;
	if (direction == DMA_TO_DEVICE) {
		txd->srcbus.addr = sgl->dma_address;
		if (plchan->runtime_addr)
			txd->dstbus.addr = plchan->runtime_addr;
		else
			txd->dstbus.addr = plchan->cd->addr;
	} else if (direction == DMA_FROM_DEVICE) {
		if (plchan->runtime_addr)
			txd->srcbus.addr = plchan->runtime_addr;
		else
			txd->srcbus.addr = plchan->cd->addr;
		txd->dstbus.addr = sgl->dma_address;
	} else {
		dev_err(&pl08x->adev->dev,
			"%s direction unsupported\n", __func__);
		return NULL;
	}
	txd->cd = plchan->cd;
	txd->tx.tx_submit = pl08x_tx_submit;
	txd->tx.callback = NULL;
	txd->tx.callback_param = NULL;
	txd->len = sgl->length;
	INIT_LIST_HEAD(&txd->node);

	ret = pl08x_prep_channel_resources(plchan, txd);
	if (ret)
		return NULL;
	/*
	 * NB: the channel lock is held at this point so tx_submit()
	 * must be called in direct succession.
	 */

	return &txd->tx;
}

static int pl08x_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
			 unsigned long arg)
{
	struct pl08x_dma_chan *plchan = to_pl08x_chan(chan);
	struct pl08x_driver_data *pl08x = plchan->host;
	unsigned long flags;
	int ret = 0;

	/* Controls applicable to inactive channels */
	if (cmd == DMA_SLAVE_CONFIG) {
		dma_set_runtime_config(chan,
				       (struct dma_slave_config *)
				       arg);
		return 0;
	}

	/*
	 * Anything succeeds on channels with no physical allocation and
	 * no queued transfers.
	 */
	spin_lock_irqsave(&plchan->lock, flags);
	if (!plchan->phychan && !plchan->at) {
		spin_unlock_irqrestore(&plchan->lock, flags);
		return 0;
	}

	switch (cmd) {
	case DMA_TERMINATE_ALL:
		plchan->state = PL08X_CHAN_IDLE;

		if (plchan->phychan) {
			pl08x_stop_phy_chan(plchan->phychan);

			/*
			 * Mark physical channel as free and free any slave
			 * signal
			 */
			if ((plchan->phychan->signal >= 0) &&
			    pl08x->pd->put_signal) {
				pl08x->pd->put_signal(plchan);
				plchan->phychan->signal = -1;
			}
			pl08x_put_phy_channel(pl08x, plchan->phychan);
			plchan->phychan = NULL;
		}
		/* Stop any pending tasklet */
		tasklet_disable(&plchan->tasklet);
		/* Dequeue jobs and free LLIs */
		if (plchan->at) {
			pl08x_free_txd(pl08x, plchan->at);
			plchan->at = NULL;
		}
		/* Dequeue jobs not yet fired as well */
		pl08x_free_txd_list(pl08x, plchan);
		break;
	case DMA_PAUSE:
		pl08x_pause_phy_chan(plchan->phychan);
		plchan->state = PL08X_CHAN_PAUSED;
		break;
	case DMA_RESUME:
		pl08x_resume_phy_chan(plchan->phychan);
		plchan->state = PL08X_CHAN_RUNNING;
		break;
	default:
		/* Unknown command */
		ret = -ENXIO;
		break;
	}

	spin_unlock_irqrestore(&plchan->lock, flags);

	return ret;
}

bool pl08x_filter_id(struct dma_chan *chan, void *chan_id)
{
	struct pl08x_dma_chan *plchan = to_pl08x_chan(chan);
	char *name = chan_id;

	/* Check that the channel is not taken! */
	if (!strcmp(plchan->name, name))
		return true;

	return false;
}

/*
 * Just check that the device is there and active
 * TODO: turn this bit on/off depending on the number of
 * physical channels actually used, if it is zero... well
 * shut it off. That will save some power. Cut the clock
 * at the same time.
 */
static void pl08x_ensure_on(struct pl08x_driver_data *pl08x)
{
	u32 val;

	val = readl(pl08x->base + PL080_CONFIG);
	val &= ~(PL080_CONFIG_M2_BE | PL080_CONFIG_M1_BE | PL080_CONFIG_ENABLE);
	/* We implictly clear bit 1 and that means little-endian mode */
	val |= PL080_CONFIG_ENABLE;
	writel(val, pl08x->base + PL080_CONFIG);
}

static void pl08x_tasklet(unsigned long data)
{
	struct pl08x_dma_chan *plchan = (struct pl08x_dma_chan *) data;
	struct pl08x_phy_chan *phychan = plchan->phychan;
	struct pl08x_driver_data *pl08x = plchan->host;

	if (!plchan)
		BUG();

	spin_lock(&plchan->lock);

	if (plchan->at) {
		dma_async_tx_callback callback =
			plchan->at->tx.callback;
		void *callback_param =
			plchan->at->tx.callback_param;

		/*
		 * Update last completed
		 */
		plchan->lc =
			(plchan->at->tx.cookie);

		/*
		 * Callback to signal completion
		 */
		if (callback)
			callback(callback_param);

		/*
		 * Device callbacks should NOT clear
		 * the current transaction on the channel
		 * Linus: sometimes they should?
		 */
		if (!plchan->at)
			BUG();

		/*
		 * Free the descriptor if it's not for a device
		 * using a circular buffer
		 */
		if (!plchan->at->cd->circular_buffer) {
			pl08x_free_txd(pl08x, plchan->at);
			plchan->at = NULL;
		}
		/*
		 * else descriptor for circular
		 * buffers only freed when
		 * client has disabled dma
		 */
	}
	/*
	 * If a new descriptor is queued, set it up
	 * plchan->at is NULL here
	 */
	if (!list_empty(&plchan->desc_list)) {
		struct pl08x_txd *next;

		next = list_first_entry(&plchan->desc_list,
					struct pl08x_txd,
					node);
		list_del(&next->node);
		plchan->at = next;
		/* Configure the physical channel for the next txd */
		pl08x_config_phychan_for_txd(plchan);
		pl08x_set_cregs(pl08x, plchan->phychan);
		pl08x_enable_phy_chan(pl08x, plchan->phychan);
	} else {
		struct pl08x_dma_chan *waiting = NULL;

		/*
		 * No more jobs, so free up the physical channel
		 * Free any allocated signal on slave transfers too
		 */
		if ((phychan->signal >= 0) && pl08x->pd->put_signal) {
			pl08x->pd->put_signal(plchan);
			phychan->signal = -1;
		}
		pl08x_put_phy_channel(pl08x, phychan);
		plchan->phychan = NULL;
		plchan->state = PL08X_CHAN_IDLE;

		/*
		 * And NOW before anyone else can grab that free:d
		 * up physical channel, see if there is some memcpy
		 * pending that seriously needs to start because of
		 * being stacked up while we were choking the
		 * physical channels with data.
		 */
		list_for_each_entry(waiting, &pl08x->memcpy.channels,
				    chan.device_node) {
		  if (waiting->state == PL08X_CHAN_WAITING &&
			    waiting->waiting != NULL) {
				int ret;

				/* This should REALLY not fail now */
				ret = prep_phy_channel(waiting,
						       waiting->waiting);
				BUG_ON(ret);
				waiting->state = PL08X_CHAN_RUNNING;
				waiting->waiting = NULL;
				pl08x_issue_pending(&waiting->chan);
				break;
			}
		}
	}

	spin_unlock(&plchan->lock);
}

static irqreturn_t pl08x_irq(int irq, void *dev)
{
	struct pl08x_driver_data *pl08x = dev;
	u32 mask = 0;
	u32 val;
	int i;

	val = readl(pl08x->base + PL080_ERR_STATUS);
	if (val) {
		/*
		 * An error interrupt (on one or more channels)
		 */
		dev_err(&pl08x->adev->dev,
			"%s error interrupt, register value 0x%08x\n",
				__func__, val);
		/*
		 * Simply clear ALL PL08X error interrupts,
		 * regardless of channel and cause
		 * FIXME: should be 0x00000003 on PL081 really.
		 */
		writel(0x000000FF, pl08x->base + PL080_ERR_CLEAR);
	}
	val = readl(pl08x->base + PL080_INT_STATUS);
	for (i = 0; i < pl08x->vd->channels; i++) {
		if ((1 << i) & val) {
			/* Locate physical channel */
			struct pl08x_phy_chan *phychan = &pl08x->phy_chans[i];
			struct pl08x_dma_chan *plchan = phychan->serving;

			/* Schedule tasklet on this channel */
			tasklet_schedule(&plchan->tasklet);

			mask |= (1 << i);
		}
	}
	/*
	 * Clear only the terminal interrupts on channels we processed
	 */
	writel(mask, pl08x->base + PL080_TC_CLEAR);

	return mask ? IRQ_HANDLED : IRQ_NONE;
}

/*
 * Initialise the DMAC memcpy/slave channels.
 * Make a local wrapper to hold required data
 */
static int pl08x_dma_init_virtual_channels(struct pl08x_driver_data *pl08x,
					   struct dma_device *dmadev,
					   unsigned int channels,
					   bool slave)
{
	struct pl08x_dma_chan *chan;
	int i;

	INIT_LIST_HEAD(&dmadev->channels);
	/*
	 * Register as many many memcpy as we have physical channels,
	 * we won't always be able to use all but the code will have
	 * to cope with that situation.
	 */
	for (i = 0; i < channels; i++) {
		chan = kzalloc(sizeof(struct pl08x_dma_chan), GFP_KERNEL);
		if (!chan) {
			dev_err(&pl08x->adev->dev,
				"%s no memory for channel\n", __func__);
			return -ENOMEM;
		}

		chan->host = pl08x;
		chan->state = PL08X_CHAN_IDLE;

		if (slave) {
			chan->slave = true;
			chan->name = pl08x->pd->slave_channels[i].bus_id;
			chan->cd = &pl08x->pd->slave_channels[i];
		} else {
			chan->cd = &pl08x->pd->memcpy_channel;
			chan->name = kasprintf(GFP_KERNEL, "memcpy%d", i);
			if (!chan->name) {
				kfree(chan);
				return -ENOMEM;
			}
		}
		dev_info(&pl08x->adev->dev,
			 "initialize virtual channel \"%s\"\n",
			 chan->name);

		chan->chan.device = dmadev;
		atomic_set(&chan->last_issued, 0);
		chan->lc = atomic_read(&chan->last_issued);

		spin_lock_init(&chan->lock);
		INIT_LIST_HEAD(&chan->desc_list);
		tasklet_init(&chan->tasklet, pl08x_tasklet,
			     (unsigned long) chan);

		list_add_tail(&chan->chan.device_node, &dmadev->channels);
	}
	dev_info(&pl08x->adev->dev, "initialized %d virtual %s channels\n",
		 i, slave ? "slave" : "memcpy");
	return i;
}

static void pl08x_free_virtual_channels(struct dma_device *dmadev)
{
	struct pl08x_dma_chan *chan = NULL;
	struct pl08x_dma_chan *next;

	list_for_each_entry_safe(chan,
				 next, &dmadev->channels, chan.device_node) {
		list_del(&chan->chan.device_node);
		kfree(chan);
	}
}

#ifdef CONFIG_DEBUG_FS
static const char *pl08x_state_str(enum pl08x_dma_chan_state state)
{
	switch (state) {
	case PL08X_CHAN_IDLE:
		return "idle";
	case PL08X_CHAN_RUNNING:
		return "running";
	case PL08X_CHAN_PAUSED:
		return "paused";
	case PL08X_CHAN_WAITING:
		return "waiting";
	default:
		break;
	}
	return "UNKNOWN STATE";
}

static int pl08x_debugfs_show(struct seq_file *s, void *data)
{
	struct pl08x_driver_data *pl08x = s->private;
	struct pl08x_dma_chan *chan;
	struct pl08x_phy_chan *ch;
	unsigned long flags;
	int i;

	seq_printf(s, "PL08x physical channels:\n");
	seq_printf(s, "CHANNEL:\tUSER:\n");
	seq_printf(s, "--------\t-----\n");
	for (i = 0; i < pl08x->vd->channels; i++) {
		struct pl08x_dma_chan *virt_chan;

		ch = &pl08x->phy_chans[i];

		spin_lock_irqsave(&ch->lock, flags);
		virt_chan = ch->serving;

		seq_printf(s, "%d\t\t%s\n",
			   ch->id, virt_chan ? virt_chan->name : "(none)");

		spin_unlock_irqrestore(&ch->lock, flags);
	}

	seq_printf(s, "\nPL08x virtual memcpy channels:\n");
	seq_printf(s, "CHANNEL:\tSTATE:\n");
	seq_printf(s, "--------\t------\n");
	list_for_each_entry(chan, &pl08x->memcpy.channels, chan.device_node) {
		seq_printf(s, "%s\t\t\%s\n", chan->name,
			   pl08x_state_str(chan->state));
	}

	seq_printf(s, "\nPL08x virtual slave channels:\n");
	seq_printf(s, "CHANNEL:\tSTATE:\n");
	seq_printf(s, "--------\t------\n");
	list_for_each_entry(chan, &pl08x->slave.channels, chan.device_node) {
		seq_printf(s, "%s\t\t\%s\n", chan->name,
			   pl08x_state_str(chan->state));
	}

	return 0;
}

static int pl08x_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, pl08x_debugfs_show, inode->i_private);
}

static const struct file_operations pl08x_debugfs_operations = {
	.open		= pl08x_debugfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void init_pl08x_debugfs(struct pl08x_driver_data *pl08x)
{
	/* Expose a simple debugfs interface to view all clocks */
	(void) debugfs_create_file(dev_name(&pl08x->adev->dev), S_IFREG | S_IRUGO,
				   NULL, pl08x,
				   &pl08x_debugfs_operations);
}

#else
static inline void init_pl08x_debugfs(struct pl08x_driver_data *pl08x)
{
}
#endif

static int pl08x_probe(struct amba_device *adev, struct amba_id *id)
{
	struct pl08x_driver_data *pl08x;
	struct vendor_data *vd = id->data;
	int ret = 0;
	int i;

	ret = amba_request_regions(adev, NULL);
	if (ret)
		return ret;

	/* Create the driver state holder */
	pl08x = kzalloc(sizeof(struct pl08x_driver_data), GFP_KERNEL);
	if (!pl08x) {
		ret = -ENOMEM;
		goto out_no_pl08x;
	}

	/* Initialize memcpy engine */
	dma_cap_set(DMA_MEMCPY, pl08x->memcpy.cap_mask);
	pl08x->memcpy.dev = &adev->dev;
	pl08x->memcpy.device_alloc_chan_resources = pl08x_alloc_chan_resources;
	pl08x->memcpy.device_free_chan_resources = pl08x_free_chan_resources;
	pl08x->memcpy.device_prep_dma_memcpy = pl08x_prep_dma_memcpy;
	pl08x->memcpy.device_prep_dma_interrupt = pl08x_prep_dma_interrupt;
	pl08x->memcpy.device_tx_status = pl08x_dma_tx_status;
	pl08x->memcpy.device_issue_pending = pl08x_issue_pending;
	pl08x->memcpy.device_control = pl08x_control;

	/* Initialize slave engine */
	dma_cap_set(DMA_SLAVE, pl08x->slave.cap_mask);
	pl08x->slave.dev = &adev->dev;
	pl08x->slave.device_alloc_chan_resources = pl08x_alloc_chan_resources;
	pl08x->slave.device_free_chan_resources = pl08x_free_chan_resources;
	pl08x->slave.device_prep_dma_interrupt = pl08x_prep_dma_interrupt;
	pl08x->slave.device_tx_status = pl08x_dma_tx_status;
	pl08x->slave.device_issue_pending = pl08x_issue_pending;
	pl08x->slave.device_prep_slave_sg = pl08x_prep_slave_sg;
	pl08x->slave.device_control = pl08x_control;

	/* Get the platform data */
	pl08x->pd = dev_get_platdata(&adev->dev);
	if (!pl08x->pd) {
		dev_err(&adev->dev, "no platform data supplied\n");
		goto out_no_platdata;
	}

	/* Assign useful pointers to the driver state */
	pl08x->adev = adev;
	pl08x->vd = vd;

	/* A DMA memory pool for LLIs, align on 1-byte boundary */
	pl08x->pool = dma_pool_create(DRIVER_NAME, &pl08x->adev->dev,
			PL08X_LLI_TSFR_SIZE, PL08X_ALIGN, 0);
	if (!pl08x->pool) {
		ret = -ENOMEM;
		goto out_no_lli_pool;
	}

	spin_lock_init(&pl08x->lock);

	pl08x->base = ioremap(adev->res.start, resource_size(&adev->res));
	if (!pl08x->base) {
		ret = -ENOMEM;
		goto out_no_ioremap;
	}

	/* Turn on the PL08x */
	pl08x_ensure_on(pl08x);

	/*
	 * Attach the interrupt handler
	 */
	writel(0x000000FF, pl08x->base + PL080_ERR_CLEAR);
	writel(0x000000FF, pl08x->base + PL080_TC_CLEAR);

	ret = request_irq(adev->irq[0], pl08x_irq, IRQF_DISABLED,
			  vd->name, pl08x);
	if (ret) {
		dev_err(&adev->dev, "%s failed to request interrupt %d\n",
			__func__, adev->irq[0]);
		goto out_no_irq;
	}

	/* Initialize physical channels */
	pl08x->phy_chans = kmalloc((vd->channels * sizeof(struct pl08x_phy_chan)),
			GFP_KERNEL);
	if (!pl08x->phy_chans) {
		dev_err(&adev->dev, "%s failed to allocate "
			"physical channel holders\n",
			__func__);
		goto out_no_phychans;
	}

	for (i = 0; i < vd->channels; i++) {
		struct pl08x_phy_chan *ch = &pl08x->phy_chans[i];

		ch->id = i;
		ch->base = pl08x->base + PL080_Cx_BASE(i);
		spin_lock_init(&ch->lock);
		ch->serving = NULL;
		ch->signal = -1;
		dev_info(&adev->dev,
			 "physical channel %d is %s\n", i,
			 pl08x_phy_channel_busy(ch) ? "BUSY" : "FREE");
	}

	/* Register as many memcpy channels as there are physical channels */
	ret = pl08x_dma_init_virtual_channels(pl08x, &pl08x->memcpy,
					      pl08x->vd->channels, false);
	if (ret <= 0) {
		dev_warn(&pl08x->adev->dev,
			 "%s failed to enumerate memcpy channels - %d\n",
			 __func__, ret);
		goto out_no_memcpy;
	}
	pl08x->memcpy.chancnt = ret;

	/* Register slave channels */
	ret = pl08x_dma_init_virtual_channels(pl08x, &pl08x->slave,
					      pl08x->pd->num_slave_channels,
					      true);
	if (ret <= 0) {
		dev_warn(&pl08x->adev->dev,
			"%s failed to enumerate slave channels - %d\n",
				__func__, ret);
		goto out_no_slave;
	}
	pl08x->slave.chancnt = ret;

	ret = dma_async_device_register(&pl08x->memcpy);
	if (ret) {
		dev_warn(&pl08x->adev->dev,
			"%s failed to register memcpy as an async device - %d\n",
			__func__, ret);
		goto out_no_memcpy_reg;
	}

	ret = dma_async_device_register(&pl08x->slave);
	if (ret) {
		dev_warn(&pl08x->adev->dev,
			"%s failed to register slave as an async device - %d\n",
			__func__, ret);
		goto out_no_slave_reg;
	}

	amba_set_drvdata(adev, pl08x);
	init_pl08x_debugfs(pl08x);
	dev_info(&pl08x->adev->dev, "ARM(R) %s DMA block initialized @%08x\n",
		vd->name, adev->res.start);
	return 0;

out_no_slave_reg:
	dma_async_device_unregister(&pl08x->memcpy);
out_no_memcpy_reg:
	pl08x_free_virtual_channels(&pl08x->slave);
out_no_slave:
	pl08x_free_virtual_channels(&pl08x->memcpy);
out_no_memcpy:
	kfree(pl08x->phy_chans);
out_no_phychans:
	free_irq(adev->irq[0], pl08x);
out_no_irq:
	iounmap(pl08x->base);
out_no_ioremap:
	dma_pool_destroy(pl08x->pool);
out_no_lli_pool:
out_no_platdata:
	kfree(pl08x);
out_no_pl08x:
	amba_release_regions(adev);
	return ret;
}

/* PL080 has 8 channels and the PL080 have just 2 */
static struct vendor_data vendor_pl080 = {
	.name = "PL080",
	.channels = 8,
	.dualmaster = true,
};

static struct vendor_data vendor_pl081 = {
	.name = "PL081",
	.channels = 2,
	.dualmaster = false,
};

static struct amba_id pl08x_ids[] = {
	/* PL080 */
	{
		.id	= 0x00041080,
		.mask	= 0x000fffff,
		.data	= &vendor_pl080,
	},
	/* PL081 */
	{
		.id	= 0x00041081,
		.mask	= 0x000fffff,
		.data	= &vendor_pl081,
	},
	/* Nomadik 8815 PL080 variant */
	{
		.id	= 0x00280880,
		.mask	= 0x00ffffff,
		.data	= &vendor_pl080,
	},
	{ 0, 0 },
};

static struct amba_driver pl08x_amba_driver = {
	.drv.name	= DRIVER_NAME,
	.id_table	= pl08x_ids,
	.probe		= pl08x_probe,
};

static int __init pl08x_init(void)
{
	int retval;
	retval = amba_driver_register(&pl08x_amba_driver);
	if (retval)
		printk(KERN_WARNING DRIVER_NAME
		       "failed to register as an amba device (%d)\n",
		       retval);
	return retval;
}
subsys_initcall(pl08x_init);
