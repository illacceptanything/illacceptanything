/*******************************************************************************
 * This file contains the iSCSI Login Thread and Thread Queue functions.
 *
 * (c) Copyright 2007-2013 Datera, Inc.
 *
 * Author: Nicholas A. Bellinger <nab@linux-iscsi.org>
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
 ******************************************************************************/

#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/bitmap.h>

#include <target/iscsi/iscsi_target_core.h>
#include "iscsi_target_tq.h"
#include "iscsi_target.h"

static LIST_HEAD(inactive_ts_list);
static DEFINE_SPINLOCK(inactive_ts_lock);
static DEFINE_SPINLOCK(ts_bitmap_lock);

static void iscsi_add_ts_to_inactive_list(struct iscsi_thread_set *ts)
{
	if (!list_empty(&ts->ts_list)) {
		WARN_ON(1);
		return;
	}
	spin_lock(&inactive_ts_lock);
	list_add_tail(&ts->ts_list, &inactive_ts_list);
	iscsit_global->inactive_ts++;
	spin_unlock(&inactive_ts_lock);
}

static struct iscsi_thread_set *iscsi_get_ts_from_inactive_list(void)
{
	struct iscsi_thread_set *ts;

	spin_lock(&inactive_ts_lock);
	if (list_empty(&inactive_ts_list)) {
		spin_unlock(&inactive_ts_lock);
		return NULL;
	}

	ts = list_first_entry(&inactive_ts_list, struct iscsi_thread_set, ts_list);

	list_del_init(&ts->ts_list);
	iscsit_global->inactive_ts--;
	spin_unlock(&inactive_ts_lock);

	return ts;
}

int iscsi_allocate_thread_sets(u32 thread_pair_count)
{
	int allocated_thread_pair_count = 0, i, thread_id;
	struct iscsi_thread_set *ts = NULL;

	for (i = 0; i < thread_pair_count; i++) {
		ts = kzalloc(sizeof(struct iscsi_thread_set), GFP_KERNEL);
		if (!ts) {
			pr_err("Unable to allocate memory for"
					" thread set.\n");
			return allocated_thread_pair_count;
		}
		/*
		 * Locate the next available regision in the thread_set_bitmap
		 */
		spin_lock(&ts_bitmap_lock);
		thread_id = bitmap_find_free_region(iscsit_global->ts_bitmap,
				iscsit_global->ts_bitmap_count, get_order(1));
		spin_unlock(&ts_bitmap_lock);
		if (thread_id < 0) {
			pr_err("bitmap_find_free_region() failed for"
				" thread_set_bitmap\n");
			kfree(ts);
			return allocated_thread_pair_count;
		}

		ts->thread_id = thread_id;
		ts->status = ISCSI_THREAD_SET_FREE;
		INIT_LIST_HEAD(&ts->ts_list);
		spin_lock_init(&ts->ts_state_lock);
		init_completion(&ts->rx_restart_comp);
		init_completion(&ts->tx_restart_comp);
		init_completion(&ts->rx_start_comp);
		init_completion(&ts->tx_start_comp);
		sema_init(&ts->ts_activate_sem, 0);

		ts->create_threads = 1;
		ts->tx_thread = kthread_run(iscsi_target_tx_thread, ts, "%s",
					ISCSI_TX_THREAD_NAME);
		if (IS_ERR(ts->tx_thread)) {
			dump_stack();
			pr_err("Unable to start iscsi_target_tx_thread\n");
			break;
		}

		ts->rx_thread = kthread_run(iscsi_target_rx_thread, ts, "%s",
					ISCSI_RX_THREAD_NAME);
		if (IS_ERR(ts->rx_thread)) {
			kthread_stop(ts->tx_thread);
			pr_err("Unable to start iscsi_target_rx_thread\n");
			break;
		}
		ts->create_threads = 0;

		iscsi_add_ts_to_inactive_list(ts);
		allocated_thread_pair_count++;
	}

	pr_debug("Spawned %d thread set(s) (%d total threads).\n",
		allocated_thread_pair_count, allocated_thread_pair_count * 2);
	return allocated_thread_pair_count;
}

static void iscsi_deallocate_thread_one(struct iscsi_thread_set *ts)
{
	spin_lock_bh(&ts->ts_state_lock);
	ts->status = ISCSI_THREAD_SET_DIE;

	if (ts->rx_thread) {
		complete(&ts->rx_start_comp);
		spin_unlock_bh(&ts->ts_state_lock);
		kthread_stop(ts->rx_thread);
		spin_lock_bh(&ts->ts_state_lock);
	}
	if (ts->tx_thread) {
		complete(&ts->tx_start_comp);
		spin_unlock_bh(&ts->ts_state_lock);
		kthread_stop(ts->tx_thread);
		spin_lock_bh(&ts->ts_state_lock);
	}
	spin_unlock_bh(&ts->ts_state_lock);
	/*
	 * Release this thread_id in the thread_set_bitmap
	 */
	spin_lock(&ts_bitmap_lock);
	bitmap_release_region(iscsit_global->ts_bitmap,
			ts->thread_id, get_order(1));
	spin_unlock(&ts_bitmap_lock);

	kfree(ts);
}

void iscsi_deallocate_thread_sets(void)
{
	struct iscsi_thread_set *ts = NULL;
	u32 released_count = 0;

	while ((ts = iscsi_get_ts_from_inactive_list())) {

		iscsi_deallocate_thread_one(ts);
		released_count++;
	}

	if (released_count)
		pr_debug("Stopped %d thread set(s) (%d total threads)."
			"\n", released_count, released_count * 2);
}

static void iscsi_deallocate_extra_thread_sets(void)
{
	u32 orig_count, released_count = 0;
	struct iscsi_thread_set *ts = NULL;

	orig_count = TARGET_THREAD_SET_COUNT;

	while ((iscsit_global->inactive_ts + 1) > orig_count) {
		ts = iscsi_get_ts_from_inactive_list();
		if (!ts)
			break;

		iscsi_deallocate_thread_one(ts);
		released_count++;
	}

	if (released_count)
		pr_debug("Stopped %d thread set(s) (%d total threads)."
			"\n", released_count, released_count * 2);
}

void iscsi_activate_thread_set(struct iscsi_conn *conn, struct iscsi_thread_set *ts)
{
	spin_lock_bh(&ts->ts_state_lock);
	conn->thread_set = ts;
	ts->conn = conn;
	ts->status = ISCSI_THREAD_SET_ACTIVE;
	spin_unlock_bh(&ts->ts_state_lock);

	complete(&ts->rx_start_comp);
	complete(&ts->tx_start_comp);

	down(&ts->ts_activate_sem);
}

struct iscsi_thread_set *iscsi_get_thread_set(void)
{
	struct iscsi_thread_set *ts;

get_set:
	ts = iscsi_get_ts_from_inactive_list();
	if (!ts) {
		iscsi_allocate_thread_sets(1);
		goto get_set;
	}

	ts->delay_inactive = 1;
	ts->signal_sent = 0;
	ts->thread_count = 2;
	init_completion(&ts->rx_restart_comp);
	init_completion(&ts->tx_restart_comp);
	sema_init(&ts->ts_activate_sem, 0);

	return ts;
}

void iscsi_set_thread_clear(struct iscsi_conn *conn, u8 thread_clear)
{
	struct iscsi_thread_set *ts = NULL;

	if (!conn->thread_set) {
		pr_err("struct iscsi_conn->thread_set is NULL\n");
		return;
	}
	ts = conn->thread_set;

	spin_lock_bh(&ts->ts_state_lock);
	ts->thread_clear &= ~thread_clear;

	if ((thread_clear & ISCSI_CLEAR_RX_THREAD) &&
	    (ts->blocked_threads & ISCSI_BLOCK_RX_THREAD))
		complete(&ts->rx_restart_comp);
	else if ((thread_clear & ISCSI_CLEAR_TX_THREAD) &&
		 (ts->blocked_threads & ISCSI_BLOCK_TX_THREAD))
		complete(&ts->tx_restart_comp);
	spin_unlock_bh(&ts->ts_state_lock);
}

void iscsi_set_thread_set_signal(struct iscsi_conn *conn, u8 signal_sent)
{
	struct iscsi_thread_set *ts = NULL;

	if (!conn->thread_set) {
		pr_err("struct iscsi_conn->thread_set is NULL\n");
		return;
	}
	ts = conn->thread_set;

	spin_lock_bh(&ts->ts_state_lock);
	ts->signal_sent |= signal_sent;
	spin_unlock_bh(&ts->ts_state_lock);
}

int iscsi_release_thread_set(struct iscsi_conn *conn)
{
	int thread_called = 0;
	struct iscsi_thread_set *ts = NULL;

	if (!conn || !conn->thread_set) {
		pr_err("connection or thread set pointer is NULL\n");
		BUG();
	}
	ts = conn->thread_set;

	spin_lock_bh(&ts->ts_state_lock);
	ts->status = ISCSI_THREAD_SET_RESET;

	if (!strncmp(current->comm, ISCSI_RX_THREAD_NAME,
			strlen(ISCSI_RX_THREAD_NAME)))
		thread_called = ISCSI_RX_THREAD;
	else if (!strncmp(current->comm, ISCSI_TX_THREAD_NAME,
			strlen(ISCSI_TX_THREAD_NAME)))
		thread_called = ISCSI_TX_THREAD;

	if (ts->rx_thread && (thread_called == ISCSI_TX_THREAD) &&
	   (ts->thread_clear & ISCSI_CLEAR_RX_THREAD)) {

		if (!(ts->signal_sent & ISCSI_SIGNAL_RX_THREAD)) {
			send_sig(SIGINT, ts->rx_thread, 1);
			ts->signal_sent |= ISCSI_SIGNAL_RX_THREAD;
		}
		ts->blocked_threads |= ISCSI_BLOCK_RX_THREAD;
		spin_unlock_bh(&ts->ts_state_lock);
		wait_for_completion(&ts->rx_restart_comp);
		spin_lock_bh(&ts->ts_state_lock);
		ts->blocked_threads &= ~ISCSI_BLOCK_RX_THREAD;
	}
	if (ts->tx_thread && (thread_called == ISCSI_RX_THREAD) &&
	   (ts->thread_clear & ISCSI_CLEAR_TX_THREAD)) {

		if (!(ts->signal_sent & ISCSI_SIGNAL_TX_THREAD)) {
			send_sig(SIGINT, ts->tx_thread, 1);
			ts->signal_sent |= ISCSI_SIGNAL_TX_THREAD;
		}
		ts->blocked_threads |= ISCSI_BLOCK_TX_THREAD;
		spin_unlock_bh(&ts->ts_state_lock);
		wait_for_completion(&ts->tx_restart_comp);
		spin_lock_bh(&ts->ts_state_lock);
		ts->blocked_threads &= ~ISCSI_BLOCK_TX_THREAD;
	}

	ts->conn = NULL;
	ts->status = ISCSI_THREAD_SET_FREE;
	spin_unlock_bh(&ts->ts_state_lock);

	return 0;
}

int iscsi_thread_set_force_reinstatement(struct iscsi_conn *conn)
{
	struct iscsi_thread_set *ts;

	if (!conn->thread_set)
		return -1;
	ts = conn->thread_set;

	spin_lock_bh(&ts->ts_state_lock);
	if (ts->status != ISCSI_THREAD_SET_ACTIVE) {
		spin_unlock_bh(&ts->ts_state_lock);
		return -1;
	}

	if (ts->tx_thread && (!(ts->signal_sent & ISCSI_SIGNAL_TX_THREAD))) {
		send_sig(SIGINT, ts->tx_thread, 1);
		ts->signal_sent |= ISCSI_SIGNAL_TX_THREAD;
	}
	if (ts->rx_thread && (!(ts->signal_sent & ISCSI_SIGNAL_RX_THREAD))) {
		send_sig(SIGINT, ts->rx_thread, 1);
		ts->signal_sent |= ISCSI_SIGNAL_RX_THREAD;
	}
	spin_unlock_bh(&ts->ts_state_lock);

	return 0;
}

static void iscsi_check_to_add_additional_sets(void)
{
	int thread_sets_add;

	spin_lock(&inactive_ts_lock);
	thread_sets_add = iscsit_global->inactive_ts;
	spin_unlock(&inactive_ts_lock);
	if (thread_sets_add == 1)
		iscsi_allocate_thread_sets(1);
}

static int iscsi_signal_thread_pre_handler(struct iscsi_thread_set *ts)
{
	spin_lock_bh(&ts->ts_state_lock);
	if (ts->status == ISCSI_THREAD_SET_DIE || kthread_should_stop() ||
	    signal_pending(current)) {
		spin_unlock_bh(&ts->ts_state_lock);
		return -1;
	}
	spin_unlock_bh(&ts->ts_state_lock);

	return 0;
}

struct iscsi_conn *iscsi_rx_thread_pre_handler(struct iscsi_thread_set *ts)
{
	int ret;

	spin_lock_bh(&ts->ts_state_lock);
	if (ts->create_threads) {
		spin_unlock_bh(&ts->ts_state_lock);
		goto sleep;
	}

	if (ts->status != ISCSI_THREAD_SET_DIE)
		flush_signals(current);

	if (ts->delay_inactive && (--ts->thread_count == 0)) {
		spin_unlock_bh(&ts->ts_state_lock);

		if (!iscsit_global->in_shutdown)
			iscsi_deallocate_extra_thread_sets();

		iscsi_add_ts_to_inactive_list(ts);
		spin_lock_bh(&ts->ts_state_lock);
	}

	if ((ts->status == ISCSI_THREAD_SET_RESET) &&
	    (ts->thread_clear & ISCSI_CLEAR_RX_THREAD))
		complete(&ts->rx_restart_comp);

	ts->thread_clear &= ~ISCSI_CLEAR_RX_THREAD;
	spin_unlock_bh(&ts->ts_state_lock);
sleep:
	ret = wait_for_completion_interruptible(&ts->rx_start_comp);
	if (ret != 0)
		return NULL;

	if (iscsi_signal_thread_pre_handler(ts) < 0)
		return NULL;

	iscsi_check_to_add_additional_sets();

	spin_lock_bh(&ts->ts_state_lock);
	if (!ts->conn) {
		pr_err("struct iscsi_thread_set->conn is NULL for"
			" RX thread_id: %s/%d\n", current->comm, current->pid);
		spin_unlock_bh(&ts->ts_state_lock);
		return NULL;
	}
	ts->thread_clear |= ISCSI_CLEAR_RX_THREAD;
	spin_unlock_bh(&ts->ts_state_lock);

	up(&ts->ts_activate_sem);

	return ts->conn;
}

struct iscsi_conn *iscsi_tx_thread_pre_handler(struct iscsi_thread_set *ts)
{
	int ret;

	spin_lock_bh(&ts->ts_state_lock);
	if (ts->create_threads) {
		spin_unlock_bh(&ts->ts_state_lock);
		goto sleep;
	}

	if (ts->status != ISCSI_THREAD_SET_DIE)
		flush_signals(current);

	if (ts->delay_inactive && (--ts->thread_count == 0)) {
		spin_unlock_bh(&ts->ts_state_lock);

		if (!iscsit_global->in_shutdown)
			iscsi_deallocate_extra_thread_sets();

		iscsi_add_ts_to_inactive_list(ts);
		spin_lock_bh(&ts->ts_state_lock);
	}
	if ((ts->status == ISCSI_THREAD_SET_RESET) &&
	    (ts->thread_clear & ISCSI_CLEAR_TX_THREAD))
		complete(&ts->tx_restart_comp);

	ts->thread_clear &= ~ISCSI_CLEAR_TX_THREAD;
	spin_unlock_bh(&ts->ts_state_lock);
sleep:
	ret = wait_for_completion_interruptible(&ts->tx_start_comp);
	if (ret != 0)
		return NULL;

	if (iscsi_signal_thread_pre_handler(ts) < 0)
		return NULL;

	iscsi_check_to_add_additional_sets();

	spin_lock_bh(&ts->ts_state_lock);
	if (!ts->conn) {
		pr_err("struct iscsi_thread_set->conn is NULL for"
			" TX thread_id: %s/%d\n", current->comm, current->pid);
		spin_unlock_bh(&ts->ts_state_lock);
		return NULL;
	}
	ts->thread_clear |= ISCSI_CLEAR_TX_THREAD;
	spin_unlock_bh(&ts->ts_state_lock);

	up(&ts->ts_activate_sem);

	return ts->conn;
}

int iscsi_thread_set_init(void)
{
	int size;

	iscsit_global->ts_bitmap_count = ISCSI_TS_BITMAP_BITS;

	size = BITS_TO_LONGS(iscsit_global->ts_bitmap_count) * sizeof(long);
	iscsit_global->ts_bitmap = kzalloc(size, GFP_KERNEL);
	if (!iscsit_global->ts_bitmap) {
		pr_err("Unable to allocate iscsit_global->ts_bitmap\n");
		return -ENOMEM;
	}

	return 0;
}

void iscsi_thread_set_free(void)
{
	kfree(iscsit_global->ts_bitmap);
}
