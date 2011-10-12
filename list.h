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

#ifndef _LIST_H
#define _LIST_H

#include <windows.h>



#ifdef __cplusplus
extern "C" {
#endif


/** This is the info needed for each item in a generic list.
What members are valid depends on the type of list.
GetHooks config currently uses four lists:

test include list:
name is required. id is required.

desktop include list:
name is required. id is unused.

hook include/exclude list:
name is optional. id is required.

program include/exclude list:
name and id are currently handled elsewhere as mutually exclusive.
if( name ) then the name is used, but if( !name ) then the id is used.
the id may represent either a PID or TID
*/
struct list_item
{
	__int64 id;
	WCHAR *name;   // _wcsdup(), free()
	
	/* The next item in the list */
	struct list_item *next;
};



/** The generic list store type.
The different types of lists that can be held by the store.
*/
enum list_type
{
	LIST_INVALID_TYPE,   // 0 is an invalid type
	LIST_INCLUDE_TEST,   // list of tests to include
	LIST_INCLUDE_DESK,   // list of desktops to include
	LIST_INCLUDE_HOOK,   // list of hooks to include
	LIST_INCLUDE_PROG,   // list of programs to include
	LIST_EXCLUDE_HOOK,   // list of hooks to exclude
	LIST_EXCLUDE_PROG   // list of programs to exclude
};



/** The generic list store. 
The list store holds a linked list of some type specified below.
*/
struct list
{
	/* list item. this is a pointer to the first item in the list. */
	struct list_item *head;   // items calloc'd. the list is freed when the store is freed.
	
	/* the last item in the list */
	struct list_item *tail;
	
	/* the list type */
	enum list_type type;
	
	/* the system utc time in FILETIME format immediately after this store has been initialized.
	this is nonzero when this store has been initialized.
	*/
	__int64 init_time;
};



/** 
these functions are documented in the comment block above their definitions in list.c
*/
void create_list_store( 
	struct list **const out   // out deref
);

struct list_item *add_list_item( 
	struct list *const store,   // in
	__int64 id,   // in, optional
	const WCHAR *name   // in, optional
);

void print_list_item( 
	const struct list_item *const item   // in
);

void print_list_store( 
	const struct list *const store   // in
);

void free_list_store( 
	struct list **const in   // in deref
);


#ifdef __cplusplus
}
#endif

#endif /* _LIST_H */
