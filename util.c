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
along with GetHooks.  If not, see <http://www.gnu.org/licenses/>.
*/

/** 
This file contains various utility functions.
Each function is documented in the comment block above its definition.

-
must_calloc()

Must calloc() or die.
-

-
must_wcsdup()

Must _wcsdup() or die.
-

-
str_to_int()

Convert a decimal string to an integer.
-

-
get_wstr_from_mbstr()

Get a wide character string from a multibyte character string.
-

-
get_user_obj_name()

Get the name of a user object.
-

-
print_init_time()

Print an initialization utc time as local time and date.
-

-
print_time()

Print the local time and date. No newline.
-

*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <limits.h>

/* traverse_threads() */
#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"

#include "util.h"



/* must_calloc()
Must calloc() or die.

returns a pointer to the allocated memory
if allocation fails this function calls exit(1)
*/
void *must_calloc(
	const size_t num,   // in
	const size_t size   // in
)
{
	void *mem;
	
	FAIL_IF( !num && !size );
	
	
	mem = calloc( num, size );
	if( !mem )
	{
		MSG_FATAL( "calloc() failed:" );
		printf( "calloc(%Iu, %Iu)\n", num, size );
		exit( 1 );
	}
	
	return mem;
}



/* must_wcsdup()
Must _wcsdup() or die.

returns a pointer to the duplicate of the wide character string pointed to by 'strSource'
if allocation fails this function calls exit(1)
*/
WCHAR *must_wcsdup( 
	const WCHAR *const strSource   // in
)
{
	WCHAR *s = NULL;
	
	FAIL_IF( !strSource );
	
	
	s = _wcsdup( strSource );
	
	if( !s )
	{
		MSG_FATAL( "_wcsdup() failed." );
		printf( "strSource: %ls\n", strSource );
		exit( 1 );
	}
	
	return s;
}



/* str_to_int()
Convert a decimal string to an integer.

'str' is the integer representation as a string

returns nonzero on success.
if success then '*num' has received the integer.
if fail then '*num' has received INT_MAX.
*/
int str_to_int( 
	int *const num,   // out
	const char *const str   // in
)
{
	int i = 0;
	
	FAIL_IF( !num );
	FAIL_IF( !str );
	
	
	*num = INT_MAX;
	
	/* skip whitespace */
	for( i = 0; ( ( str[ i ] == ' ' ) || ( str[ i ] == '\t' ) ); ++i )
	;
	
	if( str[ i ] == '0' )
	{
		/* since we're only looking for decimal numbers if a zero is the first number 
		encountered it must be the only number encountered.
		*/
		
		/* remove trailing whitespace */
		for( ++i; ( ( str[ i ] == ' ' ) || ( str[ i ] == '\t' ) ); ++i )
		;
		
		if( !str[ i ] ) // no more chars. the decimal number is just a zero which is OK
			*num = 0;
	}
	else
	{
		/* skip sign to test for decimal */
		if( ( str[ i ] == '+' ) || ( str[ i ] == '-' ) )
			++i;
		
		/* if decimal then convert */
		if( ( str[ i ] >= '1' ) && ( str[ i ] <= '9' ) )
		{
			long x = 0;
			char *endptr = NULL;
			
			x = strtol( str, &endptr, 10 );
			
			if( endptr )
			{
				/* remove trailing whitespace */
				for( ; ( ( *endptr == ' ' ) || ( *endptr == '\t' ) ); ++endptr )
				;
			}
			
			if( ( !endptr || !*endptr ) && x && ( x > INT_MIN ) && ( x < INT_MAX ) )
				*num = (int)x;
		}
	}
	
	return ( *num != INT_MAX );
}



/* get_wstr_from_mbstr()
Get a wide character string from a multibyte character string.

'mbstr' is the multibyte character string.

returns nonzero on success.
if success then '*pwcstr' has received a pointer to the wide character string. free() when done.
if fail then '*pwcstr' has received NULL.
*/
int get_wstr_from_mbstr( 
	WCHAR **const pwcstr,   // out deref
	const char *const mbstr   // in
)
{
	size_t ecount = 0;
	
	FAIL_IF( !pwcstr );
	FAIL_IF( !mbstr );
	
	
	/* get the number of wide characters, excluding null, 
	needed to output the multibyte string as a wide character string
	*/
	ecount = mbstowcs( NULL, mbstr, 0 );
	if( ecount >= (size_t)INT_MAX ) /* error */
	{
		*pwcstr = NULL;
		return FALSE;
	}
	
	++ecount; /* add 1 for null terminator */
	
	*pwcstr = must_calloc( ecount, sizeof( WCHAR ) );
	
	ecount = mbstowcs( *pwcstr, mbstr, ecount );
	if( ecount >= (size_t)INT_MAX ) /* error */
	{
		free( *pwcstr );
		*pwcstr = NULL;
		return FALSE;
	}
	
	(*pwcstr)[ ecount ] = L'\0';
	
	return TRUE;
}



/* get_user_obj_name()
Get the name of a user object.

'object' is a handle to the user object you want the name of.

returns nonzero on success.
if success then '*name' has received a pointer to the user object's name. free() when done.
if fail then '*name' has received NULL.
if fail call GetLastError() for GetUserObjectInformationW() last error
*/
int get_user_obj_name( 
	WCHAR **const name,   // out deref
	HANDLE const object   // in
)
{
	DWORD bytes_needed = 0;
	
	FAIL_IF( !name );
	FAIL_IF( !object );
	
	
	SetLastError( 0 );
	GetUserObjectInformationW( object, UOI_NAME, NULL, 0, &bytes_needed );
	if( ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
		|| ( bytes_needed < sizeof( WCHAR ) )
	)
	{
		*name = NULL;
		return FALSE;
	}
	
	*name = must_calloc( bytes_needed, 1 );
	
	SetLastError( 0 );
	if( !GetUserObjectInformationW( object, UOI_NAME, *name, bytes_needed, NULL ) )
	{
		free( *name );
		*name = NULL;
		return FALSE;
	}
	
	(*name)[ ( bytes_needed / sizeof( WCHAR ) ) - 1 ] = L'\0';
	
	return TRUE;
}



/* print_init_time()
Print an initialization utc time as local time and date.

'msg' is an optional message to print before printing the time
'utc' is the utc time

eg print_init_time( "store->init_time", store->init_time );
store->init_time: 12:35:38 PM  9/14/2011
*/
void print_init_time(
	const char *const msg,   // in, optional
	const __int64 utc   // in
)
{
	if( msg )
		printf( "%s: ", msg );
	
	if( utc )
		print_filetime_as_local( (FILETIME *)&utc );
	else
		printf( "<uninitialized>" );
	
	printf( "\n" );
	
	return;
}



/* print_time()
Print the local time and date. No newline.
*/
void print_time( void )
{
	__int64 utc = 0;
	
	
	GetSystemTimeAsFileTime( (FILETIME *)&utc );
	print_filetime_as_local( (FILETIME *)&utc );
	
	return;
}



