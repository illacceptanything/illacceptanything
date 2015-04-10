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

#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__

#if defined( _WIN32 )
#pragma warning(disable : 4244)
#endif

#include <math.h>
#include <assert.h>

//#define DotProduct(a,b)			((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
//#define VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
//#define VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
//#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
//#define VectorCopy(a,b)			((b).x=(a).x,(b).y=(a).y,(b).z=(a).z])

//#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define __VectorMA( v, s, b, o )  ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ) )
//#define CrossProduct(a,b,c)		((c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],(c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],(c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])

#define DotProduct4( x,y )        ( ( x )[0] * ( y )[0] + ( x )[1] * ( y )[1] + ( x )[2] * ( y )[2] + ( x )[3] * ( y )[3] )
#define VectorSubtract4( a,b,c )  ( ( c )[0] = ( a )[0] - ( b )[0],( c )[1] = ( a )[1] - ( b )[1],( c )[2] = ( a )[2] - ( b )[2],( c )[3] = ( a )[3] - ( b )[3] )
#define VectorAdd4( a,b,c )       ( ( c )[0] = ( a )[0] + ( b )[0],( c )[1] = ( a )[1] + ( b )[1],( c )[2] = ( a )[2] + ( b )[2],( c )[3] = ( a )[3] + ( b )[3] )
#define VectorCopy4( a,b )        ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2],( b )[3] = ( a )[3] )
#define VectorScale4( v, s, o )   ( ( o )[0] = ( v )[0] * ( s ),( o )[1] = ( v )[1] * ( s ),( o )[2] = ( v )[2] * ( s ),( o )[3] = ( v )[3] * ( s ) )
#define VectorMA4( v, s, b, o )   ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ),( o )[3] = ( v )[3] + ( b )[3] * ( s ) )


//#define VectorClear(a)			((a)[0]=(a)[1]=(a)[2]=0)
#define VectorNegate( a,b )       ( ( b )[0] = -( a )[0],( b )[1] = -( a )[1],( b )[2] = -( a )[2] )
//#define VectorSet(v, x, y, z)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
#define Vector4Copy( a,b )        ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2],( b )[3] = ( a )[3] )

#define SnapVector( v ) {v[0] = (int)v[0]; v[1] = (int)v[1]; v[2] = (int)v[2];}


//#include "util_heap.h"

#ifndef EQUAL_EPSILON
#define EQUAL_EPSILON   0.001
#endif

float Q_fabs( float f );

#ifndef ID_INLINE
#ifdef _WIN32
#define ID_INLINE __inline
#else
#define ID_INLINE inline
#endif
#endif

// if this is defined, vec3 will take four elements, which may allow
// easier SIMD optimizations
//#define	FAT_VEC3
//#ifdef __ppc__
//#pragma align(16)
//#endif

class angles_t;
#ifdef __ppc__
// Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
// runs *much* faster than calling sqrt(). We'll use two Newton-Raphson
// refinement steps to get bunch more precision in the 1/sqrt() value for very little cost.
// We'll then multiply 1/sqrt times the original value to get the sqrt.
// This is about 12.4 times faster than sqrt() and according to my testing (not exhaustive)
// it returns fairly accurate results (error below 1.0e-5 up to 100000.0 in 0.1 increments).

static inline float idSqrt( float x ) {
	const float half = 0.5;
	const float one = 1.0;
	float B, y0, y1;

	// This'll NaN if it hits frsqrte. Handle both +0.0 and -0.0
	if ( fabs( x ) == 0.0 ) {
		return x;
	}
	B = x;

#ifdef __GNUC__
	asm ( "frsqrte %0,%1" : "=f" ( y0 ) : "f" ( B ) );
#else
	y0 = __frsqrte( B );
#endif
	/* First refinement step */

	y1 = y0 + half * y0 * ( one - B * y0 * y0 );

	/* Second refinement step -- copy the output of the last step to the input of this step */

	y0 = y1;
	y1 = y0 + half * y0 * ( one - B * y0 * y0 );

	/* Get sqrt(x) from x * 1/sqrt(x) */
	return x * y1;
}
#else
static inline double idSqrt( double x ) {
	return sqrt( x );
}
#endif


//class idVec3  : public idHeap<idVec3> {
class idVec3 {
public:
#ifndef FAT_VEC3
	float x,y,z;
#else
	float x,y,z,dist;
#endif

#ifndef FAT_VEC3
	idVec3() {
	};
#else
	idVec3() {
		dist = 0.0f;
	};
#endif
	idVec3( const float x, const float y, const float z );

	operator float*();

	float operator[]( const int index ) const;
	float           &operator[]( const int index );

	void            set( const float x, const float y, const float z );

	idVec3 operator-() const;

	idVec3          &operator=( const idVec3 &a );

	float operator*( const idVec3 &a ) const;
	idVec3 operator*( const float a ) const;
	friend idVec3 operator*( float a, idVec3 b );

	idVec3 operator+( const idVec3 &a ) const;
	idVec3 operator-( const idVec3 &a ) const;

	idVec3          &operator+=( const idVec3 &a );
	idVec3          &operator-=( const idVec3 &a );
	idVec3          &operator*=( const float a );

	int operator==( const idVec3 &a ) const;
	int operator!=( const idVec3 &a ) const;

	idVec3          Cross( const idVec3 &a ) const;
	idVec3          &Cross( const idVec3 &a, const idVec3 &b );

	float           Length( void ) const;
	float           Normalize( void );

	void            Zero( void );
	void            Snap( void );
	void            SnapTowards( const idVec3 &to );

	float           toYaw( void );
	float           toPitch( void );
	angles_t        toAngles( void );
	friend idVec3   LerpVector( const idVec3 &w1, const idVec3 &w2, const float t );

	char            *string( void );
};

extern idVec3 vec_zero;

ID_INLINE idVec3::idVec3( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
#ifdef  FAT_VEC3
	this->dist = 0.0f;
#endif
}

ID_INLINE float idVec3::operator[]( const int index ) const {
	return ( &x )[ index ];
}

ID_INLINE float &idVec3::operator[]( const int index ) {
	return ( &x )[ index ];
}

ID_INLINE idVec3::operator float*( void ) {
	return &x;
}

ID_INLINE idVec3 idVec3::operator-() const {
	return idVec3( -x, -y, -z );
}

ID_INLINE idVec3 &idVec3::operator=( const idVec3 &a ) {
	x = a.x;
	y = a.y;
	z = a.z;

	return *this;
}

ID_INLINE void idVec3::set( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

ID_INLINE idVec3 idVec3::operator-( const idVec3 &a ) const {
	return idVec3( x - a.x, y - a.y, z - a.z );
}

ID_INLINE float idVec3::operator*( const idVec3 &a ) const {
	return x * a.x + y * a.y + z * a.z;
}

ID_INLINE idVec3 idVec3::operator*( const float a ) const {
	return idVec3( x * a, y * a, z * a );
}

ID_INLINE idVec3 operator*( const float a, const idVec3 b ) {
	return idVec3( b.x * a, b.y * a, b.z * a );
}

ID_INLINE idVec3 idVec3::operator+( const idVec3 &a ) const {
	return idVec3( x + a.x, y + a.y, z + a.z );
}

ID_INLINE idVec3 &idVec3::operator+=( const idVec3 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;

	return *this;
}

ID_INLINE idVec3 &idVec3::operator-=( const idVec3 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;

	return *this;
}

ID_INLINE idVec3 &idVec3::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;

	return *this;
}

ID_INLINE int idVec3::operator==( const idVec3 &a ) const {
	if ( Q_fabs( x - a.x ) > EQUAL_EPSILON ) {
		return false;
	}

	if ( Q_fabs( y - a.y ) > EQUAL_EPSILON ) {
		return false;
	}

	if ( Q_fabs( z - a.z ) > EQUAL_EPSILON ) {
		return false;
	}

	return true;
}

ID_INLINE int idVec3::operator!=( const idVec3 &a ) const {
	if ( Q_fabs( x - a.x ) > EQUAL_EPSILON ) {
		return true;
	}

	if ( Q_fabs( y - a.y ) > EQUAL_EPSILON ) {
		return true;
	}

	if ( Q_fabs( z - a.z ) > EQUAL_EPSILON ) {
		return true;
	}

	return false;
}

ID_INLINE idVec3 idVec3::Cross( const idVec3 &a ) const {
	return idVec3( y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x );
}

ID_INLINE idVec3 &idVec3::Cross( const idVec3 &a, const idVec3 &b ) {
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;

	return *this;
}

ID_INLINE float idVec3::Length( void ) const {
	float length;

	length = x * x + y * y + z * z;
	return ( float )idSqrt( length );
}

ID_INLINE float idVec3::Normalize( void ) {
	float length;
	float ilength;

	length = this->Length();
	if ( length ) {
		ilength = 1.0f / length;
		x *= ilength;
		y *= ilength;
		z *= ilength;
	}

	return length;
}

ID_INLINE void idVec3::Zero( void ) {
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

ID_INLINE void idVec3::Snap( void ) {
	x = float( int( x ) );
	y = float( int( y ) );
	z = float( int( z ) );
}

/*
======================
SnapTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
ID_INLINE void idVec3::SnapTowards( const idVec3 &to ) {
	if ( to.x <= x ) {
		x = float( int( x ) );
	} else {
		x = float( int( x ) + 1 );
	}

	if ( to.y <= y ) {
		y = float( int( y ) );
	} else {
		y = float( int( y ) + 1 );
	}

	if ( to.z <= z ) {
		z = float( int( z ) );
	} else {
		z = float( int( z ) + 1 );
	}
}

//===============================================================

class Bounds {
public:
	idVec3 b[2];

	Bounds();
	Bounds( const idVec3 &mins, const idVec3 &maxs );

	void    Clear();
	void    Zero();
	float   Radius();       // radius from origin, not from center
	idVec3  Center();
	void    AddPoint( const idVec3 &v );
	void    AddBounds( const Bounds &bb );
	bool    IsCleared();
	bool    ContainsPoint( const idVec3 &p );
	bool    IntersectsBounds( const Bounds &b2 );   // touching is NOT intersecting
};

extern Bounds boundsZero;

ID_INLINE Bounds::Bounds() {
}

ID_INLINE bool Bounds::IsCleared() {
	return b[0][0] > b[1][0];
}

ID_INLINE bool Bounds::ContainsPoint( const idVec3 &p ) {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2]
		 || p[0] > b[1][0] || p[1] > b[1][1] || p[2] > b[1][2] ) {
		return false;
	}
	return true;
}

ID_INLINE bool Bounds::IntersectsBounds( const Bounds &b2 ) {
	if ( b2.b[1][0] < b[0][0] || b2.b[1][1] < b[0][1] || b2.b[1][2] < b[0][2]
		 || b2.b[0][0] > b[1][0] || b2.b[0][1] > b[1][1] || b2.b[0][2] > b[1][2] ) {
		return false;
	}
	return true;
}

ID_INLINE Bounds::Bounds( const idVec3 &mins, const idVec3 &maxs ) {
	b[0] = mins;
	b[1] = maxs;
}

ID_INLINE idVec3 Bounds::Center() {
	return idVec3( ( b[1][0] + b[0][0] ) * 0.5f, ( b[1][1] + b[0][1] ) * 0.5f, ( b[1][2] + b[0][2] ) * 0.5f );
}

ID_INLINE void Bounds::Clear() {
	b[0][0] = b[0][1] = b[0][2] = 99999;
	b[1][0] = b[1][1] = b[1][2] = -99999;
}

ID_INLINE void Bounds::Zero() {
	b[0][0] = b[0][1] = b[0][2] =
							b[1][0] = b[1][1] = b[1][2] = 0;
}

ID_INLINE void Bounds::AddPoint( const idVec3 &v ) {
	if ( v[0] < b[0][0] ) {
		b[0][0] = v[0];
	}
	if ( v[0] > b[1][0] ) {
		b[1][0] = v[0];
	}
	if ( v[1] < b[0][1] ) {
		b[0][1] = v[1];
	}
	if ( v[1] > b[1][1] ) {
		b[1][1] = v[1];
	}
	if ( v[2] < b[0][2] ) {
		b[0][2] = v[2];
	}
	if ( v[2] > b[1][2] ) {
		b[1][2] = v[2];
	}
}


ID_INLINE void Bounds::AddBounds( const Bounds &bb ) {
	if ( bb.b[0][0] < b[0][0] ) {
		b[0][0] = bb.b[0][0];
	}
	if ( bb.b[0][1] < b[0][1] ) {
		b[0][1] = bb.b[0][1];
	}
	if ( bb.b[0][2] < b[0][2] ) {
		b[0][2] = bb.b[0][2];
	}

	if ( bb.b[1][0] > b[1][0] ) {
		b[1][0] = bb.b[1][0];
	}
	if ( bb.b[1][1] > b[1][1] ) {
		b[1][1] = bb.b[1][1];
	}
	if ( bb.b[1][2] > b[1][2] ) {
		b[1][2] = bb.b[1][2];
	}
}

ID_INLINE float Bounds::Radius() {
	int i;
	float total;
	float a, aa;

	total = 0;
	for ( i = 0 ; i < 3 ; i++ ) {
		a = (float)fabs( b[0][i] );
		aa = (float)fabs( b[1][i] );
		if ( aa > a ) {
			a = aa;
		}
		total += a * a;
	}

	return (float)idSqrt( total );
}

//===============================================================


class idVec2 {
public:
	float x;
	float y;

	operator float*();
	float operator[]( int index ) const;
	float           &operator[]( int index );
};

ID_INLINE float idVec2::operator[]( int index ) const {
	return ( &x )[ index ];
}

ID_INLINE float& idVec2::operator[]( int index ) {
	return ( &x )[ index ];
}

ID_INLINE idVec2::operator float*( void ) {
	return &x;
}

class idVec4 : public idVec3 {
public:
#ifndef FAT_VEC3
	float dist;
#endif
	idVec4();
	~idVec4() {
	};

	idVec4( float x, float y, float z, float dist );
	float operator[]( int index ) const;
	float           &operator[]( int index );
};

ID_INLINE idVec4::idVec4() {
}
ID_INLINE idVec4::idVec4( float x, float y, float z, float dist ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->dist = dist;
}

ID_INLINE float idVec4::operator[]( int index ) const {
	return ( &x )[ index ];
}

ID_INLINE float& idVec4::operator[]( int index ) {
	return ( &x )[ index ];
}


class idVec5_t : public idVec3 {
public:
	float s;
	float t;
	float operator[]( int index ) const;
	float           &operator[]( int index );
};


ID_INLINE float idVec5_t::operator[]( int index ) const {
	return ( &x )[ index ];
}

ID_INLINE float& idVec5_t::operator[]( int index ) {
	return ( &x )[ index ];
}

#endif /* !__MATH_VECTOR_H__ */
