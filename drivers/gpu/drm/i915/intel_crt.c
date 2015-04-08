/*
 * Copyright © 2006-2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Eric Anholt <eric@anholt.net>
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include "drmP.h"
#include "drm.h"
#include "drm_crtc.h"
#include "drm_crtc_helper.h"
#include "intel_drv.h"
#include "i915_drm.h"
#include "i915_drv.h"

/* Here's the desired hotplug mode */
#define ADPA_HOTPLUG_BITS (ADPA_CRT_HOTPLUG_PERIOD_128 |		\
			   ADPA_CRT_HOTPLUG_WARMUP_10MS |		\
			   ADPA_CRT_HOTPLUG_SAMPLE_4S |			\
			   ADPA_CRT_HOTPLUG_VOLTAGE_50 |		\
			   ADPA_CRT_HOTPLUG_VOLREF_325MV |		\
			   ADPA_CRT_HOTPLUG_ENABLE)

struct intel_crt {
	struct intel_encoder base;
	bool force_hotplug_required;
};

static struct intel_crt *intel_attached_crt(struct drm_connector *connector)
{
	return container_of(intel_attached_encoder(connector),
			    struct intel_crt, base);
}

static void intel_crt_dpms(struct drm_encoder *encoder, int mode)
{
	struct drm_device *dev = encoder->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 temp, reg;

	if (HAS_PCH_SPLIT(dev))
		reg = PCH_ADPA;
	else
		reg = ADPA;

	temp = I915_READ(reg);
	temp &= ~(ADPA_HSYNC_CNTL_DISABLE | ADPA_VSYNC_CNTL_DISABLE);
	temp &= ~ADPA_DAC_ENABLE;

	switch(mode) {
	case DRM_MODE_DPMS_ON:
		temp |= ADPA_DAC_ENABLE;
		break;
	case DRM_MODE_DPMS_STANDBY:
		temp |= ADPA_DAC_ENABLE | ADPA_HSYNC_CNTL_DISABLE;
		break;
	case DRM_MODE_DPMS_SUSPEND:
		temp |= ADPA_DAC_ENABLE | ADPA_VSYNC_CNTL_DISABLE;
		break;
	case DRM_MODE_DPMS_OFF:
		temp |= ADPA_HSYNC_CNTL_DISABLE | ADPA_VSYNC_CNTL_DISABLE;
		break;
	}

	I915_WRITE(reg, temp);
}

static int intel_crt_mode_valid(struct drm_connector *connector,
				struct drm_display_mode *mode)
{
	struct drm_device *dev = connector->dev;

	int max_clock = 0;
	if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
		return MODE_NO_DBLESCAN;

	if (mode->clock < 25000)
		return MODE_CLOCK_LOW;

	if (IS_GEN2(dev))
		max_clock = 350000;
	else
		max_clock = 400000;
	if (mode->clock > max_clock)
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static bool intel_crt_mode_fixup(struct drm_encoder *encoder,
				 struct drm_display_mode *mode,
				 struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void intel_crt_mode_set(struct drm_encoder *encoder,
			       struct drm_display_mode *mode,
			       struct drm_display_mode *adjusted_mode)
{

	struct drm_device *dev = encoder->dev;
	struct drm_crtc *crtc = encoder->crtc;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	struct drm_i915_private *dev_priv = dev->dev_private;
	int dpll_md_reg;
	u32 adpa, dpll_md;
	u32 adpa_reg;

	if (intel_crtc->pipe == 0)
		dpll_md_reg = DPLL_A_MD;
	else
		dpll_md_reg = DPLL_B_MD;

	if (HAS_PCH_SPLIT(dev))
		adpa_reg = PCH_ADPA;
	else
		adpa_reg = ADPA;

	/*
	 * Disable separate mode multiplier used when cloning SDVO to CRT
	 * XXX this needs to be adjusted when we really are cloning
	 */
	if (INTEL_INFO(dev)->gen >= 4 && !HAS_PCH_SPLIT(dev)) {
		dpll_md = I915_READ(dpll_md_reg);
		I915_WRITE(dpll_md_reg,
			   dpll_md & ~DPLL_MD_UDI_MULTIPLIER_MASK);
	}

	adpa = ADPA_HOTPLUG_BITS;
	if (adjusted_mode->flags & DRM_MODE_FLAG_PHSYNC)
		adpa |= ADPA_HSYNC_ACTIVE_HIGH;
	if (adjusted_mode->flags & DRM_MODE_FLAG_PVSYNC)
		adpa |= ADPA_VSYNC_ACTIVE_HIGH;

	if (intel_crtc->pipe == 0) {
		if (HAS_PCH_CPT(dev))
			adpa |= PORT_TRANS_A_SEL_CPT;
		else
			adpa |= ADPA_PIPE_A_SELECT;
		if (!HAS_PCH_SPLIT(dev))
			I915_WRITE(BCLRPAT_A, 0);
	} else {
		if (HAS_PCH_CPT(dev))
			adpa |= PORT_TRANS_B_SEL_CPT;
		else
			adpa |= ADPA_PIPE_B_SELECT;
		if (!HAS_PCH_SPLIT(dev))
			I915_WRITE(BCLRPAT_B, 0);
	}

	I915_WRITE(adpa_reg, adpa);
}

static bool intel_ironlake_crt_detect_hotplug(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct intel_crt *crt = intel_attached_crt(connector);
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 adpa;
	bool ret;

	/* The first time through, trigger an explicit detection cycle */
	if (crt->force_hotplug_required) {
		bool turn_off_dac = HAS_PCH_SPLIT(dev);
		u32 save_adpa;

		crt->force_hotplug_required = 0;

		save_adpa = adpa = I915_READ(PCH_ADPA);
		DRM_DEBUG_KMS("trigger hotplug detect cycle: adpa=0x%x\n", adpa);

		adpa |= ADPA_CRT_HOTPLUG_FORCE_TRIGGER;
		if (turn_off_dac)
			adpa &= ~ADPA_DAC_ENABLE;

		I915_WRITE(PCH_ADPA, adpa);

		if (wait_for((I915_READ(PCH_ADPA) & ADPA_CRT_HOTPLUG_FORCE_TRIGGER) == 0,
			     1000))
			DRM_DEBUG_KMS("timed out waiting for FORCE_TRIGGER");

		if (turn_off_dac) {
			I915_WRITE(PCH_ADPA, save_adpa);
			POSTING_READ(PCH_ADPA);
		}
	}

	/* Check the status to see if both blue and green are on now */
	adpa = I915_READ(PCH_ADPA);
	if ((adpa & ADPA_CRT_HOTPLUG_MONITOR_MASK) != 0)
		ret = true;
	else
		ret = false;
	DRM_DEBUG_KMS("ironlake hotplug adpa=0x%x, result %d\n", adpa, ret);

	return ret;
}

/**
 * Uses CRT_HOTPLUG_EN and CRT_HOTPLUG_STAT to detect CRT presence.
 *
 * Not for i915G/i915GM
 *
 * \return true if CRT is connected.
 * \return false if CRT is disconnected.
 */
static bool intel_crt_detect_hotplug(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	u32 hotplug_en, orig, stat;
	bool ret = false;
	int i, tries = 0;

	if (HAS_PCH_SPLIT(dev))
		return intel_ironlake_crt_detect_hotplug(connector);

	/*
	 * On 4 series desktop, CRT detect sequence need to be done twice
	 * to get a reliable result.
	 */

	if (IS_G4X(dev) && !IS_GM45(dev))
		tries = 2;
	else
		tries = 1;
	hotplug_en = orig = I915_READ(PORT_HOTPLUG_EN);
	hotplug_en |= CRT_HOTPLUG_FORCE_DETECT;

	for (i = 0; i < tries ; i++) {
		/* turn on the FORCE_DETECT */
		I915_WRITE(PORT_HOTPLUG_EN, hotplug_en);
		/* wait for FORCE_DETECT to go off */
		if (wait_for((I915_READ(PORT_HOTPLUG_EN) &
			      CRT_HOTPLUG_FORCE_DETECT) == 0,
			     1000))
			DRM_DEBUG_KMS("timed out waiting for FORCE_DETECT to go off");
	}

	stat = I915_READ(PORT_HOTPLUG_STAT);
	if ((stat & CRT_HOTPLUG_MONITOR_MASK) != CRT_HOTPLUG_MONITOR_NONE)
		ret = true;

	/* clear the interrupt we just generated, if any */
	I915_WRITE(PORT_HOTPLUG_STAT, CRT_HOTPLUG_INT_STATUS);

	/* and put the bits back */
	I915_WRITE(PORT_HOTPLUG_EN, orig);

	return ret;
}

static bool intel_crt_ddc_probe(struct drm_i915_private *dev_priv, int ddc_bus)
{
	u8 buf;
	struct i2c_msg msgs[] = {
		{
			.addr = 0xA0,
			.flags = 0,
			.len = 1,
			.buf = &buf,
		},
	};
	/* DDC monitor detect: Does it ACK a write to 0xA0? */
	return i2c_transfer(&dev_priv->gmbus[ddc_bus].adapter, msgs, 1) == 1;
}

static bool intel_crt_detect_ddc(struct intel_crt *crt)
{
	struct drm_i915_private *dev_priv = crt->base.base.dev->dev_private;

	/* CRT should always be at 0, but check anyway */
	if (crt->base.type != INTEL_OUTPUT_ANALOG)
		return false;

	if (intel_crt_ddc_probe(dev_priv, dev_priv->crt_ddc_pin)) {
		DRM_DEBUG_KMS("CRT detected via DDC:0xa0\n");
		return true;
	}

	if (intel_ddc_probe(&crt->base, dev_priv->crt_ddc_pin)) {
		DRM_DEBUG_KMS("CRT detected via DDC:0x50 [EDID]\n");
		return true;
	}

	return false;
}

static enum drm_connector_status
intel_crt_load_detect(struct drm_crtc *crtc, struct intel_crt *crt)
{
	struct drm_encoder *encoder = &crt->base.base;
	struct drm_device *dev = encoder->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_crtc *intel_crtc = to_intel_crtc(crtc);
	uint32_t pipe = intel_crtc->pipe;
	uint32_t save_bclrpat;
	uint32_t save_vtotal;
	uint32_t vtotal, vactive;
	uint32_t vsample;
	uint32_t vblank, vblank_start, vblank_end;
	uint32_t dsl;
	uint32_t bclrpat_reg;
	uint32_t vtotal_reg;
	uint32_t vblank_reg;
	uint32_t vsync_reg;
	uint32_t pipeconf_reg;
	uint32_t pipe_dsl_reg;
	uint8_t	st00;
	enum drm_connector_status status;

	DRM_DEBUG_KMS("starting load-detect on CRT\n");

	if (pipe == 0) {
		bclrpat_reg = BCLRPAT_A;
		vtotal_reg = VTOTAL_A;
		vblank_reg = VBLANK_A;
		vsync_reg = VSYNC_A;
		pipeconf_reg = PIPEACONF;
		pipe_dsl_reg = PIPEADSL;
	} else {
		bclrpat_reg = BCLRPAT_B;
		vtotal_reg = VTOTAL_B;
		vblank_reg = VBLANK_B;
		vsync_reg = VSYNC_B;
		pipeconf_reg = PIPEBCONF;
		pipe_dsl_reg = PIPEBDSL;
	}

	save_bclrpat = I915_READ(bclrpat_reg);
	save_vtotal = I915_READ(vtotal_reg);
	vblank = I915_READ(vblank_reg);

	vtotal = ((save_vtotal >> 16) & 0xfff) + 1;
	vactive = (save_vtotal & 0x7ff) + 1;

	vblank_start = (vblank & 0xfff) + 1;
	vblank_end = ((vblank >> 16) & 0xfff) + 1;

	/* Set the border color to purple. */
	I915_WRITE(bclrpat_reg, 0x500050);

	if (!IS_GEN2(dev)) {
		uint32_t pipeconf = I915_READ(pipeconf_reg);
		I915_WRITE(pipeconf_reg, pipeconf | PIPECONF_FORCE_BORDER);
		POSTING_READ(pipeconf_reg);
		/* Wait for next Vblank to substitue
		 * border color for Color info */
		intel_wait_for_vblank(dev, pipe);
		st00 = I915_READ8(VGA_MSR_WRITE);
		status = ((st00 & (1 << 4)) != 0) ?
			connector_status_connected :
			connector_status_disconnected;

		I915_WRITE(pipeconf_reg, pipeconf);
	} else {
		bool restore_vblank = false;
		int count, detect;

		/*
		* If there isn't any border, add some.
		* Yes, this will flicker
		*/
		if (vblank_start <= vactive && vblank_end >= vtotal) {
			uint32_t vsync = I915_READ(vsync_reg);
			uint32_t vsync_start = (vsync & 0xffff) + 1;

			vblank_start = vsync_start;
			I915_WRITE(vblank_reg,
				   (vblank_start - 1) |
				   ((vblank_end - 1) << 16));
			restore_vblank = true;
		}
		/* sample in the vertical border, selecting the larger one */
		if (vblank_start - vactive >= vtotal - vblank_end)
			vsample = (vblank_start + vactive) >> 1;
		else
			vsample = (vtotal + vblank_end) >> 1;

		/*
		 * Wait for the border to be displayed
		 */
		while (I915_READ(pipe_dsl_reg) >= vactive)
			;
		while ((dsl = I915_READ(pipe_dsl_reg)) <= vsample)
			;
		/*
		 * Watch ST00 for an entire scanline
		 */
		detect = 0;
		count = 0;
		do {
			count++;
			/* Read the ST00 VGA status register */
			st00 = I915_READ8(VGA_MSR_WRITE);
			if (st00 & (1 << 4))
				detect++;
		} while ((I915_READ(pipe_dsl_reg) == dsl));

		/* restore vblank if necessary */
		if (restore_vblank)
			I915_WRITE(vblank_reg, vblank);
		/*
		 * If more than 3/4 of the scanline detected a monitor,
		 * then it is assumed to be present. This works even on i830,
		 * where there isn't any way to force the border color across
		 * the screen
		 */
		status = detect * 4 > count * 3 ?
			 connector_status_connected :
			 connector_status_disconnected;
	}

	/* Restore previous settings */
	I915_WRITE(bclrpat_reg, save_bclrpat);

	return status;
}

static enum drm_connector_status
intel_crt_detect(struct drm_connector *connector, bool force)
{
	struct drm_device *dev = connector->dev;
	struct intel_crt *crt = intel_attached_crt(connector);
	struct drm_crtc *crtc;
	int dpms_mode;
	enum drm_connector_status status;

	if (I915_HAS_HOTPLUG(dev)) {
		if (intel_crt_detect_hotplug(connector)) {
			DRM_DEBUG_KMS("CRT detected via hotplug\n");
			return connector_status_connected;
		} else {
			DRM_DEBUG_KMS("CRT not detected via hotplug\n");
			return connector_status_disconnected;
		}
	}

	if (intel_crt_detect_ddc(crt))
		return connector_status_connected;

	if (!force)
		return connector->status;

	/* for pre-945g platforms use load detect */
	crtc = crt->base.base.crtc;
	if (crtc && crtc->enabled) {
		status = intel_crt_load_detect(crtc, crt);
	} else {
		crtc = intel_get_load_detect_pipe(&crt->base, connector,
						  NULL, &dpms_mode);
		if (crtc) {
			if (intel_crt_detect_ddc(crt))
				status = connector_status_connected;
			else
				status = intel_crt_load_detect(crtc, crt);
			intel_release_load_detect_pipe(&crt->base,
						       connector, dpms_mode);
		} else
			status = connector_status_unknown;
	}

	return status;
}

static void intel_crt_destroy(struct drm_connector *connector)
{
	drm_sysfs_connector_remove(connector);
	drm_connector_cleanup(connector);
	kfree(connector);
}

static int intel_crt_get_modes(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_i915_private *dev_priv = dev->dev_private;
	int ret;

	ret = intel_ddc_get_modes(connector,
				 &dev_priv->gmbus[dev_priv->crt_ddc_pin].adapter);
	if (ret || !IS_G4X(dev))
		return ret;

	/* Try to probe digital port for output in DVI-I -> VGA mode. */
	return intel_ddc_get_modes(connector,
				   &dev_priv->gmbus[GMBUS_PORT_DPB].adapter);
}

static int intel_crt_set_property(struct drm_connector *connector,
				  struct drm_property *property,
				  uint64_t value)
{
	return 0;
}

/*
 * Routines for controlling stuff on the analog port
 */

static const struct drm_encoder_helper_funcs intel_crt_helper_funcs = {
	.dpms = intel_crt_dpms,
	.mode_fixup = intel_crt_mode_fixup,
	.prepare = intel_encoder_prepare,
	.commit = intel_encoder_commit,
	.mode_set = intel_crt_mode_set,
};

static const struct drm_connector_funcs intel_crt_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = intel_crt_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = intel_crt_destroy,
	.set_property = intel_crt_set_property,
};

static const struct drm_connector_helper_funcs intel_crt_connector_helper_funcs = {
	.mode_valid = intel_crt_mode_valid,
	.get_modes = intel_crt_get_modes,
	.best_encoder = intel_best_encoder,
};

static const struct drm_encoder_funcs intel_crt_enc_funcs = {
	.destroy = intel_encoder_destroy,
};

void intel_crt_init(struct drm_device *dev)
{
	struct drm_connector *connector;
	struct intel_crt *crt;
	struct intel_connector *intel_connector;
	struct drm_i915_private *dev_priv = dev->dev_private;

	crt = kzalloc(sizeof(struct intel_crt), GFP_KERNEL);
	if (!crt)
		return;

	intel_connector = kzalloc(sizeof(struct intel_connector), GFP_KERNEL);
	if (!intel_connector) {
		kfree(crt);
		return;
	}

	connector = &intel_connector->base;
	drm_connector_init(dev, &intel_connector->base,
			   &intel_crt_connector_funcs, DRM_MODE_CONNECTOR_VGA);

	drm_encoder_init(dev, &crt->base.base, &intel_crt_enc_funcs,
			 DRM_MODE_ENCODER_DAC);

	intel_connector_attach_encoder(intel_connector, &crt->base);

	crt->base.type = INTEL_OUTPUT_ANALOG;
	crt->base.clone_mask = (1 << INTEL_SDVO_NON_TV_CLONE_BIT |
				1 << INTEL_ANALOG_CLONE_BIT |
				1 << INTEL_SDVO_LVDS_CLONE_BIT);
	crt->base.crtc_mask = (1 << 0) | (1 << 1);
	connector->interlace_allowed = 1;
	connector->doublescan_allowed = 0;

	drm_encoder_helper_add(&crt->base.base, &intel_crt_helper_funcs);
	drm_connector_helper_add(connector, &intel_crt_connector_helper_funcs);

	drm_sysfs_connector_add(connector);

	if (I915_HAS_HOTPLUG(dev))
		connector->polled = DRM_CONNECTOR_POLL_HPD;
	else
		connector->polled = DRM_CONNECTOR_POLL_CONNECT;

	/*
	 * Configure the automatic hotplug detection stuff
	 */
	crt->force_hotplug_required = 0;
	if (HAS_PCH_SPLIT(dev)) {
		u32 adpa;

		adpa = I915_READ(PCH_ADPA);
		adpa &= ~ADPA_CRT_HOTPLUG_MASK;
		adpa |= ADPA_HOTPLUG_BITS;
		I915_WRITE(PCH_ADPA, adpa);
		POSTING_READ(PCH_ADPA);

		DRM_DEBUG_KMS("pch crt adpa set to 0x%x\n", adpa);
		crt->force_hotplug_required = 1;
	}

	dev_priv->hotplug_supported_mask |= CRT_HOTPLUG_INT_STATUS;
}
