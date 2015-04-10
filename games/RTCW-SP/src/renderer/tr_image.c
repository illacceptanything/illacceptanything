/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		tr_image.c
 *
 * desc:
 *
*/

#include "tr_local.h"

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#define JPEG_INTERNALS
#include "../jpeg-6/jpeglib.h"


static void LoadBMP( const char *name, byte **pic, int *width, int *height );
static void LoadTGA( const char *name, byte **pic, int *width, int *height );
static void LoadJPG( const char *name, byte **pic, int *width, int *height );

static byte s_intensitytable[256];
static unsigned char s_gammatable[256];

int gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int gl_filter_max = GL_LINEAR;

#define FILE_HASH_SIZE      4096
static image_t*        hashTable[FILE_HASH_SIZE];

// Ridah, in order to prevent zone fragmentation, all images will
// be read into this buffer. In order to keep things as fast as possible,
// we'll give it a starting value, which will account for the majority of
// images, but allow it to grow if the buffer isn't big enough
#define R_IMAGE_BUFFER_SIZE     ( 512 * 512 * 4 )     // 512 x 512 x 32bit

typedef enum {
	BUFFER_IMAGE,
	BUFFER_SCALED,
	BUFFER_RESAMPLED,
	BUFFER_MAX_TYPES
} bufferMemType_t;

int imageBufferSize[BUFFER_MAX_TYPES] = {0,0,0};
void        *imageBufferPtr[BUFFER_MAX_TYPES] = {NULL,NULL,NULL};

void *R_GetImageBuffer( int size, bufferMemType_t bufferType ) {
	if ( imageBufferSize[bufferType] < R_IMAGE_BUFFER_SIZE && size <= imageBufferSize[bufferType] ) {
		imageBufferSize[bufferType] = R_IMAGE_BUFFER_SIZE;
		imageBufferPtr[bufferType] = malloc( imageBufferSize[bufferType] );
	}
	if ( size > imageBufferSize[bufferType] ) {   // it needs to grow
		if ( imageBufferPtr[bufferType] ) {
			free( imageBufferPtr[bufferType] );
		}
		imageBufferSize[bufferType] = size;
		imageBufferPtr[bufferType] = malloc( imageBufferSize[bufferType] );
	}

	return imageBufferPtr[bufferType];
}

void R_FreeImageBuffer( void ) {
	int bufferType;
	for ( bufferType = 0; bufferType < BUFFER_MAX_TYPES; bufferType++ ) {
		if ( !imageBufferPtr[bufferType] ) {
			return;
		}
		free( imageBufferPtr[bufferType] );
		imageBufferSize[bufferType] = 0;
		imageBufferPtr[bufferType] = NULL;
	}
}

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

typedef struct {
	char *name;
	int minimize, maximize;
} textureMode_t;

textureMode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( fname[i] != '\0' ) {
		letter = tolower( fname[i] );
		if ( letter == '.' ) {
			break;                          // don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';                   // damn path names
		}
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( FILE_HASH_SIZE - 1 );
	return hash;
}

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string ) {
	int i;
	image_t *glt;

	for ( i = 0 ; i < 6 ; i++ ) {
		if ( !Q_stricmp( modes[i].name, string ) ) {
			break;
		}
	}

	// hack to prevent trilinear from being set on voodoo,
	// because their driver freaks...
	if ( i == 5 && glConfig.hardwareType == GLHW_3DFX_2D3D ) {
		ri.Printf( PRINT_ALL, "Refusing to set trilinear on a voodoo.\n" );
		i = 3;
	}


	if ( i == 6 ) {
		ri.Printf( PRINT_ALL, "bad filter name\n" );
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for ( i = 0 ; i < tr.numImages ; i++ ) {
		glt = tr.images[ i ];
		if ( glt->mipmap ) {
			GL_Bind( glt );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
		}
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int total;
	int i;

	total = 0;
	for ( i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[i]->frameUsed == tr.frameCount ) {
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f( void ) {
	int i;
	image_t *image;
	int texels;
	const char *yesno[] = {
		"no ", "yes"
	};

	ri.Printf( PRINT_ALL, "\n      -w-- -h-- -mm- -TMU- -if-- wrap --name-------\n" );
	texels = 0;

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[ i ];

		texels += image->uploadWidth * image->uploadHeight;
		ri.Printf( PRINT_ALL,  "%4i: %4i %4i  %s   %d   ",
				   i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap], image->TMU );
		switch ( image->internalFormat ) {
		case 1:
			ri.Printf( PRINT_ALL, "I    " );
			break;
		case 2:
			ri.Printf( PRINT_ALL, "IA   " );
			break;
		case 3:
			ri.Printf( PRINT_ALL, "RGB  " );
			break;
		case 4:
			ri.Printf( PRINT_ALL, "RGBA " );
			break;
		case GL_RGBA8:
			ri.Printf( PRINT_ALL, "RGBA8" );
			break;
		case GL_RGB8:
			ri.Printf( PRINT_ALL, "RGB8" );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			ri.Printf( PRINT_ALL, "DXT5 " );
			break;
		case GL_RGB4_S3TC:
			ri.Printf( PRINT_ALL, "S3TC4" );
			break;
		case GL_RGBA4:
			ri.Printf( PRINT_ALL, "RGBA4" );
			break;
		case GL_RGB5:
			ri.Printf( PRINT_ALL, "RGB5 " );
			break;
		default:
			ri.Printf( PRINT_ALL, "???? " );
		}

		switch ( image->wrapClampMode ) {
		case GL_REPEAT:
			ri.Printf( PRINT_ALL, "rept " );
			break;
		case GL_CLAMP:
			ri.Printf( PRINT_ALL, "clmp " );
			break;
		default:
			ri.Printf( PRINT_ALL, "%4i ", image->wrapClampMode );
			break;
		}

		ri.Printf( PRINT_ALL, " %s\n", image->imgName );
	}
	ri.Printf( PRINT_ALL, " ---------\n" );
	ri.Printf( PRINT_ALL, " %i total texels (not including mipmaps)\n", texels );
	ri.Printf( PRINT_ALL, " %i total images\n\n", tr.numImages );
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
before or after.
================
*/
static void ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out,
							 int outwidth, int outheight ) {
	int i, j;
	unsigned    *inrow, *inrow2;
	unsigned frac, fracstep;
	unsigned p1[1024], p2[1024];
	byte        *pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for ( i = 0 ; i < outwidth ; i++ ) {
		p1[i] = 4 * ( frac >> 16 );
		frac += fracstep;
	}
	frac = 3 * ( fracstep >> 2 );
	for ( i = 0 ; i < outwidth ; i++ ) {
		p2[i] = 4 * ( frac >> 16 );
		frac += fracstep;
	}

	for ( i = 0 ; i < outheight ; i++, out += outwidth ) {
		inrow = in + inwidth * (int)( ( i + 0.25 ) * inheight / outheight );
		inrow2 = in + inwidth * (int)( ( i + 0.75 ) * inheight / outheight );
		frac = fracstep >> 1;
		for ( j = 0 ; j < outwidth ; j++ ) {
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			( ( byte * )( out + j ) )[0] = ( pix1[0] + pix2[0] + pix3[0] + pix4[0] ) >> 2;
			( ( byte * )( out + j ) )[1] = ( pix1[1] + pix2[1] + pix3[1] + pix4[1] ) >> 2;
			( ( byte * )( out + j ) )[2] = ( pix1[2] + pix2[2] + pix3[2] + pix4[2] ) >> 2;
			( ( byte * )( out + j ) )[3] = ( pix1[3] + pix2[3] + pix3[3] + pix4[3] ) >> 2;
		}
	}
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture( unsigned *in, int inwidth, int inheight, qboolean only_gamma ) {
	if ( only_gamma ) {
		if ( !glConfig.deviceSupportsGamma ) {
			int i, c;
			byte    *p;

			p = (byte *)in;

			c = inwidth * inheight;
			for ( i = 0 ; i < c ; i++, p += 4 )
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	} else
	{
		int i, c;
		byte    *p;

		p = (byte *)in;

		c = inwidth * inheight;

		if ( glConfig.deviceSupportsGamma ) {
			for ( i = 0 ; i < c ; i++, p += 4 )
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		} else
		{
			for ( i = 0 ; i < c ; i++, p += 4 )
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}


/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2( unsigned *in, int inWidth, int inHeight ) {
	int i, j, k;
	byte        *outpix;
	int inWidthMask, inHeightMask;
	int total;
	int outWidth, outHeight;
	unsigned    *temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ ) {
			outpix = ( byte * )( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ ) {
				total =
					1 * ( (byte *)&in[ ( ( i * 2 - 1 ) & inHeightMask ) * inWidth + ( ( j * 2 - 1 ) & inWidthMask ) ] )[k] +
					2 * ( (byte *)&in[ ( ( i * 2 - 1 ) & inHeightMask ) * inWidth + ( ( j * 2 ) & inWidthMask ) ] )[k] +
					2 * ( (byte *)&in[ ( ( i * 2 - 1 ) & inHeightMask ) * inWidth + ( ( j * 2 + 1 ) & inWidthMask ) ] )[k] +
					1 * ( (byte *)&in[ ( ( i * 2 - 1 ) & inHeightMask ) * inWidth + ( ( j * 2 + 2 ) & inWidthMask ) ] )[k] +

					2 * ( (byte *)&in[ ( ( i * 2 ) & inHeightMask ) * inWidth + ( ( j * 2 - 1 ) & inWidthMask ) ] )[k] +
					4 * ( (byte *)&in[ ( ( i * 2 ) & inHeightMask ) * inWidth + ( ( j * 2 ) & inWidthMask ) ] )[k] +
					4 * ( (byte *)&in[ ( ( i * 2 ) & inHeightMask ) * inWidth + ( ( j * 2 + 1 ) & inWidthMask ) ] )[k] +
					2 * ( (byte *)&in[ ( ( i * 2 ) & inHeightMask ) * inWidth + ( ( j * 2 + 2 ) & inWidthMask ) ] )[k] +

					2 * ( (byte *)&in[ ( ( i * 2 + 1 ) & inHeightMask ) * inWidth + ( ( j * 2 - 1 ) & inWidthMask ) ] )[k] +
					4 * ( (byte *)&in[ ( ( i * 2 + 1 ) & inHeightMask ) * inWidth + ( ( j * 2 ) & inWidthMask ) ] )[k] +
					4 * ( (byte *)&in[ ( ( i * 2 + 1 ) & inHeightMask ) * inWidth + ( ( j * 2 + 1 ) & inWidthMask ) ] )[k] +
					2 * ( (byte *)&in[ ( ( i * 2 + 1 ) & inHeightMask ) * inWidth + ( ( j * 2 + 2 ) & inWidthMask ) ] )[k] +

					1 * ( (byte *)&in[ ( ( i * 2 + 2 ) & inHeightMask ) * inWidth + ( ( j * 2 - 1 ) & inWidthMask ) ] )[k] +
					2 * ( (byte *)&in[ ( ( i * 2 + 2 ) & inHeightMask ) * inWidth + ( ( j * 2 ) & inWidthMask ) ] )[k] +
					2 * ( (byte *)&in[ ( ( i * 2 + 2 ) & inHeightMask ) * inWidth + ( ( j * 2 + 1 ) & inWidthMask ) ] )[k] +
					1 * ( (byte *)&in[ ( ( i * 2 + 2 ) & inHeightMask ) * inWidth + ( ( j * 2 + 2 ) & inWidthMask ) ] )[k];
				outpix[k] = total / 36;
			}
		}
	}

	memcpy( in, temp, outWidth * outHeight * 4 );
	ri.Hunk_FreeTempMemory( temp );
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap( byte *in, int width, int height ) {
	int i, j;
	byte    *out;
	int row;

	if ( !r_simpleMipMaps->integer ) {
		R_MipMap2( (unsigned *)in, width, height );
		return;
	}

	if ( width == 1 && height == 1 ) {
		return;
	}

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;    // get largest
		for ( i = 0 ; i < width ; i++, out += 4, in += 8 ) {
			out[0] = ( in[0] + in[4] ) >> 1;
			out[1] = ( in[1] + in[5] ) >> 1;
			out[2] = ( in[2] + in[6] ) >> 1;
			out[3] = ( in[3] + in[7] ) >> 1;
		}
		return;
	}

	for ( i = 0 ; i < height ; i++, in += row ) {
		for ( j = 0 ; j < width ; j++, out += 4, in += 8 ) {
			out[0] = ( in[0] + in[4] + in[row + 0] + in[row + 4] ) >> 2;
			out[1] = ( in[1] + in[5] + in[row + 1] + in[row + 5] ) >> 2;
			out[2] = ( in[2] + in[6] + in[row + 2] + in[row + 6] ) >> 2;
			out[3] = ( in[3] + in[7] + in[row + 3] + in[row + 7] ) >> 2;
		}
	}
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static float R_RMSE( byte *in, int width, int height ) {
	int i, j;
	float out, rmse, rtemp;
	int row;

	rmse = 0.0f;

	if ( width <= 32 || height <= 32 ) {
		return 9999.0f;
	}

	row = width * 4;

	width >>= 1;
	height >>= 1;

	for ( i = 0 ; i < height ; i++, in += row ) {
		for ( j = 0 ; j < width ; j++, out += 4, in += 8 ) {
			out = ( in[0] + in[4] + in[row + 0] + in[row + 4] ) >> 2;
			rtemp = ( ( fabs( out - in[0] ) + fabs( out - in[4] ) + fabs( out - in[row + 0] ) + fabs( out - in[row + 4] ) ) );
			rtemp = rtemp * rtemp;
			rmse += rtemp;
			out = ( in[1] + in[5] + in[row + 1] + in[row + 5] ) >> 2;
			rtemp = ( ( fabs( out - in[1] ) + fabs( out - in[5] ) + fabs( out - in[row + 1] ) + fabs( out - in[row + 5] ) ) );
			rtemp = rtemp * rtemp;
			rmse += rtemp;
			out = ( in[2] + in[6] + in[row + 2] + in[row + 6] ) >> 2;
			rtemp = ( ( fabs( out - in[2] ) + fabs( out - in[6] ) + fabs( out - in[row + 2] ) + fabs( out - in[row + 6] ) ) );
			rtemp = rtemp * rtemp;
			rmse += rtemp;
			out = ( in[3] + in[7] + in[row + 3] + in[row + 7] ) >> 2;
			rtemp = ( ( fabs( out - in[3] ) + fabs( out - in[7] ) + fabs( out - in[row + 3] ) + fabs( out - in[row + 7] ) ) );
			rtemp = rtemp * rtemp;
			rmse += rtemp;
		}
	}
	rmse = sqrt( rmse / ( height * width * 4 ) );
	return rmse;
}


/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture( byte *data, int pixelCount, byte blend[4] ) {
	int i;
	int inverseAlpha;
	int premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for ( i = 0 ; i < pixelCount ; i++, data += 4 ) {
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}

byte mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};


/*
===============
Upload32

===============
*/
static void Upload32(   unsigned *data,
						int width, int height,
						qboolean mipmap,
						qboolean picmip,
						qboolean characterMip,  //----(SA)	added
						qboolean lightMap,
						int *format,
						int *pUploadWidth, int *pUploadHeight,
						qboolean noCompress ) {
	int samples;
	int scaled_width, scaled_height;
	unsigned    *scaledBuffer = NULL;
	unsigned    *resampledBuffer = NULL;
	int i, c;
	byte        *scan;
	GLenum internalFormat = GL_RGB;
	float rMax = 0, gMax = 0, bMax = 0;
	static int rmse_saved = 0;
	float rmse;

	// do the root mean square error stuff first
	if ( r_rmse->value ) {
		while ( R_RMSE( (byte *)data, width, height ) < r_rmse->value ) {
			rmse_saved += ( height * width * 4 ) - ( ( width >> 1 ) * ( height >> 1 ) * 4 );
			resampledBuffer = R_GetImageBuffer( ( width >> 1 ) * ( height >> 1 ) * 4, BUFFER_RESAMPLED );
			ResampleTexture( data, width, height, resampledBuffer, width >> 1, height >> 1 );
			data = resampledBuffer;
			width = width >> 1;
			height = height >> 1;
			ri.Printf( PRINT_ALL, "r_rmse of %f has saved %dkb\n", r_rmse->value, ( rmse_saved / 1024 ) );
		}
	} else {
		// just do the RMSE of 1 (reduce perfect)
		while ( R_RMSE( (byte *)data, width, height ) < 1.0 ) {
			rmse_saved += ( height * width * 4 ) - ( ( width >> 1 ) * ( height >> 1 ) * 4 );
			resampledBuffer = R_GetImageBuffer( ( width >> 1 ) * ( height >> 1 ) * 4, BUFFER_RESAMPLED );
			ResampleTexture( data, width, height, resampledBuffer, width >> 1, height >> 1 );
			data = resampledBuffer;
			width = width >> 1;
			height = height >> 1;
			ri.Printf( PRINT_ALL, "r_rmse of %f has saved %dkb\n", r_rmse->value, ( rmse_saved / 1024 ) );
		}
	}
	//
	// convert to exact power of 2 sizes
	//
	for ( scaled_width = 1 ; scaled_width < width ; scaled_width <<= 1 )
		;
	for ( scaled_height = 1 ; scaled_height < height ; scaled_height <<= 1 )
		;
	if ( r_roundImagesDown->integer && scaled_width > width ) {
		scaled_width >>= 1;
	}
	if ( r_roundImagesDown->integer && scaled_height > height ) {
		scaled_height >>= 1;
	}

	if ( scaled_width != width || scaled_height != height ) {
		//resampledBuffer = ri.Hunk_AllocateTempMemory( scaled_width * scaled_height * 4 );
		resampledBuffer = R_GetImageBuffer( scaled_width * scaled_height * 4, BUFFER_RESAMPLED );
		ResampleTexture( data, width, height, resampledBuffer, scaled_width, scaled_height );
		data = resampledBuffer;
		width = scaled_width;
		height = scaled_height;
	}

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		if ( characterMip ) {
			scaled_width >>= r_picmip2->integer;
			scaled_height >>= r_picmip2->integer;
		} else {
			scaled_width >>= r_picmip->integer;
			scaled_height >>= r_picmip->integer;
		}
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( scaled_width > glConfig.maxTextureSize
			|| scaled_height > glConfig.maxTextureSize ) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	rmse = R_RMSE( (byte *)data, width, height );

	if ( r_lowMemTextureSize->integer && ( scaled_width > r_lowMemTextureSize->integer || scaled_height > r_lowMemTextureSize->integer ) && rmse < r_lowMemTextureThreshold->value ) {
		int scale;

		for ( scale = 1 ; scale < r_lowMemTextureSize->integer; scale <<= 1 ) {
			;
		}

		while ( scaled_width > scale || scaled_height > scale ) {
			scaled_width >>= 1;
			scaled_height >>= 1;
		}

		ri.Printf( PRINT_ALL, "r_lowMemTextureSize forcing reduction from %i x %i to %i x %i\n", width, height, scaled_width, scaled_height );

		resampledBuffer = R_GetImageBuffer( scaled_width * scaled_height * 4, BUFFER_RESAMPLED );
		ResampleTexture( data, width, height, resampledBuffer, scaled_width, scaled_height );
		data = resampledBuffer;
		width = scaled_width;
		height = scaled_height;

	}


	//
	// clamp to minimum size
	//
	if ( scaled_width < 1 ) {
		scaled_width = 1;
	}
	if ( scaled_height < 1 ) {
		scaled_height = 1;
	}

	//scaledBuffer = ri.Hunk_AllocateTempMemory( sizeof( unsigned ) * scaled_width * scaled_height );
	scaledBuffer = R_GetImageBuffer( sizeof( unsigned ) * scaled_width * scaled_height, BUFFER_SCALED );

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width * height;
	scan = ( (byte *)data );
	samples = 3;
	if ( !lightMap ) {
		for ( i = 0; i < c; i++ )
		{
			if ( scan[i * 4 + 0] > rMax ) {
				rMax = scan[i * 4 + 0];
			}
			if ( scan[i * 4 + 1] > gMax ) {
				gMax = scan[i * 4 + 1];
			}
			if ( scan[i * 4 + 2] > bMax ) {
				bMax = scan[i * 4 + 2];
			}
			if ( scan[i * 4 + 3] != 255 ) {
				samples = 4;
				break;
			}
		}
		// select proper internal format
		if ( samples == 3 ) {
			if ( !noCompress && glConfig.textureCompression == TC_EXT_COMP_S3TC ) {
				// TODO: which format is best for which textures?
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			} else if ( !noCompress && glConfig.textureCompression == TC_S3TC )   {
				internalFormat = GL_RGB4_S3TC;
			} else if ( r_texturebits->integer == 16 )   {
				internalFormat = GL_RGB5;
			} else if ( r_texturebits->integer == 32 )   {
				internalFormat = GL_RGB8;
			} else
			{
				internalFormat = 3;
			}
		} else if ( samples == 4 )   {
			if ( !noCompress && glConfig.textureCompression == TC_EXT_COMP_S3TC ) {
				// TODO: which format is best for which textures?
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			} else if ( r_texturebits->integer == 16 )   {
				internalFormat = GL_RGBA4;
			} else if ( r_texturebits->integer == 32 )   {
				internalFormat = GL_RGBA8;
			} else
			{
				internalFormat = 4;
			}
		}
	} else {
		internalFormat = 3;
	}
	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) &&
		 ( scaled_height == height ) ) {
		if ( !mipmap ) {
			qglTexImage2D( GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}
		memcpy( scaledBuffer, data, width * height * 4 );
	} else
	{
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height ) {
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
		memcpy( scaledBuffer, data, width * height * 4 );
	}

	R_LightScaleTexture( scaledBuffer, scaled_width, scaled_height, !mipmap );

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	qglTexImage2D( GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );

	if ( mipmap ) {
		int miplevel;

		miplevel = 0;
		while ( scaled_width > 1 || scaled_height > 1 )
		{
			R_MipMap( (byte *)scaledBuffer, scaled_width, scaled_height );
			scaled_width >>= 1;
			scaled_height >>= 1;
			if ( scaled_width < 1 ) {
				scaled_width = 1;
			}
			if ( scaled_height < 1 ) {
				scaled_height = 1;
			}
			miplevel++;

			if ( r_colorMipLevels->integer ) {
				R_BlendOverTexture( (byte *)scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel] );
			}

			qglTexImage2D( GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
		}
	}
done:

	if ( mipmap ) {
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max );
	} else
	{
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	GL_CheckErrors();

	//if ( scaledBuffer != 0 )
	//	ri.Hunk_FreeTempMemory( scaledBuffer );
	//if ( resampledBuffer != 0 )
	//	ri.Hunk_FreeTempMemory( resampledBuffer );
}



//----(SA)	modified

/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t *R_CreateImageExt( const char *name, const byte *pic, int width, int height,
						   qboolean mipmap, qboolean allowPicmip, qboolean characterMip, int glWrapClampMode ) {
	image_t     *image;
	qboolean isLightmap = qfalse;
	long hash;
	qboolean noCompress = qfalse;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Error( ERR_DROP, "R_CreateImage: \"%s\" is too long\n", name );
	}
	if ( !strncmp( name, "*lightmap", 9 ) ) {
		isLightmap = qtrue;
		noCompress = qtrue;
	}
	if ( !noCompress && strstr( name, "skies" ) ) {
		noCompress = qtrue;
	}
	if ( !noCompress && strstr( name, "weapons" ) ) {    // don't compress view weapon skins
		noCompress = qtrue;
	}
	// RF, if the shader hasn't specifically asked for it, don't allow compression
	if ( r_ext_compressed_textures->integer == 2 && ( tr.allowCompress != qtrue ) ) {
		noCompress = qtrue;
	} else if ( r_ext_compressed_textures->integer == 1 && ( tr.allowCompress < 0 ) )     {
		noCompress = qtrue;
	}

	if ( tr.numImages == MAX_DRAWIMAGES ) {
		ri.Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n" );
	}

	// Ridah
	image = tr.images[tr.numImages] = R_CacheImageAlloc( sizeof( image_t ) );

	image->texnum = 1024 + tr.numImages;

	// Ridah
	if ( r_cacheShaders->integer ) {
		R_FindFreeTexnum( image );
	}
	// done.

	tr.numImages++;

	image->mipmap = mipmap;
	image->allowPicmip = allowPicmip;

	strcpy( image->imgName, name );

	image->width = width;
	image->height = height;
	image->wrapClampMode = glWrapClampMode;

	// lightmaps are always allocated on TMU 1
	if ( qglActiveTextureARB && isLightmap ) {
		image->TMU = 1;
	} else {
		image->TMU = 0;
	}

	if ( qglActiveTextureARB ) {
		GL_SelectTexture( image->TMU );
	}

	GL_Bind( image );

	Upload32( (unsigned *)pic,
			  image->width, image->height,
			  image->mipmap,
			  allowPicmip,
			  characterMip,                     //----(SA)	added
			  isLightmap,
			  &image->internalFormat,
			  &image->uploadWidth,
			  &image->uploadHeight,
			  noCompress );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode );

	qglBindTexture( GL_TEXTURE_2D, 0 );

	if ( image->TMU == 1 ) {
		GL_SelectTexture( 0 );
	}

	hash = generateHashValue( name );
	image->next = hashTable[hash];
	hashTable[hash] = image;

	// Ridah
	image->hash = hash;

	return image;
}

image_t *R_CreateImage( const char *name, const byte *pic, int width, int height,
						qboolean mipmap, qboolean allowPicmip, int glWrapClampMode ) {
	return R_CreateImageExt( name, pic, width, height, mipmap, allowPicmip, qfalse, glWrapClampMode );
}

//----(SA)	end

/*
=========================================================

BMP LOADING

=========================================================
*/
typedef struct
{
	char id[2];
	unsigned long fileSize;
	unsigned long reserved0;
	unsigned long bitmapDataOffset;
	unsigned long bitmapHeaderSize;
	unsigned long width;
	unsigned long height;
	unsigned short planes;
	unsigned short bitsPerPixel;
	unsigned long compression;
	unsigned long bitmapDataSize;
	unsigned long hRes;
	unsigned long vRes;
	unsigned long colors;
	unsigned long importantColors;
	unsigned char palette[256][4];
} BMPHeader_t;

static void LoadBMP( const char *name, byte **pic, int *width, int *height ) {
	int columns, rows, numPixels;
	byte    *pixbuf;
	int row, column;
	byte    *buf_p;
	byte    *buffer;
	int length;
	BMPHeader_t bmpHeader;
	byte        *bmpRGBA;

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_ReadFile( ( char * ) name, (void **)&buffer );
	if ( !buffer ) {
		return;
	}

	buf_p = buffer;

	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.reserved0 = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataOffset = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.width = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.height = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.planes = LittleShort( *( short * ) buf_p );
	buf_p += 2;
	bmpHeader.bitsPerPixel = LittleShort( *( short * ) buf_p );
	buf_p += 2;
	bmpHeader.compression = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataSize = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.hRes = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.vRes = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.colors = LittleLong( *( long * ) buf_p );
	buf_p += 4;
	bmpHeader.importantColors = LittleLong( *( long * ) buf_p );
	buf_p += 4;

	memcpy( bmpHeader.palette, buf_p, sizeof( bmpHeader.palette ) );

	if ( bmpHeader.bitsPerPixel == 8 ) {
		buf_p += 1024;
	}

	if ( bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M' ) {
		ri.Error( ERR_DROP, "LoadBMP: only Windows-style BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.fileSize != length ) {
		ri.Error( ERR_DROP, "LoadBMP: header size does not match file size (%d vs. %d) (%s)\n", bmpHeader.fileSize, length, name );
	}
	if ( bmpHeader.compression != 0 ) {
		ri.Error( ERR_DROP, "LoadBMP: only uncompressed BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.bitsPerPixel < 8 ) {
		ri.Error( ERR_DROP, "LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name );
	}

	columns = bmpHeader.width;
	rows = bmpHeader.height;
	if ( rows < 0 ) {
		rows = -rows;
	}
	numPixels = columns * rows;

	if ( width ) {
		*width = columns;
	}
	if ( height ) {
		*height = rows;
	}

	bmpRGBA = R_GetImageBuffer( numPixels * 4, BUFFER_IMAGE );

	*pic = bmpRGBA;


	for ( row = rows - 1; row >= 0; row-- )
	{
		pixbuf = bmpRGBA + row * columns * 4;

		for ( column = 0; column < columns; column++ )
		{
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;

			switch ( bmpHeader.bitsPerPixel )
			{
			case 8:
				palIndex = *buf_p++;
				*pixbuf++ = bmpHeader.palette[palIndex][2];
				*pixbuf++ = bmpHeader.palette[palIndex][1];
				*pixbuf++ = bmpHeader.palette[palIndex][0];
				*pixbuf++ = 0xff;
				break;
			case 16:
				shortPixel = *( unsigned short * ) pixbuf;
				pixbuf += 2;
				*pixbuf++ = ( shortPixel & ( 31 << 10 ) ) >> 7;
				*pixbuf++ = ( shortPixel & ( 31 << 5 ) ) >> 2;
				*pixbuf++ = ( shortPixel & ( 31 ) ) << 3;
				*pixbuf++ = 0xff;
				break;

			case 24:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;
			case 32:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alpha = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				break;
			default:
				ri.Error( ERR_DROP, "LoadBMP: illegal pixel_size '%d' in file '%s'\n", bmpHeader.bitsPerPixel, name );
				break;
			}
		}
	}

	ri.FS_FreeFile( buffer );

}


/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
static void LoadPCX( const char *filename, byte **pic, byte **palette, int *width, int *height ) {
	byte    *raw;
	pcx_t   *pcx;
	int x, y;
	int len;
	int dataByte, runLength;
	byte    *out, *pix;
	int xmax, ymax;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = ri.FS_ReadFile( ( char * ) filename, (void **)&raw );
	if ( !raw ) {
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;
	raw = &pcx->data;

	xmax = LittleShort( pcx->xmax );
	ymax = LittleShort( pcx->ymax );

	if ( pcx->manufacturer != 0x0a
		 || pcx->version != 5
		 || pcx->encoding != 1
		 || pcx->bits_per_pixel != 8
		 || xmax >= 1024
		 || ymax >= 1024 ) {
		ri.Printf( PRINT_ALL, "Bad pcx file %s (%i x %i) (%i x %i)\n", filename, xmax + 1, ymax + 1, pcx->xmax, pcx->ymax );
		return;
	}

	out = R_GetImageBuffer( ( ymax + 1 ) * ( xmax + 1 ), BUFFER_IMAGE );

	*pic = out;

	pix = out;

	if ( palette ) {
		*palette = malloc( 768 );
		memcpy( *palette, (byte *)pcx + len - 768, 768 );
	}

	if ( width ) {
		*width = xmax + 1;
	}
	if ( height ) {
		*height = ymax + 1;
	}
// FIXME: use bytes_per_line here?

	for ( y = 0 ; y <= ymax ; y++, pix += xmax + 1 )
	{
		for ( x = 0 ; x <= xmax ; )
		{
			dataByte = *raw++;

			if ( ( dataByte & 0xC0 ) == 0xC0 ) {
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			} else {
				runLength = 1;
			}

			while ( runLength-- > 0 )
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len ) {
		ri.Printf( PRINT_DEVELOPER, "PCX file %s was malformed", filename );
		free( *pic );
		*pic = NULL;
	}

	ri.FS_FreeFile( pcx );
}


/*
==============
LoadPCX32
==============
*/
static void LoadPCX32( const char *filename, byte **pic, int *width, int *height ) {
	byte    *palette;
	byte    *pic8;
	int i, c, p;
	byte    *pic32;

	LoadPCX( filename, &pic8, &palette, width, height );
	if ( !pic8 ) {
		*pic = NULL;
		return;
	}

	c = ( *width ) * ( *height );
	pic32 = *pic = R_GetImageBuffer( 4 * c, BUFFER_IMAGE );
	for ( i = 0 ; i < c ; i++ ) {
		p = pic8[i];
		pic32[0] = palette[p * 3];
		pic32[1] = palette[p * 3 + 1];
		pic32[2] = palette[p * 3 + 2];
		pic32[3] = 255;
		pic32 += 4;
	}

	free( pic8 );
	free( palette );
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

/*
=============
LoadTGA
=============
*/
void LoadTGA( const char *name, byte **pic, int *width, int *height ) {
	int columns, rows, numPixels;
	byte    *pixbuf;
	int row, column;
	byte    *buf_p;
	byte    *buffer;
	TargaHeader targa_header;
	byte        *targa_rgba;

	*pic = NULL;

	//
	// load the file
	//
	ri.FS_ReadFile( ( char * ) name, (void **)&buffer );
	if ( !buffer ) {
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	targa_header.colormap_index = LittleShort( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_length = LittleShort( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort( *(short *)buf_p );
	buf_p += 2;
	targa_header.y_origin = LittleShort( *(short *)buf_p );
	buf_p += 2;
	targa_header.width = LittleShort( *(short *)buf_p );
	buf_p += 2;
	targa_header.height = LittleShort( *(short *)buf_p );
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if ( targa_header.image_type != 2
		 && targa_header.image_type != 10
		 && targa_header.image_type != 3 ) {
		ri.Error( ERR_DROP, "LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n" );
	}

	if ( targa_header.colormap_type != 0 ) {
		ri.Error( ERR_DROP, "LoadTGA: colormaps not supported\n" );
	}

	if ( ( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 ) && targa_header.image_type != 3 ) {
		ri.Error( ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n" );
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if ( width ) {
		*width = columns;
	}
	if ( height ) {
		*height = rows;
	}

	targa_rgba = R_GetImageBuffer( numPixels * 4, BUFFER_IMAGE );
	*pic = targa_rgba;

	if ( targa_header.id_length != 0 ) {
		buf_p += targa_header.id_length;  // skip TARGA image comment

	}
	if ( targa_header.image_type == 2 || targa_header.image_type == 3 ) {
		// Uncompressed RGB or gray scale image
		for ( row = rows - 1; row >= 0; row-- )
		{
			pixbuf = targa_rgba + row * columns * 4;
			for ( column = 0; column < columns; column++ )
			{
				unsigned char red,green,blue,alphabyte;
				switch ( targa_header.pixel_size )
				{

				case 8:
					blue = *buf_p++;
					green = blue;
					red = blue;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;

				case 24:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;
				default:
					ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
					break;
				}
			}
		}
	} else if ( targa_header.image_type == 10 )       { // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;

		red = 0;
		green = 0;
		blue = 0;
		alphabyte = 0xff;

		for ( row = rows - 1; row >= 0; row-- ) {
			pixbuf = targa_rgba + row * columns * 4;
			for ( column = 0; column < columns; ) {
				packetHeader = *buf_p++;
				packetSize = 1 + ( packetHeader & 0x7f );
				if ( packetHeader & 0x80 ) {        // run-length packet
					switch ( targa_header.pixel_size ) {
					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						break;
					default:
						ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
						break;
					}

					for ( j = 0; j < packetSize; j++ ) {
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if ( column == columns ) { // run spans across rows
							column = 0;
							if ( row > 0 ) {
								row--;
							} else {
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				} else {                            // non run-length packet
					for ( j = 0; j < packetSize; j++ ) {
						switch ( targa_header.pixel_size ) {
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
						default:
							ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
							break;
						}
						column++;
						if ( column == columns ) { // pixel packet run spans across rows
							column = 0;
							if ( row > 0 ) {
								row--;
							} else {
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
breakOut:;
		}
	}

	ri.FS_FreeFile( buffer );
}

static void LoadJPG( const char *filename, unsigned char **pic, int *width, int *height ) {
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	/* This struct represents a JPEG error handler.  It is declared separately
	 * because applications often want to supply a specialized error handler
	 * (see the second half of this file for an example).  But here we just
	 * take the easy way out and use the standard error handler, which will
	 * print a message on stderr and call exit() if compression fails.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct jpeg_error_mgr jerr;
	/* More stuff */
	JSAMPARRAY buffer;      /* Output row buffer */
	int row_stride;     /* physical row width in output buffer */
	unsigned char *out;
	byte  *fbuffer;
	byte  *bbuf;

	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */

	ri.FS_ReadFile( ( char * ) filename, (void **)&fbuffer );
	if ( !fbuffer ) {
		return;
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We have to set up the error handler first, in case the initialization
	 * step fails.  (Unlikely, but it could happen if you are out of memory.)
	 * This routine fills in the contents of struct jerr, and returns jerr's
	 * address which we place into the link field in cinfo.
	 */
	cinfo.err = jpeg_std_error( &jerr );

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress( &cinfo );

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src( &cinfo, fbuffer );

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header( &cinfo, TRUE );
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */

	(void) jpeg_start_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	out = R_GetImageBuffer( cinfo.output_width * cinfo.output_height * cinfo.output_components, BUFFER_IMAGE );

	*pic = out;
	*width = cinfo.output_width;
	*height = cinfo.output_height;

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while ( cinfo.output_scanline < cinfo.output_height ) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		bbuf = ( ( out + ( row_stride * cinfo.output_scanline ) ) );
		buffer = &bbuf;
		(void) jpeg_read_scanlines( &cinfo, buffer, 1 );
	}

	// clear all the alphas to 255
	{
		int i, j;
		byte    *buf;

		buf = *pic;

		j = cinfo.output_width * cinfo.output_height * 4;
		for ( i = 3 ; i < j ; i += 4 ) {
			buf[i] = 255;
		}
	}

	/* Step 7: Finish decompression */

	(void) jpeg_finish_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress( &cinfo );

	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	ri.FS_FreeFile( fbuffer );

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
}


/* Expanded data destination object for stdio output */

typedef struct {
	struct jpeg_destination_mgr pub; /* public fields */

	byte* outfile;      /* target stream */
	int size;
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;


/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

void init_destination( j_compress_ptr cinfo ) {
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	dest->pub.next_output_byte = dest->outfile;
	dest->pub.free_in_buffer = dest->size;
}


/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */

boolean empty_output_buffer( j_compress_ptr cinfo ) {
	return TRUE;
}


/*
 * Compression initialization.
 * Before calling this, all parameters and a data destination must be set up.
 *
 * We require a write_all_tables parameter as a failsafe check when writing
 * multiple datastreams from the same compression object.  Since prior runs
 * will have left all the tables marked sent_table=TRUE, a subsequent run
 * would emit an abbreviated stream (no tables) by default.  This may be what
 * is wanted, but for safety's sake it should not be the default behavior:
 * programmers should have to make a deliberate choice to emit abbreviated
 * images.  Therefore the documentation and examples should encourage people
 * to pass write_all_tables=TRUE; then it will take active thought to do the
 * wrong thing.
 */

GLOBAL void
jpeg_start_compress( j_compress_ptr cinfo, boolean write_all_tables ) {
	if ( cinfo->global_state != CSTATE_START ) {
		ERREXIT1( cinfo, JERR_BAD_STATE, cinfo->global_state );
	}

	if ( write_all_tables ) {
		jpeg_suppress_tables( cinfo, FALSE ); /* mark all tables to be written */

	}
	/* (Re)initialize error mgr and destination modules */
	( *cinfo->err->reset_error_mgr )( (j_common_ptr) cinfo );
	( *cinfo->dest->init_destination )( cinfo );
	/* Perform master selection of active modules */
	jinit_compress_master( cinfo );
	/* Set up for the first pass */
	( *cinfo->master->prepare_for_pass )( cinfo );
	/* Ready for application to drive first pass through jpeg_write_scanlines
	 * or jpeg_write_raw_data.
	 */
	cinfo->next_scanline = 0;
	cinfo->global_state = ( cinfo->raw_data_in ? CSTATE_RAW_OK : CSTATE_SCANNING );
}


/*
 * Write some scanlines of data to the JPEG compressor.
 *
 * The return value will be the number of lines actually written.
 * This should be less than the supplied num_lines only in case that
 * the data destination module has requested suspension of the compressor,
 * or if more than image_height scanlines are passed in.
 *
 * Note: we warn about excess calls to jpeg_write_scanlines() since
 * this likely signals an application programmer error.  However,
 * excess scanlines passed in the last valid call are *silently* ignored,
 * so that the application need not adjust num_lines for end-of-image
 * when using a multiple-scanline buffer.
 */

GLOBAL JDIMENSION
jpeg_write_scanlines( j_compress_ptr cinfo, JSAMPARRAY scanlines,
					  JDIMENSION num_lines ) {
	JDIMENSION row_ctr, rows_left;

	if ( cinfo->global_state != CSTATE_SCANNING ) {
		ERREXIT1( cinfo, JERR_BAD_STATE, cinfo->global_state );
	}
	if ( cinfo->next_scanline >= cinfo->image_height ) {
		WARNMS( cinfo, JWRN_TOO_MUCH_DATA );
	}

	/* Call progress monitor hook if present */
	if ( cinfo->progress != NULL ) {
		cinfo->progress->pass_counter = (long) cinfo->next_scanline;
		cinfo->progress->pass_limit = (long) cinfo->image_height;
		( *cinfo->progress->progress_monitor )( (j_common_ptr) cinfo );
	}

	/* Give master control module another chance if this is first call to
	 * jpeg_write_scanlines.  This lets output of the frame/scan headers be
	 * delayed so that application can write COM, etc, markers between
	 * jpeg_start_compress and jpeg_write_scanlines.
	 */
	if ( cinfo->master->call_pass_startup ) {
		( *cinfo->master->pass_startup )( cinfo );
	}

	/* Ignore any extra scanlines at bottom of image. */
	rows_left = cinfo->image_height - cinfo->next_scanline;
	if ( num_lines > rows_left ) {
		num_lines = rows_left;
	}

	row_ctr = 0;
	( *cinfo->main->process_data )( cinfo, scanlines, &row_ctr, num_lines );
	cinfo->next_scanline += row_ctr;
	return row_ctr;
}

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

static int hackSize;

void term_destination( j_compress_ptr cinfo ) {
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
	size_t datacount = dest->size - dest->pub.free_in_buffer;
	hackSize = datacount;
}


/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */

void jpegDest( j_compress_ptr cinfo, byte* outfile, int size ) {
	my_dest_ptr dest;

	/* The destination object is made permanent so that multiple JPEG images
	 * can be written to the same file without re-executing jpeg_stdio_dest.
	 * This makes it dangerous to use this manager and a different destination
	 * manager serially with the same JPEG object, because their private object
	 * sizes may be different.  Caveat programmer.
	 */
	if ( cinfo->dest == NULL ) { /* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr *)
				  ( *cinfo->mem->alloc_small ) ( (j_common_ptr) cinfo, JPOOL_PERMANENT,
												 sizeof( my_destination_mgr ) );
	}

	dest = (my_dest_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outfile = outfile;
	dest->size = size;
}

void SaveJPG( char * filename, int quality, int image_width, int image_height, unsigned char *image_buffer ) {
	/* This struct contains the JPEG compression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 * It is possible to have several such structures, representing multiple
	 * compression/decompression processes, in existence at once.  We refer
	 * to any one struct (and its associated working data) as a "JPEG object".
	 */
	struct jpeg_compress_struct cinfo;
	/* This struct represents a JPEG error handler.  It is declared separately
	 * because applications often want to supply a specialized error handler
	 * (see the second half of this file for an example).  But here we just
	 * take the easy way out and use the standard error handler, which will
	 * print a message on stderr and call exit() if compression fails.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct jpeg_error_mgr jerr;
	/* More stuff */
	JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
	int row_stride;     /* physical row width in image buffer */
	unsigned char *out;

	/* Step 1: allocate and initialize JPEG compression object */

	/* We have to set up the error handler first, in case the initialization
	 * step fails.  (Unlikely, but it could happen if you are out of memory.)
	 * This routine fills in the contents of struct jerr, and returns jerr's
	 * address which we place into the link field in cinfo.
	 */
	cinfo.err = jpeg_std_error( &jerr );
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress( &cinfo );

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */

	/* Here we use the library-supplied code to send compressed data to a
	 * stdio stream.  You can also write your own code to do something else.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to write binary files.
	 */
	out = ri.Hunk_AllocateTempMemory( image_width * image_height * 4 );
	jpegDest( &cinfo, out, image_width * image_height * 4 );

	/* Step 3: set parameters for compression */

	/* First we supply a description of the input image.
	 * Four fields of the cinfo struct must be filled in:
	 */
	cinfo.image_width = image_width; /* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 4;     /* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
	/* Now use the library's routine to set default compression parameters.
	 * (You must set at least cinfo.in_color_space before calling this,
	 * since the defaults depend on the source color space.)
	 */
	jpeg_set_defaults( &cinfo );
	/* Now you can set any non-default parameters you wish to.
	 * Here we just illustrate the use of quality (quantization table) scaling:
	 */
	jpeg_set_quality( &cinfo, quality, TRUE /* limit to baseline-JPEG values */ );

	/* Step 4: Start compressor */

	/* TRUE ensures that we will write a complete interchange-JPEG file.
	 * Pass TRUE unless you are very sure of what you're doing.
	 */
	jpeg_start_compress( &cinfo, TRUE );

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 * To keep things simple, we pass one scanline per call; you can pass
	 * more if you wish, though.
	 */
	row_stride = image_width * 4; /* JSAMPLEs per row in image_buffer */

	while ( cinfo.next_scanline < cinfo.image_height ) {
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could pass
		 * more than one scanline at a time if that's more convenient.
		 */
		row_pointer[0] = &image_buffer[( ( cinfo.image_height - 1 ) * row_stride ) - cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines( &cinfo, row_pointer, 1 );
	}

	/* Step 6: Finish compression */

	jpeg_finish_compress( &cinfo );
	/* After finish_compress, we can close the output file. */
	ri.FS_WriteFile( filename, out, hackSize );

	ri.Hunk_FreeTempMemory( out );

	/* Step 7: release JPEG compression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_compress( &cinfo );

	/* And we're done! */
}

//===================================================================

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *name, byte **pic, int *width, int *height ) {
	int len;

	*pic = NULL;
	*width = 0;
	*height = 0;

	len = strlen( name );
	if ( len < 5 ) {
		return;
	}

	if ( !Q_stricmp( name + len - 4, ".tga" ) ) {
		LoadTGA( name, pic, width, height );          // try tga first
		if ( !*pic ) {                              //
			char altname[MAX_QPATH];                    // try jpg in place of tga
			strcpy( altname, name );
			len = strlen( altname );
			altname[len - 3] = 'j';
			altname[len - 2] = 'p';
			altname[len - 1] = 'g';
			LoadJPG( altname, pic, width, height );
		}
	} else if ( !Q_stricmp( name + len - 4, ".pcx" ) ) {
		LoadPCX32( name, pic, width, height );
	} else if ( !Q_stricmp( name + len - 4, ".bmp" ) ) {
		LoadBMP( name, pic, width, height );
	} else if ( !Q_stricmp( name + len - 4, ".jpg" ) ) {
		LoadJPG( name, pic, width, height );
	}
}


//----(SA)	modified
/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/


image_t *R_FindImageFileExt( const char *name, qboolean mipmap, qboolean allowPicmip, qboolean characterMIP, int glWrapClampMode ) {
	image_t *image;
	int width, height;
	byte    *pic;
	long hash;

	if ( !name ) {
		return NULL;
	}

	hash = generateHashValue( name );

	// Ridah, caching
	if ( r_cacheGathering->integer ) {
		ri.Cmd_ExecuteText( EXEC_NOW, va( "cache_usedfile image %s %i %i %i %i\n", name, mipmap, allowPicmip, characterMIP, glWrapClampMode ) );
	}

	//
	// see if the image is already loaded
	//
	for ( image = hashTable[hash]; image; image = image->next ) {
		if ( !Q_stricmp( name, image->imgName ) ) {
			// the white image can be used with any set of parms, but other mismatches are errors
			if ( strcmp( name, "*white" ) ) {
				if ( image->mipmap != mipmap ) {
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed mipmap parm\n", name );
				}
				if ( image->allowPicmip != allowPicmip ) {
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed allowPicmip parm\n", name );
				}
				if ( image->characterMIP != characterMIP ) {
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed characterMIP parm\n", name );
				}
				if ( image->wrapClampMode != glWrapClampMode ) {
					ri.Printf( PRINT_ALL, "WARNING: reused image %s with mixed glWrapClampMode parm\n", name );
				}
			}
			return image;
		}
	}

	// Ridah, check the cache
	// TTimo: suggests () around assignment used as truth value
	if ( ( image = R_FindCachedImage( name, hash ) ) ) {
		return image;
	}
	// done.

	//
	// load the pic from disk
	//
	R_LoadImage( name, &pic, &width, &height );
	if ( pic == NULL ) {                                    // if we dont get a successful load
// RF, no need to check uppercase on win32 systems
// TTimo: Duane changed to _DEBUG in all cases
// I'd still want that code in the release builds on linux
// (possibly for mod authors)
// /me maintained off for win32, using otherwise but printing diagnostics as developer
#if !defined( _WIN32 )
		char altname[MAX_QPATH];                            // copy the name
		int len;                                          //
		strcpy( altname, name );                          //
		len = strlen( altname );                          //
		altname[len - 3] = toupper( altname[len - 3] );   // and try upper case extension for unix systems
		altname[len - 2] = toupper( altname[len - 2] );   //
		altname[len - 1] = toupper( altname[len - 1] );   //
		ri.Printf( PRINT_DEVELOPER, "trying %s...", altname );
		R_LoadImage( altname, &pic, &width, &height );      //
		if ( pic == NULL ) {                              // if that fails
			ri.Printf( PRINT_DEVELOPER, "no\n" );
			return NULL;                                  // bail
		}
		ri.Printf( PRINT_DEVELOPER, "yes\n" );
#else
		return NULL;
#endif
	}

	image = R_CreateImageExt( ( char * ) name, pic, width, height, mipmap, allowPicmip, characterMIP, glWrapClampMode );
	//ri.Free( pic );
	return image;
}


image_t *R_FindImageFile( const char *name, qboolean mipmap, qboolean allowPicmip, int glWrapClampMode ) {
	return R_FindImageFileExt( name, mipmap, allowPicmip, qfalse, glWrapClampMode );
}

//----(SA)	end

/*
================
R_CreateDlightImage
================
*/
#define DLIGHT_SIZE 16
static void R_CreateDlightImage( void ) {
	int x,y;
	byte data[DLIGHT_SIZE][DLIGHT_SIZE][4];
	int b;

	// make a centered inverse-square falloff blob for dynamic lighting
	for ( x = 0 ; x < DLIGHT_SIZE ; x++ ) {
		for ( y = 0 ; y < DLIGHT_SIZE ; y++ ) {
			float d;

			d = ( DLIGHT_SIZE / 2 - 0.5f - x ) * ( DLIGHT_SIZE / 2 - 0.5f - x ) +
				( DLIGHT_SIZE / 2 - 0.5f - y ) * ( DLIGHT_SIZE / 2 - 0.5f - y );
			b = 4000 / d;
			if ( b > 255 ) {
				b = 255;
			} else if ( b < 75 ) {
				b = 0;
			}
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}
	tr.dlightImage = R_CreateImage( "*dlight", (byte *)data, DLIGHT_SIZE, DLIGHT_SIZE, qfalse, qfalse, GL_CLAMP );
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int i;
	float d;
	float exp;

	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow( (float)i / ( FOG_TABLE_SIZE - 1 ), exp );

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float   R_FogFactor( float s, float t ) {
	float d;

	s -= 1.0 / 512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0 / 32 ) {
		return 0;
	}
	if ( t < 31.0 / 32 ) {
		s *= ( t - 1.0f / 32.0f ) / ( 30.0f / 32.0f );
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[ (int)( s * ( FOG_TABLE_SIZE - 1 ) ) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define FOG_S   256
#define FOG_T   32
static void R_CreateFogImage( void ) {
	int x,y;
	byte    *data;
	float g;
	float d;
	float borderColor[4];

	data = ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	g = 2.0;

	// S is distance, T is depth
	for ( x = 0 ; x < FOG_S ; x++ ) {
		for ( y = 0 ; y < FOG_T ; y++ ) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[( y * FOG_S + x ) * 4 + 0] =
				data[( y * FOG_S + x ) * 4 + 1] =
					data[( y * FOG_S + x ) * 4 + 2] = 255;
			data[( y * FOG_S + x ) * 4 + 3] = 255 * d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage( "*fog", (byte *)data, FOG_S, FOG_T, qfalse, qfalse, GL_CLAMP );
	ri.Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateDefaultImage
==================
*/
#define DEFAULT_SIZE    16
static void R_CreateDefaultImage( void ) {
	int x;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		data[0][x][0] =
			data[0][x][1] =
				data[0][x][2] = 0; //----(SA) to make the default grid noticable but not blinding
		data[0][x][3] = 255;

		data[x][0][0] =
			data[x][0][1] =
				data[x][0][2] = 0; //----(SA) to make the default grid noticable but not blinding
		data[x][0][3] = 255;

		data[DEFAULT_SIZE - 1][x][0] =
			data[DEFAULT_SIZE - 1][x][1] =
				data[DEFAULT_SIZE - 1][x][2] = 0; //----(SA) to make the default grid noticable but not blinding
		data[DEFAULT_SIZE - 1][x][3] = 255;

		data[x][DEFAULT_SIZE - 1][0] =
			data[x][DEFAULT_SIZE - 1][1] =
				data[x][DEFAULT_SIZE - 1][2] = 0; //----(SA) to make the default grid noticable but not blinding
		data[x][DEFAULT_SIZE - 1][3] = 255;
	}
	tr.defaultImage = R_CreateImage( "*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qfalse, GL_REPEAT );
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	int x,y;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage( "*white", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT );

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		for ( y = 0 ; y < DEFAULT_SIZE ; y++ ) {
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;
		}
	}

	tr.identityLightImage = R_CreateImage( "*identityLight", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT );


	for ( x = 0; x < 32; x++ ) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage( "*scratch", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qtrue, GL_CLAMP );
	}

	R_CreateDlightImage();
	R_CreateFogImage();
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	int i, j;
	float g;
	int inf;
	int shift;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if ( !glConfig.deviceSupportsGamma ) {
		tr.overbrightBits = 0;      // need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if ( !glConfig.isFullscreen ) {
		tr.overbrightBits = 0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if ( glConfig.colorBits > 16 ) {
		if ( tr.overbrightBits > 2 ) {
			tr.overbrightBits = 2;
		}
	} else {
		if ( tr.overbrightBits > 1 ) {
			tr.overbrightBits = 1;
		}
	}
	if ( tr.overbrightBits < 0 ) {
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );
	tr.identityLightByte = 255 * tr.identityLight;


	if ( r_intensity->value <= 1 ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	if ( r_gamma->value < 0.5f ) {
		ri.Cvar_Set( "r_gamma", "0.5" );
	} else if ( r_gamma->value > 3.0f ) {
		ri.Cvar_Set( "r_gamma", "3.0" );
	}

	g = r_gamma->value;

	shift = tr.overbrightBits;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow( i / 255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		if ( inf < 0 ) {
			inf = 0;
		}
		if ( inf > 255 ) {
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for ( i = 0 ; i < 256 ; i++ ) {
		j = i * r_intensity->value;
		if ( j > 255 ) {
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if ( glConfig.deviceSupportsGamma ) {
		GLimp_SetGamma( s_gammatable, s_gammatable, s_gammatable );
	}
}

/*
===============
R_InitImages
===============
*/
void    R_InitImages( void ) {
	memset( hashTable, 0, sizeof( hashTable ) );

	// Ridah, caching system
	R_InitTexnumImages( qfalse );
	// done.

	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();

	// Ridah, load the cache media, if they were loaded previously, they'll be restored from the backupImages
	R_LoadCacheImages();
	// done.
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures( void ) {
	int i;

	for ( i = 0; i < tr.numImages ; i++ ) {
		qglDeleteTextures( 1, &tr.images[i]->texnum );
	}
	memset( tr.images, 0, sizeof( tr.images ) );
	// Ridah
	R_InitTexnumImages( qtrue );
	// done.

	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static char com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while ( ( c = *data ) <= ' ' ) {
			if ( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' ) {
			while ( *data && *data != '\n' )
				data++;
		}
		// skip /* */ comments
		else if ( c == '/' && data[1] == '*' ) {
			while ( *data && ( *data != '*' || data[1] != '/' ) )
			{
				data++;
			}
			if ( *data ) {
				data += 2;
			}
		} else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if ( c == '\"' ) {
		data++;
		while ( 1 )
		{
			c = *data++;
			if ( c == '\"' || !c ) {
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if ( len < MAX_TOKEN_CHARS ) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if ( len < MAX_TOKEN_CHARS ) {
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while ( c > 32 && c != ',' );

	if ( len == MAX_TOKEN_CHARS ) {
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


//----(SA) added so client can see what model or scale for the model was specified in a skin
/*
==============
RE_GetSkinModel
==============
*/
qboolean RE_GetSkinModel( qhandle_t skinid, const char *type, char *name ) {
	int i;
	skin_t      *bar;

	bar = tr.skins[skinid];

	if ( !Q_stricmp( type, "playerscale" ) ) {    // client is requesting scale from the skin rather than a model
		Com_sprintf( name, MAX_QPATH, "%.2f %.2f %.2f", bar->scale[0], bar->scale[1], bar->scale[2] );
		return qtrue;
	}

	for ( i = 0; i < bar->numModels; i++ )
	{
		if ( !Q_stricmp( bar->models[i]->type, type ) ) { // (SA) whoops, should've been this way
			Q_strncpyz( name, bar->models[i]->model, sizeof( bar->models[i]->model ) );
			return qtrue;
		}
	}
	return qfalse;
}

/*
==============
RE_GetShaderFromModel
	return a shader index for a given model's surface
	'withlightmap' set to '0' will create a new shader that is a copy of the one found
	on the model, without the lighmap stage, if the shader has a lightmap stage

	NOTE: only works for bmodels right now.  Could modify for other models (md3's etc.)
==============
*/
qhandle_t RE_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap ) {
	model_t     *model;
	bmodel_t    *bmodel;
	msurface_t  *surf;
	shader_t    *shd;

	if ( surfnum < 0 ) {
		surfnum = 0;
	}

	model = R_GetModelByHandle( modelid );  // (SA) should be correct now

	if ( model ) {
		bmodel  = model->bmodel;
		if ( bmodel && bmodel->firstSurface ) {
			if ( surfnum >= bmodel->numSurfaces ) { // if it's out of range, return the first surface
				surfnum = 0;
			}

			surf = bmodel->firstSurface + surfnum;
//			if(surf->shader->lightmapIndex != LIGHTMAP_NONE) {
			if ( surf->shader->lightmapIndex > LIGHTMAP_NONE ) {
				image_t *image;
				long hash;
				qboolean mip = qtrue;   // mip generation on by default

				// get mipmap info for original texture
				hash = generateHashValue( surf->shader->name );
				for ( image = hashTable[hash]; image; image = image->next ) {
					if ( !strcmp( surf->shader->name, image->imgName ) ) {
						mip = image->mipmap;
						break;
					}
				}
				shd = R_FindShader( surf->shader->name, LIGHTMAP_NONE, mip );
				shd->stages[0]->rgbGen = CGEN_LIGHTING_DIFFUSE; // (SA) new
			} else {
				shd = surf->shader;
			}

			return shd->index;
		}
	}

	return 0;
}

//----(SA) end

/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin( const char *name ) {
	qhandle_t hSkin;
	skin_t      *skin;
	skinSurface_t   *surf;
	skinModel_t *model;          //----(SA) added
	char        *text, *text_p;
	char        *token;
	char surfName[MAX_QPATH];

	if ( !name || !name[0] ) {
		Com_Printf( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) ) {
			if ( skin->numSurfaces == 0 ) {
				return 0;       // default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}


//----(SA)	moved things around slightly to fix the problem where you restart
//			a map that has ai characters who had invalid skin names entered
//			in thier "skin" or "head" field

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// If not a .skin file, load as a single shader
	if ( strcmp( name + strlen( name ) - 5, ".skin" ) ) {
		tr.numSkins++;
		skin = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
		tr.skins[hSkin] = skin;
		Q_strncpyz( skin->name, name, sizeof( skin->name ) );
		skin->numSurfaces   = 0;
		skin->numModels     = 0;    //----(SA) added
		skin->numSurfaces = 1;
		skin->surfaces[0] = ri.Hunk_Alloc( sizeof( skin->surfaces[0] ), h_low );
		skin->surfaces[0]->shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
	ri.FS_ReadFile( name, (void **)&text );
	if ( !text ) {
		return 0;
	}

	tr.numSkins++;
	skin = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces   = 0;
	skin->numModels     = 0;    //----(SA) added

//----(SA)	end

	text_p = text;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}

		if ( strstr( token, "md3_" ) ) {  // this is specifying a model
			model = skin->models[ skin->numModels ] = ri.Hunk_Alloc( sizeof( *skin->models[0] ), h_low );
			Q_strncpyz( model->type, token, sizeof( model->type ) );

			// get the model name
			token = CommaParse( &text_p );

			Q_strncpyz( model->model, token, sizeof( model->model ) );

			skin->numModels++;
			continue;
		}

//----(SA)	added
		if ( strstr( token, "playerscale" ) ) {
			token = CommaParse( &text_p );
			skin->scale[0] = atof( token );   // uniform scaling for now
			skin->scale[1] = atof( token );
			skin->scale[2] = atof( token );
			continue;
		}
//----(SA) end

		// parse the shader name
		token = CommaParse( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text );


	// never let a skin have 0 shaders

	//----(SA)	allow this for the (current) special case of the loper's upper body
	//			(it's upper body has no surfaces, only tags)
	if ( skin->numSurfaces == 0 ) {
		if ( !( strstr( name, "loper" ) && strstr( name, "upper" ) ) ) {
			return 0;       // use default skin
		}
	}

	return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void    R_InitSkins( void ) {
	skin_t      *skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = ri.Hunk_Alloc( sizeof( *skin->surfaces ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t  *R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void    R_SkinList_f( void ) {
	int i, j;
	skin_t      *skin;

	ri.Printf( PRINT_ALL, "------------------\n" );

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n",
					   skin->surfaces[j]->name, skin->surfaces[j]->shader->name );
		}
	}
	ri.Printf( PRINT_ALL, "------------------\n" );
}

// Ridah, utility for automatically cropping and numbering a bunch of images in a directory
/*
=============
SaveTGA

  saves out to 24 bit uncompressed format (no alpha)
=============
*/
void SaveTGA( char *name, byte **pic, int width, int height ) {
	byte    *inpixel, *outpixel;
	byte    *outbuf, *b;

	outbuf = ri.Hunk_AllocateTempMemory( width * height * 4 + 18 );
	b = outbuf;

	memset( b, 0, 18 );
	b[2] = 2;       // uncompressed type
	b[12] = width & 255;
	b[13] = width >> 8;
	b[14] = height & 255;
	b[15] = height >> 8;
	b[16] = 24; // pixel size

	{
		int row, col;
		int rows, cols;

		rows = ( height );
		cols = ( width );

		outpixel = b + 18;

		for ( row = ( rows - 1 ); row >= 0; row-- )
		{
			inpixel = ( ( *pic ) + ( row * cols ) * 4 );

			for ( col = 0; col < cols; col++ )
			{
				*outpixel++ = *( inpixel + 2 );   // blue
				*outpixel++ = *( inpixel + 1 );   // green
				*outpixel++ = *( inpixel + 0 );   // red
				//*outpixel++ = *(inpixel + 3);	// alpha

				inpixel += 4;
			}
		}
	}

	ri.FS_WriteFile( name, outbuf, (int)( outpixel - outbuf ) );

	ri.Hunk_FreeTempMemory( outbuf );

}

/*
=============
SaveTGAAlpha

  saves out to 32 bit uncompressed format (with alpha)
=============
*/
void SaveTGAAlpha( char *name, byte **pic, int width, int height ) {
	byte    *inpixel, *outpixel;
	byte    *outbuf, *b;

	outbuf = ri.Hunk_AllocateTempMemory( width * height * 4 + 18 );
	b = outbuf;

	memset( b, 0, 18 );
	b[2] = 2;       // uncompressed type
	b[12] = width & 255;
	b[13] = width >> 8;
	b[14] = height & 255;
	b[15] = height >> 8;
	b[16] = 32; // pixel size

	{
		int row, col;
		int rows, cols;

		rows = ( height );
		cols = ( width );

		outpixel = b + 18;

		for ( row = ( rows - 1 ); row >= 0; row-- )
		{
			inpixel = ( ( *pic ) + ( row * cols ) * 4 );

			for ( col = 0; col < cols; col++ )
			{
				*outpixel++ = *( inpixel + 2 );   // blue
				*outpixel++ = *( inpixel + 1 );   // green
				*outpixel++ = *( inpixel + 0 );   // red
				*outpixel++ = *( inpixel + 3 );   // alpha

				inpixel += 4;
			}
		}
	}

	ri.FS_WriteFile( name, outbuf, (int)( outpixel - outbuf ) );

	ri.Hunk_FreeTempMemory( outbuf );

}

/*
==============
R_CropImage
==============
*/
#define CROPIMAGES_ENABLED
//#define FUNNEL_HACK
#define RESIZE
//#define QUICKTIME_BANNER
#define TWILTB2_HACK

qboolean R_CropImage( char *name, byte **pic, int border, int *width, int *height, int lastBox[2] ) {
#ifdef CROPIMAGES_ENABLED
	int row, col;
	int rows, cols;
	byte    *inpixel, *temppic, *outpixel;
	int mins[2], maxs[2];
	int diff[2];
	//int	newWidth;
	int /*a, max,*/ i;
	int alpha;
	//int	*center;
	qboolean /*invalid,*/ skip;
	vec3_t fCol, fScale;
	qboolean filterColors = qfalse;
	int fCount;
	float f,c;
#define FADE_BORDER_RANGE   ( ( *width ) / 40 )

	rows = ( *height );
	cols = ( *width );

	mins[0] = 99999;
	mins[1] = 99999;
	maxs[0] = -99999;
	maxs[1] = -99999;

#ifdef TWILTB2_HACK
	// find the filter color (use first few pixels)
	filterColors = qtrue;
	inpixel = ( *pic );
	VectorClear( fCol );
	for ( fCount = 0; fCount < 8; fCount++, inpixel += 4 ) {
		for ( i = 0; i < 3; i++ ) {
			if ( *( inpixel + i ) > 70 ) {
				continue;   // too bright, cant be noise
			}
			if ( fCol[i] < *( inpixel + i ) ) {
				fCol[i] = *( inpixel + i );
			}
		}
	}
	//VectorScale( fCol, 1.0/fCount, fCol );
	for ( i = 0; i < 3; i++ ) {
		fCol[i] += 4;
		fScale[i] = 255.0 / ( 255.0 - fCol[i] );
	}
#endif

	for ( row = 0; row < rows; row++ )
	{
		inpixel = ( ( *pic ) + ( row * cols ) * 4 );

		for ( col = 0; col < cols; col++, inpixel += 4 )
		{
			if ( filterColors ) {
				// special code for filtering the twiltb series
				for ( i = 0; i < 3; i++ ) {
					f = *( inpixel + i );
					f -= fCol[i];
					if ( f <= 0 ) {
						f = 0;
					} else { f *= fScale[i];}
					if ( f > 255 ) {
						f = 255;
					}
					*( inpixel + i ) = f;
				}
				// if this pixel is near the edge, then fade it out (just do a brightness fade)
				if ( (int *)inpixel ) {
					// calc the fade scale
					f = (float)row / FADE_BORDER_RANGE;
					if ( f > (float)( rows - row - 1 ) / FADE_BORDER_RANGE ) {
						f = (float)( rows - row - 1 ) / FADE_BORDER_RANGE;
					}
					if ( f > (float)( cols - col - 1 ) / FADE_BORDER_RANGE ) {
						f = (float)( cols - col - 1 ) / FADE_BORDER_RANGE;
					}
					if ( f > (float)( col ) / FADE_BORDER_RANGE ) {
						f = (float)( col ) / FADE_BORDER_RANGE;
					}
					if ( f < 1.0 ) {
						if ( f <= 0 ) {
							*( (int *)inpixel ) = 0;
						} else {
							f += 0.2 * crandom();
							if ( f < 1.0 ) {
								if ( f <= 0 ) {
									*( (int *)inpixel ) = 0;
								} else {
									f = 1.0 - f;
									for ( i = 0; i < 3; i++ ) {
										c = *( inpixel + i );
										c -= f * c;
										if ( c < 0 ) {
											c = 0;
										}
										*( inpixel + i ) = c;
									}
								}
							}
						}
					}
				}
				continue;
			}

			skip = qfalse;
#ifdef QUICKTIME_BANNER
			// hack for quicktime ripped images
			if ( ( col > cols - 3 || row > rows - 36 ) ) {
				// filter this pixel out
				*( inpixel + 0 ) = 0;
				*( inpixel + 1 ) = 0;
				*( inpixel + 2 ) = 0;
				skip = qtrue;
			}
#endif

			if ( !skip ) {
				if (    ( *( inpixel + 0 ) > 20 )       // blue
						||  ( *( inpixel + 1 ) > 20 ) // green
						||  ( *( inpixel + 2 ) > 20 ) ) { // red

					if ( col < mins[0] ) {
						mins[0] = col;
					}
					if ( col > maxs[0] ) {
						maxs[0] = col;
					}
					if ( row < mins[1] ) {
						mins[1] = row;
					}
					if ( row > maxs[1] ) {
						maxs[1] = row;
					}

				} else {
					// filter this pixel out
					*( inpixel + 0 ) = 0;
					*( inpixel + 1 ) = 0;
					*( inpixel + 2 ) = 0;
				}
			}

#ifdef FUNNEL_HACK  // scale brightness down
			for ( i = 0; i < 3; i++ ) {
				alpha = *( inpixel + i );
				if ( ( alpha -= 20 ) < 0 ) {
					alpha = 0;
				}
				*( inpixel + i ) = alpha;
			}
#endif

			// set the alpha component
			alpha = *( inpixel + 0 ); // + *(inpixel + 1) + *(inpixel + 2);
			if ( *( inpixel + 1 ) > alpha ) {
				alpha = *( inpixel + 1 );
			}
			if ( *( inpixel + 2 ) > alpha ) {
				alpha = *( inpixel + 2 );
			}
			//alpha = (int)((float)alpha / 3.0);
			//alpha /= 3;
			if ( alpha > 255 ) {
				alpha = 255;
			}
			*( inpixel + 3 ) = (byte)alpha;
		}
	}

#ifdef RESIZE
	return qtrue;
#endif

	// convert it so that the center is the center of the image
	// this is used for some explosions
	/*
	for (i=0; i<2; i++) {
		if (i==0)	center = width;
		else		center = height;

		if ((*center/2 - mins[i]) > (maxs[i] - *center/2)) {
			maxs[i] = *center/2 + (*center/2 - mins[i]);
		} else {
			mins[i] = *center/2 - (maxs[i] - *center/2);
		}
	}
	*/

	// HACK for funnel
#ifdef FUNNEL_HACK
	mins[0] = 210;
	maxs[0] = 430;
	mins[1] = 0;
	maxs[1] = *height - 1;

	for ( i = 0; i < 2; i++ ) {
		diff[i] = maxs[i] - mins[i];
	}
#else

#ifndef RESIZE
	// apply the border
	for ( i = 0; i < 2; i++ ) {
		mins[i] -= border;
		if ( mins[i] < 0 ) {
			mins[i] = 0;
		}
		maxs[i] += border;
		if ( i == 0 ) {
			if ( maxs[i] > *width - 1 ) {
				maxs[i] = *width - 1;
			}
		} else {
			if ( maxs[i] > *height - 1 ) {
				maxs[i] = *height - 1;
			}
		}
	}

	// we have the mins/maxs, so work out the best square to crop to
	for ( i = 0; i < 2; i++ ) {
		diff[i] = maxs[i] - mins[i];
	}

	// widen the axis that has the smallest diff
	a = -1;
	if ( diff[1] > diff[0] ) {
		a = 0;
		max = *width - 1;
	} else if ( diff[0] > diff[1] ) {
		a = 1;
		max = *height - 1;
	}
	if ( a >= 0 ) {
		invalid = qfalse;
		while ( diff[a] < diff[!a] ) {
			if ( invalid ) {
				Com_Printf( "unable to find a good crop size\n" );
				return qfalse;
			}
			invalid = qtrue;
			if ( mins[a] > 0 ) {
				mins[a] -= 1;
				diff[a] = maxs[a] - mins[a];
				invalid = qfalse;
			}
			if ( ( diff[a] < diff[!a] ) && ( maxs[a] < max ) ) {
				maxs[a] += 1;
				diff[a] = maxs[a] - mins[a];
				invalid = qfalse;
			}
		}
	}

	// make sure it's bigger or equal to the last one
	for ( i = 0; i < 2; i++ ) {
		if ( ( maxs[i] - mins[i] ) < lastBox[i] ) {
			if ( i == 0 ) {
				center = width;
			} else { center = height;}

			maxs[i] = *center / 2 + ( lastBox[i] / 2 );
			mins[i] = maxs[i] - lastBox[i];
			diff[i] = lastBox[i];
		}
	}
#else
	for ( i = 0; i < 2; i++ ) {
		diff[i] = maxs[i] - mins[i];
		lastBox[i] = diff[i];
	}
#endif  // RESIZE
#endif  // FUNNEL_HACK

	temppic = malloc( sizeof( unsigned int ) * diff[0] * diff[1] );
	outpixel = temppic;

	for ( row = mins[1]; row < maxs[1]; row++ )
	{
		inpixel = ( ( *pic ) + ( row * cols ) * 4 );
		inpixel += mins[0] * 4;

		for ( col = mins[0]; col < maxs[0]; col++ )
		{
			*outpixel++ = *( inpixel + 0 );   // blue
			*outpixel++ = *( inpixel + 1 );   // green
			*outpixel++ = *( inpixel + 2 );   // red
			*outpixel++ = *( inpixel + 3 );   // alpha

			inpixel += 4;
		}
	}

	// for some reason this causes memory drop, not worth investigating (dev command only)
	//ri.Free( *pic );

	*pic = temppic;

	*width = diff[0];
	*height = diff[1];

	return qtrue;
#else
	return qtrue;   // shutup the compiler
#endif
}


/*
===============
R_CropAndNumberImagesInDirectory
===============
*/
void    R_CropAndNumberImagesInDirectory( char *dir, char *ext, int maxWidth, int maxHeight, int withAlpha ) {
#ifdef CROPIMAGES_ENABLED
	char    **fileList;
	int numFiles, j;
#ifndef RESIZE
	int i;
#endif
	byte    *pic, *temppic;
	int width, height, newWidth, newHeight;
	char    *pch;
	int b,c,d,lastNumber;
	int lastBox[2] = {0,0};

	fileList = ri.FS_ListFiles( dir, ext, &numFiles );

	if ( !numFiles ) {
		ri.Printf( PRINT_ALL, "no '%s' files in directory '%s'\n", ext, dir );
		return;
	}

	ri.Printf( PRINT_ALL, "%i files found, beginning processing..\n", numFiles );

	for ( j = 0; j < numFiles; j++ ) {
		char filename[MAX_QPATH], outfilename[MAX_QPATH];

		if ( !Q_strncmp( fileList[j], "spr", 3 ) ) {
			continue;
		}

		Com_sprintf( filename, sizeof( filename ), "%s/%s", dir, fileList[j] );
		ri.Printf( PRINT_ALL, "...cropping '%s'.. ", filename );

		R_LoadImage( filename, &pic, &width, &height );
		if ( !pic ) {
			ri.Printf( PRINT_ALL, "error reading file, ignoring.\n" );
			continue;
		}

		// file has been read, crop it, resize it down to a power of 2, then save
		if ( !R_CropImage( filename, &pic, 6, &width, &height, lastBox ) ) {
			ri.Printf( PRINT_ALL, "unable to crop image.\n" );
			//ri.Free( pic );
			break;
		}
#ifndef RESIZE
		for ( i = 2; ( 1 << i ) <= maxWidth; i++ ) {
			if ( ( width < ( 1 << i ) ) && ( width > ( 1 << ( i - 1 ) ) ) ) {
				newWidth = ( 1 << ( i - 1 ) );
				if ( newWidth > maxWidth ) {
					newWidth = maxWidth;
				}
				newHeight = newWidth;
				temppic = ri.Z_Malloc( sizeof( unsigned int ) * newWidth * newHeight );
				ResampleTexture( (unsigned int *)pic, width, height, (unsigned int *)temppic, newWidth, newHeight );
				memcpy( pic, temppic, sizeof( unsigned int ) * newWidth * newHeight );
				ri.Free( temppic );
				width = height = newWidth;
				break;
			}
		}
		if ( width > maxWidth ) {
			// we need to force the scale downwards
			newWidth = maxWidth;
			newHeight = maxWidth;
			temppic = ri.Z_Malloc( sizeof( unsigned int ) * newWidth * newHeight );
			ResampleTexture( (unsigned int *)pic, width, height, (unsigned int *)temppic, newWidth, newHeight );
			memcpy( pic, temppic, sizeof( unsigned int ) * newWidth * newHeight );
			ri.Free( temppic );
			width = newWidth;
			height = newHeight;
		}
#else
		newWidth = maxWidth;
		newHeight = maxHeight;
		temppic = malloc( sizeof( unsigned int ) * newWidth * newHeight );
		ResampleTexture( (unsigned int *)pic, width, height, (unsigned int *)temppic, newWidth, newHeight );
		memcpy( pic, temppic, sizeof( unsigned int ) * newWidth * newHeight );
		free( temppic );
		width = newWidth;
		height = newHeight;
#endif

		// set the new filename
		pch = strrchr( filename, '/' );
		*pch = '\0';
		lastNumber = j;
		b = lastNumber / 100;
		lastNumber -= b * 100;
		c = lastNumber / 10;
		lastNumber -= c * 10;
		d = lastNumber;
		Com_sprintf( outfilename, sizeof( outfilename ), "%s/spr%i%i%i.tga", filename, b, c, d );

		if ( withAlpha ) {
			SaveTGAAlpha( outfilename, &pic, width, height );
		} else {
			SaveTGA( outfilename, &pic, width, height );
		}

		// free the pixel data
		//ri.Free( pic );

		ri.Printf( PRINT_ALL, "done.\n" );
	}
#endif
}

/*
==============
R_CropImages_f
==============
*/
void R_CropImages_f( void ) {
#ifdef CROPIMAGES_ENABLED
	if ( ri.Cmd_Argc() < 5 ) {
		ri.Printf( PRINT_ALL, "syntax: cropimages <dir> <extension> <maxWidth> <maxHeight> <alpha 0/1>\neg: 'cropimages sprites/fire1 .tga 64 64 0'\n" );
		return;
	}
	R_CropAndNumberImagesInDirectory( ri.Cmd_Argv( 1 ), ri.Cmd_Argv( 2 ), atoi( ri.Cmd_Argv( 3 ) ), atoi( ri.Cmd_Argv( 4 ) ), atoi( ri.Cmd_Argv( 5 ) ) );
#else
	ri.Printf( PRINT_ALL, "This command has been disabled.\n" );
#endif
}
// done.

//==========================================================================================
// Ridah, caching system

static int numBackupImages = 0;
static image_t  *backupHashTable[FILE_HASH_SIZE];

static image_t  *texnumImages[MAX_DRAWIMAGES * 2];

/*
===============
R_CacheImageAlloc

  this will only get called to allocate the image_t structures, not that actual image pixels
===============
*/
void *R_CacheImageAlloc( int size ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		return malloc( size );
		//return ri.Z_Malloc( size );
	} else {
		return ri.Hunk_Alloc( size, h_low );
	}
}

/*
===============
R_CacheImageFree
===============
*/
void R_CacheImageFree( void *ptr ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		free( ptr );
		//ri.Free( ptr );
	}
}

/*
===============
R_TouchImage

  remove this image from the backupHashTable and make sure it doesn't get overwritten
===============
*/
qboolean R_TouchImage( image_t *inImage ) {
	image_t *bImage, *bImagePrev;
	int hash;
	char *name;

	if ( inImage == tr.dlightImage ||
		 inImage == tr.whiteImage ||
		 inImage == tr.defaultImage ||
		 inImage->imgName[0] == '*' ) { // can't use lightmaps since they might have the same name, but different maps will have different actual lightmap pixels
		return qfalse;
	}

	hash = inImage->hash;
	name = inImage->imgName;

	bImage = backupHashTable[hash];
	bImagePrev = NULL;
	while ( bImage ) {

		if ( bImage == inImage ) {
			// add it to the current images
			if ( tr.numImages == MAX_DRAWIMAGES ) {
				ri.Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n" );
			}

			tr.images[tr.numImages] = bImage;

			// remove it from the backupHashTable
			if ( bImagePrev ) {
				bImagePrev->next = bImage->next;
			} else {
				backupHashTable[hash] = bImage->next;
			}

			// add it to the hashTable
			bImage->next = hashTable[hash];
			hashTable[hash] = bImage;

			// get the new texture
			tr.numImages++;

			return qtrue;
		}

		bImagePrev = bImage;
		bImage = bImage->next;
	}

	return qtrue;
}

/*
===============
R_PurgeImage
===============
*/
void R_PurgeImage( image_t *image ) {

	texnumImages[image->texnum - 1024] = NULL;

	qglDeleteTextures( 1, &image->texnum );

	R_CacheImageFree( image );

	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}


/*
===============
R_PurgeBackupImages

  Can specify the number of Images to purge this call (used for background purging)
===============
*/
void R_PurgeBackupImages( int purgeCount ) {
	int i, cnt;
	static int lastPurged = 0;
	image_t *image;

	if ( !numBackupImages ) {
		// nothing to purge
		lastPurged = 0;
		return;
	}

	R_SyncRenderThread();

	cnt = 0;
	for ( i = lastPurged; i < FILE_HASH_SIZE; ) {
		lastPurged = i;
		// TTimo: suggests () around assignment used as truth value
		if ( ( image = backupHashTable[i] ) ) {
			// kill it
			backupHashTable[i] = image->next;
			R_PurgeImage( image );
			cnt++;

			if ( cnt >= purgeCount ) {
				return;
			}
		} else {
			i++;    // no images in this slot, so move to the next one
		}
	}

	// all done
	numBackupImages = 0;
	lastPurged = 0;
}

/*
===============
R_BackupImages
===============
*/
void R_BackupImages( void ) {

	if ( !r_cache->integer ) {
		return;
	}
	if ( !r_cacheShaders->integer ) {
		return;
	}

	// backup the hashTable
	memcpy( backupHashTable, hashTable, sizeof( backupHashTable ) );

	// pretend we have cleared the list
	numBackupImages = tr.numImages;
	tr.numImages = 0;

	memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SelectTexture( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SelectTexture( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

/*
=============
R_FindCachedImage
=============
*/
image_t *R_FindCachedImage( const char *name, int hash ) {
	image_t *bImage, *bImagePrev;

	if ( !r_cacheShaders->integer ) {
		return NULL;
	}

	if ( !numBackupImages ) {
		return NULL;
	}

	bImage = backupHashTable[hash];
	bImagePrev = NULL;
	while ( bImage ) {

		if ( !Q_stricmp( name, bImage->imgName ) ) {
			// add it to the current images
			if ( tr.numImages == MAX_DRAWIMAGES ) {
				ri.Error( ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n" );
			}

			R_TouchImage( bImage );
			return bImage;
		}

		bImagePrev = bImage;
		bImage = bImage->next;
	}

	return NULL;
}

/*
===============
R_InitTexnumImages
===============
*/
static int last_i;
void R_InitTexnumImages( qboolean force ) {
	if ( force || !numBackupImages ) {
		memset( texnumImages, 0, sizeof( texnumImages ) );
		last_i = 0;
	}
}

/*
============
R_FindFreeTexnum
============
*/
void R_FindFreeTexnum( image_t *inImage ) {
	int i, max;
	image_t **image;

	max = ( MAX_DRAWIMAGES * 2 );
	if ( last_i && !texnumImages[last_i + 1] ) {
		i = last_i + 1;
	} else {
		i = 0;
		image = (image_t **)&texnumImages;
		while ( i < max && *( image++ ) ) {
			i++;
		}
	}

	if ( i < max ) {
		if ( i < max - 1 ) {
			last_i = i;
		} else {
			last_i = 0;
		}
		inImage->texnum = 1024 + i;
		texnumImages[i] = inImage;
	} else {
		ri.Error( ERR_DROP, "R_FindFreeTexnum: MAX_DRAWIMAGES hit\n" );
	}
}

/*
===============
R_LoadCacheImages
===============
*/
void R_LoadCacheImages( void ) {
	int len;
	byte *buf;
	char    *token, *pString;
	char name[MAX_QPATH];
	int parms[3], i;

	if ( numBackupImages ) {
		return;
	}

	len = ri.FS_ReadFile( "image.cache", NULL );

	if ( len <= 0 ) {
		return;
	}

	buf = (byte *)ri.Hunk_AllocateTempMemory( len );
	ri.FS_ReadFile( "image.cache", (void **)&buf );
	pString = (char*)buf;   //DAJ added (char*)

	while ( ( token = COM_ParseExt( &pString, qtrue ) ) && token[0] ) {
		Q_strncpyz( name, token, sizeof( name ) );
		for ( i = 0; i < 4; i++ ) {
			token = COM_ParseExt( &pString, qfalse );
			parms[i] = atoi( token );
		}
		R_FindImageFileExt( name, parms[0], parms[1], parms[2], parms[3] );
	}

	ri.Hunk_FreeTempMemory( buf );
}
// done.
//==========================================================================================
