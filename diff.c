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
match_hook_process_name()

Match a hook struct's associated GUI threads' process names to the passed in name.
-

-
match_gui_process_pid()

Compare a GUI thread's process id to the passed in process id.
-

-
match_hook_process_pid()

Match a hook struct's associated GUI threads' process pids to the passed in pid.
-

-
is_hook_wanted()

Check the user-specified configuration to determine if the hook struct should be processed.
-

-
print_hook_notice_begin()

Helper function to print a hook [begin] header with basic hook info.
-

-
print_hook_notice_end()

Helper function to print a hook [end] header.
-

-
print_diff_gui()

Compare two gui structs and print any significant differences. Helper function for print_diff_hook()
-

-
print_diff_hook()

Compare two hook structs, both for the same HOOK object, and print any significant differences.
-

-
print_initial_desktop_hook_item()

Print the HOOKs that have been found on a single attached to desktop.
-

-
print_diff_desktop_hook_items()

Print the HOOKs that have been added/removed from a single attached to desktop between snapshots.
-

-
print_initial_desktop_hook_list()

Print the HOOKs that have been found on all attached to desktops for a single snapshot.
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



static void print_hook_notice_begin(
	const struct hook *const hook,   // in
	const WCHAR *const deskname,   // in
	const enum difftype difftype   // in
);

static void print_hook_notice_end( void );

static int print_diff_gui(
	const struct hook *const oldhook,   // in
	const struct hook *const newhook,   // in
	const enum threadtype threadtype,   // in
	const WCHAR *const deskname,   // in
	unsigned *const modified_header   // in, out
);



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
	
	
	if( gui->spi && ( pid == (int)( (DWORD)gui->spi->UniqueProcessId ) ) )
		return TRUE;
	else
		return FALSE;
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
	
	
	if( ( hook->owner && match_gui_process_pid( hook->owner, pid ) )
		|| ( hook->origin && match_gui_process_pid( hook->origin, pid ) )
		|| ( hook->target && match_gui_process_pid( hook->target, pid ) )
	)
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
		)
			return FALSE; // the hook is not wanted
	}
	
	
	/* if there is a list of HOOK ids to include/exclude */
	if( G->config->hooklist->init_time 
		&& ( ( G->config->hooklist->type == LIST_INCLUDE_HOOK )
			|| ( G->config->hooklist->type == LIST_EXCLUDE_HOOK )
		)
	)
	{
		unsigned yes = 0;
		const struct list_item *item = NULL;
		
		
		for( item = G->config->hooklist->head; ( item && !yes ); item = item->next )
			yes = ( item->id == hook->object.iHook ); // match HOOK id
		
		if( ( yes && ( G->config->hooklist->type == LIST_EXCLUDE_HOOK ) )
			|| ( !yes && ( G->config->hooklist->type == LIST_INCLUDE_HOOK ) )
		)
			return FALSE; // the hook is not wanted
	}
	
	return TRUE; // the hook is wanted
}



/* print_hook_notice_begin()
Helper function to print a hook [begin] header with basic hook info.

'hook' is the hook info
'deskname' is the desktop name
'difftype' is the reported action, eg HOOK_ADDED, HOOK_MODIFIED, HOOK_REMOVED
*/
static void print_hook_notice_begin(
	const struct hook *const hook,   // in
	const WCHAR *const deskname,   // in
	const enum difftype difftype   // in
)
{
	const char *diffname = NULL;
	
	FAIL_IF( !hook );
	FAIL_IF( !deskname );
	FAIL_IF( !difftype );
	
	
	PRINT_SEP_BEGIN( "" );
	
	if( difftype == HOOK_FOUND )
		diffname = "Found";
	else if( difftype == HOOK_ADDED )
		diffname = "Added";
	else if( difftype == HOOK_MODIFIED )
		diffname = "Modified";
	else if( difftype == HOOK_REMOVED )
		diffname = "Removed";
	else
	{
		MSG_FATAL( "Unknown diff type." );
		printf( "difftype: %d\n", difftype );
		exit( 1 );
	}
	
	
	printf( "[%s]", diffname );
	
	printf( " [%ls]", deskname );
	
	printf( " [HOOK head " );
	PRINT_BARE_PTR( hook->object.head.h );
	printf( " @ kernel address " );
	PRINT_BARE_PTR( hook->entry.pHead );
	printf( "]" );
	
	printf( " [" );
	print_time();
	printf( "]" );
	
	printf( "\n" );
	
	
	printf( "Id: " );
	print_HOOK_id( hook->object.iHook );
	printf( "\n" );
	
	printf( "Owner: " );
	print_gui_brief( hook->owner );
	printf( "\n" );
	
	printf( "Origin: " );
	print_gui_brief( hook->origin );
	printf( "\n" );
	
	
	printf( "Target: " );
	
	if( hook->object.flags & HF_GLOBAL )
	{
		printf( "<GLOBAL>" );
		if( hook->object.ptiHooked ) /* A global hook should NOT have a target */
		{
			printf( " ERROR: There is target information for this global hook: " );
			print_gui_brief( hook->target );
		}
	}
	else
		print_gui_brief( hook->target );
	
	printf( "\n" );
	
	
	if( difftype == HOOK_MODIFIED )
		printf( "\n" );
	
	return;
}



/* print_hook_notice_end()
Helper function to print a hook [end] header.
*/
static void print_hook_notice_end( void )
{
	PRINT_SEP_END( "" );
	return;
}



/* print_diff_gui()
Compare two gui structs and print any significant differences. Helper function for print_diff_hook()

'oldhook' is the old hook info (optional)
'newhook' is the new hook info (optional)
'threadtype' is the gui thread info in the hook struct to compare eg THREAD_TARGET (hook->target)
'deskname' is the name of the desktop the hook is on

'*modified_header' receives nonzero if the "Modified HOOK" header is printed by this function.
the header is printed before any difference in the gui structs has been printed.
if '*modified_header' is nonzero when this function is called then the header was already printed.

returns nonzero if any difference was printed. 
if this function returns nonzero then '*modified_header' is also nonzero.
*/
static int print_diff_gui(
	const struct hook *const oldhook,   // in
	const struct hook *const newhook,   // in
	const enum threadtype threadtype,   // in
	const WCHAR *const deskname,   // in
	unsigned *const modified_header   // in, out
)
{
	/* oldhook's gui thread owner, origin, or target */
	const struct gui *a = NULL;
	
	/* newhook's gui thread owner, origin, or target */
	const struct gui *b = NULL;
	
	const char *threadname = NULL;
	
	WCHAR empty1[] = L"<unknown>";
	WCHAR empty2[] = L"<unknown>";
	struct
	{
		void *pvWin32ThreadInfo;
		void *pvTeb;
		HANDLE tid;
		HANDLE pid;
		struct
		{
			USHORT Length;
			USHORT MaximumLength;
			PWSTR  Buffer;
		} ImageName;
	} oldstuff, newstuff;
	
	FAIL_IF( !oldhook );
	FAIL_IF( !newhook );
	FAIL_IF( !threadtype );
	FAIL_IF( !deskname );
	FAIL_IF( !modified_header );
	
	
	if( threadtype == THREAD_OWNER )
	{
		threadname = "owner";
		a = oldhook->owner;
		b = newhook->owner;
	}
	else if( threadtype == THREAD_ORIGIN )
	{
		threadname = "origin";
		a = oldhook->origin;
		b = newhook->origin;
	}
	else if( threadtype == THREAD_TARGET )
	{
		threadname = "target";
		a = oldhook->target;
		b = newhook->target;
	}
	else
	{
		MSG_FATAL( "Unknown thread type." );
		printf( "threadtype: %d\n", threadtype );
		exit( 1 );
	}
	
	
	if( !a && !b )
		return FALSE;
	
	ZeroMemory( &oldstuff, sizeof( oldstuff ) );
	ZeroMemory( &newstuff, sizeof( newstuff ) );
	
	oldstuff.ImageName.Buffer = empty1;
	oldstuff.ImageName.Length = (unsigned short)( wcslen( oldstuff.ImageName.Buffer ) * sizeof( WCHAR ) );
	oldstuff.ImageName.MaximumLength = (unsigned short)( oldstuff.ImageName.Length + sizeof( WCHAR ) );
	
	newstuff.ImageName.Buffer = empty2;
	newstuff.ImageName.Length = (unsigned short)( wcslen( newstuff.ImageName.Buffer ) * sizeof( WCHAR ) );
	newstuff.ImageName.MaximumLength = (unsigned short)( newstuff.ImageName.Length + sizeof( WCHAR ) );
	
	/* both oldstuff and newstuff have been initialized empty */
	
	if( a )
	{
		oldstuff.pvWin32ThreadInfo = a->pvWin32ThreadInfo;
		oldstuff.pvTeb = a->pvTeb;
		
		if( a->sti )
			oldstuff.tid = a->sti->ClientId.UniqueThread;
		
		if( a->spi )
		{
			oldstuff.pid = a->spi->UniqueProcessId;
			
			if( a->spi->ImageName.Buffer )
			{
				oldstuff.ImageName.Buffer = a->spi->ImageName.Buffer;
				oldstuff.ImageName.Length = a->spi->ImageName.Length;
				oldstuff.ImageName.MaximumLength = a->spi->ImageName.MaximumLength;
			}
		}
	}
	
	if( b )
	{
		newstuff.pvWin32ThreadInfo = b->pvWin32ThreadInfo;
		newstuff.pvTeb = b->pvTeb;
		
		if( b->sti )
			newstuff.tid = b->sti->ClientId.UniqueThread;
		
		if( b->spi )
		{
			newstuff.pid = b->spi->UniqueProcessId;
			
			if( b->spi->ImageName.Buffer )
			{
				newstuff.ImageName.Buffer = b->spi->ImageName.Buffer;
				newstuff.ImageName.Length = b->spi->ImageName.Length;
				newstuff.ImageName.MaximumLength = b->spi->ImageName.MaximumLength;
			}
		}
	}
	
	
	if( ( oldstuff.pvWin32ThreadInfo == newstuff.pvWin32ThreadInfo )
		&& ( oldstuff.pvTeb == newstuff.pvTeb )
		&& ( oldstuff.tid == newstuff.tid )
		&& ( oldstuff.pid == newstuff.pid )
		&& ( oldstuff.ImageName.Length == newstuff.ImageName.Length )
		&& !wcsncmp( 
			oldstuff.ImageName.Buffer, 
			newstuff.ImageName.Buffer, 
			oldstuff.ImageName.Length
		)
	)
		return FALSE;
	
	
	/* If the modified header has not yet been printed by another function then print it.
	if !*modified_header then this gui diff is the first encountered between the two hook structs.
	*/
	if( !*modified_header )
	{
		print_hook_notice_begin( newhook, deskname, HOOK_MODIFIED );
		*modified_header = TRUE;
	}
	
	
	printf( "\nThe associated gui %s thread information has changed.\n", threadname );
	
	
	printf( "Old %s: ", threadname );
	
	if( ( threadtype == THREAD_TARGET ) && ( oldhook->object.flags & HF_GLOBAL ) )
	{
		printf( "<GLOBAL>" );
		if( oldhook->object.ptiHooked ) /* A global hook should NOT have a target */
		{
			printf( " ERROR: There is target information for this global hook: " );
			print_gui_brief( a );
		}
	}
	else
		print_gui_brief( a );
	
	printf( "\n" );
	
	
	printf( "New %s: ", threadname );
	
	if( ( threadtype == THREAD_TARGET ) && ( newhook->object.flags & HF_GLOBAL ) )
	{
		printf( "<GLOBAL>" );
		if( newhook->object.ptiHooked ) /* A global hook should NOT have a target */
		{
			printf( " ERROR: There is target information for this global hook: " );
			print_gui_brief( b );
		}
	}
	else
		print_gui_brief( b );
	
	printf( "\n" );
	
	
	return TRUE;
}



/* print_diff_hook()
Compare two hook structs, both for the same HOOK object, and print any significant differences.

'a' is the old hook info
'b' is the new hook info
'deskname' is the name of the desktop the HOOK is on

returns nonzero if any difference was printed. 
*/
int print_diff_hook( 
	const struct hook *const a,   // in
	const struct hook *const b,   // in
	const WCHAR *const deskname   // in
)
{
	/* modified_header is set nonzero if/when the "Modified HOOK" header has been printed */
	unsigned modified_header = FALSE;
	
	FAIL_IF( !a );
	FAIL_IF( !b );
	FAIL_IF( !deskname );
	
	
	if( a->entry.bFlags != b->entry.bFlags )
	{
		BYTE temp = 0;
		
		
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe associated HANDLEENTRY's flags have changed.\n" );
		
		temp = (BYTE)( a->entry.bFlags & b->entry.bFlags );
		if( temp )
		{
			printf( "Flags same: " );
			print_HANDLEENTRY_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( a->entry.bFlags & ~b->entry.bFlags );
		if( temp )
		{
			printf( "Flags removed: " );
			print_HANDLEENTRY_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( b->entry.bFlags & ~a->entry.bFlags );
		if( temp )
		{
			printf( "Flags added: " );
			print_HANDLEENTRY_flags( temp );
			printf( "\n" );
		}
	}
	
	/* the gui->owner struct has the process and thread info for entry.pOwner */
	print_diff_gui( a, b, THREAD_OWNER, deskname, &modified_header );
	
	if( a->object.head.h != b->object.head.h )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's handle has changed.\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.head.h );
		PRINT_NAME_FOR_PTR( "New", b->object.head.h );
	}
	
	if( a->object.head.cLockObj != b->object.head.cLockObj )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's lock count has changed.\n" );
		printf( "Old: %lu\n", a->object.head.cLockObj );
		printf( "New: %lu\n", b->object.head.cLockObj );
	}
	
	/* the gui->origin struct has the process and thread info for pti */
	print_diff_gui( a, b, THREAD_ORIGIN, deskname, &modified_header );
	
	if( a->object.rpdesk1 != b->object.rpdesk1 )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nrpdesk1 has changed. The desktop that the HOOK is on has changed?\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.rpdesk1 );
		PRINT_NAME_FOR_PTR( "New", b->object.rpdesk1 );
	}
	
	if( a->object.pSelf != b->object.pSelf )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's kernel address has changed.\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.pSelf );
		PRINT_NAME_FOR_PTR( "New", b->object.pSelf );
	}
	
	if( a->object.phkNext != b->object.phkNext )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's chain has been modified.\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.phkNext );
		PRINT_NAME_FOR_PTR( "New", b->object.phkNext );
	}
	
	if( a->object.iHook != b->object.iHook )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's id has changed.\n" );
		
		printf( "Old: " );
		print_HOOK_id( a->object.iHook );
		printf( "\n" );
		
		printf( "New: " );
		print_HOOK_id( b->object.iHook );
		printf( "\n" );
	}
	
	if( a->object.offPfn != b->object.offPfn )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's function offset has changed.\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.offPfn );
		PRINT_NAME_FOR_PTR( "New", b->object.offPfn );
	}
	
	if( a->object.flags != b->object.flags )
	{
		BYTE temp = 0;
		
		
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's flags have changed.\n" );
		
		temp = (BYTE)( a->object.flags & b->object.flags );
		if( temp )
		{
			printf( "Flags same: " );
			print_HOOK_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( a->object.flags & ~b->object.flags );
		if( temp )
		{
			printf( "Flags removed: " );
			print_HOOK_flags( temp );
			printf( "\n" );
		}
		
		temp = (BYTE)( b->object.flags & ~a->object.flags );
		if( temp )
		{
			printf( "Flags added: " );
			print_HOOK_flags( temp );
			printf( "\n" );
		}
	}
	
	if( a->object.ihmod != b->object.ihmod )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's function module atom index has changed.\n" );
		printf( "Old: %d\n", a->object.ihmod );
		printf( "New: %d\n", b->object.ihmod );
	}
	
	/* the gui->target struct has the process and thread info for ptiHooked */
	print_diff_gui( a, b, THREAD_TARGET, deskname, &modified_header );
	
	if( a->object.rpdesk2 != b->object.rpdesk2 )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nrpdesk2 has changed. HOOK locked, owner destroyed?\n" );
		PRINT_NAME_FOR_PTR( "Old", a->object.rpdesk2 );
		PRINT_NAME_FOR_PTR( "New", b->object.rpdesk2 );
	}
	
	
	if( modified_header )
		print_hook_notice_end();
	
	return !!modified_header;
}



/* print_initial_desktop_hook_item()
Print the HOOKs that have been found on a single attached to desktop.
*/
void print_initial_desktop_hook_item( 
	const struct desktop_hook_item *const b   // in
)
{
	unsigned i = 0;
	
	FAIL_IF( !b );
	FAIL_IF( !b->desktop );
	FAIL_IF( !b->hook_max );
	
	
	for( i = 0; i < b->hook_count; ++i )
	{
		if( is_hook_wanted( &b->hook[ i ] ) )
		{
			print_hook_notice_begin( &b->hook[ i ], b->desktop->pwszDesktopName, HOOK_FOUND );
			print_hook_notice_end();
		}
	}
	
	return;
}



/* print_diff_desktop_hook_items()
Print the HOOKs that have been added/removed from a single attached to desktop between snapshots.

'a' is an item from the desktop hook list of the previous snapshot
'b' is an item from the desktop hook list of the current snapshot
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
			if( is_hook_wanted( &a->hook[ a_hi ] ) )
			{
				print_hook_notice_begin( &a->hook[ a_hi ], deskname, HOOK_REMOVED );
				print_hook_notice_end();
			}
			
			++a_hi;
		}
		else if( ret > 0 ) // hook added
		{
			if( is_hook_wanted( &b->hook[ b_hi ] ) )
			{
				print_hook_notice_begin( &b->hook[ b_hi ], deskname, HOOK_ADDED );
				print_hook_notice_end();
			}
			
			++b_hi;
		}
		else
		{
			/* The hook info exists in both snapshots (same HOOK object).
			In this case check there is no reason to print the HOOK again unless certain 
			information has changed (like the hook is hung, etc).
			*/
			if( is_hook_wanted( &a->hook[ a_hi ] ) || is_hook_wanted( &b->hook[ b_hi ] ) )
				print_diff_hook( &a->hook[ a_hi ], &b->hook[ b_hi ], deskname );
			
			++a_hi;
			++b_hi;
		}
	}
	
	while( a_hi < a->hook_count ) // hooks removed
	{
		if( is_hook_wanted( &a->hook[ a_hi ] ) )
		{
			print_hook_notice_begin( &a->hook[ a_hi ], deskname, HOOK_REMOVED );
			print_hook_notice_end();
		}
		
		++a_hi;
	}
	
	while( b_hi < b->hook_count ) // hooks added
	{
		if( is_hook_wanted( &b->hook[ b_hi ] ) )
		{
			print_hook_notice_begin( &b->hook[ b_hi ], deskname, HOOK_ADDED );
			print_hook_notice_end();
		}
		
		++b_hi;
	}
	
	return;
}



/* print_initial_desktop_hook_list()
Print the HOOKs that have been found on all attached to desktops for a single snapshot.
*/
void print_initial_desktop_hook_list( 
	const struct desktop_hook_list *const list2   // in
)
{
	struct desktop_hook_item *b = NULL;
	
	FAIL_IF( !list2 );
	
	
	for( b = list2->head; b; b = b->next )
		print_initial_desktop_hook_item( b );
	
	return;
}



/* print_diff_desktop_hook_lists()
Print the HOOKs that have been added/removed from all attached to desktops between snapshots.

'list1' is the previous snapshot's desktop hook list
'list2' is the current snapshot's desktop hook list
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


