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

  function: LPC low level routines
  last mod: $Id: lpc.h,v 1.20 2003/03/07 09:13:30 xiphmont Exp $

 ********************************************************************/

#ifndef _V_LPC_H_
#define _V_LPC_H_

#include "../vorbis/codec.h"

/* simple linear scale LPC code */
extern float vorbis_lpc_from_data(float *data,float *lpc,int n,int m);

extern void vorbis_lpc_predict(float *coeff,float *prime,int m,
			       float *data,long n);

#endif
