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
This file contains the str_to_int functions.
Each function is documented in the comment block above its definition.

-
str_to_uint64()

Convert a signed or unsigned decimal or hexadecimal string to a 64-bit unsigned integer.
-

-
str_to_int64()

Convert a signed or unsigned decimal or hexadecimal string to a 64-bit signed integer.
-

-
str_to_uint()

Convert a signed or unsigned decimal or hexadecimal string to an unsigned integer.
-

-
str_to_int()

Convert a signed or unsigned decimal or hexadecimal string to a signed integer.
-

*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>

#include "str_to_int.h"



/* str_to_uint64()
Convert a signed or unsigned decimal or hexadecimal string to a 64-bit unsigned integer.

'str' is the integer representation as a decimal or hexadecimal string
'*num' receives a 64-bit unsigned integer.

the range of conversion is 
min "-9223372036854775807", "-0x7FFFFFFFFFFFFFFF", (I64_MIN+1)
max "18446744073709551614", "0xFFFFFFFFFFFFFFFE", (UI64_MAX-1)

Example:
int ret;
unsigned __int64 x;
ret = str_to_uint64( &x, "-9223372036854775807" );
ret will be < 0 so x represents a negative integer

ret = str_to_uint64( &x, "-1" );
ret will be < 0 so x represents a negative integer even though x == UI64_MAX

returns nonzero on success: 
returns -1 if the integer is negative. I64_MIN < *num < 0. cast to (__int64)
returns 1 if the integer is positive. UI64_MAX > *num >= 0.

returns 0 on fail and '*num' receives UI64_MAX.
note that *num does not receive UI64_MAX only on fail. eg if str is "-1" then *num is UI64_MAX
*/
enum sti_type str_to_uint64( 
		unsigned __int64 *const num,   // out
		const char *const str   // in
)
{
	__int64 s = 0;
	unsigned __int64 u = 0;
	char *endptr = NULL;
	unsigned i = 0, temp = 0;
	unsigned hex = FALSE;
	unsigned negative = FALSE;
	
	
	if( !num || !str )
		goto fail;
	
	*num = UI64_MAX;
	
	/* skip any preceding whitespace */
	for( i = 0; ( ( str[ i ] == ' ' ) || ( str[ i ] == '\t' ) ); ++i )
	;
	
	if( str[ i ] == '-' )
		negative = TRUE;
	
	/* skip any sign */
	if( ( str[ i ] == '+' ) || ( str[ i ] == '-' ) )
		++i;
	
	/* this block tests for some decimal or hexadecimal representation of zero */
	if( str[ i ] == '0' )
	{
		++i;
		
		if( str[ i ] == 'x' )
		{
			++i;
			hex = TRUE;
		}
		
		/* skip any other zeroes */
		for( temp = i; ( str[ i ] == '0' ); ++i )
		;
		
		/* if the string is not a hexadecimal representation or it is and at least one zero was 
		encountered after 0x
		*/
		if( !hex || ( i != temp ) )
		{
			/* example representations at this point:
			"0000000"
			"   0000"
			"0"
			"   +0x000000"
			" 0x0000 "
			"  -019"
			"019 "
			"0002341"
			"0  0"
			"00  1234"
			"0x00 A"
			"0x0   0"
			"00 yy"
			"0 123"
			"-000"
			" -0x0     "
			"0x000AAA"
			"0x0a"
			"  0x0a     "
			"0x0az"
			"-0x0    ab  "
			*/
			
			/* skip whitespace that comes after zeroes */
			for( temp = i; ( ( str[ i ] == ' ' ) || ( str[ i ] == '\t' ) ); ++i )
			;
			
			/* if the string is not a hexadecimal representation or whitespace was encountered 
			after zeroes or there are no more characters
			*/
			if( !hex || ( i != temp ) || !str[ i ] )
			{
				/* if a decimal or hexadecimal string has no additional characters and it is not 
				negative then it is valid representation of zero.
				*/
				if( !str[ i ] && !negative ) // input was scanned successfully
				{
					/* example representations at this point:
					"0000000"
					"   0000"
					"0"
					"   +0x000000"
					" 0x0000 "
					*/
					*num = 0;
					return NUM_POS;
				}
				
				/* example representations at this point:
				"  -019"
				"019 "
				"0002341"
				"0  0"
				"00  1234"
				"0x00 A"
				"0x0   0"
				"00 yy"
				"0 123"
				"-000"
				" -0x0     "
				*/
				goto fail;
			}
		}
		
		/* example representations at this point:
		"0x000AAA"
		"0x0a"
		"  0x0a     "
		"0x0az"
		"-0x0    ab  "
		*/
	}
	
	/* skip valid characters */
	for( temp = i;
		( ( str[ i ] >= '0' ) && ( str[ i ] <= '9' )
			|| ( hex ? ( ( str[ i ] >= 'A' ) && ( str[ i ] <= 'F' ) ) : 0 )
			|| ( hex ? ( ( str[ i ] >= 'a' ) && ( str[ i ] <= 'f' ) ) : 0 )
		);
		++i
	)
	;
	
	/* if no valid characters were found then fail */
	if( i == temp )
		goto fail;
	
	
	/* skip any trailing whitespace */
	for( temp = i; ( ( str[ i ] == ' ' ) || ( str[ i ] == '\t' ) ); ++i )
	;
	
	if( str[ i ] ) // input wasn't scanned successfully. unexpected character.
		goto fail;
	
	/* _strtoi64() can't read in positive hex > I64_MAX (0x7FFFFFFFFFFFFFFF).
	This is a problem because the user might specify a kernel address, and those 
	addresses can be greater than I64_MAX.
	
	_strtoui64() can read in positive hex > I64_MAX. Also it will convert negative 
	as well, because it wraps around, converting a negative integer to unsigned in 
	accordance with the standard, ie negnum + UI64_MAX + 1.
	
	That will be a problem detecting overflow on a negative number, eg:
	_strtoi64("-9999999999999999999") -9223372036854775808. detected
	_strtoui64("-9999999999999999999") 8446744073709551617. undetected
	
	Therefore the best way is to first check with _strtoi64(), and if that result 
	is I64_MIN then the negative number is out of range. If the result is I64_MAX 
	then try again with _strtoui64(). If the result is UI64_MAX then the positive 
	number is out of range. This makes the effective range:
	from (I64_MIN+1) to (UI64_MAX-1)
	min "-9223372036854775807", "-0x7FFFFFFFFFFFFFFF"
	max "18446744073709551614", "0xFFFFFFFFFFFFFFFE"
	*/
	s = _strtoi64( str, &endptr, ( hex ? 16 : 10 ) );
	
	if( s == I64_MIN )
		goto fail;
	
	if( ( s < 0 ) && !negative )
		goto fail;
	
	if( ( s >= 0 ) && negative )
		goto fail;
	
	if( s == I64_MAX )
	{
		endptr = NULL;
		u = _strtoui64( str, &endptr, ( hex ? 16 : 10 ) );
		
		if( u == UI64_MAX )
			goto fail;
	}
	else
		u = (unsigned __int64)s;
	
	/* if the result from strtoi64() or strtoui64() is 0 then the string could not 
	be converted (zero is handled separately in this function).
	*/
	if( !u )
		goto fail;
	
	/* if there are any additional characters that weren't converted */
	if( endptr )
	{
		/* remove trailing whitespace */
		for( ; ( ( *endptr == ' ' ) || ( *endptr == '\t' ) ); ++endptr )
		;
		
		/* if the result has any additional unexpected characters */
		if( *endptr )
			goto fail;
	}
	
	*num = u;
	return ( negative ? NUM_NEG : NUM_POS );
	
fail:
	if( num )
		*num = UI64_MAX;
	return NUM_FAIL;
}



/* str_to_int64()
Convert a signed or unsigned decimal or hexadecimal string to a 64-bit signed integer.

'str' is the integer representation as a decimal or hexadecimal string
'*num' receives a 64-bit signed integer.

the range of conversion is 
min "-9223372036854775807", "-0x7FFFFFFFFFFFFFFF", (I64_MIN+1)
max "9223372036854775806", "0x7FFFFFFFFFFFFFFE", (I64_MAX-1)

returns nonzero on success: 
returns -1 if the integer is negative. I64_MIN < *num < 0.
returns 1 if the integer is positive. I64_MAX > *num >= 0.

returns 0 on fail and '*num' receives I64_MAX.
*/
enum sti_type str_to_int64( 
		__int64 *const num,   // out
		const char *const str   // in
)
{
	unsigned __int64 u = 0;
	enum sti_type ret;
	
	
	if( !num || !str )
		goto fail;
	
	ret = str_to_uint64( &u, str );
	
	if( ret == NUM_FAIL )
		goto fail;
	
	if( ( ret == NUM_POS ) && ( u >= (unsigned __int64)I64_MAX ) )
		goto fail;
	
	*num = (__int64)u;
	return ret;
	
fail:
	if( num )
		*num = I64_MAX;
	return NUM_FAIL;
}



/* str_to_uint()
Convert a signed or unsigned decimal or hexadecimal string to an unsigned integer.

'str' is the integer representation as a decimal or hexadecimal string
'*num' receives an unsigned integer.

the range of conversion is 
min "-2147483647", "-0x7FFFFFFF", (INT_MIN+1)
max "4294967294", "0xFFFFFFFE", (UINT_MAX-1)

returns nonzero on success: 
returns -1 if the integer is negative. INT_MIN < *num < 0. cast to (int)
returns 1 if the integer is positive. UINT_MAX > *num >= 0.

returns 0 on fail and '*num' receives UINT_MAX.
note that *num does not receive UINT_MAX only on fail. eg if str is "-1" then *num is UINT_MAX
*/
enum sti_type str_to_uint( 
		unsigned *const num,   // out
		const char *const str   // in
)
{
	unsigned __int64 u = 0;
	enum sti_type ret;
	
	
	if( !num || !str )
		goto fail;
	
	ret = str_to_uint64( &u, str );
	
	if( ret == NUM_FAIL )
		goto fail;
	
	if( ( ret == NUM_POS ) && ( u >= (unsigned __int64)UINT_MAX ) )
		goto fail;
	
	if( ( ret == NUM_NEG ) && ( (__int64)u <= (__int64)INT_MIN ) )
		goto fail;
	
	*num = (unsigned)u;
	return ret;
	
fail:
	if( num )
		*num = UINT_MAX;
	return NUM_FAIL;
}



/* str_to_int()
Convert a signed or unsigned decimal or hexadecimal string to a signed integer.

'str' is the integer representation as a decimal or hexadecimal string
'*num' receives a signed integer.

the range of conversion is 
min "-2147483647", "-0x7FFFFFFF", (INT_MIN+1)
max "2147483646", "0x7FFFFFFE", (INT_MAX-1)

returns nonzero on success: 
returns -1 if the integer is negative. INT_MIN < *num < 0.
returns 1 if the integer is positive. INT_MAX > *num >= 0.

returns 0 on fail and '*num' receives INT_MAX.
*/
enum sti_type str_to_int( 
		int *const num,   // out
		const char *const str   // in
)
{
	unsigned u = 0;
	enum sti_type ret;
	
	
	if( !num || !str )
		goto fail;
	
	ret = str_to_uint( &u, str );
	
	if( ret == NUM_FAIL )
		goto fail;
	
	if( ( ret == NUM_POS ) && ( u >= (unsigned)INT_MAX ) )
		goto fail;
	
	if( ( ret == NUM_NEG ) && ( (int)u <= INT_MIN ) )
		goto fail;
	
	*num = (int)u;
	return ret;
	
fail:
	if( num )
		*num = INT_MAX;
	return NUM_FAIL;
}



/* test the functions in this file */
#ifdef TESTME

#include <stdio.h>

#ifdef _MSC_VER
#pragma optimize( "g", off ) /* disable global optimizations */
#pragma auto_inline( off ) /* disable automatic inlining */
#endif
static void pause( void )
{
	system( "pause" );
	return;
}
#ifdef _MSC_VER
#pragma auto_inline( on ) /* revert automatic inlining setting */
#pragma optimize( "g", on ) /* revert global optimizations setting */
#endif

int main( int argc, char *argv[] )
{
	unsigned __int64 x;
	__int64 y;
	
	{
		HANDLE hOutput = GetStdHandle( STD_OUTPUT_HANDLE );
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		
		
		ZeroMemory( &csbi, sizeof( csbi ) );
		
		if( ( hOutput != INVALID_HANDLE_VALUE )
			&& ( GetFileType( hOutput ) == FILE_TYPE_CHAR )
			&& GetConsoleScreenBufferInfo( hOutput, &csbi )
			&& !csbi.dwCursorPosition.X 
			&& !csbi.dwCursorPosition.Y
			&& ( csbi.dwSize.X > 0 )
			&& ( csbi.dwSize.Y > 0 )
		)
			atexit( pause );
	}
	
	
	if( argc < 2 )
		return 1;
	
	printf( "errno before: %d\n", errno );
	x = _strtoui64( argv[ 1 ], NULL, 0 );
	y = _strtoi64( argv[ 1 ], NULL, 0 );
	printf( "errno after: %d\n", errno );
	
	printf( "_strtoui64 x %I64d %I64u\n", x, x );
	printf( " _strtoi64 y %I64d %I64u\n", y, y );
	printf( "\n\t\tsigned unsigned hex\n" );
	printf( "_UI64_MAX\t%I64d %I64u %I64X\n", _UI64_MAX, _UI64_MAX, _UI64_MAX );
	printf( "_I64_MAX\t%I64d %I64u %I64X\n", _I64_MAX, _I64_MAX, _I64_MAX );
	printf( "_I64_MIN\t%I64d %I64u %I64X\n", _I64_MIN, _I64_MIN, _I64_MIN );
	printf( "UINT_MAX\t%d %u %X\n", UINT_MAX, UINT_MAX, UINT_MAX );
	printf( "INT_MAX\t%d %u %X\n", INT_MAX, INT_MAX, INT_MAX );
	printf( "INT_MIN\t%d %u %X\n", INT_MIN, INT_MIN, INT_MIN );
	
	printf( "\n\n\n" );
	{
		int ret;
		int a = 0;
		unsigned b = 0;
		__int64 c = 0;
		unsigned __int64 d = 0;
		
		ret = str_to_uint64( &d, argv[ 1 ] );
		printf( "%d str_to_uint64\t%I64u %I64X. %I64d\n", ret, d, d, d );
		
		ret = str_to_int64( &c, argv[ 1 ] );
		printf( "%d str_to_int64\t%I64d %I64X\n", ret, c, c );
		
		ret = str_to_uint( &b, argv[ 1 ] );
		printf( "%d str_to_uint\t%u %X. %d\n", ret, b, b, b );
		
		ret = str_to_int( &a, argv[ 1 ] );
		printf( "%d str_to_int\t%d %X\n", ret, a, a );
		
	}
	
	return 0;
}
#endif //TESTME
