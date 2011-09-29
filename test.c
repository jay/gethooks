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
This file contains functions used for testing purposes.
Each function is documented in the comment block above its definition.

-
print_handle_count()

print (and optionally poll) user handle counts of free, invalid, valid, menu, and generic handles.
-

*/

#include <stdio.h>

#include "util.h"

#include "reactos.h"

#include "test.h"

/* the global stores */
#include "global.h"




/* print_handle_count()
print (and optionally poll) user handle counts of free, invalid, valid, menu, and generic handles.

if 'seconds' > 0 poll at interval 
if 'seconds' == 0 do not poll
if 'seconds' < 0 fatal
*/
void print_handle_count( 
	int seconds   // in
)
{
	FAIL_IF( seconds < 0 );
	
	
	if( seconds > 0 )
		printf( "Polling user handle counts every %d seconds.\n", seconds );
	
	for( ;; )
	{
		unsigned i = 0, cMenu = 0, cFree = 0, cValid = 0, cInvalid = 0, cGeneric = 0;
		
		printf( "*G->prog->pcHandleEntries: %lu\n", *G->prog->pcHandleEntries );
		for( i = 0; i < *G->prog->pcHandleEntries; ++i )
		{
			HANDLEENTRY *entry = &G->prog->pSharedInfo->aheList[ i ];
			
			if( entry->bType == TYPE_MENU )
				++cMenu;
			
			if( entry->bType == TYPE_FREE )
				++cFree;
			else if( entry->bType <= TYPE_CTYPES )
				++cValid;
			else if( entry->bType < TYPE_GENERIC )
				++cInvalid;
			else
				++cGeneric;
		}
		printf( 
			"Free: %d\tMenu: %d\tValid: %d\tInvalid: %d\tGeneric: %d\n",
			cFree, cMenu, cValid, cInvalid, cGeneric
		);
		printf( "\n" );
		
		if( seconds <= 0 )
			break;
		
		Sleep( seconds * 1000 );
	}
	
	return;
}



/* testmode()
run user-specified tests
*/
void testmode( void )
{
	FAIL_IF( !G );   // The global store must exist.
	FAIL_IF( !G->prog->init_time );   // The program store must be initialized.
	FAIL_IF( !G->config->init_time );   // The configuration store must be initialized.
	FAIL_IF( !G->desktops->init_time );   // The desktop store must be initialized.
	
	FAIL_IF( !G->config->testlist->init_time );   // The testlist must be initialized.
	
	/*
	if( !G->config->testlist->head ) // no specific tests were specified. run all.
		run_all_tests
	*/
	
	
	
	
	return;
}
