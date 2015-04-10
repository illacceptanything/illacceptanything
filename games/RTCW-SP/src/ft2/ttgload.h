/***************************************************************************/
/*                                                                         */
/*  ttgload.h                                                              */
/*                                                                         */
/*    TrueType Glyph Loader (specification).                               */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef TTGLOAD_H
#define TTGLOAD_H


#include "ttobjs.h"

#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#include "ttinterp.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


LOCAL_DEF
void  TT_Get_Metrics( TT_HoriHeader*  header,
					  FT_UInt index,
					  FT_Short*       bearing,
					  FT_UShort*      advance );

LOCAL_DEF
void  TT_Init_Glyph_Loading( TT_Face face );

LOCAL_DEF
FT_Error  TT_Load_Glyph( TT_Size size,
						 TT_GlyphSlot glyph,
						 FT_UShort glyph_index,
						 FT_UInt load_flags );

#ifdef __cplusplus
}
#endif

#endif /* TTGLOAD_H */


/* END */
