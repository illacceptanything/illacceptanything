/*
 * Line6 Linux USB driver - 0.9.1beta
 *
 * Copyright (C) 2004-2010 Markus Grabner (grabner@icg.tugraz.at)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#include "audio.h"
#include "capture.h"
#include "control.h"
#include "driver.h"
#include "midi.h"
#include "playback.h"
#include "pod.h"
#include "revision.h"
#include "toneport.h"
#include "usbdefs.h"
#include "variax.h"

#define DRIVER_AUTHOR  "Markus Grabner <grabner@icg.tugraz.at>"
#define DRIVER_DESC    "Line6 USB Driver"
#define DRIVER_VERSION "0.9.1beta" DRIVER_REVISION

/* table of devices that work with this driver */
static const struct usb_device_id line6_id_table[] = {
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_BASSPODXT)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_BASSPODXTLIVE)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_BASSPODXTPRO)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_GUITARPORT)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_POCKETPOD)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_PODSTUDIO_GX)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_PODSTUDIO_UX1)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_PODSTUDIO_UX2)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_PODX3)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_PODX3LIVE)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_PODXT)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_PODXTLIVE)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_PODXTPRO)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_TONEPORT_GX)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_TONEPORT_UX1)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_TONEPORT_UX2)},
	{USB_DEVICE(LINE6_VENDOR_ID, LINE6_DEVID_VARIAX)},
	{},
};

MODULE_DEVICE_TABLE(usb, line6_id_table);

/* *INDENT-OFF* */
static struct line6_properties line6_properties_table[] = {
	{ "BassPODxt",     "BassPODxt",        LINE6_BIT_BASSPODXT,     LINE6_BIT_CONTROL_PCM_HWMON },
	{ "BassPODxtLive", "BassPODxt Live",   LINE6_BIT_BASSPODXTLIVE, LINE6_BIT_CONTROL_PCM_HWMON },
	{ "BassPODxtPro",  "BassPODxt Pro",    LINE6_BIT_BASSPODXTPRO,  LINE6_BIT_CONTROL_PCM_HWMON },
	{ "GuitarPort",    "GuitarPort",       LINE6_BIT_GUITARPORT,    LINE6_BIT_PCM               },
	{ "PocketPOD",     "Pocket POD",       LINE6_BIT_POCKETPOD,     LINE6_BIT_CONTROL           },
	{ "PODStudioGX",   "POD Studio GX",    LINE6_BIT_PODSTUDIO_GX,  LINE6_BIT_PCM               },
	{ "PODStudioUX1",  "POD Studio UX1",   LINE6_BIT_PODSTUDIO_UX1, LINE6_BIT_PCM               },
	{ "PODStudioUX2",  "POD Studio UX2",   LINE6_BIT_PODSTUDIO_UX2, LINE6_BIT_PCM               },
	{ "PODX3",         "POD X3",           LINE6_BIT_PODX3,         LINE6_BIT_PCM               },
	{ "PODX3Live",     "POD X3 Live",      LINE6_BIT_PODX3LIVE,     LINE6_BIT_PCM               },
	{ "PODxt",         "PODxt",            LINE6_BIT_PODXT,         LINE6_BIT_CONTROL_PCM_HWMON },
	{ "PODxtLive",     "PODxt Live",       LINE6_BIT_PODXTLIVE,     LINE6_BIT_CONTROL_PCM_HWMON },
	{ "PODxtPro",      "PODxt Pro",        LINE6_BIT_PODXTPRO,      LINE6_BIT_CONTROL_PCM_HWMON },
	{ "TonePortGX",    "TonePort GX",      LINE6_BIT_TONEPORT_GX,   LINE6_BIT_PCM               },
	{ "TonePortUX1",   "TonePort UX1",     LINE6_BIT_TONEPORT_UX1,  LINE6_BIT_PCM               },
	{ "TonePortUX2",   "TonePort UX2",     LINE6_BIT_TONEPORT_UX2,  LINE6_BIT_PCM               },
	{ "Variax",        "Variax Workbench", LINE6_BIT_VARIAX,        LINE6_BIT_CONTROL           }
};
/* *INDENT-ON* */

/*
	This is Line6's MIDI manufacturer ID.
*/
const unsigned char line6_midi_id[] = {
	0x00, 0x01, 0x0c
};

/*
	Code to request version of POD, Variax interface
	(and maybe other devices).
*/
static const char line6_request_version0[] = {
	0xf0, 0x7e, 0x7f, 0x06, 0x01, 0xf7
};

/*
	Copy of version request code with GFP_KERNEL flag for use in URB.
*/
static const char *line6_request_version;

struct usb_line6 *line6_devices[LINE6_MAX_DEVICES];

/**
	 Class for asynchronous messages.
*/
struct message {
	struct usb_line6 *line6;
	const char *buffer;
	int size;
	int done;
};

/*
	Forward declarations.
*/
static void line6_data_received(struct urb *urb);
static int line6_send_raw_message_async_part(struct message *msg,
					     struct urb *urb);

/*
	Start to listen on endpoint.
*/
static int line6_start_listen(struct usb_line6 *line6)
{
	int err;
	usb_fill_int_urb(line6->urb_listen, line6->usbdev,
			 usb_rcvintpipe(line6->usbdev, line6->ep_control_read),
			 line6->buffer_listen, LINE6_BUFSIZE_LISTEN,
			 line6_data_received, line6, line6->interval);
	line6->urb_listen->actual_length = 0;
	err = usb_submit_urb(line6->urb_listen, GFP_ATOMIC);
	return err;
}

/*
	Stop listening on endpoint.
*/
static void line6_stop_listen(struct usb_line6 *line6)
{
	usb_kill_urb(line6->urb_listen);
}

#ifdef CONFIG_LINE6_USB_DUMP_ANY
/*
	Write hexdump to syslog.
*/
void line6_write_hexdump(struct usb_line6 *line6, char dir,
			 const unsigned char *buffer, int size)
{
	static const int BYTES_PER_LINE = 8;
	char hexdump[100];
	char asc[BYTES_PER_LINE + 1];
	int i, j;

	for (i = 0; i < size; i += BYTES_PER_LINE) {
		int hexdumpsize = sizeof(hexdump);
		char *p = hexdump;
		int n = min(size - i, BYTES_PER_LINE);
		asc[n] = 0;

		for (j = 0; j < BYTES_PER_LINE; ++j) {
			int bytes;

			if (j < n) {
				unsigned char val = buffer[i + j];
				bytes = snprintf(p, hexdumpsize, " %02X", val);
				asc[j] = ((val >= 0x20)
					  && (val < 0x7f)) ? val : '.';
			} else
				bytes = snprintf(p, hexdumpsize, "   ");

			if (bytes > hexdumpsize)
				break;	/* buffer overflow */

			p += bytes;
			hexdumpsize -= bytes;
		}

		dev_info(line6->ifcdev, "%c%04X:%s %s\n", dir, i, hexdump, asc);
	}
}
#endif

#ifdef CONFIG_LINE6_USB_DUMP_CTRL
/*
	Dump URB data to syslog.
*/
static void line6_dump_urb(struct urb *urb)
{
	struct usb_line6 *line6 = (struct usb_line6 *)urb->context;

	if (urb->status < 0)
		return;

	line6_write_hexdump(line6, 'R', (unsigned char *)urb->transfer_buffer,
			    urb->actual_length);
}
#endif

/*
	Send raw message in pieces of wMaxPacketSize bytes.
*/
int line6_send_raw_message(struct usb_line6 *line6, const char *buffer,
			   int size)
{
	int i, done = 0;

#ifdef CONFIG_LINE6_USB_DUMP_CTRL
	line6_write_hexdump(line6, 'S', buffer, size);
#endif

	for (i = 0; i < size; i += line6->max_packet_size) {
		int partial;
		const char *frag_buf = buffer + i;
		int frag_size = min(line6->max_packet_size, size - i);
		int retval;

		retval = usb_interrupt_msg(line6->usbdev,
					   usb_sndintpipe(line6->usbdev,
							  line6->ep_control_write),
					   (char *)frag_buf, frag_size,
					   &partial, LINE6_TIMEOUT * HZ);

		if (retval) {
			dev_err(line6->ifcdev,
				"usb_interrupt_msg failed (%d)\n", retval);
			break;
		}

		done += frag_size;
	}

	return done;
}

/*
	Notification of completion of asynchronous request transmission.
*/
static void line6_async_request_sent(struct urb *urb)
{
	struct message *msg = (struct message *)urb->context;

	if (msg->done >= msg->size) {
		usb_free_urb(urb);
		kfree(msg);
	} else
		line6_send_raw_message_async_part(msg, urb);
}

/*
	Asynchronously send part of a raw message.
*/
static int line6_send_raw_message_async_part(struct message *msg,
					     struct urb *urb)
{
	int retval;
	struct usb_line6 *line6 = msg->line6;
	int done = msg->done;
	int bytes = min(msg->size - done, line6->max_packet_size);

	usb_fill_int_urb(urb, line6->usbdev,
			 usb_sndintpipe(line6->usbdev, line6->ep_control_write),
			 (char *)msg->buffer + done, bytes,
			 line6_async_request_sent, msg, line6->interval);

#ifdef CONFIG_LINE6_USB_DUMP_CTRL
	line6_write_hexdump(line6, 'S', (char *)msg->buffer + done, bytes);
#endif

	msg->done += bytes;
	retval = usb_submit_urb(urb, GFP_ATOMIC);

	if (retval < 0) {
		dev_err(line6->ifcdev, "%s: usb_submit_urb failed (%d)\n",
			__func__, retval);
		usb_free_urb(urb);
		kfree(msg);
		return -EINVAL;
	}

	return 0;
}

/*
	Setup and start timer.
*/
void line6_start_timer(struct timer_list *timer, unsigned int msecs,
		       void (*function) (unsigned long), unsigned long data)
{
	setup_timer(timer, function, data);
	timer->expires = jiffies + msecs * HZ / 1000;
	add_timer(timer);
}

/*
	Asynchronously send raw message.
*/
int line6_send_raw_message_async(struct usb_line6 *line6, const char *buffer,
				 int size)
{
	struct message *msg;
	struct urb *urb;

	/* create message: */
	msg = kmalloc(sizeof(struct message), GFP_ATOMIC);

	if (msg == NULL) {
		dev_err(line6->ifcdev, "Out of memory\n");
		return -ENOMEM;
	}

	/* create URB: */
	urb = usb_alloc_urb(0, GFP_ATOMIC);

	if (urb == NULL) {
		kfree(msg);
		dev_err(line6->ifcdev, "Out of memory\n");
		return -ENOMEM;
	}

	/* set message data: */
	msg->line6 = line6;
	msg->buffer = buffer;
	msg->size = size;
	msg->done = 0;

	/* start sending: */
	return line6_send_raw_message_async_part(msg, urb);
}

/*
	Send asynchronous device version request.
*/
int line6_version_request_async(struct usb_line6 *line6)
{
	return line6_send_raw_message_async(line6, line6_request_version,
					    sizeof(line6_request_version0));
}

/*
	Send sysex message in pieces of wMaxPacketSize bytes.
*/
int line6_send_sysex_message(struct usb_line6 *line6, const char *buffer,
			     int size)
{
	return line6_send_raw_message(line6, buffer,
				      size + SYSEX_EXTRA_SIZE) -
	    SYSEX_EXTRA_SIZE;
}

/*
	Send sysex message in pieces of wMaxPacketSize bytes.
*/
int line6_send_sysex_message_async(struct usb_line6 *line6, const char *buffer,
				   int size)
{
	return line6_send_raw_message_async(line6, buffer,
					    size + SYSEX_EXTRA_SIZE) -
	    SYSEX_EXTRA_SIZE;
}

/*
	Allocate buffer for sysex message and prepare header.
	@param code sysex message code
	@param size number of bytes between code and sysex end
*/
char *line6_alloc_sysex_buffer(struct usb_line6 *line6, int code1, int code2,
			       int size)
{
	char *buffer = kmalloc(size + SYSEX_EXTRA_SIZE, GFP_ATOMIC);

	if (!buffer) {
		dev_err(line6->ifcdev, "out of memory\n");
		return NULL;
	}

	buffer[0] = LINE6_SYSEX_BEGIN;
	memcpy(buffer + 1, line6_midi_id, sizeof(line6_midi_id));
	buffer[sizeof(line6_midi_id) + 1] = code1;
	buffer[sizeof(line6_midi_id) + 2] = code2;
	buffer[sizeof(line6_midi_id) + 3 + size] = LINE6_SYSEX_END;
	return buffer;
}

/*
	Notification of data received from the Line6 device.
*/
static void line6_data_received(struct urb *urb)
{
	struct usb_line6 *line6 = (struct usb_line6 *)urb->context;
	struct MidiBuffer *mb = &line6->line6midi->midibuf_in;
	int done;

	if (urb->status == -ESHUTDOWN)
		return;

#ifdef CONFIG_LINE6_USB_DUMP_CTRL
	line6_dump_urb(urb);
#endif

	done =
	    line6_midibuf_write(mb, urb->transfer_buffer, urb->actual_length);

	if (done < urb->actual_length) {
		line6_midibuf_ignore(mb, done);
		DEBUG_MESSAGES(dev_err
			       (line6->ifcdev,
				"%d %d buffer overflow - message skipped\n",
				done, urb->actual_length));
	}

	for (;;) {
		done =
		    line6_midibuf_read(mb, line6->buffer_message,
				       LINE6_MESSAGE_MAXLEN);

		if (done == 0)
			break;

		/* MIDI input filter */
		if (line6_midibuf_skip_message
		    (mb, line6->line6midi->midi_mask_receive))
			continue;

		line6->message_length = done;
#ifdef CONFIG_LINE6_USB_DUMP_MIDI
		line6_write_hexdump(line6, 'r', line6->buffer_message, done);
#endif
		line6_midi_receive(line6, line6->buffer_message, done);

		switch (line6->usbdev->descriptor.idProduct) {
		case LINE6_DEVID_BASSPODXT:
		case LINE6_DEVID_BASSPODXTLIVE:
		case LINE6_DEVID_BASSPODXTPRO:
		case LINE6_DEVID_PODXT:
		case LINE6_DEVID_PODXTPRO:
		case LINE6_DEVID_POCKETPOD:
			line6_pod_process_message((struct usb_line6_pod *)
						  line6);
			break;

		case LINE6_DEVID_PODXTLIVE:
			switch (line6->interface_number) {
			case PODXTLIVE_INTERFACE_POD:
				line6_pod_process_message((struct usb_line6_pod
							   *)line6);
				break;

			case PODXTLIVE_INTERFACE_VARIAX:
				line6_variax_process_message((struct
							      usb_line6_variax
							      *)line6);
				break;

			default:
				dev_err(line6->ifcdev,
					"PODxt Live interface %d not supported\n",
					line6->interface_number);
			}
			break;

		case LINE6_DEVID_VARIAX:
			line6_variax_process_message((struct usb_line6_variax *)
						     line6);
			break;

		default:
			MISSING_CASE;
		}
	}

	line6_start_listen(line6);
}

/*
	Send channel number (i.e., switch to a different sound).
*/
int line6_send_program(struct usb_line6 *line6, int value)
{
	int retval;
	unsigned char *buffer;
	int partial;

	buffer = kmalloc(2, GFP_KERNEL);

	if (!buffer) {
		dev_err(line6->ifcdev, "out of memory\n");
		return -ENOMEM;
	}

	buffer[0] = LINE6_PROGRAM_CHANGE | LINE6_CHANNEL_HOST;
	buffer[1] = value;

#ifdef CONFIG_LINE6_USB_DUMP_CTRL
	line6_write_hexdump(line6, 'S', buffer, 2);
#endif

	retval = usb_interrupt_msg(line6->usbdev,
				   usb_sndintpipe(line6->usbdev,
						  line6->ep_control_write),
				   buffer, 2, &partial, LINE6_TIMEOUT * HZ);

	if (retval)
		dev_err(line6->ifcdev, "usb_interrupt_msg failed (%d)\n",
			retval);

	kfree(buffer);
	return retval;
}

/*
	Transmit Line6 control parameter.
*/
int line6_transmit_parameter(struct usb_line6 *line6, int param, int value)
{
	int retval;
	unsigned char *buffer;
	int partial;

	buffer = kmalloc(3, GFP_KERNEL);

	if (!buffer) {
		dev_err(line6->ifcdev, "out of memory\n");
		return -ENOMEM;
	}

	buffer[0] = LINE6_PARAM_CHANGE | LINE6_CHANNEL_HOST;
	buffer[1] = param;
	buffer[2] = value;

#ifdef CONFIG_LINE6_USB_DUMP_CTRL
	line6_write_hexdump(line6, 'S', buffer, 3);
#endif

	retval = usb_interrupt_msg(line6->usbdev,
				   usb_sndintpipe(line6->usbdev,
						  line6->ep_control_write),
				   buffer, 3, &partial, LINE6_TIMEOUT * HZ);

	if (retval)
		dev_err(line6->ifcdev, "usb_interrupt_msg failed (%d)\n",
			retval);

	kfree(buffer);
	return retval;
}

/*
	Read data from device.
*/
int line6_read_data(struct usb_line6 *line6, int address, void *data,
		    size_t datalen)
{
	struct usb_device *usbdev = line6->usbdev;
	int ret;
	unsigned char len;

	/* query the serial number: */
	ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0), 0x67,
			      USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
			      (datalen << 8) | 0x21, address,
			      NULL, 0, LINE6_TIMEOUT * HZ);

	if (ret < 0) {
		dev_err(line6->ifcdev, "read request failed (error %d)\n", ret);
		return ret;
	}

	/* Wait for data length. We'll get a couple of 0xff until length arrives. */
	do {
		ret = usb_control_msg(usbdev, usb_rcvctrlpipe(usbdev, 0), 0x67,
				      USB_TYPE_VENDOR | USB_RECIP_DEVICE |
				      USB_DIR_IN,
				      0x0012, 0x0000, &len, 1,
				      LINE6_TIMEOUT * HZ);
		if (ret < 0) {
			dev_err(line6->ifcdev,
				"receive length failed (error %d)\n", ret);
			return ret;
		}
	} while (len == 0xff);

	if (len != datalen) {
		/* should be equal or something went wrong */
		dev_err(line6->ifcdev,
			"length mismatch (expected %d, got %d)\n",
			(int)datalen, (int)len);
		return -EINVAL;
	}

	/* receive the result: */
	ret = usb_control_msg(usbdev, usb_rcvctrlpipe(usbdev, 0), 0x67,
			      USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
			      0x0013, 0x0000, data, datalen,
			      LINE6_TIMEOUT * HZ);

	if (ret < 0) {
		dev_err(line6->ifcdev, "read failed (error %d)\n", ret);
		return ret;
	}

	return 0;
}

/*
	Write data to device.
*/
int line6_write_data(struct usb_line6 *line6, int address, void *data,
		     size_t datalen)
{
	struct usb_device *usbdev = line6->usbdev;
	int ret;
	unsigned char status;

	ret = usb_control_msg(usbdev, usb_sndctrlpipe(usbdev, 0), 0x67,
			      USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
			      0x0022, address, data, datalen,
			      LINE6_TIMEOUT * HZ);

	if (ret < 0) {
		dev_err(line6->ifcdev,
			"write request failed (error %d)\n", ret);
		return ret;
	}

	do {
		ret = usb_control_msg(usbdev, usb_rcvctrlpipe(usbdev, 0),
				      0x67,
				      USB_TYPE_VENDOR | USB_RECIP_DEVICE |
				      USB_DIR_IN,
				      0x0012, 0x0000,
				      &status, 1, LINE6_TIMEOUT * HZ);

		if (ret < 0) {
			dev_err(line6->ifcdev,
				"receiving status failed (error %d)\n", ret);
			return ret;
		}
	} while (status == 0xff);

	if (status != 0) {
		dev_err(line6->ifcdev, "write failed (error %d)\n", ret);
		return -EINVAL;
	}

	return 0;
}

/*
	Read Line6 device serial number.
	(POD, TonePort, GuitarPort)
*/
int line6_read_serial_number(struct usb_line6 *line6, int *serial_number)
{
	return line6_read_data(line6, 0x80d0, serial_number,
			       sizeof(*serial_number));
}

/*
	No operation (i.e., unsupported).
*/
ssize_t line6_nop_read(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	return 0;
}

/*
	No operation (i.e., unsupported).
*/
ssize_t line6_nop_write(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	return count;
}

/*
	"write" request on "raw" special file.
*/
#ifdef CONFIG_LINE6_USB_RAW
ssize_t line6_set_raw(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t count)
{
	struct usb_interface *interface = to_usb_interface(dev);
	struct usb_line6 *line6 = usb_get_intfdata(interface);
	line6_send_raw_message(line6, buf, count);
	return count;
}
#endif

/*
	Generic destructor.
*/
static void line6_destruct(struct usb_interface *interface)
{
	struct usb_line6 *line6;

	if (interface == NULL)
		return;
	line6 = usb_get_intfdata(interface);
	if (line6 == NULL)
		return;

	/* free buffer memory first: */
	kfree(line6->buffer_message);
	kfree(line6->buffer_listen);

	/* then free URBs: */
	usb_free_urb(line6->urb_listen);

	/* make sure the device isn't destructed twice: */
	usb_set_intfdata(interface, NULL);

	/* free interface data: */
	kfree(line6);
}

/*
	Probe USB device.
*/
static int line6_probe(struct usb_interface *interface,
		       const struct usb_device_id *id)
{
	int devtype;
	struct usb_device *usbdev = NULL;
	struct usb_line6 *line6 = NULL;
	const struct line6_properties *properties;
	int devnum;
	int interface_number, alternate = 0;
	int product;
	int size = 0;
	int ep_read = 0, ep_write = 0;
	int ret;

	if (interface == NULL)
		return -ENODEV;
	usbdev = interface_to_usbdev(interface);
	if (usbdev == NULL)
		return -ENODEV;

	/* we don't handle multiple configurations */
	if (usbdev->descriptor.bNumConfigurations != 1) {
		ret = -ENODEV;
		goto err_put;
	}

	/* check vendor and product id */
	for (devtype = ARRAY_SIZE(line6_id_table) - 1; devtype--;) {
		u16 idVendor = le16_to_cpu(usbdev->descriptor.idVendor);
		u16 idProduct = le16_to_cpu(usbdev->descriptor.idProduct);

		if (idVendor == line6_id_table[devtype].idVendor &&
		    idProduct == line6_id_table[devtype].idProduct)
			break;
	}

	if (devtype < 0) {
		ret = -ENODEV;
		goto err_put;
	}

	/* find free slot in device table: */
	for (devnum = 0; devnum < LINE6_MAX_DEVICES; ++devnum)
		if (line6_devices[devnum] == NULL)
			break;

	if (devnum == LINE6_MAX_DEVICES) {
		ret = -ENODEV;
		goto err_put;
	}

	/* initialize device info: */
	properties = &line6_properties_table[devtype];
	dev_info(&interface->dev, "Line6 %s found\n", properties->name);
	product = le16_to_cpu(usbdev->descriptor.idProduct);

	/* query interface number */
	interface_number = interface->cur_altsetting->desc.bInterfaceNumber;

	switch (product) {
	case LINE6_DEVID_BASSPODXTLIVE:
	case LINE6_DEVID_PODXTLIVE:
	case LINE6_DEVID_VARIAX:
		alternate = 1;
		break;

	case LINE6_DEVID_POCKETPOD:
		switch (interface_number) {
		case 0:
			return 0;	/* this interface has no endpoints */
		case 1:
			alternate = 0;
			break;
		default:
			MISSING_CASE;
		}
		break;

	case LINE6_DEVID_PODX3:
	case LINE6_DEVID_PODX3LIVE:
		switch (interface_number) {
		case 0:
			alternate = 1;
			break;
		case 1:
			alternate = 0;
			break;
		default:
			MISSING_CASE;
		}
		break;

	case LINE6_DEVID_BASSPODXT:
	case LINE6_DEVID_BASSPODXTPRO:
	case LINE6_DEVID_PODXT:
	case LINE6_DEVID_PODXTPRO:
		alternate = 5;
		break;

	case LINE6_DEVID_GUITARPORT:
	case LINE6_DEVID_PODSTUDIO_GX:
	case LINE6_DEVID_PODSTUDIO_UX1:
	case LINE6_DEVID_TONEPORT_GX:
	case LINE6_DEVID_TONEPORT_UX1:
		alternate = 2;	/* 1..4 seem to be ok */
		break;

	case LINE6_DEVID_TONEPORT_UX2:
	case LINE6_DEVID_PODSTUDIO_UX2:
		switch (interface_number) {
		case 0:
			/* defaults to 44.1kHz, 16-bit */
			alternate = 2;
			break;
		case 1:
			/* don't know yet what this is ...
			   alternate = 1;
			   break;
			 */
			return -ENODEV;
		default:
			MISSING_CASE;
		}
		break;

	default:
		MISSING_CASE;
		ret = -ENODEV;
		goto err_put;
	}

	ret = usb_set_interface(usbdev, interface_number, alternate);
	if (ret < 0) {
		dev_err(&interface->dev, "set_interface failed\n");
		goto err_put;
	}

	/* initialize device data based on product id: */
	switch (product) {
	case LINE6_DEVID_BASSPODXT:
	case LINE6_DEVID_BASSPODXTLIVE:
	case LINE6_DEVID_BASSPODXTPRO:
	case LINE6_DEVID_PODXT:
	case LINE6_DEVID_PODXTPRO:
		size = sizeof(struct usb_line6_pod);
		ep_read = 0x84;
		ep_write = 0x03;
		break;

	case LINE6_DEVID_POCKETPOD:
		size = sizeof(struct usb_line6_pod);
		ep_read = 0x82;
		ep_write = 0x02;
		break;

	case LINE6_DEVID_PODX3:
	case LINE6_DEVID_PODX3LIVE:
		/* currently unused! */
		size = sizeof(struct usb_line6_pod);
		ep_read = 0x81;
		ep_write = 0x01;
		break;

	case LINE6_DEVID_PODSTUDIO_GX:
	case LINE6_DEVID_PODSTUDIO_UX1:
	case LINE6_DEVID_PODSTUDIO_UX2:
	case LINE6_DEVID_TONEPORT_GX:
	case LINE6_DEVID_TONEPORT_UX1:
	case LINE6_DEVID_TONEPORT_UX2:
	case LINE6_DEVID_GUITARPORT:
		size = sizeof(struct usb_line6_toneport);
		/* these don't have a control channel */
		break;

	case LINE6_DEVID_PODXTLIVE:
		switch (interface_number) {
		case PODXTLIVE_INTERFACE_POD:
			size = sizeof(struct usb_line6_pod);
			ep_read = 0x84;
			ep_write = 0x03;
			break;

		case PODXTLIVE_INTERFACE_VARIAX:
			size = sizeof(struct usb_line6_variax);
			ep_read = 0x86;
			ep_write = 0x05;
			break;

		default:
			ret = -ENODEV;
			goto err_put;
		}
		break;

	case LINE6_DEVID_VARIAX:
		size = sizeof(struct usb_line6_variax);
		ep_read = 0x82;
		ep_write = 0x01;
		break;

	default:
		MISSING_CASE;
		ret = -ENODEV;
		goto err_put;
	}

	if (size == 0) {
		dev_err(line6->ifcdev,
			"driver bug: interface data size not set\n");
		ret = -ENODEV;
		goto err_put;
	}

	line6 = kzalloc(size, GFP_KERNEL);

	if (line6 == NULL) {
		dev_err(&interface->dev, "Out of memory\n");
		ret = -ENODEV;
		goto err_put;
	}

	/* store basic data: */
	line6->interface_number = interface_number;
	line6->properties = properties;
	line6->usbdev = usbdev;
	line6->ifcdev = &interface->dev;
	line6->ep_control_read = ep_read;
	line6->ep_control_write = ep_write;
	line6->product = product;

	/* get data from endpoint descriptor (see usb_maxpacket): */
	{
		struct usb_host_endpoint *ep;
		unsigned epnum =
		    usb_pipeendpoint(usb_rcvintpipe(usbdev, ep_read));
		ep = usbdev->ep_in[epnum];

		if (ep != NULL) {
			line6->interval = ep->desc.bInterval;
			line6->max_packet_size =
			    le16_to_cpu(ep->desc.wMaxPacketSize);
		} else {
			line6->interval = LINE6_FALLBACK_INTERVAL;
			line6->max_packet_size = LINE6_FALLBACK_MAXPACKETSIZE;
			dev_err(line6->ifcdev,
				"endpoint not available, using fallback values");
		}
	}

	usb_set_intfdata(interface, line6);

	if (properties->capabilities & LINE6_BIT_CONTROL) {
		/* initialize USB buffers: */
		line6->buffer_listen =
		    kmalloc(LINE6_BUFSIZE_LISTEN, GFP_KERNEL);

		if (line6->buffer_listen == NULL) {
			dev_err(&interface->dev, "Out of memory\n");
			ret = -ENOMEM;
			goto err_destruct;
		}

		line6->buffer_message =
		    kmalloc(LINE6_MESSAGE_MAXLEN, GFP_KERNEL);

		if (line6->buffer_message == NULL) {
			dev_err(&interface->dev, "Out of memory\n");
			ret = -ENOMEM;
			goto err_destruct;
		}

		line6->urb_listen = usb_alloc_urb(0, GFP_KERNEL);

		if (line6->urb_listen == NULL) {
			dev_err(&interface->dev, "Out of memory\n");
			line6_destruct(interface);
			ret = -ENOMEM;
			goto err_destruct;
		}

		ret = line6_start_listen(line6);
		if (ret < 0) {
			dev_err(&interface->dev, "%s: usb_submit_urb failed\n",
				__func__);
			goto err_destruct;
		}
	}

	/* initialize device data based on product id: */
	switch (product) {
	case LINE6_DEVID_BASSPODXT:
	case LINE6_DEVID_BASSPODXTLIVE:
	case LINE6_DEVID_BASSPODXTPRO:
	case LINE6_DEVID_POCKETPOD:
	case LINE6_DEVID_PODX3:
	case LINE6_DEVID_PODX3LIVE:
	case LINE6_DEVID_PODXT:
	case LINE6_DEVID_PODXTPRO:
		ret = line6_pod_init(interface, (struct usb_line6_pod *)line6);
		break;

	case LINE6_DEVID_PODXTLIVE:
		switch (interface_number) {
		case PODXTLIVE_INTERFACE_POD:
			ret =
			    line6_pod_init(interface,
					   (struct usb_line6_pod *)line6);
			break;

		case PODXTLIVE_INTERFACE_VARIAX:
			ret =
			    line6_variax_init(interface,
					      (struct usb_line6_variax *)line6);
			break;

		default:
			dev_err(&interface->dev,
				"PODxt Live interface %d not supported\n",
				interface_number);
			ret = -ENODEV;
		}

		break;

	case LINE6_DEVID_VARIAX:
		ret =
		    line6_variax_init(interface,
				      (struct usb_line6_variax *)line6);
		break;

	case LINE6_DEVID_PODSTUDIO_GX:
	case LINE6_DEVID_PODSTUDIO_UX1:
	case LINE6_DEVID_PODSTUDIO_UX2:
	case LINE6_DEVID_TONEPORT_GX:
	case LINE6_DEVID_TONEPORT_UX1:
	case LINE6_DEVID_TONEPORT_UX2:
	case LINE6_DEVID_GUITARPORT:
		ret =
		    line6_toneport_init(interface,
					(struct usb_line6_toneport *)line6);
		break;

	default:
		MISSING_CASE;
		ret = -ENODEV;
	}

	if (ret < 0)
		goto err_destruct;

	ret = sysfs_create_link(&interface->dev.kobj, &usbdev->dev.kobj,
				"usb_device");
	if (ret < 0)
		goto err_destruct;

	/* creation of additional special files should go here */

	dev_info(&interface->dev, "Line6 %s now attached\n",
		 line6->properties->name);
	line6_devices[devnum] = line6;

	switch (product) {
	case LINE6_DEVID_PODX3:
	case LINE6_DEVID_PODX3LIVE:
		dev_info(&interface->dev,
			 "NOTE: the Line6 %s is detected, but not yet supported\n",
			 line6->properties->name);
	}

	/* increment reference counters: */
	usb_get_intf(interface);
	usb_get_dev(usbdev);

	return 0;

err_destruct:
	line6_destruct(interface);
err_put:
	usb_put_intf(interface);
	usb_put_dev(usbdev);
	return ret;
}

/*
	Line6 device disconnected.
*/
static void line6_disconnect(struct usb_interface *interface)
{
	struct usb_line6 *line6;
	struct usb_device *usbdev;
	int interface_number, i;

	if (interface == NULL)
		return;
	usbdev = interface_to_usbdev(interface);
	if (usbdev == NULL)
		return;

	/* removal of additional special files should go here */

	sysfs_remove_link(&interface->dev.kobj, "usb_device");

	interface_number = interface->cur_altsetting->desc.bInterfaceNumber;
	line6 = usb_get_intfdata(interface);

	if (line6 != NULL) {
		if (line6->urb_listen != NULL)
			line6_stop_listen(line6);

		if (usbdev != line6->usbdev)
			dev_err(line6->ifcdev,
				"driver bug: inconsistent usb device\n");

		switch (line6->usbdev->descriptor.idProduct) {
		case LINE6_DEVID_BASSPODXT:
		case LINE6_DEVID_BASSPODXTLIVE:
		case LINE6_DEVID_BASSPODXTPRO:
		case LINE6_DEVID_POCKETPOD:
		case LINE6_DEVID_PODX3:
		case LINE6_DEVID_PODX3LIVE:
		case LINE6_DEVID_PODXT:
		case LINE6_DEVID_PODXTPRO:
			line6_pod_disconnect(interface);
			break;

		case LINE6_DEVID_PODXTLIVE:
			switch (interface_number) {
			case PODXTLIVE_INTERFACE_POD:
				line6_pod_disconnect(interface);
				break;

			case PODXTLIVE_INTERFACE_VARIAX:
				line6_variax_disconnect(interface);
				break;
			}

			break;

		case LINE6_DEVID_VARIAX:
			line6_variax_disconnect(interface);
			break;

		case LINE6_DEVID_PODSTUDIO_GX:
		case LINE6_DEVID_PODSTUDIO_UX1:
		case LINE6_DEVID_PODSTUDIO_UX2:
		case LINE6_DEVID_TONEPORT_GX:
		case LINE6_DEVID_TONEPORT_UX1:
		case LINE6_DEVID_TONEPORT_UX2:
		case LINE6_DEVID_GUITARPORT:
			line6_toneport_disconnect(interface);
			break;

		default:
			MISSING_CASE;
		}

		dev_info(&interface->dev, "Line6 %s now disconnected\n",
			 line6->properties->name);

		for (i = LINE6_MAX_DEVICES; i--;)
			if (line6_devices[i] == line6)
				line6_devices[i] = NULL;
	}

	line6_destruct(interface);

	/* decrement reference counters: */
	usb_put_intf(interface);
	usb_put_dev(usbdev);
}

#ifdef CONFIG_PM

/*
	Suspend Line6 device.
*/
static int line6_suspend(struct usb_interface *interface, pm_message_t message)
{
	struct usb_line6 *line6 = usb_get_intfdata(interface);
	struct snd_line6_pcm *line6pcm = line6->line6pcm;

	snd_power_change_state(line6->card, SNDRV_CTL_POWER_D3hot);

	if (line6->properties->capabilities & LINE6_BIT_CONTROL)
		line6_stop_listen(line6);

	if (line6pcm != NULL) {
		snd_pcm_suspend_all(line6pcm->pcm);
		line6_pcm_disconnect(line6pcm);
		line6pcm->flags = 0;
	}

	return 0;
}

/*
	Resume Line6 device.
*/
static int line6_resume(struct usb_interface *interface)
{
	struct usb_line6 *line6 = usb_get_intfdata(interface);

	if (line6->properties->capabilities & LINE6_BIT_CONTROL)
		line6_start_listen(line6);

	snd_power_change_state(line6->card, SNDRV_CTL_POWER_D0);
	return 0;
}

/*
	Resume Line6 device after reset.
*/
static int line6_reset_resume(struct usb_interface *interface)
{
	struct usb_line6 *line6 = usb_get_intfdata(interface);

	switch (line6->usbdev->descriptor.idProduct) {
	case LINE6_DEVID_PODSTUDIO_GX:
	case LINE6_DEVID_PODSTUDIO_UX1:
	case LINE6_DEVID_PODSTUDIO_UX2:
	case LINE6_DEVID_TONEPORT_GX:
	case LINE6_DEVID_TONEPORT_UX1:
	case LINE6_DEVID_TONEPORT_UX2:
	case LINE6_DEVID_GUITARPORT:
		line6_toneport_reset_resume((struct usb_line6_toneport *)line6);
	}

	return line6_resume(interface);
}

#endif /* CONFIG_PM */

static struct usb_driver line6_driver = {
	.name = DRIVER_NAME,
	.probe = line6_probe,
	.disconnect = line6_disconnect,
#ifdef CONFIG_PM
	.suspend = line6_suspend,
	.resume = line6_resume,
	.reset_resume = line6_reset_resume,
#endif
	.id_table = line6_id_table,
};

/*
	Module initialization.
*/
static int __init line6_init(void)
{
	int i, retval;

	printk(KERN_INFO "%s driver version %s\n", DRIVER_NAME, DRIVER_VERSION);

	for (i = LINE6_MAX_DEVICES; i--;)
		line6_devices[i] = NULL;

	retval = usb_register(&line6_driver);

	if (retval) {
		err("usb_register failed. Error number %d", retval);
		return retval;
	}

	line6_request_version = kmalloc(sizeof(line6_request_version0),
					GFP_KERNEL);

	if (line6_request_version == NULL) {
		err("Out of memory");
		return -ENOMEM;
	}

	memcpy((char *)line6_request_version, line6_request_version0,
	       sizeof(line6_request_version0));

	return retval;
}

/*
	Module cleanup.
*/
static void __exit line6_exit(void)
{
	int i;
	struct usb_line6 *line6;
	struct snd_line6_pcm *line6pcm;

	/* stop all PCM channels */
	for (i = LINE6_MAX_DEVICES; i--;) {
		line6 = line6_devices[i];

		if (line6 == NULL)
			continue;

		line6pcm = line6->line6pcm;

		if (line6pcm == NULL)
			continue;

		line6_pcm_stop(line6pcm, ~0);
	}

	usb_deregister(&line6_driver);
	kfree(line6_request_version);
}

module_init(line6_init);
module_exit(line6_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
