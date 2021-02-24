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

/** This is a very simple example for traverse_threads() that prints all 
process ids. First build the traverse_threads library (see BUILD.txt).

-
MinGW:

gcc -I..\include -L..\lib -o example2 example2.c -ltraverse_threads -lntdll
-

-
Visual Studio (*from Visual Studio command prompt):

cl /I..\include example2.c ..\lib\traverse_threads.lib ..\lib\ntdll.lib
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



/* example of simple callback. print the process id. 

note that 'sti' is checked for NULL, but doesn't need to be if you know for 
certain that you will not pass TRAVERSE_FLAG_ZERO_THREADS_OK to 
traverse_threads(). if you don't know or for safety purposes then check 'sti' 
as in the example.
*/
int callback_print_process_id( 
	void *cb_param,   // in, out, optional
	SYSTEM_PROCESS_INFORMATION *const spi,   // in
	SYSTEM_THREAD_INFORMATION *const sti,   // in, optional
	const ULONG remaining,   // in
	const DWORD flags   // in, optional
)
{
	if( !sti ) /* no thread info with this spi, this callback wasn't expecting that! */
		return TRAVERSE_CALLBACK_ABORT;
	
	printf( "callback says spi->UniqueProcessId: %Iu\n", spi->UniqueProcessId );
	
	/* since we are only interested in processes and not threads, 
	we can skip each process' additional threads. 
	in this way the callback is called only once for each process.
	*/
	return TRAVERSE_CALLBACK_SKIP; /* skip to the next process */
}



int main( int argc, char **argv )
{
	/** init
	*/
	int ret = 0;
	
	
	/** example
	*/
	print_license();
	printf( "\n\n" );
	
	printf( "Printing all process ids...\n\n" );
	ret = traverse_threads( callback_print_process_id, NULL, NULL, 0, 0, NULL );
	printf( "\ntraverse_threads() returned: %hs\n", traverse_threads_retcode_to_cstr( ret ) );
	
	return ( ( ret == TRAVERSE_SUCCESS ) ? 0 : 1 );
}
