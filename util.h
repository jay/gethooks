/*
Copyright (C) 2011 Jay Satiro <raysatiro@yahoo.com>
All rights reserved.

This file is part of GetHooks.

GetHooks is free software: you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

GetHooks is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with GetHooks.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _UTIL_H
#define _UTIL_H

#include <windows.h>
#include <limits.h>

#include "str_to_int.h"


#ifdef __cplusplus
extern "C" {
#endif


/* __pragma keyword only exists in Visual Studio 2008 or better according to documentation. 
however the keyword apparently exists undocumented in VS2005 and VS2003 as well
https://github.com/yestein/dream-of-idle/blob/4e362ab/Dream/CELayoutEditor-0.7.1/inc/Config.h#L427-L431
*/
#if !defined( _MSC_VER ) || ( _MSC_VER < 1300 ) // || ( _MSC_VER < 1500 )
#define __pragma(x)
#endif

#if defined( _MSC_VER ) && ( _MSC_VER < 1310  )
typedef uintptr_t size_t;
#endif

/* this is a basic function-like macro to print an error message and its location in code */
#define MSG_LOCATION(type,msg)   \
	do \
	{ \
		__int64 utc = 0; \
		 \
		GetSystemTimeAsFileTime( (FILETIME *)&utc ); \
		fflush( stdout ); \
		printf( "\n" ); \
		print_init_time( NULL, utc ); \
		printf( \
			"%s: %s line %d, %s(): %s\n", \
			( type ), __FILE__, __LINE__, __FUNCTION__, ( msg ) \
		); \
		fflush( stdout ); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
	} while( 0 ) \
__pragma(warning(pop))


#define MSG_LOCATION_GLE(type,msg)   \
	do \
	{ \
		DWORD gle = GetLastError(); \
		 \
		MSG_LOCATION( ( type ), ( msg ) ); \
		printf( "GetLastError(): %u\n", gle ); \
		fflush( stdout ); \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
	} while( 0 ) \
__pragma(warning(pop))



#define MSG_WARNING(msg)   MSG_LOCATION( "Warning", ( msg ) )
#define MSG_WARNING_GLE(msg)   MSG_LOCATION_GLE( "Warning", ( msg ) )

#define MSG_ERROR(msg)   MSG_LOCATION( "Error", ( msg ) )
#define MSG_ERROR_GLE(msg)   MSG_LOCATION_GLE( "Error", ( msg ) )

#define MSG_FATAL(msg)   MSG_LOCATION( "FATAL", ( msg ) )
#define MSG_FATAL_GLE(msg)   MSG_LOCATION_GLE( "FATAL", ( msg ) )


#define FAIL_IF(expr)   \
	do \
	{ \
		if( expr ) \
		{ \
			MSG_FATAL( "A parameter or expression failed validation." ); \
			printf( "The following expression is true: ( " #expr " )\n" ); \
			fflush( stdout ); \
			exit( 1 ); \
		} \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
	} while( 0 ) \
__pragma(warning(pop))


#define PRINT_HEX_BARE(addr)   \
	do \
	{ \
		size_t bytes = sizeof( addr ); \
		 \
		if( bytes == sizeof(BYTE) ) \
			printf( "0x%0*I64X", (int)( sizeof( BYTE ) * 2 ), (UINT64)(*(BYTE *)&addr ) ); \
		else if( bytes == sizeof(WORD) ) \
			printf( "0x%0*I64X", (int)( sizeof( WORD ) * 2 ), (UINT64)(*(WORD *)&addr ) ); \
		else if( bytes == sizeof(DWORD) ) \
			printf( "0x%0*I64X", (int)( sizeof( DWORD ) * 2 ), (UINT64)(*(DWORD *)&addr ) ); \
		else if( bytes == sizeof(UINT64) ) \
			printf( "0x%0*I64X", (int)( sizeof( UINT64 ) * 2 ), (UINT64)(*(UINT64 *)&addr ) ); \
		else \
			printf( "<unimplemented>" ); \
		 \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
	} while( 0 ) \
__pragma(warning(pop))

#define PRINT_HEX_NAME(name,addr)   \
	do \
	{ \
		const char *str = NULL; \
		str = ( name ); \
		printf( "%s%s", ( str ? str : "" ), ( ( str && str[ 0 ] ) ? ": " : "" ) ); \
		PRINT_HEX_BARE( ( addr ) ); \
		printf( "\n" ); \
		 \
__pragma(warning(push)) \
__pragma(warning(disable:4127)) \
	} while( 0 ) \
__pragma(warning(pop))

#define PRINT_HEX(addr)   PRINT_HEX_NAME( #addr, ( addr ) )


#define PRINT_SEP_BEGIN(msg)   \
	printf( "\n--------------------------- [begin] %s\n", ( msg ) )

#define PRINT_HASHSEP_BEGIN(msg)   \
	printf( "\n########################### [begin] %s\n", ( msg ) )

#define PRINT_DBLSEP_BEGIN(msg)   \
	printf( "\n=========================== [begin] %s\n", ( msg ) )


#define PRINT_SEP_END(msg)   \
	( printf( "--------------------------- [end] %s\n", ( msg ) ), fflush( stdout ) )

#define PRINT_HASHSEP_END(msg)   \
	( printf( "########################### [end] %s\n", ( msg ) ), fflush( stdout ) )

#define PRINT_DBLSEP_END(msg)   \
	( printf( "=========================== [end] %s\n", ( msg ) ), fflush( stdout ) )



/** 
these functions are documented in the comment block above their definitions in util.c
*/
void *must_calloc(
	const size_t num,   // in
	const size_t size   // in
);

WCHAR *must_wcsdup( 
	const WCHAR *const strSource   // in
);

int get_wstr_from_mbstr( 
	WCHAR **const pwcstr,   // out deref
	const char *const mbstr   // in
);

int get_user_obj_name( 
	WCHAR **const name,   // out deref
	HANDLE const object   // in
);

void print_init_time(
	const char *const msg,   // in, optional
	const __int64 utc   // in
);

void print_time( void );


#ifdef __cplusplus
}
#endif

#endif // _UTIL_H
