/*
 *	Handle incoming frames
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/netfilter_bridge.h>
#include <linux/neighbour.h>
#include <net/arp.h>
#include <linux/export.h>
#include <linux/rculist.h>
#include "br_private.h"

/* Hook for brouter */
br_should_route_hook_t __rcu *br_should_route_hook __read_mostly;
EXPORT_SYMBOL(br_should_route_hook);

static int br_pass_frame_up(struct sk_buff *skb)
{
	struct net_device *indev, *brdev = BR_INPUT_SKB_CB(skb)->brdev;
	struct net_bridge *br = netdev_priv(brdev);
	struct pcpu_sw_netstats *brstats = this_cpu_ptr(br->stats);
	struct net_port_vlans *pv;

	u64_stats_update_begin(&brstats->syncp);
	brstats->rx_packets++;
	brstats->rx_bytes += skb->len;
	u64_stats_update_end(&brstats->syncp);

	/* Bridge is just like any other port.  Make sure the
	 * packet is allowed except in promisc modue when someone
	 * may be running packet capture.
	 */
	pv = br_get_vlan_info(br);
	if (!(brdev->flags & IFF_PROMISC) &&
	    !br_allowed_egress(br, pv, skb)) {
		kfree_skb(skb);
		return NET_RX_DROP;
	}

	indev = skb->dev;
	skb->dev = brdev;
	skb = br_handle_vlan(br, pv, skb);
	if (!skb)
		return NET_RX_DROP;

	return NF_HOOK(NFPROTO_BRIDGE, NF_BR_LOCAL_IN, skb, indev, NULL,
		       netif_receive_skb);
}

static void br_do_proxy_arp(struct sk_buff *skb, struct net_bridge *br,
			    u16 vid)
{
	struct net_device *dev = br->dev;
	struct neighbour *n;
	struct arphdr *parp;
	u8 *arpptr, *sha;
	__be32 sip, tip;

	if (dev->flags & IFF_NOARP)
		return;

	if (!pskb_may_pull(skb, arp_hdr_len(dev))) {
		dev->stats.tx_dropped++;
		return;
	}
	parp = arp_hdr(skb);

	if (parp->ar_pro != htons(ETH_P_IP) ||
	    parp->ar_op != htons(ARPOP_REQUEST) ||
	    parp->ar_hln != dev->addr_len ||
	    parp->ar_pln != 4)
		return;

	arpptr = (u8 *)parp + sizeof(struct arphdr);
	sha = arpptr;
	arpptr += dev->addr_len;	/* sha */
	memcpy(&sip, arpptr, sizeof(sip));
	arpptr += sizeof(sip);
	arpptr += dev->addr_len;	/* tha */
	memcpy(&tip, arpptr, sizeof(tip));

	if (ipv4_is_loopback(tip) ||
	    ipv4_is_multicast(tip))
		return;

	n = neigh_lookup(&arp_tbl, &tip, dev);
	if (n) {
		struct net_bridge_fdb_entry *f;

		if (!(n->nud_state & NUD_VALID)) {
			neigh_release(n);
			return;
		}

		f = __br_fdb_get(br, n->ha, vid);
		if (f)
			arp_send(ARPOP_REPLY, ETH_P_ARP, sip, skb->dev, tip,
				 sha, n->ha, sha);

		neigh_release(n);
	}
}

/* note: already called with rcu_read_lock */
int br_handle_frame_finish(struct sk_buff *skb)
{
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	struct net_bridge_port *p = br_port_get_rcu(skb->dev);
	struct net_bridge *br;
	struct net_bridge_fdb_entry *dst;
	struct net_bridge_mdb_entry *mdst;
	struct sk_buff *skb2;
	bool unicast = true;
	u16 vid = 0;

	if (!p || p->state == BR_STATE_DISABLED)
		goto drop;

	if (!br_allowed_ingress(p->br, nbp_get_vlan_info(p), skb, &vid))
		goto out;

	/* insert into forwarding database after filtering to avoid spoofing */
	br = p->br;
	if (p->flags & BR_LEARNING)
		br_fdb_update(br, p, eth_hdr(skb)->h_source, vid, false);

	if (!is_broadcast_ether_addr(dest) && is_multicast_ether_addr(dest) &&
	    br_multicast_rcv(br, p, skb, vid))
		goto drop;

	if (p->state == BR_STATE_LEARNING)
		goto drop;

	BR_INPUT_SKB_CB(skb)->brdev = br->dev;

	/* The packet skb2 goes to the local host (NULL to skip). */
	skb2 = NULL;

	if (br->dev->flags & IFF_PROMISC)
		skb2 = skb;

	dst = NULL;

	if (is_broadcast_ether_addr(dest)) {
		if (IS_ENABLED(CONFIG_INET) &&
		    p->flags & BR_PROXYARP &&
		    skb->protocol == htons(ETH_P_ARP))
			br_do_proxy_arp(skb, br, vid);

		skb2 = skb;
		unicast = false;
	} else if (is_multicast_ether_addr(dest)) {
		mdst = br_mdb_get(br, skb, vid);
		if ((mdst || BR_INPUT_SKB_CB_MROUTERS_ONLY(skb)) &&
		    br_multicast_querier_exists(br, eth_hdr(skb))) {
			if ((mdst && mdst->mglist) ||
			    br_multicast_is_router(br))
				skb2 = skb;
			br_multicast_forward(mdst, skb, skb2);
			skb = NULL;
			if (!skb2)
				goto out;
		} else
			skb2 = skb;

		unicast = false;
		br->dev->stats.multicast++;
	} else if ((dst = __br_fdb_get(br, dest, vid)) &&
			dst->is_local) {
		skb2 = skb;
		/* Do not forward the packet since it's local. */
		skb = NULL;
	}

	if (skb) {
		if (dst) {
			dst->used = jiffies;
			br_forward(dst->dst, skb, skb2);
		} else
			br_flood_forward(br, skb, skb2, unicast);
	}

	if (skb2)
		return br_pass_frame_up(skb2);

out:
	return 0;
drop:
	kfree_skb(skb);
	goto out;
}
EXPORT_SYMBOL_GPL(br_handle_frame_finish);

/* note: already called with rcu_read_lock */
static int br_handle_local_finish(struct sk_buff *skb)
{
	struct net_bridge_port *p = br_port_get_rcu(skb->dev);
	u16 vid = 0;

	/* check if vlan is allowed, to avoid spoofing */
	if (p->flags & BR_LEARNING && br_should_learn(p, skb, &vid))
		br_fdb_update(p->br, p, eth_hdr(skb)->h_source, vid, false);
	return 0;	 /* process further */
}

/*
 * Return NULL if skb is handled
 * note: already called with rcu_read_lock
 */
rx_handler_result_t br_handle_frame(struct sk_buff **pskb)
{
	struct net_bridge_port *p;
	struct sk_buff *skb = *pskb;
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	br_should_route_hook_t *rhook;

	if (unlikely(skb->pkt_type == PACKET_LOOPBACK))
		return RX_HANDLER_PASS;

	if (!is_valid_ether_addr(eth_hdr(skb)->h_source))
		goto drop;

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		return RX_HANDLER_CONSUMED;

	p = br_port_get_rcu(skb->dev);

	if (unlikely(is_link_local_ether_addr(dest))) {
		u16 fwd_mask = p->br->group_fwd_mask_required;

		/*
		 * See IEEE 802.1D Table 7-10 Reserved addresses
		 *
		 * Assignment		 		Value
		 * Bridge Group Address		01-80-C2-00-00-00
		 * (MAC Control) 802.3		01-80-C2-00-00-01
		 * (Link Aggregation) 802.3	01-80-C2-00-00-02
		 * 802.1X PAE address		01-80-C2-00-00-03
		 *
		 * 802.1AB LLDP 		01-80-C2-00-00-0E
		 *
		 * Others reserved for future standardization
		 */
		switch (dest[5]) {
		case 0x00:	/* Bridge Group Address */
			/* If STP is turned off,
			   then must forward to keep loop detection */
			if (p->br->stp_enabled == BR_NO_STP ||
			    fwd_mask & (1u << dest[5]))
				goto forward;
			break;

		case 0x01:	/* IEEE MAC (Pause) */
			goto drop;

		default:
			/* Allow selective forwarding for most other protocols */
			fwd_mask |= p->br->group_fwd_mask;
			if (fwd_mask & (1u << dest[5]))
				goto forward;
		}

		/* Deliver packet to local host only */
		if (NF_HOOK(NFPROTO_BRIDGE, NF_BR_LOCAL_IN, skb, skb->dev,
			    NULL, br_handle_local_finish)) {
			return RX_HANDLER_CONSUMED; /* consumed by filter */
		} else {
			*pskb = skb;
			return RX_HANDLER_PASS;	/* continue processing */
		}
	}

forward:
	switch (p->state) {
	case BR_STATE_FORWARDING:
		rhook = rcu_dereference(br_should_route_hook);
		if (rhook) {
			if ((*rhook)(skb)) {
				*pskb = skb;
				return RX_HANDLER_PASS;
			}
			dest = eth_hdr(skb)->h_dest;
		}
		/* fall through */
	case BR_STATE_LEARNING:
		if (ether_addr_equal(p->br->dev->dev_addr, dest))
			skb->pkt_type = PACKET_HOST;

		NF_HOOK(NFPROTO_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
			br_handle_frame_finish);
		break;
	default:
drop:
		kfree_skb(skb);
	}
	return RX_HANDLER_CONSUMED;
}
