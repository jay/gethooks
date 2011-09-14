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



/* init_global_desktop_store()
Initialize the global desktop store by attaching to the user-specified or default desktop(s).
Calls add_all_desktops(), or calls add_desktop_item() for each desktop if not adding all.

For now there is only one desktop store implemented and it's a global store (G->desktops).
'G->desktops' depends on the global program (G->prog) and configuration (G->config) stores.
*/
void init_global_desktop_store( void )
{
	FAIL_IF( !G->prog->initialized );   // The program store must already be initialized.
	FAIL_IF( !G->config->initialized );   //  The configuration store must already be initialized.
	
	FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	
	/* if the user's list of desktop names is not initialized then the user did not specify the 'd' 
	option. in this case attempt to attach to all desktops in the current window station.
	*/
	if( !G->config->desklist->initialized )
	{
		G->desktops->type = DESKTOP_ALL;
		
		/* Add all desktops returns the number of accessible desktops now in the list.
		This may or may not be the total number of desktops in the process' window station.
		It's unlikely we'll be able to add *all* desktops in a window station but we should 
		have access to at least one.
		*/
		if( !add_all_desktops( G->desktops ) )
		{
			MSG_FATAL( "add_all_desktops() failed." );
			printf( "Couldn't add any desktops.\n" );
			exit( 1 );
		}
	}
	/* 
	else if the user's list of desktop names is not initialized then the user specified the 'd' 
	option but did not specify any desktop names. in this case use only the current desktop.
	*/
	else if( !G->config->desklist->head )
	{
		G->desktops->type = DESKTOP_CURRENT;
		
		if( !add_desktop_item( G->desktops, NULL ) )
		{
			MSG_FATAL( "add_desktop_item() failed." );
			printf( "Couldn't add the main thread's desktop." );
			exit( 1 );
		}
	}
	else // the user specified desktop names with the 'd' option
	{
		struct list_item *current = NULL;
		
		
		G->desktops->type = DESKTOP_SPECIFIED;
		
		/* for each of the user specified desktop names add to the desktop heap list */
		for( current = G->config->desklist->head; current; current = current->next )
		{
			if( current->name && !add_desktop_item( G->desktops, current->name ) )
			{
				MSG_FATAL( "add_desktop_item() failed." );
				printf( "Couldn't add desktop: %ls\n", current->name );
				exit( 1 );
			}
		}
	}
	
	G->desktops->initialized = TRUE;
	
	/* G->desktops has been initialized */
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
	printf( "store->initialized: %s\n", ( store->initialized ? "TRUE" : "FALSE" ) );
	
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
