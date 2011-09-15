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
This file contains functions for a desktop hook store (linked list of desktop and hook information).
Each function is documented in the comment block above its definition.

There are multiple desktop hook stores: Each snapshot has its own desktop hook store.
Each desktop hook store depends on all the other information in the snapshot (GUI, SPI, etc.)

-
create_desktop_hook_store()

Create a desktop hook store and its descendants or die.
-

-
print_desktop_hook_item()

Print an item from a desktop hook store's linked list.
-

-
print_desktop_hook_store()

Print a desktop hook store and all its descendants.
-

-
free_desktop_hook_item()

Free a desktop hook item (dirty -- linked list isn't updated).
-

-
free_desktop_hook_store()

Free a desktop hook store and all its descendants.
-

*/

#include <stdio.h>

#include "util.h"

#include "desktop_hook.h"

/* the global stores */
#include "global.h"



/* create_desktop_hook_store()
Create a desktop hook store and its descendants or die.
*/
void create_desktop_hook_store( 
	struct desktop_hook_list **const out   // out deref
)
{
	struct desktop_hook_list *desktop_hooks = NULL;
	
	FAIL_IF( !out );
	FAIL_IF( *out );
	
	
	/* allocate a desktop hook store */
	desktop_hooks = must_calloc( 1, sizeof( *desktop_hooks ) );
	
	
	/* the allocated/maximum number of elements in the array pointed to by hook.
	
	65535 is the maximum number of user objects
	http://blogs.technet.com/b/markrussinovich/archive/2010/02/24/3315174.aspx
	*/
	desktop_hooks->hook_max = 65535;
	
	/* allocate an array of hook structs */
	desktop_hooks->hook = 
		must_calloc( desktop_hooks->hook_max, sizeof( *desktop_hooks->hook );
	
	
	*out = desktop_hooks;
	return;
}



/* add_desktop_hook_item()
Create a desktop hook item and append it to the desktop hook store's linked list.
*/
void add_desktop_hook_item(
	struct desktop_hook_list *store,   // in
	struct desktop_item *desktop   // in
)
{
	
}



/* init_desktop_hook_store()
Initialize the desktop hook store by recording the hooks for each desktop.

The desktop hook store depends on the spi and gui info in its parent snapshot store and all global 
stores except other snapshot stores.
*/
void init_desktop_hook_store( 
	struct snapshot *parent   // in
)
{
	struct desktop_hook_list *store = NULL;
	
	FAIL_IF( !G->prog->init_time );   // The program store must already be initialized.
	FAIL_IF( !G->config->init_time );   // The configuration store must already be initialized.
	FAIL_IF( !G->desktops->init_time );   // The desktop store must already be initialized.
	
	FAIL_IF( !parent );   // The snapshot parent of the desktop_hook store must always be passed in
	FAIL_IF( !parent->init_time_spi );   // The snapshot's spi array must already be initialized
	FAIL_IF( !parent->init_time_gui );   // The snapshot's gui array must already be initialized
	
	FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	
	store = parent->desktop_hooks;
	
	/* this store is reused. do a soft reset */
	store->init_time = 0;
	
	if( !store->head ) // create the desktop hook list
	{
		struct desktop_item *current = NULL;
		
		/* add the desktops from the global desktop store */
		for( current = G->desktops->head; current; current = current->next )
			add_desktop_hook_item( store->desktop_hooks, current );
	}
	else // the desktop hook list already exists. reuse it.
	{
		struct desktop_hook_item *current = NULL;
		
		/* soft reset on each desktop hook item's array of hooks */
		for( current = store->head; current; current = current->next )
			current->hook_count = 0;
	}
	
	
	
	/* G->desktop_hooks has been initialized */
	GetSystemTimeAsFileTime( (FILETIME *)&G->desktop_hooks->init_time );
	return;
}



/* print_HANDLEENTRY()
Print a HANDLEENTRY struct.

if the HANDLEENTRY pointer is != NULL then print the HANDLEENTRY
*/
void print_HANDLEENTRY(
	const HANDLEENTRY *const entry   // in
)
{
	const char *const objname = "Handle Entry";
	
	
	if( !entry )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	if( entry->pHead )
	{
		PRINT_PTR( entry->pHead->h );
		printf( "entry->pHead->cLockObj: %lu\n", entry->pHead->cLockObj );
	}
	
	PRINT_PTR( entry->pOwner );
	
	if( entry->bType == TYPE_HOOK )
		printf( "entry->bType: TYPE_HOOK\n" );
	else
		printf( "entry->bType: %u\n", (unsigned)entry->bType );
	
	printf( "entry->bFlags: 0x%02X\n", (unsigned)entry->bFlags );
	printf( "entry->wUniq: %u\n", (unsigned)entry->wUniq );
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_HOOK()
Print a HOOK struct.

if the HOOK pointer is != NULL then print the HOOK
*/
void print_HOOK(
	const HOOK *const object   // in
)
{
	const char *const objname = "Hook Object";
	
	
	if( !object )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	PRINT_PTR( object->Head.h );
	printf( "object->Head.cLockObj: %lu\n", object->Head.cLockObj );
	
	PRINT_PTR( object->pti );
	PRINT_PTR( object->rpdesk1 );
	PRINT_PTR( object->pSelf );
	PRINT_PTR( object->phkNext );
	printf( "object->iHook: %d\n", object->iHook );
	printf( "object->offPfn: 0x%08lX\n", object->offPfn );
	printf( "object->flags: 0x%08lX\n", object->flags );
	printf( "object->ihmod: %d\n", object->ihmod );
	PRINT_PTR( object->ptiHooked );
	PRINT_PTR( object->rpdesk2 );
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_desktop_hook_item()
Print an item from a desktop hook store's linked list.

if the desktop hook item pointer is != NULL print the item
*/
void print_desktop_hook_item( 
	const struct desktop_hook_item *const item   // in
)
{
	const char *const objname = "Desktop Hook Item";
	int i = 0;
	
	if( !item )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	if( item->desktop )
		printf( "item->desktop->pwszDesktopName: %ls\n", item->desktop->pwszDesktopName );
	else
		MSG_ERROR( "item->desktop == NULL" );
	
	printf( "item->hook_max: %u\n", item->hook_max );
	printf( "item->hook_count: %u\n", item->hook_count );
	for( i = 0; i < item->hook_count; ++i )
	{
		print_HANDLEENTRY( &item->hook[ i ].handle );
		print_HOOK( &item->hook[ i ].object );
		print_gui( item->hook[ i ].owner );
		print_gui( item->hook[ i ].origin );
		print_gui( item->hook[ i ].target );
	}
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_desktop_hook_store()
Print a desktop hook store and all its descendants.

if the desktop hook store pointer != NULL print the store
*/
static void print_desktop_hook_store( 
	const struct desktop_hook_list *const store   // in
)
{
	struct desktop_hook_item *item = NULL;
	const char *const objname = "Desktop Hook List Store";
	
	
	if( !store )
		return;
	
	PRINT_DBLSEP_BEGIN( objname );
	print_init_time( "store->init_time", store->init_time );
	
	PRINT_PTR( store->head );
	
	for( item = store->head; item; item = item->next )
	{
		PRINT_PTR( item );
		print_desktop_hook_item( item );
	}
	
	PRINT_PTR( store->tail );
	
	PRINT_DBLSEP_END( objname );
	
	return;
}



/* free_desktop_hook_item()
Free a desktop hook item (dirty -- linked list isn't updated).

this function then sets the desktop hook item pointer to NULL and returns.

this function has no regard for the other items in the list and should only be called by 
free_desktop_hook_store().

'in' is a pointer to a pointer to the desktop hook item, which contains a desktop and its hooks.
if( !in || !*in ) then this function returns.
*/
static void free_desktop_hook_item( 
	struct desktop_hook_item **const in   // in deref
)
{
	if( !in || !*in )
		return;
	
	free( (*in)->hook );
	
	free( (*in) );
	*in = NULL;
	
	return;
}



/* free_desktop_hook_store()
Free a desktop hook store and all its descendants.

this function then sets the desktop hook store pointer to NULL and returns

'in' is a pointer to a pointer to the desktop hook store, which contains a linked list of desktops 
and their hooks.
if( !in || !*in ) then this function returns.
*/
void free_desktop_hook_store( 
	struct desktop_hook_list **const in   // in deref
)
{
	if( !in || !*in )
		return;
	
	if( (*in)->head )
	{
		struct desktop_hook_item *current = NULL, *p = NULL;
		
		for( current = (*in)->head; current; current = p )
		{
			p = current->next;
			
			free_desktop_hook_item( &current );
		}
	}
	
	free( (*in) );
	*in = NULL;
	
	return;
}
