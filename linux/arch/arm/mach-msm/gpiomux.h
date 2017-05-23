/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef __ARCH_ARM_MACH_MSM_GPIOMUX_H
#define __ARCH_ARM_MACH_MSM_GPIOMUX_H

#include <linux/bitops.h>
#include <linux/errno.h>
#include <mach/msm_gpiomux.h>
#include "gpiomux-v1.h"

/**
 * struct msm_gpiomux_config: gpiomux settings for one gpio line.
 *
 * A complete gpiomux config is the bitwise-or of a drive-strength,
 * function, and pull.  For functions other than GPIO, the OE
 * is hard-wired according to the function.  For GPIO mode,
 * OE is controlled by gpiolib.
 *
 * Available settings differ by target; see the gpiomux header
 * specific to your target arch for available configurations.
 *
 * @active: The configuration to be installed when the line is
 * active, or its reference count is > 0.
 * @suspended: The configuration to be installed when the line
 * is suspended, or its reference count is 0.
 * @ref: The reference count of the line.  For internal use of
 * the gpiomux framework only.
 */
struct msm_gpiomux_config {
	gpiomux_config_t active;
	gpiomux_config_t suspended;
	unsigned         ref;
};

/**
 * @GPIOMUX_VALID:	If set, the config field contains 'good data'.
 *                      The absence of this bit will prevent the gpiomux
 *			system from applying the configuration under all
 *			circumstances.
 */
enum {
	GPIOMUX_VALID	 = BIT(sizeof(gpiomux_config_t) * BITS_PER_BYTE - 1),
	GPIOMUX_CTL_MASK = GPIOMUX_VALID,
};

#ifdef CONFIG_MSM_GPIOMUX

/* Each architecture must provide its own instance of this table.
 * To avoid having gpiomux manage any given gpio, one or both of
 * the entries can avoid setting GPIOMUX_VALID - the absence
 * of that flag will prevent the configuration from being applied
 * during state transitions.
 */
extern struct msm_gpiomux_config msm_gpiomux_configs[GPIOMUX_NGPIOS];

/* Install a new configuration to the gpio line.  To avoid overwriting
 * a configuration, leave the VALID bit out.
 */
int msm_gpiomux_write(unsigned gpio,
		      gpiomux_config_t active,
		      gpiomux_config_t suspended);
#else
static inline int msm_gpiomux_write(unsigned gpio,
				    gpiomux_config_t active,
				    gpiomux_config_t suspended)
{
	return -ENOSYS;
}
#endif
#endif
