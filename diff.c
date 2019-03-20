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
print_unknown_address()

Print a pointed to address as unknown. No newline.
-

-
print_brief_thread_info()

Print the associated owner, origin or target thread of a HOOK.
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
print_diff_desktop_hook_items()

Print the HOOKs that have been added/removed/modified from a single desktop between snapshots.
-

-
print_diff_desktop_hook_lists()

Print the HOOKs that have been added/removed/modified from all desktops between snapshots.
-

-
print_initial_desktop_hook_item()

Print the HOOKs that have been found on a single desktop in an initial snapshot.
-

-
print_initial_desktop_hook_list()

Print the HOOKs that have been found on all desktops in an initial snapshot.
-

*/

#include <stdio.h>

#include "util.h"

#include "reactos.h"

#include "diff.h"

/* the global stores */
#include "global.h"



static void print_unknown_address(
	const void *const address   // in, optional
);

static int print_diff_gui(
	const struct hook *const oldhook,   // in
	const struct hook *const newhook,   // in
	const enum threadtype threadtype,   // in
	const WCHAR *const deskname,   // in
	unsigned *const modified_header   // in, out
);



/* print_unknown_address()
Print a pointed to address as unknown. No newline.

This is a helper function that is called when the user mode process and thread info associated with 
a kernel address could not be determined. In that case the thread and process info is unknown.

This function will print any 'address' as unknown including NULL.
eg
<unknown> (<unknown> @ 0xFDB08008)
*/
static void print_unknown_address(
	const void *const address   // in, optional
)
{
	printf( " <unknown> (<unknown> @ " );
	PRINT_HEX_BARE( address );
	printf( ")" );
	
	return;
}



/* print_brief_thread_info()
Print the associated owner, origin or target thread of a HOOK.
*/
void print_brief_thread_info(
	const struct hook *const hook,   // in
	const enum threadtype threadtype   // in
)
{
	FAIL_IF( !hook );
	FAIL_IF( !threadtype );
	
	
	if( threadtype == THREAD_OWNER )
	{
		printf( "Owner: " );
		
		if( hook->owner )
			print_gui_brief( hook->owner );
		else
			print_unknown_address( hook->entry.pOwner );
	}
	else if( threadtype == THREAD_ORIGIN )
	{
		printf( "Origin: " );
		
		if( hook->origin )
			print_gui_brief( hook->origin );
		else
			print_unknown_address( hook->object.pti );
	}
	else if( threadtype == THREAD_TARGET )
	{
		printf( "Target: " );
		
		if( hook->object.flags & HF_GLOBAL )
			printf( "<GLOBAL> " );
		
		if( hook->target )
			print_gui_brief( hook->target );
		else if( !( hook->object.flags & HF_GLOBAL ) || hook->object.ptiHooked )
			print_unknown_address( hook->object.ptiHooked );
	}
	else
	{
		MSG_FATAL( "Unknown thread type." );
		printf( "threadtype: %d\n", threadtype );
		exit( 1 );
	}
	
	printf( "\n" );
	
	return;
}



/* print_hook_notice_begin()
Helper function to print a hook [begin] header with basic hook info.

'hook' is the hook info
'deskname' is the desktop name
'difftype' is the reported action, eg HOOK_ADDED, HOOK_MODIFIED, HOOK_REMOVED
*/
void print_hook_notice_begin(
	const struct hook *const hook,   // in
	const WCHAR *const deskname,   // in
	const enum difftype difftype   // in
)
{
	const char *diffname = NULL;
	
	FAIL_IF( !hook );
	FAIL_IF( !deskname );
	FAIL_IF( !difftype );
	
	
	//PRINT_SEP_BEGIN( "" );
	printf( "\n" );
	printf( "----------------------------------------------------------------------------[b]\n" );
	
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
	
	printf( " [HOOK 0x%08I64X @ ", (UINT64)( *(UINT_PTR *)&hook->object.head.h ) );
	if( hook->entry.pHead )
		PRINT_HEX_BARE( hook->entry.pHead );
	else
		printf( "<unknown>" );
	printf( "]" );
	
	printf( " [" );
	print_time();
	printf( "]" );
	
	printf( "\n" );
	
	print_hook_anomalies( hook );
	
	printf( "\n" );
	
	
	printf( "Id: " );
	print_HOOK_id( hook->object.iHook );
	printf( "\n" );
	
	//printf( "%p, %p, %p\n", hook->owner, hook->origin, hook->target );
	
	if( hook->object.flags )
	{
		printf( "Flags: " );
		print_HOOK_flags( hook->object.flags );
		printf( "\n" );
	}
	
	if( hook->object.head.cLockObj )
		printf( "Lock count: %u\n", hook->object.head.cLockObj );
	
	/* exactly what rpdesk2 does is unclear */
	if( hook->object.rpdesk2 )
	{
		PRINT_HEX_NAME( "rpdesk1", hook->object.rpdesk1 );
		
		printf( "rpdesk2: " );
		PRINT_HEX_BARE( hook->object.rpdesk2 );
		printf( " (HOOK faulted? chain faulted? locked? owner destroyed?)\n" );
	}
	
	printf( "Desktop: %ls\n", deskname );
	
	/**
	When the desktop hook store is initialized this program matches the Win32ThreadInfo kernel 
	addresses associated with a HOOK (aka hook->object) to their user mode threads and processes.
	
	HOOK owner GUI thread info kernel address: hook->entry.pOwner
	The related user mode thread info obtained by this program: hook->owner
	
	HOOK origin GUI thread info kernel address: hook->object.pti
	The related user mode thread info obtained by this program: hook->origin
	
	HOOK target GUI thread info kernel address: hook->object.ptiHooked
	The related user mode thread info obtained by this program: hook->target
	*/
	
	if( ( hook->owner == hook->origin ) && ( hook->entry.pOwner == hook->object.pti ) )
	{
		// owner and origin have the same user mode info and kernel address and will be consolidated
		printf( "Owner/" );
		
		if( ( hook->owner == hook->target ) 
			&& ( hook->entry.pOwner == hook->object.ptiHooked ) 
			&& !( hook->object.flags & HF_GLOBAL )
		)  // the owner/origin is the same as the target and will be further consolidated
			printf( "Origin/" );
		else // the owner/origin info or address must be on a separate line from the target
			print_brief_thread_info( hook, THREAD_ORIGIN );
	}
	else // the owner and origin aren't the same and must be printed on separate lines
	{
		print_brief_thread_info( hook, THREAD_OWNER );
		print_brief_thread_info( hook, THREAD_ORIGIN );
	}
	/* REM based on the above logic the only possible way owner and origin will share a line with 
	target is if all three have the same info and address, and the HOOK isn't global.
	otherwise at this point owner and origin have been printed either on the same or separate lines.
	*/
	
	print_brief_thread_info( hook, THREAD_TARGET );
	
	
	if( G->config->verbose == 6 )
		print_HOOK( &hook->object );
	else if( G->config->verbose >= 7 )
		print_hook( hook ); // this calls print_HOOK()
	
	if( difftype == HOOK_MODIFIED )
		printf( "\n" );
	
	return;
}



/* print_hook_notice_end()
Helper function to print a hook [end] header.
*/
void print_hook_notice_end( void )
{
	//PRINT_SEP_END( "" );
	printf( "----------------------------------------------------------------------------[e]\n" );
	fflush( stdout );
	return;
}



/* print_diff_gui()
Compare two gui structs and print any significant differences. Helper function for print_diff_hook()

'oldhook' is the old hook info
'newhook' is the new hook info
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
	
	/* only "significant" attributes of the thread will be compared. */
	struct
	{
		/* The kernel address of the thread's THREADINFO */
		const void *pvWin32ThreadInfo;
		
		/* The address of the thread's environment block (TEB) */
		const void *pvTeb;
		
		/* The thread id */
		HANDLE tid;
		
		/* The thread's process' id */
		HANDLE pid;
		
		/* The thread's process' name */
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
	oldstuff.ImageName.Length = 
		(unsigned short)( wcslen( oldstuff.ImageName.Buffer ) * sizeof( WCHAR ) );
	oldstuff.ImageName.MaximumLength = 
		(unsigned short)( oldstuff.ImageName.Length + sizeof( WCHAR ) );
	
	newstuff.ImageName.Buffer = empty2;
	newstuff.ImageName.Length = 
		(unsigned short)( wcslen( newstuff.ImageName.Buffer ) * sizeof( WCHAR ) );
	newstuff.ImageName.MaximumLength = 
		(unsigned short)( newstuff.ImageName.Length + sizeof( WCHAR ) );
	
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
	
	printf( "Old " );
	print_brief_thread_info( oldhook, threadtype );
	
	printf( "New " );
	print_brief_thread_info( newhook, threadtype );
	
	
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
	
	/* compare entry.pOwner
	print any significant differences in the owner of the HOOK
	*/
	print_diff_gui( a, b, THREAD_OWNER, deskname, &modified_header );
	
	if( a->object.head.h != b->object.head.h )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's handle has changed.\n" );
		PRINT_HEX_NAME( "Old", a->object.head.h );
		PRINT_HEX_NAME( "New", b->object.head.h );
	}
	
	/* the object may be locked and unlocked frequently and that creates a lot of modification 
	notices. this modification can be ignored by the user.
	*/
	if( ( a->object.head.cLockObj != b->object.head.cLockObj )
		&& !( G->config->flags & CFG_IGNORE_LOCK_COUNTS )
	)
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's lock count has changed.\n" );
		printf( "Old: %u\n", a->object.head.cLockObj );
		printf( "New: %u\n", b->object.head.cLockObj );
	}
	
	/* compare object.pti
	print any significant differences in the origin of the HOOK
	*/
	print_diff_gui( a, b, THREAD_ORIGIN, deskname, &modified_header );
	
	if( a->object.rpdesk1 != b->object.rpdesk1 )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nrpdesk1 has changed. The desktop that the HOOK is on has changed?\n" );
		PRINT_HEX_NAME( "Old", a->object.rpdesk1 );
		PRINT_HEX_NAME( "New", b->object.rpdesk1 );
	}
	
	if( a->object.pSelf != b->object.pSelf )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's kernel address has changed.\n" );
		PRINT_HEX_NAME( "Old", a->object.pSelf );
		PRINT_HEX_NAME( "New", b->object.pSelf );
	}
	
	if( a->object.phkNext != b->object.phkNext )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nThe HOOK's chain has been modified.\n" );
		PRINT_HEX_NAME( "Old", a->object.phkNext );
		PRINT_HEX_NAME( "New", b->object.phkNext );
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
		PRINT_HEX_NAME( "Old", a->object.offPfn );
		PRINT_HEX_NAME( "New", b->object.offPfn );
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
	
	/* compare object.ptiHooked
	print any significant differences in the target of the HOOK
	*/
	print_diff_gui( a, b, THREAD_TARGET, deskname, &modified_header );
	
	if( a->object.rpdesk2 != b->object.rpdesk2 )
	{
		if( !modified_header )
		{
			print_hook_notice_begin( b, deskname, HOOK_MODIFIED );
			modified_header = TRUE;
		}
		
		printf( "\nrpdesk2 has changed." );
		if( b->object.rpdesk2 )
			printf( " HOOK faulted? chain faulted? locked? owner destroyed?\n" );
		else
			printf( " HOOK recovered?\n" );
		
		PRINT_HEX_NAME( "Old", a->object.rpdesk2 );
		PRINT_HEX_NAME( "New", b->object.rpdesk2 );
	}
	
	
	if( modified_header )
		print_hook_notice_end();
	
	return !!modified_header;
}



/* print_diff_desktop_hook_items()
Print the HOOKs that have been added/removed/modified from a single desktop between snapshots.

'a' is a desktop and its HOOKs captured in the previous snapshot
'b' is the same desktop and its HOOKs captured in the current snapshot
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
	FAIL_IF( a->hook_count > a->hook_max );
	FAIL_IF( b->hook_count > b->hook_max );
	
	
	deskname = b->desktop->pwszDesktopName;
	
	a_hi = 0, b_hi = 0;
	while( ( a_hi < a->hook_count ) && ( b_hi < b->hook_count ) )
	{
		int ret = compare_hook( &a->hook[ a_hi ], &b->hook[ b_hi ] );
		
		if( ret < 0 ) // hook removed
		{
			if( !a->hook[ a_hi ].ignore )
			{
				print_hook_notice_begin( &a->hook[ a_hi ], deskname, HOOK_REMOVED );
				print_hook_notice_end();
			}
			
			++a_hi;
		}
		else if( ret > 0 ) // hook added
		{
			if( !b->hook[ b_hi ].ignore )
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
			if( !a->hook[ a_hi ].ignore || !b->hook[ b_hi ].ignore )
				print_diff_hook( &a->hook[ a_hi ], &b->hook[ b_hi ], deskname );
			
			++a_hi;
			++b_hi;
		}
	}
	
	while( a_hi < a->hook_count ) // hooks removed
	{
		if( !a->hook[ a_hi ].ignore )
		{
			print_hook_notice_begin( &a->hook[ a_hi ], deskname, HOOK_REMOVED );
			print_hook_notice_end();
		}
		
		++a_hi;
	}
	
	while( b_hi < b->hook_count ) // hooks added
	{
		if( !b->hook[ b_hi ].ignore )
		{
			print_hook_notice_begin( &b->hook[ b_hi ], deskname, HOOK_ADDED );
			print_hook_notice_end();
		}
		
		++b_hi;
	}
	
	return;
}



/* print_diff_desktop_hook_lists()
Print the HOOKs that have been added/removed/modified from all desktops between snapshots.

'list_a' is the previous snapshot's desktop hook list
'list_b' is the current snapshot's desktop hook list
*/
void print_diff_desktop_hook_lists( 
	const struct desktop_hook_list *const list_a,   // in
	const struct desktop_hook_list *const list_b   // in
)
{
	struct desktop_hook_item *a = NULL;
	struct desktop_hook_item *b = NULL;
	
	FAIL_IF( !list_a );
	FAIL_IF( !list_b );
	
	
	for( a = list_a->head, b = list_b->head; ( a && b ); a = a->next, b = b->next )
		print_diff_desktop_hook_items( a, b );
	
	if( a || b )
	{
		MSG_FATAL( "The desktop hook stores could not be fully compared." );
		exit( 1 );
	}
	
	return;
}



/* print_initial_desktop_hook_item()
Print the HOOKs that have been found on a single desktop in an initial snapshot.

'item' is a desktop and its HOOKs captured in the snapshot

returns the number of HOOKs printed
*/
unsigned print_initial_desktop_hook_item( 
	const struct desktop_hook_item *const item   // in
)
{
	unsigned i = 0;
	unsigned printed = 0;
	
	FAIL_IF( !item );
	FAIL_IF( !item->desktop );
	FAIL_IF( !item->hook_max );
	FAIL_IF( item->hook_count > item->hook_max );
	
	
	for( i = 0; i < item->hook_count; ++i )
	{
		if( !item->hook[ i ].ignore )
		{
			print_hook_notice_begin( &item->hook[ i ], item->desktop->pwszDesktopName, HOOK_FOUND );
			print_hook_notice_end();
			++printed;
		}
	}
	
	return printed;
}



/* print_initial_desktop_hook_list()
Print the HOOKs that have been found on all desktops in an initial snapshot.

'list' is the initial snapshot's desktop hook list

returns the number of HOOKs printed
*/
unsigned print_initial_desktop_hook_list( 
	const struct desktop_hook_list *const list   // in
)
{
	unsigned printed = 0;
	struct desktop_hook_item *item = NULL;
	
	FAIL_IF( !list );
	
	/* for each desktop in a snapshot print the HOOKs found */
	for( item = list->head; item; item = item->next )
		printed += print_initial_desktop_hook_item( item );
	
	return printed;
}
