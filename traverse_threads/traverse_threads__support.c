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
This file contains supporting functions for traverse_threads().
Each function is documented in the comment block above its definition.

-
get_teb()

Get the address of the thread environment block of a thread in another process.
-

-
copy_teb()

Copy the thread environment block of a thread in another process.
-

-
callback_print_thread_state()

A default callback used by traverse_threads() if no callback was supplied by the caller.
-

-
print_filetime_as_local()

Print a FILETIME as local time and date. No newline.
-

-
traverse_threads_retcode_to_cstr()

Return the traverse_threads() retcode as its associated user-readable literal single byte string.
-

-
ThreadState_to_cstr()

Return the ThreadState as its associated user-readable literal single byte string.
-

-
WaitReason_to_cstr()

Return the WaitReason as its associated user-readable literal single byte string.
-

-
test_memory()

Test memory for read and/or write access violations using structured exception handling (SEH).
-

*/

#include <stdio.h>
#include <windows.h>

#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"

#include "nt_stuff.h"



/* get_teb()
Get the address of the thread environment block of a thread in another process.

'tid' is the thread id of the thread
'flags' is the optional flags parameter that was passed to traverse_threads() or a callback

returns the TEB address on success
*/
void *get_teb( 
	const DWORD tid,   // in
	const DWORD flags   // in, optional
)
{
	static NTSTATUS (__stdcall *NtQueryInformationThread)(
		HANDLE ThreadHandle,
		int ThreadInformationClass,
		PVOID ThreadInformation,
		ULONG ThreadInformationLength,
		PULONG ReturnLength
	);
	
	struct /* THREAD_BASIC_INFORMATION */
	{
		LONG ExitStatus;
		PVOID TebBaseAddress;
		struct
		{
			HANDLE UniqueProcess;
			HANDLE UniqueThread;
		} ClientId;
		ULONG_PTR AffinityMask;
		LONG Priority;
		LONG BasePriority;
	} tbi;
	
	LONG status = 0;
	HANDLE thread = NULL;
	void *return_code = NULL;
	
	
	if( !NtQueryInformationThread )
	{
		SetLastError( 0 ); // error code is evaluated on success
		*(FARPROC *)&NtQueryInformationThread = 
			(FARPROC)GetProcAddress( GetModuleHandleA( "ntdll" ), "NtQueryInformationThread" );
		
		if( ( flags & TRAVERSE_FLAG_DEBUG ) )
		{
			printf( "GetProcAddress() %s. GLE: %u, NtQueryInformationThread: 0x%p.\n",
				( NtQueryInformationThread ? "success" : "error" ), 
				GetLastError(), 
				NtQueryInformationThread 
			);
		}
		
		if( !NtQueryInformationThread )
			goto cleanup;
	}
	
	SetLastError( 0 ); // error code is evaluated on success
	thread = OpenThread( THREAD_QUERY_INFORMATION, FALSE, tid );
	
	if( ( flags & TRAVERSE_FLAG_DEBUG ) )
	{
		printf( "OpenThread() %s. tid: %u, GLE: %u, Handle: 0x%p.\n",
			( thread ? "success" : "error" ), 
			tid, 
			GetLastError(), 
			thread 
		);
	}
	
	if( !thread )
		goto cleanup;
	
	/* request ThreadBasicInformation */
	status = NtQueryInformationThread( thread, 0, &tbi, sizeof( tbi ), NULL );
	
	if( ( flags & TRAVERSE_FLAG_DEBUG ) )
	{
		printf( "NtQueryInformationThread() %s. status: 0x%08X.\n", 
			( ( status ) ? "!= STATUS_SUCCESS" : "== STATUS_SUCCESS" ),
			(unsigned)status 
		);
	}
	
	if( status ) // || ( tbi.ExitStatus != STATUS_PENDING ) )
		goto cleanup;
	
	return_code = tbi.TebBaseAddress;
	
cleanup:
	
	if( thread )
	{
		BOOL ret = 0;
		
		SetLastError( 0 ); // error code is evaluated on success
		ret = CloseHandle( thread );
		
		if( ( flags & TRAVERSE_FLAG_DEBUG ) )
		{
			printf( "CloseHandle() %s. GLE: %u, Handle: 0x%p\n",
				( ret ? "success" : "error" ), 
				GetLastError(), 
				thread
			);
		}
		
		thread = NULL;
	}
	
	return return_code;
}



/* copy_teb_from_thread()
Copy the thread environment block of a thread in another process.

'pid' is the process id of the thread
'tid' is the thread id of the thread
'flags' is the optional flags parameter that was passed to traverse_threads() or a callback
'*bytes_written' is the number of bytes written to buffer returned by this function

if the TEB size is unknown then the bytes written may be larger than the actual TEB, therefore the
buffer may contain trailing bytes that have nothing to do with the TEB. and if only part of the TEB
could be read then the bytes written is only those bytes. either case is considered a success.

returns a pointer to a buffer containing TEB on success. free() when done.
*/
void *copy_teb_from_thread(
	const DWORD pid,   // in
	const DWORD tid,   // in
	const DWORD flags,   // in, optional
	SIZE_T *bytes_written   // out
)
{
	BOOL ret = 0;
	HANDLE process = NULL;
	void *return_code = NULL;
	void *buffer = NULL;
	size_t buffer_size = 0;
	void *teb = NULL;
	
	
	if( !pid || !tid || !bytes_written )
		goto cleanup;
	
	SetLastError( 0 ); // error code is evaluated on success
	process = OpenProcess( PROCESS_VM_READ, FALSE, pid );
	
	if( ( flags & TRAVERSE_FLAG_DEBUG ) )
	{
		printf( "OpenProcess() %s. pid: %u, GLE: %u, Handle: 0x%p.\n",
			( process ? "success" : "error" ), 
			pid, 
			GetLastError(), 
			process 
		);
	}
	
	if( !process )
		goto cleanup;
	
	teb = get_teb( tid, flags );
	if( !teb )
		goto cleanup;

	/* Determine maximum TEB size */
	{
		DWORD dwOSVersion = GetVersion();
		DWORD dwOSMajorVersion = (BYTE)dwOSVersion;
		DWORD dwOSMinorVersion = (BYTE)( dwOSVersion >> 8 );
#ifdef _M_IX86
		buffer_size = SIZEOF_WIN7_X86_TEB32;

		if( dwOSMajorVersion > 6 || ( dwOSMajorVersion == 6 && dwOSMinorVersion > 1 ) )
			buffer_size *= 2; // size unknown, make a guess
#else
		buffer_size = SIZEOF_WIN8_X64_TEB64;

		if( dwOSMajorVersion > 6 || ( dwOSMajorVersion == 6 && dwOSMinorVersion > 2 ) )
			buffer_size *= 2; // size unknown, make a guess
#endif
	}

	buffer = calloc( 1, buffer_size );
	
	if( ( flags & TRAVERSE_FLAG_DEBUG ) )
	{
		printf( "calloc() %s. bytes: %d\n", 
			( buffer ? "success" : "error" ), 
			buffer_size
		);
	}
	
	if( !buffer )
		goto cleanup;
	
	SetLastError( 0 ); // error code is evaluated on success
	ret = ReadProcessMemory( 
		process, 
		teb,
		buffer, 
		buffer_size,
		bytes_written
	);
	
	if( ( flags & TRAVERSE_FLAG_DEBUG ) )
	{
		printf( "ReadProcessMemory() %s. GLE: %u, bytes read: %Iu, Handle: 0x%p.\n",
			( ret ? "success" : "error" ), 
			GetLastError(), 
			*bytes_written,
			process 
		);
	}
	
	if( !*bytes_written )
		goto cleanup;
	
	return_code = buffer;
	
cleanup:
	if( process )
	{
		SetLastError( 0 ); // error code is evaluated on success
		ret = CloseHandle( process );
		
		if( ( flags & TRAVERSE_FLAG_DEBUG ) )
		{
			printf( "CloseHandle() %s. GLE: %u, Handle: 0x%p\n",
				( ret ? "success" : "error" ), 
				GetLastError(), 
				process
			);
		}
		
		process = NULL;
	}
	
	if( !return_code )
		free( buffer );
	
	return return_code;
}



#ifdef _MSC_VER
#pragma warning(push)  
#pragma warning(disable:4100) /* disable unused parameter warning */
#endif

/* callback_print_thread_state()
A default callback used by traverse_threads() if no callback was supplied by the caller.

this function prints the thread state, thread create time, and also if 
TRAVERSE_FLAG_EXTENDED then any available extended members.

this function requires cb_param to point to a DWORD containing the 
operating system version returned by GetVersion().
*/
int callback_print_thread_state( 
	void *cb_param,   // in, out, optional
	SYSTEM_PROCESS_INFORMATION *const spi,   // in
	SYSTEM_THREAD_INFORMATION *const sti,   // in, optional
	const ULONG remaining,   // in
	const DWORD flags   // in, optional
)
{
	if( !cb_param ) /* missing a pointer to the OS version */
	{
		printf( "aborting: the default callback was not passed a pointer to the OS version.\n" );
		return TRAVERSE_CALLBACK_ABORT;
	}
	
	/* print image name if there is one, or <unknown> if not */
	if( spi->ImageName.Length 
		&& spi->ImageName.Buffer 
#ifdef TRAVERSE_SUPPORT_TEST_MEMORY
		/* check if the memory pointed to is accessible */
		&& ( /* either we're not testing memory or if we are call test_memory() */
			!( flags & TRAVERSE_FLAG_TEST_MEMORY )
			|| test_memory( 
				spi->ImageName.Buffer, 
				( (size_t)spi->ImageName.Length + 1 ), 
				TEST_FLAG_READ 
			)
		)
#endif
		&& *spi->ImageName.Buffer 
	)
		printf( "%.*ls: ", 
			(int)( spi->ImageName.Length / sizeof( WCHAR ) ), 
			spi->ImageName.Buffer 
		);
	else
		printf( "<unknown>: " );
	
	/* print process' id */
	printf( "PID %Iu", (size_t)spi->UniqueProcessId );
	
	/* if the process does not contain any threads then CONTINUE.
	in this case SKIP could be used as well. 
	they both do the same thing when 0 threads.
	*/
	if( !sti ) /* sti == NULL only when process has 0 threads: ZERO_THREADS_OK */
	{
		printf( ". WARNING! This process info contains 0 threads!\n" );
		return TRAVERSE_CALLBACK_CONTINUE;
	}
	
	/* print thread's id */
	printf( ", TID %Iu ", (size_t)sti->ClientId.UniqueThread );
	
	/* print thread state */
	printf( "state %s", ThreadState_to_cstr( sti->ThreadState ) );
	
	/* MS: "Thread Wait Reason is only applicable when the thread is in the Wait state."
	http://support.microsoft.com/?kbid=837372
	*/
	if( (KTHREAD_STATE)sti->ThreadState == Waiting )
	{
		/* the threadstate is waiting. print the reason it's waiting. */
		printf( " (%s", WaitReason_to_cstr( sti->WaitReason ) );
		
		if( (KWAIT_REASON)sti->WaitReason >= MaximumWaitReason )
		{
			/* the waitreason is unknown. print its number for diagnostic purposes. */
			printf( " (%lu)", sti->WaitReason );
		}
		
		printf( ")" );
	}
	else if( (KTHREAD_STATE)sti->ThreadState >= MaximumThreadState )
	{
		/* the threadstate is unknown. print its number for diagnostic purposes. */
		printf( " (%lu)", sti->ThreadState );
	}
	
	printf( ".\n" );
	
	
	/* print the thread's CreateTime as user's local time */
	printf( "CreateTime: " );
	print_filetime_as_local( (FILETIME *)&sti->CreateTime );
	printf( "\n" );
	
	/* don't access extended members unless this flag was passed in: */
	if( ( flags & TRAVERSE_FLAG_EXTENDED ) )
	{
		SYSTEM_EXTENDED_THREAD_INFORMATION *seti = 
			(SYSTEM_EXTENDED_THREAD_INFORMATION *)sti;
		
		/* the cb_param for this callback is set in traverse_threads()
		it's a pointer to the DWORD returned by GetVersion()
		*/
		DWORD dwMajorVersion = (DWORD)( LOBYTE( LOWORD( *(DWORD *)cb_param ) ) );
		DWORD dwMinorVersion = (DWORD)( HIBYTE( LOWORD( *(DWORD *)cb_param ) ) );
		
		
		if( ( ( dwMajorVersion == 5 ) && ( dwMinorVersion >= 1 ) ) // >= XP but < Vista
			|| ( dwMajorVersion >= 6 ) // >= Vista
		) // >= XP
		{
			printf( "StackBase: 0x%p\n", seti->StackBase );
			printf( "StackLimit: 0x%p\n", seti->StackLimit );
			printf( "Win32StartAddress: 0x%p\n", seti->Win32StartAddress );
			
			if( dwMajorVersion >= 6 ) // >= Vista
				printf( "TebAddress: 0x%p\n", seti->TebAddress );
		}
		else
			printf( "Extended members are only available for XP+.\n" );
	}
	
	printf( "\n" );
	
	return TRAVERSE_CALLBACK_CONTINUE;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif



/* print_filetime_as_local()
Print a FILETIME as local time and date. No newline.

This function takes a pointer to a utc system time FILETIME ('ft') and prints it as local time.
No newline is printed.

returns nonzero if the conversion succeeded and printed the local time.
returns zero if the conversion failed and printed "<conversion to local time failed>".
*/
int print_filetime_as_local( 
	const FILETIME *const ft   // in
)
{
	SYSTEMTIME utc, local;
	WORD hour = 0; /* to receive '.wHour' converted to 12 hour time */
	BOOL pm = 0; /* zero for A.M. and nonzero for P.M. */
	
	
	ZeroMemory( &utc, sizeof( utc ) );
	ZeroMemory( &local, sizeof( local ) );
	
	if( !ft
		|| !FileTimeToSystemTime( ft, &utc ) 
		|| !SystemTimeToTzSpecificLocalTime( NULL, &utc, &local ) 
		|| ( local.wHour >= 24 ) 
	)
	{
		printf( "<conversion to local time failed>" );
		return FALSE;
	}
	
	if( local.wHour == 0 )
	{
		hour = 12;
		pm = 0;
	}
	else if( local.wHour < 12 )
	{
		hour = local.wHour;
		pm = 0;
	}
	else if( local.wHour == 12 )
	{
		hour = local.wHour;
		pm = 1;
	}
	else
	{
		hour = local.wHour - 12;
		pm = 1;
	}
	
	printf( "%u:%02u:%02u %s"
		"  %u/%u/%04u", 
		hour, local.wMinute, local.wSecond, ( pm ? "PM" : "AM" ), 
		local.wMonth, local.wDay, local.wYear
	);
	
	return TRUE;
	
	
	/* Notes regarding the validity of member CreateTime.
	The CreateTime associated with some of the Threads of System 
	process is not correct. Like CreateTime == 9672016 for many...
	Process Hacker notes N/A, 
	Process Explorer gives a value that's before the process' CreateTime?
	So if CreateTime != 0 but conversion fails then print Thread's CreateTime 
	the same as the Process CreateTime? But that's not correct.
	So just print that conversion failed.
	*/
}



/* traverse_threads_retcode_to_cstr()
Return the traverse_threads() retcode as its associated user-readable literal single byte string.

if 'retcode' is unknown "TRAVERSE_ERROR_UNKNOWN" is returned.
*/
char *traverse_threads_retcode_to_cstr( 
	const int retcode   // in
)
{
	switch( retcode )
	{
		case TRAVERSE_SUCCESS:   return "TRAVERSE_SUCCESS";
		case TRAVERSE_ERROR_GENERAL:   return "TRAVERSE_ERROR_GENERAL";
		case TRAVERSE_ERROR_MEMORY:   return "TRAVERSE_ERROR_MEMORY";
		case TRAVERSE_ERROR_ALIGNMENT:   return "TRAVERSE_ERROR_ALIGNMENT";
		case TRAVERSE_ERROR_BUFFER_TOO_SMALL:   return "TRAVERSE_ERROR_BUFFER_TOO_SMALL";
		case TRAVERSE_ERROR_QUERY:   return "TRAVERSE_ERROR_QUERY";
		case TRAVERSE_ERROR_CALLBACK:   return "TRAVERSE_ERROR_CALLBACK";
		case TRAVERSE_ERROR_CALCULATION:   return "TRAVERSE_ERROR_CALCULATION";
		case TRAVERSE_ERROR_PARAMETER:   return "TRAVERSE_ERROR_PARAMETER";
		case TRAVERSE_ERROR_ACCESS_VIOLATION:   return "TRAVERSE_ERROR_ACCESS_VIOLATION";
		default:   return "TRAVERSE_ERROR_UNKNOWN";
	}
}



/* ThreadState_to_cstr()
Return the ThreadState as its associated user-readable literal single byte string.

if 'ThreadState' is unknown "Unknown" is returned.

http://processhacker.sourceforge.net/doc/phlib_2include_2ntkeapi_8h.html#a89cf35e06b66523904596d9dbdd93af4
*/
char *ThreadState_to_cstr( 
	const ULONG ThreadState   // in
)
{
	switch( (KTHREAD_STATE)ThreadState )
	{
		case Initialized:   return "Initialized";
		case Ready:   return "Ready";
		case Running:   return "Running";
		case Standby:   return "Standby";
		case Terminated:   return "Terminated";
		case Waiting:   return "Waiting";
		case Transition:   return "Transition";
		case DeferredReady:   return "DeferredReady";
		case GateWait:   return "GateWait";
		default:   return "Unknown";
	}
}



/* WaitReason_to_cstr()
Return the WaitReason as its associated user-readable literal single byte string.

MS: "Thread Wait Reason is only applicable when the thread is in the Wait state."
http://support.microsoft.com/?kbid=837372

if 'WaitReason' is unknown "Unknown" is returned.

http://processhacker.sourceforge.net/doc/phlib_2include_2ntkeapi_8h.html#a32f8868bc010efa7da787526013b93fb
*/
char *WaitReason_to_cstr( 
	const ULONG WaitReason   // in
)
{
	switch( (KWAIT_REASON)WaitReason )
	{
		case Executive:   return "Executive";
		case FreePage:   return "FreePage";
		case PageIn:   return "PageIn";
		case PoolAllocation:   return "PoolAllocation";
		case DelayExecution:   return "DelayExecution";
		case Suspended:   return "Suspended";
		case UserRequest:   return "UserRequest";
		case WrExecutive:   return "WrExecutive";
		case WrFreePage:   return "WrFreePage";
		case WrPageIn:   return "WrPageIn";
		case WrPoolAllocation:   return "WrPoolAllocation";
		case WrDelayExecution:   return "WrDelayExecution";
		case WrSuspended:   return "WrSuspended";
		case WrUserRequest:   return "WrUserRequest";
		case WrEventPair:   return "WrEventPair";
		case WrQueue:   return "WrQueue";
		case WrLpcReceive:   return "WrLpcReceive";
		case WrLpcReply:   return "WrLpcReply";
		case WrVirtualMemory:   return "WrVirtualMemory";
		case WrPageOut:   return "WrPageOut";
		case WrRendezvous:   return "WrRendezvous";
		case Spare2:   return "Spare2";
		case Spare3:   return "Spare3";
		case Spare4:   return "Spare4";
		case Spare5:   return "Spare5";
		case WrCalloutStack:   return "WrCalloutStack";
		case WrKernel:   return "WrKernel";
		case WrResource:   return "WrResource";
		case WrPushLock:   return "WrPushLock";
		case WrMutex:   return "WrMutex";
		case WrQuantumEnd:   return "WrQuantumEnd";
		case WrDispatchInt:   return "WrDispatchInt";
		case WrPreempted:   return "WrPreempted";
		case WrYieldExecution:   return "WrYieldExecution";
		case WrFastMutex:   return "WrFastMutex";
		case WrGuardedMutex:   return "WrGuardedMutex";
		case WrRundown:   return "WrRundown";
		default:   return "Unknown";
	}
}



#ifdef TRAVERSE_SUPPORT_TEST_MEMORY
/* test_memory()
Test memory for read and/or write access violations using structured exception handling (SEH).

======
OVERVIEW:
======

test_memory()'s behavior is similar to IsBadWritePtr()/IsBadReadPtr().

a function like this to catch bad pointers can make a program unreliable, and should not be used.
http://blogs.msdn.com/b/larryosterman/archive/2004/05/18/134471.aspx
http://blogs.msdn.com/b/oldnewthing/archive/2006/09/27/773741.aspx

in traverse_threads(), and its default callback callback_print_thread_state(), this function is 
called to test pointers only in the special case of TRAVERSE_FLAG_TEST_MEMORY.



======
PARAMETERS:
======

# [IN OUT]
# 
# void *mem
#
pointer to some memory


# [IN]
# 
# size_t mem_bcount
#
the size of the memory in bytes


# [IN]
# 
# DWORD flags
#
one or more of the following:

-
TEST_FLAG_READ:

Test reading. Attempt to read first and last byte.
-

-
TEST_FLAG_WRITE:

Test writing. Attempt to overwrite first and last byte to 0.
-



======
RETURN:
======

if the memory is inaccessible or any parameter is zero then zero is returned
if the memory is accessible then nonzero is returned

*/
#pragma optimize( "g", off ) /* disable global optimizations */
#pragma auto_inline( off ) /* disable automatic inlining */
unsigned test_memory( 
	void *const address,   // in, out
	const size_t address_bcount,   // in
	const DWORD flags   // in
)
{
	unsigned a = 0;
	
	if( !address  || !address_bcount || !flags )
		return 0;
	
	__try
	{
		/* test the first and last byte.
		*/
		if( ( flags & TEST_FLAG_READ ) )
		{
			/* the value of 'a' doesn't matter. 
			what's important is both addresses are accessed. 
			*/
			a = *(unsigned char *)address ^ *( (unsigned char *)address + ( address_bcount - 1 ) );
		}
		if( ( flags & TEST_FLAG_WRITE ) )
		{
			*(unsigned char *)address = '\0';
			*( (unsigned char *)address + ( address_bcount - 1 ) ) = '\0';
		}
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		return 0;
	}
	
	/* if this point is reached the memory is accessible. 
	the value of 'a' doesn't matter, but lie and make it look like it does to 
	prevent it from being optimized out.
	*/
	return ( a ? 2 : 3 );
}
#pragma auto_inline( on ) /* revert automatic inlining setting */
#pragma optimize( "g", on ) /* revert global optimizations setting */
#endif /* TRAVERSE_SUPPORT_TEST_MEMORY */
