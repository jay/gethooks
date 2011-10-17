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

#ifndef _DEBUG_H
#define _DEBUG_H

#include <windows.h>



#ifdef __cplusplus
extern "C" {
#endif


/** 
these functions are documented in the comment block above their definitions in debug.c
*/
int dump_teb( 
	const DWORD pid,   // in
	const DWORD tid,   // in
	const DWORD flags   // in, optional
);


#ifdef __cplusplus
}
#endif

#endif /* _DEBUG_H */
