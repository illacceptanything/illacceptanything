/*
 * omap_hwmod implementation for OMAP2/3/4
 *
 * Copyright (C) 2009-2010 Nokia Corporation
 *
 * Paul Walmsley, Benoît Cousson, Kevin Hilman
 *
 * Created in collaboration with (alphabetical order): Thara Gopinath,
 * Tony Lindgren, Rajendra Nayak, Vikram Pandita, Sakari Poussa, Anand
 * Sawant, Santosh Shilimkar, Richard Woodruff
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Introduction
 * ------------
 * One way to view an OMAP SoC is as a collection of largely unrelated
 * IP blocks connected by interconnects.  The IP blocks include
 * devices such as ARM processors, audio serial interfaces, UARTs,
 * etc.  Some of these devices, like the DSP, are created by TI;
 * others, like the SGX, largely originate from external vendors.  In
 * TI's documentation, on-chip devices are referred to as "OMAP
 * modules."  Some of these IP blocks are identical across several
 * OMAP versions.  Others are revised frequently.
 *
 * These OMAP modules are tied together by various interconnects.
 * Most of the address and data flow between modules is via OCP-based
 * interconnects such as the L3 and L4 buses; but there are other
 * interconnects that distribute the hardware clock tree, handle idle
 * and reset signaling, supply power, and connect the modules to
 * various pads or balls on the OMAP package.
 *
 * OMAP hwmod provides a consistent way to describe the on-chip
 * hardware blocks and their integration into the rest of the chip.
 * This description can be automatically generated from the TI
 * hardware database.  OMAP hwmod provides a standard, consistent API
 * to reset, enable, idle, and disable these hardware blocks.  And
 * hwmod provides a way for other core code, such as the Linux device
 * code or the OMAP power management and address space mapping code,
 * to query the hardware database.
 *
 * Using hwmod
 * -----------
 * Drivers won't call hwmod functions directly.  That is done by the
 * omap_device code, and in rare occasions, by custom integration code
 * in arch/arm/ *omap*.  The omap_device code includes functions to
 * build a struct platform_device using omap_hwmod data, and that is
 * currently how hwmod data is communicated to drivers and to the
 * Linux driver model.  Most drivers will call omap_hwmod functions only
 * indirectly, via pm_runtime*() functions.
 *
 * From a layering perspective, here is where the OMAP hwmod code
 * fits into the kernel software stack:
 *
 *            +-------------------------------+
 *            |      Device driver code       |
 *            |      (e.g., drivers/)         |
 *            +-------------------------------+
 *            |      Linux driver model       |
 *            |     (platform_device /        |
 *            |  platform_driver data/code)   |
 *            +-------------------------------+
 *            | OMAP core-driver integration  |
 *            |(arch/arm/mach-omap2/devices.c)|
 *            +-------------------------------+
 *            |      omap_device code         |
 *            | (../plat-omap/omap_device.c)  |
 *            +-------------------------------+
 *   ---->    |    omap_hwmod code/data       |    <-----
 *            | (../mach-omap2/omap_hwmod*)   |
 *            +-------------------------------+
 *            | OMAP clock/PRCM/register fns  |
 *            | (__raw_{read,write}l, clk*)   |
 *            +-------------------------------+
 *
 * Device drivers should not contain any OMAP-specific code or data in
 * them.  They should only contain code to operate the IP block that
 * the driver is responsible for.  This is because these IP blocks can
 * also appear in other SoCs, either from TI (such as DaVinci) or from
 * other manufacturers; and drivers should be reusable across other
 * platforms.
 *
 * The OMAP hwmod code also will attempt to reset and idle all on-chip
 * devices upon boot.  The goal here is for the kernel to be
 * completely self-reliant and independent from bootloaders.  This is
 * to ensure a repeatable configuration, both to ensure consistent
 * runtime behavior, and to make it easier for others to reproduce
 * bugs.
 *
 * OMAP module activity states
 * ---------------------------
 * The hwmod code considers modules to be in one of several activity
 * states.  IP blocks start out in an UNKNOWN state, then once they
 * are registered via the hwmod code, proceed to the REGISTERED state.
 * Once their clock names are resolved to clock pointers, the module
 * enters the CLKS_INITED state; and finally, once the module has been
 * reset and the integration registers programmed, the INITIALIZED state
 * is entered.  The hwmod code will then place the module into either
 * the IDLE state to save power, or in the case of a critical system
 * module, the ENABLED state.
 *
 * OMAP core integration code can then call omap_hwmod*() functions
 * directly to move the module between the IDLE, ENABLED, and DISABLED
 * states, as needed.  This is done during both the PM idle loop, and
 * in the OMAP core integration code's implementation of the PM runtime
 * functions.
 *
 * References
 * ----------
 * This is a partial list.
 * - OMAP2420 Multimedia Processor Silicon Revision 2.1.1, 2.2 (SWPU064)
 * - OMAP2430 Multimedia Device POP Silicon Revision 2.1 (SWPU090)
 * - OMAP34xx Multimedia Device Silicon Revision 3.1 (SWPU108)
 * - OMAP4430 Multimedia Device Silicon Revision 1.0 (SWPU140)
 * - Open Core Protocol Specification 2.2
 *
 * To do:
 * - pin mux handling
 * - handle IO mapping
 * - bus throughput & module latency measurement code
 *
 * XXX add tests at the beginning of each function to ensure the hwmod is
 * in the appropriate state
 * XXX error return values should be checked to ensure that they are
 * appropriate
 */
#undef DEBUG

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/mutex.h>

#include <plat/common.h>
#include <plat/cpu.h>
#include <plat/clockdomain.h>
#include <plat/powerdomain.h>
#include <plat/clock.h>
#include <plat/omap_hwmod.h>
#include <plat/prcm.h>

#include "cm.h"
#include "prm.h"

/* Maximum microseconds to wait for OMAP module to softreset */
#define MAX_MODULE_SOFTRESET_WAIT	10000

/* Name of the OMAP hwmod for the MPU */
#define MPU_INITIATOR_NAME		"mpu"

/* omap_hwmod_list contains all registered struct omap_hwmods */
static LIST_HEAD(omap_hwmod_list);

static DEFINE_MUTEX(omap_hwmod_mutex);

/* mpu_oh: used to add/remove MPU initiator from sleepdep list */
static struct omap_hwmod *mpu_oh;

/* inited: 0 if omap_hwmod_init() has not yet been called; 1 otherwise */
static u8 inited;


/* Private functions */

/**
 * _update_sysc_cache - return the module OCP_SYSCONFIG register, keep copy
 * @oh: struct omap_hwmod *
 *
 * Load the current value of the hwmod OCP_SYSCONFIG register into the
 * struct omap_hwmod for later use.  Returns -EINVAL if the hwmod has no
 * OCP_SYSCONFIG register or 0 upon success.
 */
static int _update_sysc_cache(struct omap_hwmod *oh)
{
	if (!oh->class->sysc) {
		WARN(1, "omap_hwmod: %s: cannot read OCP_SYSCONFIG: not defined on hwmod's class\n", oh->name);
		return -EINVAL;
	}

	/* XXX ensure module interface clock is up */

	oh->_sysc_cache = omap_hwmod_read(oh, oh->class->sysc->sysc_offs);

	if (!(oh->class->sysc->sysc_flags & SYSC_NO_CACHE))
		oh->_int_flags |= _HWMOD_SYSCONFIG_LOADED;

	return 0;
}

/**
 * _write_sysconfig - write a value to the module's OCP_SYSCONFIG register
 * @v: OCP_SYSCONFIG value to write
 * @oh: struct omap_hwmod *
 *
 * Write @v into the module class' OCP_SYSCONFIG register, if it has
 * one.  No return value.
 */
static void _write_sysconfig(u32 v, struct omap_hwmod *oh)
{
	if (!oh->class->sysc) {
		WARN(1, "omap_hwmod: %s: cannot write OCP_SYSCONFIG: not defined on hwmod's class\n", oh->name);
		return;
	}

	/* XXX ensure module interface clock is up */

	if (oh->_sysc_cache != v) {
		oh->_sysc_cache = v;
		omap_hwmod_write(v, oh, oh->class->sysc->sysc_offs);
	}
}

/**
 * _set_master_standbymode: set the OCP_SYSCONFIG MIDLEMODE field in @v
 * @oh: struct omap_hwmod *
 * @standbymode: MIDLEMODE field bits
 * @v: pointer to register contents to modify
 *
 * Update the master standby mode bits in @v to be @standbymode for
 * the @oh hwmod.  Does not write to the hardware.  Returns -EINVAL
 * upon error or 0 upon success.
 */
static int _set_master_standbymode(struct omap_hwmod *oh, u8 standbymode,
				   u32 *v)
{
	u32 mstandby_mask;
	u8 mstandby_shift;

	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_MIDLEMODE))
		return -EINVAL;

	if (!oh->class->sysc->sysc_fields) {
		WARN(1, "omap_hwmod: %s: offset struct for sysconfig not provided in class\n", oh->name);
		return -EINVAL;
	}

	mstandby_shift = oh->class->sysc->sysc_fields->midle_shift;
	mstandby_mask = (0x3 << mstandby_shift);

	*v &= ~mstandby_mask;
	*v |= __ffs(standbymode) << mstandby_shift;

	return 0;
}

/**
 * _set_slave_idlemode: set the OCP_SYSCONFIG SIDLEMODE field in @v
 * @oh: struct omap_hwmod *
 * @idlemode: SIDLEMODE field bits
 * @v: pointer to register contents to modify
 *
 * Update the slave idle mode bits in @v to be @idlemode for the @oh
 * hwmod.  Does not write to the hardware.  Returns -EINVAL upon error
 * or 0 upon success.
 */
static int _set_slave_idlemode(struct omap_hwmod *oh, u8 idlemode, u32 *v)
{
	u32 sidle_mask;
	u8 sidle_shift;

	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_SIDLEMODE))
		return -EINVAL;

	if (!oh->class->sysc->sysc_fields) {
		WARN(1, "omap_hwmod: %s: offset struct for sysconfig not provided in class\n", oh->name);
		return -EINVAL;
	}

	sidle_shift = oh->class->sysc->sysc_fields->sidle_shift;
	sidle_mask = (0x3 << sidle_shift);

	*v &= ~sidle_mask;
	*v |= __ffs(idlemode) << sidle_shift;

	return 0;
}

/**
 * _set_clockactivity: set OCP_SYSCONFIG.CLOCKACTIVITY bits in @v
 * @oh: struct omap_hwmod *
 * @clockact: CLOCKACTIVITY field bits
 * @v: pointer to register contents to modify
 *
 * Update the clockactivity mode bits in @v to be @clockact for the
 * @oh hwmod.  Used for additional powersaving on some modules.  Does
 * not write to the hardware.  Returns -EINVAL upon error or 0 upon
 * success.
 */
static int _set_clockactivity(struct omap_hwmod *oh, u8 clockact, u32 *v)
{
	u32 clkact_mask;
	u8  clkact_shift;

	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_CLOCKACTIVITY))
		return -EINVAL;

	if (!oh->class->sysc->sysc_fields) {
		WARN(1, "omap_hwmod: %s: offset struct for sysconfig not provided in class\n", oh->name);
		return -EINVAL;
	}

	clkact_shift = oh->class->sysc->sysc_fields->clkact_shift;
	clkact_mask = (0x3 << clkact_shift);

	*v &= ~clkact_mask;
	*v |= clockact << clkact_shift;

	return 0;
}

/**
 * _set_softreset: set OCP_SYSCONFIG.CLOCKACTIVITY bits in @v
 * @oh: struct omap_hwmod *
 * @v: pointer to register contents to modify
 *
 * Set the SOFTRESET bit in @v for hwmod @oh.  Returns -EINVAL upon
 * error or 0 upon success.
 */
static int _set_softreset(struct omap_hwmod *oh, u32 *v)
{
	u32 softrst_mask;

	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_SOFTRESET))
		return -EINVAL;

	if (!oh->class->sysc->sysc_fields) {
		WARN(1, "omap_hwmod: %s: offset struct for sysconfig not provided in class\n", oh->name);
		return -EINVAL;
	}

	softrst_mask = (0x1 << oh->class->sysc->sysc_fields->srst_shift);

	*v |= softrst_mask;

	return 0;
}

/**
 * _set_module_autoidle: set the OCP_SYSCONFIG AUTOIDLE field in @v
 * @oh: struct omap_hwmod *
 * @autoidle: desired AUTOIDLE bitfield value (0 or 1)
 * @v: pointer to register contents to modify
 *
 * Update the module autoidle bit in @v to be @autoidle for the @oh
 * hwmod.  The autoidle bit controls whether the module can gate
 * internal clocks automatically when it isn't doing anything; the
 * exact function of this bit varies on a per-module basis.  This
 * function does not write to the hardware.  Returns -EINVAL upon
 * error or 0 upon success.
 */
static int _set_module_autoidle(struct omap_hwmod *oh, u8 autoidle,
				u32 *v)
{
	u32 autoidle_mask;
	u8 autoidle_shift;

	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_AUTOIDLE))
		return -EINVAL;

	if (!oh->class->sysc->sysc_fields) {
		WARN(1, "omap_hwmod: %s: offset struct for sysconfig not provided in class\n", oh->name);
		return -EINVAL;
	}

	autoidle_shift = oh->class->sysc->sysc_fields->autoidle_shift;
	autoidle_mask = (0x3 << autoidle_shift);

	*v &= ~autoidle_mask;
	*v |= autoidle << autoidle_shift;

	return 0;
}

/**
 * _enable_wakeup: set OCP_SYSCONFIG.ENAWAKEUP bit in the hardware
 * @oh: struct omap_hwmod *
 *
 * Allow the hardware module @oh to send wakeups.  Returns -EINVAL
 * upon error or 0 upon success.
 */
static int _enable_wakeup(struct omap_hwmod *oh)
{
	u32 v, wakeup_mask;

	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_ENAWAKEUP))
		return -EINVAL;

	if (!oh->class->sysc->sysc_fields) {
		WARN(1, "omap_hwmod: %s: offset struct for sysconfig not provided in class\n", oh->name);
		return -EINVAL;
	}

	wakeup_mask = (0x1 << oh->class->sysc->sysc_fields->enwkup_shift);

	v = oh->_sysc_cache;
	v |= wakeup_mask;
	_write_sysconfig(v, oh);

	/* XXX test pwrdm_get_wken for this hwmod's subsystem */

	oh->_int_flags |= _HWMOD_WAKEUP_ENABLED;

	return 0;
}

/**
 * _disable_wakeup: clear OCP_SYSCONFIG.ENAWAKEUP bit in the hardware
 * @oh: struct omap_hwmod *
 *
 * Prevent the hardware module @oh to send wakeups.  Returns -EINVAL
 * upon error or 0 upon success.
 */
static int _disable_wakeup(struct omap_hwmod *oh)
{
	u32 v, wakeup_mask;

	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_ENAWAKEUP))
		return -EINVAL;

	if (!oh->class->sysc->sysc_fields) {
		WARN(1, "omap_hwmod: %s: offset struct for sysconfig not provided in class\n", oh->name);
		return -EINVAL;
	}

	wakeup_mask = (0x1 << oh->class->sysc->sysc_fields->enwkup_shift);

	v = oh->_sysc_cache;
	v &= ~wakeup_mask;
	_write_sysconfig(v, oh);

	/* XXX test pwrdm_get_wken for this hwmod's subsystem */

	oh->_int_flags &= ~_HWMOD_WAKEUP_ENABLED;

	return 0;
}

/**
 * _add_initiator_dep: prevent @oh from smart-idling while @init_oh is active
 * @oh: struct omap_hwmod *
 *
 * Prevent the hardware module @oh from entering idle while the
 * hardare module initiator @init_oh is active.  Useful when a module
 * will be accessed by a particular initiator (e.g., if a module will
 * be accessed by the IVA, there should be a sleepdep between the IVA
 * initiator and the module).  Only applies to modules in smart-idle
 * mode.  Returns -EINVAL upon error or passes along
 * clkdm_add_sleepdep() value upon success.
 */
static int _add_initiator_dep(struct omap_hwmod *oh, struct omap_hwmod *init_oh)
{
	if (!oh->_clk)
		return -EINVAL;

	return clkdm_add_sleepdep(oh->_clk->clkdm, init_oh->_clk->clkdm);
}

/**
 * _del_initiator_dep: allow @oh to smart-idle even if @init_oh is active
 * @oh: struct omap_hwmod *
 *
 * Allow the hardware module @oh to enter idle while the hardare
 * module initiator @init_oh is active.  Useful when a module will not
 * be accessed by a particular initiator (e.g., if a module will not
 * be accessed by the IVA, there should be no sleepdep between the IVA
 * initiator and the module).  Only applies to modules in smart-idle
 * mode.  Returns -EINVAL upon error or passes along
 * clkdm_del_sleepdep() value upon success.
 */
static int _del_initiator_dep(struct omap_hwmod *oh, struct omap_hwmod *init_oh)
{
	if (!oh->_clk)
		return -EINVAL;

	return clkdm_del_sleepdep(oh->_clk->clkdm, init_oh->_clk->clkdm);
}

/**
 * _init_main_clk - get a struct clk * for the the hwmod's main functional clk
 * @oh: struct omap_hwmod *
 *
 * Called from _init_clocks().  Populates the @oh _clk (main
 * functional clock pointer) if a main_clk is present.  Returns 0 on
 * success or -EINVAL on error.
 */
static int _init_main_clk(struct omap_hwmod *oh)
{
	int ret = 0;

	if (!oh->main_clk)
		return 0;

	oh->_clk = omap_clk_get_by_name(oh->main_clk);
	if (!oh->_clk) {
		pr_warning("omap_hwmod: %s: cannot clk_get main_clk %s\n",
			   oh->name, oh->main_clk);
		return -EINVAL;
	}

	if (!oh->_clk->clkdm)
		pr_warning("omap_hwmod: %s: missing clockdomain for %s.\n",
			   oh->main_clk, oh->_clk->name);

	return ret;
}

/**
 * _init_interface_clks - get a struct clk * for the the hwmod's interface clks
 * @oh: struct omap_hwmod *
 *
 * Called from _init_clocks().  Populates the @oh OCP slave interface
 * clock pointers.  Returns 0 on success or -EINVAL on error.
 */
static int _init_interface_clks(struct omap_hwmod *oh)
{
	struct clk *c;
	int i;
	int ret = 0;

	if (oh->slaves_cnt == 0)
		return 0;

	for (i = 0; i < oh->slaves_cnt; i++) {
		struct omap_hwmod_ocp_if *os = oh->slaves[i];

		if (!os->clk)
			continue;

		c = omap_clk_get_by_name(os->clk);
		if (!c) {
			pr_warning("omap_hwmod: %s: cannot clk_get interface_clk %s\n",
				   oh->name, os->clk);
			ret = -EINVAL;
		}
		os->_clk = c;
	}

	return ret;
}

/**
 * _init_opt_clk - get a struct clk * for the the hwmod's optional clocks
 * @oh: struct omap_hwmod *
 *
 * Called from _init_clocks().  Populates the @oh omap_hwmod_opt_clk
 * clock pointers.  Returns 0 on success or -EINVAL on error.
 */
static int _init_opt_clks(struct omap_hwmod *oh)
{
	struct omap_hwmod_opt_clk *oc;
	struct clk *c;
	int i;
	int ret = 0;

	for (i = oh->opt_clks_cnt, oc = oh->opt_clks; i > 0; i--, oc++) {
		c = omap_clk_get_by_name(oc->clk);
		if (!c) {
			pr_warning("omap_hwmod: %s: cannot clk_get opt_clk %s\n",
				   oh->name, oc->clk);
			ret = -EINVAL;
		}
		oc->_clk = c;
	}

	return ret;
}

/**
 * _enable_clocks - enable hwmod main clock and interface clocks
 * @oh: struct omap_hwmod *
 *
 * Enables all clocks necessary for register reads and writes to succeed
 * on the hwmod @oh.  Returns 0.
 */
static int _enable_clocks(struct omap_hwmod *oh)
{
	int i;

	pr_debug("omap_hwmod: %s: enabling clocks\n", oh->name);

	if (oh->_clk)
		clk_enable(oh->_clk);

	if (oh->slaves_cnt > 0) {
		for (i = 0; i < oh->slaves_cnt; i++) {
			struct omap_hwmod_ocp_if *os = oh->slaves[i];
			struct clk *c = os->_clk;

			if (c && (os->flags & OCPIF_SWSUP_IDLE))
				clk_enable(c);
		}
	}

	/* The opt clocks are controlled by the device driver. */

	return 0;
}

/**
 * _disable_clocks - disable hwmod main clock and interface clocks
 * @oh: struct omap_hwmod *
 *
 * Disables the hwmod @oh main functional and interface clocks.  Returns 0.
 */
static int _disable_clocks(struct omap_hwmod *oh)
{
	int i;

	pr_debug("omap_hwmod: %s: disabling clocks\n", oh->name);

	if (oh->_clk)
		clk_disable(oh->_clk);

	if (oh->slaves_cnt > 0) {
		for (i = 0; i < oh->slaves_cnt; i++) {
			struct omap_hwmod_ocp_if *os = oh->slaves[i];
			struct clk *c = os->_clk;

			if (c && (os->flags & OCPIF_SWSUP_IDLE))
				clk_disable(c);
		}
	}

	/* The opt clocks are controlled by the device driver. */

	return 0;
}

static void _enable_optional_clocks(struct omap_hwmod *oh)
{
	struct omap_hwmod_opt_clk *oc;
	int i;

	pr_debug("omap_hwmod: %s: enabling optional clocks\n", oh->name);

	for (i = oh->opt_clks_cnt, oc = oh->opt_clks; i > 0; i--, oc++)
		if (oc->_clk) {
			pr_debug("omap_hwmod: enable %s:%s\n", oc->role,
				 oc->_clk->name);
			clk_enable(oc->_clk);
		}
}

static void _disable_optional_clocks(struct omap_hwmod *oh)
{
	struct omap_hwmod_opt_clk *oc;
	int i;

	pr_debug("omap_hwmod: %s: disabling optional clocks\n", oh->name);

	for (i = oh->opt_clks_cnt, oc = oh->opt_clks; i > 0; i--, oc++)
		if (oc->_clk) {
			pr_debug("omap_hwmod: disable %s:%s\n", oc->role,
				 oc->_clk->name);
			clk_disable(oc->_clk);
		}
}

/**
 * _find_mpu_port_index - find hwmod OCP slave port ID intended for MPU use
 * @oh: struct omap_hwmod *
 *
 * Returns the array index of the OCP slave port that the MPU
 * addresses the device on, or -EINVAL upon error or not found.
 */
static int _find_mpu_port_index(struct omap_hwmod *oh)
{
	int i;
	int found = 0;

	if (!oh || oh->slaves_cnt == 0)
		return -EINVAL;

	for (i = 0; i < oh->slaves_cnt; i++) {
		struct omap_hwmod_ocp_if *os = oh->slaves[i];

		if (os->user & OCP_USER_MPU) {
			found = 1;
			break;
		}
	}

	if (found)
		pr_debug("omap_hwmod: %s: MPU OCP slave port ID  %d\n",
			 oh->name, i);
	else
		pr_debug("omap_hwmod: %s: no MPU OCP slave port found\n",
			 oh->name);

	return (found) ? i : -EINVAL;
}

/**
 * _find_mpu_rt_base - find hwmod register target base addr accessible by MPU
 * @oh: struct omap_hwmod *
 *
 * Return the virtual address of the base of the register target of
 * device @oh, or NULL on error.
 */
static void __iomem *_find_mpu_rt_base(struct omap_hwmod *oh, u8 index)
{
	struct omap_hwmod_ocp_if *os;
	struct omap_hwmod_addr_space *mem;
	int i;
	int found = 0;
	void __iomem *va_start;

	if (!oh || oh->slaves_cnt == 0)
		return NULL;

	os = oh->slaves[index];

	for (i = 0, mem = os->addr; i < os->addr_cnt; i++, mem++) {
		if (mem->flags & ADDR_TYPE_RT) {
			found = 1;
			break;
		}
	}

	if (found) {
		va_start = ioremap(mem->pa_start, mem->pa_end - mem->pa_start);
		if (!va_start) {
			pr_err("omap_hwmod: %s: Could not ioremap\n", oh->name);
			return NULL;
		}
		pr_debug("omap_hwmod: %s: MPU register target at va %p\n",
			 oh->name, va_start);
	} else {
		pr_debug("omap_hwmod: %s: no MPU register target found\n",
			 oh->name);
	}

	return (found) ? va_start : NULL;
}

/**
 * _enable_sysc - try to bring a module out of idle via OCP_SYSCONFIG
 * @oh: struct omap_hwmod *
 *
 * If module is marked as SWSUP_SIDLE, force the module out of slave
 * idle; otherwise, configure it for smart-idle.  If module is marked
 * as SWSUP_MSUSPEND, force the module out of master standby;
 * otherwise, configure it for smart-standby.  No return value.
 */
static void _enable_sysc(struct omap_hwmod *oh)
{
	u8 idlemode, sf;
	u32 v;

	if (!oh->class->sysc)
		return;

	v = oh->_sysc_cache;
	sf = oh->class->sysc->sysc_flags;

	if (sf & SYSC_HAS_SIDLEMODE) {
		idlemode = (oh->flags & HWMOD_SWSUP_SIDLE) ?
			HWMOD_IDLEMODE_NO : HWMOD_IDLEMODE_SMART;
		_set_slave_idlemode(oh, idlemode, &v);
	}

	if (sf & SYSC_HAS_MIDLEMODE) {
		idlemode = (oh->flags & HWMOD_SWSUP_MSTANDBY) ?
			HWMOD_IDLEMODE_NO : HWMOD_IDLEMODE_SMART;
		_set_master_standbymode(oh, idlemode, &v);
	}

	/*
	 * XXX The clock framework should handle this, by
	 * calling into this code.  But this must wait until the
	 * clock structures are tagged with omap_hwmod entries
	 */
	if ((oh->flags & HWMOD_SET_DEFAULT_CLOCKACT) &&
	    (sf & SYSC_HAS_CLOCKACTIVITY))
		_set_clockactivity(oh, oh->class->sysc->clockact, &v);

	_write_sysconfig(v, oh);

	/* If slave is in SMARTIDLE, also enable wakeup */
	if ((sf & SYSC_HAS_SIDLEMODE) && !(oh->flags & HWMOD_SWSUP_SIDLE))
		_enable_wakeup(oh);

	/*
	 * Set the autoidle bit only after setting the smartidle bit
	 * Setting this will not have any impact on the other modules.
	 */
	if (sf & SYSC_HAS_AUTOIDLE) {
		idlemode = (oh->flags & HWMOD_NO_OCP_AUTOIDLE) ?
			0 : 1;
		_set_module_autoidle(oh, idlemode, &v);
		_write_sysconfig(v, oh);
	}
}

/**
 * _idle_sysc - try to put a module into idle via OCP_SYSCONFIG
 * @oh: struct omap_hwmod *
 *
 * If module is marked as SWSUP_SIDLE, force the module into slave
 * idle; otherwise, configure it for smart-idle.  If module is marked
 * as SWSUP_MSUSPEND, force the module into master standby; otherwise,
 * configure it for smart-standby.  No return value.
 */
static void _idle_sysc(struct omap_hwmod *oh)
{
	u8 idlemode, sf;
	u32 v;

	if (!oh->class->sysc)
		return;

	v = oh->_sysc_cache;
	sf = oh->class->sysc->sysc_flags;

	if (sf & SYSC_HAS_SIDLEMODE) {
		idlemode = (oh->flags & HWMOD_SWSUP_SIDLE) ?
			HWMOD_IDLEMODE_FORCE : HWMOD_IDLEMODE_SMART;
		_set_slave_idlemode(oh, idlemode, &v);
	}

	if (sf & SYSC_HAS_MIDLEMODE) {
		idlemode = (oh->flags & HWMOD_SWSUP_MSTANDBY) ?
			HWMOD_IDLEMODE_FORCE : HWMOD_IDLEMODE_SMART;
		_set_master_standbymode(oh, idlemode, &v);
	}

	_write_sysconfig(v, oh);
}

/**
 * _shutdown_sysc - force a module into idle via OCP_SYSCONFIG
 * @oh: struct omap_hwmod *
 *
 * Force the module into slave idle and master suspend. No return
 * value.
 */
static void _shutdown_sysc(struct omap_hwmod *oh)
{
	u32 v;
	u8 sf;

	if (!oh->class->sysc)
		return;

	v = oh->_sysc_cache;
	sf = oh->class->sysc->sysc_flags;

	if (sf & SYSC_HAS_SIDLEMODE)
		_set_slave_idlemode(oh, HWMOD_IDLEMODE_FORCE, &v);

	if (sf & SYSC_HAS_MIDLEMODE)
		_set_master_standbymode(oh, HWMOD_IDLEMODE_FORCE, &v);

	if (sf & SYSC_HAS_AUTOIDLE)
		_set_module_autoidle(oh, 1, &v);

	_write_sysconfig(v, oh);
}

/**
 * _lookup - find an omap_hwmod by name
 * @name: find an omap_hwmod by name
 *
 * Return a pointer to an omap_hwmod by name, or NULL if not found.
 * Caller must hold omap_hwmod_mutex.
 */
static struct omap_hwmod *_lookup(const char *name)
{
	struct omap_hwmod *oh, *temp_oh;

	oh = NULL;

	list_for_each_entry(temp_oh, &omap_hwmod_list, node) {
		if (!strcmp(name, temp_oh->name)) {
			oh = temp_oh;
			break;
		}
	}

	return oh;
}

/**
 * _init_clocks - clk_get() all clocks associated with this hwmod
 * @oh: struct omap_hwmod *
 * @data: not used; pass NULL
 *
 * Called by omap_hwmod_late_init() (after omap2_clk_init()).
 * Resolves all clock names embedded in the hwmod.  Returns -EINVAL if
 * the omap_hwmod has not yet been registered or if the clocks have
 * already been initialized, 0 on success, or a non-zero error on
 * failure.
 */
static int _init_clocks(struct omap_hwmod *oh, void *data)
{
	int ret = 0;

	if (!oh || (oh->_state != _HWMOD_STATE_REGISTERED))
		return -EINVAL;

	pr_debug("omap_hwmod: %s: looking up clocks\n", oh->name);

	ret |= _init_main_clk(oh);
	ret |= _init_interface_clks(oh);
	ret |= _init_opt_clks(oh);

	if (!ret)
		oh->_state = _HWMOD_STATE_CLKS_INITED;

	return 0;
}

/**
 * _wait_target_ready - wait for a module to leave slave idle
 * @oh: struct omap_hwmod *
 *
 * Wait for a module @oh to leave slave idle.  Returns 0 if the module
 * does not have an IDLEST bit or if the module successfully leaves
 * slave idle; otherwise, pass along the return value of the
 * appropriate *_cm_wait_module_ready() function.
 */
static int _wait_target_ready(struct omap_hwmod *oh)
{
	struct omap_hwmod_ocp_if *os;
	int ret;

	if (!oh)
		return -EINVAL;

	if (oh->_int_flags & _HWMOD_NO_MPU_PORT)
		return 0;

	os = oh->slaves[oh->_mpu_port_index];

	if (oh->flags & HWMOD_NO_IDLEST)
		return 0;

	/* XXX check module SIDLEMODE */

	/* XXX check clock enable states */

	if (cpu_is_omap24xx() || cpu_is_omap34xx()) {
		ret = omap2_cm_wait_module_ready(oh->prcm.omap2.module_offs,
						 oh->prcm.omap2.idlest_reg_id,
						 oh->prcm.omap2.idlest_idle_bit);
	} else if (cpu_is_omap44xx()) {
		ret = omap4_cm_wait_module_ready(oh->prcm.omap4.clkctrl_reg);
	} else {
		BUG();
	};

	return ret;
}

/**
 * _lookup_hardreset - return the register bit shift for this hwmod/reset line
 * @oh: struct omap_hwmod *
 * @name: name of the reset line in the context of this hwmod
 *
 * Return the bit position of the reset line that match the
 * input name. Return -ENOENT if not found.
 */
static u8 _lookup_hardreset(struct omap_hwmod *oh, const char *name)
{
	int i;

	for (i = 0; i < oh->rst_lines_cnt; i++) {
		const char *rst_line = oh->rst_lines[i].name;
		if (!strcmp(rst_line, name)) {
			u8 shift = oh->rst_lines[i].rst_shift;
			pr_debug("omap_hwmod: %s: _lookup_hardreset: %s: %d\n",
				 oh->name, rst_line, shift);

			return shift;
		}
	}

	return -ENOENT;
}

/**
 * _assert_hardreset - assert the HW reset line of submodules
 * contained in the hwmod module.
 * @oh: struct omap_hwmod *
 * @name: name of the reset line to lookup and assert
 *
 * Some IP like dsp, ipu or iva contain processor that require
 * an HW reset line to be assert / deassert in order to enable fully
 * the IP.
 */
static int _assert_hardreset(struct omap_hwmod *oh, const char *name)
{
	u8 shift;

	if (!oh)
		return -EINVAL;

	shift = _lookup_hardreset(oh, name);
	if (IS_ERR_VALUE(shift))
		return shift;

	if (cpu_is_omap24xx() || cpu_is_omap34xx())
		return omap2_prm_assert_hardreset(oh->prcm.omap2.module_offs,
						  shift);
	else if (cpu_is_omap44xx())
		return omap4_prm_assert_hardreset(oh->prcm.omap4.rstctrl_reg,
						  shift);
	else
		return -EINVAL;
}

/**
 * _deassert_hardreset - deassert the HW reset line of submodules contained
 * in the hwmod module.
 * @oh: struct omap_hwmod *
 * @name: name of the reset line to look up and deassert
 *
 * Some IP like dsp, ipu or iva contain processor that require
 * an HW reset line to be assert / deassert in order to enable fully
 * the IP.
 */
static int _deassert_hardreset(struct omap_hwmod *oh, const char *name)
{
	u8 shift;
	int r;

	if (!oh)
		return -EINVAL;

	shift = _lookup_hardreset(oh, name);
	if (IS_ERR_VALUE(shift))
		return shift;

	if (cpu_is_omap24xx() || cpu_is_omap34xx())
		r = omap2_prm_deassert_hardreset(oh->prcm.omap2.module_offs,
						 shift);
	else if (cpu_is_omap44xx())
		r = omap4_prm_deassert_hardreset(oh->prcm.omap4.rstctrl_reg,
						 shift);
	else
		return -EINVAL;

	if (r == -EBUSY)
		pr_warning("omap_hwmod: %s: failed to hardreset\n", oh->name);

	return r;
}

/**
 * _read_hardreset - read the HW reset line state of submodules
 * contained in the hwmod module
 * @oh: struct omap_hwmod *
 * @name: name of the reset line to look up and read
 *
 * Return the state of the reset line.
 */
static int _read_hardreset(struct omap_hwmod *oh, const char *name)
{
	u8 shift;

	if (!oh)
		return -EINVAL;

	shift = _lookup_hardreset(oh, name);
	if (IS_ERR_VALUE(shift))
		return shift;

	if (cpu_is_omap24xx() || cpu_is_omap34xx()) {
		return omap2_prm_is_hardreset_asserted(oh->prcm.omap2.module_offs,
						       shift);
	} else if (cpu_is_omap44xx()) {
		return omap4_prm_is_hardreset_asserted(oh->prcm.omap4.rstctrl_reg,
						       shift);
	} else {
		return -EINVAL;
	}
}

/**
 * _reset - reset an omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Resets an omap_hwmod @oh via the OCP_SYSCONFIG bit.  hwmod must be
 * enabled for this to work.  Returns -EINVAL if the hwmod cannot be
 * reset this way or if the hwmod is in the wrong state, -ETIMEDOUT if
 * the module did not reset in time, or 0 upon success.
 *
 * In OMAP3 a specific SYSSTATUS register is used to get the reset status.
 * Starting in OMAP4, some IPs does not have SYSSTATUS register and instead
 * use the SYSCONFIG softreset bit to provide the status.
 *
 * Note that some IP like McBSP does have a reset control but no reset status.
 */
static int _reset(struct omap_hwmod *oh)
{
	u32 v;
	int c = 0;
	int ret = 0;

	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_SOFTRESET))
		return -EINVAL;

	/* clocks must be on for this operation */
	if (oh->_state != _HWMOD_STATE_ENABLED) {
		pr_warning("omap_hwmod: %s: reset can only be entered from "
			   "enabled state\n", oh->name);
		return -EINVAL;
	}

	/* For some modules, all optionnal clocks need to be enabled as well */
	if (oh->flags & HWMOD_CONTROL_OPT_CLKS_IN_RESET)
		_enable_optional_clocks(oh);

	pr_debug("omap_hwmod: %s: resetting\n", oh->name);

	v = oh->_sysc_cache;
	ret = _set_softreset(oh, &v);
	if (ret)
		goto dis_opt_clks;
	_write_sysconfig(v, oh);

	if (oh->class->sysc->sysc_flags & SYSS_HAS_RESET_STATUS)
		omap_test_timeout((omap_hwmod_read(oh,
						    oh->class->sysc->syss_offs)
				   & SYSS_RESETDONE_MASK),
				  MAX_MODULE_SOFTRESET_WAIT, c);
	else if (oh->class->sysc->sysc_flags & SYSC_HAS_RESET_STATUS)
		omap_test_timeout(!(omap_hwmod_read(oh,
						     oh->class->sysc->sysc_offs)
				   & SYSC_TYPE2_SOFTRESET_MASK),
				  MAX_MODULE_SOFTRESET_WAIT, c);

	if (c == MAX_MODULE_SOFTRESET_WAIT)
		pr_warning("omap_hwmod: %s: softreset failed (waited %d usec)\n",
			   oh->name, MAX_MODULE_SOFTRESET_WAIT);
	else
		pr_debug("omap_hwmod: %s: softreset in %d usec\n", oh->name, c);

	/*
	 * XXX add _HWMOD_STATE_WEDGED for modules that don't come back from
	 * _wait_target_ready() or _reset()
	 */

	ret = (c == MAX_MODULE_SOFTRESET_WAIT) ? -ETIMEDOUT : 0;

dis_opt_clks:
	if (oh->flags & HWMOD_CONTROL_OPT_CLKS_IN_RESET)
		_disable_optional_clocks(oh);

	return ret;
}

/**
 * _omap_hwmod_enable - enable an omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Enables an omap_hwmod @oh such that the MPU can access the hwmod's
 * register target.  (This function has a full name --
 * _omap_hwmod_enable() rather than simply _enable() -- because it is
 * currently required by the pm34xx.c idle loop.)  Returns -EINVAL if
 * the hwmod is in the wrong state or passes along the return value of
 * _wait_target_ready().
 */
int _omap_hwmod_enable(struct omap_hwmod *oh)
{
	int r;

	if (oh->_state != _HWMOD_STATE_INITIALIZED &&
	    oh->_state != _HWMOD_STATE_IDLE &&
	    oh->_state != _HWMOD_STATE_DISABLED) {
		WARN(1, "omap_hwmod: %s: enabled state can only be entered "
		     "from initialized, idle, or disabled state\n", oh->name);
		return -EINVAL;
	}

	pr_debug("omap_hwmod: %s: enabling\n", oh->name);

	/*
	 * If an IP contains only one HW reset line, then de-assert it in order
	 * to allow to enable the clocks. Otherwise the PRCM will return
	 * Intransition status, and the init will failed.
	 */
	if ((oh->_state == _HWMOD_STATE_INITIALIZED ||
	     oh->_state == _HWMOD_STATE_DISABLED) && oh->rst_lines_cnt == 1)
		_deassert_hardreset(oh, oh->rst_lines[0].name);

	/* XXX mux balls */

	_add_initiator_dep(oh, mpu_oh);
	_enable_clocks(oh);

	r = _wait_target_ready(oh);
	if (!r) {
		oh->_state = _HWMOD_STATE_ENABLED;

		/* Access the sysconfig only if the target is ready */
		if (oh->class->sysc) {
			if (!(oh->_int_flags & _HWMOD_SYSCONFIG_LOADED))
				_update_sysc_cache(oh);
			_enable_sysc(oh);
		}
	} else {
		pr_debug("omap_hwmod: %s: _wait_target_ready: %d\n",
			 oh->name, r);
	}

	return r;
}

/**
 * _omap_hwmod_idle - idle an omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Idles an omap_hwmod @oh.  This should be called once the hwmod has
 * no further work.  (This function has a full name --
 * _omap_hwmod_idle() rather than simply _idle() -- because it is
 * currently required by the pm34xx.c idle loop.)  Returns -EINVAL if
 * the hwmod is in the wrong state or returns 0.
 */
int _omap_hwmod_idle(struct omap_hwmod *oh)
{
	if (oh->_state != _HWMOD_STATE_ENABLED) {
		WARN(1, "omap_hwmod: %s: idle state can only be entered from "
		     "enabled state\n", oh->name);
		return -EINVAL;
	}

	pr_debug("omap_hwmod: %s: idling\n", oh->name);

	if (oh->class->sysc)
		_idle_sysc(oh);
	_del_initiator_dep(oh, mpu_oh);
	_disable_clocks(oh);

	oh->_state = _HWMOD_STATE_IDLE;

	return 0;
}

/**
 * _shutdown - shutdown an omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Shut down an omap_hwmod @oh.  This should be called when the driver
 * used for the hwmod is removed or unloaded or if the driver is not
 * used by the system.  Returns -EINVAL if the hwmod is in the wrong
 * state or returns 0.
 */
static int _shutdown(struct omap_hwmod *oh)
{
	if (oh->_state != _HWMOD_STATE_IDLE &&
	    oh->_state != _HWMOD_STATE_ENABLED) {
		WARN(1, "omap_hwmod: %s: disabled state can only be entered "
		     "from idle, or enabled state\n", oh->name);
		return -EINVAL;
	}

	pr_debug("omap_hwmod: %s: disabling\n", oh->name);

	if (oh->class->sysc)
		_shutdown_sysc(oh);

	/*
	 * If an IP contains only one HW reset line, then assert it
	 * before disabling the clocks and shutting down the IP.
	 */
	if (oh->rst_lines_cnt == 1)
		_assert_hardreset(oh, oh->rst_lines[0].name);

	/* clocks and deps are already disabled in idle */
	if (oh->_state == _HWMOD_STATE_ENABLED) {
		_del_initiator_dep(oh, mpu_oh);
		/* XXX what about the other system initiators here? dma, dsp */
		_disable_clocks(oh);
	}
	/* XXX Should this code also force-disable the optional clocks? */

	/* XXX mux any associated balls to safe mode */

	oh->_state = _HWMOD_STATE_DISABLED;

	return 0;
}

/**
 * _setup - do initial configuration of omap_hwmod
 * @oh: struct omap_hwmod *
 * @skip_setup_idle_p: do not idle hwmods at the end of the fn if 1
 *
 * Writes the CLOCKACTIVITY bits @clockact to the hwmod @oh
 * OCP_SYSCONFIG register.  @skip_setup_idle is intended to be used on
 * a system that will not call omap_hwmod_enable() to enable devices
 * (e.g., a system without PM runtime).  Returns -EINVAL if the hwmod
 * is in the wrong state or returns 0.
 */
static int _setup(struct omap_hwmod *oh, void *data)
{
	int i, r;
	u8 skip_setup_idle;

	if (!oh || !data)
		return -EINVAL;

	skip_setup_idle = *(u8 *)data;

	/* Set iclk autoidle mode */
	if (oh->slaves_cnt > 0) {
		for (i = 0; i < oh->slaves_cnt; i++) {
			struct omap_hwmod_ocp_if *os = oh->slaves[i];
			struct clk *c = os->_clk;

			if (!c)
				continue;

			if (os->flags & OCPIF_SWSUP_IDLE) {
				/* XXX omap_iclk_deny_idle(c); */
			} else {
				/* XXX omap_iclk_allow_idle(c); */
				clk_enable(c);
			}
		}
	}

	mutex_init(&oh->_mutex);
	oh->_state = _HWMOD_STATE_INITIALIZED;

	/*
	 * In the case of hwmod with hardreset that should not be
	 * de-assert at boot time, we have to keep the module
	 * initialized, because we cannot enable it properly with the
	 * reset asserted. Exit without warning because that behavior is
	 * expected.
	 */
	if ((oh->flags & HWMOD_INIT_NO_RESET) && oh->rst_lines_cnt == 1)
		return 0;

	r = _omap_hwmod_enable(oh);
	if (r) {
		pr_warning("omap_hwmod: %s: cannot be enabled (%d)\n",
			   oh->name, oh->_state);
		return 0;
	}

	if (!(oh->flags & HWMOD_INIT_NO_RESET)) {
		_reset(oh);

		/*
		 * OCP_SYSCONFIG bits need to be reprogrammed after a softreset.
		 * The _omap_hwmod_enable() function should be split to
		 * avoid the rewrite of the OCP_SYSCONFIG register.
		 */
		if (oh->class->sysc) {
			_update_sysc_cache(oh);
			_enable_sysc(oh);
		}
	}

	if (!(oh->flags & HWMOD_INIT_NO_IDLE) && !skip_setup_idle)
		_omap_hwmod_idle(oh);

	return 0;
}



/* Public functions */

u32 omap_hwmod_read(struct omap_hwmod *oh, u16 reg_offs)
{
	if (oh->flags & HWMOD_16BIT_REG)
		return __raw_readw(oh->_mpu_rt_va + reg_offs);
	else
		return __raw_readl(oh->_mpu_rt_va + reg_offs);
}

void omap_hwmod_write(u32 v, struct omap_hwmod *oh, u16 reg_offs)
{
	if (oh->flags & HWMOD_16BIT_REG)
		__raw_writew(v, oh->_mpu_rt_va + reg_offs);
	else
		__raw_writel(v, oh->_mpu_rt_va + reg_offs);
}

/**
 * omap_hwmod_set_slave_idlemode - set the hwmod's OCP slave idlemode
 * @oh: struct omap_hwmod *
 * @idlemode: SIDLEMODE field bits (shifted to bit 0)
 *
 * Sets the IP block's OCP slave idlemode in hardware, and updates our
 * local copy.  Intended to be used by drivers that have some erratum
 * that requires direct manipulation of the SIDLEMODE bits.  Returns
 * -EINVAL if @oh is null, or passes along the return value from
 * _set_slave_idlemode().
 *
 * XXX Does this function have any current users?  If not, we should
 * remove it; it is better to let the rest of the hwmod code handle this.
 * Any users of this function should be scrutinized carefully.
 */
int omap_hwmod_set_slave_idlemode(struct omap_hwmod *oh, u8 idlemode)
{
	u32 v;
	int retval = 0;

	if (!oh)
		return -EINVAL;

	v = oh->_sysc_cache;

	retval = _set_slave_idlemode(oh, idlemode, &v);
	if (!retval)
		_write_sysconfig(v, oh);

	return retval;
}

/**
 * omap_hwmod_register - register a struct omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Registers the omap_hwmod @oh.  Returns -EEXIST if an omap_hwmod
 * already has been registered by the same name; -EINVAL if the
 * omap_hwmod is in the wrong state, if @oh is NULL, if the
 * omap_hwmod's class field is NULL; if the omap_hwmod is missing a
 * name, or if the omap_hwmod's class is missing a name; or 0 upon
 * success.
 *
 * XXX The data should be copied into bootmem, so the original data
 * should be marked __initdata and freed after init.  This would allow
 * unneeded omap_hwmods to be freed on multi-OMAP configurations.  Note
 * that the copy process would be relatively complex due to the large number
 * of substructures.
 */
int omap_hwmod_register(struct omap_hwmod *oh)
{
	int ret, ms_id;

	if (!oh || !oh->name || !oh->class || !oh->class->name ||
	    (oh->_state != _HWMOD_STATE_UNKNOWN))
		return -EINVAL;

	mutex_lock(&omap_hwmod_mutex);

	pr_debug("omap_hwmod: %s: registering\n", oh->name);

	if (_lookup(oh->name)) {
		ret = -EEXIST;
		goto ohr_unlock;
	}

	ms_id = _find_mpu_port_index(oh);
	if (!IS_ERR_VALUE(ms_id)) {
		oh->_mpu_port_index = ms_id;
		oh->_mpu_rt_va = _find_mpu_rt_base(oh, oh->_mpu_port_index);
	} else {
		oh->_int_flags |= _HWMOD_NO_MPU_PORT;
	}

	list_add_tail(&oh->node, &omap_hwmod_list);

	oh->_state = _HWMOD_STATE_REGISTERED;

	ret = 0;

ohr_unlock:
	mutex_unlock(&omap_hwmod_mutex);
	return ret;
}

/**
 * omap_hwmod_lookup - look up a registered omap_hwmod by name
 * @name: name of the omap_hwmod to look up
 *
 * Given a @name of an omap_hwmod, return a pointer to the registered
 * struct omap_hwmod *, or NULL upon error.
 */
struct omap_hwmod *omap_hwmod_lookup(const char *name)
{
	struct omap_hwmod *oh;

	if (!name)
		return NULL;

	mutex_lock(&omap_hwmod_mutex);
	oh = _lookup(name);
	mutex_unlock(&omap_hwmod_mutex);

	return oh;
}

/**
 * omap_hwmod_for_each - call function for each registered omap_hwmod
 * @fn: pointer to a callback function
 * @data: void * data to pass to callback function
 *
 * Call @fn for each registered omap_hwmod, passing @data to each
 * function.  @fn must return 0 for success or any other value for
 * failure.  If @fn returns non-zero, the iteration across omap_hwmods
 * will stop and the non-zero return value will be passed to the
 * caller of omap_hwmod_for_each().  @fn is called with
 * omap_hwmod_for_each() held.
 */
int omap_hwmod_for_each(int (*fn)(struct omap_hwmod *oh, void *data),
			void *data)
{
	struct omap_hwmod *temp_oh;
	int ret;

	if (!fn)
		return -EINVAL;

	mutex_lock(&omap_hwmod_mutex);
	list_for_each_entry(temp_oh, &omap_hwmod_list, node) {
		ret = (*fn)(temp_oh, data);
		if (ret)
			break;
	}
	mutex_unlock(&omap_hwmod_mutex);

	return ret;
}


/**
 * omap_hwmod_init - init omap_hwmod code and register hwmods
 * @ohs: pointer to an array of omap_hwmods to register
 *
 * Intended to be called early in boot before the clock framework is
 * initialized.  If @ohs is not null, will register all omap_hwmods
 * listed in @ohs that are valid for this chip.  Returns -EINVAL if
 * omap_hwmod_init() has already been called or 0 otherwise.
 */
int omap_hwmod_init(struct omap_hwmod **ohs)
{
	struct omap_hwmod *oh;
	int r;

	if (inited)
		return -EINVAL;

	inited = 1;

	if (!ohs)
		return 0;

	oh = *ohs;
	while (oh) {
		if (omap_chip_is(oh->omap_chip)) {
			r = omap_hwmod_register(oh);
			WARN(r, "omap_hwmod: %s: omap_hwmod_register returned "
			     "%d\n", oh->name, r);
		}
		oh = *++ohs;
	}

	return 0;
}

/**
 * omap_hwmod_late_init - do some post-clock framework initialization
 * @skip_setup_idle: if 1, do not idle hwmods in _setup()
 *
 * Must be called after omap2_clk_init().  Resolves the struct clk names
 * to struct clk pointers for each registered omap_hwmod.  Also calls
 * _setup() on each hwmod.  Returns 0.
 */
int omap_hwmod_late_init(u8 skip_setup_idle)
{
	int r;

	/* XXX check return value */
	r = omap_hwmod_for_each(_init_clocks, NULL);
	WARN(r, "omap_hwmod: omap_hwmod_late_init(): _init_clocks failed\n");

	mpu_oh = omap_hwmod_lookup(MPU_INITIATOR_NAME);
	WARN(!mpu_oh, "omap_hwmod: could not find MPU initiator hwmod %s\n",
	     MPU_INITIATOR_NAME);

	if (skip_setup_idle)
		pr_debug("omap_hwmod: will leave hwmods enabled during setup\n");

	omap_hwmod_for_each(_setup, &skip_setup_idle);

	return 0;
}

/**
 * omap_hwmod_unregister - unregister an omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Unregisters a previously-registered omap_hwmod @oh.  There's probably
 * no use case for this, so it is likely to be removed in a later version.
 *
 * XXX Free all of the bootmem-allocated structures here when that is
 * implemented.  Make it clear that core code is the only code that is
 * expected to unregister modules.
 */
int omap_hwmod_unregister(struct omap_hwmod *oh)
{
	if (!oh)
		return -EINVAL;

	pr_debug("omap_hwmod: %s: unregistering\n", oh->name);

	mutex_lock(&omap_hwmod_mutex);
	iounmap(oh->_mpu_rt_va);
	list_del(&oh->node);
	mutex_unlock(&omap_hwmod_mutex);

	return 0;
}

/**
 * omap_hwmod_enable - enable an omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Enable an omap_hwmod @oh.  Intended to be called by omap_device_enable().
 * Returns -EINVAL on error or passes along the return value from _enable().
 */
int omap_hwmod_enable(struct omap_hwmod *oh)
{
	int r;

	if (!oh)
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	r = _omap_hwmod_enable(oh);
	mutex_unlock(&oh->_mutex);

	return r;
}


/**
 * omap_hwmod_idle - idle an omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Idle an omap_hwmod @oh.  Intended to be called by omap_device_idle().
 * Returns -EINVAL on error or passes along the return value from _idle().
 */
int omap_hwmod_idle(struct omap_hwmod *oh)
{
	if (!oh)
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	_omap_hwmod_idle(oh);
	mutex_unlock(&oh->_mutex);

	return 0;
}

/**
 * omap_hwmod_shutdown - shutdown an omap_hwmod
 * @oh: struct omap_hwmod *
 *
 * Shutdown an omap_hwmod @oh.  Intended to be called by
 * omap_device_shutdown().  Returns -EINVAL on error or passes along
 * the return value from _shutdown().
 */
int omap_hwmod_shutdown(struct omap_hwmod *oh)
{
	if (!oh)
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	_shutdown(oh);
	mutex_unlock(&oh->_mutex);

	return 0;
}

/**
 * omap_hwmod_enable_clocks - enable main_clk, all interface clocks
 * @oh: struct omap_hwmod *oh
 *
 * Intended to be called by the omap_device code.
 */
int omap_hwmod_enable_clocks(struct omap_hwmod *oh)
{
	mutex_lock(&oh->_mutex);
	_enable_clocks(oh);
	mutex_unlock(&oh->_mutex);

	return 0;
}

/**
 * omap_hwmod_disable_clocks - disable main_clk, all interface clocks
 * @oh: struct omap_hwmod *oh
 *
 * Intended to be called by the omap_device code.
 */
int omap_hwmod_disable_clocks(struct omap_hwmod *oh)
{
	mutex_lock(&oh->_mutex);
	_disable_clocks(oh);
	mutex_unlock(&oh->_mutex);

	return 0;
}

/**
 * omap_hwmod_ocp_barrier - wait for posted writes against the hwmod to complete
 * @oh: struct omap_hwmod *oh
 *
 * Intended to be called by drivers and core code when all posted
 * writes to a device must complete before continuing further
 * execution (for example, after clearing some device IRQSTATUS
 * register bits)
 *
 * XXX what about targets with multiple OCP threads?
 */
void omap_hwmod_ocp_barrier(struct omap_hwmod *oh)
{
	BUG_ON(!oh);

	if (!oh->class->sysc || !oh->class->sysc->sysc_flags) {
		WARN(1, "omap_device: %s: OCP barrier impossible due to "
		      "device configuration\n", oh->name);
		return;
	}

	/*
	 * Forces posted writes to complete on the OCP thread handling
	 * register writes
	 */
	omap_hwmod_read(oh, oh->class->sysc->sysc_offs);
}

/**
 * omap_hwmod_reset - reset the hwmod
 * @oh: struct omap_hwmod *
 *
 * Under some conditions, a driver may wish to reset the entire device.
 * Called from omap_device code.  Returns -EINVAL on error or passes along
 * the return value from _reset().
 */
int omap_hwmod_reset(struct omap_hwmod *oh)
{
	int r;

	if (!oh)
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	r = _reset(oh);
	mutex_unlock(&oh->_mutex);

	return r;
}

/**
 * omap_hwmod_count_resources - count number of struct resources needed by hwmod
 * @oh: struct omap_hwmod *
 * @res: pointer to the first element of an array of struct resource to fill
 *
 * Count the number of struct resource array elements necessary to
 * contain omap_hwmod @oh resources.  Intended to be called by code
 * that registers omap_devices.  Intended to be used to determine the
 * size of a dynamically-allocated struct resource array, before
 * calling omap_hwmod_fill_resources().  Returns the number of struct
 * resource array elements needed.
 *
 * XXX This code is not optimized.  It could attempt to merge adjacent
 * resource IDs.
 *
 */
int omap_hwmod_count_resources(struct omap_hwmod *oh)
{
	int ret, i;

	ret = oh->mpu_irqs_cnt + oh->sdma_reqs_cnt;

	for (i = 0; i < oh->slaves_cnt; i++)
		ret += oh->slaves[i]->addr_cnt;

	return ret;
}

/**
 * omap_hwmod_fill_resources - fill struct resource array with hwmod data
 * @oh: struct omap_hwmod *
 * @res: pointer to the first element of an array of struct resource to fill
 *
 * Fill the struct resource array @res with resource data from the
 * omap_hwmod @oh.  Intended to be called by code that registers
 * omap_devices.  See also omap_hwmod_count_resources().  Returns the
 * number of array elements filled.
 */
int omap_hwmod_fill_resources(struct omap_hwmod *oh, struct resource *res)
{
	int i, j;
	int r = 0;

	/* For each IRQ, DMA, memory area, fill in array.*/

	for (i = 0; i < oh->mpu_irqs_cnt; i++) {
		(res + r)->name = (oh->mpu_irqs + i)->name;
		(res + r)->start = (oh->mpu_irqs + i)->irq;
		(res + r)->end = (oh->mpu_irqs + i)->irq;
		(res + r)->flags = IORESOURCE_IRQ;
		r++;
	}

	for (i = 0; i < oh->sdma_reqs_cnt; i++) {
		(res + r)->name = (oh->sdma_reqs + i)->name;
		(res + r)->start = (oh->sdma_reqs + i)->dma_req;
		(res + r)->end = (oh->sdma_reqs + i)->dma_req;
		(res + r)->flags = IORESOURCE_DMA;
		r++;
	}

	for (i = 0; i < oh->slaves_cnt; i++) {
		struct omap_hwmod_ocp_if *os;

		os = oh->slaves[i];

		for (j = 0; j < os->addr_cnt; j++) {
			(res + r)->start = (os->addr + j)->pa_start;
			(res + r)->end = (os->addr + j)->pa_end;
			(res + r)->flags = IORESOURCE_MEM;
			r++;
		}
	}

	return r;
}

/**
 * omap_hwmod_get_pwrdm - return pointer to this module's main powerdomain
 * @oh: struct omap_hwmod *
 *
 * Return the powerdomain pointer associated with the OMAP module
 * @oh's main clock.  If @oh does not have a main clk, return the
 * powerdomain associated with the interface clock associated with the
 * module's MPU port. (XXX Perhaps this should use the SDMA port
 * instead?)  Returns NULL on error, or a struct powerdomain * on
 * success.
 */
struct powerdomain *omap_hwmod_get_pwrdm(struct omap_hwmod *oh)
{
	struct clk *c;

	if (!oh)
		return NULL;

	if (oh->_clk) {
		c = oh->_clk;
	} else {
		if (oh->_int_flags & _HWMOD_NO_MPU_PORT)
			return NULL;
		c = oh->slaves[oh->_mpu_port_index]->_clk;
	}

	if (!c->clkdm)
		return NULL;

	return c->clkdm->pwrdm.ptr;

}

/**
 * omap_hwmod_get_mpu_rt_va - return the module's base address (for the MPU)
 * @oh: struct omap_hwmod *
 *
 * Returns the virtual address corresponding to the beginning of the
 * module's register target, in the address range that is intended to
 * be used by the MPU.  Returns the virtual address upon success or NULL
 * upon error.
 */
void __iomem *omap_hwmod_get_mpu_rt_va(struct omap_hwmod *oh)
{
	if (!oh)
		return NULL;

	if (oh->_int_flags & _HWMOD_NO_MPU_PORT)
		return NULL;

	if (oh->_state == _HWMOD_STATE_UNKNOWN)
		return NULL;

	return oh->_mpu_rt_va;
}

/**
 * omap_hwmod_add_initiator_dep - add sleepdep from @init_oh to @oh
 * @oh: struct omap_hwmod *
 * @init_oh: struct omap_hwmod * (initiator)
 *
 * Add a sleep dependency between the initiator @init_oh and @oh.
 * Intended to be called by DSP/Bridge code via platform_data for the
 * DSP case; and by the DMA code in the sDMA case.  DMA code, *Bridge
 * code needs to add/del initiator dependencies dynamically
 * before/after accessing a device.  Returns the return value from
 * _add_initiator_dep().
 *
 * XXX Keep a usecount in the clockdomain code
 */
int omap_hwmod_add_initiator_dep(struct omap_hwmod *oh,
				 struct omap_hwmod *init_oh)
{
	return _add_initiator_dep(oh, init_oh);
}

/*
 * XXX what about functions for drivers to save/restore ocp_sysconfig
 * for context save/restore operations?
 */

/**
 * omap_hwmod_del_initiator_dep - remove sleepdep from @init_oh to @oh
 * @oh: struct omap_hwmod *
 * @init_oh: struct omap_hwmod * (initiator)
 *
 * Remove a sleep dependency between the initiator @init_oh and @oh.
 * Intended to be called by DSP/Bridge code via platform_data for the
 * DSP case; and by the DMA code in the sDMA case.  DMA code, *Bridge
 * code needs to add/del initiator dependencies dynamically
 * before/after accessing a device.  Returns the return value from
 * _del_initiator_dep().
 *
 * XXX Keep a usecount in the clockdomain code
 */
int omap_hwmod_del_initiator_dep(struct omap_hwmod *oh,
				 struct omap_hwmod *init_oh)
{
	return _del_initiator_dep(oh, init_oh);
}

/**
 * omap_hwmod_enable_wakeup - allow device to wake up the system
 * @oh: struct omap_hwmod *
 *
 * Sets the module OCP socket ENAWAKEUP bit to allow the module to
 * send wakeups to the PRCM.  Eventually this should sets PRCM wakeup
 * registers to cause the PRCM to receive wakeup events from the
 * module.  Does not set any wakeup routing registers beyond this
 * point - if the module is to wake up any other module or subsystem,
 * that must be set separately.  Called by omap_device code.  Returns
 * -EINVAL on error or 0 upon success.
 */
int omap_hwmod_enable_wakeup(struct omap_hwmod *oh)
{
	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_ENAWAKEUP))
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	_enable_wakeup(oh);
	mutex_unlock(&oh->_mutex);

	return 0;
}

/**
 * omap_hwmod_disable_wakeup - prevent device from waking the system
 * @oh: struct omap_hwmod *
 *
 * Clears the module OCP socket ENAWAKEUP bit to prevent the module
 * from sending wakeups to the PRCM.  Eventually this should clear
 * PRCM wakeup registers to cause the PRCM to ignore wakeup events
 * from the module.  Does not set any wakeup routing registers beyond
 * this point - if the module is to wake up any other module or
 * subsystem, that must be set separately.  Called by omap_device
 * code.  Returns -EINVAL on error or 0 upon success.
 */
int omap_hwmod_disable_wakeup(struct omap_hwmod *oh)
{
	if (!oh->class->sysc ||
	    !(oh->class->sysc->sysc_flags & SYSC_HAS_ENAWAKEUP))
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	_disable_wakeup(oh);
	mutex_unlock(&oh->_mutex);

	return 0;
}

/**
 * omap_hwmod_assert_hardreset - assert the HW reset line of submodules
 * contained in the hwmod module.
 * @oh: struct omap_hwmod *
 * @name: name of the reset line to lookup and assert
 *
 * Some IP like dsp, ipu or iva contain processor that require
 * an HW reset line to be assert / deassert in order to enable fully
 * the IP.  Returns -EINVAL if @oh is null or if the operation is not
 * yet supported on this OMAP; otherwise, passes along the return value
 * from _assert_hardreset().
 */
int omap_hwmod_assert_hardreset(struct omap_hwmod *oh, const char *name)
{
	int ret;

	if (!oh)
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	ret = _assert_hardreset(oh, name);
	mutex_unlock(&oh->_mutex);

	return ret;
}

/**
 * omap_hwmod_deassert_hardreset - deassert the HW reset line of submodules
 * contained in the hwmod module.
 * @oh: struct omap_hwmod *
 * @name: name of the reset line to look up and deassert
 *
 * Some IP like dsp, ipu or iva contain processor that require
 * an HW reset line to be assert / deassert in order to enable fully
 * the IP.  Returns -EINVAL if @oh is null or if the operation is not
 * yet supported on this OMAP; otherwise, passes along the return value
 * from _deassert_hardreset().
 */
int omap_hwmod_deassert_hardreset(struct omap_hwmod *oh, const char *name)
{
	int ret;

	if (!oh)
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	ret = _deassert_hardreset(oh, name);
	mutex_unlock(&oh->_mutex);

	return ret;
}

/**
 * omap_hwmod_read_hardreset - read the HW reset line state of submodules
 * contained in the hwmod module
 * @oh: struct omap_hwmod *
 * @name: name of the reset line to look up and read
 *
 * Return the current state of the hwmod @oh's reset line named @name:
 * returns -EINVAL upon parameter error or if this operation
 * is unsupported on the current OMAP; otherwise, passes along the return
 * value from _read_hardreset().
 */
int omap_hwmod_read_hardreset(struct omap_hwmod *oh, const char *name)
{
	int ret;

	if (!oh)
		return -EINVAL;

	mutex_lock(&oh->_mutex);
	ret = _read_hardreset(oh, name);
	mutex_unlock(&oh->_mutex);

	return ret;
}


/**
 * omap_hwmod_for_each_by_class - call @fn for each hwmod of class @classname
 * @classname: struct omap_hwmod_class name to search for
 * @fn: callback function pointer to call for each hwmod in class @classname
 * @user: arbitrary context data to pass to the callback function
 *
 * For each omap_hwmod of class @classname, call @fn.  Takes
 * omap_hwmod_mutex to prevent the hwmod list from changing during the
 * iteration.  If the callback function returns something other than
 * zero, the iterator is terminated, and the callback function's return
 * value is passed back to the caller.  Returns 0 upon success, -EINVAL
 * if @classname or @fn are NULL, or passes back the error code from @fn.
 */
int omap_hwmod_for_each_by_class(const char *classname,
				 int (*fn)(struct omap_hwmod *oh,
					   void *user),
				 void *user)
{
	struct omap_hwmod *temp_oh;
	int ret = 0;

	if (!classname || !fn)
		return -EINVAL;

	pr_debug("omap_hwmod: %s: looking for modules of class %s\n",
		 __func__, classname);

	mutex_lock(&omap_hwmod_mutex);

	list_for_each_entry(temp_oh, &omap_hwmod_list, node) {
		if (!strcmp(temp_oh->class->name, classname)) {
			pr_debug("omap_hwmod: %s: %s: calling callback fn\n",
				 __func__, temp_oh->name);
			ret = (*fn)(temp_oh, user);
			if (ret)
				break;
		}
	}

	mutex_unlock(&omap_hwmod_mutex);

	if (ret)
		pr_debug("omap_hwmod: %s: iterator terminated early: %d\n",
			 __func__, ret);

	return ret;
}

