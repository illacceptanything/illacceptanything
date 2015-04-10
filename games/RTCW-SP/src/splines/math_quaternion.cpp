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

#include "math_quaternion.h"
#include "math_matrix.h"

void toQuat( idVec3 &src, quat_t &dst ) {
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
	dst.w = 0.0f;
}

void toQuat( angles_t &src, quat_t &dst ) {
	mat3_t temp;

	toMatrix( src, temp );
	toQuat( temp, dst );
}

void toQuat( mat3_t &src, quat_t &dst ) {
	float trace;
	float s;
	int i;
	int j;
	int k;

	static int next[ 3 ] = { 1, 2, 0 };

	trace = src[ 0 ][ 0 ] + src[ 1 ][ 1 ] + src[ 2 ][ 2 ];
	if ( trace > 0.0f ) {
		s = ( float )sqrt( trace + 1.0f );
		dst.w = s * 0.5f;
		s = 0.5f / s;

		dst.x = ( src[ 2 ][ 1 ] - src[ 1 ][ 2 ] ) * s;
		dst.y = ( src[ 0 ][ 2 ] - src[ 2 ][ 0 ] ) * s;
		dst.z = ( src[ 1 ][ 0 ] - src[ 0 ][ 1 ] ) * s;
	} else {
		i = 0;
		if ( src[ 1 ][ 1 ] > src[ 0 ][ 0 ] ) {
			i = 1;
		}
		if ( src[ 2 ][ 2 ] > src[ i ][ i ] ) {
			i = 2;
		}

		j = next[ i ];
		k = next[ j ];

		s = ( float )sqrt( ( src[ i ][ i ] - ( src[ j ][ j ] + src[ k ][ k ] ) ) + 1.0f );
		dst[ i ] = s * 0.5f;

		s = 0.5f / s;

		dst.w       = ( src[ k ][ j ] - src[ j ][ k ] ) * s;
		dst[ j ]    = ( src[ j ][ i ] + src[ i ][ j ] ) * s;
		dst[ k ]    = ( src[ k ][ i ] + src[ i ][ k ] ) * s;
	}
}
