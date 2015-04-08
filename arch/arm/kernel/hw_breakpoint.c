/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2009, 2010 ARM Limited
 *
 * Author: Will Deacon <will.deacon@arm.com>
 */

/*
 * HW_breakpoint: a unified kernel/user-space hardware breakpoint facility,
 * using the CPU's debug registers.
 */
#define pr_fmt(fmt) "hw-breakpoint: " fmt

#include <linux/errno.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/smp.h>

#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/current.h>
#include <asm/hw_breakpoint.h>
#include <asm/kdebug.h>
#include <asm/system.h>
#include <asm/traps.h>

/* Breakpoint currently in use for each BRP. */
static DEFINE_PER_CPU(struct perf_event *, bp_on_reg[ARM_MAX_BRP]);

/* Watchpoint currently in use for each WRP. */
static DEFINE_PER_CPU(struct perf_event *, wp_on_reg[ARM_MAX_WRP]);

/* Number of BRP/WRP registers on this CPU. */
static int core_num_brps;
static int core_num_wrps;

/* Debug architecture version. */
static u8 debug_arch;

/* Maximum supported watchpoint length. */
static u8 max_watchpoint_len;

/* Determine number of BRP registers available. */
static int get_num_brps(void)
{
	u32 didr;
	ARM_DBG_READ(c0, 0, didr);
	return ((didr >> 24) & 0xf) + 1;
}

/* Determine number of WRP registers available. */
static int get_num_wrps(void)
{
	/*
	 * FIXME: When a watchpoint fires, the only way to work out which
	 * watchpoint it was is by disassembling the faulting instruction
	 * and working out the address of the memory access.
	 *
	 * Furthermore, we can only do this if the watchpoint was precise
	 * since imprecise watchpoints prevent us from calculating register
	 * based addresses.
	 *
	 * For the time being, we only report 1 watchpoint register so we
	 * always know which watchpoint fired. In the future we can either
	 * add a disassembler and address generation emulator, or we can
	 * insert a check to see if the DFAR is set on watchpoint exception
	 * entry [the ARM ARM states that the DFAR is UNKNOWN, but
	 * experience shows that it is set on some implementations].
	 */

#if 0
	u32 didr, wrps;
	ARM_DBG_READ(c0, 0, didr);
	return ((didr >> 28) & 0xf) + 1;
#endif

	return 1;
}

int hw_breakpoint_slots(int type)
{
	/*
	 * We can be called early, so don't rely on
	 * our static variables being initialised.
	 */
	switch (type) {
	case TYPE_INST:
		return get_num_brps();
	case TYPE_DATA:
		return get_num_wrps();
	default:
		pr_warning("unknown slot type: %d\n", type);
		return 0;
	}
}

/* Determine debug architecture. */
static u8 get_debug_arch(void)
{
	u32 didr;

	/* Do we implement the extended CPUID interface? */
	if (((read_cpuid_id() >> 16) & 0xf) != 0xf) {
		pr_warning("CPUID feature registers not supported. "
				"Assuming v6 debug is present.\n");
		return ARM_DEBUG_ARCH_V6;
	}

	ARM_DBG_READ(c0, 0, didr);
	return (didr >> 16) & 0xf;
}

/* Does this core support mismatch breakpoints? */
static int core_has_mismatch_bps(void)
{
	return debug_arch >= ARM_DEBUG_ARCH_V7_ECP14 && core_num_brps > 1;
}

u8 arch_get_debug_arch(void)
{
	return debug_arch;
}

#define READ_WB_REG_CASE(OP2, M, VAL)		\
	case ((OP2 << 4) + M):			\
		ARM_DBG_READ(c ## M, OP2, VAL); \
		break

#define WRITE_WB_REG_CASE(OP2, M, VAL)		\
	case ((OP2 << 4) + M):			\
		ARM_DBG_WRITE(c ## M, OP2, VAL);\
		break

#define GEN_READ_WB_REG_CASES(OP2, VAL)		\
	READ_WB_REG_CASE(OP2, 0, VAL);		\
	READ_WB_REG_CASE(OP2, 1, VAL);		\
	READ_WB_REG_CASE(OP2, 2, VAL);		\
	READ_WB_REG_CASE(OP2, 3, VAL);		\
	READ_WB_REG_CASE(OP2, 4, VAL);		\
	READ_WB_REG_CASE(OP2, 5, VAL);		\
	READ_WB_REG_CASE(OP2, 6, VAL);		\
	READ_WB_REG_CASE(OP2, 7, VAL);		\
	READ_WB_REG_CASE(OP2, 8, VAL);		\
	READ_WB_REG_CASE(OP2, 9, VAL);		\
	READ_WB_REG_CASE(OP2, 10, VAL);		\
	READ_WB_REG_CASE(OP2, 11, VAL);		\
	READ_WB_REG_CASE(OP2, 12, VAL);		\
	READ_WB_REG_CASE(OP2, 13, VAL);		\
	READ_WB_REG_CASE(OP2, 14, VAL);		\
	READ_WB_REG_CASE(OP2, 15, VAL)

#define GEN_WRITE_WB_REG_CASES(OP2, VAL)	\
	WRITE_WB_REG_CASE(OP2, 0, VAL);		\
	WRITE_WB_REG_CASE(OP2, 1, VAL);		\
	WRITE_WB_REG_CASE(OP2, 2, VAL);		\
	WRITE_WB_REG_CASE(OP2, 3, VAL);		\
	WRITE_WB_REG_CASE(OP2, 4, VAL);		\
	WRITE_WB_REG_CASE(OP2, 5, VAL);		\
	WRITE_WB_REG_CASE(OP2, 6, VAL);		\
	WRITE_WB_REG_CASE(OP2, 7, VAL);		\
	WRITE_WB_REG_CASE(OP2, 8, VAL);		\
	WRITE_WB_REG_CASE(OP2, 9, VAL);		\
	WRITE_WB_REG_CASE(OP2, 10, VAL);	\
	WRITE_WB_REG_CASE(OP2, 11, VAL);	\
	WRITE_WB_REG_CASE(OP2, 12, VAL);	\
	WRITE_WB_REG_CASE(OP2, 13, VAL);	\
	WRITE_WB_REG_CASE(OP2, 14, VAL);	\
	WRITE_WB_REG_CASE(OP2, 15, VAL)

static u32 read_wb_reg(int n)
{
	u32 val = 0;

	switch (n) {
	GEN_READ_WB_REG_CASES(ARM_OP2_BVR, val);
	GEN_READ_WB_REG_CASES(ARM_OP2_BCR, val);
	GEN_READ_WB_REG_CASES(ARM_OP2_WVR, val);
	GEN_READ_WB_REG_CASES(ARM_OP2_WCR, val);
	default:
		pr_warning("attempt to read from unknown breakpoint "
				"register %d\n", n);
	}

	return val;
}

static void write_wb_reg(int n, u32 val)
{
	switch (n) {
	GEN_WRITE_WB_REG_CASES(ARM_OP2_BVR, val);
	GEN_WRITE_WB_REG_CASES(ARM_OP2_BCR, val);
	GEN_WRITE_WB_REG_CASES(ARM_OP2_WVR, val);
	GEN_WRITE_WB_REG_CASES(ARM_OP2_WCR, val);
	default:
		pr_warning("attempt to write to unknown breakpoint "
				"register %d\n", n);
	}
	isb();
}

/*
 * In order to access the breakpoint/watchpoint control registers,
 * we must be running in debug monitor mode. Unfortunately, we can
 * be put into halting debug mode at any time by an external debugger
 * but there is nothing we can do to prevent that.
 */
static int enable_monitor_mode(void)
{
	u32 dscr;
	int ret = 0;

	ARM_DBG_READ(c1, 0, dscr);

	/* Ensure that halting mode is disabled. */
	if (WARN_ONCE(dscr & ARM_DSCR_HDBGEN, "halting debug mode enabled."
				"Unable to access hardware resources.")) {
		ret = -EPERM;
		goto out;
	}

	/* Write to the corresponding DSCR. */
	switch (debug_arch) {
	case ARM_DEBUG_ARCH_V6:
	case ARM_DEBUG_ARCH_V6_1:
		ARM_DBG_WRITE(c1, 0, (dscr | ARM_DSCR_MDBGEN));
		break;
	case ARM_DEBUG_ARCH_V7_ECP14:
		ARM_DBG_WRITE(c2, 2, (dscr | ARM_DSCR_MDBGEN));
		break;
	default:
		ret = -ENODEV;
		goto out;
	}

	/* Check that the write made it through. */
	ARM_DBG_READ(c1, 0, dscr);
	if (WARN_ONCE(!(dscr & ARM_DSCR_MDBGEN),
				"failed to enable monitor mode.")) {
		ret = -EPERM;
	}

out:
	return ret;
}

/*
 * Check if 8-bit byte-address select is available.
 * This clobbers WRP 0.
 */
static u8 get_max_wp_len(void)
{
	u32 ctrl_reg;
	struct arch_hw_breakpoint_ctrl ctrl;
	u8 size = 4;

	if (debug_arch < ARM_DEBUG_ARCH_V7_ECP14)
		goto out;

	if (enable_monitor_mode())
		goto out;

	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.len = ARM_BREAKPOINT_LEN_8;
	ctrl_reg = encode_ctrl_reg(ctrl);

	write_wb_reg(ARM_BASE_WVR, 0);
	write_wb_reg(ARM_BASE_WCR, ctrl_reg);
	if ((read_wb_reg(ARM_BASE_WCR) & ctrl_reg) == ctrl_reg)
		size = 8;

out:
	return size;
}

u8 arch_get_max_wp_len(void)
{
	return max_watchpoint_len;
}

/*
 * Handler for reactivating a suspended watchpoint when the single
 * step `mismatch' breakpoint is triggered.
 */
static void wp_single_step_handler(struct perf_event *bp, int unused,
				   struct perf_sample_data *data,
				   struct pt_regs *regs)
{
	perf_event_enable(counter_arch_bp(bp)->suspended_wp);
	unregister_hw_breakpoint(bp);
}

static int bp_is_single_step(struct perf_event *bp)
{
	return bp->overflow_handler == wp_single_step_handler;
}

/*
 * Install a perf counter breakpoint.
 */
int arch_install_hw_breakpoint(struct perf_event *bp)
{
	struct arch_hw_breakpoint *info = counter_arch_bp(bp);
	struct perf_event **slot, **slots;
	int i, max_slots, ctrl_base, val_base, ret = 0;

	/* Ensure that we are in monitor mode and halting mode is disabled. */
	ret = enable_monitor_mode();
	if (ret)
		goto out;

	if (info->ctrl.type == ARM_BREAKPOINT_EXECUTE) {
		/* Breakpoint */
		ctrl_base = ARM_BASE_BCR;
		val_base = ARM_BASE_BVR;
		slots = __get_cpu_var(bp_on_reg);
		max_slots = core_num_brps - 1;

		if (bp_is_single_step(bp)) {
			info->ctrl.mismatch = 1;
			i = max_slots;
			slots[i] = bp;
			goto setup;
		}
	} else {
		/* Watchpoint */
		ctrl_base = ARM_BASE_WCR;
		val_base = ARM_BASE_WVR;
		slots = __get_cpu_var(wp_on_reg);
		max_slots = core_num_wrps;
	}

	for (i = 0; i < max_slots; ++i) {
		slot = &slots[i];

		if (!*slot) {
			*slot = bp;
			break;
		}
	}

	if (WARN_ONCE(i == max_slots, "Can't find any breakpoint slot")) {
		ret = -EBUSY;
		goto out;
	}

setup:
	/* Setup the address register. */
	write_wb_reg(val_base + i, info->address);

	/* Setup the control register. */
	write_wb_reg(ctrl_base + i, encode_ctrl_reg(info->ctrl) | 0x1);

out:
	return ret;
}

void arch_uninstall_hw_breakpoint(struct perf_event *bp)
{
	struct arch_hw_breakpoint *info = counter_arch_bp(bp);
	struct perf_event **slot, **slots;
	int i, max_slots, base;

	if (info->ctrl.type == ARM_BREAKPOINT_EXECUTE) {
		/* Breakpoint */
		base = ARM_BASE_BCR;
		slots = __get_cpu_var(bp_on_reg);
		max_slots = core_num_brps - 1;

		if (bp_is_single_step(bp)) {
			i = max_slots;
			slots[i] = NULL;
			goto reset;
		}
	} else {
		/* Watchpoint */
		base = ARM_BASE_WCR;
		slots = __get_cpu_var(wp_on_reg);
		max_slots = core_num_wrps;
	}

	/* Remove the breakpoint. */
	for (i = 0; i < max_slots; ++i) {
		slot = &slots[i];

		if (*slot == bp) {
			*slot = NULL;
			break;
		}
	}

	if (WARN_ONCE(i == max_slots, "Can't find any breakpoint slot"))
		return;

reset:
	/* Reset the control register. */
	write_wb_reg(base + i, 0);
}

static int get_hbp_len(u8 hbp_len)
{
	unsigned int len_in_bytes = 0;

	switch (hbp_len) {
	case ARM_BREAKPOINT_LEN_1:
		len_in_bytes = 1;
		break;
	case ARM_BREAKPOINT_LEN_2:
		len_in_bytes = 2;
		break;
	case ARM_BREAKPOINT_LEN_4:
		len_in_bytes = 4;
		break;
	case ARM_BREAKPOINT_LEN_8:
		len_in_bytes = 8;
		break;
	}

	return len_in_bytes;
}

/*
 * Check whether bp virtual address is in kernel space.
 */
int arch_check_bp_in_kernelspace(struct perf_event *bp)
{
	unsigned int len;
	unsigned long va;
	struct arch_hw_breakpoint *info = counter_arch_bp(bp);

	va = info->address;
	len = get_hbp_len(info->ctrl.len);

	return (va >= TASK_SIZE) && ((va + len - 1) >= TASK_SIZE);
}

/*
 * Extract generic type and length encodings from an arch_hw_breakpoint_ctrl.
 * Hopefully this will disappear when ptrace can bypass the conversion
 * to generic breakpoint descriptions.
 */
int arch_bp_generic_fields(struct arch_hw_breakpoint_ctrl ctrl,
			   int *gen_len, int *gen_type)
{
	/* Type */
	switch (ctrl.type) {
	case ARM_BREAKPOINT_EXECUTE:
		*gen_type = HW_BREAKPOINT_X;
		break;
	case ARM_BREAKPOINT_LOAD:
		*gen_type = HW_BREAKPOINT_R;
		break;
	case ARM_BREAKPOINT_STORE:
		*gen_type = HW_BREAKPOINT_W;
		break;
	case ARM_BREAKPOINT_LOAD | ARM_BREAKPOINT_STORE:
		*gen_type = HW_BREAKPOINT_RW;
		break;
	default:
		return -EINVAL;
	}

	/* Len */
	switch (ctrl.len) {
	case ARM_BREAKPOINT_LEN_1:
		*gen_len = HW_BREAKPOINT_LEN_1;
		break;
	case ARM_BREAKPOINT_LEN_2:
		*gen_len = HW_BREAKPOINT_LEN_2;
		break;
	case ARM_BREAKPOINT_LEN_4:
		*gen_len = HW_BREAKPOINT_LEN_4;
		break;
	case ARM_BREAKPOINT_LEN_8:
		*gen_len = HW_BREAKPOINT_LEN_8;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * Construct an arch_hw_breakpoint from a perf_event.
 */
static int arch_build_bp_info(struct perf_event *bp)
{
	struct arch_hw_breakpoint *info = counter_arch_bp(bp);

	/* Type */
	switch (bp->attr.bp_type) {
	case HW_BREAKPOINT_X:
		info->ctrl.type = ARM_BREAKPOINT_EXECUTE;
		break;
	case HW_BREAKPOINT_R:
		info->ctrl.type = ARM_BREAKPOINT_LOAD;
		break;
	case HW_BREAKPOINT_W:
		info->ctrl.type = ARM_BREAKPOINT_STORE;
		break;
	case HW_BREAKPOINT_RW:
		info->ctrl.type = ARM_BREAKPOINT_LOAD | ARM_BREAKPOINT_STORE;
		break;
	default:
		return -EINVAL;
	}

	/* Len */
	switch (bp->attr.bp_len) {
	case HW_BREAKPOINT_LEN_1:
		info->ctrl.len = ARM_BREAKPOINT_LEN_1;
		break;
	case HW_BREAKPOINT_LEN_2:
		info->ctrl.len = ARM_BREAKPOINT_LEN_2;
		break;
	case HW_BREAKPOINT_LEN_4:
		info->ctrl.len = ARM_BREAKPOINT_LEN_4;
		break;
	case HW_BREAKPOINT_LEN_8:
		info->ctrl.len = ARM_BREAKPOINT_LEN_8;
		if ((info->ctrl.type != ARM_BREAKPOINT_EXECUTE)
			&& max_watchpoint_len >= 8)
			break;
	default:
		return -EINVAL;
	}

	/* Address */
	info->address = bp->attr.bp_addr;

	/* Privilege */
	info->ctrl.privilege = ARM_BREAKPOINT_USER;
	if (arch_check_bp_in_kernelspace(bp) && !bp_is_single_step(bp))
		info->ctrl.privilege |= ARM_BREAKPOINT_PRIV;

	/* Enabled? */
	info->ctrl.enabled = !bp->attr.disabled;

	/* Mismatch */
	info->ctrl.mismatch = 0;

	return 0;
}

/*
 * Validate the arch-specific HW Breakpoint register settings.
 */
int arch_validate_hwbkpt_settings(struct perf_event *bp)
{
	struct arch_hw_breakpoint *info = counter_arch_bp(bp);
	int ret = 0;
	u32 bytelen, max_len, offset, alignment_mask = 0x3;

	/* Build the arch_hw_breakpoint. */
	ret = arch_build_bp_info(bp);
	if (ret)
		goto out;

	/* Check address alignment. */
	if (info->ctrl.len == ARM_BREAKPOINT_LEN_8)
		alignment_mask = 0x7;
	if (info->address & alignment_mask) {
		/*
		 * Try to fix the alignment. This may result in a length
		 * that is too large, so we must check for that.
		 */
		bytelen = get_hbp_len(info->ctrl.len);
		max_len = info->ctrl.type == ARM_BREAKPOINT_EXECUTE ? 4 :
				max_watchpoint_len;

		if (max_len >= 8)
			offset = info->address & 0x7;
		else
			offset = info->address & 0x3;

		if (bytelen > (1 << ((max_len - (offset + 1)) >> 1))) {
			ret = -EFBIG;
			goto out;
		}

		info->ctrl.len <<= offset;
		info->address &= ~offset;

		pr_debug("breakpoint alignment fixup: length = 0x%x, "
			"address = 0x%x\n", info->ctrl.len, info->address);
	}

	/*
	 * Currently we rely on an overflow handler to take
	 * care of single-stepping the breakpoint when it fires.
	 * In the case of userspace breakpoints on a core with V7 debug,
	 * we can use the mismatch feature as a poor-man's hardware single-step.
	 */
	if (WARN_ONCE(!bp->overflow_handler &&
		(arch_check_bp_in_kernelspace(bp) || !core_has_mismatch_bps()),
			"overflow handler required but none found")) {
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
}

static void update_mismatch_flag(int idx, int flag)
{
	struct perf_event *bp = __get_cpu_var(bp_on_reg[idx]);
	struct arch_hw_breakpoint *info;

	if (bp == NULL)
		return;

	info = counter_arch_bp(bp);

	/* Update the mismatch field to enter/exit `single-step' mode */
	if (!bp->overflow_handler && info->ctrl.mismatch != flag) {
		info->ctrl.mismatch = flag;
		write_wb_reg(ARM_BASE_BCR + idx, encode_ctrl_reg(info->ctrl) | 0x1);
	}
}

static void watchpoint_handler(unsigned long unknown, struct pt_regs *regs)
{
	int i;
	struct perf_event *bp, **slots = __get_cpu_var(wp_on_reg);
	struct arch_hw_breakpoint *info;
	struct perf_event_attr attr;

	/* Without a disassembler, we can only handle 1 watchpoint. */
	BUG_ON(core_num_wrps > 1);

	hw_breakpoint_init(&attr);
	attr.bp_addr	= regs->ARM_pc & ~0x3;
	attr.bp_len	= HW_BREAKPOINT_LEN_4;
	attr.bp_type	= HW_BREAKPOINT_X;

	for (i = 0; i < core_num_wrps; ++i) {
		rcu_read_lock();

		if (slots[i] == NULL) {
			rcu_read_unlock();
			continue;
		}

		/*
		 * The DFAR is an unknown value. Since we only allow a
		 * single watchpoint, we can set the trigger to the lowest
		 * possible faulting address.
		 */
		info = counter_arch_bp(slots[i]);
		info->trigger = slots[i]->attr.bp_addr;
		pr_debug("watchpoint fired: address = 0x%x\n", info->trigger);
		perf_bp_event(slots[i], regs);

		/*
		 * If no overflow handler is present, insert a temporary
		 * mismatch breakpoint so we can single-step over the
		 * watchpoint trigger.
		 */
		if (!slots[i]->overflow_handler) {
			bp = register_user_hw_breakpoint(&attr,
							 wp_single_step_handler,
							 current);
			counter_arch_bp(bp)->suspended_wp = slots[i];
			perf_event_disable(slots[i]);
		}

		rcu_read_unlock();
	}
}

static void breakpoint_handler(unsigned long unknown, struct pt_regs *regs)
{
	int i;
	int mismatch;
	u32 ctrl_reg, val, addr;
	struct perf_event *bp, **slots = __get_cpu_var(bp_on_reg);
	struct arch_hw_breakpoint *info;
	struct arch_hw_breakpoint_ctrl ctrl;

	/* The exception entry code places the amended lr in the PC. */
	addr = regs->ARM_pc;

	for (i = 0; i < core_num_brps; ++i) {
		rcu_read_lock();

		bp = slots[i];

		if (bp == NULL) {
			rcu_read_unlock();
			continue;
		}

		mismatch = 0;

		/* Check if the breakpoint value matches. */
		val = read_wb_reg(ARM_BASE_BVR + i);
		if (val != (addr & ~0x3))
			goto unlock;

		/* Possible match, check the byte address select to confirm. */
		ctrl_reg = read_wb_reg(ARM_BASE_BCR + i);
		decode_ctrl_reg(ctrl_reg, &ctrl);
		if ((1 << (addr & 0x3)) & ctrl.len) {
			mismatch = 1;
			info = counter_arch_bp(bp);
			info->trigger = addr;
		}

unlock:
		if ((mismatch && !info->ctrl.mismatch) || bp_is_single_step(bp)) {
			pr_debug("breakpoint fired: address = 0x%x\n", addr);
			perf_bp_event(bp, regs);
		}

		update_mismatch_flag(i, mismatch);
		rcu_read_unlock();
	}
}

/*
 * Called from either the Data Abort Handler [watchpoint] or the
 * Prefetch Abort Handler [breakpoint].
 */
static int hw_breakpoint_pending(unsigned long addr, unsigned int fsr,
				 struct pt_regs *regs)
{
	int ret = 1; /* Unhandled fault. */
	u32 dscr;

	/* We only handle watchpoints and hardware breakpoints. */
	ARM_DBG_READ(c1, 0, dscr);

	/* Perform perf callbacks. */
	switch (ARM_DSCR_MOE(dscr)) {
	case ARM_ENTRY_BREAKPOINT:
		breakpoint_handler(addr, regs);
		break;
	case ARM_ENTRY_ASYNC_WATCHPOINT:
		WARN(1, "Asynchronous watchpoint exception taken. Debugging results may be unreliable\n");
	case ARM_ENTRY_SYNC_WATCHPOINT:
		watchpoint_handler(addr, regs);
		break;
	default:
		goto out;
	}

	ret = 0;
out:
	return ret;
}

/*
 * One-time initialisation.
 */
static void __init reset_ctrl_regs(void *unused)
{
	int i;

	if (enable_monitor_mode())
		return;

	for (i = 0; i < core_num_brps; ++i) {
		write_wb_reg(ARM_BASE_BCR + i, 0UL);
		write_wb_reg(ARM_BASE_BVR + i, 0UL);
	}

	for (i = 0; i < core_num_wrps; ++i) {
		write_wb_reg(ARM_BASE_WCR + i, 0UL);
		write_wb_reg(ARM_BASE_WVR + i, 0UL);
	}
}

static int __init arch_hw_breakpoint_init(void)
{
	int ret = 0;
	u32 dscr;

	debug_arch = get_debug_arch();

	if (debug_arch > ARM_DEBUG_ARCH_V7_ECP14) {
		pr_info("debug architecture 0x%x unsupported.\n", debug_arch);
		ret = -ENODEV;
		goto out;
	}

	/* Determine how many BRPs/WRPs are available. */
	core_num_brps = get_num_brps();
	core_num_wrps = get_num_wrps();

	pr_info("found %d breakpoint and %d watchpoint registers.\n",
			core_num_brps, core_num_wrps);

	if (core_has_mismatch_bps())
		pr_info("1 breakpoint reserved for watchpoint single-step.\n");

	ARM_DBG_READ(c1, 0, dscr);
	if (dscr & ARM_DSCR_HDBGEN) {
		pr_warning("halting debug mode enabled. Assuming maximum "
				"watchpoint size of 4 bytes.");
	} else {
		/* Work out the maximum supported watchpoint length. */
		max_watchpoint_len = get_max_wp_len();
		pr_info("maximum watchpoint size is %u bytes.\n",
				max_watchpoint_len);

		/*
		 * Reset the breakpoint resources. We assume that a halting
		 * debugger will leave the world in a nice state for us.
		 */
		smp_call_function(reset_ctrl_regs, NULL, 1);
		reset_ctrl_regs(NULL);
	}

	/* Register debug fault handler. */
	hook_fault_code(2, hw_breakpoint_pending, SIGTRAP, TRAP_HWBKPT,
			"watchpoint debug exception");
	hook_ifault_code(2, hw_breakpoint_pending, SIGTRAP, TRAP_HWBKPT,
			"breakpoint debug exception");

out:
	return ret;
}
arch_initcall(arch_hw_breakpoint_init);

void hw_breakpoint_pmu_read(struct perf_event *bp)
{
}

/*
 * Dummy function to register with die_notifier.
 */
int hw_breakpoint_exceptions_notify(struct notifier_block *unused,
					unsigned long val, void *data)
{
	return NOTIFY_DONE;
}
