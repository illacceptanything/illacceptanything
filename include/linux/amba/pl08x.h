/*
 * linux/amba/pl08x.h - ARM PrimeCell DMA Controller driver
 *
 * Copyright (C) 2005 ARM Ltd
 * Copyright (C) 2010 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * pl08x information required by platform code
 *
 * Please credit ARM.com
 * Documentation: ARM DDI 0196D
 *
 */

#ifndef AMBA_PL08X_H
#define AMBA_PL08X_H

/* We need sizes of structs from this header */
#include <linux/dmaengine.h>
#include <linux/interrupt.h>

/**
 * struct pl08x_channel_data - data structure to pass info between
 * platform and PL08x driver regarding channel configuration
 * @bus_id: name of this device channel, not just a device name since
 * devices may have more than one channel e.g. "foo_tx"
 * @min_signal: the minimum DMA signal number to be muxed in for this
 * channel (for platforms supporting muxed signals). If you have
 * static assignments, make sure this is set to the assigned signal
 * number, PL08x have 16 possible signals in number 0 thru 15 so
 * when these are not enough they often get muxed (in hardware)
 * disabling simultaneous use of the same channel for two devices.
 * @max_signal: the maximum DMA signal number to be muxed in for
 * the channel. Set to the same as min_signal for
 * devices with static assignments
 * @muxval: a number usually used to poke into some mux regiser to
 * mux in the signal to this channel
 * @cctl_opt: default options for the channel control register
 * @addr: source/target address in physical memory for this DMA channel,
 * can be the address of a FIFO register for burst requests for example.
 * This can be left undefined if the PrimeCell API is used for configuring
 * this.
 * @circular_buffer: whether the buffer passed in is circular and
 * shall simply be looped round round (like a record baby round
 * round round round)
 * @single: the device connected to this channel will request single
 * DMA transfers, not bursts. (Bursts are default.)
 */
struct pl08x_channel_data {
	char *bus_id;
	int min_signal;
	int max_signal;
	u32 muxval;
	u32 cctl;
	u32 ccfg;
	dma_addr_t addr;
	bool circular_buffer;
	bool single;
};

/**
 * Struct pl08x_bus_data - information of source or destination
 * busses for a transfer
 * @addr: current address
 * @maxwidth: the maximum width of a transfer on this bus
 * @buswidth: the width of this bus in bytes: 1, 2 or 4
 * @fill_bytes: bytes required to fill to the next bus memory
 * boundary
 */
struct pl08x_bus_data {
	dma_addr_t addr;
	u8 maxwidth;
	u8 buswidth;
	u32 fill_bytes;
};

/**
 * struct pl08x_phy_chan - holder for the physical channels
 * @id: physical index to this channel
 * @lock: a lock to use when altering an instance of this struct
 * @signal: the physical signal (aka channel) serving this
 * physical channel right now
 * @serving: the virtual channel currently being served by this
 * physical channel
 */
struct pl08x_phy_chan {
	unsigned int id;
	void __iomem *base;
	spinlock_t lock;
	int signal;
	struct pl08x_dma_chan *serving;
	u32 csrc;
	u32 cdst;
	u32 clli;
	u32 cctl;
	u32 ccfg;
};

/**
 * struct pl08x_txd - wrapper for struct dma_async_tx_descriptor
 * @llis_bus: DMA memory address (physical) start for the LLIs
 * @llis_va: virtual memory address start for the LLIs
 */
struct pl08x_txd {
	struct dma_async_tx_descriptor tx;
	struct list_head node;
	enum dma_data_direction	direction;
	struct pl08x_bus_data srcbus;
	struct pl08x_bus_data dstbus;
	int len;
	dma_addr_t llis_bus;
	void *llis_va;
	struct pl08x_channel_data *cd;
	bool active;
	/*
	 * Settings to be put into the physical channel when we
	 * trigger this txd
	 */
	u32 csrc;
	u32 cdst;
	u32 clli;
	u32 cctl;
};

/**
 * struct pl08x_dma_chan_state - holds the PL08x specific virtual
 * channel states
 * @PL08X_CHAN_IDLE: the channel is idle
 * @PL08X_CHAN_RUNNING: the channel has allocated a physical transport
 * channel and is running a transfer on it
 * @PL08X_CHAN_PAUSED: the channel has allocated a physical transport
 * channel, but the transfer is currently paused
 * @PL08X_CHAN_WAITING: the channel is waiting for a physical transport
 * channel to become available (only pertains to memcpy channels)
 */
enum pl08x_dma_chan_state {
	PL08X_CHAN_IDLE,
	PL08X_CHAN_RUNNING,
	PL08X_CHAN_PAUSED,
	PL08X_CHAN_WAITING,
};

/**
 * struct pl08x_dma_chan - this structure wraps a DMA ENGINE channel
 * @chan: wrappped abstract channel
 * @phychan: the physical channel utilized by this channel, if there is one
 * @tasklet: tasklet scheduled by the IRQ to handle actual work etc
 * @name: name of channel
 * @cd: channel platform data
 * @runtime_addr: address for RX/TX according to the runtime config
 * @runtime_direction: current direction of this channel according to
 * runtime config
 * @lc: last completed transaction on this channel
 * @desc_list: queued transactions pending on this channel
 * @at: active transaction on this channel
 * @lockflags: sometimes we let a lock last between two function calls,
 * especially prep/submit, and then we need to store the IRQ flags
 * in the channel state, here
 * @lock: a lock for this channel data
 * @host: a pointer to the host (internal use)
 * @state: whether the channel is idle, paused, running etc
 * @slave: whether this channel is a device (slave) or for memcpy
 * @waiting: a TX descriptor on this channel which is waiting for
 * a physical channel to become available
 */
struct pl08x_dma_chan {
	struct dma_chan chan;
	struct pl08x_phy_chan *phychan;
	struct tasklet_struct tasklet;
	char *name;
	struct pl08x_channel_data *cd;
	dma_addr_t runtime_addr;
	enum dma_data_direction	runtime_direction;
	atomic_t last_issued;
	dma_cookie_t lc;
	struct list_head desc_list;
	struct pl08x_txd *at;
	unsigned long lockflags;
	spinlock_t lock;
	void *host;
	enum pl08x_dma_chan_state state;
	bool slave;
	struct pl08x_txd *waiting;
};

/**
 * struct pl08x_platform_data - the platform configuration for the
 * PL08x PrimeCells.
 * @slave_channels: the channels defined for the different devices on the
 * platform, all inclusive, including multiplexed channels. The available
 * physical channels will be multiplexed around these signals as they
 * are requested, just enumerate all possible channels.
 * @get_signal: request a physical signal to be used for a DMA
 * transfer immediately: if there is some multiplexing or similar blocking
 * the use of the channel the transfer can be denied by returning
 * less than zero, else it returns the allocated signal number
 * @put_signal: indicate to the platform that this physical signal is not
 * running any DMA transfer and multiplexing can be recycled
 * @bus_bit_lli: Bit[0] of the address indicated which AHB bus master the
 * LLI addresses are on 0/1 Master 1/2.
 */
struct pl08x_platform_data {
	struct pl08x_channel_data *slave_channels;
	unsigned int num_slave_channels;
	struct pl08x_channel_data memcpy_channel;
	int (*get_signal)(struct pl08x_dma_chan *);
	void (*put_signal)(struct pl08x_dma_chan *);
};

#ifdef CONFIG_AMBA_PL08X
bool pl08x_filter_id(struct dma_chan *chan, void *chan_id);
#else
static inline bool pl08x_filter_id(struct dma_chan *chan, void *chan_id)
{
	return false;
}
#endif

#endif	/* AMBA_PL08X_H */
