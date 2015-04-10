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


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_log.h"

typedef enum {false, true}  qboolean;

//#define PATHSEPERATOR_STR		"\\"

void Error( char *error, ... ) {
	va_list argptr;

	va_start( argptr, error );
	vprintf( error, argptr );
	va_end( argptr );

	exit( 1 );
}

/*
int FileLength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
} //end of the function FileLength

void Remove(char *buf, int length, char *from, char *to, char *skip)
{
	int i, remove = false;

	for (i = 0; i < length; i++)
	{
		if (remove)
		{
			if ((unsigned) length - i > strlen(skip))
			{
				if (!strncmp(&buf[i], skip, strlen(skip)))
				{
					i += strlen(skip);
				} //end if
			} //end if
			if ((unsigned) length - i > strlen(to))
			{
				if (!strncmp(&buf[i], to, strlen(to)))
				{
					length = i + strlen(to);
				} //end if
			} //end if
			if (buf[i]) buf[i] = 'a';
		} //end if
		else
		{
			if ((unsigned) length - i < strlen(from)) return;
			if (!strncmp(&buf[i], from, strlen(from))) remove = true;
		} //end else
	} //end for
} //end of the function Remove

void main(int argc, char *argv[])
{
	FILE *fp;
	int filelength;
	char *from, *to, *skip, *ptr;

	if (argc < 2) Error("USAGE: screwup <infile> <outfile> <from> <to>");
	fp = fopen(argv[1], "rb");
	if (!fp) Error("error opening %s\n", argv[1]);

	filelength = FileLength(fp);
	ptr = malloc(filelength);
	fread(ptr, filelength, 1, fp);
	fclose(fp);

	from = argv[3];//"be_aas_bspq2.c";
	to = argv[4];//"BotWeaponNameFromModel";
	skip = "GetBotAPI";

	Remove(ptr, filelength, from, to, skip);

	fp = fopen(argv[2], "wb");
	if (!fp) Error("error opening %s\n", argv[2]);
	fwrite(ptr, filelength, 1, fp);
	fclose(fp);

	free(ptr);
} //end of the function main
*/

typedef struct replacefunc_s
{
	char *name;
	char *newname;
	char *filename;
	char dec[MAX_TOKEN];            //function declaration
	struct replacefunc_s *next;
} replacefunc_t;

replacefunc_t *replacefuncs;
int numfuncs;

extern int Q_stricmp( const char *s1, const char *s2 );

// the function names
//#define DEFAULT_FUNCBASE "g_func"
static char *func_filename = "g_funcs.h";
static char *func_filedesc = "g_func_decs.h";

void DumpReplaceFunctions( void ) {
	replacefunc_t *rf;
	char path[_MAX_PATH];
	FILE    *f;
	int len, newlen;
	unsigned char *buf, *newbuf;
	int updated;

	updated = 0;

	// dump the function header
	strcpy( path, "." );
	strcat( path, PATHSEPERATOR_STR );
	strcat( path, "g_funcs.tmp" );
	Log_Open( path );
	for ( rf = replacefuncs; rf; rf = rf->next )
	{
		Log_Print( "{\"%s\", (byte *)%s},\n", rf->name, rf->name );
	} //end for
	Log_Print( "{0, 0}\n" );
	Log_Close();

	// if it's different, rename the file over the real header
	strcpy( path, "g_funcs.tmp" );
	f = fopen( path, "rb" );
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	buf = (unsigned char *) malloc( len + 1 );
	fseek( f, 0, SEEK_SET );
	fread( buf, len, 1, f );
	buf[len] = 0;
	fclose( f );

	strcpy( path, func_filename );
	if ( f = fopen( path, "rb" ) ) {
		fseek( f, 0, SEEK_END );
		newlen = ftell( f );
		newbuf = (unsigned char *) malloc( newlen + 1 );
		fseek( f, 0, SEEK_SET );
		fread( newbuf, newlen, 1, f );
		newbuf[newlen] = 0;
		fclose( f );

		if ( len != newlen || Q_stricmp( buf, newbuf ) ) {
			char newpath[_MAX_PATH];

			// delete the old file, rename the new one
			strcpy( path, func_filename );
			remove( path );

			strcpy( newpath, "g_funcs.tmp" );
			rename( newpath, path );

			// make g_save recompile itself
			remove( "debug\\g_save.obj" );
			remove( "debug\\g_save.sbr" );
			remove( "release\\g_save.obj" );
			remove( "release\\g_save.sbr" );

			updated = 1;
		} else {
			// delete the old file
			strcpy( path, "g_funcs.tmp" );
			remove( path );
		}
	} else {
		rename( "g_funcs.tmp", func_filename );
	}

	free( buf );
	free( newbuf );

	// dump the function declarations
	strcpy( path, "g_func_decs.tmp" );
	Log_Open( path );
	for ( rf = replacefuncs; rf; rf = rf->next )
	{
		Log_Print( "extern %s;\n", rf->dec );
	} //end for
	Log_Close();

	// if it's different, rename the file over the real header
	strcpy( path, "g_func_decs.tmp" );
	f = fopen( path, "rb" );
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	buf = (unsigned char *) malloc( len + 1 );
	fseek( f, 0, SEEK_SET );
	fread( buf, len, 1, f );
	buf[len] = 0;
	fclose( f );

	strcpy( path, func_filedesc );
	if ( f = fopen( path, "rb" ) ) {
		fseek( f, 0, SEEK_END );
		newlen = ftell( f );
		newbuf = (unsigned char *) malloc( newlen + 1 );
		fseek( f, 0, SEEK_SET );
		fread( newbuf, newlen, 1, f );
		newbuf[newlen] = 0;
		fclose( f );

		if ( len != newlen || Q_stricmp( buf, newbuf ) ) {
			char newpath[_MAX_PATH];

			// delete the old file, rename the new one
			strcpy( path, func_filedesc );
			remove( path );

			strcpy( newpath, "g_func_decs.tmp" );
			rename( newpath, path );

			// make g_save recompile itself
			// NOTE TTimo win32 only? (harmless on *nix anyway)
			remove( "debug\\g_save.obj" );
			remove( "debug\\g_save.sbr" );
			remove( "release\\g_save.obj" );
			remove( "release\\g_save.sbr" );

			updated = 1;
		} else {
			// delete the old file
			strcpy( path, "g_func_decs.tmp" );
			remove( path );
		}
	} else {
		rename( "g_func_decs.tmp", func_filedesc );
	}

	free( buf );
	free( newbuf );

	if ( updated ) {
		printf( "Updated the function table, recompile required.\n" );
	}

} //end of the function DumpReplaceFunctions

replacefunc_t *FindFunctionName( char *funcname ) {
	replacefunc_t *f;

	for ( f = replacefuncs; f; f = f->next )
	{
		if ( !strcmp( f->name, funcname ) ) {
			return f;
		}
	} //end for
	return NULL;
} //end of the function FindFunctionName

int MayScrewUp( char *funcname ) {
	if ( !strcmp( funcname, "GetBotAPI" ) ) {
		return false;
	}
	if ( !strcmp( funcname, "main" ) ) {
		return false;
	}
	if ( !strcmp( funcname, "WinMain" ) ) {
		return false;
	}
	return true;
} //end of the function MayScrewUp

typedef struct tokenList_s {
	token_t token;
	struct tokenList_s *next;
} tokenList_t;

#define MAX_TOKEN_LIST  64
tokenList_t tokenList[MAX_TOKEN_LIST];
int tokenListHead = 0;

void ConcatDec( tokenList_t *list, char *str, int inc ) {
/*
	if (!((list->token.type == TT_NAME) || (list->token.string[0] == '*'))) {
		if (list->token.string[0] == ')' || list->token.string[0] == '(') {
			if (inc++ >= 2)
				return;
		} else {
			return;
		}
	}
*/
	if ( list->next ) {
		ConcatDec( list->next, str, inc );
	}
	strcat( str, list->token.string );
	strcat( str, " " );
}

void AddFunctionName( char *funcname, char *filename, tokenList_t *head ) {
	replacefunc_t *f;
	tokenList_t     *list;

	if ( FindFunctionName( funcname ) ) {
		return;
	}

#if defined( __linux__ )
	// the bad thing is, this doesn't preprocess .. on __linux__ this
	// function is not implemented (q_math.c)
	if ( !Q_stricmp( funcname, "BoxOnPlaneSide" ) ) {
		return;
	}
#endif

	// NERVE - SMF - workaround for Graeme's predifined MACOSX functions
	// TTimo - looks like linux version needs to escape those too
#if defined( _WIN32 ) || defined( __linux__ )
	if ( !Q_stricmp( funcname, "qmax" ) ) {
		return;
	} else if ( !Q_stricmp( funcname, "qmin" ) ) {
		return;
	}
#endif
	// -NERVE - SMF

	f = (replacefunc_t *) GetMemory( sizeof( replacefunc_t ) + strlen( funcname ) + 1 + 6 + strlen( filename ) + 1 );
	f->name = (char *) f + sizeof( replacefunc_t );
	strcpy( f->name, funcname );
	f->newname = (char *) f + sizeof( replacefunc_t ) + strlen( funcname ) + 1;
	sprintf( f->newname, "F%d", numfuncs++ );
	f->filename = (char *) f + sizeof( replacefunc_t ) + strlen( funcname ) + 1 + strlen( f->newname ) + 1;
	strcpy( f->filename, filename );
	f->next = replacefuncs;
	replacefuncs = f;

	// construct the declaration
	list = head;
	f->dec[0] = '\0';
	ConcatDec( list, f->dec, 0 );

} //end of the function AddFunctionName

void AddTokenToList( tokenList_t **head, token_t *token ) {
	tokenList_t *newhead;

	newhead = &tokenList[tokenListHead++]; //GetMemory( sizeof( tokenList_t ) );
	if ( tokenListHead == MAX_TOKEN_LIST ) {
		tokenListHead = 0;
	}

	newhead->next = *head;
	newhead->token = *token;

	*head = newhead;
}
/*
void KillTokenList( tokenList_t *head )
{
	if (head->next) {
		KillTokenList( head->next );
		FreeMemory( head->next );
		head->next = NULL;
	}
}
*/
void StripTokenList( tokenList_t *head ) {
	tokenList_t *trav, *lastTrav;

	trav = head;

	// now go back to the start of the declaration
	lastTrav = trav;
	trav = trav->next;  // should be on the function name now
	while ( ( trav->token.type == TT_NAME ) || ( trav->token.string[0] == '*' ) ) {
		lastTrav = trav;
		trav = trav->next;
		if ( !trav ) {
			return;
		}
	}
	// now kill everything after lastTrav
//	KillTokenList( lastTrav );
	lastTrav->next = NULL;
}

void GetFunctionNamesFromFile( char *filename ) {
	source_t *source;
	token_t token, lasttoken;
	int indent = 0, brace;
	int isStatic = 0;
	tokenList_t *listHead;

	// filter some files out
	if ( !Q_stricmp( filename, "bg_lib.c" ) ) {
		return;
	}

	listHead = NULL;
	source = LoadSourceFile( filename );
	if ( !source ) {
		Error( "error opening %s", filename );
		return;
	} //end if
//	printf("loaded %s\n", filename);
//	if (!PC_ReadToken(source, &lasttoken))
//	{
//		FreeSource(source);
//		return;
//	} //end if
	while ( 1 )
	{
		if ( !PC_ReadToken( source, &token ) ) {
			break;
		}
		AddTokenToList( &listHead, &token );
		if ( token.type == TT_PUNCTUATION ) {
			switch ( token.string[0] )
			{
			case ';':
			{
				isStatic = 0;
				break;
			}
			case '{':
			{
				indent++;
				break;
			}     //end case
			case '}':
			{
				indent--;
				if ( indent < 0 ) {
					indent = 0;
				}
				break;
			}     //end case
			case '(':
			{
				if ( indent <= 0 && lasttoken.type == TT_NAME ) {
					StripTokenList( listHead );

					brace = 1;
					while ( PC_ReadToken( source, &token ) )
					{
						AddTokenToList( &listHead, &token );
						if ( token.string[0] == '(' ) {
							brace++;
						}     //end if
						else if ( token.string[0] == ')' ) {
							brace--;
							if ( brace <= 0 ) {
								if ( !PC_ReadToken( source, &token ) ) {
									break;
								}
								if ( token.string[0] == '{' ) {
									indent++;
									if ( !isStatic && MayScrewUp( lasttoken.string ) ) {
										AddFunctionName( lasttoken.string, filename, listHead );
									}     //end if
								}     //end if
								break;
							}     //end if
						}     //end if
					}     //end while
				}     //end if
				break;
			}     //end case
			} //end if
		} //end switch
		if ( token.type == TT_NAME ) {
			if ( token.string[0] == 's' && !strcmp( token.string, "static" ) ) {
				isStatic = 1;
			}
		}
		memcpy( &lasttoken, &token, sizeof( token_t ) );
	} //end while
	FreeSource( source );
} //end of the function GetFunctionNamesFromFile

void WriteWhiteSpace( FILE *fp, script_t *script ) {
	int c;
	//write out the white space
	c = PS_NextWhiteSpaceChar( script );
	while ( c )
	{
		//NOTE: do NOT write out carriage returns (for unix/linux compatibility
		if ( c != 13 ) {
			fputc( c, fp );
		}
		c = PS_NextWhiteSpaceChar( script );
	} //end while
} //end of the function WriteWhiteSpace

void WriteString( FILE *fp, script_t *script ) {
	char *ptr;

	ptr = script->endwhitespace_p;
	while ( ptr < script->script_p )
	{
		fputc( *ptr, fp );
		ptr++;
	} //end while
} //end of the function WriteString

void ScrewUpFile( char *oldfile, char *newfile ) {
	FILE *fp;
	script_t *script;
	token_t token;
	replacefunc_t *f;
	char *ptr;

	printf( "screwing up file %s\n", oldfile );
	script = LoadScriptFile( oldfile );
	if ( !script ) {
		Error( "error opening %s\n", oldfile );
	}
	fp = fopen( newfile, "wb" );
	if ( !fp ) {
		Error( "error opening %s\n", newfile );
	}
	//
	while ( PS_ReadToken( script, &token ) )
	{
		WriteWhiteSpace( fp, script );
		if ( token.type == TT_NAME ) {
			f = FindFunctionName( token.string );
			if ( f ) {
				ptr = f->newname;
			} else { ptr = token.string;}
			while ( *ptr )
			{
				fputc( *ptr, fp );
				ptr++;
			} //end while
		} //end if
		else
		{
			WriteString( fp, script );
		} //end else
	} //end while
	WriteWhiteSpace( fp, script );
	FreeMemory( script );
	fclose( fp );
} //end of the function ScrewUpFile

int verbose = 0;

#ifdef _WIN32

void main( int argc, char *argv[] ) {
	WIN32_FIND_DATA filedata;
	HWND handle;
	int done; //, i;

	if ( argc < 2 ) {
		Error( "USAGE: screwup <file filter>\n" );
	} //end if

	handle = FindFirstFile( argv[1], &filedata );
	done = ( handle == INVALID_HANDLE_VALUE );
	while ( !done )
	{
		if ( !( filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
			//
			GetFunctionNamesFromFile( filedata.cFileName );
		} //end if
		  //find the next file
		done = !FindNextFile( handle, &filedata );
	} //end while
	DumpReplaceFunctions();
} //end of the function main

#else

void Usage() {
	Error( "USAGE: screwup  [-o <funcs> <func_desc>] <file1> [<file2> ..]\n"
		   "no -o defaults to g_funcs.h g_func_decs.h\n" );
}

/*
*nix version, let the shell do the pattern matching
(that's what shells are for :-))
*/
int main( int argc, char *argv[] ) {
	int i;
	int argbase = 1;

	if ( argc < 2 ) {
		Usage();
	} //end if

	if ( !Q_stricmp( argv[1],"-o" ) ) {
		if ( argc < 5 ) {
			Usage();
		}
		func_filename = argv[2];
		func_filedesc = argv[3];
		argbase = 4;
	}

	for ( i = argbase; i < argc; i++ )
	{
		printf( "%d: %s\n", i, argv[i] );
		GetFunctionNamesFromFile( argv[i] );
	}
	DumpReplaceFunctions();
}

#endif
