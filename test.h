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

#ifndef _TEST_H
#define _TEST_H

#include <windows.h>



#ifdef __cplusplus
extern "C" {
#endif


/** 
these functions are documented in the comment block above their definitions in test.c
*/
__int64 print_handle_count( 
	__int64 seconds   // in
);

__int64 print_kernel_HOOK(
	__int64 addr   // in
);

__int64 print_kernel_HOOK_chain(
	__int64 addr   // in
);

__int64 print_desktop_HOOK_chains( 
	__int64 unused   // unused
);


void print_testmode_usage( void );

int testmode( void );


#ifdef __cplusplus
}
#endif

#endif // _TEST_H
