/*
 * ALSA SoC TLV320AIC3X codec driver
 *
 * Author:      Vladimir Barinov, <vbarinov@embeddedalley.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * Based on sound/soc/codecs/wm8753.c by Liam Girdwood
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Notes:
 *  The AIC3X is a driver for a low power stereo audio
 *  codecs aic31, aic32, aic33, aic3007.
 *
 *  It supports full aic33 codec functionality.
 *  The compatibility with aic32, aic31 and aic3007 is as follows:
 *    aic32/aic3007    |        aic31
 *  ---------------------------------------
 *   MONO_LOUT -> N/A  |  MONO_LOUT -> N/A
 *                     |  IN1L -> LINE1L
 *                     |  IN1R -> LINE1R
 *                     |  IN2L -> LINE2L
 *                     |  IN2R -> LINE2R
 *                     |  MIC3L/R -> N/A
 *   truncated internal functionality in
 *   accordance with documentation
 *  ---------------------------------------
 *
 *  Hence the machine layer should disable unsupported inputs/outputs by
 *  snd_soc_dapm_disable_pin(codec, "MONO_LOUT"), etc.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/tlv320aic3x.h>

#include "tlv320aic3x.h"

#define AIC3X_NUM_SUPPLIES	4
static const char *aic3x_supply_names[AIC3X_NUM_SUPPLIES] = {
	"IOVDD",	/* I/O Voltage */
	"DVDD",		/* Digital Core Voltage */
	"AVDD",		/* Analog DAC Voltage */
	"DRVDD",	/* ADC Analog and Output Driver Voltage */
};

struct aic3x_priv;

struct aic3x_disable_nb {
	struct notifier_block nb;
	struct aic3x_priv *aic3x;
};

/* codec private data */
struct aic3x_priv {
	struct snd_soc_codec *codec;
	struct regulator_bulk_data supplies[AIC3X_NUM_SUPPLIES];
	struct aic3x_disable_nb disable_nb[AIC3X_NUM_SUPPLIES];
	enum snd_soc_control_type control_type;
	struct aic3x_setup_data *setup;
	void *control_data;
	unsigned int sysclk;
	int master;
	int gpio_reset;
	int power;
#define AIC3X_MODEL_3X 0
#define AIC3X_MODEL_33 1
#define AIC3X_MODEL_3007 2
	u16 model;
};

/*
 * AIC3X register cache
 * We can't read the AIC3X register space when we are
 * using 2 wire for device control, so we cache them instead.
 * There is no point in caching the reset register
 */
static const u8 aic3x_reg[AIC3X_CACHEREGNUM] = {
	0x00, 0x00, 0x00, 0x10,	/* 0 */
	0x04, 0x00, 0x00, 0x00,	/* 4 */
	0x00, 0x00, 0x00, 0x01,	/* 8 */
	0x00, 0x00, 0x00, 0x80,	/* 12 */
	0x80, 0xff, 0xff, 0x78,	/* 16 */
	0x78, 0x78, 0x78, 0x78,	/* 20 */
	0x78, 0x00, 0x00, 0xfe,	/* 24 */
	0x00, 0x00, 0xfe, 0x00,	/* 28 */
	0x18, 0x18, 0x00, 0x00,	/* 32 */
	0x00, 0x00, 0x00, 0x00,	/* 36 */
	0x00, 0x00, 0x00, 0x80,	/* 40 */
	0x80, 0x00, 0x00, 0x00,	/* 44 */
	0x00, 0x00, 0x00, 0x04,	/* 48 */
	0x00, 0x00, 0x00, 0x00,	/* 52 */
	0x00, 0x00, 0x04, 0x00,	/* 56 */
	0x00, 0x00, 0x00, 0x00,	/* 60 */
	0x00, 0x04, 0x00, 0x00,	/* 64 */
	0x00, 0x00, 0x00, 0x00,	/* 68 */
	0x04, 0x00, 0x00, 0x00,	/* 72 */
	0x00, 0x00, 0x00, 0x00,	/* 76 */
	0x00, 0x00, 0x00, 0x00,	/* 80 */
	0x00, 0x00, 0x00, 0x00,	/* 84 */
	0x00, 0x00, 0x00, 0x00,	/* 88 */
	0x00, 0x00, 0x00, 0x00,	/* 92 */
	0x00, 0x00, 0x00, 0x00,	/* 96 */
	0x00, 0x00, 0x02,	/* 100 */
};

/*
 * read from the aic3x register space. Only use for this function is if
 * wanting to read volatile bits from those registers that has both read-only
 * and read/write bits. All other cases should use snd_soc_read.
 */
static int aic3x_read(struct snd_soc_codec *codec, unsigned int reg,
		      u8 *value)
{
	u8 *cache = codec->reg_cache;

	if (codec->cache_only)
		return -EINVAL;
	if (reg >= AIC3X_CACHEREGNUM)
		return -1;

	*value = codec->hw_read(codec, reg);
	cache[reg] = *value;

	return 0;
}

#define SOC_DAPM_SINGLE_AIC3X(xname, reg, shift, mask, invert) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
	.info = snd_soc_info_volsw, \
	.get = snd_soc_dapm_get_volsw, .put = snd_soc_dapm_put_volsw_aic3x, \
	.private_value =  SOC_SINGLE_VALUE(reg, shift, mask, invert) }

/*
 * All input lines are connected when !0xf and disconnected with 0xf bit field,
 * so we have to use specific dapm_put call for input mixer
 */
static int snd_soc_dapm_put_volsw_aic3x(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_widget *widget = snd_kcontrol_chip(kcontrol);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	int max = mc->max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
	unsigned short val, val_mask;
	int ret;
	struct snd_soc_dapm_path *path;
	int found = 0;

	val = (ucontrol->value.integer.value[0] & mask);

	mask = 0xf;
	if (val)
		val = mask;

	if (invert)
		val = mask - val;
	val_mask = mask << shift;
	val = val << shift;

	mutex_lock(&widget->codec->mutex);

	if (snd_soc_test_bits(widget->codec, reg, val_mask, val)) {
		/* find dapm widget path assoc with kcontrol */
		list_for_each_entry(path, &widget->codec->dapm_paths, list) {
			if (path->kcontrol != kcontrol)
				continue;

			/* found, now check type */
			found = 1;
			if (val)
				/* new connection */
				path->connect = invert ? 0 : 1;
			else
				/* old connection must be powered down */
				path->connect = invert ? 1 : 0;
			break;
		}

		if (found)
			snd_soc_dapm_sync(widget->codec);
	}

	ret = snd_soc_update_bits(widget->codec, reg, val_mask, val);

	mutex_unlock(&widget->codec->mutex);
	return ret;
}

static const char *aic3x_left_dac_mux[] = { "DAC_L1", "DAC_L3", "DAC_L2" };
static const char *aic3x_right_dac_mux[] = { "DAC_R1", "DAC_R3", "DAC_R2" };
static const char *aic3x_left_hpcom_mux[] =
    { "differential of HPLOUT", "constant VCM", "single-ended" };
static const char *aic3x_right_hpcom_mux[] =
    { "differential of HPROUT", "constant VCM", "single-ended",
      "differential of HPLCOM", "external feedback" };
static const char *aic3x_linein_mode_mux[] = { "single-ended", "differential" };
static const char *aic3x_adc_hpf[] =
    { "Disabled", "0.0045xFs", "0.0125xFs", "0.025xFs" };

#define LDAC_ENUM	0
#define RDAC_ENUM	1
#define LHPCOM_ENUM	2
#define RHPCOM_ENUM	3
#define LINE1L_ENUM	4
#define LINE1R_ENUM	5
#define LINE2L_ENUM	6
#define LINE2R_ENUM	7
#define ADC_HPF_ENUM	8

static const struct soc_enum aic3x_enum[] = {
	SOC_ENUM_SINGLE(DAC_LINE_MUX, 6, 3, aic3x_left_dac_mux),
	SOC_ENUM_SINGLE(DAC_LINE_MUX, 4, 3, aic3x_right_dac_mux),
	SOC_ENUM_SINGLE(HPLCOM_CFG, 4, 3, aic3x_left_hpcom_mux),
	SOC_ENUM_SINGLE(HPRCOM_CFG, 3, 5, aic3x_right_hpcom_mux),
	SOC_ENUM_SINGLE(LINE1L_2_LADC_CTRL, 7, 2, aic3x_linein_mode_mux),
	SOC_ENUM_SINGLE(LINE1R_2_RADC_CTRL, 7, 2, aic3x_linein_mode_mux),
	SOC_ENUM_SINGLE(LINE2L_2_LADC_CTRL, 7, 2, aic3x_linein_mode_mux),
	SOC_ENUM_SINGLE(LINE2R_2_RADC_CTRL, 7, 2, aic3x_linein_mode_mux),
	SOC_ENUM_DOUBLE(AIC3X_CODEC_DFILT_CTRL, 6, 4, 4, aic3x_adc_hpf),
};

/*
 * DAC digital volumes. From -63.5 to 0 dB in 0.5 dB steps
 */
static DECLARE_TLV_DB_SCALE(dac_tlv, -6350, 50, 0);
/* ADC PGA gain volumes. From 0 to 59.5 dB in 0.5 dB steps */
static DECLARE_TLV_DB_SCALE(adc_tlv, 0, 50, 0);
/*
 * Output stage volumes. From -78.3 to 0 dB. Muted below -78.3 dB.
 * Step size is approximately 0.5 dB over most of the scale but increasing
 * near the very low levels.
 * Define dB scale so that it is mostly correct for range about -55 to 0 dB
 * but having increasing dB difference below that (and where it doesn't count
 * so much). This setting shows -50 dB (actual is -50.3 dB) for register
 * value 100 and -58.5 dB (actual is -78.3 dB) for register value 117.
 */
static DECLARE_TLV_DB_SCALE(output_stage_tlv, -5900, 50, 1);

static const struct snd_kcontrol_new aic3x_snd_controls[] = {
	/* Output */
	SOC_DOUBLE_R_TLV("PCM Playback Volume",
			 LDAC_VOL, RDAC_VOL, 0, 0x7f, 1, dac_tlv),

	/*
	 * Output controls that map to output mixer switches. Note these are
	 * only for swapped L-to-R and R-to-L routes. See below stereo controls
	 * for direct L-to-L and R-to-R routes.
	 */
	SOC_SINGLE_TLV("Left Line Mixer Line2R Bypass Volume",
		       LINE2R_2_LLOPM_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Left Line Mixer PGAR Bypass Volume",
		       PGAR_2_LLOPM_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Left Line Mixer DACR1 Playback Volume",
		       DACR1_2_LLOPM_VOL, 0, 118, 1, output_stage_tlv),

	SOC_SINGLE_TLV("Right Line Mixer Line2L Bypass Volume",
		       LINE2L_2_RLOPM_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Right Line Mixer PGAL Bypass Volume",
		       PGAL_2_RLOPM_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Right Line Mixer DACL1 Playback Volume",
		       DACL1_2_RLOPM_VOL, 0, 118, 1, output_stage_tlv),

	SOC_SINGLE_TLV("Left HP Mixer Line2R Bypass Volume",
		       LINE2R_2_HPLOUT_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Left HP Mixer PGAR Bypass Volume",
		       PGAR_2_HPLOUT_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Left HP Mixer DACR1 Playback Volume",
		       DACR1_2_HPLOUT_VOL, 0, 118, 1, output_stage_tlv),

	SOC_SINGLE_TLV("Right HP Mixer Line2L Bypass Volume",
		       LINE2L_2_HPROUT_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Right HP Mixer PGAL Bypass Volume",
		       PGAL_2_HPROUT_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Right HP Mixer DACL1 Playback Volume",
		       DACL1_2_HPROUT_VOL, 0, 118, 1, output_stage_tlv),

	SOC_SINGLE_TLV("Left HPCOM Mixer Line2R Bypass Volume",
		       LINE2R_2_HPLCOM_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Left HPCOM Mixer PGAR Bypass Volume",
		       PGAR_2_HPLCOM_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Left HPCOM Mixer DACR1 Playback Volume",
		       DACR1_2_HPLCOM_VOL, 0, 118, 1, output_stage_tlv),

	SOC_SINGLE_TLV("Right HPCOM Mixer Line2L Bypass Volume",
		       LINE2L_2_HPRCOM_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Right HPCOM Mixer PGAL Bypass Volume",
		       PGAL_2_HPRCOM_VOL, 0, 118, 1, output_stage_tlv),
	SOC_SINGLE_TLV("Right HPCOM Mixer DACL1 Playback Volume",
		       DACL1_2_HPRCOM_VOL, 0, 118, 1, output_stage_tlv),

	/* Stereo output controls for direct L-to-L and R-to-R routes */
	SOC_DOUBLE_R_TLV("Line Line2 Bypass Volume",
			 LINE2L_2_LLOPM_VOL, LINE2R_2_RLOPM_VOL,
			 0, 118, 1, output_stage_tlv),
	SOC_DOUBLE_R_TLV("Line PGA Bypass Volume",
			 PGAL_2_LLOPM_VOL, PGAR_2_RLOPM_VOL,
			 0, 118, 1, output_stage_tlv),
	SOC_DOUBLE_R_TLV("Line DAC Playback Volume",
			 DACL1_2_LLOPM_VOL, DACR1_2_RLOPM_VOL,
			 0, 118, 1, output_stage_tlv),

	SOC_DOUBLE_R_TLV("Mono Line2 Bypass Volume",
			 LINE2L_2_MONOLOPM_VOL, LINE2R_2_MONOLOPM_VOL,
			 0, 118, 1, output_stage_tlv),
	SOC_DOUBLE_R_TLV("Mono PGA Bypass Volume",
			 PGAL_2_MONOLOPM_VOL, PGAR_2_MONOLOPM_VOL,
			 0, 118, 1, output_stage_tlv),
	SOC_DOUBLE_R_TLV("Mono DAC Playback Volume",
			 DACL1_2_MONOLOPM_VOL, DACR1_2_MONOLOPM_VOL,
			 0, 118, 1, output_stage_tlv),

	SOC_DOUBLE_R_TLV("HP Line2 Bypass Volume",
			 LINE2L_2_HPLOUT_VOL, LINE2R_2_HPROUT_VOL,
			 0, 118, 1, output_stage_tlv),
	SOC_DOUBLE_R_TLV("HP PGA Bypass Volume",
			 PGAL_2_HPLOUT_VOL, PGAR_2_HPROUT_VOL,
			 0, 118, 1, output_stage_tlv),
	SOC_DOUBLE_R_TLV("HP DAC Playback Volume",
			 DACL1_2_HPLOUT_VOL, DACR1_2_HPROUT_VOL,
			 0, 118, 1, output_stage_tlv),

	SOC_DOUBLE_R_TLV("HPCOM Line2 Bypass Volume",
			 LINE2L_2_HPLCOM_VOL, LINE2R_2_HPRCOM_VOL,
			 0, 118, 1, output_stage_tlv),
	SOC_DOUBLE_R_TLV("HPCOM PGA Bypass Volume",
			 PGAL_2_HPLCOM_VOL, PGAR_2_HPRCOM_VOL,
			 0, 118, 1, output_stage_tlv),
	SOC_DOUBLE_R_TLV("HPCOM DAC Playback Volume",
			 DACL1_2_HPLCOM_VOL, DACR1_2_HPRCOM_VOL,
			 0, 118, 1, output_stage_tlv),

	/* Output pin mute controls */
	SOC_DOUBLE_R("Line Playback Switch", LLOPM_CTRL, RLOPM_CTRL, 3,
		     0x01, 0),
	SOC_SINGLE("Mono Playback Switch", MONOLOPM_CTRL, 3, 0x01, 0),
	SOC_DOUBLE_R("HP Playback Switch", HPLOUT_CTRL, HPROUT_CTRL, 3,
		     0x01, 0),
	SOC_DOUBLE_R("HPCOM Playback Switch", HPLCOM_CTRL, HPRCOM_CTRL, 3,
		     0x01, 0),

	/*
	 * Note: enable Automatic input Gain Controller with care. It can
	 * adjust PGA to max value when ADC is on and will never go back.
	*/
	SOC_DOUBLE_R("AGC Switch", LAGC_CTRL_A, RAGC_CTRL_A, 7, 0x01, 0),

	/* Input */
	SOC_DOUBLE_R_TLV("PGA Capture Volume", LADC_VOL, RADC_VOL,
			 0, 119, 0, adc_tlv),
	SOC_DOUBLE_R("PGA Capture Switch", LADC_VOL, RADC_VOL, 7, 0x01, 1),

	SOC_ENUM("ADC HPF Cut-off", aic3x_enum[ADC_HPF_ENUM]),
};

/*
 * Class-D amplifier gain. From 0 to 18 dB in 6 dB steps
 */
static DECLARE_TLV_DB_SCALE(classd_amp_tlv, 0, 600, 0);

static const struct snd_kcontrol_new aic3x_classd_amp_gain_ctrl =
	SOC_DOUBLE_TLV("Class-D Amplifier Gain", CLASSD_CTRL, 6, 4, 3, 0, classd_amp_tlv);

/* Left DAC Mux */
static const struct snd_kcontrol_new aic3x_left_dac_mux_controls =
SOC_DAPM_ENUM("Route", aic3x_enum[LDAC_ENUM]);

/* Right DAC Mux */
static const struct snd_kcontrol_new aic3x_right_dac_mux_controls =
SOC_DAPM_ENUM("Route", aic3x_enum[RDAC_ENUM]);

/* Left HPCOM Mux */
static const struct snd_kcontrol_new aic3x_left_hpcom_mux_controls =
SOC_DAPM_ENUM("Route", aic3x_enum[LHPCOM_ENUM]);

/* Right HPCOM Mux */
static const struct snd_kcontrol_new aic3x_right_hpcom_mux_controls =
SOC_DAPM_ENUM("Route", aic3x_enum[RHPCOM_ENUM]);

/* Left Line Mixer */
static const struct snd_kcontrol_new aic3x_left_line_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line2L Bypass Switch", LINE2L_2_LLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAL Bypass Switch", PGAL_2_LLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACL1 Switch", DACL1_2_LLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("Line2R Bypass Switch", LINE2R_2_LLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAR Bypass Switch", PGAR_2_LLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACR1 Switch", DACR1_2_LLOPM_VOL, 7, 1, 0),
};

/* Right Line Mixer */
static const struct snd_kcontrol_new aic3x_right_line_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line2L Bypass Switch", LINE2L_2_RLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAL Bypass Switch", PGAL_2_RLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACL1 Switch", DACL1_2_RLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("Line2R Bypass Switch", LINE2R_2_RLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAR Bypass Switch", PGAR_2_RLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACR1 Switch", DACR1_2_RLOPM_VOL, 7, 1, 0),
};

/* Mono Mixer */
static const struct snd_kcontrol_new aic3x_mono_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line2L Bypass Switch", LINE2L_2_MONOLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAL Bypass Switch", PGAL_2_MONOLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACL1 Switch", DACL1_2_MONOLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("Line2R Bypass Switch", LINE2R_2_MONOLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAR Bypass Switch", PGAR_2_MONOLOPM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACR1 Switch", DACR1_2_MONOLOPM_VOL, 7, 1, 0),
};

/* Left HP Mixer */
static const struct snd_kcontrol_new aic3x_left_hp_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line2L Bypass Switch", LINE2L_2_HPLOUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAL Bypass Switch", PGAL_2_HPLOUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACL1 Switch", DACL1_2_HPLOUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("Line2R Bypass Switch", LINE2R_2_HPLOUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAR Bypass Switch", PGAR_2_HPLOUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACR1 Switch", DACR1_2_HPLOUT_VOL, 7, 1, 0),
};

/* Right HP Mixer */
static const struct snd_kcontrol_new aic3x_right_hp_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line2L Bypass Switch", LINE2L_2_HPROUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAL Bypass Switch", PGAL_2_HPROUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACL1 Switch", DACL1_2_HPROUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("Line2R Bypass Switch", LINE2R_2_HPROUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAR Bypass Switch", PGAR_2_HPROUT_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACR1 Switch", DACR1_2_HPROUT_VOL, 7, 1, 0),
};

/* Left HPCOM Mixer */
static const struct snd_kcontrol_new aic3x_left_hpcom_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line2L Bypass Switch", LINE2L_2_HPLCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAL Bypass Switch", PGAL_2_HPLCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACL1 Switch", DACL1_2_HPLCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("Line2R Bypass Switch", LINE2R_2_HPLCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAR Bypass Switch", PGAR_2_HPLCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACR1 Switch", DACR1_2_HPLCOM_VOL, 7, 1, 0),
};

/* Right HPCOM Mixer */
static const struct snd_kcontrol_new aic3x_right_hpcom_mixer_controls[] = {
	SOC_DAPM_SINGLE("Line2L Bypass Switch", LINE2L_2_HPRCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAL Bypass Switch", PGAL_2_HPRCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACL1 Switch", DACL1_2_HPRCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("Line2R Bypass Switch", LINE2R_2_HPRCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("PGAR Bypass Switch", PGAR_2_HPRCOM_VOL, 7, 1, 0),
	SOC_DAPM_SINGLE("DACR1 Switch", DACR1_2_HPRCOM_VOL, 7, 1, 0),
};

/* Left PGA Mixer */
static const struct snd_kcontrol_new aic3x_left_pga_mixer_controls[] = {
	SOC_DAPM_SINGLE_AIC3X("Line1L Switch", LINE1L_2_LADC_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE_AIC3X("Line1R Switch", LINE1R_2_LADC_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE_AIC3X("Line2L Switch", LINE2L_2_LADC_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE_AIC3X("Mic3L Switch", MIC3LR_2_LADC_CTRL, 4, 1, 1),
	SOC_DAPM_SINGLE_AIC3X("Mic3R Switch", MIC3LR_2_LADC_CTRL, 0, 1, 1),
};

/* Right PGA Mixer */
static const struct snd_kcontrol_new aic3x_right_pga_mixer_controls[] = {
	SOC_DAPM_SINGLE_AIC3X("Line1R Switch", LINE1R_2_RADC_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE_AIC3X("Line1L Switch", LINE1L_2_RADC_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE_AIC3X("Line2R Switch", LINE2R_2_RADC_CTRL, 3, 1, 1),
	SOC_DAPM_SINGLE_AIC3X("Mic3L Switch", MIC3LR_2_RADC_CTRL, 4, 1, 1),
	SOC_DAPM_SINGLE_AIC3X("Mic3R Switch", MIC3LR_2_RADC_CTRL, 0, 1, 1),
};

/* Left Line1 Mux */
static const struct snd_kcontrol_new aic3x_left_line1_mux_controls =
SOC_DAPM_ENUM("Route", aic3x_enum[LINE1L_ENUM]);

/* Right Line1 Mux */
static const struct snd_kcontrol_new aic3x_right_line1_mux_controls =
SOC_DAPM_ENUM("Route", aic3x_enum[LINE1R_ENUM]);

/* Left Line2 Mux */
static const struct snd_kcontrol_new aic3x_left_line2_mux_controls =
SOC_DAPM_ENUM("Route", aic3x_enum[LINE2L_ENUM]);

/* Right Line2 Mux */
static const struct snd_kcontrol_new aic3x_right_line2_mux_controls =
SOC_DAPM_ENUM("Route", aic3x_enum[LINE2R_ENUM]);

static const struct snd_soc_dapm_widget aic3x_dapm_widgets[] = {
	/* Left DAC to Left Outputs */
	SND_SOC_DAPM_DAC("Left DAC", "Left Playback", DAC_PWR, 7, 0),
	SND_SOC_DAPM_MUX("Left DAC Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_left_dac_mux_controls),
	SND_SOC_DAPM_MUX("Left HPCOM Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_left_hpcom_mux_controls),
	SND_SOC_DAPM_PGA("Left Line Out", LLOPM_CTRL, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Left HP Out", HPLOUT_CTRL, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Left HP Com", HPLCOM_CTRL, 0, 0, NULL, 0),

	/* Right DAC to Right Outputs */
	SND_SOC_DAPM_DAC("Right DAC", "Right Playback", DAC_PWR, 6, 0),
	SND_SOC_DAPM_MUX("Right DAC Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_right_dac_mux_controls),
	SND_SOC_DAPM_MUX("Right HPCOM Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_right_hpcom_mux_controls),
	SND_SOC_DAPM_PGA("Right Line Out", RLOPM_CTRL, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Right HP Out", HPROUT_CTRL, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Right HP Com", HPRCOM_CTRL, 0, 0, NULL, 0),

	/* Mono Output */
	SND_SOC_DAPM_PGA("Mono Out", MONOLOPM_CTRL, 0, 0, NULL, 0),

	/* Inputs to Left ADC */
	SND_SOC_DAPM_ADC("Left ADC", "Left Capture", LINE1L_2_LADC_CTRL, 2, 0),
	SND_SOC_DAPM_MIXER("Left PGA Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_left_pga_mixer_controls[0],
			   ARRAY_SIZE(aic3x_left_pga_mixer_controls)),
	SND_SOC_DAPM_MUX("Left Line1L Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_left_line1_mux_controls),
	SND_SOC_DAPM_MUX("Left Line1R Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_left_line1_mux_controls),
	SND_SOC_DAPM_MUX("Left Line2L Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_left_line2_mux_controls),

	/* Inputs to Right ADC */
	SND_SOC_DAPM_ADC("Right ADC", "Right Capture",
			 LINE1R_2_RADC_CTRL, 2, 0),
	SND_SOC_DAPM_MIXER("Right PGA Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_right_pga_mixer_controls[0],
			   ARRAY_SIZE(aic3x_right_pga_mixer_controls)),
	SND_SOC_DAPM_MUX("Right Line1L Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_right_line1_mux_controls),
	SND_SOC_DAPM_MUX("Right Line1R Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_right_line1_mux_controls),
	SND_SOC_DAPM_MUX("Right Line2R Mux", SND_SOC_NOPM, 0, 0,
			 &aic3x_right_line2_mux_controls),

	/*
	 * Not a real mic bias widget but similar function. This is for dynamic
	 * control of GPIO1 digital mic modulator clock output function when
	 * using digital mic.
	 */
	SND_SOC_DAPM_REG(snd_soc_dapm_micbias, "GPIO1 dmic modclk",
			 AIC3X_GPIO1_REG, 4, 0xf,
			 AIC3X_GPIO1_FUNC_DIGITAL_MIC_MODCLK,
			 AIC3X_GPIO1_FUNC_DISABLED),

	/*
	 * Also similar function like mic bias. Selects digital mic with
	 * configurable oversampling rate instead of ADC converter.
	 */
	SND_SOC_DAPM_REG(snd_soc_dapm_micbias, "DMic Rate 128",
			 AIC3X_ASD_INTF_CTRLA, 0, 3, 1, 0),
	SND_SOC_DAPM_REG(snd_soc_dapm_micbias, "DMic Rate 64",
			 AIC3X_ASD_INTF_CTRLA, 0, 3, 2, 0),
	SND_SOC_DAPM_REG(snd_soc_dapm_micbias, "DMic Rate 32",
			 AIC3X_ASD_INTF_CTRLA, 0, 3, 3, 0),

	/* Mic Bias */
	SND_SOC_DAPM_REG(snd_soc_dapm_micbias, "Mic Bias 2V",
			 MICBIAS_CTRL, 6, 3, 1, 0),
	SND_SOC_DAPM_REG(snd_soc_dapm_micbias, "Mic Bias 2.5V",
			 MICBIAS_CTRL, 6, 3, 2, 0),
	SND_SOC_DAPM_REG(snd_soc_dapm_micbias, "Mic Bias AVDD",
			 MICBIAS_CTRL, 6, 3, 3, 0),

	/* Output mixers */
	SND_SOC_DAPM_MIXER("Left Line Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_left_line_mixer_controls[0],
			   ARRAY_SIZE(aic3x_left_line_mixer_controls)),
	SND_SOC_DAPM_MIXER("Right Line Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_right_line_mixer_controls[0],
			   ARRAY_SIZE(aic3x_right_line_mixer_controls)),
	SND_SOC_DAPM_MIXER("Mono Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_mono_mixer_controls[0],
			   ARRAY_SIZE(aic3x_mono_mixer_controls)),
	SND_SOC_DAPM_MIXER("Left HP Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_left_hp_mixer_controls[0],
			   ARRAY_SIZE(aic3x_left_hp_mixer_controls)),
	SND_SOC_DAPM_MIXER("Right HP Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_right_hp_mixer_controls[0],
			   ARRAY_SIZE(aic3x_right_hp_mixer_controls)),
	SND_SOC_DAPM_MIXER("Left HPCOM Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_left_hpcom_mixer_controls[0],
			   ARRAY_SIZE(aic3x_left_hpcom_mixer_controls)),
	SND_SOC_DAPM_MIXER("Right HPCOM Mixer", SND_SOC_NOPM, 0, 0,
			   &aic3x_right_hpcom_mixer_controls[0],
			   ARRAY_SIZE(aic3x_right_hpcom_mixer_controls)),

	SND_SOC_DAPM_OUTPUT("LLOUT"),
	SND_SOC_DAPM_OUTPUT("RLOUT"),
	SND_SOC_DAPM_OUTPUT("MONO_LOUT"),
	SND_SOC_DAPM_OUTPUT("HPLOUT"),
	SND_SOC_DAPM_OUTPUT("HPROUT"),
	SND_SOC_DAPM_OUTPUT("HPLCOM"),
	SND_SOC_DAPM_OUTPUT("HPRCOM"),

	SND_SOC_DAPM_INPUT("MIC3L"),
	SND_SOC_DAPM_INPUT("MIC3R"),
	SND_SOC_DAPM_INPUT("LINE1L"),
	SND_SOC_DAPM_INPUT("LINE1R"),
	SND_SOC_DAPM_INPUT("LINE2L"),
	SND_SOC_DAPM_INPUT("LINE2R"),

	/*
	 * Virtual output pin to detection block inside codec. This can be
	 * used to keep codec bias on if gpio or detection features are needed.
	 * Force pin on or construct a path with an input jack and mic bias
	 * widgets.
	 */
	SND_SOC_DAPM_OUTPUT("Detection"),
};

static const struct snd_soc_dapm_widget aic3007_dapm_widgets[] = {
	/* Class-D outputs */
	SND_SOC_DAPM_PGA("Left Class-D Out", CLASSD_CTRL, 3, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Right Class-D Out", CLASSD_CTRL, 2, 0, NULL, 0),

	SND_SOC_DAPM_OUTPUT("SPOP"),
	SND_SOC_DAPM_OUTPUT("SPOM"),
};

static const struct snd_soc_dapm_route intercon[] = {
	/* Left Input */
	{"Left Line1L Mux", "single-ended", "LINE1L"},
	{"Left Line1L Mux", "differential", "LINE1L"},

	{"Left Line2L Mux", "single-ended", "LINE2L"},
	{"Left Line2L Mux", "differential", "LINE2L"},

	{"Left PGA Mixer", "Line1L Switch", "Left Line1L Mux"},
	{"Left PGA Mixer", "Line1R Switch", "Left Line1R Mux"},
	{"Left PGA Mixer", "Line2L Switch", "Left Line2L Mux"},
	{"Left PGA Mixer", "Mic3L Switch", "MIC3L"},
	{"Left PGA Mixer", "Mic3R Switch", "MIC3R"},

	{"Left ADC", NULL, "Left PGA Mixer"},
	{"Left ADC", NULL, "GPIO1 dmic modclk"},

	/* Right Input */
	{"Right Line1R Mux", "single-ended", "LINE1R"},
	{"Right Line1R Mux", "differential", "LINE1R"},

	{"Right Line2R Mux", "single-ended", "LINE2R"},
	{"Right Line2R Mux", "differential", "LINE2R"},

	{"Right PGA Mixer", "Line1L Switch", "Right Line1L Mux"},
	{"Right PGA Mixer", "Line1R Switch", "Right Line1R Mux"},
	{"Right PGA Mixer", "Line2R Switch", "Right Line2R Mux"},
	{"Right PGA Mixer", "Mic3L Switch", "MIC3L"},
	{"Right PGA Mixer", "Mic3R Switch", "MIC3R"},

	{"Right ADC", NULL, "Right PGA Mixer"},
	{"Right ADC", NULL, "GPIO1 dmic modclk"},

	/*
	 * Logical path between digital mic enable and GPIO1 modulator clock
	 * output function
	 */
	{"GPIO1 dmic modclk", NULL, "DMic Rate 128"},
	{"GPIO1 dmic modclk", NULL, "DMic Rate 64"},
	{"GPIO1 dmic modclk", NULL, "DMic Rate 32"},

	/* Left DAC Output */
	{"Left DAC Mux", "DAC_L1", "Left DAC"},
	{"Left DAC Mux", "DAC_L2", "Left DAC"},
	{"Left DAC Mux", "DAC_L3", "Left DAC"},

	/* Right DAC Output */
	{"Right DAC Mux", "DAC_R1", "Right DAC"},
	{"Right DAC Mux", "DAC_R2", "Right DAC"},
	{"Right DAC Mux", "DAC_R3", "Right DAC"},

	/* Left Line Output */
	{"Left Line Mixer", "Line2L Bypass Switch", "Left Line2L Mux"},
	{"Left Line Mixer", "PGAL Bypass Switch", "Left PGA Mixer"},
	{"Left Line Mixer", "DACL1 Switch", "Left DAC Mux"},
	{"Left Line Mixer", "Line2R Bypass Switch", "Right Line2R Mux"},
	{"Left Line Mixer", "PGAR Bypass Switch", "Right PGA Mixer"},
	{"Left Line Mixer", "DACR1 Switch", "Right DAC Mux"},

	{"Left Line Out", NULL, "Left Line Mixer"},
	{"Left Line Out", NULL, "Left DAC Mux"},
	{"LLOUT", NULL, "Left Line Out"},

	/* Right Line Output */
	{"Right Line Mixer", "Line2L Bypass Switch", "Left Line2L Mux"},
	{"Right Line Mixer", "PGAL Bypass Switch", "Left PGA Mixer"},
	{"Right Line Mixer", "DACL1 Switch", "Left DAC Mux"},
	{"Right Line Mixer", "Line2R Bypass Switch", "Right Line2R Mux"},
	{"Right Line Mixer", "PGAR Bypass Switch", "Right PGA Mixer"},
	{"Right Line Mixer", "DACR1 Switch", "Right DAC Mux"},

	{"Right Line Out", NULL, "Right Line Mixer"},
	{"Right Line Out", NULL, "Right DAC Mux"},
	{"RLOUT", NULL, "Right Line Out"},

	/* Mono Output */
	{"Mono Mixer", "Line2L Bypass Switch", "Left Line2L Mux"},
	{"Mono Mixer", "PGAL Bypass Switch", "Left PGA Mixer"},
	{"Mono Mixer", "DACL1 Switch", "Left DAC Mux"},
	{"Mono Mixer", "Line2R Bypass Switch", "Right Line2R Mux"},
	{"Mono Mixer", "PGAR Bypass Switch", "Right PGA Mixer"},
	{"Mono Mixer", "DACR1 Switch", "Right DAC Mux"},

	{"Mono Out", NULL, "Mono Mixer"},
	{"MONO_LOUT", NULL, "Mono Out"},

	/* Left HP Output */
	{"Left HP Mixer", "Line2L Bypass Switch", "Left Line2L Mux"},
	{"Left HP Mixer", "PGAL Bypass Switch", "Left PGA Mixer"},
	{"Left HP Mixer", "DACL1 Switch", "Left DAC Mux"},
	{"Left HP Mixer", "Line2R Bypass Switch", "Right Line2R Mux"},
	{"Left HP Mixer", "PGAR Bypass Switch", "Right PGA Mixer"},
	{"Left HP Mixer", "DACR1 Switch", "Right DAC Mux"},

	{"Left HP Out", NULL, "Left HP Mixer"},
	{"Left HP Out", NULL, "Left DAC Mux"},
	{"HPLOUT", NULL, "Left HP Out"},

	/* Right HP Output */
	{"Right HP Mixer", "Line2L Bypass Switch", "Left Line2L Mux"},
	{"Right HP Mixer", "PGAL Bypass Switch", "Left PGA Mixer"},
	{"Right HP Mixer", "DACL1 Switch", "Left DAC Mux"},
	{"Right HP Mixer", "Line2R Bypass Switch", "Right Line2R Mux"},
	{"Right HP Mixer", "PGAR Bypass Switch", "Right PGA Mixer"},
	{"Right HP Mixer", "DACR1 Switch", "Right DAC Mux"},

	{"Right HP Out", NULL, "Right HP Mixer"},
	{"Right HP Out", NULL, "Right DAC Mux"},
	{"HPROUT", NULL, "Right HP Out"},

	/* Left HPCOM Output */
	{"Left HPCOM Mixer", "Line2L Bypass Switch", "Left Line2L Mux"},
	{"Left HPCOM Mixer", "PGAL Bypass Switch", "Left PGA Mixer"},
	{"Left HPCOM Mixer", "DACL1 Switch", "Left DAC Mux"},
	{"Left HPCOM Mixer", "Line2R Bypass Switch", "Right Line2R Mux"},
	{"Left HPCOM Mixer", "PGAR Bypass Switch", "Right PGA Mixer"},
	{"Left HPCOM Mixer", "DACR1 Switch", "Right DAC Mux"},

	{"Left HPCOM Mux", "differential of HPLOUT", "Left HP Mixer"},
	{"Left HPCOM Mux", "constant VCM", "Left HPCOM Mixer"},
	{"Left HPCOM Mux", "single-ended", "Left HPCOM Mixer"},
	{"Left HP Com", NULL, "Left HPCOM Mux"},
	{"HPLCOM", NULL, "Left HP Com"},

	/* Right HPCOM Output */
	{"Right HPCOM Mixer", "Line2L Bypass Switch", "Left Line2L Mux"},
	{"Right HPCOM Mixer", "PGAL Bypass Switch", "Left PGA Mixer"},
	{"Right HPCOM Mixer", "DACL1 Switch", "Left DAC Mux"},
	{"Right HPCOM Mixer", "Line2R Bypass Switch", "Right Line2R Mux"},
	{"Right HPCOM Mixer", "PGAR Bypass Switch", "Right PGA Mixer"},
	{"Right HPCOM Mixer", "DACR1 Switch", "Right DAC Mux"},

	{"Right HPCOM Mux", "differential of HPROUT", "Right HP Mixer"},
	{"Right HPCOM Mux", "constant VCM", "Right HPCOM Mixer"},
	{"Right HPCOM Mux", "single-ended", "Right HPCOM Mixer"},
	{"Right HPCOM Mux", "differential of HPLCOM", "Left HPCOM Mixer"},
	{"Right HPCOM Mux", "external feedback", "Right HPCOM Mixer"},
	{"Right HP Com", NULL, "Right HPCOM Mux"},
	{"HPRCOM", NULL, "Right HP Com"},
};

static const struct snd_soc_dapm_route intercon_3007[] = {
	/* Class-D outputs */
	{"Left Class-D Out", NULL, "Left Line Out"},
	{"Right Class-D Out", NULL, "Left Line Out"},
	{"SPOP", NULL, "Left Class-D Out"},
	{"SPOM", NULL, "Right Class-D Out"},
};

static int aic3x_add_widgets(struct snd_soc_codec *codec)
{
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);

	snd_soc_dapm_new_controls(codec, aic3x_dapm_widgets,
				  ARRAY_SIZE(aic3x_dapm_widgets));

	/* set up audio path interconnects */
	snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));

	if (aic3x->model == AIC3X_MODEL_3007) {
		snd_soc_dapm_new_controls(codec, aic3007_dapm_widgets,
			ARRAY_SIZE(aic3007_dapm_widgets));
		snd_soc_dapm_add_routes(codec, intercon_3007, ARRAY_SIZE(intercon_3007));
	}

	return 0;
}

static int aic3x_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params,
			   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec =rtd->codec;
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);
	int codec_clk = 0, bypass_pll = 0, fsref, last_clk = 0;
	u8 data, j, r, p, pll_q, pll_p = 1, pll_r = 1, pll_j = 1;
	u16 d, pll_d = 1;
	u8 reg;
	int clk;

	/* select data word length */
	data = snd_soc_read(codec, AIC3X_ASD_INTF_CTRLB) & (~(0x3 << 4));
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		data |= (0x01 << 4);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		data |= (0x02 << 4);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		data |= (0x03 << 4);
		break;
	}
	snd_soc_write(codec, AIC3X_ASD_INTF_CTRLB, data);

	/* Fsref can be 44100 or 48000 */
	fsref = (params_rate(params) % 11025 == 0) ? 44100 : 48000;

	/* Try to find a value for Q which allows us to bypass the PLL and
	 * generate CODEC_CLK directly. */
	for (pll_q = 2; pll_q < 18; pll_q++)
		if (aic3x->sysclk / (128 * pll_q) == fsref) {
			bypass_pll = 1;
			break;
		}

	if (bypass_pll) {
		pll_q &= 0xf;
		snd_soc_write(codec, AIC3X_PLL_PROGA_REG, pll_q << PLLQ_SHIFT);
		snd_soc_write(codec, AIC3X_GPIOB_REG, CODEC_CLKIN_CLKDIV);
		/* disable PLL if it is bypassed */
		reg = snd_soc_read(codec, AIC3X_PLL_PROGA_REG);
		snd_soc_write(codec, AIC3X_PLL_PROGA_REG, reg & ~PLL_ENABLE);

	} else {
		snd_soc_write(codec, AIC3X_GPIOB_REG, CODEC_CLKIN_PLLDIV);
		/* enable PLL when it is used */
		reg = snd_soc_read(codec, AIC3X_PLL_PROGA_REG);
		snd_soc_write(codec, AIC3X_PLL_PROGA_REG, reg | PLL_ENABLE);
	}

	/* Route Left DAC to left channel input and
	 * right DAC to right channel input */
	data = (LDAC2LCH | RDAC2RCH);
	data |= (fsref == 44100) ? FSREF_44100 : FSREF_48000;
	if (params_rate(params) >= 64000)
		data |= DUAL_RATE_MODE;
	snd_soc_write(codec, AIC3X_CODEC_DATAPATH_REG, data);

	/* codec sample rate select */
	data = (fsref * 20) / params_rate(params);
	if (params_rate(params) < 64000)
		data /= 2;
	data /= 5;
	data -= 2;
	data |= (data << 4);
	snd_soc_write(codec, AIC3X_SAMPLE_RATE_SEL_REG, data);

	if (bypass_pll)
		return 0;

	/* Use PLL, compute apropriate setup for j, d, r and p, the closest
	 * one wins the game. Try with d==0 first, next with d!=0.
	 * Constraints for j are according to the datasheet.
	 * The sysclk is divided by 1000 to prevent integer overflows.
	 */

	codec_clk = (2048 * fsref) / (aic3x->sysclk / 1000);

	for (r = 1; r <= 16; r++)
		for (p = 1; p <= 8; p++) {
			for (j = 4; j <= 55; j++) {
				/* This is actually 1000*((j+(d/10000))*r)/p
				 * The term had to be converted to get
				 * rid of the division by 10000; d = 0 here
				 */
				int tmp_clk = (1000 * j * r) / p;

				/* Check whether this values get closer than
				 * the best ones we had before
				 */
				if (abs(codec_clk - tmp_clk) <
					abs(codec_clk - last_clk)) {
					pll_j = j; pll_d = 0;
					pll_r = r; pll_p = p;
					last_clk = tmp_clk;
				}

				/* Early exit for exact matches */
				if (tmp_clk == codec_clk)
					goto found;
			}
		}

	/* try with d != 0 */
	for (p = 1; p <= 8; p++) {
		j = codec_clk * p / 1000;

		if (j < 4 || j > 11)
			continue;

		/* do not use codec_clk here since we'd loose precision */
		d = ((2048 * p * fsref) - j * aic3x->sysclk)
			* 100 / (aic3x->sysclk/100);

		clk = (10000 * j + d) / (10 * p);

		/* check whether this values get closer than the best
		 * ones we had before */
		if (abs(codec_clk - clk) < abs(codec_clk - last_clk)) {
			pll_j = j; pll_d = d; pll_r = 1; pll_p = p;
			last_clk = clk;
		}

		/* Early exit for exact matches */
		if (clk == codec_clk)
			goto found;
	}

	if (last_clk == 0) {
		printk(KERN_ERR "%s(): unable to setup PLL\n", __func__);
		return -EINVAL;
	}

found:
	data = snd_soc_read(codec, AIC3X_PLL_PROGA_REG);
	snd_soc_write(codec, AIC3X_PLL_PROGA_REG,
		      data | (pll_p << PLLP_SHIFT));
	snd_soc_write(codec, AIC3X_OVRF_STATUS_AND_PLLR_REG,
		      pll_r << PLLR_SHIFT);
	snd_soc_write(codec, AIC3X_PLL_PROGB_REG, pll_j << PLLJ_SHIFT);
	snd_soc_write(codec, AIC3X_PLL_PROGC_REG,
		      (pll_d >> 6) << PLLD_MSB_SHIFT);
	snd_soc_write(codec, AIC3X_PLL_PROGD_REG,
		      (pll_d & 0x3F) << PLLD_LSB_SHIFT);

	return 0;
}

static int aic3x_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 ldac_reg = snd_soc_read(codec, LDAC_VOL) & ~MUTE_ON;
	u8 rdac_reg = snd_soc_read(codec, RDAC_VOL) & ~MUTE_ON;

	if (mute) {
		snd_soc_write(codec, LDAC_VOL, ldac_reg | MUTE_ON);
		snd_soc_write(codec, RDAC_VOL, rdac_reg | MUTE_ON);
	} else {
		snd_soc_write(codec, LDAC_VOL, ldac_reg);
		snd_soc_write(codec, RDAC_VOL, rdac_reg);
	}

	return 0;
}

static int aic3x_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);

	aic3x->sysclk = freq;
	return 0;
}

static int aic3x_set_dai_fmt(struct snd_soc_dai *codec_dai,
			     unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);
	u8 iface_areg, iface_breg;
	int delay = 0;

	iface_areg = snd_soc_read(codec, AIC3X_ASD_INTF_CTRLA) & 0x3f;
	iface_breg = snd_soc_read(codec, AIC3X_ASD_INTF_CTRLB) & 0x3f;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		aic3x->master = 1;
		iface_areg |= BIT_CLK_MASTER | WORD_CLK_MASTER;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		aic3x->master = 0;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * match both interface format and signal polarities since they
	 * are fixed
	 */
	switch (fmt & (SND_SOC_DAIFMT_FORMAT_MASK |
		       SND_SOC_DAIFMT_INV_MASK)) {
	case (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF):
		break;
	case (SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_IB_NF):
		delay = 1;
	case (SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_IB_NF):
		iface_breg |= (0x01 << 6);
		break;
	case (SND_SOC_DAIFMT_RIGHT_J | SND_SOC_DAIFMT_NB_NF):
		iface_breg |= (0x02 << 6);
		break;
	case (SND_SOC_DAIFMT_LEFT_J | SND_SOC_DAIFMT_NB_NF):
		iface_breg |= (0x03 << 6);
		break;
	default:
		return -EINVAL;
	}

	/* set iface */
	snd_soc_write(codec, AIC3X_ASD_INTF_CTRLA, iface_areg);
	snd_soc_write(codec, AIC3X_ASD_INTF_CTRLB, iface_breg);
	snd_soc_write(codec, AIC3X_ASD_INTF_CTRLC, delay);

	return 0;
}

static int aic3x_init_3007(struct snd_soc_codec *codec)
{
	u8 tmp1, tmp2, *cache = codec->reg_cache;

	/*
	 * There is no need to cache writes to undocumented page 0xD but
	 * respective page 0 register cache entries must be preserved
	 */
	tmp1 = cache[0xD];
	tmp2 = cache[0x8];
	/* Class-D speaker driver init; datasheet p. 46 */
	snd_soc_write(codec, AIC3X_PAGE_SELECT, 0x0D);
	snd_soc_write(codec, 0xD, 0x0D);
	snd_soc_write(codec, 0x8, 0x5C);
	snd_soc_write(codec, 0x8, 0x5D);
	snd_soc_write(codec, 0x8, 0x5C);
	snd_soc_write(codec, AIC3X_PAGE_SELECT, 0x00);
	cache[0xD] = tmp1;
	cache[0x8] = tmp2;

	return 0;
}

static int aic3x_regulator_event(struct notifier_block *nb,
				 unsigned long event, void *data)
{
	struct aic3x_disable_nb *disable_nb =
		container_of(nb, struct aic3x_disable_nb, nb);
	struct aic3x_priv *aic3x = disable_nb->aic3x;

	if (event & REGULATOR_EVENT_DISABLE) {
		/*
		 * Put codec to reset and require cache sync as at least one
		 * of the supplies was disabled
		 */
		if (aic3x->gpio_reset >= 0)
			gpio_set_value(aic3x->gpio_reset, 0);
		aic3x->codec->cache_sync = 1;
	}

	return 0;
}

static int aic3x_set_power(struct snd_soc_codec *codec, int power)
{
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);
	int i, ret;
	u8 *cache = codec->reg_cache;

	if (power) {
		ret = regulator_bulk_enable(ARRAY_SIZE(aic3x->supplies),
					    aic3x->supplies);
		if (ret)
			goto out;
		aic3x->power = 1;
		/*
		 * Reset release and cache sync is necessary only if some
		 * supply was off or if there were cached writes
		 */
		if (!codec->cache_sync)
			goto out;

		if (aic3x->gpio_reset >= 0) {
			udelay(1);
			gpio_set_value(aic3x->gpio_reset, 1);
		}

		/* Sync reg_cache with the hardware */
		codec->cache_only = 0;
		for (i = 0; i < ARRAY_SIZE(aic3x_reg); i++)
			snd_soc_write(codec, i, cache[i]);
		if (aic3x->model == AIC3X_MODEL_3007)
			aic3x_init_3007(codec);
		codec->cache_sync = 0;
	} else {
		aic3x->power = 0;
		/* HW writes are needless when bias is off */
		codec->cache_only = 1;
		ret = regulator_bulk_disable(ARRAY_SIZE(aic3x->supplies),
					     aic3x->supplies);
	}
out:
	return ret;
}

static int aic3x_set_bias_level(struct snd_soc_codec *codec,
				enum snd_soc_bias_level level)
{
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);
	u8 reg;

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		if (codec->bias_level == SND_SOC_BIAS_STANDBY &&
		    aic3x->master) {
			/* enable pll */
			reg = snd_soc_read(codec, AIC3X_PLL_PROGA_REG);
			snd_soc_write(codec, AIC3X_PLL_PROGA_REG,
				      reg | PLL_ENABLE);
		}
		break;
	case SND_SOC_BIAS_STANDBY:
		if (!aic3x->power)
			aic3x_set_power(codec, 1);
		if (codec->bias_level == SND_SOC_BIAS_PREPARE &&
		    aic3x->master) {
			/* disable pll */
			reg = snd_soc_read(codec, AIC3X_PLL_PROGA_REG);
			snd_soc_write(codec, AIC3X_PLL_PROGA_REG,
				      reg & ~PLL_ENABLE);
		}
		break;
	case SND_SOC_BIAS_OFF:
		if (aic3x->power)
			aic3x_set_power(codec, 0);
		break;
	}
	codec->bias_level = level;

	return 0;
}

void aic3x_set_gpio(struct snd_soc_codec *codec, int gpio, int state)
{
	u8 reg = gpio ? AIC3X_GPIO2_REG : AIC3X_GPIO1_REG;
	u8 bit = gpio ? 3: 0;
	u8 val = snd_soc_read(codec, reg) & ~(1 << bit);
	snd_soc_write(codec, reg, val | (!!state << bit));
}
EXPORT_SYMBOL_GPL(aic3x_set_gpio);

int aic3x_get_gpio(struct snd_soc_codec *codec, int gpio)
{
	u8 reg = gpio ? AIC3X_GPIO2_REG : AIC3X_GPIO1_REG;
	u8 val, bit = gpio ? 2: 1;

	aic3x_read(codec, reg, &val);
	return (val >> bit) & 1;
}
EXPORT_SYMBOL_GPL(aic3x_get_gpio);

void aic3x_set_headset_detection(struct snd_soc_codec *codec, int detect,
				 int headset_debounce, int button_debounce)
{
	u8 val;

	val = ((detect & AIC3X_HEADSET_DETECT_MASK)
		<< AIC3X_HEADSET_DETECT_SHIFT) |
	      ((headset_debounce & AIC3X_HEADSET_DEBOUNCE_MASK)
		<< AIC3X_HEADSET_DEBOUNCE_SHIFT) |
	      ((button_debounce & AIC3X_BUTTON_DEBOUNCE_MASK)
		<< AIC3X_BUTTON_DEBOUNCE_SHIFT);

	if (detect & AIC3X_HEADSET_DETECT_MASK)
		val |= AIC3X_HEADSET_DETECT_ENABLED;

	snd_soc_write(codec, AIC3X_HEADSET_DETECT_CTRL_A, val);
}
EXPORT_SYMBOL_GPL(aic3x_set_headset_detection);

int aic3x_headset_detected(struct snd_soc_codec *codec)
{
	u8 val;
	aic3x_read(codec, AIC3X_HEADSET_DETECT_CTRL_B, &val);
	return (val >> 4) & 1;
}
EXPORT_SYMBOL_GPL(aic3x_headset_detected);

int aic3x_button_pressed(struct snd_soc_codec *codec)
{
	u8 val;
	aic3x_read(codec, AIC3X_HEADSET_DETECT_CTRL_B, &val);
	return (val >> 5) & 1;
}
EXPORT_SYMBOL_GPL(aic3x_button_pressed);

#define AIC3X_RATES	SNDRV_PCM_RATE_8000_96000
#define AIC3X_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
			 SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops aic3x_dai_ops = {
	.hw_params	= aic3x_hw_params,
	.digital_mute	= aic3x_mute,
	.set_sysclk	= aic3x_set_dai_sysclk,
	.set_fmt	= aic3x_set_dai_fmt,
};

static struct snd_soc_dai_driver aic3x_dai = {
	.name = "tlv320aic3x-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = AIC3X_RATES,
		.formats = AIC3X_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = AIC3X_RATES,
		.formats = AIC3X_FORMATS,},
	.ops = &aic3x_dai_ops,
	.symmetric_rates = 1,
};

static int aic3x_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	aic3x_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int aic3x_resume(struct snd_soc_codec *codec)
{
	aic3x_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

/*
 * initialise the AIC3X driver
 * register the mixer and dsp interfaces with the kernel
 */
static int aic3x_init(struct snd_soc_codec *codec)
{
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);
	int reg;

	snd_soc_write(codec, AIC3X_PAGE_SELECT, PAGE0_SELECT);
	snd_soc_write(codec, AIC3X_RESET, SOFT_RESET);

	/* DAC default volume and mute */
	snd_soc_write(codec, LDAC_VOL, DEFAULT_VOL | MUTE_ON);
	snd_soc_write(codec, RDAC_VOL, DEFAULT_VOL | MUTE_ON);

	/* DAC to HP default volume and route to Output mixer */
	snd_soc_write(codec, DACL1_2_HPLOUT_VOL, DEFAULT_VOL | ROUTE_ON);
	snd_soc_write(codec, DACR1_2_HPROUT_VOL, DEFAULT_VOL | ROUTE_ON);
	snd_soc_write(codec, DACL1_2_HPLCOM_VOL, DEFAULT_VOL | ROUTE_ON);
	snd_soc_write(codec, DACR1_2_HPRCOM_VOL, DEFAULT_VOL | ROUTE_ON);
	/* DAC to Line Out default volume and route to Output mixer */
	snd_soc_write(codec, DACL1_2_LLOPM_VOL, DEFAULT_VOL | ROUTE_ON);
	snd_soc_write(codec, DACR1_2_RLOPM_VOL, DEFAULT_VOL | ROUTE_ON);
	/* DAC to Mono Line Out default volume and route to Output mixer */
	snd_soc_write(codec, DACL1_2_MONOLOPM_VOL, DEFAULT_VOL | ROUTE_ON);
	snd_soc_write(codec, DACR1_2_MONOLOPM_VOL, DEFAULT_VOL | ROUTE_ON);

	/* unmute all outputs */
	reg = snd_soc_read(codec, LLOPM_CTRL);
	snd_soc_write(codec, LLOPM_CTRL, reg | UNMUTE);
	reg = snd_soc_read(codec, RLOPM_CTRL);
	snd_soc_write(codec, RLOPM_CTRL, reg | UNMUTE);
	reg = snd_soc_read(codec, MONOLOPM_CTRL);
	snd_soc_write(codec, MONOLOPM_CTRL, reg | UNMUTE);
	reg = snd_soc_read(codec, HPLOUT_CTRL);
	snd_soc_write(codec, HPLOUT_CTRL, reg | UNMUTE);
	reg = snd_soc_read(codec, HPROUT_CTRL);
	snd_soc_write(codec, HPROUT_CTRL, reg | UNMUTE);
	reg = snd_soc_read(codec, HPLCOM_CTRL);
	snd_soc_write(codec, HPLCOM_CTRL, reg | UNMUTE);
	reg = snd_soc_read(codec, HPRCOM_CTRL);
	snd_soc_write(codec, HPRCOM_CTRL, reg | UNMUTE);

	/* ADC default volume and unmute */
	snd_soc_write(codec, LADC_VOL, DEFAULT_GAIN);
	snd_soc_write(codec, RADC_VOL, DEFAULT_GAIN);
	/* By default route Line1 to ADC PGA mixer */
	snd_soc_write(codec, LINE1L_2_LADC_CTRL, 0x0);
	snd_soc_write(codec, LINE1R_2_RADC_CTRL, 0x0);

	/* PGA to HP Bypass default volume, disconnect from Output Mixer */
	snd_soc_write(codec, PGAL_2_HPLOUT_VOL, DEFAULT_VOL);
	snd_soc_write(codec, PGAR_2_HPROUT_VOL, DEFAULT_VOL);
	snd_soc_write(codec, PGAL_2_HPLCOM_VOL, DEFAULT_VOL);
	snd_soc_write(codec, PGAR_2_HPRCOM_VOL, DEFAULT_VOL);
	/* PGA to Line Out default volume, disconnect from Output Mixer */
	snd_soc_write(codec, PGAL_2_LLOPM_VOL, DEFAULT_VOL);
	snd_soc_write(codec, PGAR_2_RLOPM_VOL, DEFAULT_VOL);
	/* PGA to Mono Line Out default volume, disconnect from Output Mixer */
	snd_soc_write(codec, PGAL_2_MONOLOPM_VOL, DEFAULT_VOL);
	snd_soc_write(codec, PGAR_2_MONOLOPM_VOL, DEFAULT_VOL);

	/* Line2 to HP Bypass default volume, disconnect from Output Mixer */
	snd_soc_write(codec, LINE2L_2_HPLOUT_VOL, DEFAULT_VOL);
	snd_soc_write(codec, LINE2R_2_HPROUT_VOL, DEFAULT_VOL);
	snd_soc_write(codec, LINE2L_2_HPLCOM_VOL, DEFAULT_VOL);
	snd_soc_write(codec, LINE2R_2_HPRCOM_VOL, DEFAULT_VOL);
	/* Line2 Line Out default volume, disconnect from Output Mixer */
	snd_soc_write(codec, LINE2L_2_LLOPM_VOL, DEFAULT_VOL);
	snd_soc_write(codec, LINE2R_2_RLOPM_VOL, DEFAULT_VOL);
	/* Line2 to Mono Out default volume, disconnect from Output Mixer */
	snd_soc_write(codec, LINE2L_2_MONOLOPM_VOL, DEFAULT_VOL);
	snd_soc_write(codec, LINE2R_2_MONOLOPM_VOL, DEFAULT_VOL);

	if (aic3x->model == AIC3X_MODEL_3007) {
		aic3x_init_3007(codec);
		snd_soc_write(codec, CLASSD_CTRL, 0);
	}

	return 0;
}

static int aic3x_probe(struct snd_soc_codec *codec)
{
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);
	int ret, i;

	codec->control_data = aic3x->control_data;
	aic3x->codec = codec;
	codec->idle_bias_off = 1;

	ret = snd_soc_codec_set_cache_io(codec, 8, 8, aic3x->control_type);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	if (aic3x->gpio_reset >= 0) {
		ret = gpio_request(aic3x->gpio_reset, "tlv320aic3x reset");
		if (ret != 0)
			goto err_gpio;
		gpio_direction_output(aic3x->gpio_reset, 0);
	}

	for (i = 0; i < ARRAY_SIZE(aic3x->supplies); i++)
		aic3x->supplies[i].supply = aic3x_supply_names[i];

	ret = regulator_bulk_get(codec->dev, ARRAY_SIZE(aic3x->supplies),
				 aic3x->supplies);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to request supplies: %d\n", ret);
		goto err_get;
	}
	for (i = 0; i < ARRAY_SIZE(aic3x->supplies); i++) {
		aic3x->disable_nb[i].nb.notifier_call = aic3x_regulator_event;
		aic3x->disable_nb[i].aic3x = aic3x;
		ret = regulator_register_notifier(aic3x->supplies[i].consumer,
						  &aic3x->disable_nb[i].nb);
		if (ret) {
			dev_err(codec->dev,
				"Failed to request regulator notifier: %d\n",
				 ret);
			goto err_notif;
		}
	}

	codec->cache_only = 1;
	aic3x_init(codec);

	if (aic3x->setup) {
		/* setup GPIO functions */
		snd_soc_write(codec, AIC3X_GPIO1_REG,
			      (aic3x->setup->gpio_func[0] & 0xf) << 4);
		snd_soc_write(codec, AIC3X_GPIO2_REG,
			      (aic3x->setup->gpio_func[1] & 0xf) << 4);
	}

	snd_soc_add_controls(codec, aic3x_snd_controls,
			     ARRAY_SIZE(aic3x_snd_controls));
	if (aic3x->model == AIC3X_MODEL_3007)
		snd_soc_add_controls(codec, &aic3x_classd_amp_gain_ctrl, 1);

	aic3x_add_widgets(codec);

	return 0;

err_notif:
	while (i--)
		regulator_unregister_notifier(aic3x->supplies[i].consumer,
					      &aic3x->disable_nb[i].nb);
	regulator_bulk_free(ARRAY_SIZE(aic3x->supplies), aic3x->supplies);
err_get:
	if (aic3x->gpio_reset >= 0)
		gpio_free(aic3x->gpio_reset);
err_gpio:
	kfree(aic3x);
	return ret;
}

static int aic3x_remove(struct snd_soc_codec *codec)
{
	struct aic3x_priv *aic3x = snd_soc_codec_get_drvdata(codec);
	int i;

	aic3x_set_bias_level(codec, SND_SOC_BIAS_OFF);
	if (aic3x->gpio_reset >= 0) {
		gpio_set_value(aic3x->gpio_reset, 0);
		gpio_free(aic3x->gpio_reset);
	}
	for (i = 0; i < ARRAY_SIZE(aic3x->supplies); i++)
		regulator_unregister_notifier(aic3x->supplies[i].consumer,
					      &aic3x->disable_nb[i].nb);
	regulator_bulk_free(ARRAY_SIZE(aic3x->supplies), aic3x->supplies);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_aic3x = {
	.set_bias_level = aic3x_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(aic3x_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = aic3x_reg,
	.probe = aic3x_probe,
	.remove = aic3x_remove,
	.suspend = aic3x_suspend,
	.resume = aic3x_resume,
};

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
/*
 * AIC3X 2 wire address can be up to 4 devices with device addresses
 * 0x18, 0x19, 0x1A, 0x1B
 */

static const struct i2c_device_id aic3x_i2c_id[] = {
	[AIC3X_MODEL_3X] = { "tlv320aic3x", 0 },
	[AIC3X_MODEL_33] = { "tlv320aic33", 0 },
	[AIC3X_MODEL_3007] = { "tlv320aic3007", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, aic3x_i2c_id);

/*
 * If the i2c layer weren't so broken, we could pass this kind of data
 * around
 */
static int aic3x_i2c_probe(struct i2c_client *i2c,
			   const struct i2c_device_id *id)
{
	struct aic3x_pdata *pdata = i2c->dev.platform_data;
	struct aic3x_priv *aic3x;
	int ret;
	const struct i2c_device_id *tbl;

	aic3x = kzalloc(sizeof(struct aic3x_priv), GFP_KERNEL);
	if (aic3x == NULL) {
		dev_err(&i2c->dev, "failed to create private data\n");
		return -ENOMEM;
	}

	aic3x->control_data = i2c;
	aic3x->control_type = SND_SOC_I2C;

	i2c_set_clientdata(i2c, aic3x);
	if (pdata) {
		aic3x->gpio_reset = pdata->gpio_reset;
		aic3x->setup = pdata->setup;
	} else {
		aic3x->gpio_reset = -1;
	}

	for (tbl = aic3x_i2c_id; tbl->name[0]; tbl++) {
		if (!strcmp(tbl->name, id->name))
			break;
	}
	aic3x->model = tbl - aic3x_i2c_id;

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_aic3x, &aic3x_dai, 1);
	if (ret < 0)
		kfree(aic3x);
	return ret;
}

static int aic3x_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

/* machine i2c codec control layer */
static struct i2c_driver aic3x_i2c_driver = {
	.driver = {
		.name = "tlv320aic3x-codec",
		.owner = THIS_MODULE,
	},
	.probe	= aic3x_i2c_probe,
	.remove = aic3x_i2c_remove,
	.id_table = aic3x_i2c_id,
};

static inline void aic3x_i2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&aic3x_i2c_driver);
	if (ret)
		printk(KERN_ERR "%s: error regsitering i2c driver, %d\n",
		       __func__, ret);
}

static inline void aic3x_i2c_exit(void)
{
	i2c_del_driver(&aic3x_i2c_driver);
}
#endif

static int __init aic3x_modinit(void)
{
	int ret = 0;
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	ret = i2c_add_driver(&aic3x_i2c_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register TLV320AIC3x I2C driver: %d\n",
		       ret);
	}
#endif
	return ret;
}
module_init(aic3x_modinit);

static void __exit aic3x_exit(void)
{
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_del_driver(&aic3x_i2c_driver);
#endif
}
module_exit(aic3x_exit);

MODULE_DESCRIPTION("ASoC TLV320AIC3X codec driver");
MODULE_AUTHOR("Vladimir Barinov");
MODULE_LICENSE("GPL");
