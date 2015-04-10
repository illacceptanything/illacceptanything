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

 function: toplevel residue templates for 32/44.1/48kHz
 last mod: $Id: residue_44.h,v 1.16 2002/07/11 06:41:04 xiphmont Exp $

 ********************************************************************/

#include "../vorbis/codec.h"
#include "backends.h"
#include "books/coupled/res_books_stereo.h"

/***** residue backends *********************************************/

static vorbis_info_residue0 _residue_44_mid={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8  */
  {0},
  {-1},
  {  .5,  1.5,  1.5,  2.5,  2.5,  4.5,  8.5,  16.5, 32.5},
  {  .5,   .5, 999.,   .5,  999., 4.5,  8.5,  16.5, 32.5},
};

static vorbis_info_residue0 _residue_44_high={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8  */
  {0},
  {-1},
  {  .5,  1.5,  2.5,  4.5,  8.5, 16.5, 32.5, 71.5,157.5},
  {  .5,  1.5,  2.5,  3.5,  4.5,  8.5, 16.5, 71.5,157.5},
};

static static_bookblock _resbook_44s_0={
  {
    {0},{0,0,&_44c0_s_p1_0},{0,0,&_44c0_s_p2_0},{0,0,&_44c0_s_p3_0},
    {0,0,&_44c0_s_p4_0},{0,0,&_44c0_s_p5_0},{0,0,&_44c0_s_p6_0},
    {&_44c0_s_p7_0,&_44c0_s_p7_1},{&_44c0_s_p8_0,&_44c0_s_p8_1},
    {&_44c0_s_p9_0,&_44c0_s_p9_1,&_44c0_s_p9_2}
   }
};
static static_bookblock _resbook_44sm_0={
  {
    {0},{0,0,&_44c0_sm_p1_0},{0,0,&_44c0_sm_p2_0},{0,0,&_44c0_sm_p3_0},
    {0,0,&_44c0_sm_p4_0},{0,0,&_44c0_sm_p5_0},{0,0,&_44c0_sm_p6_0},
    {&_44c0_sm_p7_0,&_44c0_sm_p7_1},{&_44c0_sm_p8_0,&_44c0_sm_p8_1},
    {&_44c0_sm_p9_0,&_44c0_sm_p9_1,&_44c0_sm_p9_2}
   }
};
static static_bookblock _resbook_44s_1={
  {
    {0},{0,0,&_44c1_s_p1_0},{0,0,&_44c1_s_p2_0},{0,0,&_44c1_s_p3_0},
    {0,0,&_44c1_s_p4_0},{0,0,&_44c1_s_p5_0},{0,0,&_44c1_s_p6_0},
    {&_44c1_s_p7_0,&_44c1_s_p7_1},{&_44c1_s_p8_0,&_44c1_s_p8_1},
    {&_44c1_s_p9_0,&_44c1_s_p9_1,&_44c1_s_p9_2}
   }
};
static static_bookblock _resbook_44sm_1={
  {
    {0},{0,0,&_44c1_sm_p1_0},{0,0,&_44c1_sm_p2_0},{0,0,&_44c1_sm_p3_0},
    {0,0,&_44c1_sm_p4_0},{0,0,&_44c1_sm_p5_0},{0,0,&_44c1_sm_p6_0},
    {&_44c1_sm_p7_0,&_44c1_sm_p7_1},{&_44c1_sm_p8_0,&_44c1_sm_p8_1},
    {&_44c1_sm_p9_0,&_44c1_sm_p9_1,&_44c1_sm_p9_2}
   }
};
static static_bookblock _resbook_44s_2={
  {
    {0},{0,0,&_44c2_s_p1_0},{0,0,&_44c2_s_p2_0},{0,0,&_44c2_s_p3_0},
    {0,0,&_44c2_s_p4_0},{0,0,&_44c2_s_p5_0},{0,0,&_44c2_s_p6_0},
    {&_44c2_s_p7_0,&_44c2_s_p7_1},{&_44c2_s_p8_0,&_44c2_s_p8_1},
    {&_44c2_s_p9_0,&_44c2_s_p9_1,&_44c2_s_p9_2}
   }
};
static static_bookblock _resbook_44s_3={
  {
    {0},{0,0,&_44c3_s_p1_0},{0,0,&_44c3_s_p2_0},{0,0,&_44c3_s_p3_0},
    {0,0,&_44c3_s_p4_0},{0,0,&_44c3_s_p5_0},{0,0,&_44c3_s_p6_0},
    {&_44c3_s_p7_0,&_44c3_s_p7_1},{&_44c3_s_p8_0,&_44c3_s_p8_1},
    {&_44c3_s_p9_0,&_44c3_s_p9_1,&_44c3_s_p9_2}
   }
};
static static_bookblock _resbook_44s_4={
  {
    {0},{0,0,&_44c4_s_p1_0},{0,0,&_44c4_s_p2_0},{0,0,&_44c4_s_p3_0},
    {0,0,&_44c4_s_p4_0},{0,0,&_44c4_s_p5_0},{0,0,&_44c4_s_p6_0},
    {&_44c4_s_p7_0,&_44c4_s_p7_1},{&_44c4_s_p8_0,&_44c4_s_p8_1},
    {&_44c4_s_p9_0,&_44c4_s_p9_1,&_44c4_s_p9_2}
   }
};
static static_bookblock _resbook_44s_5={
  {
    {0},{0,0,&_44c5_s_p1_0},{0,0,&_44c5_s_p2_0},{0,0,&_44c5_s_p3_0},
    {0,0,&_44c5_s_p4_0},{0,0,&_44c5_s_p5_0},{0,0,&_44c5_s_p6_0},
    {&_44c5_s_p7_0,&_44c5_s_p7_1},{&_44c5_s_p8_0,&_44c5_s_p8_1},
    {&_44c5_s_p9_0,&_44c5_s_p9_1,&_44c5_s_p9_2}
   }
};
static static_bookblock _resbook_44s_6={
  {
    {0},{0,0,&_44c6_s_p1_0},{0,0,&_44c6_s_p2_0},{0,0,&_44c6_s_p3_0},
    {0,0,&_44c6_s_p4_0},
    {&_44c6_s_p5_0,&_44c6_s_p5_1},
    {&_44c6_s_p6_0,&_44c6_s_p6_1},
    {&_44c6_s_p7_0,&_44c6_s_p7_1},
    {&_44c6_s_p8_0,&_44c6_s_p8_1},
    {&_44c6_s_p9_0,&_44c6_s_p9_1,&_44c6_s_p9_2}
   }
};
static static_bookblock _resbook_44s_7={
  {
    {0},{0,0,&_44c7_s_p1_0},{0,0,&_44c7_s_p2_0},{0,0,&_44c7_s_p3_0},
    {0,0,&_44c7_s_p4_0},
    {&_44c7_s_p5_0,&_44c7_s_p5_1},
    {&_44c7_s_p6_0,&_44c7_s_p6_1},
    {&_44c7_s_p7_0,&_44c7_s_p7_1},
    {&_44c7_s_p8_0,&_44c7_s_p8_1},
    {&_44c7_s_p9_0,&_44c7_s_p9_1,&_44c7_s_p9_2}
   }
};
static static_bookblock _resbook_44s_8={
  {
    {0},{0,0,&_44c8_s_p1_0},{0,0,&_44c8_s_p2_0},{0,0,&_44c8_s_p3_0},
    {0,0,&_44c8_s_p4_0},
    {&_44c8_s_p5_0,&_44c8_s_p5_1},
    {&_44c8_s_p6_0,&_44c8_s_p6_1},
    {&_44c8_s_p7_0,&_44c8_s_p7_1},
    {&_44c8_s_p8_0,&_44c8_s_p8_1},
    {&_44c8_s_p9_0,&_44c8_s_p9_1,&_44c8_s_p9_2}
   }
};
static static_bookblock _resbook_44s_9={
  {
    {0},{0,0,&_44c9_s_p1_0},{0,0,&_44c9_s_p2_0},{0,0,&_44c9_s_p3_0},
    {0,0,&_44c9_s_p4_0},
    {&_44c9_s_p5_0,&_44c9_s_p5_1},
    {&_44c9_s_p6_0,&_44c9_s_p6_1},
    {&_44c9_s_p7_0,&_44c9_s_p7_1},
    {&_44c9_s_p8_0,&_44c9_s_p8_1},
    {&_44c9_s_p9_0,&_44c9_s_p9_1,&_44c9_s_p9_2}
   }
};


static vorbis_residue_template _res_44s_0[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__44c0_s_short,&_huff_book__44c0_sm_short,
   &_resbook_44s_0,&_resbook_44sm_0},

  {2,0,  &_residue_44_mid,
   &_huff_book__44c0_s_long,&_huff_book__44c0_sm_long,
   &_resbook_44s_0,&_resbook_44sm_0}
};
static vorbis_residue_template _res_44s_1[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__44c1_s_short,&_huff_book__44c1_sm_short,
   &_resbook_44s_1,&_resbook_44sm_1},

  {2,0,  &_residue_44_mid,
   &_huff_book__44c1_s_long,&_huff_book__44c1_sm_long,
   &_resbook_44s_1,&_resbook_44sm_1}
};
static vorbis_residue_template _res_44s_2[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__44c2_s_short,&_huff_book__44c2_s_short,
   &_resbook_44s_2,&_resbook_44s_2},

  {2,0,  &_residue_44_mid,
   &_huff_book__44c2_s_long,&_huff_book__44c2_s_long,
   &_resbook_44s_2,&_resbook_44s_2}
};
static vorbis_residue_template _res_44s_3[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__44c3_s_short,&_huff_book__44c3_s_short,
   &_resbook_44s_3,&_resbook_44s_3},

  {2,0,  &_residue_44_mid,
   &_huff_book__44c3_s_long,&_huff_book__44c3_s_long,
   &_resbook_44s_3,&_resbook_44s_3}
};
static vorbis_residue_template _res_44s_4[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__44c4_s_short,&_huff_book__44c4_s_short,
   &_resbook_44s_4,&_resbook_44s_4},

  {2,0,  &_residue_44_mid,
   &_huff_book__44c4_s_long,&_huff_book__44c4_s_long,
   &_resbook_44s_4,&_resbook_44s_4}
};
static vorbis_residue_template _res_44s_5[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__44c5_s_short,&_huff_book__44c5_s_short,
   &_resbook_44s_5,&_resbook_44s_5},

  {2,0,  &_residue_44_mid,
   &_huff_book__44c5_s_long,&_huff_book__44c5_s_long,
   &_resbook_44s_5,&_resbook_44s_5}
};
static vorbis_residue_template _res_44s_6[]={
  {2,0,  &_residue_44_high,
   &_huff_book__44c6_s_short,&_huff_book__44c6_s_short,
   &_resbook_44s_6,&_resbook_44s_6},

  {2,0,  &_residue_44_high,
   &_huff_book__44c6_s_long,&_huff_book__44c6_s_long,
   &_resbook_44s_6,&_resbook_44s_6}
};
static vorbis_residue_template _res_44s_7[]={
  {2,0,  &_residue_44_high,
   &_huff_book__44c7_s_short,&_huff_book__44c7_s_short,
   &_resbook_44s_7,&_resbook_44s_7},

  {2,0,  &_residue_44_high,
   &_huff_book__44c7_s_long,&_huff_book__44c7_s_long,
   &_resbook_44s_7,&_resbook_44s_7}
};
static vorbis_residue_template _res_44s_8[]={
  {2,0,  &_residue_44_high,
   &_huff_book__44c8_s_short,&_huff_book__44c8_s_short,
   &_resbook_44s_8,&_resbook_44s_8},

  {2,0,  &_residue_44_high,
   &_huff_book__44c8_s_long,&_huff_book__44c8_s_long,
   &_resbook_44s_8,&_resbook_44s_8}
};
static vorbis_residue_template _res_44s_9[]={
  {2,0,  &_residue_44_high,
   &_huff_book__44c9_s_short,&_huff_book__44c9_s_short,
   &_resbook_44s_9,&_resbook_44s_9},

  {2,0,  &_residue_44_high,
   &_huff_book__44c9_s_long,&_huff_book__44c9_s_long,
   &_resbook_44s_9,&_resbook_44s_9}
};

static vorbis_mapping_template _mapres_template_44_stereo[]={
  { _map_nominal, _res_44s_0 }, /* 0 */
  { _map_nominal, _res_44s_1 }, /* 1 */
  { _map_nominal, _res_44s_2 }, /* 2 */
  { _map_nominal, _res_44s_3 }, /* 3 */
  { _map_nominal, _res_44s_4 }, /* 4 */
  { _map_nominal, _res_44s_5 }, /* 5 */
  { _map_nominal, _res_44s_6 }, /* 6 */
  { _map_nominal, _res_44s_7 }, /* 7 */
  { _map_nominal, _res_44s_8 }, /* 8 */
  { _map_nominal, _res_44s_9 }, /* 9 */
};
