/*
 * drivers/pci/iov.c
 *
 * Copyright (C) 2009 Intel Corporation, Yu Zhao <yu.zhao@intel.com>
 *
 * PCI Express I/O Virtualization (IOV) support.
 *   Single Root IOV 1.0
 *   Address Translation Service 1.0
 */

#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/pci-ats.h>
#include "pci.h"

#define VIRTFN_ID_LEN	16

static inline u8 virtfn_bus(struct pci_dev *dev, int id)
{
	return dev->bus->number + ((dev->devfn + dev->sriov->offset +
				    dev->sriov->stride * id) >> 8);
}

static inline u8 virtfn_devfn(struct pci_dev *dev, int id)
{
	return (dev->devfn + dev->sriov->offset +
		dev->sriov->stride * id) & 0xff;
}

static struct pci_bus *virtfn_add_bus(struct pci_bus *bus, int busnr)
{
	struct pci_bus *child;

	if (bus->number == busnr)
		return bus;

	child = pci_find_bus(pci_domain_nr(bus), busnr);
	if (child)
		return child;

	child = pci_add_new_bus(bus, NULL, busnr);
	if (!child)
		return NULL;

	pci_bus_insert_busn_res(child, busnr, busnr);

	return child;
}

static void virtfn_remove_bus(struct pci_bus *physbus, struct pci_bus *virtbus)
{
	if (physbus != virtbus && list_empty(&virtbus->devices))
		pci_remove_bus(virtbus);
}

static int virtfn_add(struct pci_dev *dev, int id, int reset)
{
	int i;
	int rc = -ENOMEM;
	u64 size;
	char buf[VIRTFN_ID_LEN];
	struct pci_dev *virtfn;
	struct resource *res;
	struct pci_sriov *iov = dev->sriov;
	struct pci_bus *bus;

	mutex_lock(&iov->dev->sriov->lock);
	bus = virtfn_add_bus(dev->bus, virtfn_bus(dev, id));
	if (!bus)
		goto failed;

	virtfn = pci_alloc_dev(bus);
	if (!virtfn)
		goto failed0;

	virtfn->devfn = virtfn_devfn(dev, id);
	virtfn->vendor = dev->vendor;
	pci_read_config_word(dev, iov->pos + PCI_SRIOV_VF_DID, &virtfn->device);
	pci_setup_device(virtfn);
	virtfn->dev.parent = dev->dev.parent;
	virtfn->physfn = pci_dev_get(dev);
	virtfn->is_virtfn = 1;
	virtfn->multifunction = 0;

	for (i = 0; i < PCI_SRIOV_NUM_BARS; i++) {
		res = dev->resource + PCI_IOV_RESOURCES + i;
		if (!res->parent)
			continue;
		virtfn->resource[i].name = pci_name(virtfn);
		virtfn->resource[i].flags = res->flags;
		size = resource_size(res);
		do_div(size, iov->total_VFs);
		virtfn->resource[i].start = res->start + size * id;
		virtfn->resource[i].end = virtfn->resource[i].start + size - 1;
		rc = request_resource(res, &virtfn->resource[i]);
		BUG_ON(rc);
	}

	if (reset)
		__pci_reset_function(virtfn);

	pci_device_add(virtfn, virtfn->bus);
	mutex_unlock(&iov->dev->sriov->lock);

	pci_bus_add_device(virtfn);
	sprintf(buf, "virtfn%u", id);
	rc = sysfs_create_link(&dev->dev.kobj, &virtfn->dev.kobj, buf);
	if (rc)
		goto failed1;
	rc = sysfs_create_link(&virtfn->dev.kobj, &dev->dev.kobj, "physfn");
	if (rc)
		goto failed2;

	kobject_uevent(&virtfn->dev.kobj, KOBJ_CHANGE);

	return 0;

failed2:
	sysfs_remove_link(&dev->dev.kobj, buf);
failed1:
	pci_dev_put(dev);
	mutex_lock(&iov->dev->sriov->lock);
	pci_stop_and_remove_bus_device(virtfn);
failed0:
	virtfn_remove_bus(dev->bus, bus);
failed:
	mutex_unlock(&iov->dev->sriov->lock);

	return rc;
}

static void virtfn_remove(struct pci_dev *dev, int id, int reset)
{
	char buf[VIRTFN_ID_LEN];
	struct pci_dev *virtfn;
	struct pci_sriov *iov = dev->sriov;

	virtfn = pci_get_domain_bus_and_slot(pci_domain_nr(dev->bus),
					     virtfn_bus(dev, id),
					     virtfn_devfn(dev, id));
	if (!virtfn)
		return;

	if (reset) {
		device_release_driver(&virtfn->dev);
		__pci_reset_function(virtfn);
	}

	sprintf(buf, "virtfn%u", id);
	sysfs_remove_link(&dev->dev.kobj, buf);
	/*
	 * pci_stop_dev() could have been called for this virtfn already,
	 * so the directory for the virtfn may have been removed before.
	 * Double check to avoid spurious sysfs warnings.
	 */
	if (virtfn->dev.kobj.sd)
		sysfs_remove_link(&virtfn->dev.kobj, "physfn");

	mutex_lock(&iov->dev->sriov->lock);
	pci_stop_and_remove_bus_device(virtfn);
	virtfn_remove_bus(dev->bus, virtfn->bus);
	mutex_unlock(&iov->dev->sriov->lock);

	/* balance pci_get_domain_bus_and_slot() */
	pci_dev_put(virtfn);
	pci_dev_put(dev);
}

static int sriov_enable(struct pci_dev *dev, int nr_virtfn)
{
	int rc;
	int i, j;
	int nres;
	u16 offset, stride, initial;
	struct resource *res;
	struct pci_dev *pdev;
	struct pci_sriov *iov = dev->sriov;
	int bars = 0;

	if (!nr_virtfn)
		return 0;

	if (iov->num_VFs)
		return -EINVAL;

	pci_read_config_word(dev, iov->pos + PCI_SRIOV_INITIAL_VF, &initial);
	if (initial > iov->total_VFs ||
	    (!(iov->cap & PCI_SRIOV_CAP_VFM) && (initial != iov->total_VFs)))
		return -EIO;

	if (nr_virtfn < 0 || nr_virtfn > iov->total_VFs ||
	    (!(iov->cap & PCI_SRIOV_CAP_VFM) && (nr_virtfn > initial)))
		return -EINVAL;

	pci_read_config_word(dev, iov->pos + PCI_SRIOV_VF_OFFSET, &offset);
	pci_read_config_word(dev, iov->pos + PCI_SRIOV_VF_STRIDE, &stride);
	if (!offset || (nr_virtfn > 1 && !stride))
		return -EIO;

	nres = 0;
	for (i = 0; i < PCI_SRIOV_NUM_BARS; i++) {
		bars |= (1 << (i + PCI_IOV_RESOURCES));
		res = dev->resource + PCI_IOV_RESOURCES + i;
		if (res->parent)
			nres++;
	}
	if (nres != iov->nres) {
		dev_err(&dev->dev, "not enough MMIO resources for SR-IOV\n");
		return -ENOMEM;
	}

	iov->offset = offset;
	iov->stride = stride;

	if (virtfn_bus(dev, nr_virtfn - 1) > dev->bus->busn_res.end) {
		dev_err(&dev->dev, "SR-IOV: bus number out of range\n");
		return -ENOMEM;
	}

	if (pci_enable_resources(dev, bars)) {
		dev_err(&dev->dev, "SR-IOV: IOV BARS not allocated\n");
		return -ENOMEM;
	}

	if (iov->link != dev->devfn) {
		pdev = pci_get_slot(dev->bus, iov->link);
		if (!pdev)
			return -ENODEV;

		if (!pdev->is_physfn) {
			pci_dev_put(pdev);
			return -ENOSYS;
		}

		rc = sysfs_create_link(&dev->dev.kobj,
					&pdev->dev.kobj, "dep_link");
		pci_dev_put(pdev);
		if (rc)
			return rc;
	}

	pci_write_config_word(dev, iov->pos + PCI_SRIOV_NUM_VF, nr_virtfn);
	iov->ctrl |= PCI_SRIOV_CTRL_VFE | PCI_SRIOV_CTRL_MSE;
	pci_cfg_access_lock(dev);
	pci_write_config_word(dev, iov->pos + PCI_SRIOV_CTRL, iov->ctrl);
	msleep(100);
	pci_cfg_access_unlock(dev);

	iov->initial_VFs = initial;
	if (nr_virtfn < initial)
		initial = nr_virtfn;

	for (i = 0; i < initial; i++) {
		rc = virtfn_add(dev, i, 0);
		if (rc)
			goto failed;
	}

	kobject_uevent(&dev->dev.kobj, KOBJ_CHANGE);
	iov->num_VFs = nr_virtfn;

	return 0;

failed:
	for (j = 0; j < i; j++)
		virtfn_remove(dev, j, 0);

	iov->ctrl &= ~(PCI_SRIOV_CTRL_VFE | PCI_SRIOV_CTRL_MSE);
	pci_cfg_access_lock(dev);
	pci_write_config_word(dev, iov->pos + PCI_SRIOV_CTRL, iov->ctrl);
	pci_write_config_word(dev, iov->pos + PCI_SRIOV_NUM_VF, 0);
	ssleep(1);
	pci_cfg_access_unlock(dev);

	if (iov->link != dev->devfn)
		sysfs_remove_link(&dev->dev.kobj, "dep_link");

	return rc;
}

static void sriov_disable(struct pci_dev *dev)
{
	int i;
	struct pci_sriov *iov = dev->sriov;

	if (!iov->num_VFs)
		return;

	for (i = 0; i < iov->num_VFs; i++)
		virtfn_remove(dev, i, 0);

	iov->ctrl &= ~(PCI_SRIOV_CTRL_VFE | PCI_SRIOV_CTRL_MSE);
	pci_cfg_access_lock(dev);
	pci_write_config_word(dev, iov->pos + PCI_SRIOV_CTRL, iov->ctrl);
	ssleep(1);
	pci_cfg_access_unlock(dev);

	if (iov->link != dev->devfn)
		sysfs_remove_link(&dev->dev.kobj, "dep_link");

	iov->num_VFs = 0;
	pci_write_config_word(dev, iov->pos + PCI_SRIOV_NUM_VF, 0);
}

static int sriov_init(struct pci_dev *dev, int pos)
{
	int i;
	int rc;
	int nres;
	u32 pgsz;
	u16 ctrl, total, offset, stride;
	struct pci_sriov *iov;
	struct resource *res;
	struct pci_dev *pdev;

	if (pci_pcie_type(dev) != PCI_EXP_TYPE_RC_END &&
	    pci_pcie_type(dev) != PCI_EXP_TYPE_ENDPOINT)
		return -ENODEV;

	pci_read_config_word(dev, pos + PCI_SRIOV_CTRL, &ctrl);
	if (ctrl & PCI_SRIOV_CTRL_VFE) {
		pci_write_config_word(dev, pos + PCI_SRIOV_CTRL, 0);
		ssleep(1);
	}

	pci_read_config_word(dev, pos + PCI_SRIOV_TOTAL_VF, &total);
	if (!total)
		return 0;

	ctrl = 0;
	list_for_each_entry(pdev, &dev->bus->devices, bus_list)
		if (pdev->is_physfn)
			goto found;

	pdev = NULL;
	if (pci_ari_enabled(dev->bus))
		ctrl |= PCI_SRIOV_CTRL_ARI;

found:
	pci_write_config_word(dev, pos + PCI_SRIOV_CTRL, ctrl);
	pci_write_config_word(dev, pos + PCI_SRIOV_NUM_VF, 0);
	pci_read_config_word(dev, pos + PCI_SRIOV_VF_OFFSET, &offset);
	pci_read_config_word(dev, pos + PCI_SRIOV_VF_STRIDE, &stride);
	if (!offset || (total > 1 && !stride))
		return -EIO;

	pci_read_config_dword(dev, pos + PCI_SRIOV_SUP_PGSIZE, &pgsz);
	i = PAGE_SHIFT > 12 ? PAGE_SHIFT - 12 : 0;
	pgsz &= ~((1 << i) - 1);
	if (!pgsz)
		return -EIO;

	pgsz &= ~(pgsz - 1);
	pci_write_config_dword(dev, pos + PCI_SRIOV_SYS_PGSIZE, pgsz);

	nres = 0;
	for (i = 0; i < PCI_SRIOV_NUM_BARS; i++) {
		res = dev->resource + PCI_IOV_RESOURCES + i;
		i += __pci_read_base(dev, pci_bar_unknown, res,
				     pos + PCI_SRIOV_BAR + i * 4);
		if (!res->flags)
			continue;
		if (resource_size(res) & (PAGE_SIZE - 1)) {
			rc = -EIO;
			goto failed;
		}
		res->end = res->start + resource_size(res) * total - 1;
		nres++;
	}

	iov = kzalloc(sizeof(*iov), GFP_KERNEL);
	if (!iov) {
		rc = -ENOMEM;
		goto failed;
	}

	iov->pos = pos;
	iov->nres = nres;
	iov->ctrl = ctrl;
	iov->total_VFs = total;
	iov->offset = offset;
	iov->stride = stride;
	iov->pgsz = pgsz;
	iov->self = dev;
	pci_read_config_dword(dev, pos + PCI_SRIOV_CAP, &iov->cap);
	pci_read_config_byte(dev, pos + PCI_SRIOV_FUNC_LINK, &iov->link);
	if (pci_pcie_type(dev) == PCI_EXP_TYPE_RC_END)
		iov->link = PCI_DEVFN(PCI_SLOT(dev->devfn), iov->link);

	if (pdev)
		iov->dev = pci_dev_get(pdev);
	else
		iov->dev = dev;

	mutex_init(&iov->lock);

	dev->sriov = iov;
	dev->is_physfn = 1;

	return 0;

failed:
	for (i = 0; i < PCI_SRIOV_NUM_BARS; i++) {
		res = dev->resource + PCI_IOV_RESOURCES + i;
		res->flags = 0;
	}

	return rc;
}

static void sriov_release(struct pci_dev *dev)
{
	BUG_ON(dev->sriov->num_VFs);

	if (dev != dev->sriov->dev)
		pci_dev_put(dev->sriov->dev);

	mutex_destroy(&dev->sriov->lock);

	kfree(dev->sriov);
	dev->sriov = NULL;
}

static void sriov_restore_state(struct pci_dev *dev)
{
	int i;
	u16 ctrl;
	struct pci_sriov *iov = dev->sriov;

	pci_read_config_word(dev, iov->pos + PCI_SRIOV_CTRL, &ctrl);
	if (ctrl & PCI_SRIOV_CTRL_VFE)
		return;

	for (i = PCI_IOV_RESOURCES; i <= PCI_IOV_RESOURCE_END; i++)
		pci_update_resource(dev, i);

	pci_write_config_dword(dev, iov->pos + PCI_SRIOV_SYS_PGSIZE, iov->pgsz);
	pci_write_config_word(dev, iov->pos + PCI_SRIOV_NUM_VF, iov->num_VFs);
	pci_write_config_word(dev, iov->pos + PCI_SRIOV_CTRL, iov->ctrl);
	if (iov->ctrl & PCI_SRIOV_CTRL_VFE)
		msleep(100);
}

/**
 * pci_iov_init - initialize the IOV capability
 * @dev: the PCI device
 *
 * Returns 0 on success, or negative on failure.
 */
int pci_iov_init(struct pci_dev *dev)
{
	int pos;

	if (!pci_is_pcie(dev))
		return -ENODEV;

	pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_SRIOV);
	if (pos)
		return sriov_init(dev, pos);

	return -ENODEV;
}

/**
 * pci_iov_release - release resources used by the IOV capability
 * @dev: the PCI device
 */
void pci_iov_release(struct pci_dev *dev)
{
	if (dev->is_physfn)
		sriov_release(dev);
}

/**
 * pci_iov_resource_bar - get position of the SR-IOV BAR
 * @dev: the PCI device
 * @resno: the resource number
 *
 * Returns position of the BAR encapsulated in the SR-IOV capability.
 */
int pci_iov_resource_bar(struct pci_dev *dev, int resno)
{
	if (resno < PCI_IOV_RESOURCES || resno > PCI_IOV_RESOURCE_END)
		return 0;

	BUG_ON(!dev->is_physfn);

	return dev->sriov->pos + PCI_SRIOV_BAR +
		4 * (resno - PCI_IOV_RESOURCES);
}

/**
 * pci_sriov_resource_alignment - get resource alignment for VF BAR
 * @dev: the PCI device
 * @resno: the resource number
 *
 * Returns the alignment of the VF BAR found in the SR-IOV capability.
 * This is not the same as the resource size which is defined as
 * the VF BAR size multiplied by the number of VFs.  The alignment
 * is just the VF BAR size.
 */
resource_size_t pci_sriov_resource_alignment(struct pci_dev *dev, int resno)
{
	struct resource tmp;
	int reg = pci_iov_resource_bar(dev, resno);

	if (!reg)
		return 0;

	 __pci_read_base(dev, pci_bar_unknown, &tmp, reg);
	return resource_alignment(&tmp);
}

/**
 * pci_restore_iov_state - restore the state of the IOV capability
 * @dev: the PCI device
 */
void pci_restore_iov_state(struct pci_dev *dev)
{
	if (dev->is_physfn)
		sriov_restore_state(dev);
}

/**
 * pci_iov_bus_range - find bus range used by Virtual Function
 * @bus: the PCI bus
 *
 * Returns max number of buses (exclude current one) used by Virtual
 * Functions.
 */
int pci_iov_bus_range(struct pci_bus *bus)
{
	int max = 0;
	u8 busnr;
	struct pci_dev *dev;

	list_for_each_entry(dev, &bus->devices, bus_list) {
		if (!dev->is_physfn)
			continue;
		busnr = virtfn_bus(dev, dev->sriov->total_VFs - 1);
		if (busnr > max)
			max = busnr;
	}

	return max ? max - bus->number : 0;
}

/**
 * pci_enable_sriov - enable the SR-IOV capability
 * @dev: the PCI device
 * @nr_virtfn: number of virtual functions to enable
 *
 * Returns 0 on success, or negative on failure.
 */
int pci_enable_sriov(struct pci_dev *dev, int nr_virtfn)
{
	might_sleep();

	if (!dev->is_physfn)
		return -ENOSYS;

	return sriov_enable(dev, nr_virtfn);
}
EXPORT_SYMBOL_GPL(pci_enable_sriov);

/**
 * pci_disable_sriov - disable the SR-IOV capability
 * @dev: the PCI device
 */
void pci_disable_sriov(struct pci_dev *dev)
{
	might_sleep();

	if (!dev->is_physfn)
		return;

	sriov_disable(dev);
}
EXPORT_SYMBOL_GPL(pci_disable_sriov);

/**
 * pci_num_vf - return number of VFs associated with a PF device_release_driver
 * @dev: the PCI device
 *
 * Returns number of VFs, or 0 if SR-IOV is not enabled.
 */
int pci_num_vf(struct pci_dev *dev)
{
	if (!dev->is_physfn)
		return 0;

	return dev->sriov->num_VFs;
}
EXPORT_SYMBOL_GPL(pci_num_vf);

/**
 * pci_vfs_assigned - returns number of VFs are assigned to a guest
 * @dev: the PCI device
 *
 * Returns number of VFs belonging to this device that are assigned to a guest.
 * If device is not a physical function returns 0.
 */
int pci_vfs_assigned(struct pci_dev *dev)
{
	struct pci_dev *vfdev;
	unsigned int vfs_assigned = 0;
	unsigned short dev_id;

	/* only search if we are a PF */
	if (!dev->is_physfn)
		return 0;

	/*
	 * determine the device ID for the VFs, the vendor ID will be the
	 * same as the PF so there is no need to check for that one
	 */
	pci_read_config_word(dev, dev->sriov->pos + PCI_SRIOV_VF_DID, &dev_id);

	/* loop through all the VFs to see if we own any that are assigned */
	vfdev = pci_get_device(dev->vendor, dev_id, NULL);
	while (vfdev) {
		/*
		 * It is considered assigned if it is a virtual function with
		 * our dev as the physical function and the assigned bit is set
		 */
		if (vfdev->is_virtfn && (vfdev->physfn == dev) &&
			pci_is_dev_assigned(vfdev))
			vfs_assigned++;

		vfdev = pci_get_device(dev->vendor, dev_id, vfdev);
	}

	return vfs_assigned;
}
EXPORT_SYMBOL_GPL(pci_vfs_assigned);

/**
 * pci_sriov_set_totalvfs -- reduce the TotalVFs available
 * @dev: the PCI PF device
 * @numvfs: number that should be used for TotalVFs supported
 *
 * Should be called from PF driver's probe routine with
 * device's mutex held.
 *
 * Returns 0 if PF is an SRIOV-capable device and
 * value of numvfs valid. If not a PF return -ENOSYS;
 * if numvfs is invalid return -EINVAL;
 * if VFs already enabled, return -EBUSY.
 */
int pci_sriov_set_totalvfs(struct pci_dev *dev, u16 numvfs)
{
	if (!dev->is_physfn)
		return -ENOSYS;
	if (numvfs > dev->sriov->total_VFs)
		return -EINVAL;

	/* Shouldn't change if VFs already enabled */
	if (dev->sriov->ctrl & PCI_SRIOV_CTRL_VFE)
		return -EBUSY;
	else
		dev->sriov->driver_max_VFs = numvfs;

	return 0;
}
EXPORT_SYMBOL_GPL(pci_sriov_set_totalvfs);

/**
 * pci_sriov_get_totalvfs -- get total VFs supported on this device
 * @dev: the PCI PF device
 *
 * For a PCIe device with SRIOV support, return the PCIe
 * SRIOV capability value of TotalVFs or the value of driver_max_VFs
 * if the driver reduced it.  Otherwise 0.
 */
int pci_sriov_get_totalvfs(struct pci_dev *dev)
{
	if (!dev->is_physfn)
		return 0;

	if (dev->sriov->driver_max_VFs)
		return dev->sriov->driver_max_VFs;

	return dev->sriov->total_VFs;
}
EXPORT_SYMBOL_GPL(pci_sriov_get_totalvfs);
