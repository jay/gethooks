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
This file contains functions for a desktop store (linked list of desktops' heap and thread info).
Each function is documented in the comment block above its definition.

For now there is only one desktop store implemented and it's a global store (G->desktops).
'G->desktops' depends on the global program (G->prog) and configuration (G->config) stores.

-
create_desktop_store()

Create a desktop store and its descendants or die.
-

-
attach()

Attach the calling thread to a desktop and record its heap info in a desktop item.
Calls SetThreadDesktop().
-

-
thread()

This is the worker thread main function.
Calls attach() to attach to a desktop.
-

-
add_desktop_item()

Create a desktop item, attach to a desktop, and append the item to the desktop store's linked list.
Calls _beginthreadex() to call thread(), or calls attach() directly.
-

-
EnumDesktopProc()

Callback that EnumDesktopsW() passes desktop names.
Calls add_desktop_item().
-

-
add_all_desktops()

Add all desktops in the current window station.
Calls EnumDesktopsW() to call EnumDesktopProc().
-

-
init_global_desktop_store()

Initialize the global desktop store by attaching to the user-specified or default desktop(s).
Calls add_all_desktops(), or calls add_desktop_item() for each desktop if not adding all.
-

-
print_desktop_item()

Print an item from a desktop store's linked list.
-

-
print_desktop_store()

Print a desktop store and all its descendants.
-

-
print_global_desktop_store()

Print the global desktop store and all its descendants.
-

-
free_desktop_item()

Free a desktop item (dirty -- linked list isn't updated).
-

-
free_desktop_store()

Free a desktop store and all its descendants.
-

*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <process.h>

#include "util.h"

#include "desktop.h"

/* the global stores */
#include "global.h"



static int attach( 
	struct desktop_item *d   // in, out
);

static unsigned __stdcall thread( 
	void *param   // in
);

static struct desktop_item *add_desktop_item( 
	struct desktop_list *store,   // out
	const WCHAR *name   // in, optional
);

static BOOL CALLBACK EnumDesktopProc(
	LPWSTR param1,   // in
	LPARAM param2   // in
);

static int add_all_desktops( 
	struct desktop_list *store   // out
);

static void print_desktop_store( 
	const struct desktop_list *const store   // in
);

static void free_desktop_item( 
	struct desktop_item **const in   // in deref
);



/* create_desktop_store()
Create a desktop store and its descendants or die.
*/
void create_desktop_store( 
	struct desktop_list **const out   // out deref
)
{
	struct desktop_list *desktops = NULL;
	
	FAIL_IF( !out );
	FAIL_IF( *out );
	
	
	/* allocate a desktop store */
	desktops = must_calloc( 1, sizeof( *desktops ) );
	
	
	*out = desktops;
	return;
}



/* attach()
Attach the calling thread to a desktop and record its heap info in a desktop item.
Calls SetThreadDesktop().

this function attaches the thread running this function to the desktop whose name is specified 
in d->pwszDesktopName (in). if the attachment is successful then the desktop item 'd' is 
populated with the associated thread info and the attached desktop's heap info.

returns nonzero on success.

x86 only. I'll have to fix some of the server client pointer offsets here and elsewhere for x64.
*/
static int attach( 
	struct desktop_item *d   // in, out
)
{
	size_t offsetof_pDeskInfo = 0;
	size_t offsetof_ulClientDelta = 0;
	
	FAIL_IF( !d );
	
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	if( d->pwszDesktopName )
	{
		FAIL_IF( GetCurrentThreadId() == G->prog->dwMainThreadId );   // any thread but the main
		
		d->hDesktop = OpenDesktopW( d->pwszDesktopName, 0, 0, DESKTOP_READOBJECTS );
		if( !d->hDesktop )
		{
			if( G->config->verbose >= 2 )
			{
				MSG_ERROR_GLE( "OpenDesktopW() failed." );
				printf( 
					"Failed to open desktop '%ls' for DESKTOP_READOBJECTS access.\n", 
					d->pwszDesktopName 
				);
			}
			
			goto fail;
		}
		
		if( !SetThreadDesktop( d->hDesktop ) )
		{
			if( G->config->verbose >= 1 )
			{
				MSG_ERROR_GLE( "SetThreadDesktop() failed." );
				printf( "Failed to attach to desktop '%ls'.\n", d->pwszDesktopName );
			}
			
			goto fail;
		}
	}
	else // as a special case if no name is specified we're using the main thread and its desktop
		FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	
	/* TID */
	d->dwThreadId = GetCurrentThreadId();
	
	/* TEB */
	d->pvTeb = NtCurrentTeb();
	if( !d->pvTeb )
	{
		if( G->config->verbose >= 1 )
		{
			MSG_ERROR( "NtCurrentTeb() failed." );
			printf( "d->dwThreadId: %u\n", d->dwThreadId );
			printf( "d->pwszDesktopName: %ls\n", d->pwszDesktopName );
		}
		
		goto fail;
	}
	
	
	/* Get the pointer to the CLIENTINFO struct. &TEB.Win32ClientInfo */
	d->pvWin32ClientInfo = (void *)( (char *)d->pvTeb + 0x6cc );
	
	
	/* offsetof( struct CLIENTINFO, pDeskInfo ) */
	if( ( G->prog->dwOSMajorVersion == 5 ) && ( G->prog->dwOSMinorVersion == 0 ) ) // win2k
		offsetof_pDeskInfo = 20;
	else // XP+
		offsetof_pDeskInfo = 24;
	
	/* Get the pointer to the DESKTOPINFO struct.
	(char *)&TEB.Win32ClientInfo + offsetof(struct CLIENTINFO, pDeskInfo ).
	*/
	d->pDeskInfo = *(void **)( (char *)d->pvWin32ClientInfo + offsetof_pDeskInfo );
	
	if( !d->pDeskInfo )
	{
		if( G->config->verbose >= 1 )
		{
			MSG_ERROR( "Failed to get a pointer to the DESKTOPINFO struct." );
			printf( "d->pwszDesktopName: %ls\n", d->pwszDesktopName );
		}
		
		goto fail;
	}
	
	/* ulClientDelta is the next member after pDeskInfo in CLIENTINFO */
	offsetof_ulClientDelta = offsetof_pDeskInfo + sizeof( void * );
	
	/* Get the value of ulClientDelta.
	(char *)&TEB.Win32ClientInfo + offsetof(struct CLIENTINFO, ulClientDelta ), aka 
	(char *)&TEB.Win32ClientInfo + offsetof(struct CLIENTINFO, pDeskInfo ) + sizeof( void * ).
	The latter works because ulClientDelta is the next member after pDeskInfo.
	
	ulClientDelta is apparently declared as a ULONG but is it a ULONG_PTR for x64? hm
	*/
	d->pvClientDelta = *(void **)( (char *)d->pvWin32ClientInfo + offsetof_ulClientDelta );
	
	
	if( !d->pvClientDelta
		|| !d->pDeskInfo->pvDesktopBase 
		|| !d->pDeskInfo->pvDesktopLimit 
		|| ( d->pDeskInfo->pvDesktopBase >= d->pDeskInfo->pvDesktopLimit )
		|| ( d->pvClientDelta > d->pDeskInfo->pvDesktopBase ) 
	)
	{
		if( G->config->verbose >= 1 )
		{
			MSG_ERROR( "Desktop heap info is invalid." );
			printf( "d->pwszDesktopName: %ls\n", d->pwszDesktopName );
			PRINT_HEX( d->pvClientDelta );
			PRINT_HEX( d->pDeskInfo->pvDesktopBase );
			PRINT_HEX( d->pDeskInfo->pvDesktopLimit );
		}
		
		goto fail;
	}
	
	return 1;
	
fail:
	return 0;
}



/* stuff to be passed to thread()
this struct members' annotations are similar to those of function parameters
"actual" is used if the structure member will be modified by the function, regardless of if what it 
points to will be modified ("out").
*/
struct stuff
{
	/* a new desktop item, calloc'd. only d->pwszDesktopName is expected to be valid. */
	struct desktop_item *d;   // in, out
	
	/* this event is signaled when the thread's initialization has completed.
	whether or not it was successful is determined by checking if( d->hEventTerminate )
	*/
	HANDLE hEventInitialized;   // in
};

/* thread()
This is the worker thread main function.
Calls attach() to attach to a desktop.

separate threads must be created to attach to each additional desktop.

use _beginthreadex() to call this function.
currently the return value doesn't matter as long as it's != STILL_ACTIVE (259)
*/
static unsigned __stdcall thread( 
	void *param   // in
)
{
	/* the thread stuff (see above) */
	const struct stuff *stuff = param;
	
	/* when this event is signaled the thread terminates */
	HANDLE hEventTerminate = NULL;
	
	FAIL_IF( !stuff );
	FAIL_IF( !stuff->hEventInitialized );
	FAIL_IF( !stuff->d );
	FAIL_IF( !stuff->d->pwszDesktopName );
	
	
	/* create the event that this thread waits on to terminate */
	hEventTerminate = CreateEvent( NULL, 0, 0, NULL );
	if( !hEventTerminate )
	{
		MSG_FATAL_GLE( "CreateEvent() failed." );
		printf( "Failed to create the termination event.\n" );
		exit( 1 );
	}
	
	/* attach to the desktop specified by d->pwszDesktopName and get the desktop's heap info */
	if( !attach( stuff->d ) )
	{
		if( G->config->verbose >= 2 )
		{
			MSG_ERROR( "attach() failed." );
		}
		
		/* initialization was unsuccessful. after the thread's initialization event is signaled the 
		main thread checks if the terminate event is NULL to determine whether or not the 
		initialization was successful. because it wasn't successful the terminate event should be 
		freed and set NULL.
		*/
		CloseHandle( hEventTerminate );
		hEventTerminate = NULL;
	}
	
	/* make the terminate event accessible from the main thread */
	stuff->d->hEventTerminate = hEventTerminate;
	
	/* desktop item d has been initialized, but not yet added to the list. main thread does that */
	
	/* the main thread waits for this initialization signal and will then test if 
	d->hEventTerminate != NULL to determine if initialization was successful.
	*/
	if( !SetEvent( stuff->hEventInitialized ) )
	{
		MSG_FATAL_GLE( "SetEvent() failed." );
		printf( "Failed to signal the initialization event.\n" );
		exit( 1 );
	}
	
	/* after initialization is signaled the main thread frees the memory pointed to by 'stuff' */
	stuff = NULL;
	
	/* REM the desktop item must not be dereferenced after initialization is signaled.
	the main thread could free the desktop item and all of its associated resources before this 
	thread has terminated. the main thread does not free the event handle d->hEventTerminate, 
	which is the hEventTerminate resource created by this thread to monitor for termination.
	*/
	
	if( hEventTerminate ) // this worker thread's init was successful. it is attached to a desktop.
	{
		/* wait for the main thread to signal for this thread's termination */
		SetLastError( 0 ); // error code is not set by WaitForSingleObject() unless WAIT_FAILED
		if( WaitForSingleObject( hEventTerminate, INFINITE ) )
		{
			MSG_FATAL_GLE( "WaitForSingleObject() failed." );
			exit( 1 );
		}
		
		CloseHandle( hEventTerminate );
		hEventTerminate = NULL;
	}
	
	return 0; /* doesn't matter right now, as long as it's != STILL_ACTIVE (259) */
}



/* add_desktop_item()
Create a desktop item, attach to a desktop, and append the item to the desktop store's linked list.
Calls _beginthreadex() to call thread(), or calls attach() directly.

This should only be called from the main thread.

Create a thread, attach it to a desktop and add that desktop's heap info to a desktop list store.
When a thread is attached to a desktop that desktop's heap is mapped in the process space. 
HOOK objects can then be read from that desktop's heap.

'store' is the desktop list store to append the item to.
'name' is the name of the desktop to attach to.

the item's name will point to a duplicate of the passed in 'name'.
if 'name' is NULL then skip attach and use the main thread's current desktop.

returns on success a pointer to the desktop item that was added to the list.
if there is already an existing item with the same name a pointer to it is returned.
returns NULL on fail
*/
static struct desktop_item *add_desktop_item( 
	struct desktop_list *store,   // out
	const WCHAR *name   // in, optional
)
{
	HDESK hMainDesktop = NULL;
	WCHAR *pwszMainDesktopName = NULL;
	struct desktop_item *current = NULL;
	struct desktop_item *d = NULL;
	
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	
	/* Get the main thread's desktop */
	hMainDesktop = GetThreadDesktop( G->prog->dwMainThreadId );
	if( !hMainDesktop )
	{
		MSG_FATAL_GLE( "GetThreadDesktop() failed." );
		printf( "Failed to get main thread's desktop.\n" );
		exit( 1 );
	}
	
	/* Get the main thread's desktop name */
	if( !get_user_obj_name( &pwszMainDesktopName, hMainDesktop ) )
	{
		MSG_FATAL_GLE( "get_user_obj_name() failed." );
		printf( "Failed to get main thread's desktop name.\n" );
		exit( 1 );
	}
	
	/* if no name was specified we're adding the main thread's desktop */
	if( !name )
		name = pwszMainDesktopName;
	
	/* rem GetThreadDesktop() handle doesn't need to be closed */
	hMainDesktop = NULL;
	
	/* check if there is already an entry for this desktop */
	for( current = store->head; current; current = current->next )
	{
		if( current->pwszDesktopName && !_wcsicmp( name, current->pwszDesktopName ) )
		{
			if( G->config->verbose >= 1 )
			{
				MSG_WARNING( "Already attached to desktop." );
				printf( "desktop: %ls\n", name );
				d = current;
			}
			
			goto cleanup;
		}
	}
	
	
	/* make a new desktop_item */
	d = must_calloc( 1, sizeof( *d ) );
	
	/* if the desktop requested is not the main thread's desktop then a worker thread must be 
	spawned to attach to that desktop.
	*/
	if( _wcsicmp( name, pwszMainDesktopName ) )
	{
		struct stuff stuff;
		
		ZeroMemory( &stuff, sizeof( stuff ) );
		
		/* the worker thread signals this event when it has initialized, regardless of if the 
		initialization was a success 
		*/
		stuff.hEventInitialized = CreateEvent( NULL, 0, 0, NULL );
		if( !stuff.hEventInitialized )
		{
			MSG_FATAL_GLE( "CreateEvent() failed." );
			printf( "Failed to create the initialization event.\n" );
			exit( 1 );
		}
		
		d->pwszDesktopName = must_wcsdup( name );
		stuff.d = d;
		
		/* create the worker thread, which calls attach() */
		d->hThread = (HANDLE)_beginthreadex( NULL, 0, thread, &stuff, 0, NULL );
		if( !d->hThread )
		{
			MSG_FATAL( _strerror( "_beginthreadex() failed" ) );
			printf( "Failed to create a worker thread.\n" );
			exit( 1 );
		}
		
		SetLastError( 0 ); // error code is not set by WaitForSingleObject() unless WAIT_FAILED
		if( WaitForSingleObject( stuff.hEventInitialized, INFINITE ) )
		{
			MSG_FATAL_GLE( "WaitForSingleObject() failed." );
			printf( "Failed to wait for a worker thread to initialize.\n" );
			exit( 1 );
		}
		
		/* if the initialization failed there is no terminate event and the worker thread 
		has terminated or is in the process of terminating.
		*/
		if( !d->hEventTerminate ) // worker thread's initialization failed
		{
			if( G->config->verbose >= 2 )
			{
				MSG_ERROR( "Worker thread initialization failed." );
			}
			
			goto fail;
		}
	}
	else // the main thread is already attached to the desktop requested.
	{
		/* this is a special case to get the main thread's desktop's heap info without attaching.
		when d->pwszDesktopName == NULL then attach() skips the actual attaching and 
		populates d with the desktop heap info for the thread that is running attach(), in this 
		case the main thread.
		*/
		d->pwszDesktopName = NULL;
		
		if( !attach( d ) )
		{
			if( G->config->verbose >= 1 )
			{
				MSG_ERROR( "attach() failed." );
			}
			
			goto fail;
		}
		
		/* affter attach() has returned successfully point to the main thread's desktop name */
		d->pwszDesktopName = must_wcsdup( pwszMainDesktopName );
	}
	
	/* the desktop item is initialized. add the new item to the end of the list */
	
	if( !store->head )
	{
		store->head = d;
		store->tail = d;
	}
	else
	{
		store->tail->next = d;
		store->tail = d;
	}
	
	goto cleanup;
	
fail:
	/* freeing the desktop item frees all its associated resources except 
	hEventTerminate which is freed by the worker thread before terminating
	*/
	free_desktop_item( &d );
	
cleanup:
	
	if( d )
	{
		printf( "Attached to desktop '%ls'.\n", name );
	}
	else
	{
		/* some desktops attaching is expected to fail. if a desktop attach fails when it's 
		expected then don't show a message unless verbose.
		the only known failure right now is Winlogon (tested on Windows 7 x86 SP1).
		*/
		if( _wcsicmp( name, L"Winlogon" ) ) 
			printf( "Failed to attach to desktop '%ls'.\n", name );
		else if( G->config->verbose >= 1 )
			printf( "Failed to attach to desktop '%ls'. (expected)\n", name );
	}
	
	free( pwszMainDesktopName );
	
	return d;
}



/* EnumDesktopProc()
Callback that EnumDesktopsW() passes desktop names.
Calls add_desktop_item().

EnumDesktopsW() is called by aad_all_desktops() which is called to attach to all desktops.

this function always returns TRUE to continue enumeration.
if any desktop cannot be attached to it is not considered fatal.
*/
static BOOL CALLBACK EnumDesktopProc(
	LPWSTR param1,   // in
	LPARAM param2   // in
)
{
	WCHAR *const pwszDesktopName = (WCHAR *)param1;
	struct desktop_list *const store = (struct desktop_list *)param2;
	
	FAIL_IF( !pwszDesktopName );
	FAIL_IF( !store );
	
	
	add_desktop_item( store, pwszDesktopName );
	
	return TRUE;  // continue enumeration
}



/* add_all_desktops()
Add all desktops in the current window station.
Calls EnumDesktopsW() to call EnumDesktopProc().

attempt to attach to each of the desktops in the current window station and add them to 'store'

for each desktop name EnumDesktopProc() is called, which calls add_desktop_item().

returns the total number of attached to desktops in the store. this includes any already attached.
*/
static int add_all_desktops( 
	struct desktop_list *store   // out
)
{
	int count = 0;
	HWINSTA station = NULL;
	struct desktop_item *d = NULL;
	
	FAIL_IF( !store );
	
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	
	printf( "Attempting to attach to all desktops in the current window station.\n" );
	/*
	Remarkably, unless I fouled something up, calling EnumDesktopsW() with NULL for the station 
	handle does not work properly, despite what Microsoft documentation says for hwinsta:
	"If this parameter is NULL, the current window station is used."
	http://msdn.microsoft.com/en-us/library/ms682614.aspx
	
	In fact doing that enumerates not desktop names but workstation names instead.
	When I call EnumDesktopsW() with hwinsta param as NULL the function will pass 
	a name to my callback function of "WinSta0". In fact this is a window station 
	name and not a desktop name. Tested on XP/Vista/Win7.
	
	Unfortunately passing the GetProcessWindowStation() handle to enumerate desktops does not 
	always work. The handle needs the WINSTA_ENUMDESKTOPS right, but does not always have it.
	
	The implemented solution is:
	Call GetUserObjectInformationW() with GetProcessWindowStation() handle to get name (see main).
	Open that name with OpenWindowStationW() using the WINSTA_ENUMDESKTOPS flag.
	Use the handle from OpenWindowStationW() to pass to EnumDesktopsW()
	
	If opening the window station fails then the desktops cannot be enumerated.
	*/
	
	/* Open the process' window station name with OpenWindowStationW() using the 
	WINSTA_ENUMDESKTOPS flag.
	*/
	station = OpenWindowStationW( G->prog->pwszWinstaName, FALSE, WINSTA_ENUMDESKTOPS );
	if( !station )
	{
		MSG_FATAL_GLE( "OpenWindowStationW() failed." );
		printf( 
			"Failed to open window station '%ls' for WINSTA_ENUMDESKTOPS access.\n", 
			G->prog->pwszWinstaName 
		);
		printf( "The desktop names cannot be enumerated. Use the -d switch instead.\n" );
		exit( 1 );
	}
	
	/* Enumerate the window station's desktop names */
	SetLastError( 0 ); // error code may not be set by EnumDesktopsW() on error
	if( !EnumDesktopsW( station, EnumDesktopProc, (LPARAM)store ) )
	{
		MSG_FATAL_GLE( "EnumDesktopsW() failed." );
		printf( 
			"Failed to enumerate desktops in window station '%ls'.\n", 
			G->prog->pwszWinstaName 
		);
		printf( "The desktop names cannot be enumerated. Use the -d switch instead.\n" );
		exit( 1 );
	}
	
	CloseWindowStation( station );
	station = NULL;
	
	/* count how many desktops this store is now attached to */
	for( d = store->head; d; d = d->next )
		++count;
	
	return count;
}



/* init_global_desktop_store()
Initialize the global desktop store by attaching to the user-specified or default desktop(s).
Calls add_all_desktops(), or calls add_desktop_item() for each desktop if not adding all.

This function must only be called from the main thread.
For now there is only one desktop store implemented and it's a global store (G->desktops).
'G->desktops' depends on the global program (G->prog) and configuration (G->config) stores.
*/
void init_global_desktop_store( void )
{
	FAIL_IF( !G );   // The global store must exist.
	
	FAIL_IF( G->desktops->init_time );   // Fail if this store has already been initialized.
	
	FAIL_IF( !G->prog->init_time );   // The program store must be initialized.
	FAIL_IF( !G->config->init_time );   // The configuration store must be initialized.
	
	FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	
	/* if the user's list of desktop names is not initialized then the user did not specify the 'd' 
	option. in this case attempt to attach to all desktops in the current window station.
	*/
	if( !G->config->desklist->init_time )
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
	
	
	/* G->desktops has been initialized */
	GetSystemTimeAsFileTime( (FILETIME *)&G->desktops->init_time );
	return;
}



/* print_desktop_item()
Print an item from a desktop store's linked list.

if 'item' is NULL this function returns without having printed anything.
*/
void print_desktop_item( 
	const struct desktop_item *const item   // in
)
{
	const char *const objname = "Desktop Item";
	
	
	if( !item )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	printf( "item->pwszDesktopName: %ls\n", item->pwszDesktopName );
	PRINT_HEX( item->hDesktop );
	PRINT_HEX( item->hThread );
	PRINT_HEX( item->hEventTerminate );
	printf( "item->dwThreadId: %lu\n", item->dwThreadId );
	PRINT_HEX( item->pvTeb );
	PRINT_HEX( item->pvWin32ClientInfo );
	PRINT_HEX( item->pvClientDelta );
	PRINT_HEX( item->pDeskInfo );
	PRINT_HEX( item->pDeskInfo->pvDesktopBase );
	PRINT_HEX( item->pDeskInfo->pvDesktopLimit );
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_desktop_store()
Print a desktop store and all its descendants.

if 'store' is NULL this function returns without having printed anything.
*/
static void print_desktop_store( 
	const struct desktop_list *const store   // in
)
{
	const char *const objname = "Desktop List Store";
	struct desktop_item *item = NULL;
	
	
	if( !store )
		return;
	
	PRINT_DBLSEP_BEGIN( objname );
	print_init_time( "store->init_time", store->init_time );
	
	printf( "store->type: " );
	switch( store->type )
	{
		case DESKTOP_INVALID_TYPE:
			printf( "DESKTOP_INVALID_TYPE (the desktop heap list type hasn't been set.)" );
			break;
		case DESKTOP_CURRENT:
			printf( "DESKTOP_CURRENT (user specified 'd' option but did not specify names.)" );
			break;
		case DESKTOP_SPECIFIED:
			printf( "DESKTOP_SPECIFIED (user specified 'd' option and specified desktop names.)" );
			break;
		case DESKTOP_ALL:
			printf( "DESKTOP_ALL (all accessible desktops. user didn't specify the 'd' option.)" );
			break;
		default:
			printf( "%d (unknown type)", store->type );
	}
	printf( "\n" );
	
	//printf( "\nNow printing the desktop items in the list from head to tail.\n " );
	PRINT_HEX( store->head );
	
	for( item = store->head; item; item = item->next )
	{
		//PRINT_HEX( item );
		print_desktop_item( item );
	}
	
	PRINT_HEX( store->tail );
	
	PRINT_DBLSEP_END( objname );
	
	return;
}



/* print_global_desktop_store()
Print the global desktop store and all its descendants.
*/
void print_global_desktop_store( void )
{
	print_desktop_store( G->desktops );
	return;
}



/* free_desktop_item()
Free a desktop item (dirty -- linked list isn't updated).

this function then sets the desktop item pointer to NULL and returns.

this function has no regard for the other items in the list and should only be called by 
free_desktop_store() or add_desktop_item().

'in' is a pointer to a pointer to the desktop item, which contains desktop heap information.
if( !in || !*in ) then this function returns.
*/
static void free_desktop_item( 
	struct desktop_item **const in   // in deref
)
{
	if( !in || !*in )
		return;
	
	// if the thread is alive then both hThread and hEventTerminate are != NULL and not signaled.
	if( (*in)->hThread && (*in)->hEventTerminate ) // active worker thread
	{
		/* signal the worker thread to terminate */
		if( !SetEvent( (*in)->hEventTerminate ) )
		{
			MSG_FATAL_GLE( "SetEvent() failed." );
			printf( "Failed to signal the worker thread's termination event.\n" );
			exit( 1 );
		}
		
		/* wait for the worker thread to terminate. maybe always do this. */
		SetLastError( 0 ); // error code is not set by WaitForSingleObject() unless WAIT_FAILED
		if( WaitForSingleObject( (*in)->hThread, INFINITE ) )
		{
			MSG_FATAL_GLE( "WaitForSingleObject() failed." );
			printf( "Failed to wait for a worker thread to terminate.\n" );
			exit( 1 );
		}
	}
	
	/* The hEventTerminate event handle is closed by the worker thread before it terminates. */
	
	if( (*in)->hThread )
		CloseHandle( (*in)->hThread );
	
	free( (*in)->pwszDesktopName );
	
	/* the handle to the desktop should be closed after the thread's other resources are freed */
	if( (*in)->hDesktop )
		FAIL_IF( !CloseDesktop( (*in)->hDesktop ) ); // thread has terminated so this should work
	
	free( (*in) );
	*in = NULL;
	
	return;
}



/* free_desktop_store()
Free a desktop store and all its descendants.

this function then sets the desktop store pointer to NULL and returns

'in' is a pointer to a pointer to the desktop store, which contains a linked list of desktop heap 
information.
if( !in || !*in ) then this function returns.
*/
void free_desktop_store( 
	struct desktop_list **const in   // in deref
)
{
	if( !in || !*in )
		return;
	
	if( (*in)->head )
	{
		struct desktop_item *current = NULL, *p = NULL;
		
		for( current = (*in)->head; current; current = p )
		{
			p = current->next;
			
			free_desktop_item( &current );
		}
	}
	
	free( (*in) );
	*in = NULL;
	
	return;
}
