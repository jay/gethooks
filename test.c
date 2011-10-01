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

All of the user accessible test functions return an __int64 and also accept a single __int64 as a 
parameter, which in most cases can be supplied by the user. The configuration parser passes the 
value I64_MIN as a parameter for the function if the user did not specify a parameter.

-
print_handle_count()

Print counts of USER free, invalid, valid, menu, hook, and generic handles.
-

-
print_kernel_HOOK()

Print a HOOK. Pass in a pointer to the kernel address of a HOOK.
-

-
find_kernel_HOOK()

Take a snapshot and search it for the passed in HOOK.
-

-
find_most_preceding_kernel_HOOK()

Take a snapshot and search it for the HOOK that most precedes the passed in HOOK in its chain.
-

-
print_kernel_HOOK_chain()

Print a HOOK chain. Pass in a pointer to the kernel address of a HOOK.
-

-
print_kernel_HOOK_desktop_chains()

Print the HOOK chains in DESKTOPINFO.aphkStart[] for each attached to desktop.
-

-
function[], function__count

An array of structures holding info on every user accessible function in this file.
-

-
print_function_usage()

Print an individual testmode function's usage.
-

-
print_testmode_usage()

Print the testmode functions and their usage.
-

-
testmode()

Run user-specified tests.
-

*/

#include <stdio.h>

#include "util.h"

#include "reactos.h"

#include "diff.h"

#include "test.h"

/* the global stores */
#include "global.h"



#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* unreferenced formal parameter */
#endif



static int find_kernel_HOOK( 
	struct desktop_item **const out,   // out deref optional
	const __int64 addr   // in
);

static int find_most_preceding_kernel_HOOK( 
	__int64 *const out,   // out
	const __int64 addr   // in
);

static void print_function_usage( 
	unsigned i   // in
);



/* print_handle_count()
Print counts of USER free, invalid, valid, menu, hook, and generic handles.

Specify the number of 'seconds' to enable polling.
if 'seconds' > 0 then poll

returns nonzero in any case
*/
__int64 print_handle_count( 
	__int64 seconds   // in
)
{
	FAIL_IF( seconds >= INT_MAX );
	
	
	if( seconds > 0 )
		printf( "Polling user handle counts every %d seconds.\n", seconds );
	
	for( ;; )
	{
		unsigned i = 0, cMenu = 0, cHook = 0, cFree = 0, cValid = 0, cInvalid = 0, cGeneric = 0;
		
		printf( "*G->prog->pcHandleEntries: %lu\n", *G->prog->pcHandleEntries );
		for( i = 0; i < *G->prog->pcHandleEntries; ++i )
		{
			HANDLEENTRY *entry = &G->prog->pSharedInfo->aheList[ i ];
			
			if( entry->bType == TYPE_HOOK )
				++cHook;
			else if( entry->bType == TYPE_MENU )
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
			"Free: %u   Hook: %u   Menu: %u   Valid: %u   Invalid: %u   Generic: %u\n",
			cFree, cHook, cMenu, cValid, cInvalid, cGeneric
		);
		printf( "\n" );
		
		if( seconds <= 0 )
			break;
		
		Sleep( (DWORD)( seconds * 1000 ) );
	}
	
	return TRUE;
}



/* print_kernel_HOOK()
Print a HOOK. Pass in a pointer to the kernel address of a HOOK.

'addr' is the kernel address of a HOOK. cast to __int64

returns a pointer to the next HOOK in the chain or 0
*/
__int64 print_kernel_HOOK(
	__int64 addr   // in
)
{
	struct hook hook;
	struct snapshot *snapshot = NULL;
	struct desktop_item *desktop = NULL;
	
	
	ZeroMemory( &hook, sizeof( hook ) );
	
	/* Check to see if the HOOK is located on a desktop we're attached to */
	for( desktop = G->desktops->head; desktop; desktop = desktop->next )
	{
		if( ( (size_t)addr < ( (size_t)desktop->pDeskInfo->pvDesktopLimit - sizeof( HOOK ) ) )
			&& ( (size_t)addr >= (size_t)desktop->pDeskInfo->pvDesktopBase )
		) /* The HOOK is on an accessible desktop */
			break;
	}
	
	printf( "HOOK at kernel address " );
	PRINT_BARE_PTR( addr );
	
	if( !desktop )
	{
		printf( " is on an inaccessible desktop.\n" );
		
		return 0;
	}
	
	printf( " is on desktop '%ls'.\n", desktop->pwszDesktopName );
	
	hook.object = *(HOOK *)( (size_t)addr - (size_t)desktop->pvClientDelta );
	
	if( addr != (__int64)hook.object.pSelf )
	{
		MSG_WARNING( "Probable invalid HOOK address." );
		printf( "pSelf is not the same as the passed in address.\n\n" );
	}
	
	create_snapshot_store( &snapshot );
	if( init_snapshot_store( snapshot ) )
	{
		hook.origin = find_Win32ThreadInfo( snapshot, hook.object.pti );
		hook.target = find_Win32ThreadInfo( snapshot, hook.object.ptiHooked );
	}
	else
		MSG_WARNING( "Could not initialize the snapshot store." );
	
	print_brief_thread_info( &hook, THREAD_ORIGIN );
	print_brief_thread_info( &hook, THREAD_TARGET );
	print_HOOK( &hook.object );
	
	free_snapshot_store( &snapshot );
	return (__int64)hook.object.phkNext;
}



/* find_kernel_HOOK()
Take a snapshot and search it for the passed in HOOK.

'addr' is the kernel address of a HOOK. cast to __int64
'*out' receives a pointer to the attached to desktop that contains the HOOK.

returns nonzero if the HOOK was found in the snapshot.
returns zero otherwise and '*out' receives NULL.
*/
static int find_kernel_HOOK( 
	struct desktop_item **const out,   // out deref optional
	const __int64 addr   // in
)
{
	unsigned i = 0;
	struct snapshot *snapshot = NULL;
	struct desktop_item *desktop = NULL;
	struct desktop_hook_item *dh = NULL;
	
	
	if( !addr )
		goto cleanup;
	
	create_snapshot_store( &snapshot );
	if( !init_snapshot_store( snapshot ) )
	{
		MSG_ERROR( "Could not initialize the snapshot store." );
		goto cleanup;
	}
	
	/* for each desktop in the list of attached to desktops */
	for( dh = snapshot->desktop_hooks->head; dh; dh = dh->next )
	{
		/* for each hook info in the desktop's array of hook info structs */
		for( i = 0; i < dh->hook_count; ++i )
		{
			if( addr == (__int64)dh->hook[ i ].entry.pHead )
			{
				desktop = dh->desktop;
				goto cleanup;
			}
		}
	}
	
cleanup:
	if( out )
		*out = desktop;
	
	free_snapshot_store( &snapshot );
	return !!desktop;
}



/* find_most_preceding_kernel_HOOK()
Take a snapshot and search it for the HOOK that most precedes the passed in HOOK in its chain.

'addr' is the kernel address of a HOOK. cast to __int64
'*out' receives the kernel address of the HOOK that most precedes 'addr' in its chain.

returns nonzero if a preceding HOOK was found.
returns zero otherwise and '*out' receives 0.
*/
static int find_most_preceding_kernel_HOOK( 
	__int64 *const out,   // out
	const __int64 addr   // in
)
{
	unsigned i = 0, j = 0;
	struct snapshot *snapshot = NULL;
	struct desktop_hook_item *dh = NULL;
	
	/* The kernel address of the most preceding HOOK */
	__int64 phk = 0;
	
	/* This function will search through a HOOK chain of maximum length 'chainmax' */
	const unsigned chainmax = 100;
	
	FAIL_IF( !out );
	
	
	*out = 0;
	
	if( !addr )
		goto cleanup;
	
	create_snapshot_store( &snapshot );
	if( !init_snapshot_store( snapshot ) )
	{
		MSG_ERROR( "Could not initialize the snapshot store." );
		goto cleanup;
	}
	
	for( phk = addr, j = 0; j < chainmax; ++j )
	{
		__int64 found = 0;
		
		/* for each desktop in the list of attached to desktops */
		for( dh = snapshot->desktop_hooks->head; dh; dh = dh->next )
		{
			/* for each hook info in the desktop's array of hook info structs */
			for( i = 0; i < dh->hook_count; ++i )
			{
				/* if there is a HOOK that points to HOOK address 'phk' then a preceding HOOK has 
				been found.
				*/
				if( phk == (__int64)dh->hook[ i ].object.phkNext )
				{
					/* dh->hook[ i ] has the HOOK preceding HOOK 'phk' in a chain */
					
					/* if a different HOOK that also points to 'phk' was already found then alert 
					the user. this shouldn't ever happen. 
					*/
					if( found && ( found != (__int64)dh->hook[ i ].entry.pHead ) )
					{
						PRINT_DBLSEP_BEGIN( "wtf?" );
						
						MSG_ERROR( "Two different HOOKs point to the same link in a chain.\n" );
						print_kernel_HOOK( (__int64)found );
						print_kernel_HOOK( (__int64)dh->hook[ i ].entry.pHead );
						
						PRINT_DBLSEP_END( "wtf?" );
						continue;
					}
					
					found = (__int64)dh->hook[ i ].entry.pHead;
				}
			}
		}
		
		/* Stop if no preceding HOOK was found or if for some reason 'addr' points to itself as the 
		next HOOK in the chain
		*/
		if( !found || ( found == addr ) )
			break;
		
		phk = found;
	}
	
	if( j == chainmax )
	{
		MSG_ERROR( "HOOK chain exceeded maximum supported length." );
		printf( "Maximum supported length: %u\n", chainmax );
	}
	
	if( phk != addr ) // preceding HOOK was found
		*out = phk;
	
cleanup:
	free_snapshot_store( &snapshot );
	return !!*out;
}



/* print_kernel_HOOK_chain()
Print a HOOK chain. Pass in a pointer to the kernel address of a HOOK.

'addr' is the kernel address of a HOOK. cast to __int64

returns nonzero in any case
*/
__int64 print_kernel_HOOK_chain(
	__int64 addr   // in
)
{
	const char *const objname = "HOOK chain";
	unsigned i = 0;
	__int64 head = 0;
	struct desktop_item *desktop = NULL;
	
	
	PRINT_DBLSEP_BEGIN( objname );
	
	if( find_most_preceding_kernel_HOOK( &head, addr ) )
	{
		/* head points to the most preceding HOOK found in the chain */
		MSG_WARNING( "The HOOK address is not for the first HOOK in the chain." );
		PRINT_PTR( addr );
		printf( "\n" );
		printf( "The first HOOK in the chain according to a system snapshot is " );
		PRINT_BARE_PTR( head );
		printf( ".\n" );
	}
	
	
	for( i = 0; addr; ++i )
	{
		if( find_kernel_HOOK( &desktop, addr ) )
		{
			printf( "HOOK " );
			PRINT_BARE_PTR( addr );
			printf( " was found on desktop '%ls' in the snapshot.\n", desktop->pwszDesktopName );
		}
		else
		{
			MSG_WARNING( "Possible invalid HOOK." );
			printf( "The address was not found in the snapshot.\n" );
			PRINT_PTR( addr );
		}
		
		printf( "\nPosition in chain relative to passed in HOOK: %u\n", i );
		
		addr = print_kernel_HOOK( addr );
	}
	
	PRINT_DBLSEP_END( objname );
	
	return TRUE;
}



/* print_kernel_HOOK_desktop_chains()
Print the HOOK chains in DESKTOPINFO.aphkStart[] for each attached to desktop.

returns nonzero in any case
*/
__int64 print_kernel_HOOK_desktop_chains( 
	__int64 unused   // unused
)
{
	int i = 0;
	struct desktop_item *desktop = NULL;
	
	
	for( desktop = G->desktops->head; desktop; desktop = desktop->next, PRINT_HASHSEP_END( "" ) )
	{
		printf( "\n\n\n" );
		PRINT_HASHSEP_BEGIN( "" );
		printf( "Enumerating aphkStart[] on desktop '%ls'...\n", desktop->pwszDesktopName );
		
		for( i = 0; i < CWINHOOKS; ++i )
		{
			int hookid = WH_MIN + i;
			
			
			if( !is_HOOK_id_wanted( hookid ) )
				continue;
			
			if( !desktop->pDeskInfo->aphkStart[ i ] )
				continue;
			
			printf( "\n\naphkStart[ %d ]: ", i );
			print_HOOK_id( WH_MIN + i );
			printf( ": HOOK ");
			PRINT_BARE_PTR( desktop->pDeskInfo->aphkStart[ i ] );
			printf( " on desktop '%ls'.", desktop->pwszDesktopName );
			
			print_kernel_HOOK_chain( (__int64)desktop->pDeskInfo->aphkStart[ i ] );
		}
	}
	
	return TRUE;
}



const struct
{
	__int64 (*pfn)(__int64);
	const WCHAR *name; // function name as it should be specified by the user
	const WCHAR *description; // function description
	const WCHAR *param_name; // parameter name
	const BOOL param_required; // whether a parameter is required or not
	const WCHAR *extra_info; // extra info 
	const WCHAR *example_name; // an example of the command line parameters
	const WCHAR *example_description; // a description of the example
} function[] =
{
	{
		print_handle_count,   // pfn
		L"user",   // name
		/* description */
		L"Print counts of USER free, invalid, valid, menu, hook, and generic handles.",
		L"seconds",   // param_name
		FALSE,   // param_required
		L"Specify the number of seconds to enable polling.",   // extra_info
		L"3",   // example_name
		L"Print the count every 3 seconds."   // example_description
	},
	{
		print_kernel_HOOK,   // pfn
		L"hook",   // name
		/* description */
		L"Print a HOOK. Pass in a pointer to the kernel address of a HOOK.",
		L"address",   // param_name
		TRUE,   // param_required
		NULL,   // extra_info
		L"0xFE893E68",   // example_name
		L"Print HOOK at 0xFE893E68."   // example_description
	},
	{
		print_kernel_HOOK_chain,   // pfn
		L"chain",   // name
		/* description */
		L"Print a HOOK chain. Pass in a pointer to the kernel address of a HOOK.",
		L"address",   // param_name
		TRUE,   // param_required
		NULL,   // extra_info
		L"0xFE893E68",   // example_name
		L"Print HOOK at 0xFE893E68 and any HOOKs after it in the chain.",   // example_description
	},
	{
		print_kernel_HOOK_desktop_chains,   // pfn
		L"deskhooks",   // name
		/* description */
		L"Print the HOOK chains in DESKTOPINFO.aphkStart[] for each attached to desktop.",
		NULL,   // param_name
		FALSE,   // param_required
		L"Use the user-specified hook include/exclude list for filtering.",   // extra_info
		L"-d -i WH_KEYBOARD_LL",   // example_name
		L"Print the WH_KEYBOARD_LL chain on the current desktop.",   // example_description
	}
};
const unsigned function_count = sizeof( function ) / sizeof( function[ 0 ] );



/* print_function_usage()
Print an individual testmode function's usage.

'i' is the index of the function in array
*/
static void print_function_usage( 
	unsigned i   // in
)
{
	FAIL_IF( i >= function_count );
	FAIL_IF( !function[ i ].name );
	
	printf( "----------------------------------------------------------------------------[b]\n" );
	
	if( function[ i ].description )
		printf( "%ls\n", function[ i ].description );
	
	printf( "%s -t %ls", G->prog->pszBasename, function[ i ].name );
	if( function[ i ].param_name )
	{
		BOOL req = !!function[ i ].param_required;
		printf( " " );
		printf( "%c%ls%c", ( req ? '<' : '[' ), function[ i ].param_name, ( req ? '>' : ']' ) );
	}
	printf( "\n" );
	
	if( function[ i ].extra_info )
	{
		printf( "\n" );
		printf( "%ls\n", function[ i ].extra_info );
	}
	
	if( function[ i ].example_name )
	{
		printf( "\n" );
		if( function[ i ].example_description )
			printf( "Example: %ls\n", function[ i ].example_description );
		
		printf( "%s -t %ls %ls\n", 
			G->prog->pszBasename, function[ i ].name, function[ i ].example_name 
		);
	}
	
	printf( "----------------------------------------------------------------------------[e]\n" );
	printf( "\n" );
	return;
}



/* print_testmode_usage()
Print the testmode functions and their usage.
*/
void print_testmode_usage( void )
{
	unsigned i = 0;
	
	printf( "\n" );
	for( i = 0; i < function_count; ++i )
		print_function_usage( i );
	
	return;
}



/* testmode()
Run user-specified tests.

returns nonzero in any case
*/
int testmode( void )
{
	unsigned i = 0;
	struct list_item *item = NULL;
	
	FAIL_IF( !G );   // The global store must exist.
	FAIL_IF( !G->prog->init_time );   // The program store must be initialized.
	FAIL_IF( !G->config->init_time );   // The configuration store must be initialized.
	FAIL_IF( !G->desktops->init_time );   // The desktop store must be initialized.
	
	FAIL_IF( !G->config->testlist->init_time );   // The testlist must be initialized.
	
	/*
	if( !G->config->testlist->head ) // no specific tests were specified. run all.
		run_all_tests
	*/
	
	
	for( item = G->config->testlist->head; item; item = item->next )
	{
		printf( "\n\n\n\n" );
		print_list_item( item );
		
		for( i = 0; i < function_count; ++i )
		{
			if( function[ i ].name 
				&& item->name
				&& !_wcsicmp( function[ i ].name, item->name )
			)
			{
				printf( "\nCalling test function '%ls'.\n", function[ i ].name );
				function[ i ].pfn( item->id );
				break;
			}
		}
		
		if( i == function_count )
			printf( "\nUnknown function.\n", item->name );
	}
	
	return TRUE;
}



#ifdef _MSC_VER
#pragma warning(pop)
#endif
