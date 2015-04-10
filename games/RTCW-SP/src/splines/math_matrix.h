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

#ifndef __MATH_MATRIX_H__
#define __MATH_MATRIX_H__

#include <string.h>
#include "math_vector.h"

#ifndef ID_INLINE
#ifdef _WIN32
#define ID_INLINE __inline
#else
#define ID_INLINE inline
#endif
#endif

class quat_t;
class angles_t;

class mat3_t {
public:
idVec3 mat[ 3 ];

mat3_t();
mat3_t( float src[ 3 ][ 3 ] );
mat3_t( idVec3 const &x, idVec3 const &y, idVec3 const &z );
mat3_t( const float xx, const float xy, const float xz, const float yx, const float yy, const float yz, const float zx, const float zy, const float zz );

friend void     toMatrix( quat_t const &src, mat3_t &dst );
friend void     toMatrix( angles_t const &src, mat3_t &dst );
friend void     toMatrix( idVec3 const &src, mat3_t &dst );

idVec3 operator[]( int index ) const;
idVec3          &operator[]( int index );

idVec3 operator*( const idVec3 &vec ) const;
mat3_t operator*( const mat3_t &a ) const;
mat3_t operator*( float a ) const;
mat3_t operator+( mat3_t const &a ) const;
mat3_t operator-( mat3_t const &a ) const;

friend idVec3 operator*( const idVec3 &vec, const mat3_t &mat );
friend mat3_t operator*( float a, mat3_t const &b );

mat3_t          &operator*=( float a );
mat3_t          &operator+=( mat3_t const &a );
mat3_t          &operator-=( mat3_t const &a );

void            Clear( void );

void            ProjectVector( const idVec3 &src, idVec3 &dst ) const;
void            UnprojectVector( const idVec3 &src, idVec3 &dst ) const;

void            OrthoNormalize( void );
void            Transpose( mat3_t &matrix );
void            Transpose( void );
mat3_t          Inverse( void ) const;
void            Identity( void );

friend void     InverseMultiply( const mat3_t &inv, const mat3_t &b, mat3_t &dst );
friend mat3_t   SkewSymmetric( idVec3 const &src );
};

ID_INLINE mat3_t::mat3_t() {
}

ID_INLINE mat3_t::mat3_t( float src[ 3 ][ 3 ] ) {
	memcpy( mat, src, sizeof( src ) );
}

ID_INLINE mat3_t::mat3_t( idVec3 const &x, idVec3 const &y, idVec3 const &z ) {
	mat[ 0 ].x = x.x; mat[ 0 ].y = x.y; mat[ 0 ].z = x.z;
	mat[ 1 ].x = y.x; mat[ 1 ].y = y.y; mat[ 1 ].z = y.z;
	mat[ 2 ].x = z.x; mat[ 2 ].y = z.y; mat[ 2 ].z = z.z;
}

ID_INLINE mat3_t::mat3_t( const float xx, const float xy, const float xz, const float yx, const float yy, const float yz, const float zx, const float zy, const float zz ) {
	mat[ 0 ].x = xx; mat[ 0 ].y = xy; mat[ 0 ].z = xz;
	mat[ 1 ].x = yx; mat[ 1 ].y = yy; mat[ 1 ].z = yz;
	mat[ 2 ].x = zx; mat[ 2 ].y = zy; mat[ 2 ].z = zz;
}

ID_INLINE idVec3 mat3_t::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return mat[ index ];
}

ID_INLINE idVec3& mat3_t::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return mat[ index ];
}

ID_INLINE idVec3 mat3_t::operator*( const idVec3 &vec ) const {
	return idVec3(
			   mat[ 0 ].x * vec.x + mat[ 1 ].x * vec.y + mat[ 2 ].x * vec.z,
			   mat[ 0 ].y * vec.x + mat[ 1 ].y * vec.y + mat[ 2 ].y * vec.z,
			   mat[ 0 ].z * vec.x + mat[ 1 ].z * vec.y + mat[ 2 ].z * vec.z );
}

ID_INLINE mat3_t mat3_t::operator*( const mat3_t &a ) const {
	return mat3_t(
			   mat[0].x * a[0].x + mat[0].y * a[1].x + mat[0].z * a[2].x,
			   mat[0].x * a[0].y + mat[0].y * a[1].y + mat[0].z * a[2].y,
			   mat[0].x * a[0].z + mat[0].y * a[1].z + mat[0].z * a[2].z,
			   mat[1].x * a[0].x + mat[1].y * a[1].x + mat[1].z * a[2].x,
			   mat[1].x * a[0].y + mat[1].y * a[1].y + mat[1].z * a[2].y,
			   mat[1].x * a[0].z + mat[1].y * a[1].z + mat[1].z * a[2].z,
			   mat[2].x * a[0].x + mat[2].y * a[1].x + mat[2].z * a[2].x,
			   mat[2].x * a[0].y + mat[2].y * a[1].y + mat[2].z * a[2].y,
			   mat[2].x * a[0].z + mat[2].y * a[1].z + mat[2].z * a[2].z );
}

ID_INLINE mat3_t mat3_t::operator*( float a ) const {
	return mat3_t(
			   mat[0].x * a, mat[0].y * a, mat[0].z * a,
			   mat[1].x * a, mat[1].y * a, mat[1].z * a,
			   mat[2].x * a, mat[2].y * a, mat[2].z * a );
}

ID_INLINE mat3_t mat3_t::operator+( mat3_t const &a ) const {
	return mat3_t(
			   mat[0].x + a[0].x, mat[0].y + a[0].y, mat[0].z + a[0].z,
			   mat[1].x + a[1].x, mat[1].y + a[1].y, mat[1].z + a[1].z,
			   mat[2].x + a[2].x, mat[2].y + a[2].y, mat[2].z + a[2].z );
}

ID_INLINE mat3_t mat3_t::operator-( mat3_t const &a ) const {
	return mat3_t(
			   mat[0].x - a[0].x, mat[0].y - a[0].y, mat[0].z - a[0].z,
			   mat[1].x - a[1].x, mat[1].y - a[1].y, mat[1].z - a[1].z,
			   mat[2].x - a[2].x, mat[2].y - a[2].y, mat[2].z - a[2].z );
}

ID_INLINE idVec3 operator*( const idVec3 &vec, const mat3_t &mat ) {
	return idVec3(
			   mat[ 0 ].x * vec.x + mat[ 1 ].x * vec.y + mat[ 2 ].x * vec.z,
			   mat[ 0 ].y * vec.x + mat[ 1 ].y * vec.y + mat[ 2 ].y * vec.z,
			   mat[ 0 ].z * vec.x + mat[ 1 ].z * vec.y + mat[ 2 ].z * vec.z );
}

ID_INLINE mat3_t operator*( float a, mat3_t const &b ) {
	return mat3_t(
			   b[0].x * a, b[0].y * a, b[0].z * a,
			   b[1].x * a, b[1].y * a, b[1].z * a,
			   b[2].x * a, b[2].y * a, b[2].z * a );
}

ID_INLINE mat3_t &mat3_t::operator*=( float a ) {
	mat[0].x *= a; mat[0].y *= a; mat[0].z *= a;
	mat[1].x *= a; mat[1].y *= a; mat[1].z *= a;
	mat[2].x *= a; mat[2].y *= a; mat[2].z *= a;

	return *this;
}

ID_INLINE mat3_t &mat3_t::operator+=( mat3_t const &a ) {
	mat[0].x += a[0].x; mat[0].y += a[0].y; mat[0].z += a[0].z;
	mat[1].x += a[1].x; mat[1].y += a[1].y; mat[1].z += a[1].z;
	mat[2].x += a[2].x; mat[2].y += a[2].y; mat[2].z += a[2].z;

	return *this;
}

ID_INLINE mat3_t &mat3_t::operator-=( mat3_t const &a ) {
	mat[0].x -= a[0].x; mat[0].y -= a[0].y; mat[0].z -= a[0].z;
	mat[1].x -= a[1].x; mat[1].y -= a[1].y; mat[1].z -= a[1].z;
	mat[2].x -= a[2].x; mat[2].y -= a[2].y; mat[2].z -= a[2].z;

	return *this;
}

ID_INLINE void mat3_t::OrthoNormalize( void ) {
	mat[ 0 ].Normalize();
	mat[ 2 ].Cross( mat[ 0 ], mat[ 1 ] );
	mat[ 2 ].Normalize();
	mat[ 1 ].Cross( mat[ 2 ], mat[ 0 ] );
	mat[ 1 ].Normalize();
}

ID_INLINE void mat3_t::Identity( void ) {
	mat[ 0 ].x = 1.f; mat[ 0 ].y = 0.f; mat[ 0 ].z = 0.f;
	mat[ 1 ].x = 0.f; mat[ 1 ].y = 1.f; mat[ 1 ].z = 0.f;
	mat[ 2 ].x = 0.f; mat[ 2 ].y = 0.f; mat[ 2 ].z = 1.f;
}

ID_INLINE void InverseMultiply( const mat3_t &inv, const mat3_t &b, mat3_t &dst ) {
	dst[0].x = inv[0].x * b[0].x + inv[1].x * b[1].x + inv[2].x * b[2].x;
	dst[0].y = inv[0].x * b[0].y + inv[1].x * b[1].y + inv[2].x * b[2].y;
	dst[0].z = inv[0].x * b[0].z + inv[1].x * b[1].z + inv[2].x * b[2].z;
	dst[1].x = inv[0].y * b[0].x + inv[1].y * b[1].x + inv[2].y * b[2].x;
	dst[1].y = inv[0].y * b[0].y + inv[1].y * b[1].y + inv[2].y * b[2].y;
	dst[1].z = inv[0].y * b[0].z + inv[1].y * b[1].z + inv[2].y * b[2].z;
	dst[2].x = inv[0].z * b[0].x + inv[1].z * b[1].x + inv[2].z * b[2].x;
	dst[2].y = inv[0].z * b[0].y + inv[1].z * b[1].y + inv[2].z * b[2].y;
	dst[2].z = inv[0].z * b[0].z + inv[1].z * b[1].z + inv[2].z * b[2].z;
}

ID_INLINE mat3_t SkewSymmetric( idVec3 const &src ) {
	return mat3_t( 0.0f, -src.z,  src.y, src.z,   0.0f, -src.x, -src.y,  src.x,   0.0f );
}

extern mat3_t mat3_default;

#endif /* !__MATH_MATRIX_H__ */
