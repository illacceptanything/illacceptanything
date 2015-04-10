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

#ifndef __MATH_ANGLES_H__
#define __MATH_ANGLES_H__

#include <stdlib.h>
#include <assert.h>

#include "math_vector.h"

class mat3_t;
class quat_t;
class idVec3;
typedef idVec3 &vec3_p;

class angles_t {
public:
float pitch;
float yaw;
float roll;

angles_t();
angles_t( float pitch, float yaw, float roll );
angles_t( const idVec3 &vec );

friend void     toAngles( idVec3 &src, angles_t &dst );
friend void     toAngles( quat_t &src, angles_t &dst );
friend void     toAngles( mat3_t &src, angles_t &dst );

operator vec3_p();

float operator[]( int index ) const;
float&          operator[]( int index );

void            set( float pitch, float yaw, float roll );

void operator=( angles_t const &a );
void operator=( idVec3 const &a );

friend angles_t operator+( const angles_t &a, const angles_t &b );
angles_t        &operator+=( angles_t const &a );
angles_t        &operator+=( idVec3 const &a );

friend angles_t operator-( angles_t &a, angles_t &b );
angles_t        &operator-=( angles_t &a );

friend angles_t operator*( const angles_t &a, float b );
friend angles_t operator*( float a, const angles_t &b );
angles_t        &operator*=( float a );

friend int operator==( angles_t &a, angles_t &b );

friend int operator!=( angles_t &a, angles_t &b );

void            toVectors( idVec3 *forward, idVec3 *right = NULL, idVec3 *up = NULL );
idVec3          toForward( void );

angles_t        &Zero( void );

angles_t        &Normalize360( void );
angles_t        &Normalize180( void );
};

extern angles_t ang_zero;

inline angles_t::angles_t() {
}

inline angles_t::angles_t( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw   = yaw;
	this->roll  = roll;
}

inline angles_t::angles_t( const idVec3 &vec ) {
	this->pitch = vec.x;
	this->yaw   = vec.y;
	this->roll  = vec.z;
}

inline float angles_t::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[ index ];
}

inline float& angles_t::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[ index ];
}

inline angles_t::operator vec3_p( void ) {
	return *( idVec3 * )&pitch;
}

inline void angles_t::set( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw   = yaw;
	this->roll  = roll;
}

inline void angles_t::operator=( angles_t const &a ) {
	pitch   = a.pitch;
	yaw     = a.yaw;
	roll    = a.roll;
}

inline void angles_t::operator=( idVec3 const &a ) {
	pitch   = a[ 0 ];
	yaw     = a[ 1 ];
	roll    = a[ 2 ];
}

inline angles_t operator+( const angles_t &a, const angles_t &b ) {
	return angles_t( a.pitch + b.pitch, a.yaw + b.yaw, a.roll + b.roll );
}

inline angles_t& angles_t::operator+=( angles_t const &a ) {
	pitch   += a.pitch;
	yaw     += a.yaw;
	roll    += a.roll;

	return *this;
}

inline angles_t& angles_t::operator+=( idVec3 const &a ) {
	pitch   += a.x;
	yaw += a.y;
	roll    += a.z;

	return *this;
}

inline angles_t operator-( angles_t &a, angles_t &b ) {
	return angles_t( a.pitch - b.pitch, a.yaw - b.yaw, a.roll - b.roll );
}

inline angles_t& angles_t::operator-=( angles_t &a ) {
	pitch   -= a.pitch;
	yaw     -= a.yaw;
	roll    -= a.roll;

	return *this;
}

inline angles_t operator*( const angles_t &a, float b ) {
	return angles_t( a.pitch * b, a.yaw * b, a.roll * b );
}

inline angles_t operator*( float a, const angles_t &b ) {
	return angles_t( a * b.pitch, a * b.yaw, a * b.roll );
}

inline angles_t& angles_t::operator*=( float a ) {
	pitch   *= a;
	yaw     *= a;
	roll    *= a;

	return *this;
}

inline int operator==( angles_t &a, angles_t &b ) {
	return ( ( a.pitch == b.pitch ) && ( a.yaw == b.yaw ) && ( a.roll == b.roll ) );
}

inline int operator!=( angles_t &a, angles_t &b ) {
	return ( ( a.pitch != b.pitch ) || ( a.yaw != b.yaw ) || ( a.roll != b.roll ) );
}

inline angles_t& angles_t::Zero( void ) {
	pitch   = 0.0f;
	yaw     = 0.0f;
	roll    = 0.0f;

	return *this;
}

#endif /* !__MATH_ANGLES_H__ */
