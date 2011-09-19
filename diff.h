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

#ifndef _GETHOOKS_H
#define _GETHOOKS_H

#include <windows.h>

/* the desktop hook store */
#include "desktop_hook.h"



#ifdef __cplusplus
extern "C" {
#endif


/** 
these functions are documented in the comment block above their definitions in gethooks.c
*/
int match_gui_process_name(
	const struct gui *const gui,   // in
	const WCHAR *const name   // in
);

int match_gui_process_pid(
	const struct gui *const gui,   // in
	const int pid   // in
);

int is_hook_wanted( 
	const struct hook *const hook   // in
);

int match_hook_process_pid(
	const struct hook *const hook,   // in
	const int pid   // in
);

int match_hook_process_name(
	const struct hook *const hook,   // in
	const WCHAR *const name   // in
);

void print_diff_desktop_hook_items( 
	const struct desktop_hook_item *const a,   // in
	const struct desktop_hook_item *const b   // in
);

void print_diff_desktop_hook_lists( 
	const struct desktop_hook_list *const list1,   // in
	const struct desktop_hook_list *const list2   // in
);

#ifdef __cplusplus
}
#endif

#endif // _GETHOOKS_H
