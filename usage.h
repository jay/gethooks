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

#ifndef _USAGE_H
#define _USAGE_H

#include <windows.h>



#ifdef __cplusplus
extern "C" {
#endif


/** 
these functions are documented in the comment block above their definitions in usage.c
*/
void print_overview_and_exit( void );

void print_more_options_and_exit( void );

void print_more_examples_and_exit( void );

void print_usage_and_exit( void );


#ifdef __cplusplus
}
#endif

#endif /* _USAGE_H */
