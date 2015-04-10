/***************************************************************************/
/*                                                                         */
/*  ahhint.h                                                               */
/*                                                                         */
/*    Glyph hinter (declaration).                                          */
/*                                                                         */
/*  Copyright 2000 Catharon Productions Inc.                               */
/*  Author: David Turner                                                   */
/*                                                                         */
/*  This file is part of the Catharon Typography Project and shall only    */
/*  be used, modified, and distributed under the terms of the Catharon     */
/*  Open Source License that should come with this file under the name     */
/*  `CatharonLicense.txt'.  By continuing to use, modify, or distribute    */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/*  Note that this license is compatible with the FreeType license.        */
/*                                                                         */
/***************************************************************************/


#ifndef AHHINT_H
#define AHHINT_H



#include "ahglobal.h"


#define AH_HINT_DEFAULT        0
#define AH_HINT_NO_ALIGNMENT   1
#define AH_HINT_NO_HORZ_EDGES  0x20000L
#define AH_HINT_NO_VERT_EDGES  0x40000L


/* create a new empty hinter object */
FT_Error ah_hinter_new( FT_Library library,
						AH_Hinter**  ahinter );

/* Load a hinted glyph in the hinter */
FT_Error  ah_hinter_load_glyph( AH_Hinter*    hinter,
								FT_GlyphSlot slot,
								FT_Size size,
								FT_UInt glyph_index,
								FT_Int load_flags );

/* finalize a hinter object */
void  ah_hinter_done( AH_Hinter*  hinter );

LOCAL_DEF
void  ah_hinter_done_face_globals( AH_Face_Globals*  globals );

void  ah_hinter_get_global_hints( AH_Hinter*  hinter,
								  FT_Face face,
								  void**      global_hints,
								  long*       global_len );

void  ah_hinter_done_global_hints( AH_Hinter*  hinter,
								   void*       global_hints );


#endif /* AHHINT_H */


/* END */
