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

/**
This file contains traverse_threads(). It is documented in traverse_threads.txt
*/

#include <stdio.h>
#include <windows.h>

#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"

#include "nt_stuff.h"



/** traverse_threads()
This function is well documented in traverse_threads.txt 
*/
int traverse_threads( 
	int ( __cdecl *callback )( 
		void *cb_param,   // in, out, optional
		SYSTEM_PROCESS_INFORMATION *const spi,   // in
		SYSTEM_THREAD_INFORMATION *const sti,   // in, optional
		const ULONG remaining,   // in
		const DWORD flags   // in, optional
	),   // in, optional
	void *cb_param,   // in, out, optional
	void *buffer,   // in, out, optional
	size_t buffer_bcount,   // in, optional
	const DWORD flags,   // in, optional
	LONG *status   // out, optional
)
{
	/** initialization
	*/
	/* only print debug information when debugging */
	#define dbg_printf   if( ( flags & TRAVERSE_FLAG_DEBUG ) )printf
	
	/* function pointer for NtQuerySystemInformation */
	static NTSTATUS (__stdcall *NtQuerySystemInformation)(
		SYSTEM_INFORMATION_CLASS SystemInformationClass,
		PVOID SystemInformation,
		ULONG SystemInformationLength,
		PULONG ReturnLength
	);
	
	/* a pointer to the current spi struct */
	SYSTEM_PROCESS_INFORMATION *spi = NULL;
	
	/* the type of system information that will be requested */
	SYSTEM_INFORMATION_CLASS infotype = -1;
	
	/* the size in bytes of SYSTEM_THREAD_INFORMATION, or if TRAVERSE_FLAG_EXTENDED 
	then SYSTEM_EXTENDED_THREAD_INFORMATION */
	size_t sti_bcount = 0;
	
	/* a pointer to temporary memory malloc'd by this process, and its size in bytes */
	void *memory = NULL;
	size_t memory_bcount = 0;
	
	/* the endpoint of the buffer. 
	NtQuerySystemInformation() has not written at or past this point.
	*/
	size_t buffer_end = 0;
	
	/* if the caller didn't pass in a status pointer then point to this placeholder instead */
	LONG status_placeholder = 0;
	
	/* to receive the buffer length in bytes needed/written by NtQuerySystemInformation() */
	ULONG retlen = 0;
	
	/* the version number of the operating system returned by GetVersion() */
	DWORD dwVersion = 0;
	
	/* error_code is the variable returned by this function */
	int error_code = TRAVERSE_ERROR_GENERAL;
	
	
	/** special sanity struct for internal use only.
	this struct is used for validation on RECYCLE calls.
	it can also be used for diagnostic purposes. if a user sends me their 
	buffer I can easily RECYCLE it and see what went wrong.
	this sanity struct is not written to the user's buffer on return from a RECYCLE call.
	only on original calls (!RECYCLE) will this struct be written to the buffer.
	*/
	struct 
	{
		/* the recycle_verify struct holds the information that must be verified as 
		the exact same on a RECYCLE call.
		This struct must be the first member of the sanity struct.
		*/
		struct
		{
			/* some magic number to signify the beginning of the sanity struct.
			this must be the first member.
			*/
			char magic_begin[ TRAVERSE_MAGIC_LEN ];
			
			/* the sizeof the sanity struct */
			DWORD sanity_size;
			
			/* these are the same as those parameters passed in to this function */
			void *buffer;
			size_t buffer_bcount;
		} recycle_must_verify;
		
		/* the rest of the sanity struct is just any variable I need available across calls, 
		or for diagnostic purposes. Some of these variables might need to be verified, 
		but not necessarily be exactly the same on a RECYCLE call.
		*/
		
		/* a copy of 'flags' */
		DWORD flags;
		
		/* a copy of 'retlen' */
		ULONG retlen;
		
		/* a copy of 'error_code' */
		int error_code;
		
		/* a copy of '*status' */
		LONG status;
		
		/* a copy of 'dwVersion' */
		DWORD dwVersion;
		
		/* a copy of 'reserved' */
		void *reserved;
		
		/* some magic number to signify the end of struct.
		this must be the last member. this must also be verified.
		*/
		char magic_end[ TRAVERSE_MAGIC_LEN ]; 
	} sanity;
	
	/* pointer to the start of the reserved space in buffer. 
	the sanity struct will stored in the reserved space on an original call
	*/
	void *reserved = NULL;
	
	/* the sanity struct's memory must be zeroed to make sure padding is zeroed */
	ZeroMemory( &sanity, sizeof( sanity ) );
	
	/* set all 'sanity.recycle_must_verify' members to prepare for sanity check */
	memcpy( 
		sanity.recycle_must_verify.magic_begin, 
		TRAVERSE_MAGIC_BEGIN, 
		sizeof( sanity.recycle_must_verify.magic_begin ) 
	);
	sanity.recycle_must_verify.sanity_size = sizeof( sanity );
	sanity.recycle_must_verify.buffer = buffer;
	sanity.recycle_must_verify.buffer_bcount = buffer_bcount;
	
	
	
	/** test memory
	*/
#ifdef TRAVERSE_SUPPORT_TEST_MEMORY
	if( ( flags & TRAVERSE_FLAG_TEST_MEMORY ) )
	{
		DWORD memtest_flags = ( TEST_FLAG_READ | TEST_FLAG_WRITE );
		
		if( status
			&& !test_memory( 
				status, 
				sizeof( *status ), 
				memtest_flags
			)
		)
		{
			dbg_printf( "Error: inaccessible memory pointed to by status parameter.\n" );
			
			error_code = TRAVERSE_ERROR_PARAMETER;
			goto quit;
		}
		
		/* test buffer. rem testing for write overwrites two bytes in memory.
		if RECYCLE then the current buffer is for input not output.
		only test WRITE if the current buffer is for output.
		*/
		if( ( flags & TRAVERSE_FLAG_RECYCLE ) ) /* buffer contains valid info, don't overwrite! */
			memtest_flags = TEST_FLAG_READ;
		else
			memtest_flags = ( TEST_FLAG_READ | TEST_FLAG_WRITE );
		
		if( buffer && !test_memory( buffer, buffer_bcount, memtest_flags ) )
		{
			dbg_printf( "Error: inaccessible memory pointed to by buffer parameter.\n" );
			
			error_code = TRAVERSE_ERROR_PARAMETER;
			goto quit;
		}
	}
#endif // TRAVERSE_SUPPORT_TEST_MEMORY
	
	
	
	/** more init
	*/
	/* get OS version */
	dwVersion = GetVersion();
	
	/* determine the type of information being requested */
	if( ( flags & TRAVERSE_FLAG_EXTENDED ) )
	{
		sti_bcount = sizeof( SYSTEM_EXTENDED_THREAD_INFORMATION );
		infotype = SystemExtendedProcessInformation;
		dbg_printf( "Process info type: SystemExtendedProcessInformation\n" );
	}
	else
	{
		sti_bcount = sizeof( SYSTEM_THREAD_INFORMATION );
		infotype = SystemProcessInformation;
		dbg_printf( "Process info type: SystemProcessInformation\n" );
	}
	
	
	if( !status ) /* the caller did not specify a location to receive status. use placeholder */
		status = &status_placeholder;
	
	/* rem initialize after test_memory() call, which changed the pointed to memory */
	*status = -1;
	
	
	if( !NtQuerySystemInformation )
	{
		SetLastError( 0 ); // error code is evaluated on success
		*(FARPROC *)&NtQuerySystemInformation = 
			(FARPROC)GetProcAddress( GetModuleHandleA( "ntdll" ), "NtQuerySystemInformation" );
		
		dbg_printf( "GetProcAddress() %s. GLE: %u, NtQuerySystemInformation: 0x%p.\n",
			( NtQuerySystemInformation ? "success" : "error" ), 
			GetLastError(), 
			NtQuerySystemInformation 
		);
		
		if( !NtQuerySystemInformation )
		{
			error_code = TRAVERSE_ERROR_QUERY;
			goto quit;
		}
	}
	
	
	/* 
	If there is no callback and no output buffer (either no buffer or the buffer 
	is being used as input because TRAVERSE_FLAG_RECYCLE was specified), this 
	function will use the default callback, callback_print_thread_state(), which 
	will be called to printf each thread's state and creation time. All numbers 
	will be in decimal format unless 0x prefixed. 
	*/
	if( !callback && ( !buffer || ( flags & TRAVERSE_FLAG_RECYCLE ) ) )
	{
		callback = callback_print_thread_state;
		cb_param = &sanity.dwVersion;
	}
	
	
	
	/** allocate a buffer if necessary
	*/
	if( !buffer ) /* get buffer size and malloc for memory */
	{
		/* stub address must be aligned, no char */
		SYSTEM_PROCESS_INFORMATION stub;
		
		
		/* if this is a RECYCLE call a buffer should have been passed in */
		if( ( flags & TRAVERSE_FLAG_RECYCLE ) )
		{
			dbg_printf( "Error: missing input buffer (RECYCLE).\n" );
			
			error_code = TRAVERSE_ERROR_PARAMETER;
			goto quit;
		}
		
		
		/* pass in a stub to receive the buffer's approximate needed size */
		retlen = 0;
		dbg_printf( "Calling NtQuerySystemInformation() to get buffer size estimate.\n" );
		*status = (LONG)NtQuerySystemInformation( infotype, &stub, 1, &retlen );
		dbg_printf( 
			"NtQuerySystemInformation() status: 0x%08X retlen: %lu\n\n", 
			(unsigned)*status, 
			retlen
		);
		
		/* the smaller the length returned by NtQuerySystemInformation() the 
		less doubling it will help. here check against a minimum size. 
		this might be unnecessary.
		*/
		if( retlen < 1048576UL )
			retlen = 1048576UL;
		
		/* retlen is a rough estimate, in bytes, of the amount of memory needed.
		the actual amount of memory needed depends on the number of threads 
		and processes at exactly the time the function is called.
		double the size of the length to make sure there's enough space.
		*/
		memory_bcount = retlen * 2;
		if( !memory_bcount )
		{
			dbg_printf( "Error: can't determine memory size. retlen: %lu\n", retlen );
			
			error_code = TRAVERSE_ERROR_MEMORY;
			goto quit;
		}
		
		
		dbg_printf( "Calling malloc( %Iu )\n", memory_bcount );
		memory = malloc( memory_bcount );
		if( !memory )
		{
			dbg_printf( "Error: malloc() failed.\n" );
			
			error_code = TRAVERSE_ERROR_MEMORY;
			goto quit;
		}
		
		/* use the temporary memory as the buffer */
		buffer = memory;
		buffer_bcount = memory_bcount;
	}
	
	/* make sure there's space to allow for sanity struct contents at the end of the buffer */
	if( buffer_bcount <= sizeof( sanity ) )
	{
		dbg_printf( "Error: buffer_bcount is too small. buffer_bcount: %Iu\n", buffer_bcount );
		
		error_code = TRAVERSE_ERROR_PARAMETER;
		goto quit;
	}
	
	/* reserve space for sanity struct by decreasing the available size in bytes of buffer */
	buffer_bcount -= sizeof( sanity );
	reserved = (void *)( (size_t)buffer + buffer_bcount );
	
	
	
	/** if this is a RECYCLE call there's already an array of 
	SYSTEM_PROCESS_INFORMATION structs in the buffer.
	*/
	if( ( flags & TRAVERSE_FLAG_RECYCLE ) ) /* buffer is used for input */
	{
		dbg_printf( "Sanity checking buffer (RECYCLE).\n" );
		
		/* if the buffer is valid it should contain its own copy of 
		'sanity.recycle_must_verify', and its bytes should match this call's
		'sanity.recycle_must_verify' exactly.
		it would be incorrect to cast and access individual members because 
		the location in the buffer holding the struct may not be aligned.
		*/
		if( memcmp( 
				reserved, /* buffer's recycle_must_verify data */
				&sanity.recycle_must_verify, /* our recycle_must_verify data */
				sizeof( sanity.recycle_must_verify ) 
			) 
		) /* the memory does not match, sanity check failed */
		{
			dbg_printf( "Error: Sanity check failed. Memory doesn't match.\n" );
			
			error_code = TRAVERSE_ERROR_PARAMETER;
			goto quit;
		}
		
		/* the buffer contains valid data from a prior call, import the entire sanity struct
		*/
		memcpy( &sanity, reserved, sizeof( sanity ) );
		
		/* do some more error checking */
		if( !sanity.retlen || ( sanity.retlen > buffer_bcount ) )
		{
			dbg_printf( 
				"Error: Sanity check failed. sanity.retlen is out of bounds: %lu\n", 
				sanity.retlen 
			);
			
			error_code = TRAVERSE_ERROR_PARAMETER;
			goto quit;
		}
		
		if( ( ( flags ^ sanity.flags ) & TRAVERSE_FLAG_EXTENDED ) )
		{ /* EXTENDED flag differs from the original call. error */
			dbg_printf( "Error: Sanity check failed. EXTENDED flag differs from original call.\n" );
			
			error_code = TRAVERSE_ERROR_PARAMETER;
			goto quit;
		}
		
		if( memcmp( TRAVERSE_MAGIC_END, sanity.magic_end, sizeof( sanity.magic_end ) ) )
		{ /* missing end magic. maybe the original call didn't exit successfully */
			dbg_printf( 
				"Error: Sanity check failed. End magic incorrect. sanity.error_code: %d\n", 
				sanity.error_code 
			);
			
			error_code = TRAVERSE_ERROR_PARAMETER;
			goto quit;
		}
		
		/* restore the variables needed from the original call */
		retlen = sanity.retlen;
		*status = sanity.status;
		
		dbg_printf( "Sanity check passed (RECYCLE). retlen: %lu\n", retlen );
	}
	/** else this is an original call (!RECYCLE) so call NtQuerySystemInformation() 
	to get an array of SYSTEM_PROCESS_INFORMATION structs
	*/
	else
	{
		retlen = 0;

		if( buffer_bcount > ULONG_MAX )
		{
				dbg_printf( "Error: the buffer is too large.\n" );
				dbg_printf( "buffer_bcount: %Iu\n", buffer_bcount );

				error_code = TRAVERSE_ERROR_MEMORY;
				goto quit;
		}

		dbg_printf( "Calling NtQuerySystemInformation() to get process info.\n" );
		*status = (LONG)NtQuerySystemInformation( 
			infotype, 
			buffer, 
			(ULONG)buffer_bcount,
			&retlen 
		);
		dbg_printf( 
			"NtQuerySystemInformation() status: 0x%08X retlen: %lu\n\n", 
			(unsigned)*status, 
			retlen
		);
		
		/* check for STATUS_DATATYPE_MISALIGNMENT failure */
		if( *status == (LONG)0x80000002L )
		{
			dbg_printf( "Error: STATUS_DATATYPE_MISALIGNMENT\n" );
			dbg_printf( "buffer: %Iu\n", (size_t)buffer );
			
			error_code = TRAVERSE_ERROR_ALIGNMENT;
			goto quit;
		}
		
		/* check for STATUS_INFO_LENGTH_MISMATCH failure */
		if( ( retlen > buffer_bcount ) || ( *status == (LONG)0xC0000004L ) )
		{
			dbg_printf( "Error: the buffer is too small.\n" );
			dbg_printf( 
				"status: 0x%08X, buffer_bcount: %Iu, retlen: %lu\n", 
				(unsigned)*status, 
				buffer_bcount, 
				retlen
			);
			
			error_code = TRAVERSE_ERROR_BUFFER_TOO_SMALL;
			goto quit;
		}
		
		/* check for some other status code. it would not be safe to continue */
		if( *status || ( retlen < sizeof( SYSTEM_PROCESS_INFORMATION ) ) )
		{
			dbg_printf( "Error: NtQuerySystemInformation() failed.\n" );
			dbg_printf( "status: 0x%08X\n", (unsigned)*status );
			
			error_code = TRAVERSE_ERROR_QUERY;
			goto quit;
		}
	}
	
	
	
	/** main loop.
	traverse the SYSTEM_THREAD_INFORMATION struct array in each 
	SYSTEM_PROCESS_INFORMATION struct.
	*/
	/* point to the first SYSTEM_PROCESS_INFORMATION struct */
	spi = (SYSTEM_PROCESS_INFORMATION *)buffer;
	
	/* the endpoint address of the array of SYSTEM_PROCESS_INFORMATION structs */
	buffer_end = (size_t)buffer + retlen;
	
	for( ;; )
	{
		/** initialization 
		*/
		/* the endpoint address of the currently pointed to SYSTEM_PROCESS_INFORMATION struct */
		size_t spi_end = 
			( spi->NextEntryOffset ? ( (size_t)spi + spi->NextEntryOffset ) : buffer_end );
		
		/* how many bytes are needed to hold the reported size of the 
		SYSTEM_THREAD_INFORMATION struct array in this spi
		*/
		unsigned __int64 threads_bcount = 
			(unsigned __int64)(ULONG)spi->NumberOfThreads * sti_bcount;
		
		/* how many SYSTEM_THREAD_INFORMATION structs are in the currently pointed to 
		SYSTEM_PROCESS_INFORMATION struct (spi)
		*/
		ULONG threads_ecount = 0; // calculated after error checks
		
		/* the endpoint address of the array of SYSTEM_THREAD_INFORMATION structs 
		in the currently pointed to SYSTEM_PROCESS_INFORMATION struct (spi)
		*/
		size_t threads_end = (size_t)( (size_t)&spi->Threads + threads_bcount );
		
		
		
		/** check for calculation errors
		*/
		dbg_printf( "============================================\n" );
		
		/* this would only happen in the case of garbage data: */
		if( ( threads_end < (size_t)buffer ) 
			|| ( spi_end < (size_t)buffer ) 
			|| ( threads_end < (size_t)&spi->Threads ) 
			|| ( spi_end < (size_t)&spi->Threads ) 
		) 
		{
			dbg_printf( "Error: Definite garbage data, quitting...\n" );
			
			error_code = TRAVERSE_ERROR_CALCULATION;
			goto quit;
		}
		
		/* check if fewer accessible threads than reported */
		if( ( threads_end > buffer_end ) 
			|| ( threads_end > spi_end ) 
			|| ( spi_end > buffer_end )
		)
		{
			if( ( flags & TRAVERSE_FLAG_DEBUG ) )
			{
				printf( "-\n" );
				printf( "Error: process info may contain fewer thread structs than reported.\n" );
				
				printf( "All numbers not prefixed with 0x are in decimal format.\n" );
				
				if( ( flags & TRAVERSE_FLAG_EXTENDED ) )
					printf( "TRAVERSE_FLAG_EXTENDED is set.\n" );
				
				printf( "buffer: %Iu\n", (size_t)buffer );
				printf( "buffer_end: %Iu\n", buffer_end );
				printf( "spi: %Iu\n", (size_t)spi );
				printf( "spi_end: %Iu\n", spi_end );
				printf( "threads_end: %Iu\n", threads_end );
				
				printf( "spi->NumberOfThreads: %lu\n", spi->NumberOfThreads );
				printf( "sti_bcount: %Iu\n", sti_bcount );
				printf( 
					"bytes needed(%lu*%Iu): %I64u\n", 
					spi->NumberOfThreads,
					sti_bcount,
					threads_bcount
				);
				
				printf( "( &spi->Threads ): %Iu\n", (size_t)&spi->Threads );
				printf( 
					"}} calculated thread struct array endpoint (%Iu+%I64u): %Iu\n\n", 
					(size_t)&spi->Threads, 
					threads_bcount,
					threads_end
				);
				
				printf( "spi struct address: %Iu\n", (size_t)spi );
				printf( "array of spi structs endpoint: %Iu\n", buffer_end );
				printf( "spi->NextEntryOffset: %lu\n", spi->NextEntryOffset );
				if( spi->NextEntryOffset )
				{
					printf( 
						"}} calculated process struct endpoint (%Iu+%lu): %Iu\n",
						(size_t)spi, 
						spi->NextEntryOffset, 
						( (size_t)spi + (size_t)spi->NextEntryOffset )
					);
				}
				else
				{
					printf( "This is reportedly the last process struct in the array.\n" );
					printf( 
						"}} calculated process struct endpoint (array endpoint): %Iu\n",
						buffer_end
					);
				}
				
				printf( "-\n" );
			}
			
			/* if fewer accessible threads there is either a bug in this 
			program or NtQuerySystemInformation reported incorrectly?
			quit unless IGNORE_CALCULATION_ERRORS was specified.
			*/
			if( !( flags & TRAVERSE_FLAG_IGNORE_CALCULATION_ERRORS ) )
			{
				if( ( flags & TRAVERSE_FLAG_DEBUG ) )
					printf( "Error: Calculation Error, quitting...\n" );
				
				error_code = TRAVERSE_ERROR_CALCULATION;
				goto quit;
			}
			
			
			/** since IGNORE_CALCULATION_ERRORS was specified, 
			attempt to recover by adjusting the endpoints so they aren't out-of-bounds
			*/
			if( spi_end > buffer_end )
				spi_end = buffer_end;
			
			if( threads_end > spi_end )
				threads_end = spi_end;
			
			if( threads_end < (size_t)&spi->Threads )
			{
				dbg_printf( "Error: Definite garbage data, quitting...\n" );
				
				error_code = TRAVERSE_ERROR_CALCULATION;
				goto quit;
			}
			
			dbg_printf( "recovered. spi_end: %Iu, threads_end: %Iu\n", spi_end, threads_end );
			dbg_printf( "-\n\n" );
		}
		
		
		
		/** print ImageName.Buffer (process' name) and info if accessible
		*/
		dbg_printf( "UniqueProcessId: %Iu\n", (size_t)spi->UniqueProcessId );
		dbg_printf( "NumberOfThreads: %lu\n", spi->NumberOfThreads );
		dbg_printf( "ImageName.Length: %hu\n", spi->ImageName.Length );
		dbg_printf( "Checking for image name...\n" );
		
		if( !spi->ImageName.Buffer )
		{
			dbg_printf( "ImageName.Buffer: <ImageName.Buffer == 0>\n" );
		}
		else if( !spi->ImageName.Length )
		{
			dbg_printf( "ImageName.Buffer: <ImageName.Length == 0>\n" );
		}
		else if( ( (size_t)spi->ImageName.Buffer < threads_end )
			|| ( ( (size_t)spi->ImageName.Buffer + spi->ImageName.Length ) > spi_end )
		) /* ImageName.Buffer does not point to memory in the current process info (spi) */
		{
			if( ( flags & TRAVERSE_FLAG_DEBUG ) )
			{
				printf( "ImageName.Buffer: <out of range: address 0x%0*IX, bytes: %Iu>\n", 
					(int)( sizeof( size_t ) * 2 ), 
					(size_t)spi->ImageName.Buffer, 
					( (size_t)spi->ImageName.Length + 1 ) 
				);
				printf( "Warning: ImageName.Buffer is out of process info memory range!\n" );
				printf( "All numbers not prefixed with 0x are in decimal format.\n" );
				printf( "spi->ImageName.Buffer: 0x%0*IX (%Iu)\n", 
					(int)( sizeof( size_t ) * 2 ), 
					(size_t)spi->ImageName.Buffer,
					(size_t)spi->ImageName.Buffer
				);
				printf( "spi->ImageName.Length: %hu\n", spi->ImageName.Length );
				printf( "threads_end: %Iu\n", threads_end );
				printf( "spi: %Iu\n", (size_t)spi );
				printf( "spi_end: %Iu\n", spi_end );
				printf( "buffer: %Iu\n", (size_t)buffer );
				printf( "buffer_end: %Iu\n", buffer_end );
			}
			
			if( !( flags & TRAVERSE_FLAG_IGNORE_CALCULATION_ERRORS ) )
			{
				dbg_printf( "Error: Calculation Error, quitting...\n" );
				
				error_code = TRAVERSE_ERROR_CALCULATION;
				goto quit;
			}
		}
		else
		{
			/* position of terminating null in ImageName.Buffer */
			unsigned epos = spi->ImageName.Length / sizeof( WCHAR );
			
			/* warn if there's no terminating null */
			if( spi->ImageName.Buffer[ epos ] )
				dbg_printf( "Warning: <ImageName.Buffer[ %hu ] != 0>\n", epos );
			
			dbg_printf( "ImageName.Buffer: %.*ls\n", (int)epos, spi->ImageName.Buffer );
		}
		
		
		
		/** calculate how many elements in this process' SYSTEM_THREAD_INFORMATION 
		array, then compare to the process' member NumberOfThreads.
		*/
		threads_ecount = 
			(ULONG)( ( threads_end - (size_t)&spi->Threads ) / sti_bcount );
		
		if( threads_ecount != spi->NumberOfThreads )
		{
			if( ( flags & TRAVERSE_FLAG_DEBUG ) )
			{
				printf( "Error: threads_ecount != spi->NumberOfThreads\n" );
				printf( "All numbers not prefixed with 0x are in decimal format.\n" );
				printf( "sti_bcount: %Iu\n", sti_bcount );
				printf( "( &spi->Threads ): %Iu\n", (size_t)&spi->Threads );
				printf( "threads_end: %Iu\n", threads_end );
				printf( "threads_ecount: %lu\n", threads_ecount );
				printf( "spi->NumberOfThreads: %lu\n", spi->NumberOfThreads );
				printf( 
					"}} calculated thread count ((%Iu-%Iu)/%Iu): %lu\n",
					threads_end, 
					(size_t)&spi->Threads, 
					sti_bcount, 
					threads_ecount
				);
			}
			
			if( !( flags & TRAVERSE_FLAG_IGNORE_CALCULATION_ERRORS ) )
			{
				dbg_printf( "Error: Calculation Error, quitting...\n" );
				
				error_code = TRAVERSE_ERROR_CALCULATION;
				goto quit;
			}
			
			/* TRAVERSE_FLAG_IGNORE_CALCULATION_ERRORS is specified */
			
			dbg_printf( 
				"Warning: threads_ecount (%lu) != spi->NumberOfThreads (%lu)\n",
				threads_ecount, 
				spi->NumberOfThreads
			);
			
			/* if the calculated count is bigger than the reported count then recover */
			if( threads_ecount > spi->NumberOfThreads )
			{
				threads_ecount = spi->NumberOfThreads;
				dbg_printf( "recovered. " );
			}
			dbg_printf( "using threads_ecount: %lu\n", threads_ecount );
		}
		
		
		
		/** callback if there are threads to be processed, or zero threads is ok
		*/
		if( callback 
			&& ( threads_ecount || ( flags & TRAVERSE_FLAG_ZERO_THREADS_OK ) ) 
		)
		{
			/* a pointer to the current thread info struct. cast size_t for pointer arithmetic */
			SYSTEM_THREAD_INFORMATION *sti = NULL;
			
			/* how many threads in this spi have not yet been processed */
			ULONG remaining = 0;
			
			if( threads_ecount ) /* there are thread info structs to be processed */
			{
				sti = (SYSTEM_THREAD_INFORMATION *)&spi->Threads;
				remaining = threads_ecount - 1;
			}
			
			for( ;; ) /* for each thread info in the current spi */
			{
				/* callback return code */
				int ret;
				
				
				if( ( flags & TRAVERSE_FLAG_DEBUG ) )
				{
					printf( 
						">>>Calling callback function on process id %Iu, thread id ",
						(size_t)spi->UniqueProcessId
					);
					
					if( sti )
						printf( "%Iu.", (size_t)sti->ClientId.UniqueThread );
					else
						printf( "(null)." );
					
					printf( "\n" );
				}
				
				ret = callback( 
					cb_param, /* the cb_param that was passed in to traverse_threads()*/
					spi, /* the current process info struct */
					sti, /* the current thread info struct */
					remaining, /* how many threads in this spi have not yet been processed */
					flags /* the flags that were passed in to traverse_threads() */
				);
				
				if( ret == TRAVERSE_CALLBACK_SKIP ) /* do not process spi's remaining threads */
				{
					dbg_printf( 
						"<<<Callback function returned: skip process' remaining threads.\n" 
					);
					
					break;
				}
				else if( ret != TRAVERSE_CALLBACK_CONTINUE ) /* some other problem. quit */
				{
					dbg_printf( 
						"<<<Callback function returned: abort immediately. ret: %d\n",
						ret 
					);
					
					error_code = TRAVERSE_ERROR_CALLBACK;
					goto quit;
				}
				
				dbg_printf( "<<<Callback returned normally.\n\n" );
				
				
				if( !remaining ) /* no more threads in this spi */
					break;
				
				--remaining;
				sti = (SYSTEM_THREAD_INFORMATION *)( (size_t)sti + sti_bcount );
			}
		}
		
		/* break if there are no more spi structs to process */
		if( !spi->NextEntryOffset || spi_end == buffer_end )
			break;
		
		/* the endpoint of the current spi struct is a pointer to the next spi struct in the array 
		*/
		spi = (SYSTEM_PROCESS_INFORMATION *)spi_end;
	} /** loop to process the next spi */
	
	
	error_code = TRAVERSE_SUCCESS;
	
	
quit:
	dbg_printf( "============================================\n\n" );
	
	/* if memory isn't null then temporary memory was allocated for buffer
	( ie no buffer was passed in )
	*/
	if( memory )
	{
		free( memory );
		memory = NULL;
		buffer = NULL;
	}
	
	/* else if there was a buffer passed in by the caller and this is an 
	original call (ie not a RECYCLE) and reserved space is available then 
	write the sanity struct to the buffer. 
	*/
	if( buffer && reserved && !( flags & TRAVERSE_FLAG_RECYCLE ) )
	{
		const char *magic_end_str = NULL;
		
		/* if this original call wasn't a (partial) success then invalidate the magic end.
		the buffer will not be usable for a future RECYCLE call.
		*/
		if( ( error_code != TRAVERSE_SUCCESS ) && ( error_code != TRAVERSE_ERROR_CALLBACK ) )
			magic_end_str = TRAVERSE_MAGIC_BAD;
		else // buffer is ok
			magic_end_str = TRAVERSE_MAGIC_END;
		
		memcpy( sanity.magic_end, magic_end_str, sizeof( sanity.magic_end ) );
		
		/* all the variables to keep track of */
		sanity.flags = flags;
		sanity.retlen = retlen;
		sanity.error_code = error_code;
		sanity.status = *status;
		sanity.dwVersion = dwVersion;
		sanity.reserved = reserved;
		
		/* write the sanity struct to the end of the buffer in the location reserved earlier */
		memcpy( 
			reserved, /* location of reserved space */
			&sanity, /* address of sanity struct */
			sizeof( sanity ) /* size of sanity struct == size of reserved space */
		);
	}
	
	return error_code;
}


