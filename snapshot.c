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
This file contains functions for a snapshot store (system process info, gui threads and hooks).
Each function is documented in the comment block above its definition.

The snapshot store(s) depend on all the other global stores.

-
create_snapshot_store()

Create a snapshot store and its descendants or die.
-

-
free_snapshot_store()

Free a snapshot store and all its descendants.
-

*/

#include <stdio.h>

#include "util.h"

#include "snapshot.h"

/* the global stores */
#include "global.h"

/* traverse_threads() */
#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"



/* create_snapshot_store()
Create a snapshot store and its descendants or die.

The snapshot store holds hooks, gui thread info and system thread info recorded consecutively.
*/
void create_snapshot_store( 
	struct snapshot **const out   // out deref
)
{
	struct snapshot *snapshot = NULL;
	
	FAIL_IF( !out );
	FAIL_IF( *out );
	
	
	/* allocate a snapshot store */
	snapshot = must_calloc( 1, sizeof( *snapshot ) );
	
	
	/* the allocated/maximum number of elements in the array pointed to by gui.
	this is also the maximum number of threads that can be handled in this snapshot.
	
	between the gui array and SYSTEM_PROCESS_INFORMATION array , having 20,000 
	elements each is about 7MB total per snapshot which is reasonable. 
	so for now the maximum threads that can be handled is 20,000.
	*/
	snapshot->gui_max = 20000;
	
	/* allocate an array of gui structs */
	snapshot->gui = 
		must_calloc( snapshot->gui_max, sizeof( *snapshot->gui ) );
	
	
	/* the allocated size of the buffer in bytes.
	
	when traverse_threads() is called how much memory is needed depends on 
	how many threads in the system, the thread process ratio and whether extended process 
	information was requested.  because this information is constantly changing 
	depending on the state of the system, and to avoid too many allocations and frees, 
	i'm using one big buffer that can be continually refilled.
	
	the size is calculated based on the worst-case scenario of one thread per process.
	eg 20k max threads is about a 6.5MB buffer
	*/
	snapshot->spi_max_bytes = 
	( 
		snapshot->gui_max 
		* ( sizeof( SYSTEM_PROCESS_INFORMATION ) 
			+ sizeof( SYSTEM_EXTENDED_THREAD_INFORMATION ) 
		)
	);
	
	/* allocate the buffer
	the buffer is read and written by traverse_threads().
	the buffer contains the SYSTEM_PROCESS_INFORMATION array for the snapshot.
	*/
	snapshot->spi = must_calloc( snapshot->spi_max_bytes, 1 );
	
	
	create_desktop_hook_store( &snapshot->desktop_hooks );
	
	
	*out = snapshot;
	return;
}


/* print_gui()
Print a gui struct.

if the gui struct pointer is != NULL then print the gui struct
*/
void print_gui(
	const struct gui *const gui   // in
)
{
	const char *const objname = "GUI Thread Info";
	
	
	if( !gui )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	PRINT_PTR( gui->pvWin32ThreadInfo );
	PRINT_PTR( gui->pvTeb );
	
	/* This uses my default callback for traverse_threads() to print some very basic information 
	from the gui's associated system process info (spi) and system thread info (sti).
	eg
	procexp.exe: PID 5860, TID 4076 state Waiting (UserRequest).
	CreateTime: 12:45:24 PM  9/7/2011
	*/
	if( gui->spi )
		callback_print_thread_state( &G->prog->dwOSVersion, spi, sti, 0, 0 );
	else
		MSG_ERROR( "gui->spi == NULL" );
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_snapshot_store()
Print a snapshot store and all its descendants.

if the snapshot store pointer != NULL print the store
*/
static void print_snapshot_store( 
	const struct snapshot *const store   // in
)
{
	const char *const objname = "Snapshot Store";
	DWORD flags = 0;
	int i = 0, ret = 0;
	///asdf
	
	if( !store )
		return;
	
	PRINT_DBLSEP_BEGIN( objname );
	printf( "store->initialized: %s\n", ( store->initialized ? "TRUE" : "FALSE" ) );
	
	
	/* traverse_threads() should use as input the output buffer from a previous call (store->spi) */
	flags |= TRAVERSE_FLAG_RECYCLE;
	
	/* also call the callback for any process info reporting zero threads */
	flags |= TRAVERSE_FLAG_ZERO_THREADS_OK;
	
	/* If the flag TRAVERSE_FLAG_EXTENDED was passed in for the original call it must also be 
	passed in on a recycle call.
	*/
	if( store->spi_extended )
		flags |= TRAVERSE_FLAG_EXTENDED;
	
	/* print some basic information from the spi array */
	ret = traverse_threads( 
		NULL, /* use the default callback to print some basic information */
		NULL, /* an optional pointer to pass to the callback. if default this isn't used */
		store->spi, /* the spi buffer */
		store->spi_max_bytes, /* spi buffer's byte count */
		flags, /* flags */
		NULL /* pointer to receive status. unused here */
	);
	if( ret )
	{
		MSG_ERROR( "traverse_threads() failed to print the spi array." );
		printf( "traverse_threads() returned: %s\n", traverse_threads_retcode_to_cstr( ret ) );
	}
	
	
	printf( "store->gui_max: %u\n", store->gui_max );
	printf( "store->gui_count: %u\n", store->gui_count );
	for( i = 0; i < store->gui_count; ++i )
		print_gui( store->gui );
	
	print_desktop_hook_store( store->desktop_hooks );
	
	PRINT_DBLSEP_END( objname );
	
	return;
}



/* free_snapshot_store()
Free a snapshot store and all its descendants.

this function then sets the snapshot store pointer to NULL and returns

'in' is a pointer to a pointer to the snapshot store, which contains the snapshot information.
if( !in || !*in ) then this function returns.
*/
void free_snapshot_store( 
	struct snapshot **const in   // in deref
)
{
	if( !in || !*in )
		return;
	
	free_desktop_hook_store( &(*in)->desktop_hooks );
	
	free( (*in)->gui );
	
	free( (*in)->spi );
	
	free( (*in) );
	*in = NULL;
	
	return;
}
