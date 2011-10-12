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
This file contains functions for a generic list store (linked list of names and/or ids).
Each function is documented in the comment block above its definition.

-
create_list_store()

Create a list store and its descendants or die.
-

-
add_list_item()

Append an item to a list store's linked list.
-

-
print_list_item()

Print an item from a list store's linked list.
-

-
print_list_store()

Print a list store and all its descendants.
-

-
free_list_store()

Free a list store and all its descendants.
-

*/

#include <stdio.h>

#include "util.h"

#include "reactos.h"

#include "list.h"



/* create_list_store()
Create a list store and its descendants or die.
*/
void create_list_store( 
	struct list **const out   // out deref
)
{
	FAIL_IF( !out );
	FAIL_IF( *out );
	
	
	*out = must_calloc( 1, sizeof( **out ) );
	
	return;
}



/* add_list_item()
Append an item to a list store's linked list.

this function appends an item to a list if the id and/or name (depending on list type) is not 
already in the list. any comparison of 'name' is case insensitive.

'store' is the generic list store to append the item to.
whether 'id' and/or 'name' is used depends on the type of list. refer to list_item struct in list.h

hook:
'name' is optional. 'id' is required.
if 'name' then its corresponding id will be used for id instead of the passed in 'id'.
if not 'name' then the corresponding name (if any) of the passed in 'id' will be used for name.

prog:
'name' and 'id' are mutually exclusive. if 'name' use name and ignore 'id', else use 'id'.

desktop:
'name' required. 'id' is ignored.

test:
'name' required. 'id' required.

the item's name will point to a duplicate of the passed in 'name'.

returns on success a pointer to the list item that was added to the list. if there is already an 
existing item with the same id and/or name (depending on list type) a pointer to it is returned.
*/
struct list_item *add_list_item( 
	struct list *const store,   // in
	__int64 id,   // in, optional
	const WCHAR *name   // in, optional
)
{
	struct list_item *item = NULL;
	
	/* special case, if there's no 'name' for a hook point to a copy of the corresponding name 
	associated with the 'id'. this must be freed if a new item will not be created.
	*/
	WCHAR *hookname = NULL;
	
	FAIL_IF( !store );
	FAIL_IF( !store->type );
	
	
	/* if an item already in the list matches then return */
	if( ( store->type == LIST_INCLUDE_HOOK ) || ( store->type == LIST_EXCLUDE_HOOK ) )
	{
		if( name )
		{
			int hookid = 0;
			
			/* it is considered fatal if there's no id associated with a name. 
			it's possible this program's list of hooks is outdated, but in that case the user 
			would have to specify by id instead of name
			*/
			if( !get_HOOK_id_from_name( &hookid, name ) )
			{
				MSG_ERROR( "get_HOOK_id_from_name() failed." );
				printf( "Unknown id for hook name: %ls\n", name );
				printf( "\n" );
				item = NULL;
				goto existing_item;
			}
			
			id = hookid;
		}
		else
		{
			/* it is not considered fatal if there's no name associated with an id.
			maybe the user specified some undocumented hook ids in use without a name?
			hookname points to allocated memory and must be freed if a new item will not be created.
			*/
			if( !get_HOOK_name_from_id( &hookname, (int)id ) )
			{
				MSG_WARNING( "get_HOOK_name_from_id() failed." );
				printf( "Unknown name for hook id: %I64d\n", id );
				printf( "\n" );
			}
		}
		
		/* check if the hook id is already in the list.
		a hook id always has the same name (if any).
		if the hook id is already in the list a new item will not be created.
		*/
		for( item = store->head; item; item = item->next )
		{
			if( id == item->id ) /* hook id in list */
			{
				MSG_WARNING( "Hook id already in list." );
				print_list_item( item );
				printf( "\n" );
				goto existing_item;
			}
		}
		
		goto new_item;
	}
	else if( ( store->type == LIST_INCLUDE_PROG ) || ( store->type == LIST_EXCLUDE_PROG ) )
	{
		/* for the program list, name and id are mutually exclusive.
		if name then a program name has been passed in, 
		otherwise an id has been passed in. 
		if name or id is already in the list then there is no reason to append
		*/
		if( name )
		{
			/* check if the program name is already in the list */
			for( item = store->head; item; item = item->next )
			{
				if( item->name && !_wcsicmp( item->name, name ) ) /* name in list */
				{
					MSG_WARNING( "Program name already in list." );
					print_list_item( item );
					printf( "\n" );
					goto existing_item;
				}
			}
		}
		else
		{
			/* check if the PID/TID is already in the list */
			for( item = store->head; item; item = item->next )
			{
				/* a program list item's id is only valid if doesn't have a name */
				if( !item->name && id == item->id ) /* PID/TID in list */
				{
					MSG_WARNING( "PID/TID already in list." );
					print_list_item( item );
					printf( "\n" );
					goto existing_item;
				}
			}
		}
		
		goto new_item;
	}
	else if( store->type == LIST_INCLUDE_DESK )
	{
		FAIL_IF( !name );
		FAIL_IF( id ); // not expecting an id parameter for a desktop list
		
		/* for a desktop list only the name is used. check if it's already in the list */
		for( item = store->head; item; item = item->next )
		{
			if( item->name && !_wcsicmp( item->name, name ) ) /* name in list */
			{
				MSG_WARNING( "Desktop name already in list." );
				print_list_item( item );
				printf( "\n" );
				goto existing_item;
			}
		}
		
		goto new_item;
	}
	else if( store->type == LIST_INCLUDE_TEST )
	{
		FAIL_IF( !name );
		// id can be 0
		
		/* for a test list both name and id are used. check if it's already in the list */
		for( item = store->head; item; item = item->next )
		{
			if( ( item->id && id )
				&& ( item->name && !_wcsicmp( item->name, name ) )
			) // name/id combo already in list
			{
				MSG_WARNING( "Test name/id combo already in list." );
				print_list_item( item );
				printf( "\n" );
				goto existing_item;
			}
		}
		
		goto new_item;
	}
	else // handle generic here?
	{
		MSG_FATAL( "Unknown list type." );
		printf( "store->type: %d\n", store->type );
		exit( 1 );
	}
	
	
new_item:
	/* create a new item and add it to the list */
	
	item = must_calloc( 1, sizeof( *item ) );
	
	if( hookname ) /* special case. this function already has made a copy of the name to store */
		item->name = hookname;
	else if( name ) /* store a copy of the passed in name */
		item->name = must_wcsdup( name );
	else
		item->name = NULL;
	
	item->id = id;
	item->next = NULL;
	
	if( !store->head )
	{
		store->head = item;
		store->tail = item;
	}
	else
	{
		store->tail->next = item;
		store->tail = item;
	}
	
	return item;
	
	
existing_item:
	/* do not create a new item. free resources and return existing item, if any */
	
	free( hookname );
	return item;
}



/* print_list_item()
Print an item from a list store's linked list.

if 'item' is NULL this function returns without having printed anything.
*/
void print_list_item( 
	const struct list_item *const item   // in
)
{
	const char *const objname = "Generic List Item";
	
	
	if( !item )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	printf( "item->name: %ls\n", item->name );
	//printf( "item->id: %I64d (0x%I64X)\n", item->id, item->id );
	printf( "item->id (signed): %I64d\n", item->id );
	printf( "item->id (unsigned): %I64u\n", item->id );
	printf( "item->id (hex): 0x%I64X\n", item->id );
	
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_list_store()
Print a list store and all its descendants.

if 'store' is NULL this function returns without having printed anything.
*/
void print_list_store( 
	const struct list *const store   // in
)
{
	const char *const objname = "Generic List Store";
	struct list_item *item = NULL;
	
	
	if( !store )
		return;
	
	PRINT_DBLSEP_BEGIN( objname );
	print_init_time( "store->init_time", store->init_time );
	
	printf( "store->type: " );
	switch( store->type )
	{
		case LIST_INVALID_TYPE:
			printf( "LIST_INVALID_TYPE (the user-specified list type hasn't been set.)" );
			break;
		case LIST_INCLUDE_TEST:
			printf( "LIST_INCLUDE_TEST (user-specified list of tests to include.)" );
			break;
		case LIST_INCLUDE_DESK:
			printf( "LIST_INCLUDE_DESK (user-specified list of desktops to include.)" );
			break;
		case LIST_INCLUDE_HOOK:
			printf( "LIST_INCLUDE_HOOK (user-specified list of hooks to include.)" );
			break;
		case LIST_INCLUDE_PROG:
			printf( "LIST_INCLUDE_PROG (user-specified list of programs to include.)" );
			break;
		case LIST_EXCLUDE_HOOK:
			printf( "LIST_EXCLUDE_HOOK (user-specified list of hooks to exclude.)" );
			break;
		case LIST_EXCLUDE_PROG:
			printf( "LIST_EXCLUDE_PROG (user-specified list of programs to exclude.)" );
			break;
		default:
			printf( "%d (unknown type)", store->type );
	}
	printf( "\n" );
	
	//printf( "\nNow printing the desktop items in the list from head to tail.\n " );
	PRINT_HEX( store->head );
	
	for( item = store->head; item; item = item->next )
	{
		//PRINT_HEX( item );
		print_list_item( item );
	}
	
	PRINT_HEX( store->tail );
	
	PRINT_DBLSEP_END( objname );
	
	return;
}



/* free_list_store()
Free a list store and all its descendants.

this function then sets the list store pointer to NULL and returns

'in' is a pointer to a pointer to the list store, which contains the list information.
if( !in || !*in ) then this function returns.
*/
void free_list_store( 
	struct list **const in   // in deref
)
{
	if( !in || !*in )
		return;
	
	if( (*in)->head )
	{
		struct list_item *current = NULL, *p = NULL;
		
		for( current = (*in)->head; current; current = p )
		{
			p = current->next;
			
			/* free any resources associated with the item */
			if( current->name )
				free( current->name );
			
			/* free the item */
			free( current );
		}
	}
	
	free( (*in) );
	*in = NULL;
	
	return;
}
