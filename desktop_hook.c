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
add_desktop_hook_item()

Create a desktop hook item and append it to the desktop hook store's linked list.
-

-
compare_hook()

Compare two hook structs according their HANDLEENTRY info.
-

-
init_desktop_hook_store()

Initialize the desktop hook store by recording the hooks for each desktop.
-

-
print_hook()

Print a hook struct.
-

-
print_hook_array()

Print a desktop hook item's array of hook structs.
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

#include "snapshot.h"

#include "desktop_hook.h"

/* the global stores */
#include "global.h"



static struct desktop_hook_item *add_desktop_hook_item(
	struct desktop_hook_list *const store,   // in
	struct desktop_item *const desktop   // in
);

static void free_desktop_hook_item( 
	struct desktop_hook_item **const in   // in deref
);



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
	
	
	*out = desktop_hooks;
	return;
}



/* add_desktop_hook_item()
Create a desktop hook item and append it to the desktop hook store's linked list.

returns on success a pointer to the desktop hook item that was added to the list.
if there is already an existing item with the same desktop a pointer to it is returned.
returns NULL on fail
*/
static struct desktop_hook_item *add_desktop_hook_item(
	struct desktop_hook_list *const store,   // in
	struct desktop_item *const desktop   // in
)
{
	struct desktop_hook_item *item = NULL;
	
	FAIL_IF( !store );
	FAIL_IF( !desktop );
	
	
	/* check if there is already an entry for this desktop */
	for( item = store->head; item; item = item->next )
	{
		if( item->desktop == desktop )
			return item;
	}
	
	
	/* create a new item and add it to the list */
	
	item = must_calloc( 1, sizeof( *item ) );
	
	item->desktop = desktop;
	
	/* the allocated/maximum number of elements in the array pointed to by hook.
	
	65535 is the maximum number of user objects
	http://blogs.technet.com/b/markrussinovich/archive/2010/02/24/3315174.aspx
	*/
	item->hook_max = 65535;
	
	/* allocate an array of hook structs */
	item->hook = must_calloc( item->hook_max, sizeof( *item->hook ) );
	
	
	/* the desktop hook item is initialized. add the new item to the end of the list */
	
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
}



/* compare_hook()
Compare two hook structs according their HANDLEENTRY info.

Compare the HANDLEENTRY pHead, which was the kernel address of the associated HOOK struct 
at that point in time (before the HOOK was copied to hook->object). If the pHead is the same then 
check what was the HANDLEENTRY's index in the list (aheList) at that point in time (before the 
HANDLEENTRY was copied to hook->entry). If the index is the same then check the HOOK's handle.
If the handles compare the same as well then assume the two hook structs have information on the 
same HOOK. This will happen when comparing the hook info from two different system snapshots.

There is no way to passively determine whether or not a HOOK is actually the same. This function 
essentially provides a best guess. Windows reuses the memory locations and all the other associated 
information can change.

qsort() callback: this function is called to sort the hook array

returns -1 if p1's entry is less than p2's entry
returns 1 if p1's entry is greater than p2's entry
returns 0 if p1's entry is the same as p2's entry
*/
int compare_hook( 
	const void *const p1,   // in
	const void *const p2   // in
)
{
	const struct hook *const a = p1;
	const struct hook *const b = p2;
	
	
	if( a->entry.pHead < b->entry.pHead )
		return -1;
	else if( a->entry.pHead > b->entry.pHead )
		return 1;
	else if( a->entry_index < b->entry_index )
		return -1;
	else if( a->entry_index > b->entry_index )
		return 1;
	else if( a->object.head.h < b->object.head.h )
		return -1;
	else if( a->object.head.h > b->object.head.h )
		return 1;
	else
		return 0;
}



/* init_desktop_hook_store()
Initialize the desktop hook store by recording the hooks for each desktop.

The desktop hook store depends on the spi and gui info in its parent snapshot store and all global 
stores except other snapshot stores.

returns nonzero on success
*/
int init_desktop_hook_store( 
	const struct snapshot *const parent   // in
)
{
	unsigned i = 0;
	struct desktop_hook_list *store = NULL;
	struct desktop_hook_item *item = NULL;
	
	FAIL_IF( !G );   // The global store must exist.
	FAIL_IF( !G->prog->init_time );   // The program store must be initialized.
	FAIL_IF( !G->config->init_time );   // The configuration store must be initialized.
	FAIL_IF( !G->desktops->init_time );   // The desktop store must be initialized.
	
	FAIL_IF( !parent );   // The snapshot parent of the desktop_hook store must always be passed in.
	FAIL_IF( !parent->init_time_spi );   // The snapshot's spi array must be initialized.
	FAIL_IF( !parent->init_time_gui );   // The snapshot's gui array must be initialized.
	
	FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	
	store = parent->desktop_hooks;
	
	/* this store is reused. do a soft reset */
	store->init_time = 0;
	
	/* if the desktop hook store does not have a list of desktops yet create it */
	if( !store->head )
	{
		struct desktop_item *current = NULL;
		
		/* add the desktops from the global desktop store */
		for( current = G->desktops->head; current; current = current->next )
			add_desktop_hook_item( store, current );
	}
	else // the desktop hook list already exists. reuse it.
	{
		struct desktop_hook_item *current = NULL;
		
		/* soft reset on each desktop hook item's array of hooks */
		for( current = store->head; current; current = current->next )
			current->hook_count = 0; // soft reset of hook array
	}
	
	
	SwitchToThread();
	/* for every handle if it is a HOOK then add it to the desktop's hook array */
	for( i = 0; i < *G->prog->pcHandleEntries; ++i )
	{
		/* copy the HANDLEENTRY struct from the list of entries in the shared info section.
		the info may change so it can't just be pointed to.
		*/
		HANDLEENTRY entry = G->prog->pSharedInfo->aheList[ i ];
		struct hook *hook = NULL;
		
		if( G->config->verbose >= 9 )
		{
			printf( "\n*G->prog->pcHandleEntries: %lu\n", *G->prog->pcHandleEntries );
			printf( "Now printing G->prog->pSharedInfo->aheList[ %u ]\n", i );
			print_HANDLEENTRY( &entry );
		}
		
		if( entry.bType != TYPE_HOOK ) /* not for a HOOK object */
			continue;
		
		/* Check to see if the HOOK is located on a desktop we're attached to */
		for( item = store->head; item; item = item->next )
		{
			if( ( (void *)entry.pHead >= item->desktop->pvDesktopBase )
				&& ( (void *)entry.pHead < item->desktop->pvDesktopLimit )
			) /* The HOOK is on an accessible desktop */
				break;
		}
		
		if( !item ) /* The HOOK is on an inaccessible desktop */
		{
			if( G->config->verbose >= 9 )
				printf( "The above HANDLEENTRY points to a HOOK on an inaccessible desktop.\n" );
			
			continue;
		}
		else
		{
			if( G->config->verbose >= 9 )
			{
				printf( "The above HANDLEENTRY points to a HOOK on desktop '%ls'.\n", 
					item->desktop->pwszDesktopName 
				);
			}
		}
		
		hook = &item->hook[ item->hook_count ];
		
		hook->entry_index = i;
		hook->entry = entry;
		
		/* copy the HOOK struct from the desktop heap.
		the info may change so it can't just be pointed to.
		*/
		hook->object = 
			*(HOOK *)( (size_t)hook->entry.pHead - (size_t)item->desktop->pvClientDelta );
		
		/* search the gui threads to find the owner origin and target of the HOOK */
		hook->owner = find_Win32ThreadInfo( parent, hook->entry.pOwner );
		hook->origin = find_Win32ThreadInfo( parent, hook->object.pti );
		hook->target = find_Win32ThreadInfo( parent, hook->object.ptiHooked );
		
		item->hook_count++;
		if( item->hook_count >= item->hook_max )
		{
			MSG_ERROR( "Too many HOOK objects!" );
			printf( "item->hook_count: %u\n", item->hook_count );
			printf( "item->hook_max: %u\n", item->hook_max );
			
			if( item->hook_count > item->hook_max )
			{
				printf( "Setting hook_count to hook_max.\n" );
				item->hook_count = item->hook_max;
			}
			
			return FALSE;
		}
	}
	
	
	/* sort the hook array for each desktop according to its position in the heap */
	for( item = store->head; item; item = item->next )
	{
		/* sort according to HANDLEENTRY's entry.pHead */
		qsort( 
			item->hook, 
			item->hook_count, 
			sizeof( *item->hook ), 
			compare_hook
		);
		
		/* search for invalid or duplicate entry.pHead */
		for( i = 1; i < item->hook_count; ++i )
		{
			const struct hook *const a = &item->hook[ i  - 1 ];
			const struct hook *const b = &item->hook[ i ];
			
			
			if( a->entry.pHead == b->entry.pHead )
			{
				MSG_ERROR( "Duplicate pHead." );
				print_hook( a );
				print_hook( b );
				return FALSE;
			}
			
			if( !a->entry.pHead )
			{
				MSG_ERROR( "Invalid pHead." );
				print_hook( a );
				return FALSE;
			}
			
			if( !b->entry.pHead )
			{
				MSG_ERROR( "Invalid pHead." );
				print_hook( b );
				return FALSE;
			}
		}
	}
	
	
	/* the desktop hook store has been initialized */
	GetSystemTimeAsFileTime( (FILETIME *)&store->init_time );
	return TRUE;
}



/* print_hook()
Print a hook struct.

if 'hook' is NULL this function returns without having printed anything.
*/
void print_hook(
	const struct hook *const hook   // in
)
{
	const char *const objname = "hook struct";
	
	
	if( !hook )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	printf( "\nhook->entry_index: %u\n", hook->entry_index );
	print_HANDLEENTRY( &hook->entry );
	
	print_HOOK( &hook->object );
	
	if( hook->owner )
	{
		printf( "\nhook->owner GUI info:\n" );
		print_gui( hook->owner );
	}
	
	if( hook->origin )
	{
		printf( "\nhook->origin GUI info:\n" );
		print_gui( hook->origin );
	}
	
	if( hook->target )
	{
		printf( "\nhook->target GUI info:\n" );
		print_gui( hook->target );
	}
	
	printf( "\n" );
	PRINT_SEP_END( objname );
	
	return;
}



/* print_hook_array()
Print a desktop hook item's array of hook structs.

if 'item' is NULL this function returns without having printed anything.
*/
void print_hook_array(
	const struct desktop_hook_item *const item   // in
)
{
	const char *const objname = "array of hook structs";
	unsigned i = 0;
	
	
	if( !item )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	printf( "item->hook_max: %u\n", item->hook_max );
	printf( "item->hook_count: %u\n", item->hook_count );
	
	if( item->hook )
	{
		for( i = 0; ( ( i < item->hook_count ) && ( i < item->hook_max ) ); ++i )
			print_hook( &item->hook[ i ] );
	}
	else
	{
		printf( "item->hook: NULL\n" );
	}
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_desktop_hook_item()
Print an item from a desktop hook store's linked list.

if 'item' is NULL this function returns without having printed anything.
*/
void print_desktop_hook_item( 
	const struct desktop_hook_item *const item   // in
)
{
	const char *const objname = "Desktop Hook Item";
	
	
	if( !item )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	if( item->desktop )
		printf( "item->desktop->pwszDesktopName: %ls\n", item->desktop->pwszDesktopName );
	else
		MSG_ERROR( "item->desktop == NULL" );
	
	print_hook_array( item );
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_desktop_hook_store()
Print a desktop hook store and all its descendants.

if 'store' is NULL this function returns without having printed anything.
*/
void print_desktop_hook_store( 
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
		printf( "\n" );
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
