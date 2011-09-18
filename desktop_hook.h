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

#ifndef _DESKTOP_HOOK_H
#define _DESKTOP_HOOK_H

#include <windows.h>

/* ReactOS structures for hooks and handles */
#include "reactos_structs.h"

/* desktop store (linked list of desktops' heap and thread info) */
#include "desktop.h"



#ifdef __cplusplus
extern "C" {
#endif


/** This is the info to keep track of when a HOOK object is found.
For each HANDLEENTRY traversed if its bType == TYPE_HOOK then the handle entry is for a HOOK.
*/
struct hook 
{
	/* a copy of the HANDLEENTRY for the HOOK */
	HANDLEENTRY entry;
	
	/* a copy of the HOOK */
	HOOK object;
	
	/* the thread that owns the handle entry to the HOOK */
	struct gui *owner;
	
	/* the thread where the HOOK originated */
	struct gui *origin;
	
	/* the thread that's hooked */
	struct gui *target;
};



/** This is the info needed for each item in the desktop hook list.
Each item has information on a desktop and its hooks.
*/
struct desktop_hook_item
{
	/* the desktop */
	struct desktop_item *desktop;
	
	
	
	/** an array of hook structs. these are the hooks on desktop.
	*/
	/* the hook array */
	struct hook *hook;   // calloc(), free()
	
	/* the allocated/maximum number of elements in the hook array */
	unsigned hook_max;
	
	/* how many handle entries for HOOK objects were found.
	this is also the number of elements written to in the hook array.
	*/
	unsigned hook_count;
	
	
	
	/* The next item in the list */
	struct desktop_hook_item *next;
};



/** The desktop hook store.
The desktop hook store holds a linked list of desktops and their hooks.
*/
struct desktop_hook_list
{
	/* linked list of desktop_hook_item.
	this is a pointer to the first item in the desktop hook list.
	*/
	struct desktop_hook_item *head;   // items calloc'd. the list is freed when the store is freed.
	
	/* the last item in the desktop hook list */
	struct desktop_hook_item *tail;
	
	/* the desktop list type */
	//enum desktop_hook_type type;
	
	/* the system utc time in FILETIME format immediately after this store has been initialized.
	this is nonzero when this store has been initialized.
	*/
	__int64 init_time;
};



/** 
these functions are documented in the comment block above their definitions in desktop_hook.c
*/
void create_desktop_hook_store( 
	struct desktop_hook_list **const out   // out deref
);

static struct desktop_hook_item *add_desktop_hook_item(
	struct desktop_hook_list *const store,   // in
	struct desktop_item *const desktop   // in
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

int compare_hook( 
	const void *const p1,   // in
	const void *const p2   // in
);

int init_desktop_hook_store( 
	struct snapshot *const parent   // in
);

void print_HANDLEENTRY(
	const HANDLEENTRY *const entry   // in
);

void print_HOOK(
	const HOOK *const object   // in
);

void print_hook(
	const struct hook *const hook   // in
);

void print_desktop_hook_item( 
	const struct desktop_hook_item *const item   // in
);

void print_desktop_hook_store( 
	const struct desktop_hook_list *const store   // in
);

static void free_desktop_hook_item( 
	struct desktop_hook_item **const in   // in deref
);

void free_desktop_hook_store( 
	struct desktop_hook_list **const in   // in deref
);


#ifdef __cplusplus
}
#endif

#endif // _DESKTOP_HOOK_H
