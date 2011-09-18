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
This file contains gethooks(). 
It is documented in the comment block above its definition.
*/

#include <stdio.h>
#include <windows.h>

#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"

#include "nt_stuff.h"

#include "gethooks.h"



static void diff( 
	struct desktop_hook_item *a,
	struct desktop_hook_item *b
)
{
	unsigned a_hi = 0, b_hi = 0;
	
	FAIL_IF( !a );
	FAIL_IF( !b );
	FAIL_IF( a->desktop != b->desktop );
	FAIL_IF( a->hook_max != b->hook_max );
	
	
	a_hi = 0, b_hi = 0;
	while( ( a_hi < a->hook_count ) && ( b_hi < b->hook_count ) )
	{
		int ret = compare_hook( &a->hook[ a_hi ], &b->hook[ b_hi ] );
		
		if( ret < 0 ) // hook removed
		{
			if( match( &a->hook[ a_hi ] ) )
			{
				printf( "HOOK removed from desktop %ls.\n", a->desktop->pwszDesktopName );
				print_hook_simple( &a->hook[ a_hi ] );
			}
			
			++a_hi;
		}
		else if( ret > 0 ) // hook added
		{
			if( match( &b->hook[ b_hi ] ) )
			{
				printf( "HOOK added to desktop %ls.\n", b->desktop->pwszDesktopName );
				print_hook_simple( &b->hook[ b_hi ] );
			}
			
			++b_hi;
		}
		else // hook same
		{
			++a_hi;
			++b_hi;
		}
	}
	
	while( a_hi < a->hook_count ) // hooks removed
	{
		printf( "HOOK removed from desktop %ls.\n", a->desktop->pwszDesktopName );
		print_hook_simple( &a->hook[ a_hi ] );
		
		++a_hi;
	}
	
	while( b_hi < b->hook_count ) // hooks added
	{
		printf( "HOOK added to desktop %ls.\n", b->desktop->pwszDesktopName );
		print_hook_simple( &b->hook[ b_hi ] );
		
		++b_hi;
	}
	
	return;
}



void compare_hooks( 
	struct desktop_hook_list *dh_list1,   // in
	struct desktop_hook_list *dh_list2   // in
)
{
	struct desktop_hook_item *dh_item1 = NULL;
	struct desktop_hook_item *dh_item2 = NULL;
	
	
	/* make sure both desktop hook stores have a list of the same desktops */
	dh_item1 = dh_list1->head;
	dh_item2 = dh_list2->head;
	while( 
		( dh_item1 && dh_item2 ) 
		&& ( dh_item1->desktop == dh_item2->desktop ) 
		&& ( dh_item
	)
	{
		dh_item1 = dh_item1->next;
		dh_item2 = dh_item2->next;
	}
	
	if( dh_item1 || dh_item2 )
		MSG_FATAL( "The desktop hook stores cannot be compared." );
	
	
	dh_item1 = dh_list1->head;
	dh_item2 = dh_list2->head;
	while( dh_item1 && dh_item2 )
	{
		hookdiff( dh_item1, dh_item2 );
	}
	
	return;
}


