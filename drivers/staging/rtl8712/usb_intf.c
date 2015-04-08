/******************************************************************************
 * usb_intf.c
 *
 * Copyright(c) 2007 - 2010 Realtek Corporation. All rights reserved.
 * Linux device driver for RTL8192SU
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * Modifications for inclusion into the Linux staging tree are
 * Copyright(c) 2010 Larry Finger. All rights reserved.
 *
 * Contact information:
 * WLAN FAE <wlanfae@realtek.com>
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 ******************************************************************************/

#define _HCI_INTF_C_

#include "osdep_service.h"
#include "drv_types.h"
#include "recv_osdep.h"
#include "xmit_osdep.h"
#include "rtl8712_efuse.h"
#include "usb_vendor_req.h"
#include "usb_ops.h"
#include "usb_osintf.h"

#define DRVER  "v7_0.20100831"

static struct usb_interface *pintf;

static int r871xu_drv_init(struct usb_interface *pusb_intf,
			   const struct usb_device_id *pdid);

static void r871xu_dev_remove(struct usb_interface *pusb_intf);

static struct usb_device_id rtl871x_usb_id_tbl[] = {
	/*92SU
	 * Realtek */
	{USB_DEVICE(0x0bda, 0x8171)},
	{USB_DEVICE(0x0bda, 0x8172)},
	{USB_DEVICE(0x0bda, 0x8173)},
	{USB_DEVICE(0x0bda, 0x8174)},
	{USB_DEVICE(0x0bda, 0x8712)},
	{USB_DEVICE(0x0bda, 0x8713)},
	{USB_DEVICE(0x0bda, 0xC512)},
	/* Abocom  */
	{USB_DEVICE(0x07B8, 0x8188)},
	/* Corega */
	{USB_DEVICE(0x07aa, 0x0047)},
	/* Dlink */
	{USB_DEVICE(0x07d1, 0x3303)},
	{USB_DEVICE(0x07d1, 0x3302)},
	{USB_DEVICE(0x07d1, 0x3300)},
	/* Dlink for Skyworth */
	{USB_DEVICE(0x14b2, 0x3300)},
	{USB_DEVICE(0x14b2, 0x3301)},
	{USB_DEVICE(0x14b2, 0x3302)},
	/* EnGenius */
	{USB_DEVICE(0x1740, 0x9603)},
	{USB_DEVICE(0x1740, 0x9605)},
	/* Belkin */
	{USB_DEVICE(0x050d, 0x815F)},
	{USB_DEVICE(0x050d, 0x945A)},
	{USB_DEVICE(0x050d, 0x845A)},
	/* Guillemot */
	{USB_DEVICE(0x06f8, 0xe031)},
	/* Edimax */
	{USB_DEVICE(0x7392, 0x7611)},
	{USB_DEVICE(0x7392, 0x7612)},
	{USB_DEVICE(0x7392, 0x7622)},
	/* Sitecom */
	{USB_DEVICE(0x0DF6, 0x0045)},
	/* Hawking */
	{USB_DEVICE(0x0E66, 0x0015)},
	{USB_DEVICE(0x0E66, 0x0016)},
	{USB_DEVICE(0x0b05, 0x1786)},
	{USB_DEVICE(0x0b05, 0x1791)},    /* 11n mode disable */

	{USB_DEVICE(0x13D3, 0x3306)},
	{USB_DEVICE(0x13D3, 0x3309)},
	{USB_DEVICE(0x13D3, 0x3310)},
	{USB_DEVICE(0x13D3, 0x3311)},    /* 11n mode disable */
	{USB_DEVICE(0x13D3, 0x3325)},
	{USB_DEVICE(0x083A, 0xC512)},
	{}
};

MODULE_DEVICE_TABLE(usb, rtl871x_usb_id_tbl);

static struct specific_device_id specific_device_id_tbl[] = {
	{.idVendor = 0x0b05, .idProduct = 0x1791,
		 .flags = SPEC_DEV_ID_DISABLE_HT},
	{.idVendor = 0x13D3, .idProduct = 0x3311,
		 .flags = SPEC_DEV_ID_DISABLE_HT},
	{}
};

struct drv_priv {
	struct usb_driver r871xu_drv;
	int drv_registered;
};

#ifdef CONFIG_PM
static int r871x_suspend(struct usb_interface *pusb_intf, pm_message_t state)
{
	struct net_device *pnetdev = usb_get_intfdata(pusb_intf);

	printk(KERN_INFO "r8712: suspending...\n");
	if (!pnetdev || !netif_running(pnetdev)) {
		printk(KERN_INFO "r8712: unable to suspend\n");
		return 0;
	}
	if (pnetdev->netdev_ops->ndo_stop)
		pnetdev->netdev_ops->ndo_stop(pnetdev);
	mdelay(10);
	netif_device_detach(pnetdev);
	return 0;
}

static int r871x_resume(struct usb_interface *pusb_intf)
{
	struct net_device *pnetdev = usb_get_intfdata(pusb_intf);

	printk(KERN_INFO "r8712: resuming...\n");
	if (!pnetdev || !netif_running(pnetdev)) {
		printk(KERN_INFO "r8712: unable to resume\n");
		return 0;
	}
	netif_device_attach(pnetdev);
	if (pnetdev->netdev_ops->ndo_open)
		pnetdev->netdev_ops->ndo_open(pnetdev);
	return 0;
}

static int r871x_reset_resume(struct usb_interface *pusb_intf)
{
	/* dummy routine */
	return 0;
}

#endif

static struct drv_priv drvpriv = {
	.r871xu_drv.name = "r8712u",
	.r871xu_drv.id_table = rtl871x_usb_id_tbl,
	.r871xu_drv.probe = r871xu_drv_init,
	.r871xu_drv.disconnect = r871xu_dev_remove,
#ifdef CONFIG_PM
	.r871xu_drv.suspend = r871x_suspend,
	.r871xu_drv.resume = r871x_resume,
	.r871xu_drv.reset_resume = r871x_reset_resume,
#endif
};

static uint r8712_usb_dvobj_init(struct _adapter *padapter)
{
	uint	status = _SUCCESS;
	struct	usb_device_descriptor		*pdev_desc;
	struct	usb_host_config			*phost_conf;
	struct	usb_config_descriptor		*pconf_desc;
	struct	usb_host_interface		*phost_iface;
	struct	usb_interface_descriptor	*piface_desc;
	struct dvobj_priv *pdvobjpriv = &padapter->dvobjpriv;
	struct usb_device *pusbd = pdvobjpriv->pusbdev;

	pdvobjpriv->padapter = padapter;
	padapter->EepromAddressSize = 6;
	pdev_desc = &pusbd->descriptor;
	phost_conf = pusbd->actconfig;
	pconf_desc = &phost_conf->desc;
	phost_iface = &pintf->altsetting[0];
	piface_desc = &phost_iface->desc;
	pdvobjpriv->nr_endpoint = piface_desc->bNumEndpoints;
	if (pusbd->speed == USB_SPEED_HIGH) {
		pdvobjpriv->ishighspeed = true;
		printk(KERN_INFO "r8712u: USB_SPEED_HIGH with %d endpoints\n",
		       pdvobjpriv->nr_endpoint);
	} else {
		pdvobjpriv->ishighspeed = false;
		printk(KERN_INFO "r8712u: USB_SPEED_LOW with %d endpoints\n",
		       pdvobjpriv->nr_endpoint);
	}
	if ((r8712_alloc_io_queue(padapter)) == _FAIL)
		status = _FAIL;
	sema_init(&(padapter->dvobjpriv.usb_suspend_sema), 0);
	return status;
}

static void r8712_usb_dvobj_deinit(struct _adapter *padapter)
{
}

void rtl871x_intf_stop(struct _adapter *padapter)
{
	/*disable_hw_interrupt*/
	if (padapter->bSurpriseRemoved == false) {
		/*device still exists, so driver can do i/o operation
		 * TODO: */
	}

	/* cancel in irp */
	if (padapter->dvobjpriv.inirp_deinit != NULL)
		padapter->dvobjpriv.inirp_deinit(padapter);
	/* cancel out irp */
	r8712_usb_write_port_cancel(padapter);
	/* TODO:cancel other irps */
}

void r871x_dev_unload(struct _adapter *padapter)
{
	if (padapter->bup == true) {
		/*s1.*/
		padapter->bDriverStopped = true;

		/*s3.*/
		rtl871x_intf_stop(padapter);

		/*s4.*/
		r8712_stop_drv_threads(padapter);

		/*s5.*/
		if (padapter->bSurpriseRemoved == false) {
			padapter->hw_init_completed = false;
			rtl8712_hal_deinit(padapter);
		}

		/*s6.*/
		if (padapter->dvobj_deinit)
			padapter->dvobj_deinit(padapter);
		padapter->bup = false;
	}
}

static void disable_ht_for_spec_devid(const struct usb_device_id *pdid,
				      struct _adapter *padapter)
{
	u16 vid, pid;
	u32 flags;
	int i;
	int num = sizeof(specific_device_id_tbl) /
		  sizeof(struct specific_device_id);

	for (i = 0; i < num; i++) {
		vid = specific_device_id_tbl[i].idVendor;
		pid = specific_device_id_tbl[i].idProduct;
		flags = specific_device_id_tbl[i].flags;

		if ((pdid->idVendor == vid) && (pdid->idProduct == pid) &&
		    (flags&SPEC_DEV_ID_DISABLE_HT)) {
			padapter->registrypriv.ht_enable = 0;
			padapter->registrypriv.cbw40_enable = 0;
			padapter->registrypriv.ampdu_enable = 0;
		}
	}
}

static u8 key_2char2num(u8 hch, u8 lch)
{
	return (hex_to_bin(hch) << 4) | hex_to_bin(lch);
}

/*
 * drv_init() - a device potentially for us
 *
 * notes: drv_init() is called when the bus driver has located a card for us
 * to support. We accept the new device by returning 0.
*/
static int r871xu_drv_init(struct usb_interface *pusb_intf,
			   const struct usb_device_id *pdid)
{
	uint status;
	struct _adapter *padapter = NULL;
	struct dvobj_priv *pdvobjpriv;
	struct net_device *pnetdev;

	printk(KERN_INFO "r8712u: DriverVersion: %s\n", DRVER);
	/* In this probe function, O.S. will provide the usb interface pointer
	 * to driver. We have to increase the reference count of the usb device
	 * structure by using the usb_get_dev function.
	 */
	usb_get_dev(interface_to_usbdev(pusb_intf));
	pintf = pusb_intf;
	/* step 1. */
	pnetdev = r8712_init_netdev();
	if (!pnetdev)
		goto error;
	padapter = (struct _adapter *)_netdev_priv(pnetdev);
	disable_ht_for_spec_devid(pdid, padapter);
	pdvobjpriv = &padapter->dvobjpriv;
	pdvobjpriv->padapter = padapter;
	padapter->dvobjpriv.pusbdev = interface_to_usbdev(pusb_intf);
	usb_set_intfdata(pusb_intf, pnetdev);
	SET_NETDEV_DEV(pnetdev, &pusb_intf->dev);
	/* step 2. */
	padapter->dvobj_init = &r8712_usb_dvobj_init;
	padapter->dvobj_deinit = &r8712_usb_dvobj_deinit;
	padapter->halpriv.hal_bus_init = &r8712_usb_hal_bus_init;
	padapter->dvobjpriv.inirp_init = &r8712_usb_inirp_init;
	padapter->dvobjpriv.inirp_deinit = &r8712_usb_inirp_deinit;
	/* step 3.
	 * initialize the dvobj_priv
	 */
	if (padapter->dvobj_init == NULL)
			goto error;
	else {
		status = padapter->dvobj_init(padapter);
		if (status != _SUCCESS)
			goto error;
	}
	/* step 4. */
	status = r8712_init_drv_sw(padapter);
	if (status == _FAIL)
		goto error;
	/* step 5. read efuse/eeprom data and get mac_addr */
	{
		int i, offset;
		u8 mac[6];
		u8 tmpU1b, AutoloadFail, eeprom_CustomerID;
		u8 *pdata = padapter->eeprompriv.efuse_eeprom_data;

		tmpU1b = r8712_read8(padapter, EE_9346CR);/*CR9346*/

		/* To check system boot selection.*/
		printk(KERN_INFO "r8712u: Boot from %s: Autoload %s\n",
		       (tmpU1b & _9356SEL) ? "EEPROM" : "EFUSE",
		       (tmpU1b & _EEPROM_EN) ? "OK" : "Failed");

		/* To check autoload success or not.*/
		if (tmpU1b & _EEPROM_EN) {
			AutoloadFail = true;
			/* The following operations prevent Efuse leakage by
			 * turning on 2.5V.
			 */
			tmpU1b = r8712_read8(padapter, EFUSE_TEST+3);
			r8712_write8(padapter, EFUSE_TEST + 3, tmpU1b | 0x80);
			msleep(20);
			r8712_write8(padapter, EFUSE_TEST + 3,
				     (tmpU1b & (~BIT(7))));

			/* Retrieve Chip version.
			 * Recognize IC version by Reg0x4 BIT15.
			 */
			tmpU1b = (u8)((r8712_read32(padapter, PMC_FSM) >> 15) &
						    0x1F);
			if (tmpU1b == 0x3)
				padapter->registrypriv.chip_version =
				     RTL8712_3rdCUT;
			else
				padapter->registrypriv.chip_version =
				     (tmpU1b >> 1) + 1;
			switch (padapter->registrypriv.chip_version) {
			case RTL8712_1stCUT:
			case RTL8712_2ndCUT:
			case RTL8712_3rdCUT:
				break;
			default:
				padapter->registrypriv.chip_version =
				     RTL8712_2ndCUT;
				break;
			}

			for (i = 0, offset = 0; i < 128; i += 8, offset++)
				r8712_efuse_pg_packet_read(padapter, offset,
						     &pdata[i]);

			if (r8712_initmac) {
				/* Users specify the mac address */
				int jj, kk;

				for (jj = 0, kk = 0; jj < ETH_ALEN;
				     jj++, kk += 3)
					mac[jj] =
					   key_2char2num(r8712_initmac[kk],
					   r8712_initmac[kk + 1]);
			} else {
				/* Use the mac address stored in the Efuse
				 * offset = 0x12 for usb in efuse
				 */
				memcpy(mac, &pdata[0x12], ETH_ALEN);
			}
			eeprom_CustomerID = pdata[0x52];
			switch (eeprom_CustomerID) {
			case EEPROM_CID_ALPHA:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_ALPHA;
				break;
			case EEPROM_CID_CAMEO:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_CAMEO;
				break;
			case EEPROM_CID_SITECOM:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_Sitecom;
				break;
			case EEPROM_CID_COREGA:
				padapter->eeprompriv.CustomerID =
						 RT_CID_COREGA;
				break;
			case EEPROM_CID_Senao:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_Senao;
				break;
			case EEPROM_CID_EDIMAX_BELKIN:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_Edimax_Belkin;
				break;
			case EEPROM_CID_SERCOMM_BELKIN:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_Sercomm_Belkin;
				break;
			case EEPROM_CID_WNC_COREGA:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_WNC_COREGA;
				break;
			case EEPROM_CID_WHQL:
				break;
			case EEPROM_CID_NetCore:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_Netcore;
				break;
			case EEPROM_CID_CAMEO1:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_CAMEO1;
				break;
			case EEPROM_CID_CLEVO:
				padapter->eeprompriv.CustomerID =
						 RT_CID_819x_CLEVO;
				break;
			default:
				padapter->eeprompriv.CustomerID =
						 RT_CID_DEFAULT;
				break;
			}
			printk(KERN_INFO "r8712u: CustomerID = 0x%.4x\n",
			     padapter->eeprompriv.CustomerID);
			/* Led mode */
			switch (padapter->eeprompriv.CustomerID) {
			case RT_CID_DEFAULT:
			case RT_CID_819x_ALPHA:
			case RT_CID_819x_CAMEO:
				padapter->ledpriv.LedStrategy = SW_LED_MODE1;
				padapter->ledpriv.bRegUseLed = true;
				break;
			case RT_CID_819x_Sitecom:
				padapter->ledpriv.LedStrategy = SW_LED_MODE2;
				padapter->ledpriv.bRegUseLed = true;
				break;
			case RT_CID_COREGA:
			case RT_CID_819x_Senao:
				padapter->ledpriv.LedStrategy = SW_LED_MODE3;
				padapter->ledpriv.bRegUseLed = true;
				break;
			case RT_CID_819x_Edimax_Belkin:
				padapter->ledpriv.LedStrategy = SW_LED_MODE4;
				padapter->ledpriv.bRegUseLed = true;
				break;
			case RT_CID_819x_Sercomm_Belkin:
				padapter->ledpriv.LedStrategy = SW_LED_MODE5;
				padapter->ledpriv.bRegUseLed = true;
				break;
			case RT_CID_819x_WNC_COREGA:
				padapter->ledpriv.LedStrategy = SW_LED_MODE6;
				padapter->ledpriv.bRegUseLed = true;
				break;
			default:
				padapter->ledpriv.LedStrategy = SW_LED_MODE0;
				padapter->ledpriv.bRegUseLed = false;
				break;
			}
		} else
			AutoloadFail = false;
		if (((mac[0] == 0xff) && (mac[1] == 0xff) &&
		     (mac[2] == 0xff) && (mac[3] == 0xff) &&
		     (mac[4] == 0xff) && (mac[5] == 0xff)) ||
		    ((mac[0] == 0x00) && (mac[1] == 0x00) &&
		     (mac[2] == 0x00) && (mac[3] == 0x00) &&
		     (mac[4] == 0x00) && (mac[5] == 0x00)) ||
		     (AutoloadFail == false)) {
			mac[0] = 0x00;
			mac[1] = 0xe0;
			mac[2] = 0x4c;
			mac[3] = 0x87;
			mac[4] = 0x00;
			mac[5] = 0x00;
		}
		if (r8712_initmac) {
			/* Make sure the user did not select a multicast
			 * address by setting bit 1 of first octet.
			 */
			mac[0] &= 0xFE;
			printk(KERN_INFO "r8712u: MAC Address from user = "
			       "%pM\n", mac);
		} else
			printk(KERN_INFO "r8712u: MAC Address from efuse = "
			       "%pM\n", mac);
		memcpy(pnetdev->dev_addr, mac, ETH_ALEN);
	}
	/* step 6. Tell the network stack we exist */
	if (register_netdev(pnetdev) != 0)
		goto error;
	return 0;
error:
	usb_put_dev(interface_to_usbdev(pusb_intf));
	usb_set_intfdata(pusb_intf, NULL);
	if (padapter->dvobj_deinit != NULL)
		padapter->dvobj_deinit(padapter);
	if (pnetdev)
		os_free_netdev(pnetdev);
	return -ENODEV;
}

/* rmmod module & unplug(SurpriseRemoved) will call r871xu_dev_remove()
 * => how to recognize both */
static void r871xu_dev_remove(struct usb_interface *pusb_intf)
{
	struct net_device *pnetdev = usb_get_intfdata(pusb_intf);
	struct _adapter *padapter = (struct _adapter *)netdev_priv(pnetdev);
	struct usb_device *udev = interface_to_usbdev(pusb_intf);

	if (padapter) {
		if (drvpriv.drv_registered == true)
			padapter->bSurpriseRemoved = true;
		if (pnetdev != NULL) {
			/* will call netdev_close() */
			unregister_netdev(pnetdev);
		}
		flush_scheduled_work();
		udelay(1);
		r871x_dev_unload(padapter);
		r8712_free_drv_sw(padapter);
	}
	usb_set_intfdata(pusb_intf, NULL);
	/* decrease the reference count of the usb device structure
	 * when disconnect */
	usb_put_dev(udev);
	/* If we didn't unplug usb dongle and remove/insert modlue, driver
	 * fails on sitesurvey for the first time when device is up.
	 * Reset usb port for sitesurvey fail issue. */
	if (udev->state != USB_STATE_NOTATTACHED)
		usb_reset_device(udev);
	return;
}

static int __init r8712u_drv_entry(void)
{
	drvpriv.drv_registered = true;
	return usb_register(&drvpriv.r871xu_drv);
}

static void __exit r8712u_drv_halt(void)
{
	drvpriv.drv_registered = false;
	usb_deregister(&drvpriv.r871xu_drv);
	printk(KERN_INFO "r8712u: Driver unloaded\n");
}

module_init(r8712u_drv_entry);
module_exit(r8712u_drv_halt);
