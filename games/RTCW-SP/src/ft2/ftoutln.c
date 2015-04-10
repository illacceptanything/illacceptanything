/***************************************************************************/
/*                                                                         */
/*  ftoutln.c                                                              */
/*                                                                         */
/*    FreeType outline management (body).                                  */
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


/*************************************************************************/
/*                                                                       */
/* All functions are declared in freetype.h.                             */
/*                                                                       */
/*************************************************************************/


#include "ftoutln.h"
#include "ftobjs.h"


/*************************************************************************/
/*                                                                       */
/* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
/* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
/* messages during execution.                                            */
/*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_outline


static
const FT_Outline null_outline = { 0, 0, 0, 0, 0, 0 };


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Decompose                                               */
/*                                                                       */
/* <Description>                                                         */
/*    Walks over an outline's structure to decompose it into individual  */
/*    segments and Bezier arcs.  This function is also able to emit      */
/*    `move to' and `close to' operations to indicate the start and end  */
/*    of new contours in the outline.                                    */
/*                                                                       */
/* <Input>                                                               */
/*    outline   :: A pointer to the source target.                       */
/*                                                                       */
/*    interface :: A table of `emitters', i.e,. function pointers called */
/*                 during decomposition to indicate path operations.     */
/*                                                                       */
/*    user      :: A typeless pointer which is passed to each emitter    */
/*                 during the decomposition.  It can be used to store    */
/*                 the state during the decomposition.                   */
/*                                                                       */
/* <Return>                                                              */
/*    FreeType error code.  0 means sucess.                              */
/*                                                                       */
FT_EXPORT_FUNC( FT_Error )  FT_Outline_Decompose(
	FT_Outline *        outline,
	FT_Outline_Funcs *  interface,
	void*              user )
{
#undef SCALED
#define SCALED( x )  ( ( ( x ) << shift ) - delta )

	FT_Vector v_last;
	FT_Vector v_control;
	FT_Vector v_start;

	FT_Vector*  point;
	FT_Vector*  limit;
	char*       tags;

	FT_Error error;

	FT_Int n;           /* index of contour in outline     */
	FT_UInt first;      /* index of first point in contour */
	char tag;           /* current point's state           */

	FT_Int shift;
	FT_Pos delta;


	if ( !outline || !interface ) {
		return FT_Err_Invalid_Argument;
	}

	shift = interface->shift;
	delta = interface->delta;
	first = 0;

	for ( n = 0; n < outline->n_contours; n++ )
	{
		FT_Int last; /* index of last point in contour */


		last  = outline->contours[n];
		limit = outline->points + last;

		v_start = outline->points[first];
		v_last  = outline->points[last];

		v_start.x = SCALED( v_start.x ); v_start.y = SCALED( v_start.y );
		v_last.x  = SCALED( v_last.x );  v_last.y  = SCALED( v_last.y );

		v_control = v_start;

		point = outline->points + first;
		tags  = outline->tags  + first;
		tag   = FT_CURVE_TAG( tags[0] );

		/* A contour cannot start with a cubic control point! */
		if ( tag == FT_Curve_Tag_Cubic ) {
			goto Invalid_Outline;
		}

		/* check first point to determine origin */
		if ( tag == FT_Curve_Tag_Conic ) {
			/* first point is conic control.  Yes, this happens. */
			if ( FT_CURVE_TAG( outline->tags[last] ) == FT_Curve_Tag_On ) {
				/* start at last point if it is on the curve */
				v_start = v_last;
				limit--;
			} else
			{
				/* if both first and last points are conic,         */
				/* start at their middle and record its position    */
				/* for closure                                      */
				v_start.x = ( v_start.x + v_last.x ) / 2;
				v_start.y = ( v_start.y + v_last.y ) / 2;

				v_last = v_start;
			}
			point--;
			tags--;
		}

		error = interface->move_to( &v_start, user );
		if ( error ) {
			goto Exit;
		}

		while ( point < limit )
		{
			point++;
			tags++;

			tag = FT_CURVE_TAG( tags[0] );
			switch ( tag )
			{
			case FT_Curve_Tag_On: /* emit a single line_to */
			{
				FT_Vector vec;


				vec.x = SCALED( point->x );
				vec.y = SCALED( point->y );

				error = interface->line_to( &vec, user );
				if ( error ) {
					goto Exit;
				}
				continue;
			}

			case FT_Curve_Tag_Conic: /* consume conic arcs */
				v_control.x = SCALED( point->x );
				v_control.y = SCALED( point->y );

Do_Conic:
				if ( point < limit ) {
					FT_Vector vec;
					FT_Vector v_middle;


					point++;
					tags++;
					tag = FT_CURVE_TAG( tags[0] );

					vec.x = SCALED( point->x );
					vec.y = SCALED( point->y );

					if ( tag == FT_Curve_Tag_On ) {
						error = interface->conic_to( &v_control, &vec, user );
						if ( error ) {
							goto Exit;
						}
						continue;
					}

					if ( tag != FT_Curve_Tag_Conic ) {
						goto Invalid_Outline;
					}

					v_middle.x = ( v_control.x + vec.x ) / 2;
					v_middle.y = ( v_control.y + vec.y ) / 2;

					error = interface->conic_to( &v_control, &v_middle, user );
					if ( error ) {
						goto Exit;
					}

					v_control = vec;
					goto Do_Conic;
				}

				error = interface->conic_to( &v_control, &v_start, user );
				goto Close;

			default: /* FT_Curve_Tag_Cubic */
			{
				FT_Vector vec1, vec2;


				if ( point + 1 > limit                             ||
					 FT_CURVE_TAG( tags[1] ) != FT_Curve_Tag_Cubic ) {
					goto Invalid_Outline;
				}

				point += 2;
				tags  += 2;

				vec1.x = SCALED( point[-2].x ); vec1.y = SCALED( point[-2].y );
				vec2.x = SCALED( point[-1].x ); vec2.y = SCALED( point[-1].y );

				if ( point <= limit ) {
					FT_Vector vec;


					vec.x = SCALED( point->x );
					vec.y = SCALED( point->y );

					error = interface->cubic_to( &vec1, &vec2, &vec, user );
					if ( error ) {
						goto Exit;
					}
					continue;
				}

				error = interface->cubic_to( &vec1, &vec2, &v_start, user );
				goto Close;
			}
			}
		}

		/* close the contour with a line segment */
		error = interface->line_to( &v_start, user );

Close:
		if ( error ) {
			goto Exit;
		}

		first = last + 1;
	}

	return 0;

Exit:
	return error;

Invalid_Outline:
	return FT_Err_Invalid_Outline;
}


FT_EXPORT_FUNC( FT_Error )  FT_Outline_New_Internal(
	FT_Memory memory,
	FT_UInt numPoints,
	FT_Int numContours,
	FT_Outline *  outline )
{
	FT_Error error;


	if ( !outline || !memory ) {
		return FT_Err_Invalid_Argument;
	}

	*outline = null_outline;

	if ( ALLOC_ARRAY( outline->points,   numPoints * 2L, FT_Pos    ) ||
		 ALLOC_ARRAY( outline->tags,     numPoints,      FT_Byte   ) ||
		 ALLOC_ARRAY( outline->contours, numContours,    FT_UShort ) ) {
		goto Fail;
	}

	outline->n_points    = (FT_UShort)numPoints;
	outline->n_contours  = (FT_Short)numContours;
	outline->flags      |= ft_outline_owner;

	return FT_Err_Ok;

Fail:
	outline->flags |= ft_outline_owner;
	FT_Outline_Done_Internal( memory, outline );

	return error;
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_New                                                     */
/*                                                                       */
/* <Description>                                                         */
/*    Creates a new outline of a given size.                             */
/*                                                                       */
/* <Input>                                                               */
/*    library     :: A handle to the library object from where the       */
/*                   outline is allocated.  Note however that the new    */
/*                   outline will NOT necessarily be FREED, when         */
/*                   destroying the library, by FT_Done_FreeType().      */
/*                                                                       */
/*    numPoints   :: The maximal number of points within the outline.    */
/*                                                                       */
/*    numContours :: The maximal number of contours within the outline.  */
/*                                                                       */
/* <Output>                                                              */
/*    outline     :: A handle to the new outline.  NULL in case of       */
/*                   error.                                              */
/*                                                                       */
/* <Return>                                                              */
/*    FreeType error code.  0 means success.                             */
/*                                                                       */
/* <MT-Note>                                                             */
/*    No.                                                                */
/*                                                                       */
/* <Note>                                                                */
/*    The reason why this function takes a `library' parameter is simply */
/*    to use the library's memory allocator.                             */
/*                                                                       */
FT_EXPORT_FUNC( FT_Error )  FT_Outline_New( FT_Library library,
											FT_UInt numPoints,
											FT_Int numContours,
											FT_Outline *  outline )
{
	if ( !library ) {
		return FT_Err_Invalid_Library_Handle;
	}

	return FT_Outline_New_Internal( library->memory, numPoints,
									numContours, outline );
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Copy                                                    */
/*                                                                       */
/* <Description>                                                         */
/*    Copies an outline into another one.  Both objects must have the    */
/*    same sizes (number of points & number of contours) when this       */
/*    function is called.                                                */
/*                                                                       */
/* <Input>                                                               */
/*    source :: A handle to the source outline.                          */
/*                                                                       */
/* <Output>                                                              */
/*    target :: A handle to the target outline.                          */
/*                                                                       */
/* <Return>                                                              */
/*    FreeType error code.  0 means success.                             */
/*                                                                       */
FT_EXPORT_FUNC( FT_Error )  FT_Outline_Copy( FT_Outline *  source,
											 FT_Outline *  target )
{
	FT_Int is_owner;


	if ( !source            || !target            ||
		 source->n_points   != target->n_points   ||
		 source->n_contours != target->n_contours ) {
		return FT_Err_Invalid_Argument;
	}

	MEM_Copy( target->points, source->points,
			  source->n_points * sizeof( FT_Vector ) );

	MEM_Copy( target->tags, source->tags,
			  source->n_points * sizeof( FT_Byte ) );

	MEM_Copy( target->contours, source->contours,
			  source->n_contours * sizeof( FT_Short ) );

	/* copy all flags, except the `ft_outline_owner' one */
	is_owner      = target->flags & ft_outline_owner;
	target->flags = source->flags;

	target->flags &= ~ft_outline_owner;
	target->flags |= is_owner;

	return FT_Err_Ok;
}


FT_EXPORT_FUNC( FT_Error )  FT_Outline_Done_Internal( FT_Memory memory,
													  FT_Outline *  outline )
{
	if ( outline ) {
		if ( outline->flags & ft_outline_owner ) {
			FREE( outline->points   );
			FREE( outline->tags     );
			FREE( outline->contours );
		}
		*outline = null_outline;

		return FT_Err_Ok;
	} else {
		return FT_Err_Invalid_Argument;
	}
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Done                                                    */
/*                                                                       */
/* <Description>                                                         */
/*    Destroys an outline created with FT_Outline_New().                 */
/*                                                                       */
/* <Input>                                                               */
/*    library :: A handle of the library object used to allocate the     */
/*               outline.                                                */
/*                                                                       */
/*    outline :: A pointer to the outline object to be discarded.        */
/*                                                                       */
/* <Return>                                                              */
/*    FreeType error code.  0 means success.                             */
/*                                                                       */
/* <MT-Note>                                                             */
/*    No.                                                                */
/*                                                                       */
/* <Note>                                                                */
/*    If the outline's `owner' field is not set, only the outline        */
/*    descriptor will be released.                                       */
/*                                                                       */
/*    The reason why this function takes an `outline' parameter is       */
/*    simply to use FT_Free().                                           */
/*                                                                       */
FT_EXPORT_FUNC( FT_Error )  FT_Outline_Done( FT_Library library,
											 FT_Outline *  outline )
{
	/* check for valid `outline' in FT_Outline_Done_Internal() */

	if ( !library ) {
		return FT_Err_Invalid_Library_Handle;
	}

	return FT_Outline_Done_Internal( library->memory, outline );
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Get_CBox                                                */
/*                                                                       */
/* <Description>                                                         */
/*    Returns an outline's `control box'.  The control box encloses all  */
/*    the outline's points, including Bezier control points.  Though it  */
/*    coincides with the exact bounding box for most glyphs, it can be   */
/*    slightly larger in some situations (like when rotating an outline  */
/*    which contains Bezier outside arcs).                               */
/*                                                                       */
/*    Computing the control box is very fast, while getting the bounding */
/*    box can take much more time as it needs to walk over all segments  */
/*    and arcs in the outline.  To get the latter, you can use the       */
/*    `ftbbox' component which is dedicated to this single task.         */
/*                                                                       */
/* <Input>                                                               */
/*    outline :: A pointer to the source outline descriptor.             */
/*                                                                       */
/* <Output>                                                              */
/*    cbox    :: The outline's control box.                              */
/*                                                                       */
/* <MT-Note>                                                             */
/*    Yes.                                                               */
/*                                                                       */
FT_EXPORT_FUNC( void )  FT_Outline_Get_CBox( FT_Outline *  outline,
											 FT_BBox *     cbox )
{
	FT_Pos xMin, yMin, xMax, yMax;


	if ( outline && cbox ) {
		if ( outline->n_points == 0 ) {
			xMin = 0;
			yMin = 0;
			xMax = 0;
			yMax = 0;
		} else
		{
			FT_Vector*  vec   = outline->points;
			FT_Vector*  limit = vec + outline->n_points;


			xMin = xMax = vec->x;
			yMin = yMax = vec->y;
			vec++;

			for ( ; vec < limit; vec++ )
			{
				FT_Pos x, y;


				x = vec->x;
				if ( x < xMin ) {
					xMin = x;
				}
				if ( x > xMax ) {
					xMax = x;
				}

				y = vec->y;
				if ( y < yMin ) {
					yMin = y;
				}
				if ( y > yMax ) {
					yMax = y;
				}
			}
		}
		cbox->xMin = xMin;
		cbox->xMax = xMax;
		cbox->yMin = yMin;
		cbox->yMax = yMax;
	}
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Translate                                               */
/*                                                                       */
/* <Description>                                                         */
/*    Applies a simple translation to the points of an outline.          */
/*                                                                       */
/* <Input>                                                               */
/*    outline :: A pointer to the target outline descriptor.             */
/*                                                                       */
/*    xOffset :: The horizontal offset.                                  */
/*                                                                       */
/*    yOffset :: The vertical offset.                                    */
/*                                                                       */
/* <MT-Note>                                                             */
/*    Yes.                                                               */
/*                                                                       */
FT_EXPORT_FUNC( void )  FT_Outline_Translate( FT_Outline *  outline,
											  FT_Pos xOffset,
											  FT_Pos yOffset )
{
	FT_UShort n;
	FT_Vector*  vec = outline->points;


	for ( n = 0; n < outline->n_points; n++ )
	{
		vec->x += xOffset;
		vec->y += yOffset;
		vec++;
	}
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Reverse                                                 */
/*                                                                       */
/* <Description>                                                         */
/*    Reverses the drawing direction of an outline.  This is used to     */
/*    ensure consistent fill conventions for mirrored glyphs.            */
/*                                                                       */
/* <Input>                                                               */
/*    outline :: A pointer to the target outline descriptor.             */
/*                                                                       */
/* <Note>                                                                */
/*    This functions toggles the bit flag `ft_outline_reverse_fill' in   */
/*    the outline's `flags' field.                                       */
/*                                                                       */
FT_EXPORT_FUNC( void )  FT_Outline_Reverse( FT_Outline *  outline )
{
	FT_UShort n;
	FT_Int first, last;


	first = 0;

	for ( n = 0; n < outline->n_contours; n++ )
	{
		last  = outline->contours[n];

		/* reverse point table */
		{
			FT_Vector*  p = outline->points + first;
			FT_Vector*  q = outline->points + last;
			FT_Vector swap;


			while ( p < q )
			{
				swap = *p;
				*p   = *q;
				*q   = swap;
				p++;
				q--;
			}
		}

		/* reverse tags table */
		{
			char*  p = outline->tags + first;
			char*  q = outline->tags + last;
			char swap;


			while ( p < q )
			{
				swap = *p;
				*p   = *q;
				*q   = swap;
				p++;
				q--;
			}
		}

		first = last + 1;
	}

	outline->flags ^= ft_outline_reverse_fill;
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Render                                                  */
/*                                                                       */
/* <Description>                                                         */
/*    Renders an outline within a bitmap using the current scan-convert. */
/*    This functions uses an FT_Raster_Params structure as an argument,  */
/*    allowing advanced features like direct composition, translucency,  */
/*    etc.                                                               */
/*                                                                       */
/* <Input>                                                               */
/*    library :: A handle to a FreeType library object.                  */
/*                                                                       */
/*    outline :: A pointer to the source outline descriptor.             */
/*                                                                       */
/*    params  :: A pointer to a FT_Raster_Params structure used to       */
/*               describe the rendering operation.                       */
/*                                                                       */
/* <Return>                                                              */
/*    FreeType error code.  0 means success.                             */
/*                                                                       */
/* <MT-Note>                                                             */
/*    YES.  Rendering is synchronized, so that concurrent calls to the   */
/*    scan-line converter will be serialized.                            */
/*                                                                       */
/* <Note>                                                                */
/*    You should know what you are doing and how FT_Raster_Params works  */
/*    to use this function.                                              */
/*                                                                       */
/*    The field `params.source' will be set to `outline' before the scan */
/*    converter is called, which means that the value you give to it is  */
/*    actually ignored.                                                  */
/*                                                                       */
FT_EXPORT_FUNC( FT_Error )  FT_Outline_Render( FT_Library library,
											   FT_Outline *        outline,
											   FT_Raster_Params *  params )
{
	FT_Error error;
	FT_Bool update = 0;
	FT_Renderer renderer;
	FT_ListNode node;


	if ( !library ) {
		return FT_Err_Invalid_Library_Handle;
	}

	if ( !params ) {
		return FT_Err_Invalid_Argument;
	}

	renderer = library->cur_renderer;
	node     = library->renderers.head;

	params->source = (void*)outline;

	error = FT_Err_Cannot_Render_Glyph;
	while ( renderer )
	{
		error = renderer->raster_render( renderer->raster, params );
		if ( !error || error != FT_Err_Cannot_Render_Glyph ) {
			break;
		}

		/* FT_Err_Cannot_Render_Glyph is returned if the render mode   */
		/* is unsupported by the current renderer for this glyph image */
		/* format                                                      */

		/* now, look for another renderer that supports the same */
		/* format                                                */
		renderer = FT_Lookup_Renderer( library, ft_glyph_format_outline,
									   &node );
		update   = 1;
	}

	/* if we changed the current renderer for the glyph image format */
	/* we need to select it as the next current one                  */
	if ( !error && update && renderer ) {
		FT_Set_Renderer( library, renderer, 0, 0 );
	}

	return error;
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Get_Bitmap                                              */
/*                                                                       */
/* <Description>                                                         */
/*    Renders an outline within a bitmap.  The outline's image is simply */
/*    OR-ed to the target bitmap.                                        */
/*                                                                       */
/* <Input>                                                               */
/*    library :: A handle to a FreeType library object.                  */
/*                                                                       */
/*    outline :: A pointer to the source outline descriptor.             */
/*                                                                       */
/*    map     :: A pointer to the target bitmap descriptor.              */
/*                                                                       */
/* <Return>                                                              */
/*    FreeType error code.  0 means success.                             */
/*                                                                       */
/* <MT-Note>                                                             */
/*    YES.  Rendering is synchronized, so that concurrent calls to the   */
/*    scan-line converter will be serialized.                            */
/*                                                                       */
/* <Note>                                                                */
/*    This function does NOT CREATE the bitmap, it only renders an       */
/*    outline image within the one you pass to it!                       */
/*                                                                       */
/*    It will use the raster correponding to the default glyph format.   */
/*                                                                       */
FT_EXPORT_FUNC( FT_Error )  FT_Outline_Get_Bitmap( FT_Library library,
												   FT_Outline *  outline,
												   FT_Bitmap *   bitmap )
{
	FT_Raster_Params params;


	if ( !bitmap ) {
		return FT_Err_Invalid_Argument;
	}

	/* other checks are delayed to FT_Outline_Render() */

	params.target = bitmap;
	params.flags  = 0;

	if ( bitmap->pixel_mode == ft_pixel_mode_grays ) {
		params.flags |= ft_raster_flag_aa;
	}

	return FT_Outline_Render( library, outline, &params );
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Vector_Transform                                                */
/*                                                                       */
/* <Description>                                                         */
/*    Transforms a single vector through a 2x2 matrix.                   */
/*                                                                       */
/* <InOut>                                                               */
/*    vector :: The target vector to transform.                          */
/*                                                                       */
/* <Input>                                                               */
/*    matrix :: A pointer to the source 2x2 matrix.                      */
/*                                                                       */
/* <MT-Note>                                                             */
/*    Yes.                                                               */
/*                                                                       */
/* <Note>                                                                */
/*    The result is undefined if either `vector' or `matrix' is invalid. */
/*                                                                       */
FT_EXPORT_FUNC( void )  FT_Vector_Transform( FT_Vector *  vector,
											 FT_Matrix *  matrix )
{
	FT_Pos xz, yz;


	if ( !vector || !matrix ) {
		return;
	}

	xz = FT_MulFix( vector->x, matrix->xx ) +
		 FT_MulFix( vector->y, matrix->xy );

	yz = FT_MulFix( vector->x, matrix->yx ) +
		 FT_MulFix( vector->y, matrix->yy );

	vector->x = xz;
	vector->y = yz;
}


/*************************************************************************/
/*                                                                       */
/* <Function>                                                            */
/*    FT_Outline_Transform                                               */
/*                                                                       */
/* <Description>                                                         */
/*    Applies a simple 2x2 matrix to all of an outline's points.  Useful */
/*    for applying rotations, slanting, flipping, etc.                   */
/*                                                                       */
/* <Input>                                                               */
/*    outline :: A pointer to the target outline descriptor.             */
/*                                                                       */
/*    matrix  :: A pointer to the transformation matrix.                 */
/*                                                                       */
/* <MT-Note>                                                             */
/*    Yes.                                                               */
/*                                                                       */
/* <Note>                                                                */
/*    You can use FT_Outline_Translate() if you need to translate the    */
/*    outline's points.                                                  */
/*                                                                       */
FT_EXPORT_FUNC( void )  FT_Outline_Transform( FT_Outline *  outline,
											  FT_Matrix *   matrix )
{
	FT_Vector*  vec = outline->points;
	FT_Vector*  limit = vec + outline->n_points;


	for ( ; vec < limit; vec++ )
		FT_Vector_Transform( vec, matrix );
}

/* END */
