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

 function: catch-all toplevel settings for q modes only
 last mod: $Id: setup_X.h,v 1.3 2002/07/11 06:41:05 xiphmont Exp $

 ********************************************************************/

static double rate_mapping_X[11]={
  -1.,-1.,-1.,-1.,-1.,
  -1.,-1.,-1.,-1.,-1.,-1.
};

ve_setup_data_template ve_setup_X_stereo={
  10,
  rate_mapping_X,
  quality_mapping_44,
  2,
  50000,
  200000,
  
  blocksize_short_44,
  blocksize_long_44,

  _psy_tone_masteratt_44,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_otherblock,
  _vp_tonemask_adj_longblock,
  _vp_tonemask_adj_otherblock,

  _psy_noiseguards_44,
  _psy_noisebias_impulse,
  _psy_noisebias_padding,
  _psy_noisebias_trans,
  _psy_noisebias_long,
  _psy_noise_suppress,
  
  _psy_compand_44,
  _psy_compand_short_mapping,
  _psy_compand_long_mapping,

  {_noise_start_short_44,_noise_start_long_44},
  {_noise_part_short_44,_noise_part_long_44},
  _noise_thresh_44,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_44,

  _psy_global_44,
  _global_mapping_44,
  _psy_stereo_modes_44,

  _floor_books,
  _floor,
  _floor_short_mapping_44,
  _floor_long_mapping_44,

  _mapres_template_44_stereo
};

ve_setup_data_template ve_setup_X_uncoupled={
  10,
  rate_mapping_X,
  quality_mapping_44,
  -1,
  50000,
  200000,
  
  blocksize_short_44,
  blocksize_long_44,

  _psy_tone_masteratt_44,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_otherblock,
  _vp_tonemask_adj_longblock,
  _vp_tonemask_adj_otherblock,

  _psy_noiseguards_44,
  _psy_noisebias_impulse,
  _psy_noisebias_padding,
  _psy_noisebias_trans,
  _psy_noisebias_long,
  _psy_noise_suppress,
  
  _psy_compand_44,
  _psy_compand_short_mapping,
  _psy_compand_long_mapping,

  {_noise_start_short_44,_noise_start_long_44},
  {_noise_part_short_44,_noise_part_long_44},
  _noise_thresh_44_2,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_44,

  _psy_global_44,
  _global_mapping_44,
  NULL,

  _floor_books,
  _floor,
  _floor_short_mapping_44,
  _floor_long_mapping_44,

  _mapres_template_44_uncoupled
};

ve_setup_data_template ve_setup_X_stereo_low={
  1,
  rate_mapping_X,
  quality_mapping_44_stereo_low,
  2,
  50000,
  200000,
  
  blocksize_short_44_low,
  blocksize_long_44_low,

  _psy_tone_masteratt_44_low,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_otherblock,
  _vp_tonemask_adj_longblock,
  _vp_tonemask_adj_otherblock,

  _psy_noiseguards_44,
  _psy_noisebias_trans_low,
  _psy_noisebias_trans_low,
  _psy_noisebias_trans_low,
  _psy_noisebias_long_low,
  _psy_noise_suppress,
  
  _psy_compand_44,
  _psy_compand_short_mapping,
  _psy_compand_long_mapping,

  {_noise_start_short_44_low,_noise_start_long_44_low},
  {_noise_part_short_44,_noise_part_long_44},
  _noise_thresh_44,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_44_low,

  _psy_global_44,
  _global_mapping_44,
  _psy_stereo_modes_44_low,

  _floor_books,
  _floor,
  _floor_short_mapping_44_low,
  _floor_long_mapping_44_low,

  _mapres_template_44_stereo
};


ve_setup_data_template ve_setup_X_uncoupled_low={
  1,
  rate_mapping_X,
  quality_mapping_44_stereo_low,
  -1,
  50000,
  200000,
  
  blocksize_short_44_low,
  blocksize_long_44_low,

  _psy_tone_masteratt_44_low,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_otherblock,
  _vp_tonemask_adj_longblock,
  _vp_tonemask_adj_otherblock,

  _psy_noiseguards_44,
  _psy_noisebias_trans_low,
  _psy_noisebias_trans_low,
  _psy_noisebias_trans_low,
  _psy_noisebias_long_low,
  _psy_noise_suppress,
  
  _psy_compand_44,
  _psy_compand_short_mapping,
  _psy_compand_long_mapping,

  {_noise_start_short_44_low,_noise_start_long_44_low},
  {_noise_part_short_44,_noise_part_long_44},
  _noise_thresh_44_2,

  _psy_ath_floater,
  _psy_ath_abs,
  
  _psy_lowpass_44_low,

  _psy_global_44,
  _global_mapping_44,
  NULL,

  _floor_books,
  _floor,
  _floor_short_mapping_44_low,
  _floor_long_mapping_44_low,

  _mapres_template_44_uncoupled
};

ve_setup_data_template ve_setup_XX_stereo={
  2,
  rate_mapping_X,
  quality_mapping_8,
  2,
  0,
  8000,
  
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

ve_setup_data_template ve_setup_XX_uncoupled={
  2,
  rate_mapping_X,
  quality_mapping_8,
  -1,
  0,
  8000,
  
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

