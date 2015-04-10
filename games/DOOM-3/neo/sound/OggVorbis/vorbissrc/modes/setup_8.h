/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: 8kHz settings 
 last mod: $Id: setup_8.h,v 1.2 2002/07/11 06:41:05 xiphmont Exp $

 ********************************************************************/

#include "psych_8.h"
#include "residue_8.h"

static int blocksize_8[2]={
  512,512
};

static int _floor_mapping_8[2]={
  1,1,
};

static double rate_mapping_8[3]={
  6000.,9000.,32000.,
};

static double rate_mapping_8_uncoupled[3]={
  8000.,14000.,42000.,
};

static double quality_mapping_8[3]={
  -.1,.0,1.
};

static double _psy_compand_8_mapping[3]={ 0., 1., 1.};

static double _global_mapping_8[3]={ 1., 2., 3. };

ve_setup_data_template ve_setup_8_stereo={
  2,
  rate_mapping_8,
  quality_mapping_8,
  2,
  8000,
  9000,
  
  blocksize_8,
  blocksize_8,

  _psy_tone_masteratt_8,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_8,
  NULL,
  _vp_tonemask_adj_8,

  _psy_noiseguards_8,
  _psy_noisebias_8,
  _psy_noisebias_8,
  NULL,
  NULL,
  _psy_noise_suppress,
  
  _psy_compand_8,
  _psy_compand_8_mapping,
  NULL,

  {_noise_start_8,_noise_start_8},
  {_noise_part_8,_noise_part_8},
  _noise_thresh_44_2,

  _psy_ath_floater_8,
  _psy_ath_abs_8,
  
  _psy_lowpass_8,

  _psy_global_44,
  _global_mapping_8,
  _psy_stereo_modes_8,

  _floor_books,
  _floor,
  _floor_mapping_8,
  NULL,

  _mapres_template_8_stereo
};

ve_setup_data_template ve_setup_8_uncoupled={
  2,
  rate_mapping_8_uncoupled,
  quality_mapping_8,
  -1,
  8000,
  9000,
  
  blocksize_8,
  blocksize_8,

  _psy_tone_masteratt_8,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_8,
  NULL,
  _vp_tonemask_adj_8,

  _psy_noiseguards_8,
  _psy_noisebias_8,
  _psy_noisebias_8,
  NULL,
  NULL,
  _psy_noise_suppress,
  
  _psy_compand_8,
  _psy_compand_8_mapping,
  NULL,

  {_noise_start_8,_noise_start_8},
  {_noise_part_8,_noise_part_8},
  _noise_thresh_44_2,

  _psy_ath_floater_8,
  _psy_ath_abs_8,
  
  _psy_lowpass_8,

  _psy_global_44,
  _global_mapping_8,
  _psy_stereo_modes_8,

  _floor_books,
  _floor,
  _floor_mapping_8,
  NULL,

  _mapres_template_8_uncoupled
};

