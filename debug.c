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
This file contains functions to debug problems in GetHooks.
Each function is documented in the comment block above its definition.

-
dump_teb()

Dump to a file the thread environment block of a thread in another process.
-

*/
#pragma warning(disable:4996) /* 'function': was declared deprecated */
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE


#include <stdio.h>

#include "util.h"

/* traverse_threads() */
#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"

#include "debug.h"

/* the global stores */
#include "global.h"



/* dump_teb()
Dump to a file the thread environment block of a thread in another process.

'pid' is the process id of the thread
'tid' is the thread id of the thread
'flags' are the thread traversal flags. currently only TRAVERSE_FLAG_DEBUG is checked

this function does not have any fatal path.

returns nonzero on success
*/
int dump_teb( 
	const DWORD pid,   // in
	const DWORD tid,   // in
	const DWORD flags   // in, optional
)
{
	size_t ret = 0;
	FILE *fp = NULL;
	void *buffer = NULL;
	const unsigned filename_max = 80;
	char *filename = NULL;
	void *pvWin32ThreadInfo = NULL;
	int return_code = 0;
	SIZE_T buffer_size = 0;
	
	
	if( !pid || !tid )
		goto cleanup;
	
	buffer = copy_teb_from_thread( pid, tid, flags, &buffer_size );
	if( !buffer )
		goto cleanup;
	
/* offsetof W32ThreadInfo: 0x40 TEB32, 0x78 TEB64 */
#ifdef _M_IX86
#define OFFSET_OF_W32THREADINFO 0x040
#else
#define OFFSET_OF_W32THREADINFO 0x078
#endif

	pvWin32ThreadInfo = *(void **)( (char *)buffer + OFFSET_OF_W32THREADINFO );
	
	filename = must_calloc( filename_max, sizeof( *filename ) );
	_snprintf( filename, filename_max, "pid%lu_tid%lu_%p.teb", pid, tid, pvWin32ThreadInfo );
	filename[ filename_max - 1 ] = 0;
	
	SetLastError( 0 ); // error code is evaluated on success
	fp = fopen( filename, "wb" );
	
	if( ( flags & TRAVERSE_FLAG_DEBUG ) )
	{
		printf( "fopen() %s. filename: %s, GLE: %lu, fp: 0x%p.\n", 
			( fp ? "success" : "error" ), 
			filename, 
			GetLastError(), 
			fp
		);
	}
	
	if( !fp )
		goto cleanup;
	
	SetLastError( 0 ); // error code is evaluated on success
	ret = fwrite( buffer, 1, buffer_size, fp );
	
	if( ( flags & TRAVERSE_FLAG_DEBUG ) )
	{
		printf( "fwrite() %s. ret: %Iu, GLE: %lu, fp: 0x%p.\n", 
			( ( ret == buffer_size ) ? "success" : "error" ),
			ret, 
			GetLastError(), 
			fp 
		);
	}
	
	if( ret == buffer_size )
	{
		printf( "Dumped TEB to file %s\n", filename );
		return_code = 1;
	}
	
cleanup:
	if( fp )
		fclose( fp );
	
	free( buffer );
	free( filename );
	
	return return_code;
}
