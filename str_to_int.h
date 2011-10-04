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

#ifndef _STR_TO_INT_H
#define _STR_TO_INT_H

#include <windows.h>
#include <limits.h>



#ifdef __cplusplus
extern "C" {
#endif


/* I64_MAX and I64_MIN are supposed to be defined in limits.h but often they aren't */
#ifndef I64_MAX
#define I64_MAX _I64_MAX
#endif
#ifndef I64_MIN
#define I64_MIN _I64_MIN
#endif
#ifndef UI64_MAX
#define UI64_MAX _UI64_MAX
#endif



/* returns for str_to_uint64(), str_to_int64(), str_to_uint(), str_to_int() */
enum sti_type
{
	/* the number is negative */
	NUM_NEG = -1, 
	
	/* the function failed */
	NUM_FAIL = 0, 
	
	/* the number is positive or 0 */
	NUM_POS = 1 
};



/** 
these functions are documented in the comment block above their definitions in str_to_int.c
*/
enum sti_type str_to_uint64( 
		unsigned __int64 *const num,   // out
		const char *const str   // in
);

enum sti_type str_to_int64( 
		__int64 *const num,   // out
		const char *const str   // in
);

enum sti_type str_to_uint( 
		unsigned *const num,   // out
		const char *const str   // in
);

enum sti_type str_to_int( 
		int *const num,   // out
		const char *const str   // in
);


#ifdef __cplusplus
}
#endif

#endif // _STR_TO_INT_H
