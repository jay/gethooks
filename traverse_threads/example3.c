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
along with GetHooks.  If not, see <https://www.gnu.org/licenses/>.
*/

/** This is an example for traverse_threads() that prints process thread ids 
depending on several caller specified parameters. First build the 
traverse_threads library (see BUILD.txt).

-
MinGW:

gcc -I..\include -L..\lib -o example3 example3.c -ltraverse_threads -lntdll
-

-
Visual Studio (*from Visual Studio command prompt):

cl /I..\include example3.c ..\lib\traverse_threads.lib ..\lib\ntdll.lib
-
*/

#define _CRT_SECURE_NO_WARNINGS
#include <mbstring.h> // mbstowcs()

#include <stdio.h>
#include <windows.h>

/* if your project does _not_ have these structs declared in another include:

SYSTEM_THREAD_INFORMATION,
SYSTEM_EXTENDED_THREAD_INFORMATION,
SYSTEM_PROCESS_INFORMATION

then include nt_independent_sysprocinfo_structs.h before traverse_threads.h
*/
#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"



void print_license( void )
{
	printf( 
		"-\n"
		"Copyright (C) 2011 Jay Satiro <raysatiro@yahoo.com> \n"
		"All rights reserved. License GPLv3+: GNU GPL version 3 or later \n"
		"<https://www.gnu.org/licenses/gpl.html>. \n"
		"This is free software: you are free to change and redistribute it. \n"
		"There is NO WARRANTY, to the extent permitted by law. \n"
		"-\n"
	);
}



/* thread state needed by callback */
typedef enum _KTHREAD_STATE
{
	Initialized,
	Ready,
	Running,
	Standby,
	Terminated,
	Waiting,
	Transition,
	DeferredReady,
	GateWait,
	MaximumThreadState
} KTHREAD_STATE, *PKTHREAD_STATE;



/* thread wait reason needed by callback */
typedef enum _KWAIT_REASON
{
	Executive,
	FreePage,
	PageIn,
	PoolAllocation,
	DelayExecution,
	Suspended,
	UserRequest,
	WrExecutive,
	WrFreePage,
	WrPageIn,
	WrPoolAllocation,
	WrDelayExecution,
	WrSuspended,
	WrUserRequest,
	WrEventPair,
	WrQueue,
	WrLpcReceive,
	WrLpcReply,
	WrVirtualMemory,
	WrPageOut,
	WrRendezvous,
	Spare2,
	Spare3,
	Spare4,
	Spare5,
	WrCalloutStack,
	WrKernel,
	WrResource,
	WrPushLock,
	WrMutex,
	WrQuantumEnd,
	WrDispatchInt,
	WrPreempted,
	WrYieldExecution,
	WrFastMutex,
	WrGuardedMutex,
	WrRundown,
	MaximumWaitReason
} KWAIT_REASON, *PKWAIT_REASON;



/** some data that is passed to your callback.
all members must be initialized before passing to traverse_threads()
*/
struct your_callback_data
{
	/* set to process name you want to match. ie svchost.exe */
	WCHAR *image_name;   // in, optional
	
	/* set nonzero to print only those threads that are suspended */
	unsigned suspended_threads_only;   // in
	
	/* to keep count of how many processes have threads that are suspended */
	unsigned processes_that_have_suspended_threads;   // in, out
	
	/* to keep count of how many processes there are total in the system */
	unsigned processes_total;   // in, out
	
	/* to keep count of how many processes match image_name */
	unsigned processes_that_match_name;   // in, out, optional
	
	/* to keep count of how many threads there are total in the system */
	unsigned threads_total;   // in, out
	
	/* to keep count of how many threads match everything specified */
	unsigned threads_that_match;   // in, out
	
	/* to store a pointer to the prior process info that has a suspended thread */
	SYSTEM_PROCESS_INFORMATION *last_known_spi_with_a_suspended_thread;   // in, out
	
};



/* example of a typical callback

count all processes and print any process' threads whose process name matches 
image_name. if image_name is NULL then print all process' threads.

if suspended_threads_only is nonzero only threads that are suspended will be 
printed.
*/
int callback_count_and_print( 
	void *cb_param,   // in, out, optional
	SYSTEM_PROCESS_INFORMATION *const spi,   // in
	SYSTEM_THREAD_INFORMATION *const sti,   // in, optional
	const ULONG remaining,   // in
	const DWORD flags   // in, optional
)
{
	/** 
	init
	*/
	
	/* this function's return code */
	int return_code = TRAVERSE_CALLBACK_ABORT;
	
	/* your callback data that you passed in as a parameter to traverse_threads() */
	struct your_callback_data *const stuff = (struct your_callback_data *)cb_param;
	
	/* nonzero if the passed in thread (sti) is suspended, or zero if not */
	unsigned thread_suspended = 0;
	
	/* as noted in the documentation this can be used to check if 
	this is the first time a process (spi) has been passed to the callback.
	nonzero if this is the first time the spi has been passed, or zero if not.
	*/
	unsigned process_is_new = ( !sti || ( sti == (void *)&spi->Threads ) ); // new spi
	
	
	if( !stuff ) /* no pointer to your callback data, wasn't expecting that! */
	{
		return_code = TRAVERSE_CALLBACK_ABORT;
		goto cleanup;
	}
	
	
	
	/** 
	processing
	*/
	
	/* if image_name isn't NULL then do a case insensitive compare */
	if( stuff->image_name )
	{
		/* if there is no image name or the image name doesn't match then return */
		if( !spi->ImageName.Buffer || _wcsicmp( spi->ImageName.Buffer, stuff->image_name ) )
		{
			/* since we are looking for a specific process name, and this one doesn't match, 
			we can skip this process' additional threads.
			*/
			return_code = TRAVERSE_CALLBACK_SKIP;
			goto cleanup;
		}
		
		/* the image_name matches, and if this is the first time this 
		process info has been processed then increment the count of 
		processes with matching names
		*/
		if( process_is_new )
			stuff->processes_that_match_name++;
	}
	
	/* if the process does not contain any threads then CONTINUE.
	in this case SKIP could be used as well. 
	they both do the same thing when 0 threads.
	*/
	if( !sti ) /* sti == NULL only when process has 0 threads: ZERO_THREADS_OK */
	{
		return_code = TRAVERSE_CALLBACK_SKIP;
		goto cleanup;
	}
	
	/* determine if the thread is suspended.
	
	MS: "Thread Wait Reason is only applicable when the thread is in the Wait state."
	https://web.archive.org/web/20070322041640/http://support.microsoft.com/kb/837372
	*/
	thread_suspended = ( 
		( (KTHREAD_STATE)sti->ThreadState == Waiting )
		&& ( ( (KWAIT_REASON)sti->WaitReason == Suspended )
			|| ( (KWAIT_REASON)sti->WaitReason == WrSuspended )
		)
	);
	
	/* if the thread isn't suspended and only processing suspended threads then return */
	if( !thread_suspended && stuff->suspended_threads_only )
	{
		return_code = TRAVERSE_CALLBACK_CONTINUE; /* continue normally to the next thread */
		goto cleanup;
	}
	
	
	/** thread meets requirements */
	
	/* increment count of threads that match everything specified */
	stuff->threads_that_match++;
	
	/* if this is the first suspended thread found in this spi then 
	increment the count of processes that have suspended threads
	*/
	if( thread_suspended && ( stuff->last_known_spi_with_a_suspended_thread != spi ) )
	{
		/* increment count of proccesses that have suspended threads */
		stuff->processes_that_have_suspended_threads++;
		
		/* update this spi as having a suspended thread.
		by keeping track of this a process with more than one thread 
		suspended will not be counted more than once
		*/
		stuff->last_known_spi_with_a_suspended_thread = spi;
	}
	
	/* print process' image name */
	if( spi->ImageName.Buffer && *spi->ImageName.Buffer )
		printf( "%ls: ", spi->ImageName.Buffer );
	else
		printf( "<unknown>: " );
	
	/* print process' id */
	printf( "PID %Iu, ", (size_t)spi->UniqueProcessId );
	
	/* print thread's id */
	printf( "TID %Iu ", (size_t)sti->ClientId.UniqueThread );
	
	/* print suspended if thread is suspended */
	if( thread_suspended )
		printf( "(suspended) " );
	
	printf( "\n" );
	
	return_code = TRAVERSE_CALLBACK_CONTINUE;
	
	
cleanup:
	
	if( stuff )
	{
		/* check if this is the first time this process info has been 
		passed to the callback. if it is then increment the process 
		count and add the process' thread count to the total thread count.
		*/
		if( process_is_new ) /* new process info (spi) */
		{
			/* increment the process count */
			stuff->processes_total++;
			
			/* add all threads in this new spi to the total thread count.
			because remaining threads in a process can be SKIPped the right way 
			to ensure all threads are counted is adding all of them the first 
			time the process is encountered instead of incrementing on each call.
			*/
			if( sti ) /* sti != NULL if the process info has thread infos, and it should */
				stuff->threads_total += remaining + 1;
		}
	}
	
	return return_code;
}



void print_usage_and_exit( char *progname )
{
	print_license();
	printf( "\n\n" );
	
	printf( "this program prints threads depending on whether they are suspended\n\n" );
	
	printf( "usage: %s <suspended> [process name]\n\n", progname );
	
	printf( "<suspended>: specify 1 to print suspended threads, or 0 to print all threads\n" );
	printf( "[process name]: optionally filter for processes matching this name\n\n" );
	
	printf( "example to print suspended threads in svchost.exe processes:\n" );
	printf( " %s 1 svchost.exe\n", progname );
	exit( 1 );
}



/* make a wide character string (wstr) from a multibyte character string.
returns NULL on failure or wstr on success.
this function returns malloc'd memory, free() when done.
*/
WCHAR *make_wstr( 
	char *mbstr   // in
)
{
	size_t ecount = 0;
	WCHAR *wstr = NULL;
	
	
	if( !mbstr )
		return NULL;
	
	/* get the number of wide characters, excluding null, 
	needed to output the multibyte string as a wide character string
	*/
	ecount = mbstowcs( NULL, mbstr, 0 );
	if( ecount >= (size_t)INT_MAX ) /* error */
		return NULL;
	
	++ecount; /* add 1 for null terminator */
	
	wstr = calloc( ecount, sizeof( WCHAR ) );
	if( !wstr )
		return NULL;
	
	ecount = mbstowcs( wstr, mbstr, ecount );
	if( ecount >= (size_t)INT_MAX ) /* error */
	{
		free( wstr );
		return NULL;
	}
	
	wstr[ ecount ] = L'\0';
	
	return wstr;
}



int main( int argc, char **argv )
{
	/** init
	*/
	int ret = 0;
	struct your_callback_data stuff;
	
	ZeroMemory( &stuff, sizeof( stuff ) );
	
	
	/** example
	*/
	if( argc < 2 )
		print_usage_and_exit( argv[ 0 ] );
	
	if( argc >= 2 ) /* get suspended thread preference */
	{
		if( argv[ 1 ][ 0 ] == '0' && argv[ 1 ][ 1 ] == '\0' )
			stuff.suspended_threads_only = 0;
		else if( argv[ 1 ][ 0 ] == '1' && argv[ 1 ][ 1 ] == '\0' )
			stuff.suspended_threads_only = 1;
		else
			print_usage_and_exit( argv[ 0 ] );
	}
	
	if( argc == 3 ) /* get the optional process name */
	{
		/* convert the multibyte string containing the process name to 
		a wide character string 
		*/
		stuff.image_name = make_wstr( argv[ 2 ] );
		
		if( !stuff.image_name )
		{
			printf( "FATAL: some error occurred in make_wstr().\n" );
			exit( 1 );
		}
	}
	else if( argc > 3 )
	{
		printf( "FATAL: Too many command line arguments.\n" );
		exit( 1 );
	}
	
	print_license();
	printf( "\n\n" );
	
	printf( 
		"Printing %s %ls threads...\n\n", 
		( stuff.suspended_threads_only ? "suspended" : "all" ), 
		( stuff.image_name ? stuff.image_name : L"" )
	);
	ret = traverse_threads( 
		callback_count_and_print, /* your callback */
		&stuff, /* your callback data */
		NULL, /* your buffer. unused in this example  */
		0, /* your buffer's byte count. unused in this example  */
		0, /* your flags. unused in this example  */
		NULL /* pointer to receive status. unused in this example */
	);
	printf( "\ntraverse_threads() returned: %hs\n", traverse_threads_retcode_to_cstr( ret ) );
	
	printf( 
		"\n%u processes total are in the system and were passed to the callback.\n", 
		stuff.processes_total
	);
	
	if( stuff.image_name )
	{
		printf( "\n%u processes matched name %ls.\n", 
			stuff.processes_that_match_name, 
			stuff.image_name 
		);
	}
	printf( 
		"%u of those processes have one or more suspended threads.\n", 
		stuff.processes_that_have_suspended_threads
	);
	
	printf( 
		"\n%u threads total are in the system and %u of those matched your parameters.\n",
		stuff.threads_total,  
		stuff.threads_that_match
	);
	
	if( stuff.image_name )
		free( stuff.image_name );
	
	return ( ( ret == TRAVERSE_SUCCESS ) ? 0 : 1 );
}
