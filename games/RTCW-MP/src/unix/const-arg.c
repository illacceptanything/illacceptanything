/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
http://www.eskimo.com/~scs/C-faq/q11.10.html

Question 11.10

Why can't I pass a char ** to a function which expects a const char **?

You can use a pointer-to-T (for any type T) where a pointer-to-const-T is
expected. However, the rule (an explicit exception) which permits slight
mismatches in qualified pointer types is not applied recursively, but only at
the top level.

You must use explicit casts (e.g. (const char **) in this case) when assigning
(or passing) pointers which have qualifier mismatches at other than the first
level of indirection.

References: ANSI Sec. 3.1.2.6, Sec. 3.3.16.1, Sec. 3.5.3
ISO Sec. 6.1.2.6, Sec. 6.3.16.1, Sec. 6.5.3
H&S Sec. 7.9.1 pp. 221-2
*/

#include <stdio.h>

typedef int int3_t[3];

void f00func( const int3_t thing[3] ) {
	printf( "bleh" );
}

int main( int argc, void *argv[] ) {
	int3_t foo[3];
	// warning: passing arg 1 of `f00func' from incompatible pointer type
	f00func( foo );
	// explicit casting, no warning
	f00func( (const int3_t *)foo );
	return 0;
}

