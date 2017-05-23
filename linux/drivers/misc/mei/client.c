/*
 *
 * Intel Management Engine Interface (Intel MEI) Linux driver
 * Copyright (c) 2003-2012, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>

#include <linux/mei.h>

#include "mei_dev.h"
#include "hbm.h"
#include "client.h"

/**
 * mei_me_cl_init - initialize me client
 *
 * @me_cl: me client
 */
void mei_me_cl_init(struct mei_me_client *me_cl)
{
	INIT_LIST_HEAD(&me_cl->list);
	kref_init(&me_cl->refcnt);
}

/**
 * mei_me_cl_get - increases me client refcount
 *
 * @me_cl: me client
 *
 * Locking: called under "dev->device_lock" lock
 *
 * Return: me client or NULL
 */
struct mei_me_client *mei_me_cl_get(struct mei_me_client *me_cl)
{
	if (me_cl)
		kref_get(&me_cl->refcnt);

	return me_cl;
}

/**
 * mei_me_cl_release - unlink and free me client
 *
 * Locking: called under "dev->device_lock" lock
 *
 * @ref: me_client refcount
 */
static void mei_me_cl_release(struct kref *ref)
{
	struct mei_me_client *me_cl =
		container_of(ref, struct mei_me_client, refcnt);
	list_del(&me_cl->list);
	kfree(me_cl);
}
/**
 * mei_me_cl_put - decrease me client refcount and free client if necessary
 *
 * Locking: called under "dev->device_lock" lock
 *
 * @me_cl: me client
 */
void mei_me_cl_put(struct mei_me_client *me_cl)
{
	if (me_cl)
		kref_put(&me_cl->refcnt, mei_me_cl_release);
}

/**
 * mei_me_cl_by_uuid - locate me client by uuid
 *	increases ref count
 *
 * @dev: mei device
 * @uuid: me client uuid
 *
 * Locking: called under "dev->device_lock" lock
 *
 * Return: me client or NULL if not found
 */
struct mei_me_client *mei_me_cl_by_uuid(const struct mei_device *dev,
					const uuid_le *uuid)
{
	struct mei_me_client *me_cl;

	list_for_each_entry(me_cl, &dev->me_clients, list)
		if (uuid_le_cmp(*uuid, me_cl->props.protocol_name) == 0)
			return mei_me_cl_get(me_cl);

	return NULL;
}

/**
 * mei_me_cl_by_id - locate me client by client id
 *	increases ref count
 *
 * @dev: the device structure
 * @client_id: me client id
 *
 * Locking: called under "dev->device_lock" lock
 *
 * Return: me client or NULL if not found
 */
struct mei_me_client *mei_me_cl_by_id(struct mei_device *dev, u8 client_id)
{

	struct mei_me_client *me_cl;

	list_for_each_entry(me_cl, &dev->me_clients, list)
		if (me_cl->client_id == client_id)
			return mei_me_cl_get(me_cl);

	return NULL;
}

/**
 * mei_me_cl_by_uuid_id - locate me client by client id and uuid
 *	increases ref count
 *
 * @dev: the device structure
 * @uuid: me client uuid
 * @client_id: me client id
 *
 * Locking: called under "dev->device_lock" lock
 *
 * Return: me client or NULL if not found
 */
struct mei_me_client *mei_me_cl_by_uuid_id(struct mei_device *dev,
					   const uuid_le *uuid, u8 client_id)
{
	struct mei_me_client *me_cl;

	list_for_each_entry(me_cl, &dev->me_clients, list)
		if (uuid_le_cmp(*uuid, me_cl->props.protocol_name) == 0 &&
		    me_cl->client_id == client_id)
			return mei_me_cl_get(me_cl);

	return NULL;
}

/**
 * mei_me_cl_rm_by_uuid - remove all me clients matching uuid
 *
 * @dev: the device structure
 * @uuid: me client uuid
 *
 * Locking: called under "dev->device_lock" lock
 */
void mei_me_cl_rm_by_uuid(struct mei_device *dev, const uuid_le *uuid)
{
	struct mei_me_client *me_cl, *next;

	dev_dbg(dev->dev, "remove %pUl\n", uuid);
	list_for_each_entry_safe(me_cl, next, &dev->me_clients, list)
		if (uuid_le_cmp(*uuid, me_cl->props.protocol_name) == 0)
			mei_me_cl_put(me_cl);
}

/**
 * mei_me_cl_rm_by_uuid_id - remove all me clients matching client id
 *
 * @dev: the device structure
 * @uuid: me client uuid
 * @id: me client id
 *
 * Locking: called under "dev->device_lock" lock
 */
void mei_me_cl_rm_by_uuid_id(struct mei_device *dev, const uuid_le *uuid, u8 id)
{
	struct mei_me_client *me_cl, *next;
	const uuid_le *pn;

	dev_dbg(dev->dev, "remove %pUl %d\n", uuid, id);
	list_for_each_entry_safe(me_cl, next, &dev->me_clients, list) {
		pn =  &me_cl->props.protocol_name;
		if (me_cl->client_id == id && uuid_le_cmp(*uuid, *pn) == 0)
			mei_me_cl_put(me_cl);
	}
}

/**
 * mei_me_cl_rm_all - remove all me clients
 *
 * @dev: the device structure
 *
 * Locking: called under "dev->device_lock" lock
 */
void mei_me_cl_rm_all(struct mei_device *dev)
{
	struct mei_me_client *me_cl, *next;

	list_for_each_entry_safe(me_cl, next, &dev->me_clients, list)
			mei_me_cl_put(me_cl);
}



/**
 * mei_cl_cmp_id - tells if the clients are the same
 *
 * @cl1: host client 1
 * @cl2: host client 2
 *
 * Return: true  - if the clients has same host and me ids
 *         false - otherwise
 */
static inline bool mei_cl_cmp_id(const struct mei_cl *cl1,
				const struct mei_cl *cl2)
{
	return cl1 && cl2 &&
		(cl1->host_client_id == cl2->host_client_id) &&
		(cl1->me_client_id == cl2->me_client_id);
}

/**
 * mei_io_list_flush - removes cbs belonging to cl.
 *
 * @list:  an instance of our list structure
 * @cl:    host client, can be NULL for flushing the whole list
 * @free:  whether to free the cbs
 */
static void __mei_io_list_flush(struct mei_cl_cb *list,
				struct mei_cl *cl, bool free)
{
	struct mei_cl_cb *cb;
	struct mei_cl_cb *next;

	/* enable removing everything if no cl is specified */
	list_for_each_entry_safe(cb, next, &list->list, list) {
		if (!cl || mei_cl_cmp_id(cl, cb->cl)) {
			list_del(&cb->list);
			if (free)
				mei_io_cb_free(cb);
		}
	}
}

/**
 * mei_io_list_flush - removes list entry belonging to cl.
 *
 * @list:  An instance of our list structure
 * @cl: host client
 */
void mei_io_list_flush(struct mei_cl_cb *list, struct mei_cl *cl)
{
	__mei_io_list_flush(list, cl, false);
}


/**
 * mei_io_list_free - removes cb belonging to cl and free them
 *
 * @list:  An instance of our list structure
 * @cl: host client
 */
static inline void mei_io_list_free(struct mei_cl_cb *list, struct mei_cl *cl)
{
	__mei_io_list_flush(list, cl, true);
}

/**
 * mei_io_cb_free - free mei_cb_private related memory
 *
 * @cb: mei callback struct
 */
void mei_io_cb_free(struct mei_cl_cb *cb)
{
	if (cb == NULL)
		return;

	kfree(cb->request_buffer.data);
	kfree(cb->response_buffer.data);
	kfree(cb);
}

/**
 * mei_io_cb_init - allocate and initialize io callback
 *
 * @cl: mei client
 * @fp: pointer to file structure
 *
 * Return: mei_cl_cb pointer or NULL;
 */
struct mei_cl_cb *mei_io_cb_init(struct mei_cl *cl, struct file *fp)
{
	struct mei_cl_cb *cb;

	cb = kzalloc(sizeof(struct mei_cl_cb), GFP_KERNEL);
	if (!cb)
		return NULL;

	mei_io_list_init(cb);

	cb->file_object = fp;
	cb->cl = cl;
	cb->buf_idx = 0;
	return cb;
}

/**
 * mei_io_cb_alloc_req_buf - allocate request buffer
 *
 * @cb: io callback structure
 * @length: size of the buffer
 *
 * Return: 0 on success
 *         -EINVAL if cb is NULL
 *         -ENOMEM if allocation failed
 */
int mei_io_cb_alloc_req_buf(struct mei_cl_cb *cb, size_t length)
{
	if (!cb)
		return -EINVAL;

	if (length == 0)
		return 0;

	cb->request_buffer.data = kmalloc(length, GFP_KERNEL);
	if (!cb->request_buffer.data)
		return -ENOMEM;
	cb->request_buffer.size = length;
	return 0;
}
/**
 * mei_io_cb_alloc_resp_buf - allocate response buffer
 *
 * @cb: io callback structure
 * @length: size of the buffer
 *
 * Return: 0 on success
 *         -EINVAL if cb is NULL
 *         -ENOMEM if allocation failed
 */
int mei_io_cb_alloc_resp_buf(struct mei_cl_cb *cb, size_t length)
{
	if (!cb)
		return -EINVAL;

	if (length == 0)
		return 0;

	cb->response_buffer.data = kmalloc(length, GFP_KERNEL);
	if (!cb->response_buffer.data)
		return -ENOMEM;
	cb->response_buffer.size = length;
	return 0;
}



/**
 * mei_cl_flush_queues - flushes queue lists belonging to cl.
 *
 * @cl: host client
 *
 * Return: 0 on success, -EINVAL if cl or cl->dev is NULL.
 */
int mei_cl_flush_queues(struct mei_cl *cl)
{
	struct mei_device *dev;

	if (WARN_ON(!cl || !cl->dev))
		return -EINVAL;

	dev = cl->dev;

	cl_dbg(dev, cl, "remove list entry belonging to cl\n");
	mei_io_list_flush(&cl->dev->read_list, cl);
	mei_io_list_free(&cl->dev->write_list, cl);
	mei_io_list_free(&cl->dev->write_waiting_list, cl);
	mei_io_list_flush(&cl->dev->ctrl_wr_list, cl);
	mei_io_list_flush(&cl->dev->ctrl_rd_list, cl);
	mei_io_list_flush(&cl->dev->amthif_cmd_list, cl);
	mei_io_list_flush(&cl->dev->amthif_rd_complete_list, cl);
	return 0;
}


/**
 * mei_cl_init - initializes cl.
 *
 * @cl: host client to be initialized
 * @dev: mei device
 */
void mei_cl_init(struct mei_cl *cl, struct mei_device *dev)
{
	memset(cl, 0, sizeof(struct mei_cl));
	init_waitqueue_head(&cl->wait);
	init_waitqueue_head(&cl->rx_wait);
	init_waitqueue_head(&cl->tx_wait);
	INIT_LIST_HEAD(&cl->link);
	INIT_LIST_HEAD(&cl->device_link);
	cl->reading_state = MEI_IDLE;
	cl->writing_state = MEI_IDLE;
	cl->dev = dev;
}

/**
 * mei_cl_allocate - allocates cl  structure and sets it up.
 *
 * @dev: mei device
 * Return:  The allocated file or NULL on failure
 */
struct mei_cl *mei_cl_allocate(struct mei_device *dev)
{
	struct mei_cl *cl;

	cl = kmalloc(sizeof(struct mei_cl), GFP_KERNEL);
	if (!cl)
		return NULL;

	mei_cl_init(cl, dev);

	return cl;
}

/**
 * mei_cl_find_read_cb - find this cl's callback in the read list
 *
 * @cl: host client
 *
 * Return: cb on success, NULL on error
 */
struct mei_cl_cb *mei_cl_find_read_cb(struct mei_cl *cl)
{
	struct mei_device *dev = cl->dev;
	struct mei_cl_cb *cb;

	list_for_each_entry(cb, &dev->read_list.list, list)
		if (mei_cl_cmp_id(cl, cb->cl))
			return cb;
	return NULL;
}

/** mei_cl_link: allocate host id in the host map
 *
 * @cl - host client
 * @id - fixed host id or -1 for generic one
 *
 * Return: 0 on success
 *	-EINVAL on incorrect values
 *	-ENONET if client not found
 */
int mei_cl_link(struct mei_cl *cl, int id)
{
	struct mei_device *dev;
	long open_handle_count;

	if (WARN_ON(!cl || !cl->dev))
		return -EINVAL;

	dev = cl->dev;

	/* If Id is not assigned get one*/
	if (id == MEI_HOST_CLIENT_ID_ANY)
		id = find_first_zero_bit(dev->host_clients_map,
					MEI_CLIENTS_MAX);

	if (id >= MEI_CLIENTS_MAX) {
		dev_err(dev->dev, "id exceeded %d", MEI_CLIENTS_MAX);
		return -EMFILE;
	}

	open_handle_count = dev->open_handle_count + dev->iamthif_open_count;
	if (open_handle_count >= MEI_MAX_OPEN_HANDLE_COUNT) {
		dev_err(dev->dev, "open_handle_count exceeded %d",
			MEI_MAX_OPEN_HANDLE_COUNT);
		return -EMFILE;
	}

	dev->open_handle_count++;

	cl->host_client_id = id;
	list_add_tail(&cl->link, &dev->file_list);

	set_bit(id, dev->host_clients_map);

	cl->state = MEI_FILE_INITIALIZING;

	cl_dbg(dev, cl, "link cl\n");
	return 0;
}

/**
 * mei_cl_unlink - remove me_cl from the list
 *
 * @cl: host client
 *
 * Return: always 0
 */
int mei_cl_unlink(struct mei_cl *cl)
{
	struct mei_device *dev;

	/* don't shout on error exit path */
	if (!cl)
		return 0;

	/* wd and amthif might not be initialized */
	if (!cl->dev)
		return 0;

	dev = cl->dev;

	cl_dbg(dev, cl, "unlink client");

	if (dev->open_handle_count > 0)
		dev->open_handle_count--;

	/* never clear the 0 bit */
	if (cl->host_client_id)
		clear_bit(cl->host_client_id, dev->host_clients_map);

	list_del_init(&cl->link);

	cl->state = MEI_FILE_INITIALIZING;

	return 0;
}


void mei_host_client_init(struct work_struct *work)
{
	struct mei_device *dev = container_of(work,
					      struct mei_device, init_work);
	struct mei_me_client *me_cl;
	struct mei_client_properties *props;

	mutex_lock(&dev->device_lock);

	list_for_each_entry(me_cl, &dev->me_clients, list) {
		props = &me_cl->props;

		if (!uuid_le_cmp(props->protocol_name, mei_amthif_guid))
			mei_amthif_host_init(dev);
		else if (!uuid_le_cmp(props->protocol_name, mei_wd_guid))
			mei_wd_host_init(dev);
		else if (!uuid_le_cmp(props->protocol_name, mei_nfc_guid))
			mei_nfc_host_init(dev);

	}

	dev->dev_state = MEI_DEV_ENABLED;
	dev->reset_count = 0;

	mutex_unlock(&dev->device_lock);

	pm_runtime_mark_last_busy(dev->dev);
	dev_dbg(dev->dev, "rpm: autosuspend\n");
	pm_runtime_autosuspend(dev->dev);
}

/**
 * mei_hbuf_acquire - try to acquire host buffer
 *
 * @dev: the device structure
 * Return: true if host buffer was acquired
 */
bool mei_hbuf_acquire(struct mei_device *dev)
{
	if (mei_pg_state(dev) == MEI_PG_ON ||
	    dev->pg_event == MEI_PG_EVENT_WAIT) {
		dev_dbg(dev->dev, "device is in pg\n");
		return false;
	}

	if (!dev->hbuf_is_ready) {
		dev_dbg(dev->dev, "hbuf is not ready\n");
		return false;
	}

	dev->hbuf_is_ready = false;

	return true;
}

/**
 * mei_cl_disconnect - disconnect host client from the me one
 *
 * @cl: host client
 *
 * Locking: called under "dev->device_lock" lock
 *
 * Return: 0 on success, <0 on failure.
 */
int mei_cl_disconnect(struct mei_cl *cl)
{
	struct mei_device *dev;
	struct mei_cl_cb *cb;
	int rets;

	if (WARN_ON(!cl || !cl->dev))
		return -ENODEV;

	dev = cl->dev;

	cl_dbg(dev, cl, "disconnecting");

	if (cl->state != MEI_FILE_DISCONNECTING)
		return 0;

	rets = pm_runtime_get(dev->dev);
	if (rets < 0 && rets != -EINPROGRESS) {
		pm_runtime_put_noidle(dev->dev);
		cl_err(dev, cl, "rpm: get failed %d\n", rets);
		return rets;
	}

	cb = mei_io_cb_init(cl, NULL);
	if (!cb) {
		rets = -ENOMEM;
		goto free;
	}

	cb->fop_type = MEI_FOP_DISCONNECT;

	if (mei_hbuf_acquire(dev)) {
		if (mei_hbm_cl_disconnect_req(dev, cl)) {
			rets = -ENODEV;
			cl_err(dev, cl, "failed to disconnect.\n");
			goto free;
		}
		cl->timer_count = MEI_CONNECT_TIMEOUT;
		mdelay(10); /* Wait for hardware disconnection ready */
		list_add_tail(&cb->list, &dev->ctrl_rd_list.list);
	} else {
		cl_dbg(dev, cl, "add disconnect cb to control write list\n");
		list_add_tail(&cb->list, &dev->ctrl_wr_list.list);

	}
	mutex_unlock(&dev->device_lock);

	wait_event_timeout(cl->wait,
			MEI_FILE_DISCONNECTED == cl->state,
			mei_secs_to_jiffies(MEI_CL_CONNECT_TIMEOUT));

	mutex_lock(&dev->device_lock);

	if (MEI_FILE_DISCONNECTED == cl->state) {
		rets = 0;
		cl_dbg(dev, cl, "successfully disconnected from FW client.\n");
	} else {
		cl_dbg(dev, cl, "timeout on disconnect from FW client.\n");
		rets = -ETIME;
	}

	mei_io_list_flush(&dev->ctrl_rd_list, cl);
	mei_io_list_flush(&dev->ctrl_wr_list, cl);
free:
	cl_dbg(dev, cl, "rpm: autosuspend\n");
	pm_runtime_mark_last_busy(dev->dev);
	pm_runtime_put_autosuspend(dev->dev);

	mei_io_cb_free(cb);
	return rets;
}


/**
 * mei_cl_is_other_connecting - checks if other
 *    client with the same me client id is connecting
 *
 * @cl: private data of the file object
 *
 * Return: true if other client is connected, false - otherwise.
 */
bool mei_cl_is_other_connecting(struct mei_cl *cl)
{
	struct mei_device *dev;
	struct mei_cl *ocl; /* the other client */

	if (WARN_ON(!cl || !cl->dev))
		return false;

	dev = cl->dev;

	list_for_each_entry(ocl, &dev->file_list, link) {
		if (ocl->state == MEI_FILE_CONNECTING &&
		    ocl != cl &&
		    cl->me_client_id == ocl->me_client_id)
			return true;

	}

	return false;
}

/**
 * mei_cl_connect - connect host client to the me one
 *
 * @cl: host client
 * @file: pointer to file structure
 *
 * Locking: called under "dev->device_lock" lock
 *
 * Return: 0 on success, <0 on failure.
 */
int mei_cl_connect(struct mei_cl *cl, struct file *file)
{
	struct mei_device *dev;
	struct mei_cl_cb *cb;
	int rets;

	if (WARN_ON(!cl || !cl->dev))
		return -ENODEV;

	dev = cl->dev;

	rets = pm_runtime_get(dev->dev);
	if (rets < 0 && rets != -EINPROGRESS) {
		pm_runtime_put_noidle(dev->dev);
		cl_err(dev, cl, "rpm: get failed %d\n", rets);
		return rets;
	}

	cb = mei_io_cb_init(cl, file);
	if (!cb) {
		rets = -ENOMEM;
		goto out;
	}

	cb->fop_type = MEI_FOP_CONNECT;

	/* run hbuf acquire last so we don't have to undo */
	if (!mei_cl_is_other_connecting(cl) && mei_hbuf_acquire(dev)) {
		cl->state = MEI_FILE_CONNECTING;
		if (mei_hbm_cl_connect_req(dev, cl)) {
			rets = -ENODEV;
			goto out;
		}
		cl->timer_count = MEI_CONNECT_TIMEOUT;
		list_add_tail(&cb->list, &dev->ctrl_rd_list.list);
	} else {
		cl->state = MEI_FILE_INITIALIZING;
		list_add_tail(&cb->list, &dev->ctrl_wr_list.list);
	}

	mutex_unlock(&dev->device_lock);
	wait_event_timeout(cl->wait,
			(cl->state == MEI_FILE_CONNECTED ||
			 cl->state == MEI_FILE_DISCONNECTED),
			mei_secs_to_jiffies(MEI_CL_CONNECT_TIMEOUT));
	mutex_lock(&dev->device_lock);

	if (cl->state != MEI_FILE_CONNECTED) {
		cl->state = MEI_FILE_DISCONNECTED;
		/* something went really wrong */
		if (!cl->status)
			cl->status = -EFAULT;

		mei_io_list_flush(&dev->ctrl_rd_list, cl);
		mei_io_list_flush(&dev->ctrl_wr_list, cl);
	}

	rets = cl->status;

out:
	cl_dbg(dev, cl, "rpm: autosuspend\n");
	pm_runtime_mark_last_busy(dev->dev);
	pm_runtime_put_autosuspend(dev->dev);

	mei_io_cb_free(cb);
	return rets;
}

/**
 * mei_cl_flow_ctrl_creds - checks flow_control credits for cl.
 *
 * @cl: private data of the file object
 *
 * Return: 1 if mei_flow_ctrl_creds >0, 0 - otherwise.
 *	-ENOENT if mei_cl is not present
 *	-EINVAL if single_recv_buf == 0
 */
int mei_cl_flow_ctrl_creds(struct mei_cl *cl)
{
	struct mei_device *dev;
	struct mei_me_client *me_cl;
	int rets = 0;

	if (WARN_ON(!cl || !cl->dev))
		return -EINVAL;

	dev = cl->dev;

	if (cl->mei_flow_ctrl_creds > 0)
		return 1;

	me_cl = mei_me_cl_by_uuid_id(dev, &cl->cl_uuid, cl->me_client_id);
	if (!me_cl) {
		cl_err(dev, cl, "no such me client %d\n", cl->me_client_id);
		return -ENOENT;
	}

	if (me_cl->mei_flow_ctrl_creds > 0) {
		rets = 1;
		if (WARN_ON(me_cl->props.single_recv_buf == 0))
			rets = -EINVAL;
	}
	mei_me_cl_put(me_cl);
	return rets;
}

/**
 * mei_cl_flow_ctrl_reduce - reduces flow_control.
 *
 * @cl: private data of the file object
 *
 * Return:
 *	0 on success
 *	-ENOENT when me client is not found
 *	-EINVAL when ctrl credits are <= 0
 */
int mei_cl_flow_ctrl_reduce(struct mei_cl *cl)
{
	struct mei_device *dev;
	struct mei_me_client *me_cl;
	int rets;

	if (WARN_ON(!cl || !cl->dev))
		return -EINVAL;

	dev = cl->dev;

	me_cl = mei_me_cl_by_uuid_id(dev, &cl->cl_uuid, cl->me_client_id);
	if (!me_cl) {
		cl_err(dev, cl, "no such me client %d\n", cl->me_client_id);
		return -ENOENT;
	}

	if (me_cl->props.single_recv_buf) {
		if (WARN_ON(me_cl->mei_flow_ctrl_creds <= 0)) {
			rets = -EINVAL;
			goto out;
		}
		me_cl->mei_flow_ctrl_creds--;
	} else {
		if (WARN_ON(cl->mei_flow_ctrl_creds <= 0)) {
			rets = -EINVAL;
			goto out;
		}
		cl->mei_flow_ctrl_creds--;
	}
	rets = 0;
out:
	mei_me_cl_put(me_cl);
	return rets;
}

/**
 * mei_cl_read_start - the start read client message function.
 *
 * @cl: host client
 * @length: number of bytes to read
 *
 * Return: 0 on success, <0 on failure.
 */
int mei_cl_read_start(struct mei_cl *cl, size_t length)
{
	struct mei_device *dev;
	struct mei_cl_cb *cb;
	struct mei_me_client *me_cl;
	int rets;

	if (WARN_ON(!cl || !cl->dev))
		return -ENODEV;

	dev = cl->dev;

	if (!mei_cl_is_connected(cl))
		return -ENODEV;

	if (cl->read_cb) {
		cl_dbg(dev, cl, "read is pending.\n");
		return -EBUSY;
	}
	me_cl = mei_me_cl_by_uuid_id(dev, &cl->cl_uuid, cl->me_client_id);
	if (!me_cl) {
		cl_err(dev, cl, "no such me client %d\n", cl->me_client_id);
		return  -ENOTTY;
	}
	/* always allocate at least client max message */
	length = max_t(size_t, length, me_cl->props.max_msg_length);
	mei_me_cl_put(me_cl);

	rets = pm_runtime_get(dev->dev);
	if (rets < 0 && rets != -EINPROGRESS) {
		pm_runtime_put_noidle(dev->dev);
		cl_err(dev, cl, "rpm: get failed %d\n", rets);
		return rets;
	}

	cb = mei_io_cb_init(cl, NULL);
	if (!cb) {
		rets = -ENOMEM;
		goto out;
	}

	rets = mei_io_cb_alloc_resp_buf(cb, length);
	if (rets)
		goto out;

	cb->fop_type = MEI_FOP_READ;
	if (mei_hbuf_acquire(dev)) {
		rets = mei_hbm_cl_flow_control_req(dev, cl);
		if (rets < 0)
			goto out;

		list_add_tail(&cb->list, &dev->read_list.list);
	} else {
		list_add_tail(&cb->list, &dev->ctrl_wr_list.list);
	}

	cl->read_cb = cb;

out:
	cl_dbg(dev, cl, "rpm: autosuspend\n");
	pm_runtime_mark_last_busy(dev->dev);
	pm_runtime_put_autosuspend(dev->dev);

	if (rets)
		mei_io_cb_free(cb);

	return rets;
}

/**
 * mei_cl_irq_write - write a message to device
 *	from the interrupt thread context
 *
 * @cl: client
 * @cb: callback block.
 * @cmpl_list: complete list.
 *
 * Return: 0, OK; otherwise error.
 */
int mei_cl_irq_write(struct mei_cl *cl, struct mei_cl_cb *cb,
		     struct mei_cl_cb *cmpl_list)
{
	struct mei_device *dev;
	struct mei_msg_data *buf;
	struct mei_msg_hdr mei_hdr;
	size_t len;
	u32 msg_slots;
	int slots;
	int rets;

	if (WARN_ON(!cl || !cl->dev))
		return -ENODEV;

	dev = cl->dev;

	buf = &cb->request_buffer;

	rets = mei_cl_flow_ctrl_creds(cl);
	if (rets < 0)
		return rets;

	if (rets == 0) {
		cl_dbg(dev, cl, "No flow control credentials: not sending.\n");
		return 0;
	}

	slots = mei_hbuf_empty_slots(dev);
	len = buf->size - cb->buf_idx;
	msg_slots = mei_data2slots(len);

	mei_hdr.host_addr = cl->host_client_id;
	mei_hdr.me_addr = cl->me_client_id;
	mei_hdr.reserved = 0;
	mei_hdr.internal = cb->internal;

	if (slots >= msg_slots) {
		mei_hdr.length = len;
		mei_hdr.msg_complete = 1;
	/* Split the message only if we can write the whole host buffer */
	} else if (slots == dev->hbuf_depth) {
		msg_slots = slots;
		len = (slots * sizeof(u32)) - sizeof(struct mei_msg_hdr);
		mei_hdr.length = len;
		mei_hdr.msg_complete = 0;
	} else {
		/* wait for next time the host buffer is empty */
		return 0;
	}

	cl_dbg(dev, cl, "buf: size = %d idx = %lu\n",
			cb->request_buffer.size, cb->buf_idx);

	rets = mei_write_message(dev, &mei_hdr, buf->data + cb->buf_idx);
	if (rets) {
		cl->status = rets;
		list_move_tail(&cb->list, &cmpl_list->list);
		return rets;
	}

	cl->status = 0;
	cl->writing_state = MEI_WRITING;
	cb->buf_idx += mei_hdr.length;

	if (mei_hdr.msg_complete) {
		if (mei_cl_flow_ctrl_reduce(cl))
			return -EIO;
		list_move_tail(&cb->list, &dev->write_waiting_list.list);
	}

	return 0;
}

/**
 * mei_cl_write - submit a write cb to mei device
 *	assumes device_lock is locked
 *
 * @cl: host client
 * @cb: write callback with filled data
 * @blocking: block until completed
 *
 * Return: number of bytes sent on success, <0 on failure.
 */
int mei_cl_write(struct mei_cl *cl, struct mei_cl_cb *cb, bool blocking)
{
	struct mei_device *dev;
	struct mei_msg_data *buf;
	struct mei_msg_hdr mei_hdr;
	int rets;


	if (WARN_ON(!cl || !cl->dev))
		return -ENODEV;

	if (WARN_ON(!cb))
		return -EINVAL;

	dev = cl->dev;


	buf = &cb->request_buffer;

	cl_dbg(dev, cl, "size=%d\n", buf->size);

	rets = pm_runtime_get(dev->dev);
	if (rets < 0 && rets != -EINPROGRESS) {
		pm_runtime_put_noidle(dev->dev);
		cl_err(dev, cl, "rpm: get failed %d\n", rets);
		return rets;
	}

	cb->fop_type = MEI_FOP_WRITE;
	cb->buf_idx = 0;
	cl->writing_state = MEI_IDLE;

	mei_hdr.host_addr = cl->host_client_id;
	mei_hdr.me_addr = cl->me_client_id;
	mei_hdr.reserved = 0;
	mei_hdr.msg_complete = 0;
	mei_hdr.internal = cb->internal;

	rets = mei_cl_flow_ctrl_creds(cl);
	if (rets < 0)
		goto err;

	if (rets == 0) {
		cl_dbg(dev, cl, "No flow control credentials: not sending.\n");
		rets = buf->size;
		goto out;
	}
	if (!mei_hbuf_acquire(dev)) {
		cl_dbg(dev, cl, "Cannot acquire the host buffer: not sending.\n");
		rets = buf->size;
		goto out;
	}

	/* Check for a maximum length */
	if (buf->size > mei_hbuf_max_len(dev)) {
		mei_hdr.length = mei_hbuf_max_len(dev);
		mei_hdr.msg_complete = 0;
	} else {
		mei_hdr.length = buf->size;
		mei_hdr.msg_complete = 1;
	}

	rets = mei_write_message(dev, &mei_hdr, buf->data);
	if (rets)
		goto err;

	cl->writing_state = MEI_WRITING;
	cb->buf_idx = mei_hdr.length;

out:
	if (mei_hdr.msg_complete) {
		rets = mei_cl_flow_ctrl_reduce(cl);
		if (rets < 0)
			goto err;

		list_add_tail(&cb->list, &dev->write_waiting_list.list);
	} else {
		list_add_tail(&cb->list, &dev->write_list.list);
	}


	if (blocking && cl->writing_state != MEI_WRITE_COMPLETE) {

		mutex_unlock(&dev->device_lock);
		rets = wait_event_interruptible(cl->tx_wait,
				cl->writing_state == MEI_WRITE_COMPLETE);
		mutex_lock(&dev->device_lock);
		/* wait_event_interruptible returns -ERESTARTSYS */
		if (rets) {
			if (signal_pending(current))
				rets = -EINTR;
			goto err;
		}
	}

	rets = buf->size;
err:
	cl_dbg(dev, cl, "rpm: autosuspend\n");
	pm_runtime_mark_last_busy(dev->dev);
	pm_runtime_put_autosuspend(dev->dev);

	return rets;
}


/**
 * mei_cl_complete - processes completed operation for a client
 *
 * @cl: private data of the file object.
 * @cb: callback block.
 */
void mei_cl_complete(struct mei_cl *cl, struct mei_cl_cb *cb)
{
	if (cb->fop_type == MEI_FOP_WRITE) {
		mei_io_cb_free(cb);
		cb = NULL;
		cl->writing_state = MEI_WRITE_COMPLETE;
		if (waitqueue_active(&cl->tx_wait))
			wake_up_interruptible(&cl->tx_wait);

	} else if (cb->fop_type == MEI_FOP_READ &&
			MEI_READING == cl->reading_state) {
		cl->reading_state = MEI_READ_COMPLETE;
		if (waitqueue_active(&cl->rx_wait))
			wake_up_interruptible(&cl->rx_wait);
		else
			mei_cl_bus_rx_event(cl);

	}
}


/**
 * mei_cl_all_disconnect - disconnect forcefully all connected clients
 *
 * @dev: mei device
 */

void mei_cl_all_disconnect(struct mei_device *dev)
{
	struct mei_cl *cl;

	list_for_each_entry(cl, &dev->file_list, link) {
		cl->state = MEI_FILE_DISCONNECTED;
		cl->mei_flow_ctrl_creds = 0;
		cl->timer_count = 0;
	}
}


/**
 * mei_cl_all_wakeup  - wake up all readers and writers they can be interrupted
 *
 * @dev: mei device
 */
void mei_cl_all_wakeup(struct mei_device *dev)
{
	struct mei_cl *cl;

	list_for_each_entry(cl, &dev->file_list, link) {
		if (waitqueue_active(&cl->rx_wait)) {
			cl_dbg(dev, cl, "Waking up reading client!\n");
			wake_up_interruptible(&cl->rx_wait);
		}
		if (waitqueue_active(&cl->tx_wait)) {
			cl_dbg(dev, cl, "Waking up writing client!\n");
			wake_up_interruptible(&cl->tx_wait);
		}
	}
}

/**
 * mei_cl_all_write_clear - clear all pending writes
 *
 * @dev: mei device
 */
void mei_cl_all_write_clear(struct mei_device *dev)
{
	mei_io_list_free(&dev->write_list, NULL);
	mei_io_list_free(&dev->write_waiting_list, NULL);
}


