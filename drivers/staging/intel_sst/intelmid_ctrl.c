/*
 *  intelmid_ctrl.c - Intel Sound card driver for MID
 *
 *  Copyright (C) 2008-10 Intel Corp
 *  Authors:	Harsha Priya <priya.harsha@intel.com>
 *		Vinod Koul <vinod.koul@intel.com>
 *		Dharageswari R <dharageswari.r@intel.com>
 *		KP Jeeja <jeeja.kp@intel.com>
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  ALSA driver handling mixer controls for Intel MAD chipset
 */
#include <sound/core.h>
#include <sound/control.h>
#include "jack.h"
#include "intel_sst.h"
#include "intel_sst_ioctl.h"
#include "intelmid_snd_control.h"
#include "intelmid.h"

static char *out_names_mrst[] = {"Headphones",
				"Internal speakers"};
static char *in_names_mrst[] = {"AMIC",
				"DMIC",
				"HS_MIC"};
static char *out_names_mfld[] = {"Headset ",
				"EarPiece  "};
static char *in_names_mfld[] = {"AMIC",
				"DMIC"};

struct snd_control_val intelmad_ctrl_val[MAX_VENDORS] = {
	{
		.playback_vol_max = 63,
		.playback_vol_min = 0,
		.capture_vol_max = 63,
		.capture_vol_min = 0,
	},
	{
		.playback_vol_max = 0,
		.playback_vol_min = -31,
		.capture_vol_max = 0,
		.capture_vol_min = -20,
	},
	{
		.playback_vol_max = 0,
		.playback_vol_min = -126,
		.capture_vol_max = 0,
		.capture_vol_min = -31,
	},
};

/* control path functionalities */

static inline int snd_intelmad_volume_info(struct snd_ctl_elem_info *uinfo,
					int control_type, int max, int min)
{
	WARN_ON(!uinfo);

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = control_type;
	uinfo->value.integer.min = min;
	uinfo->value.integer.max = max;
	return 0;
}

/**
* snd_intelmad_mute_info - provides information about the mute controls
*
* @kcontrol:	pointer to the control
* @uinfo:	pointer to the structure where the control's info need
*		to be filled
*
* This function is called when a mixer application requests for control's info
*/
static int snd_intelmad_mute_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	WARN_ON(!uinfo);
	WARN_ON(!kcontrol);

	/* set up the mute as a boolean mono control with min-max values */
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = MONO_CNTL;
	uinfo->value.integer.min = MIN_MUTE;
	uinfo->value.integer.max = MAX_MUTE;
	return 0;
}

/**
* snd_intelmad_capture_volume_info - provides info about the volume control
*
* @kcontrol:	pointer to the control
* @uinfo:	pointer to the structure where the control's info need
*		to be filled
*
* This function is called when a mixer application requests for control's info
*/
static int snd_intelmad_capture_volume_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	snd_intelmad_volume_info(uinfo, MONO_CNTL,
		intelmad_ctrl_val[sst_card_vendor_id].capture_vol_max,
		intelmad_ctrl_val[sst_card_vendor_id].capture_vol_min);
	return 0;
}

/**
* snd_intelmad_playback_volume_info - provides info about the volume control
*
* @kcontrol:	pointer to the control
* @uinfo:	pointer to the structure where the control's info need
*		to be filled
*
* This function is called when a mixer application requests for control's info
*/
static int snd_intelmad_playback_volume_info(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	snd_intelmad_volume_info(uinfo, STEREO_CNTL,
		intelmad_ctrl_val[sst_card_vendor_id].playback_vol_max,
		intelmad_ctrl_val[sst_card_vendor_id].playback_vol_min);
	return 0;
}

/**
* snd_intelmad_device_info_mrst - provides information about the devices available
*
* @kcontrol:	pointer to the control
* @uinfo:	pointer to the structure where the devices's info need
*		to be filled
*
* This function is called when a mixer application requests for device's info
*/
static int snd_intelmad_device_info_mrst(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{

	WARN_ON(!kcontrol);
	WARN_ON(!uinfo);

	/* setup device select as drop down controls with different values */
	if (kcontrol->id.numid == OUTPUT_SEL)
		uinfo->value.enumerated.items = ARRAY_SIZE(out_names_mrst);
	else
		uinfo->value.enumerated.items = ARRAY_SIZE(in_names_mrst);
	uinfo->count = MONO_CNTL;
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item = 1;
	if (kcontrol->id.numid == OUTPUT_SEL)
		strncpy(uinfo->value.enumerated.name,
			out_names_mrst[uinfo->value.enumerated.item],
			sizeof(uinfo->value.enumerated.name)-1);
	else
		strncpy(uinfo->value.enumerated.name,
			in_names_mrst[uinfo->value.enumerated.item],
			sizeof(uinfo->value.enumerated.name)-1);
	return 0;
}

static int snd_intelmad_device_info_mfld(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_info *uinfo)
{
	WARN_ON(!kcontrol);
	WARN_ON(!uinfo);
	/* setup device select as drop down controls with different values */
	if (kcontrol->id.numid == OUTPUT_SEL)
		uinfo->value.enumerated.items = ARRAY_SIZE(out_names_mfld);
	else
		uinfo->value.enumerated.items = ARRAY_SIZE(in_names_mfld);
	uinfo->count = MONO_CNTL;
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item = 1;
	if (kcontrol->id.numid == OUTPUT_SEL)
		strncpy(uinfo->value.enumerated.name,
			out_names_mfld[uinfo->value.enumerated.item],
			sizeof(uinfo->value.enumerated.name)-1);
	else
		strncpy(uinfo->value.enumerated.name,
			in_names_mfld[uinfo->value.enumerated.item],
			sizeof(uinfo->value.enumerated.name)-1);
	return 0;
}

/**
* snd_intelmad_volume_get - gets the current volume for the control
*
* @kcontrol:	pointer to the control
* @uval:	pointer to the structure where the control's info need
*		to be filled
*
* This function is called when .get function of a control is invoked from app
*/
static int snd_intelmad_volume_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *uval)
{
	int ret_val = 0, cntl_list[2] = {0,};
	int value = 0;
	struct snd_intelmad *intelmaddata;
	struct snd_pmic_ops *scard_ops;

	pr_debug("sst: snd_intelmad_volume_get called\n");

	WARN_ON(!uval);
	WARN_ON(!kcontrol);

	intelmaddata = kcontrol->private_data;

	WARN_ON(!intelmaddata->sstdrv_ops);

	scard_ops = intelmaddata->sstdrv_ops->scard_ops;

	WARN_ON(!scard_ops);

	switch (kcontrol->id.numid) {
	case PLAYBACK_VOL:
		cntl_list[0] = PMIC_SND_RIGHT_PB_VOL;
		cntl_list[1] = PMIC_SND_LEFT_PB_VOL;
		break;

	case CAPTURE_VOL:
		cntl_list[0] = PMIC_SND_CAPTURE_VOL;
		break;
	default:
		return -EINVAL;
	}

	ret_val = scard_ops->get_vol(cntl_list[0], &value);
	uval->value.integer.value[0] = value;

	if (ret_val)
		return ret_val;

	if (kcontrol->id.numid == PLAYBACK_VOL) {
		ret_val = scard_ops->get_vol(cntl_list[1], &value);
		uval->value.integer.value[1] = value;
	}
	return ret_val;
}

/**
* snd_intelmad_mute_get - gets the current mute status for the control
*
* @kcontrol:	pointer to the control
* @uval:	pointer to the structure where the control's info need
*		to be filled
*
* This function is called when .get function of a control is invoked from app
*/
static int snd_intelmad_mute_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *uval)
{

	int cntl_list = 0, ret_val = 0;
	u8 value = 0;
	struct snd_intelmad *intelmaddata;
	struct snd_pmic_ops *scard_ops;

	pr_debug("sst: Mute_get called\n");

	WARN_ON(!uval);
	WARN_ON(!kcontrol);

	intelmaddata = kcontrol->private_data;

	WARN_ON(!intelmaddata->sstdrv_ops);

	scard_ops = intelmaddata->sstdrv_ops->scard_ops;

	WARN_ON(!scard_ops);

	switch (kcontrol->id.numid) {
	case PLAYBACK_MUTE:
		if (intelmaddata->output_sel == STEREO_HEADPHONE)
			cntl_list = PMIC_SND_LEFT_HP_MUTE;
		else if ((intelmaddata->output_sel == INTERNAL_SPKR) ||
			(intelmaddata->output_sel == MONO_EARPIECE))
			cntl_list = PMIC_SND_LEFT_SPEAKER_MUTE;
		break;

	case CAPTURE_MUTE:
		if (intelmaddata->input_sel == DMIC)
			cntl_list = PMIC_SND_DMIC_MUTE;
		else if (intelmaddata->input_sel == AMIC)
			cntl_list = PMIC_SND_AMIC_MUTE;
		else if (intelmaddata->input_sel == HS_MIC)
			cntl_list = PMIC_SND_HP_MIC_MUTE;
		break;
	case MASTER_MUTE:
		uval->value.integer.value[0] = intelmaddata->master_mute;
		return 0;
	default:
		return -EINVAL;
	}

	ret_val = scard_ops->get_mute(cntl_list, &value);
	uval->value.integer.value[0] = value;
	return ret_val;
}

/**
* snd_intelmad_volume_set - sets the volume control's info
*
* @kcontrol:	pointer to the control
* @uval:	pointer to the structure where the control's info is
*		available to be set
*
* This function is called when .set function of a control is invoked from app
*/
static int snd_intelmad_volume_set(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *uval)
{

	int ret_val, cntl_list[2] = {0,};
	struct snd_intelmad *intelmaddata;
	struct snd_pmic_ops *scard_ops;

	pr_debug("sst: volume set called:%ld %ld\n",
			uval->value.integer.value[0],
			uval->value.integer.value[1]);

	WARN_ON(!uval);
	WARN_ON(!kcontrol);

	intelmaddata = kcontrol->private_data;

	WARN_ON(!intelmaddata->sstdrv_ops);

	scard_ops = intelmaddata->sstdrv_ops->scard_ops;

	WARN_ON(!scard_ops);

	switch (kcontrol->id.numid) {
	case PLAYBACK_VOL:
		cntl_list[0] = PMIC_SND_LEFT_PB_VOL;
		cntl_list[1] = PMIC_SND_RIGHT_PB_VOL;
		break;

	case CAPTURE_VOL:
		cntl_list[0] = PMIC_SND_CAPTURE_VOL;
		break;
	default:
		return -EINVAL;
	}

	ret_val = scard_ops->set_vol(cntl_list[0],
				uval->value.integer.value[0]);
	if (ret_val)
		return ret_val;

	if (kcontrol->id.numid == PLAYBACK_VOL)
		ret_val = scard_ops->set_vol(cntl_list[1],
				uval->value.integer.value[1]);
	return ret_val;
}

/**
* snd_intelmad_mute_set - sets the mute control's info
*
* @kcontrol:	pointer to the control
* @uval:	pointer to the structure where the control's info is
*		available to be set
*
* This function is called when .set function of a control is invoked from app
*/
static int snd_intelmad_mute_set(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *uval)
{
	int cntl_list[2] = {0,}, ret_val;
	struct snd_intelmad *intelmaddata;
	struct snd_pmic_ops *scard_ops;

	pr_debug("sst: snd_intelmad_mute_set called\n");

	WARN_ON(!uval);
	WARN_ON(!kcontrol);

	intelmaddata = kcontrol->private_data;

	WARN_ON(!intelmaddata->sstdrv_ops);

	scard_ops = intelmaddata->sstdrv_ops->scard_ops;

	WARN_ON(!scard_ops);

	kcontrol->private_value = uval->value.integer.value[0];

	switch (kcontrol->id.numid) {
	case PLAYBACK_MUTE:
		if (intelmaddata->output_sel == STEREO_HEADPHONE) {
			cntl_list[0] = PMIC_SND_LEFT_HP_MUTE;
			cntl_list[1] = PMIC_SND_RIGHT_HP_MUTE;
		} else if ((intelmaddata->output_sel == INTERNAL_SPKR) ||
				(intelmaddata->output_sel == MONO_EARPIECE)) {
			cntl_list[0] = PMIC_SND_LEFT_SPEAKER_MUTE;
			cntl_list[1] = PMIC_SND_RIGHT_SPEAKER_MUTE;
		}
		break;

	case CAPTURE_MUTE:/*based on sel device mute the i/p dev*/
		if (intelmaddata->input_sel == DMIC)
			cntl_list[0] = PMIC_SND_DMIC_MUTE;
		else if (intelmaddata->input_sel == AMIC)
			cntl_list[0] = PMIC_SND_AMIC_MUTE;
		else if (intelmaddata->input_sel == HS_MIC)
			cntl_list[0] = PMIC_SND_HP_MIC_MUTE;
		break;
	case MASTER_MUTE:
		cntl_list[0] = PMIC_SND_MUTE_ALL;
		intelmaddata->master_mute = uval->value.integer.value[0];
		break;
	default:
		return -EINVAL;
	}

	ret_val = scard_ops->set_mute(cntl_list[0],
				uval->value.integer.value[0]);
	if (ret_val)
		return ret_val;

	if (kcontrol->id.numid == PLAYBACK_MUTE)
		ret_val = scard_ops->set_mute(cntl_list[1],
					uval->value.integer.value[0]);
	return ret_val;
}

/**
* snd_intelmad_device_get - get the device select control's info
*
* @kcontrol:	pointer to the control
* @uval:	pointer to the structure where the control's info is
*		to be filled
*
* This function is called when .get function of a control is invoked from app
*/
static int snd_intelmad_device_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *uval)
{
	struct snd_intelmad *intelmaddata;
	struct snd_pmic_ops *scard_ops;
	pr_debug("sst: device_get called\n");

	WARN_ON(!uval);
	WARN_ON(!kcontrol);

	intelmaddata = kcontrol->private_data;
	if (intelmaddata->cpu_id == CPU_CHIP_PENWELL) {
		scard_ops = intelmaddata->sstdrv_ops->scard_ops;
		if (kcontrol->id.numid == OUTPUT_SEL)
			uval->value.enumerated.item[0] =
					scard_ops->output_dev_id;
		else if (kcontrol->id.numid == INPUT_SEL)
			uval->value.enumerated.item[0] =
					scard_ops->input_dev_id;
		else
			return -EINVAL;
	} else
	uval->value.enumerated.item[0] = kcontrol->private_value;
	return 0;
}

/**
* snd_intelmad_device_set - set the device select control's info
*
* @kcontrol:	pointer to the control
* @uval:	pointer to the structure where the control's info is
*		available to be set
*
* This function is called when .set function of a control is invoked from app
*/
static int snd_intelmad_device_set(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *uval)
{
	struct snd_intelmad *intelmaddata;
	struct snd_pmic_ops *scard_ops;
	int ret_val = 0, vendor, status;

	pr_debug("sst: snd_intelmad_device_set called\n");

	WARN_ON(!uval);
	WARN_ON(!kcontrol);
	status = -1;

	intelmaddata = kcontrol->private_data;

	WARN_ON(!intelmaddata->sstdrv_ops);

	scard_ops = intelmaddata->sstdrv_ops->scard_ops;

	WARN_ON(!scard_ops);

	/* store value with driver */
	kcontrol->private_value = uval->value.enumerated.item[0];

	switch (kcontrol->id.numid) {
	case OUTPUT_SEL:
		ret_val = scard_ops->set_output_dev(
				uval->value.enumerated.item[0]);
		intelmaddata->output_sel = uval->value.enumerated.item[0];
		break;
	case INPUT_SEL:
		vendor = intelmaddata->sstdrv_ops->vendor_id;
		if ((vendor == SND_MX) || (vendor == SND_FS)) {
			if (uval->value.enumerated.item[0] == HS_MIC) {
				status = 1;
				intelmaddata->sstdrv_ops->
				control_set(SST_ENABLE_RX_TIME_SLOT, &status);
			} else {
				status = 0;
				intelmaddata->sstdrv_ops->
				control_set(SST_ENABLE_RX_TIME_SLOT, &status);
			}
		}
		ret_val = scard_ops->set_input_dev(
				uval->value.enumerated.item[0]);
		intelmaddata->input_sel = uval->value.enumerated.item[0];
		break;
	default:
		return -EINVAL;
	}
	kcontrol->private_value = uval->value.enumerated.item[0];
	return ret_val;
}

struct snd_kcontrol_new snd_intelmad_controls_mrst[MAX_CTRL] __devinitdata = {
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"PCM Playback Source",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_device_info_mrst,
	.get		=	snd_intelmad_device_get,
	.put		=	snd_intelmad_device_set,
	.private_value	=	0,
},
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"PCM Capture Source",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_device_info_mrst,
	.get		=	snd_intelmad_device_get,
	.put		=	snd_intelmad_device_set,
	.private_value	=	0,
},
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"PCM Playback Volume",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_playback_volume_info,
	.get		=	snd_intelmad_volume_get,
	.put		=	snd_intelmad_volume_set,
	.private_value	=	0,
},
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"PCM Playback Switch",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_mute_info,
	.get		=	snd_intelmad_mute_get,
	.put		=	snd_intelmad_mute_set,
	.private_value	=	0,
},
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"PCM Capture Volume",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_capture_volume_info,
	.get		=	snd_intelmad_volume_get,
	.put		=	snd_intelmad_volume_set,
	.private_value	=	0,
},
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"PCM Capture Switch",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_mute_info,
	.get		=	snd_intelmad_mute_get,
	.put		=	snd_intelmad_mute_set,
	.private_value	=	0,
},
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"Master Playback Switch",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_mute_info,
	.get		=	snd_intelmad_mute_get,
	.put		=	snd_intelmad_mute_set,
	.private_value	=	0,
},
};

struct snd_kcontrol_new
snd_intelmad_controls_mfld[MAX_CTRL_MFLD] __devinitdata = {
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"PCM Playback Source",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_device_info_mfld,
	.get		=	snd_intelmad_device_get,
	.put		=	snd_intelmad_device_set,
	.private_value	=	0,
},
{
	.iface		=	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name		=	"PCM Capture Source",
	.access		=	SNDRV_CTL_ELEM_ACCESS_READWRITE,
	.info		=	snd_intelmad_device_info_mfld,
	.get		=	snd_intelmad_device_get,
	.put		=	snd_intelmad_device_set,
	.private_value	=	0,
},
};

