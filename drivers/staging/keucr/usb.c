#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/utsname.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>

#include "usb.h"
#include "scsiglue.h"
#include "transport.h"

/* Some informational data */
MODULE_AUTHOR("Domao");
MODULE_DESCRIPTION("ENE USB Mass Storage driver for Linux");
MODULE_LICENSE("GPL");

static unsigned int delay_use = 1;

static struct usb_device_id eucr_usb_ids [] = {
	{ USB_DEVICE(0x058f, 0x6366) },
	{ USB_DEVICE(0x0cf2, 0x6230) },
	{ USB_DEVICE(0x0cf2, 0x6250) },
	{ }                                            /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, eucr_usb_ids);


#ifdef CONFIG_PM

int eucr_suspend(struct usb_interface *iface, pm_message_t message)
{
	struct us_data *us = usb_get_intfdata(iface);
	printk("--- eucr_suspend ---\n");
	/* Wait until no command is running */
	mutex_lock(&us->dev_mutex);

	//US_DEBUGP("%s\n", __func__);
	if (us->suspend_resume_hook)
		(us->suspend_resume_hook)(us, US_SUSPEND);

	/* When runtime PM is working, we'll set a flag to indicate
	 * whether we should autoresume when a SCSI request arrives. */
	// us->Power_IsResum = true;
	//us->SD_Status.Ready = 0;

	mutex_unlock(&us->dev_mutex);
	return 0;
}
//EXPORT_SYMBOL_GPL(eucr_suspend);

int eucr_resume(struct usb_interface *iface)
{
	BYTE    tmp = 0;

	struct us_data *us = usb_get_intfdata(iface);
	printk("--- eucr_resume---\n");
	mutex_lock(&us->dev_mutex);

	//US_DEBUGP("%s\n", __func__);
	if (us->suspend_resume_hook)
		(us->suspend_resume_hook)(us, US_RESUME);


	mutex_unlock(&us->dev_mutex);

	
         us->Power_IsResum = true;
	//
	//us->SD_Status.Ready = 0; //??
	us->SD_Status = *(PSD_STATUS)&tmp;
    	us->MS_Status = *(PMS_STATUS)&tmp;
    	us->SM_Status = *(PSM_STATUS)&tmp;
    	
	return 0;
}
//EXPORT_SYMBOL_GPL(eucr_resume);
int eucr_reset_resume(struct usb_interface *iface)
{
	BYTE    tmp = 0;
	struct us_data *us = usb_get_intfdata(iface);

	printk("--- eucr_reset_resume---\n");
	//US_DEBUGP("%s\n", __func__);

	/* Report the reset to the SCSI core */
	usb_stor_report_bus_reset(us);

	/* FIXME: Notify the subdrivers that they need to reinitialize
	 * the device */
	//ENE_InitMedia(us);
 	us->Power_IsResum = true;
	//
	//us->SD_Status.Ready = 0; //??
	us->SD_Status = *(PSD_STATUS)&tmp;
    	us->MS_Status = *(PMS_STATUS)&tmp;
    	us->SM_Status = *(PSM_STATUS)&tmp;
	return 0;
}
//EXPORT_SYMBOL_GPL(usb_stor_reset_resume);

#else

#define eucr_suspend		NULL
#define eucr_resume		NULL
#define eucr_reset_resume	NULL

#endif

//----- eucr_pre_reset() ---------------------
static int eucr_pre_reset(struct usb_interface *iface)
{
	struct us_data *us = usb_get_intfdata(iface);

      printk("usb --- eucr_pre_reset\n");

	/* Make sure no command runs during the reset */
	mutex_lock(&us->dev_mutex);
	return 0;
}

//----- eucr_post_reset() ---------------------
static int eucr_post_reset(struct usb_interface *iface)
{
	struct us_data *us = usb_get_intfdata(iface);

      printk("usb --- eucr_post_reset\n");

	/* Report the reset to the SCSI core */
	usb_stor_report_bus_reset(us);

	mutex_unlock(&us->dev_mutex);
	return 0;
}

//----- fill_inquiry_response() ---------------------
void fill_inquiry_response(struct us_data *us, unsigned char *data, unsigned int data_len)
{
      printk("usb --- fill_inquiry_response\n");
	if (data_len<36) // You lose.
		return;

	if (data[0]&0x20)
	{
		memset(data+8,0,28);
	}
	else
	{
		u16 bcdDevice = le16_to_cpu(us->pusb_dev->descriptor.bcdDevice);
		memcpy(data+8, us->unusual_dev->vendorName,
			strlen(us->unusual_dev->vendorName) > 8 ? 8 :
			strlen(us->unusual_dev->vendorName));
		memcpy(data+16, us->unusual_dev->productName,
			strlen(us->unusual_dev->productName) > 16 ? 16 :
			strlen(us->unusual_dev->productName));
		data[32] = 0x30 + ((bcdDevice>>12) & 0x0F);
		data[33] = 0x30 + ((bcdDevice>>8) & 0x0F);
		data[34] = 0x30 + ((bcdDevice>>4) & 0x0F);
		data[35] = 0x30 + ((bcdDevice) & 0x0F);
	}
	usb_stor_set_xfer_buf(us, data, data_len, us->srb, TO_XFER_BUF);
}

//----- usb_stor_control_thread() ---------------------
static int usb_stor_control_thread(void * __us)
{
	struct us_data *us = (struct us_data *)__us;
	struct Scsi_Host *host = us_to_host(us);

      printk("usb --- usb_stor_control_thread\n");
	for(;;)
	{
		if (wait_for_completion_interruptible(&us->cmnd_ready))
			break;
			
		/* lock the device pointers */
		mutex_lock(&(us->dev_mutex));

		/* if the device has disconnected, we are free to exit */
		if (test_bit(US_FLIDX_DISCONNECTING, &us->dflags)) {
			mutex_unlock(&us->dev_mutex);
			break;
		}

		/* lock access to the state */
		scsi_lock(host);

		/* When we are called with no command pending, we're done */
		if (us->srb == NULL)
		{
			scsi_unlock(host);
			mutex_unlock(&us->dev_mutex);
			//US_DEBUGP("-- exiting\n");
			break;
		}

		/* has the command timed out *already* ? */
		if (test_bit(US_FLIDX_TIMED_OUT, &us->dflags))
		{
			us->srb->result = DID_ABORT << 16;
			goto SkipForAbort;
		}

		scsi_unlock(host);

		if (us->srb->sc_data_direction == DMA_BIDIRECTIONAL)
		{
			us->srb->result = DID_ERROR << 16;
		}
		else if (us->srb->device->id && !(us->fflags & US_FL_SCM_MULT_TARG))
		{
			us->srb->result = DID_BAD_TARGET << 16;
		}
		else if (us->srb->device->lun > us->max_lun)
		{
			us->srb->result = DID_BAD_TARGET << 16;
		}
		else if ((us->srb->cmnd[0] == INQUIRY) && (us->fflags & US_FL_FIX_INQUIRY))
		{
			unsigned char data_ptr[36] = {0x00, 0x80, 0x02, 0x02, 0x1F, 0x00, 0x00, 0x00};

			fill_inquiry_response(us, data_ptr, 36);
			us->srb->result = SAM_STAT_GOOD;
		}
		else
		{
			us->proto_handler(us->srb, us);
		}

		/* lock access to the state */
		scsi_lock(host);

		/* indicate that the command is done */
		if (us->srb->result != DID_ABORT << 16)
		{
			us->srb->scsi_done(us->srb);
		}
		else
		{
SkipForAbort:
			printk("scsi command aborted\n");
		}

		if (test_bit(US_FLIDX_TIMED_OUT, &us->dflags))
		{
			complete(&(us->notify));

			/* Allow USB transfers to resume */
			clear_bit(US_FLIDX_ABORTING, &us->dflags);
			clear_bit(US_FLIDX_TIMED_OUT, &us->dflags);
		}

		/* finished working on this command */
		us->srb = NULL;
		scsi_unlock(host);

		/* unlock the device pointers */
		mutex_unlock(&us->dev_mutex);
	} /* for (;;) */

	/* Wait until we are told to stop */
	for (;;)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}	

//----- associate_dev() ---------------------
static int associate_dev(struct us_data *us, struct usb_interface *intf)
{
      printk("usb --- associate_dev\n");

	/* Fill in the device-related fields */
	us->pusb_dev = interface_to_usbdev(intf);
	us->pusb_intf = intf;
	us->ifnum = intf->cur_altsetting->desc.bInterfaceNumber;

	/* Store our private data in the interface */
	usb_set_intfdata(intf, us);

	/* Allocate the device-related DMA-mapped buffers */
	us->cr = usb_alloc_coherent(us->pusb_dev, sizeof(*us->cr), GFP_KERNEL, &us->cr_dma);
	if (!us->cr)
	{
		printk("usb_ctrlrequest allocation failed\n");
		return -ENOMEM;
	}

	us->iobuf = usb_alloc_coherent(us->pusb_dev, US_IOBUF_SIZE, GFP_KERNEL, &us->iobuf_dma);
	if (!us->iobuf)
	{
		printk("I/O buffer allocation failed\n");
		return -ENOMEM;
	}

	us->sensebuf = kmalloc(US_SENSE_SIZE, GFP_KERNEL);
	if (!us->sensebuf)
	{
		printk("Sense buffer allocation failed\n");
		return -ENOMEM;
	}
	return 0;
}

//----- get_device_info() ---------------------
static int get_device_info(struct us_data *us, const struct usb_device_id *id)
{
	struct usb_device *dev = us->pusb_dev;
	struct usb_interface_descriptor *idesc = &us->pusb_intf->cur_altsetting->desc;

      printk("usb --- get_device_info\n");

	us->subclass = idesc->bInterfaceSubClass;
	us->protocol = idesc->bInterfaceProtocol;
	us->fflags = USB_US_ORIG_FLAGS(id->driver_info);
	us->Power_IsResum = false;

	if (us->fflags & US_FL_IGNORE_DEVICE)
	{
		printk("device ignored\n");
		return -ENODEV;
	}

	if (dev->speed != USB_SPEED_HIGH)
		us->fflags &= ~US_FL_GO_SLOW;

	return 0;
}

//----- get_transport() ---------------------
static int get_transport(struct us_data *us)
{
      printk("usb --- get_transport\n");
	switch (us->protocol) {
	case USB_PR_BULK:
		us->transport_name = "Bulk";
		us->transport = usb_stor_Bulk_transport;
		us->transport_reset = usb_stor_Bulk_reset;
		break;

	default:
		return -EIO;
	}
	//printk("Transport: %s\n", us->transport_name);

	/* fix for single-lun devices */
	if (us->fflags & US_FL_SINGLE_LUN)
		us->max_lun = 0;
	return 0;
}

//----- get_protocol() ---------------------
static int get_protocol(struct us_data *us)
{
      printk("usb --- get_protocol\n");
	printk("us->pusb_dev->descriptor.idVendor = %x\n", us->pusb_dev->descriptor.idVendor);
	printk("us->pusb_dev->descriptor.idProduct = %x\n", us->pusb_dev->descriptor.idProduct);
	switch (us->subclass) {
	case USB_SC_SCSI:
		us->protocol_name = "Transparent SCSI";
		if( (us->pusb_dev->descriptor.idVendor == 0x0CF2) && (us->pusb_dev->descriptor.idProduct == 0x6250) )
			us->proto_handler = ENE_stor_invoke_transport;
		else
			us->proto_handler = usb_stor_invoke_transport;
		break;

	default:
		return -EIO;
	}
	//printk("Protocol: %s\n", us->protocol_name);
	return 0;
}

//----- get_pipes() ---------------------
static int get_pipes(struct us_data *us)
{
	struct usb_host_interface *altsetting = us->pusb_intf->cur_altsetting;
	int i;
	struct usb_endpoint_descriptor *ep;
	struct usb_endpoint_descriptor *ep_in = NULL;
	struct usb_endpoint_descriptor *ep_out = NULL;
	struct usb_endpoint_descriptor *ep_int = NULL;

      printk("usb --- get_pipes\n");

	for (i = 0; i < altsetting->desc.bNumEndpoints; i++)
	{
		ep = &altsetting->endpoint[i].desc;

		if (usb_endpoint_xfer_bulk(ep))
		{
			if (usb_endpoint_dir_in(ep))
			{
				if (!ep_in)
					ep_in = ep;
			}
			else
			{
				if (!ep_out)
					ep_out = ep;
			}
		}
		else if (usb_endpoint_is_int_in(ep))
		{
			if (!ep_int)
				ep_int = ep;
		}
	}

	if (!ep_in || !ep_out || (us->protocol == USB_PR_CBI && !ep_int))
	{
		printk("Endpoint sanity check failed! Rejecting dev.\n");
		return -EIO;
	}

	/* Calculate and store the pipe values */
	us->send_ctrl_pipe = usb_sndctrlpipe(us->pusb_dev, 0);
	us->recv_ctrl_pipe = usb_rcvctrlpipe(us->pusb_dev, 0);
	us->send_bulk_pipe = usb_sndbulkpipe(us->pusb_dev, ep_out->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	us->recv_bulk_pipe = usb_rcvbulkpipe(us->pusb_dev, ep_in->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
	if (ep_int)
	{
		us->recv_intr_pipe = usb_rcvintpipe(us->pusb_dev, ep_int->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
		us->ep_bInterval = ep_int->bInterval;
	}
	return 0;
}

//----- usb_stor_acquire_resources() ---------------------
static int usb_stor_acquire_resources(struct us_data *us)
{
	struct task_struct *th;

      printk("usb --- usb_stor_acquire_resources\n");
	us->current_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!us->current_urb)
	{
		printk("URB allocation failed\n");
		return -ENOMEM;
	}

	/* Start up our control thread */
	th = kthread_run(usb_stor_control_thread, us, "eucr-storage");
	if (IS_ERR(th))
	{
		printk("Unable to start control thread\n");
		return PTR_ERR(th);
	}
	us->ctl_thread = th;

	return 0;
}

//----- usb_stor_release_resources() ---------------------
static void usb_stor_release_resources(struct us_data *us)
{
	printk("usb --- usb_stor_release_resources\n");

	SM_FreeMem();

	complete(&us->cmnd_ready);
	if (us->ctl_thread)
		kthread_stop(us->ctl_thread);

	/* Call the destructor routine, if it exists */
	if (us->extra_destructor)
	{
		printk("-- calling extra_destructor()\n");
		us->extra_destructor(us->extra);
	}

	/* Free the extra data and the URB */
	kfree(us->extra);
	usb_free_urb(us->current_urb);
}

//----- dissociate_dev() ---------------------
static void dissociate_dev(struct us_data *us)
{
      printk("usb --- dissociate_dev\n");

	kfree(us->sensebuf);

	/* Free the device-related DMA-mapped buffers */
	if (us->cr)
		usb_free_coherent(us->pusb_dev, sizeof(*us->cr), us->cr, us->cr_dma);
	if (us->iobuf)
		usb_free_coherent(us->pusb_dev, US_IOBUF_SIZE, us->iobuf, us->iobuf_dma);

	/* Remove our private data from the interface */
	usb_set_intfdata(us->pusb_intf, NULL);
}

//----- quiesce_and_remove_host() ---------------------
static void quiesce_and_remove_host(struct us_data *us)
{
	struct Scsi_Host *host = us_to_host(us);

      printk("usb --- quiesce_and_remove_host\n");

	/* If the device is really gone, cut short reset delays */
	if (us->pusb_dev->state == USB_STATE_NOTATTACHED)
		set_bit(US_FLIDX_DISCONNECTING, &us->dflags);

	/* Prevent SCSI-scanning (if it hasn't started yet)
	 * and wait for the SCSI-scanning thread to stop.
	 */
	set_bit(US_FLIDX_DONT_SCAN, &us->dflags);
	wake_up(&us->delay_wait);
	wait_for_completion(&us->scanning_done);

	/* Removing the host will perform an orderly shutdown: caches
	 * synchronized, disks spun down, etc.
	 */
	scsi_remove_host(host);

	/* Prevent any new commands from being accepted and cut short
	 * reset delays.
	 */
	scsi_lock(host);
	set_bit(US_FLIDX_DISCONNECTING, &us->dflags);
	scsi_unlock(host);
	wake_up(&us->delay_wait);
}

//----- release_everything() ---------------------
static void release_everything(struct us_data *us)
{
      printk("usb --- release_everything\n");

	usb_stor_release_resources(us);
	dissociate_dev(us);
	scsi_host_put(us_to_host(us));
}

//----- usb_stor_scan_thread() ---------------------
static int usb_stor_scan_thread(void * __us)
{
	struct us_data *us = (struct us_data *)__us;

      printk("usb --- usb_stor_scan_thread\n");
	printk("EUCR : device found at %d\n", us->pusb_dev->devnum);

	set_freezable();
	/* Wait for the timeout to expire or for a disconnect */
	if (delay_use > 0) {
		wait_event_freezable_timeout(us->delay_wait,
				test_bit(US_FLIDX_DONT_SCAN, &us->dflags),
				delay_use * HZ);
	}

	/* If the device is still connected, perform the scanning */
	if (!test_bit(US_FLIDX_DONT_SCAN, &us->dflags))
	{
		/* For bulk-only devices, determine the max LUN value */
		if (us->protocol == USB_PR_BULK && !(us->fflags & US_FL_SINGLE_LUN))
		{
			mutex_lock(&us->dev_mutex);
			us->max_lun = usb_stor_Bulk_max_lun(us);
			mutex_unlock(&us->dev_mutex);
		}
		scsi_scan_host(us_to_host(us));
		printk("EUCR : device scan complete\n");
	}
	complete_and_exit(&us->scanning_done, 0);
}

//----- eucr_probe() ---------------------
static int eucr_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct Scsi_Host *host;
	struct us_data *us;
	int result;
	struct task_struct *th;

      printk("usb --- eucr_probe\n");

      host = scsi_host_alloc(&usb_stor_host_template, sizeof(*us));
	if (!host)
	{
		printk("Unable to allocate the scsi host\n");
		return -ENOMEM;
	}

	/* Allow 16-byte CDBs and thus > 2TB */
	host->max_cmd_len = 16;
	us = host_to_us(host);
	memset(us, 0, sizeof(struct us_data));
	mutex_init(&(us->dev_mutex));
	init_completion(&us->cmnd_ready);
	init_completion(&(us->notify));
	init_waitqueue_head(&us->delay_wait);
	init_completion(&us->scanning_done);

	/* Associate the us_data structure with the USB device */
	result = associate_dev(us, intf);
	if (result)
		goto BadDevice;

	/* Get Device info */
	result = get_device_info(us, id);
	if (result)
		goto BadDevice;

	/* Get the transport, protocol, and pipe settings */
	result = get_transport(us);
	if (result)
		goto BadDevice;
	result = get_protocol(us);
	if (result)
		goto BadDevice;
	result = get_pipes(us);
	if (result)
		goto BadDevice;

	/* Acquire all the other resources and add the host */
	result = usb_stor_acquire_resources(us);
	if (result)
		goto BadDevice;

	result = scsi_add_host(host, &intf->dev);
	if (result)
	{
		printk("Unable to add the scsi host\n");
		goto BadDevice;
	}

	/* Start up the thread for delayed SCSI-device scanning */
	th = kthread_create(usb_stor_scan_thread, us, "eucr-stor-scan");
	if (IS_ERR(th))
	{
		printk("Unable to start the device-scanning thread\n");
		complete(&us->scanning_done);
		quiesce_and_remove_host(us);
		result = PTR_ERR(th);
		goto BadDevice;
	}
	wake_up_process(th);
	return 0;

	/* We come here if there are any problems */
BadDevice:
      printk("usb --- eucr_probe failed\n");
	release_everything(us);
	return result;
}

//----- eucr_disconnect() ---------------------
static void eucr_disconnect(struct usb_interface *intf)
{
	struct us_data *us = usb_get_intfdata(intf);

      printk("usb --- eucr_disconnect\n");
	quiesce_and_remove_host(us);
	release_everything(us);
}

/***********************************************************************
 * Initialization and registration
 ***********************************************************************/

//----- usb_storage_driver() ---------------------
static struct usb_driver usb_storage_driver = {
	.name =		"eucr",
	.probe =		eucr_probe,
    	.suspend =	    eucr_suspend,
	.resume =	    eucr_resume,
    	.reset_resume =	eucr_reset_resume,
	.disconnect =	eucr_disconnect,
	.pre_reset =	eucr_pre_reset,
	.post_reset =	eucr_post_reset,
	.id_table =		eucr_usb_ids,
	.soft_unbind =	1,
};

//----- usb_stor_init() ---------------------
static int __init usb_stor_init(void)
{
	int retval;
      printk("usb --- usb_stor_init start\n");

	retval = usb_register(&usb_storage_driver);
	if (retval == 0)
            printk("ENE USB Mass Storage support registered.\n");

	return retval;
}

//----- usb_stor_exit() ---------------------
static void __exit usb_stor_exit(void)
{
      printk("usb --- usb_stor_exit\n");

	usb_deregister(&usb_storage_driver) ;
}

module_init(usb_stor_init);
module_exit(usb_stor_exit);
