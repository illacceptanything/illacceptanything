/*
 * Copyright (C) 2010 B.A.T.M.A.N. contributors:
 *
 * Andreas Langer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 *
 */

#include "main.h"
#include "unicast.h"
#include "send.h"
#include "soft-interface.h"
#include "hash.h"
#include "translation-table.h"
#include "routing.h"
#include "hard-interface.h"


struct sk_buff *merge_frag_packet(struct list_head *head,
				  struct frag_packet_list_entry *tfp,
				  struct sk_buff *skb)
{
	struct unicast_frag_packet *up =
		(struct unicast_frag_packet *)skb->data;
	struct sk_buff *tmp_skb;

	/* set skb to the first part and tmp_skb to the second part */
	if (up->flags & UNI_FRAG_HEAD) {
		tmp_skb = tfp->skb;
	} else {
		tmp_skb = skb;
		skb = tfp->skb;
	}

	skb_pull(tmp_skb, sizeof(struct unicast_frag_packet));
	if (pskb_expand_head(skb, 0, tmp_skb->len, GFP_ATOMIC) < 0) {
		/* free buffered skb, skb will be freed later */
		kfree_skb(tfp->skb);
		return NULL;
	}

	/* move free entry to end */
	tfp->skb = NULL;
	tfp->seqno = 0;
	list_move_tail(&tfp->list, head);

	memcpy(skb_put(skb, tmp_skb->len), tmp_skb->data, tmp_skb->len);
	kfree_skb(tmp_skb);
	return skb;
}

void create_frag_entry(struct list_head *head, struct sk_buff *skb)
{
	struct frag_packet_list_entry *tfp;
	struct unicast_frag_packet *up =
		(struct unicast_frag_packet *)skb->data;

	/* free and oldest packets stand at the end */
	tfp = list_entry((head)->prev, typeof(*tfp), list);
	kfree_skb(tfp->skb);

	tfp->seqno = ntohs(up->seqno);
	tfp->skb = skb;
	list_move(&tfp->list, head);
	return;
}

int create_frag_buffer(struct list_head *head)
{
	int i;
	struct frag_packet_list_entry *tfp;

	for (i = 0; i < FRAG_BUFFER_SIZE; i++) {
		tfp = kmalloc(sizeof(struct frag_packet_list_entry),
			GFP_ATOMIC);
		if (!tfp) {
			frag_list_free(head);
			return -ENOMEM;
		}
		tfp->skb = NULL;
		tfp->seqno = 0;
		INIT_LIST_HEAD(&tfp->list);
		list_add(&tfp->list, head);
	}

	return 0;
}

struct frag_packet_list_entry *search_frag_packet(struct list_head *head,
						 struct unicast_frag_packet *up)
{
	struct frag_packet_list_entry *tfp;
	struct unicast_frag_packet *tmp_up = NULL;
	uint16_t search_seqno;

	if (up->flags & UNI_FRAG_HEAD)
		search_seqno = ntohs(up->seqno)+1;
	else
		search_seqno = ntohs(up->seqno)-1;

	list_for_each_entry(tfp, head, list) {

		if (!tfp->skb)
			continue;

		if (tfp->seqno == ntohs(up->seqno))
			goto mov_tail;

		tmp_up = (struct unicast_frag_packet *)tfp->skb->data;

		if (tfp->seqno == search_seqno) {

			if ((tmp_up->flags & UNI_FRAG_HEAD) !=
			    (up->flags & UNI_FRAG_HEAD))
				return tfp;
			else
				goto mov_tail;
		}
	}
	return NULL;

mov_tail:
	list_move_tail(&tfp->list, head);
	return NULL;
}

void frag_list_free(struct list_head *head)
{
	struct frag_packet_list_entry *pf, *tmp_pf;

	if (!list_empty(head)) {

		list_for_each_entry_safe(pf, tmp_pf, head, list) {
			kfree_skb(pf->skb);
			list_del(&pf->list);
			kfree(pf);
		}
	}
	return;
}

static int unicast_send_frag_skb(struct sk_buff *skb, struct bat_priv *bat_priv,
			  struct batman_if *batman_if, uint8_t dstaddr[],
			  struct orig_node *orig_node)
{
	struct unicast_frag_packet *ucast_frag1, *ucast_frag2;
	int hdr_len = sizeof(struct unicast_frag_packet);
	struct sk_buff *frag_skb;
	int data_len = skb->len;

	if (!bat_priv->primary_if)
		goto dropped;

	frag_skb = dev_alloc_skb(data_len - (data_len / 2) + hdr_len);
	skb_split(skb, frag_skb, data_len / 2);

	if (my_skb_head_push(frag_skb, hdr_len) < 0 ||
	    my_skb_head_push(skb, hdr_len) < 0)
		goto drop_frag;

	ucast_frag1 = (struct unicast_frag_packet *)skb->data;
	ucast_frag2 = (struct unicast_frag_packet *)frag_skb->data;

	ucast_frag1->version = COMPAT_VERSION;
	ucast_frag1->packet_type = BAT_UNICAST_FRAG;
	ucast_frag1->ttl = TTL;
	memcpy(ucast_frag1->orig,
	       bat_priv->primary_if->net_dev->dev_addr, ETH_ALEN);
	memcpy(ucast_frag1->dest, orig_node->orig, ETH_ALEN);

	memcpy(ucast_frag2, ucast_frag1, sizeof(struct unicast_frag_packet));

	ucast_frag1->flags |= UNI_FRAG_HEAD;
	ucast_frag2->flags &= ~UNI_FRAG_HEAD;

	ucast_frag1->seqno = htons((uint16_t)atomic_inc_return(
						&batman_if->frag_seqno));

	ucast_frag2->seqno = htons((uint16_t)atomic_inc_return(
						&batman_if->frag_seqno));

	send_skb_packet(skb, batman_if, dstaddr);
	send_skb_packet(frag_skb, batman_if, dstaddr);
	return 0;

drop_frag:
	kfree_skb(frag_skb);
dropped:
	kfree_skb(skb);
	return 1;
}

int unicast_send_skb(struct sk_buff *skb, struct bat_priv *bat_priv)
{
	struct ethhdr *ethhdr = (struct ethhdr *)skb->data;
	struct unicast_packet *unicast_packet;
	struct orig_node *orig_node;
	struct batman_if *batman_if;
	struct neigh_node *router;
	int data_len = skb->len;
	uint8_t dstaddr[6];
	unsigned long flags;

	spin_lock_irqsave(&bat_priv->orig_hash_lock, flags);

	/* get routing information */
	orig_node = ((struct orig_node *)hash_find(bat_priv->orig_hash,
						   ethhdr->h_dest));

	/* check for hna host */
	if (!orig_node)
		orig_node = transtable_search(bat_priv, ethhdr->h_dest);

	router = find_router(bat_priv, orig_node, NULL);

	if (!router)
		goto unlock;

	/* don't lock while sending the packets ... we therefore
		* copy the required data before sending */

	batman_if = router->if_incoming;
	memcpy(dstaddr, router->addr, ETH_ALEN);

	spin_unlock_irqrestore(&bat_priv->orig_hash_lock, flags);

	if (batman_if->if_status != IF_ACTIVE)
		goto dropped;

	if (atomic_read(&bat_priv->frag_enabled) &&
	    data_len + sizeof(struct unicast_packet) > batman_if->net_dev->mtu)
		return unicast_send_frag_skb(skb, bat_priv, batman_if,
					     dstaddr, orig_node);

	if (my_skb_head_push(skb, sizeof(struct unicast_packet)) < 0)
		goto dropped;

	unicast_packet = (struct unicast_packet *)skb->data;

	unicast_packet->version = COMPAT_VERSION;
	/* batman packet type: unicast */
	unicast_packet->packet_type = BAT_UNICAST;
	/* set unicast ttl */
	unicast_packet->ttl = TTL;
	/* copy the destination for faster routing */
	memcpy(unicast_packet->dest, orig_node->orig, ETH_ALEN);

	send_skb_packet(skb, batman_if, dstaddr);
	return 0;

unlock:
	spin_unlock_irqrestore(&bat_priv->orig_hash_lock, flags);
dropped:
	kfree_skb(skb);
	return 1;
}
