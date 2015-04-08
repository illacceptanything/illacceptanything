/*
 * Linux network driver for Brocade Converged Network Adapter.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (GPL) Version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
/*
 * Copyright (c) 2005-2010 Brocade Communications Systems, Inc.
 * All rights reserved
 * www.brocade.com
 */
#ifndef __BNAD_H__
#define __BNAD_H__

#include <linux/rtnetlink.h>
#include <linux/workqueue.h>
#include <linux/ipv6.h>
#include <linux/etherdevice.h>
#include <linux/mutex.h>
#include <linux/firmware.h>

/* Fix for IA64 */
#include <asm/checksum.h>
#include <net/ip6_checksum.h>

#include <net/ip.h>
#include <net/tcp.h>

#include "bna.h"

#define BNAD_TXQ_DEPTH		2048
#define BNAD_RXQ_DEPTH		2048

#define BNAD_MAX_TXS		1
#define BNAD_MAX_TXQ_PER_TX	8	/* 8 priority queues */
#define BNAD_TXQ_NUM		1

#define BNAD_MAX_RXS		1
#define BNAD_MAX_RXPS_PER_RX	16

/*
 * Control structure pointed to ccb->ctrl, which
 * determines the NAPI / LRO behavior CCB
 * There is 1:1 corres. between ccb & ctrl
 */
struct bnad_rx_ctrl {
	struct bna_ccb *ccb;
	struct napi_struct	napi;
};

#define BNAD_RXMODE_PROMISC_DEFAULT	BNA_RXMODE_PROMISC

#define BNAD_GET_TX_ID(_skb)	(0)

/*
 * GLOBAL #defines (CONSTANTS)
 */
#define BNAD_NAME			"bna"
#define BNAD_NAME_LEN			64

#define BNAD_VERSION			"2.3.2.0"

#define BNAD_MAILBOX_MSIX_VECTORS	1

#define BNAD_STATS_TIMER_FREQ		1000 	/* in msecs */
#define BNAD_DIM_TIMER_FREQ		1000 	/* in msecs */

#define BNAD_MAX_Q_DEPTH		0x10000
#define BNAD_MIN_Q_DEPTH		0x200

#define BNAD_JUMBO_MTU			9000

#define BNAD_NETIF_WAKE_THRESHOLD	8

#define BNAD_RXQ_REFILL_THRESHOLD_SHIFT	3

/* Bit positions for tcb->flags */
#define BNAD_TXQ_FREE_SENT		0

/* Bit positions for rcb->flags */
#define BNAD_RXQ_REFILL			0
#define BNAD_RXQ_STARTED		1

/*
 * DATA STRUCTURES
 */

/* enums */
enum bnad_intr_source {
	BNAD_INTR_TX		= 1,
	BNAD_INTR_RX		= 2
};

enum bnad_link_state {
	BNAD_LS_DOWN		= 0,
	BNAD_LS_UP 		= 1
};

struct bnad_completion {
	struct completion 	ioc_comp;
	struct completion 	ucast_comp;
	struct completion	mcast_comp;
	struct completion	tx_comp;
	struct completion	rx_comp;
	struct completion	stats_comp;
	struct completion	port_comp;

	u8			ioc_comp_status;
	u8			ucast_comp_status;
	u8			mcast_comp_status;
	u8			tx_comp_status;
	u8			rx_comp_status;
	u8			stats_comp_status;
	u8			port_comp_status;
};

/* Tx Rx Control Stats */
struct bnad_drv_stats {
	u64 		netif_queue_stop;
	u64		netif_queue_wakeup;
	u64		tso4;
	u64		tso6;
	u64		tso_err;
	u64		tcpcsum_offload;
	u64		udpcsum_offload;
	u64		csum_help;
	u64		csum_help_err;

	u64		hw_stats_updates;
	u64		netif_rx_schedule;
	u64		netif_rx_complete;
	u64		netif_rx_dropped;

	u64		link_toggle;
	u64		cee_up;

	u64		rxp_info_alloc_failed;
	u64		mbox_intr_disabled;
	u64		mbox_intr_enabled;
	u64		tx_unmap_q_alloc_failed;
	u64		rx_unmap_q_alloc_failed;

	u64		rxbuf_alloc_failed;
};

/* Complete driver stats */
struct bnad_stats {
	struct bnad_drv_stats drv_stats;
	struct bna_stats *bna_stats;
};

/* Tx / Rx Resources */
struct bnad_tx_res_info {
	struct bna_res_info res_info[BNA_TX_RES_T_MAX];
};

struct bnad_rx_res_info {
	struct bna_res_info res_info[BNA_RX_RES_T_MAX];
};

struct bnad_tx_info {
	struct bna_tx *tx; /* 1:1 between tx_info & tx */
	struct bna_tcb *tcb[BNAD_MAX_TXQ_PER_TX];
} ____cacheline_aligned;

struct bnad_rx_info {
	struct bna_rx *rx; /* 1:1 between rx_info & rx */

	struct bnad_rx_ctrl rx_ctrl[BNAD_MAX_RXPS_PER_RX];
} ____cacheline_aligned;

/* Unmap queues for Tx / Rx cleanup */
struct bnad_skb_unmap {
	struct sk_buff		*skb;
	DECLARE_PCI_UNMAP_ADDR(dma_addr)
};

struct bnad_unmap_q {
	u32		producer_index;
	u32		consumer_index;
	u32 		q_depth;
	/* This should be the last one */
	struct bnad_skb_unmap unmap_array[1];
};

/* Bit mask values for bnad->cfg_flags */
#define	BNAD_CF_DIM_ENABLED		0x01	/* DIM */
#define	BNAD_CF_PROMISC			0x02
#define BNAD_CF_ALLMULTI		0x04
#define	BNAD_CF_MSIX			0x08	/* If in MSIx mode */

/* Defines for run_flags bit-mask */
/* Set, tested & cleared using xxx_bit() functions */
/* Values indicated bit positions */
#define	BNAD_RF_CEE_RUNNING		1
#define BNAD_RF_HW_ERROR 		2
#define BNAD_RF_MBOX_IRQ_DISABLED	3
#define BNAD_RF_TX_STARTED		4
#define BNAD_RF_RX_STARTED		5
#define BNAD_RF_DIM_TIMER_RUNNING	6
#define BNAD_RF_STATS_TIMER_RUNNING	7

struct bnad {
	struct net_device 	*netdev;

	/* Data path */
	struct bnad_tx_info tx_info[BNAD_MAX_TXS];
	struct bnad_rx_info rx_info[BNAD_MAX_RXS];

	struct vlan_group	*vlan_grp;
	/*
	 * These q numbers are global only because
	 * they are used to calculate MSIx vectors.
	 * Actually the exact # of queues are per Tx/Rx
	 * object.
	 */
	u32		num_tx;
	u32		num_rx;
	u32		num_txq_per_tx;
	u32		num_rxp_per_rx;

	u32		txq_depth;
	u32		rxq_depth;

	u8			tx_coalescing_timeo;
	u8			rx_coalescing_timeo;

	struct bna_rx_config rx_config[BNAD_MAX_RXS];
	struct bna_tx_config tx_config[BNAD_MAX_TXS];

	u32		rx_csum;

	void __iomem		*bar0;	/* BAR0 address */

	struct bna bna;

	u32		cfg_flags;
	unsigned long		run_flags;

	struct pci_dev 		*pcidev;
	u64		mmio_start;
	u64		mmio_len;

	u32		msix_num;
	struct msix_entry	*msix_table;

	struct mutex		conf_mutex;
	spinlock_t		bna_lock ____cacheline_aligned;

	/* Timers */
	struct timer_list	ioc_timer;
	struct timer_list	dim_timer;
	struct timer_list	stats_timer;

	/* Control path resources, memory & irq */
	struct bna_res_info res_info[BNA_RES_T_MAX];
	struct bnad_tx_res_info tx_res_info[BNAD_MAX_TXS];
	struct bnad_rx_res_info rx_res_info[BNAD_MAX_RXS];

	struct bnad_completion bnad_completions;

	/* Burnt in MAC address */
	mac_t			perm_addr;

	struct tasklet_struct	tx_free_tasklet;

	/* Statistics */
	struct bnad_stats stats;

	struct bnad_diag *diag;

	char			adapter_name[BNAD_NAME_LEN];
	char 			port_name[BNAD_NAME_LEN];
	char			mbox_irq_name[BNAD_NAME_LEN];
};

/*
 * EXTERN VARIABLES
 */
extern struct firmware *bfi_fw;
extern u32 		bnad_rxqs_per_cq;

/*
 * EXTERN PROTOTYPES
 */
extern u32 *cna_get_firmware_buf(struct pci_dev *pdev);
/* Netdev entry point prototypes */
extern void bnad_set_ethtool_ops(struct net_device *netdev);

/* Configuration & setup */
extern void bnad_tx_coalescing_timeo_set(struct bnad *bnad);
extern void bnad_rx_coalescing_timeo_set(struct bnad *bnad);

extern int bnad_setup_rx(struct bnad *bnad, uint rx_id);
extern int bnad_setup_tx(struct bnad *bnad, uint tx_id);
extern void bnad_cleanup_tx(struct bnad *bnad, uint tx_id);
extern void bnad_cleanup_rx(struct bnad *bnad, uint rx_id);

/* Timer start/stop protos */
extern void bnad_dim_timer_start(struct bnad *bnad);

/* Statistics */
extern void bnad_netdev_qstats_fill(struct bnad *bnad, struct rtnl_link_stats64 *stats);
extern void bnad_netdev_hwstats_fill(struct bnad *bnad, struct rtnl_link_stats64 *stats);

/**
 * MACROS
 */
/* To set & get the stats counters */
#define BNAD_UPDATE_CTR(_bnad, _ctr)				\
				(((_bnad)->stats.drv_stats._ctr)++)

#define BNAD_GET_CTR(_bnad, _ctr) ((_bnad)->stats.drv_stats._ctr)

#define bnad_enable_rx_irq_unsafe(_ccb)			\
{							\
	bna_ib_coalescing_timer_set((_ccb)->i_dbell,	\
		(_ccb)->rx_coalescing_timeo);		\
	bna_ib_ack((_ccb)->i_dbell, 0);			\
}

#define bnad_dim_timer_running(_bnad)				\
	(((_bnad)->cfg_flags & BNAD_CF_DIM_ENABLED) && 		\
	(test_bit(BNAD_RF_DIM_TIMER_RUNNING, &((_bnad)->run_flags))))

#endif /* __BNAD_H__ */
