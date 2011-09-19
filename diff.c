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
This file contains functions for comparing two snapshots for differences in hook information.
Each function is documented in the comment block above its definition.

-
match_gui_process_name()

Compare a GUI thread's process name to the passed in name.
-

-
match_gui_process_pid()

Compare a GUI thread's process id to the passed in process id.
-

-
is_hook_wanted()

Check the user-specified configuration to determine if the hook struct should be processed.
-

-
match_hook_process_pid()

Match a hook struct's associated GUI threads' process pids to the passed in pid.
-

-
match_hook_process_name()

Match a hook struct's associated GUI threads' process names to the passed in name.
-

-
print_diff_desktop_hook_items()

Print the HOOKs that have been added/removed from a single attached to desktop between snapshots.
-

-
print_diff_desktop_hook_lists()

Print the HOOKs that have been added/removed from all attached to desktops between snapshots.
-

*/

#include <stdio.h>

#include "util.h"

#include "reactos.h"

#include "diff.h"

/* the global stores */
#include "global.h"



/* match_gui_process_name()
Compare a GUI thread's process name to the passed in name.

returns nonzero on success ('name' matches the GUI thread's process name)
*/
int match_gui_process_name(
	const struct gui *const gui,   // in
	const WCHAR *const name   // in
)
{
	FAIL_IF( !gui );
	FAIL_IF( !name );
	
	
	if( gui->spi 
		&& gui->spi->ImageName.Buffer 
		&& !_wcsicmp( gui->spi->ImageName.Buffer, name )
	)
		return TRUE;
	else
		return FALSE;
}



/* match_gui_process_pid()
Compare a GUI thread's process id to the passed in process id.

returns nonzero on success ('pid' matches the GUI thread's process id)
*/
int match_gui_process_pid(
	const struct gui *const gui,   // in
	const int pid   // in
)
{
	FAIL_IF( !gui );
	FAIL_IF( !pid );
	
	
	if( gui->spi && ( pid == (int)( (DWORD)gui->spi->UniqueProcessId ) ) )
		return TRUE;
	else
		return FALSE;
}



/* is_hook_wanted()
Check the user-specified configuration to determine if the hook struct should be processed.

The user can filter hooks (eg WH_MOUSE) and programs (eg notepad.exe).

returns nonzero if the hook struct should be processed
*/
int is_hook_wanted( 
	const struct hook *const hook   // in
)
{
	FAIL_IF( !hook );
	
	
	/* if there is a list of programs to include/exclude */
	if( G->config->proglist->init_time 
		&& ( ( G->config->proglist->type == LIST_INCLUDE_PROG )
			|| ( G->config->proglist->type == LIST_EXCLUDE_PROG )
		)
	)
	{
		unsigned yes = 0;
		const struct list_item *item = NULL;
		
		
		for( item = G->config->proglist->head; ( item && !yes ); item = item->next )
		{
			if( item->name ) // match program name
				yes = !!match_hook_process_name( hook, item->name );
			else // match program id
				yes = !!match_hook_process_pid( hook, item->id );
		}
		
		if( ( yes && ( G->config->proglist->type == LIST_EXCLUDE_PROG ) )
			|| ( !yes && ( G->config->proglist->type == LIST_INCLUDE_PROG ) )
				return FALSE; // the hook is not wanted
	}
	
	
	/* if there is a list of hooks to include/exclude */
	if( G->config->hooklist->init_time 
		&& ( ( G->config->hooklist->type == LIST_INCLUDE_HOOK )
			|| ( G->config->hooklist->type == LIST_EXCLUDE_HOOK )
		)
	)
	{
		unsigned yes = 0;
		const struct list_item *item = NULL;
		
		
		for( item = G->config->hooklist->head; ( item && !yes ); item = item->next )
			yes = ( item->id == hook->object->iHook );
		
		if( ( yes && ( G->config->hooklist->type == LIST_EXCLUDE_HOOK ) )
			|| ( !yes && ( G->config->hooklist->type == LIST_INCLUDE_HOOK ) )
				return FALSE; // the hook is not wanted
	}
	
	return TRUE; // the hook is wanted
}



/* match_hook_process_pid()
Match a hook struct's associated GUI threads' process pids to the passed in pid.

returns nonzero on success ('pid' matched one of the hook struct's GUI thread process pids)
*/
int match_hook_process_pid(
	const struct hook *const hook,   // in
	const int pid   // in
)
{
	FAIL_IF( !hook );
	FAIL_IF( !name );
	
	
	if( ( hook->owner && match_gui_process_pid( hook->owner, pid ) )
		|| ( hook->origin && match_gui_process_pid( hook->origin, pid ) )
		|| ( hook->target && match_gui_process_pid( hook->target, pid ) )
	)
		return TRUE;
	else
		return FALSE;
}



/* match_hook_process_name()
Match a hook struct's associated GUI threads' process names to the passed in name.

returns nonzero on success ('name' matched one of the hook struct's GUI thread process names)
*/
int match_hook_process_name(
	const struct hook *const hook,   // in
	const WCHAR *const name   // in
)
{
	FAIL_IF( !hook );
	FAIL_IF( !name );
	
	
	if( ( hook->owner && match_gui_process_name( hook->owner, name ) )
		|| ( hook->origin && match_gui_process_name( hook->origin, name ) )
		|| ( hook->target && match_gui_process_name( hook->target, name ) )
	)
		return TRUE;
	else
		return FALSE;
}



/* print_diff_hook()
Compare two hook structs, both for the same HOOK object, and print any significant differences.

'a' is the old hook info
'b' is the new hook info
'deskname' is the name of the desktop the HOOK is on
*/
void print_diff_hook
	const struct hook *const a,   // in
	const struct hook *const b,   // in
	const WCHAR *const deskname   // in
)
{
	FAIL_IF( !a );
	FAIL_IF( !b );
	FAIL_IF( !deskname );
	
	notice printf( "HOOK object modified on desktop %ls:\n", deskname );
	
	if( a->entry.bFlags != b->entry.bFlags )
	{
		BYTE temp = 0;
		
		
		printf( "The HANDLEENTRY's flags have changed.\n" );
		
		temp = (BYTE)( a->entry.bFlags & ~b->entry.bFlags );
		if( temp )
		{
			printf( "Removed: " );
			print_HANDLEENTRY_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( b->entry.bFlags & ~a->entry.bFlags );
		if( temp )
		{
			printf( "Added: " );
			print_HANDLEENTRY_flags( temp );
			printf( "\n" );
		}
	}
	
	if( a->object.flags != b->object.flags )
	{
		BYTE temp = 0;
		
		
		printf( "The HOOK's flags have changed.\n" );
		
		temp = (BYTE)( a->object.flags & ~b->object.flags );
		if( temp )
		{
			printf( "Removed: " );
			print_HOOK_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( b->object.flags & ~a->object.flags );
		if( temp )
		{
			printf( "Added: " );
			print_HOOK_flags( temp );
			printf( "\n" );
		}
	}
	
	if( ( a->object.offPfn != b->object.offPfn )
	{
		printf( "The offset of the HOOK's function has changed.\n" );
		
		printf( "Old: 0x%08lX\tNew: 0x%08lX", a->object.offPfn, b->object.offPfn
		printf( "Old: 0x%08lX\tNew: 0x%08lX", a->object.offPfn, b->object.offPfn
	}
	
	return;
}



/* print_diff_desktop_hook_items()
Print the HOOKs that have been added/removed from a single attached to desktop between snapshots.
*/
void print_diff_desktop_hook_items( 
	const struct desktop_hook_item *const a,   // in
	const struct desktop_hook_item *const b   // in
)
{
	WCHAR *deskname = NULL;
	unsigned a_hi = 0, b_hi = 0;
	
	FAIL_IF( !a );
	FAIL_IF( !b );
	
	/* Both desktop hook items should have a pointer to the same desktop item */
	FAIL_IF( !a->desktop );
	FAIL_IF( !b->desktop );
	FAIL_IF( a->desktop != b->desktop );
	FAIL_IF( a->hook_max != b->hook_max );
	
	
	deskname = b->desktop->pwszDesktopName;
	
	a_hi = 0, b_hi = 0;
	while( ( a_hi < a->hook_count ) && ( b_hi < b->hook_count ) )
	{
		int ret = compare_hook( &a->hook[ a_hi ], &b->hook[ b_hi ] );
		
		if( ret < 0 ) // hook removed
		{
			if( match( &a->hook[ a_hi ] ) )
			{
				printf( "HOOK object removed from desktop %ls:\n", deskname );
				print_basic_hook_info( &a->hook[ a_hi ] );
			}
			
			++a_hi;
		}
		else if( ret > 0 ) // hook added
		{
			if( match( &b->hook[ b_hi ] ) )
			{
				printf( "HOOK object added to desktop %ls:\n", deskname );
				print_basic_hook_info( &b->hook[ b_hi ] );
			}
			
			++b_hi;
		}
		else
		{
			/* The hook info exists in both snapshots (same HOOK object).
			In this case check there is no reason to print the HOOK again unless certain 
			information has changed (like the hook is hung, etc).
			*/
			print_diff_hook( &a->hook[ a_hi ], &b->hook[ b_hi ], deskname );
			
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


/* print_diff_desktop_hook_lists()
Print the HOOKs that have been added/removed from all attached to desktops between snapshots.
*/
void print_diff_desktop_hook_lists( 
	const struct desktop_hook_list *const list1,   // in
	const struct desktop_hook_list *const list2   // in
)
{
	struct desktop_hook_item *a = NULL;
	struct desktop_hook_item *b = NULL;
	
	FAIL_IF( !list1 );
	FAIL_IF( !list2 );
	
	
	for( a = list1->head, b = list2->head; ( a && b ); a = a->next, b = b->next )
		print_diff_desktop_hook_items( a, b );
	
	if( a || b )
	{
		MSG_FATAL( "The desktop hook stores could not be fully compared." );
		exit( 1 );
	}
	
	return;
}


