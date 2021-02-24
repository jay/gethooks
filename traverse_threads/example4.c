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

/** The first part of this example calls traverse_threads() in debug mode, and 
a buffer is passed in to receive output allowing the caller to do their own 
processing of the array received after traverse_threads() calls 
NtQuerySystemInformation() with SystemProcessInformation or 
SystemExtendedProcessInformation (TRAVERSE_FLAG_EXTENDED).

traverse_threads() is designed to work with a callback although in this case 
one isn't passed in. And because an output buffer is specified the default 
callback, callback_print_thread_state(), will not be called. This is 
defined behavior, however there are few reasons to do what is done in this 
example. It's easiest to work with a callback if you can.


The second part of this example calls traverse_threads() again, this time 
using the output buffer from the first call as the input buffer for the second 
call (recycle). Again, a callback is not specified. However when an output 
buffer is passed in as input (TRAVERSE_FLAG_RECYCLE) and no callback is 
specified traverse_threads() calls the default callback, 
callback_print_thread_state() in traverse_threads__support.c

First build the traverse_threads library (see BUILD.txt).

-
MinGW:

gcc -I..\include -L..\lib -o example4 example4.c -ltraverse_threads -lntdll
-

-
Visual Studio (*from Visual Studio command prompt):

cl /I..\include example4.c ..\lib\traverse_threads.lib ..\lib\ntdll.lib
-
*/

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



/* your own processing of the SYSTEM_PROCESS_INFORMATION array.
returns 0 on success and nonzero on error
*/
int your_processing( void *const buffer, const DWORD flags )
{
	SYSTEM_PROCESS_INFORMATION *spi = NULL;
	SYSTEM_THREAD_INFORMATION *sti = NULL;
	SYSTEM_EXTENDED_THREAD_INFORMATION *seti = NULL;
	
	if( !buffer )
		return 1;
	
	spi = (SYSTEM_PROCESS_INFORMATION *)buffer;
	
	do
	{
		int i = 0;
		
		/* SYSTEM_THREAD_INFORMATION is the first member of 
		SYSTEM_EXTENDED_THREAD_INFORMATION, and that allows for using the 
		former to access members in either case.
		*/
		sti = (SYSTEM_THREAD_INFORMATION *)&spi->Threads;
		
		if( ( flags & TRAVERSE_FLAG_EXTENDED ) )
			seti = (SYSTEM_EXTENDED_THREAD_INFORMATION *)&spi->Threads;
		else
			seti = NULL;
		
		for( i = 0; i < spi->NumberOfThreads; ++i )
		{
			printf( "Thread Id: %Iu", sti->ClientId.UniqueThread );
			
			if( seti )
				printf( ", StackBase: %p", seti->StackBase );
			
			printf( "\n" );
			
			/* increment to the next (extended) thread info in the array */
			if( ( flags & TRAVERSE_FLAG_EXTENDED ) )
			{
				++seti;
				sti = (SYSTEM_THREAD_INFORMATION *)seti;
			}
			else
			{
				++sti;
				seti = NULL;
			}
		}
		
		spi = (SYSTEM_PROCESS_INFORMATION *)( (size_t)spi + spi->NextEntryOffset );
	} while( spi->NextEntryOffset );
	
	return 0;
}



void print_usage_and_exit( char *progname )
{
	print_license();
	printf( "\n\n" );
	
	printf( "this program prints the id [and extended info] of all threads\n" );
	
	printf( "usage: %s <extended>\n\n", progname );
	
	printf( "<extended>: specify 1 to get extended thread info or 0 to just get regular\n");
	
	printf( "\nexample to print the extended info of all threads\n" );
	printf( " %s 1 \n", progname );
	exit( 1 );
}



int main( int argc, char **argv )
{
	/** init
	*/
	/* to receive any function's return code */
	int ret = 0;
	
	/* pointer to your buffer */
	void *buffer = NULL;
	
	/* buffer size in bytes */
	size_t buffer_bcount = 0;
	
	/* to receive the ntstatus if there's some sort of error */
	LONG status = 0;
	
	/* flags to pass to traverse_threads() */
	DWORD flags = 0;
	
	/* nonzero if console output and zero otherwise */
	unsigned console = 0;
	
	
	
	/** example
	*/
	if( argc <= 1 )
		print_usage_and_exit( argv[ 0 ] );
	
	if( argc > 1 ) /* get extended thread info preference */
	{
		if( argv[ 1 ][ 0 ] == '1' && argv[ 1 ][ 1 ] == '\0' )
			flags |= TRAVERSE_FLAG_EXTENDED; /* add extended flag */
		else if( !( argv[ 1 ][ 0 ] == '0' && argv[ 1 ][ 1 ] == '\0' ) )
			print_usage_and_exit( argv[ 0 ] );
	}
	
	/* check if stdout is going to a console */
	console = ( GetFileType( GetStdHandle( STD_OUTPUT_HANDLE ) ) == FILE_TYPE_CHAR );
	
	
	print_license();
	printf( "\n\n" );
	
	
	/* 12MB is enough space to get information on tens of thousands of threads */
	buffer_bcount = 12582912;

	printf( "allocating buffer. malloc(%Iu)...\n", buffer_bcount );
	buffer = malloc( buffer_bcount );	
	if( !buffer )
	{
		printf( "FATAL: buffer malloc(%Iu) failed. Exiting.\n" );
		exit( 1 );
	}
	
	
	printf( 
		"\n\n\n"
		"Example where no callback will be called by traverse_threads():\n"
		"Calling traverse_threads() with an output buffer to receive an array of \n"
		"SYSTEM_PROCESS_INFORMATION structs. When there is an output buffer specified \n"
		"but no callback, no callback will be called. Calling with DEBUG flag.\n"
	);
	
	if( ( flags & TRAVERSE_FLAG_EXTENDED ) )
		printf( "Also calling with EXTENDED flag.\n " );
	
	printf( "\n\n\n" );
	
	flags |= TRAVERSE_FLAG_DEBUG; /* add debug flag */
	
	if( console )
	{
		printf( "press enter to continue...\n" ); 
		getchar();
	}
	
	printf( "Outputting to buffer and using DEBUG mode...\n" );
	ret = traverse_threads( 
		NULL, /* your callback. unused in this example */
		NULL, /* an optional pointer to pass to the callback. unused in this example */
		buffer, /* your buffer. used for output in this example */
		buffer_bcount, /* your buffer's byte count */
		flags, /* your flags */
		&status /* pointer to receive status */
	);
	printf( "traverse_threads() returned: %hs\n\n", traverse_threads_retcode_to_cstr( ret ) );
	
	if( ret == TRAVERSE_ERROR_QUERY )
		printf( "NtQuerySystemInformation() status: 0x%08X \n", status );
	
	/* if ret TRAVERSE_SUCCESS or TRAVERSE_ERROR_CALLBACK you could do something 
	with buffer output. or nothing. it's easier to use your callback function instead, 
	which I expect would usually be the point of calling traverse_threads().
	*/
	if( 
		( ret == TRAVERSE_SUCCESS ) 
		/* traverse_threads() completed successfully. the buffer is valid and has been 
		sanity checked ( already looped through and no errors found )
		*/
		
		|| ( ret == TRAVERSE_ERROR_CALLBACK ) 
		/* traverse_threads() aborted because the callback returned abort.
		the buffer is valid but has not been sanity checked past the abort point.
		
		this check is for demonstration purposes only, and would not happen in 
		this part of the example because there's an output buffer and no user
		specified callback which means no callback would be called and this 
		error would not be returned.
		*/
	)
	{
		/* do some extra processing that your callback for some reason couldn't do */
		printf( 
			"\n\n\n"
			"Now calling your_processing() to postprocess the buffer. As noted in the \n"
			"comments it's easier to use a callback function instead, which I expect would \n"
			"usually be the reason for calling traverse_threads().\n"
			"\n\n\n"
		);
		
		if( console )
		{
			printf( "press enter to continue...\n" ); 
			getchar();
		}
		
		ret = your_processing( buffer, flags );
		if( ret )
		{
			printf( "FATAL: your_processing() failed.\n" );
			goto quit;
		}
	}
	else
	{
		printf( "FATAL: Buffer output is invalid!\n" );
		goto quit;
	}
	
	
	
	printf( 
		"\n\n\n"
		"TRAVERSE_FLAG_RECYCLE example:\n"
		"Calling traverse_threads() again and using the output buffer from the \n"
		"original call as input (recycle). When there is no callback and no output \n"
		"buffer (either no buffer or the buffer is being used as input \n"
		"because TRAVERSE_FLAG_RECYCLE was specified) traverse_threads() calls the \n"
		"default callback, callback_print_thread_state() in traverse_threads__support.c\n"
		"\n"
		"Calling with RECYCLE and ZERO_THREADS_OK (zero thread processes will also be \n"
		"passed to the callback) flags.\n"
	);
	
	if( ( flags & TRAVERSE_FLAG_EXTENDED ) )
		printf( "Also calling with EXTENDED flag.\n " );
	
	printf( "\n\n\n" );
	
	flags &= ~TRAVERSE_FLAG_DEBUG; /* remove debug flag */
	flags |= TRAVERSE_FLAG_RECYCLE; /* add recycle flag */
	flags |= TRAVERSE_FLAG_ZERO_THREADS_OK; /* add zero thread processes flag */
	
	if( console )
	{
		printf( "press enter to continue...\n" ); 
		getchar();
	}
	
	printf( "Recycling buffer (using it as input)...\n\n" );
	ret = traverse_threads( 
		NULL, /* your callback. unused in this example */
		NULL, /* an optional pointer to pass to the callback. unused in this example */
		buffer, /* your buffer. used for input in this example */
		buffer_bcount, /* your buffer's byte count */
		flags, /* your flags */
		NULL /* pointer to receive status. unused in this example */
	);
	printf( "\ntraverse_threads() returned: %hs\n\n", traverse_threads_retcode_to_cstr( ret ) );
	
quit:
	printf( "free()ing buffer.\n" );
	free( buffer );
	
	if( console )
	{
		printf( "done, quitting. press enter to continue...\n" ); 
		getchar();
	}
	
	return !!ret;
}
