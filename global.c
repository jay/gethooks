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
This file contains functions for creating the global store 'G', which is the store of global stores.
Each function is documented in the comment block above its definition.

There are several global stores (these are referred to as descendants of 'G'):
'G->prog' is the global program store. It holds basic program and system info.
'G->config' is the global configuration store. It holds the user's configuration.
'G->desktops' is the global desktop store. It holds the list of attached to desktops.

Each of the global stores and their functions are defined in their own units, eg prog.h/prog.c

-
create_global_store()

Create the global store and its descendants or die.
-

-
print_global_store()

Print the global store and all its descendants.
-

-
free_global_store()

Free the global store and all its descendants.
-

*/

#include <stdio.h>

#include "util.h"

#include "global.h"



/* A pointer to this process' global store. The global store is the store of global stores. */
struct global *G;   // create_global_store(), free_global_store()



/* create_global_store()
Create the global store and its descendants or die.

Everything that must be available globally descends from the global store.
 */
void create_global_store( void )
{
	FAIL_IF( G );   // Fail if the global store already exists
	
	
	/* the global store (the store of global stores) */
	G = must_calloc( 1, sizeof( *G ) );
	
	/* program store (command line arguments, OS version, etc) */
	create_prog_store( &G->prog );
	
	/* configuration store (user-specified command line configuration) */
	create_config_store( &G->config );
	
	/* desktop store (linked list of desktops' heap and thread info) */
	create_desktop_store( &G->desktops );
	
	
	return;
}



/* print_global_store()
Print the global store and all its descendants.
*/
void print_global_store( void )
{
	if( !G )
		return;
	
	printf( "\n" );
	print_global_prog_store();
	printf( "\n" );
	print_global_config_store();
	printf( "\n" );
	print_global_desktop_store();
	printf( "\n" );
	
	return;
}



/* free_global_store()
Free the global store and all its descendants.

this function then sets the global store pointer (G) to NULL and returns
if( !G ) then this function returns.
*/
void free_global_store( void )
{
	if( !G )
		return;
	
	free_desktop_store( &G->desktops );
	
	free_config_store( &G->config );
	
	free_prog_store( &G->prog );
	
	free( G );
	G = NULL;
	
	return;
}
