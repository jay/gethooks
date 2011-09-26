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
This file contains functions for a snapshot store (system process info, gui threads, desktop hooks).
Each function is documented in the comment block above its definition.

The snapshot store(s) depend on all the other global stores.

-
create_snapshot_store()

Create a snapshot store and its descendants or die.
-

-
callback_add_gui()

If the passed in thread info is for a GUI thread add it to the passed in snapshot's gui array.
-

-
compare_gui()

Compare two gui structs according to the kernel address of the associated Win32ThreadInfo struct.
-

-
find_Win32ThreadInfo()

Search a snapshot store's array of gui threads for a Win32ThreadInfo address.
-

-
init_snapshot_store()

Take a snapshot of the system state. This initializes a snapshot store.
-

-
print_gui_brief()

Print some brief information on a GUI thread's process name, process id, thread id, etc. No newline.
-

-
print_gui()

Print a gui struct.
-

-
print_snapshot_store()

Print a snapshot store and all its descendants.
-

-
free_snapshot_store()

Free a snapshot store and all its descendants.
-

*/

#include <stdio.h>

#include "util.h"

/* traverse_threads() */
#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"

#include "snapshot.h"

/* the global stores */
#include "global.h"



static int callback_add_gui( 
	void *cb_param,   // in, out
	SYSTEM_PROCESS_INFORMATION *const spi,   // in
	SYSTEM_THREAD_INFORMATION *const sti,   // in
	const ULONG remaining,   // in
	const DWORD flags   // in, optional
);

static int compare_gui( 
	const void *const p1,   // in
	const void *const p2   // in
);

static void print_snapshot_store( 
	const struct snapshot *const store   // in
);



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



/* stuff to be passed to callback_add_gui().
this struct members' annotations are similar to those of function parameters
"actual" is used if the structure member will be modified by the function, regardless of if what it 
points to will be modified ("out").
*/
struct callback_info
{
	/* The store which holds the gui array to add to. */
	struct snapshot *store;   // in, out
	
	/* Temporary process handle opened in a prior call to the callback function.
	If traverse_threads() did not terminate successfully this handle must be closed.
	*/
	HANDLE process;   // in, out, actual, optional
};

/* callback_add_gui()
If the passed in thread info is for a GUI thread add it to the passed in snapshot's gui array.

traverse_threads() callback: this function is called for every SYSTEM_THREAD_INFORMATION.
This function uses x86 offsets only, it will have to be fixed for x64.

The behavior of a traverse_threads() callback is documented in traverse_threads.txt.
*/
static int callback_add_gui( 
	void *cb_param,   // in, out
	SYSTEM_PROCESS_INFORMATION *const spi,   // in
	SYSTEM_THREAD_INFORMATION *const sti,   // in
	const ULONG remaining,   // in
	const DWORD flags   // in, optional
)
{
	/** Init
	*/
	#define dbg_printf   if( ( flags & TRAVERSE_FLAG_DEBUG ) )printf
	
	// address of thread environment block (TEB)
	void *pvTeb = NULL;
	
	// address of Win32ThreadInfo
	void *pvWin32ThreadInfo = NULL;
	
	// the return code of this function
	int return_code = TRAVERSE_CALLBACK_ABORT;
	
	// callback data
	struct callback_info *const ci = (struct callback_info *)cb_param; 
	
	// whether or not the process has already been passed to the callback. 1 if true, 0 if false.
	const unsigned process_is_new = ( !sti || ( sti == (void *)&spi->Threads ) ); // new spi
	
	
	if( !sti || !ci || !ci->store )
	{
		/* a parameter required by this callback is missing. 
		this shouldn't happen. abort 
		*/
		dbg_printf( "Parameter validation failed:\n" );
		dbg_printf( "sti: %IX\n", sti );
		dbg_printf( "ci: %IX\n", ci );
		dbg_printf( "ci->store: %IX\n", ci->store );
		return_code = TRAVERSE_CALLBACK_ABORT;
		goto cleanup;
	}
	
	/*	traverse_threads() writes the spi array before the first time it calls a callback function.
	the first time this callback is called is the earliest time that the spi init can be recorded.
	*/
	if( !ci->store->init_time_spi )
	{
		/* ci->store->spi array was just initialized. record init time. */
		GetSystemTimeAsFileTime( (FILETIME *)&ci->store->init_time_spi );
	}
	
	
	
	/** 
	Open the process if it isn't open already
	*/
	dbg_printf( "PID: %Iu, ImageName: %ls\n", spi->UniqueProcessId, spi->ImageName.Buffer );
	
	/* if there's no process id then skip traversing its threads */
	if( !spi->UniqueProcessId )
	{
		dbg_printf( "Ignoring process with id 0.\n" );
		
		return_code = TRAVERSE_CALLBACK_SKIP;
		goto cleanup;
	}
	
	if( process_is_new )
	{
		if( ci->process ) // there is a process handle already open
		{
			/* the last opened process' handle should have already been closed.
			this shouldn't happen. abort 
			*/
			dbg_printf( "There is a process handle already open. Aborting!\n" );
			return_code = TRAVERSE_CALLBACK_ABORT;
			goto cleanup;
		}
		
		SetLastError( 0 ); // set because error code may not be set on success
		ci->process = OpenProcess( PROCESS_VM_READ, FALSE, (DWORD)spi->UniqueProcessId );
		
		dbg_printf( "OpenProcess() %s. GLE: %I32u, Handle: 0x%IX.\n", 
			( ci->process ? "success" : "error" ), 
			GetLastError(), 
			ci->process 
		);
		
		/* if the process couldn't be opened then skip traversing its threads */
		if( !ci->process )
		{
			return_code = TRAVERSE_CALLBACK_SKIP;
			goto cleanup;
		}
	}
	
	
	
	/** 
	Get the thread's environment block (TEB)
	*/
	dbg_printf( "TID: %Iu\n", sti->ClientId.UniqueThread );
	
	/* if there's no thread id then continue to the next thread */
	if( !sti->ClientId.UniqueThread )
	{
		dbg_printf( "Ignoring thread with id 0.\n" );
		
		return_code = TRAVERSE_CALLBACK_CONTINUE;
		goto cleanup;
	}
	
	/* check to see if we already have this thread's TEB address.
	if TRAVERSE_FLAG_EXTENDED was passed in then traverse_threads()
	called NtQuerySystemInformation() with SystemExtendedProcessInformation.
	On Vista+ (major >= 6) that should have yielded the TEB address.
	*/
	if( ( flags & TRAVERSE_FLAG_EXTENDED ) && ( G->prog->dwOSMajorVersion >= 6 ) )
	{
		dbg_printf( "Getting TEB address from SYSTEM_EXTENDED_THREAD_INFORMATION\n" );
		pvTeb = ( (SYSTEM_EXTENDED_THREAD_INFORMATION *)sti )->TebAddress;
	}
	else
	{
		dbg_printf( "Getting TEB address from get_teb()\n" );
		pvTeb = get_teb( (DWORD)sti->ClientId.UniqueThread, flags );
	}
	
	dbg_printf( "TEB: 0x%IX\n", pvTeb );
	
	/* if there's no TEB associated with the thread then continue to the next thread. */
	if( !pvTeb )
	{
		return_code = TRAVERSE_CALLBACK_CONTINUE;
		goto cleanup;
	}
	
	
	/** 
	Get Win32ThreadInfo from the TEB
	*/
	{
		BOOL ret = 0;
		
		ret = ReadProcessMemory( 
			ci->process, 
			(char *)pvTeb + 0x040, /* pvTeb + offsetof W32ThreadInfo. 0x40 TEB32, 0x78 TEB64 */
			&pvWin32ThreadInfo, 
			sizeof( pvWin32ThreadInfo ), 
			NULL 
		);
		
		dbg_printf( "ReadProcessMemory() %s. GLE: %I32u, Handle: 0x%IX.\n", 
			( ret ? "success" : "error" ), 
			GetLastError(), 
			ci->process 
		);
		
		if( !ret )
			pvWin32ThreadInfo = 0;
	}
	
	dbg_printf( "Win32ThreadInfo: 0x%IX\n", pvWin32ThreadInfo );
	
	/* if there's no Win32ThreadInfo then this thread is not a GUI thread.
	continue to the next thread
	*/
	if( !pvWin32ThreadInfo )
	{
		return_code = TRAVERSE_CALLBACK_CONTINUE;
		goto cleanup;
	}
	
	/* if the number of gui threads found is more than can be held in the array 
	then abort. this is a high number like 10,000 - 100,000 so this shouldn't happen.
	*/
	if( ci->store->gui_count >= ci->store->gui_max ) // all array elements filled
	{
		dbg_printf( "ci->store->gui_count >= ci->store->gui_max\n" );
		dbg_printf( "ci->store->gui_count: %u\n", ci->store->gui_count );
		dbg_printf( "ci->store->gui_max: %u\n", ci->store->gui_max );
		return_code = TRAVERSE_CALLBACK_ABORT;
		goto cleanup;
	}
	
	
	
	/** add the GUI thread's info to the array of gui thread infos.
	*/
	ci->store->gui[ ci->store->gui_count ].pvWin32ThreadInfo = pvWin32ThreadInfo;
	ci->store->gui[ ci->store->gui_count ].pvTeb = pvTeb;
	ci->store->gui[ ci->store->gui_count ].spi = spi;
	ci->store->gui[ ci->store->gui_count ].sti = sti;
	
	// increment the number of gui threads found
	ci->store->gui_count++;
	
	return_code = TRAVERSE_CALLBACK_CONTINUE;
	
cleanup:
	
	/* if there's a handle to a process and either there's no more remaining 
	threads in the process to be traversed or the callback isn't continuing to 
	traverse the process' threads then close the process handle
	*/
	if( ci && ci->process && ( !remaining || ( return_code != TRAVERSE_CALLBACK_CONTINUE ) ) ) 
	{
		BOOL ret = 0;
		
		
		SetLastError( 0 ); // set because error code may not be set on success
		ret = CloseHandle( ci->process ); // it's ok if the handle is already closed/invalid
		
		dbg_printf( "CloseHandle(0x%IX) %s. GLE: %I32u.\n", 
			ci->process, 
			( ret ? "success" : "error" ), 
			GetLastError() 
		);
		
		ci->process = NULL;
	}
	
	return return_code;
}



/* compare_gui()
Compare two gui structs according to the kernel address of the associated Win32ThreadInfo struct.

qsort() callback: this function is called when sorting the gui array according to Win32ThreadInfo
bsearch() callback: this function is called when searching the gui array for a Win32ThreadInfo

returns -1 if 'p1' Win32ThreadInfo < 'p2' Win32ThreadInfo
returns 1 if 'p1' Win32ThreadInfo > 'p2' Win32ThreadInfo
returns 0 if 'p1' Win32ThreadInfo == 'p2' Win32ThreadInfo
*/
static int compare_gui( 
	const void *const p1,   // in
	const void *const p2   // in
)
{
	const struct gui *const a = p1;
	const struct gui *const b = p2;
	
	
	if( a->pvWin32ThreadInfo < b->pvWin32ThreadInfo )
		return -1;
	else if( a->pvWin32ThreadInfo > b->pvWin32ThreadInfo )
		return 1;
	else
		return 0;
}



/* find_Win32ThreadInfo()
Search a snapshot store's array of gui threads for a Win32ThreadInfo address.

returns the gui struct that contains the matching pvWin32ThreadInfo
*/
struct gui *find_Win32ThreadInfo( 
	struct snapshot *const store,   // in
	void *const pvWin32ThreadInfo   // in
)
{
	struct gui findme;
	
	
	ZeroMemory( &findme, sizeof( findme ) );
	
	/* only the pvWin32ThreadInfo member is compared */
	findme.pvWin32ThreadInfo = pvWin32ThreadInfo;
	
	return 
		bsearch( 
			&findme, 
			store->gui, 
			store->gui_count, 
			sizeof( *store->gui ), 
			compare_gui 
		);
}



/* init_snapshot_store()
Take a snapshot of the system state. This initializes a snapshot store.

A snapshot store depends on all global stores, but does not depend on any other snapshot stores.

The following info in the snapshot store is recorded consecutively:
system process info (spi)
gui thread info (gui). this depends on spi.
desktop hook info (desktop_hooks). this depends on gui.

All of that information makes a snapshot of the state of the system.

Unlike other stores the snapshot stores are reused/reinitialized rather than freeing and 
recreating the stores, to avoid delay when taking continuous snapshots.

This function must only be called from the main thread.

returns nonzero on success
*/
int init_snapshot_store( 
	struct snapshot *const store   // in
)
{
	unsigned i = 0;
	int ret = 0;
	LONG nt_status = 0;
	struct callback_info ci;
	
	FAIL_IF( !G->prog->init_time );   // The program store must be initialized.
	FAIL_IF( !G->config->init_time );   // The configuration store must be initialized.
	FAIL_IF( !G->desktops->init_time );   // The desktop store must be initialized.
	
	FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	FAIL_IF( !store );   // a snapshot store must always be passed in
	
	
	/* snapshot stores are reused. do a soft reset to reuse gui array */
	store->gui_count = 0;
	/* the spi array doesn't have a count. traverse_threads() overwrites the spi regardless */
	/* store->desktop_hooks is soft reset by init_desktop_hook_store() */
	
	/* reset init times */
	store->init_time = 0;
	store->init_time_gui = 0;
	store->init_time_spi = 0;
	
	
	ZeroMemory( &ci, sizeof( ci ) );
	ci.store = store;
	
	/* call traverse_threads() to write the array of spi and gui.
	traverse_threads() calls callback_add_gui() which writes to the store's array of gui and sets 
	the spi init time.
	*/
	ret = traverse_threads( 
		callback_add_gui, /* callback */
		&ci, /* pointer to callback data */
		ci.store->spi, /* buffer that will receive the array of spi */
		ci.store->spi_max_bytes, /* buffer's byte count */
		TRAVERSE_FLAG_EXTENDED, /* flags. callback_add_gui() gets TEBs faster with EXTENDED */
		&nt_status /* pointer to receive status */
	);
	
	if( ret != TRAVERSE_SUCCESS )
	{
		MSG_ERROR( "traverse_threads() failed." );
		printf( "traverse_threads() returned: %s\n", traverse_threads_retcode_to_cstr( ret ) );
		
		if( ( ret == TRAVERSE_ERROR_ALIGNMENT )
			|| ( ret == TRAVERSE_ERROR_BUFFER_TOO_SMALL )
			|| ( ret == TRAVERSE_ERROR_QUERY )
		)
			printf( "nt_status: 0x%08lX\n", nt_status );
		
		/* the callback sets the initialization time of spi, however if traverse_threads() failed 
		the data may not be valid so clear the init
		*/
		store->init_time_spi = 0;
		
		return FALSE;
	}
	
	/* sort the gui array according to Win32ThreadInfo.
	this array must be sorted so that bsearch() can be called to later search for a Win32ThreadInfo
	*/
	qsort( 
		store->gui,
		store->gui_count,
		sizeof( *store->gui ),
		compare_gui
	);
	
	/* search for invalid or duplicate Win32ThreadInfo */
	for( i = 1; i < store->gui_count; ++i )
	{
		struct gui *a = &store->gui[ i - 1 ];
		struct gui *b = &store->gui[ i ];
		
		
		if( a->pvWin32ThreadInfo == b->pvWin32ThreadInfo )
		{
			MSG_FATAL( "Duplicate pvWin32ThreadInfo." );
			print_gui( a );
			print_gui( b );
			exit( 1 );
		}
		
		if( !a->pvWin32ThreadInfo )
		{
			MSG_FATAL( "Invalid pvWin32ThreadInfo." );
			print_gui( a );
			exit( 1 );
		}
		
		if( !b->pvWin32ThreadInfo )
		{
			MSG_FATAL( "Invalid pvWin32ThreadInfo." );
			print_gui( b );
			exit( 1 );
		}
	}
	
	/* the gui array has been initialized */
	GetSystemTimeAsFileTime( (FILETIME *)&store->init_time_gui );
	
	
	/* init the desktop hook store. depends on spi and gui arrays */
	if( !init_desktop_hook_store( store ) )
		return FALSE;
	
	
	/* the snapshot store has been initialized */
	GetSystemTimeAsFileTime( (FILETIME *)&store->init_time );
	return TRUE;
}



/* print_gui_brief()
Print some brief information on a GUI thread's process name, process id, thread id, etc. No newline.
*/
void print_gui_brief( 
	const struct gui *const gui   // in
)
{
	if( !gui )
	{
		printf( "<unknown>" );
		return;
	}
	
	if( gui->spi && gui->spi->ImageName.Buffer )
	{
		printf( "%.*ls", 
			(int)( gui->spi->ImageName.Length / sizeof( WCHAR ) ), 
			gui->spi->ImageName.Buffer 
		);
	}
	else
		printf( "<unknown>" );
	
	printf( " (" );
	printf( "PID %Iu", (size_t)( gui->spi ? gui->spi->UniqueProcessId : 0 ) );
	printf( ", TID %Iu", (size_t)( gui->sti ? gui->sti->ClientId.UniqueThread : 0 ) );
	printf( " @ " );
	PRINT_BARE_PTR( gui->pvWin32ThreadInfo );
	printf( ")" );
	
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
	const char *const objname = "gui struct";
	
	
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
		callback_print_thread_state( &G->prog->dwOSVersion, gui->spi, gui->sti, 0, 0 );
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
	int ret = 0;
	unsigned i = 0;
	
	
	if( !store )
		return;
	
	PRINT_DBLSEP_BEGIN( objname );
	print_init_time( "store->init_time", store->init_time );
	
	
	/* traverse_threads() should use as input the output buffer from a previous call (store->spi) */
	flags |= TRAVERSE_FLAG_RECYCLE;
	
	/* also call the callback for any process info reporting zero threads */
	flags |= TRAVERSE_FLAG_ZERO_THREADS_OK;
	
	/* If the flag TRAVERSE_FLAG_EXTENDED was passed in for the original call it must also be 
	passed in on a recycle call. Basically this has to track with the call in init_snapshot().
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
