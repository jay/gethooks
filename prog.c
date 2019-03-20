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
This file contains functions for a program store (command line arguments, system info, etc).
Each function is documented in the comment block above its definition.

For now there is only one program store implemented and it's a global store (G->prog).

-
create_prog_store()

Create a program store and its descendants or die.
-

-
get_SharedInfo()

Return the address of Microsoft's SHAREDINFO structure (aka gSharedInfo) or die.
-

-
init_global_prog_store()

Initialize the global program store by storing command line arguments, OS version, etc.
-

-
print_SharedInfo()

Print some pointers from the SHAREDINFO struct.
-

-
print_prog_store()

Print a program store and all its descendants.
-

-
print_global_prog_store()

Print the global program store and all its descendants.
-

-
free_prog_store()

Free a program store and all its descendants.
-

*/

#include <stdio.h>

#include "util.h"

#include "prog.h"

/* the global stores */
#include "global.h"



static void print_prog_store( 
	struct prog *store   // in
);



/* create_prog_store()
Create a program store and its descendants or die.

The program store holds some basic program and system info.
*/
void create_prog_store( 
	struct prog **const out   // out deref
)
{
	struct prog *prog = NULL;
	
	FAIL_IF( !out );
	FAIL_IF( *out );
	
	
	/* allocate a prog store */
	prog = must_calloc( 1, sizeof( *prog ) );
	
	
	*out = prog;
	return;
}



/* get_SharedInfo()
Return the address of Microsoft's SHAREDINFO structure (aka gSharedInfo) or die.
*/
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4054) /* function pointer cast as data pointer */
#endif
#ifdef _M_IX86
SHAREDINFO *get_SharedInfo( void )
{
	static DWORD SharedInfo;
	
	int i = 0;
	char *p = NULL;
	void *User32InitializeImmEntryTable = NULL;
	
	
	if( SharedInfo )
		return (SHAREDINFO *)SharedInfo;
	
	User32InitializeImmEntryTable = 
		(void *)GetProcAddress( LoadLibraryA( "user32" ), "User32InitializeImmEntryTable" );
	
	if( !User32InitializeImmEntryTable )
	{
		MSG_FATAL_GLE( "GetProcAddress() failed." );
		printf( "Failed to get address of User32InitializeImmEntryTable() in user32.dll\n" );
		exit( 1 );
	}
	
	/* This is my C implementation of the algorithm to find SharedInfo created by 
	(alex ntinternals org), originally implemented in asm in EnumWindowsHooks.c
	http://www.ntinternals.org/
	
	This works on XP, Vista, Win7 x86. To be sure an alternate way would be to check the different 
	win32k pdb files and find the addresses there.
	*/
	p = (char *)User32InitializeImmEntryTable;
	for( i = 0; i < 127; ++i )
	{
		if( ( *p++ == 0x50 ) && ( *p == 0x68 ) )
		{
			*( (char *)&SharedInfo + 0 ) = *++p;
			*( (char *)&SharedInfo + 1 ) = *++p;
			*( (char *)&SharedInfo + 2 ) = *++p;
			*( (char *)&SharedInfo + 3 ) = *++p;
			break;
		}
	}
	
	if( !SharedInfo )
	{
		MSG_FATAL( "Failed to get address of SharedInfo. The magic number wasn't found." );
		exit( 1 );
	}
	
	return (SHAREDINFO *)SharedInfo;
}
#else // x64
SHAREDINFO *get_SharedInfo( void )
{
	static SHAREDINFO *SharedInfo;


	if( SharedInfo )
		return (SHAREDINFO *)SharedInfo;

	/* I don't think this will work if <= Vista */
	SharedInfo = (void *)GetProcAddress(GetModuleHandleA("user32"), "gSharedInfo");

	if( !SharedInfo )
	{
		MSG_FATAL( "Failed to get address of SharedInfo. gSharedInfo not found in user32." );
		exit( 1 );
	}

	return (SHAREDINFO *)SharedInfo;
}
#endif
#ifdef _MSC_VER
#pragma warning(pop) /* function pointer cast as data pointer */
#endif



/* init_global_prog_store()
Initialize the global program store by storing command line arguments, OS version, etc.

This function must only be called from the main thread.
For now there is only one program store implemented and it's a global store (G->prog).
*/
void init_global_prog_store( 
	int argc,   // in
	char **argv   // in deref
)
{
	unsigned offsetof_cHandleEntries = 0;
	
	FAIL_IF( !G );   // The global store must exist.
	
	FAIL_IF( G->prog->init_time );   // Fail if this store has already been initialized.
	
	
	/* get_SharedInfo() or die.
	This function loads user32.dll and must be called before any other pointer to GUI related info 
	is initialized.
	*/
	G->prog->pSharedInfo = get_SharedInfo();
	
	
	G->prog->argc = argc;
	G->prog->argv = argv;
	
	/* point pszBasename to this program's basename */
	if( argc && argv[ 0 ][ 0 ] )
	{
		char *clip = NULL;
		
		clip = strrchr( argv[ 0 ], '\\' );
		if( clip )
			++clip;
		
		G->prog->pszBasename = ( clip && *clip ) ? clip : argv[ 0 ];
	}
	else
		G->prog->pszBasename = "<unknown>";
	
	
	/* main thread id */
	G->prog->dwMainThreadId = GetCurrentThreadId();
	
	/* operating system version and platform */
	G->prog->dwOSVersion = GetVersion();
	G->prog->dwOSMajorVersion = (BYTE)G->prog->dwOSVersion;
	G->prog->dwOSMinorVersion = (BYTE)( G->prog->dwOSVersion >> 8 );
	G->prog->dwOSBuild = 
		( G->prog->dwOSVersion < 0x80000000 ) ? ( G->prog->dwOSVersion >> 16 ) : 0;
	
	/* the name of this program's window station */
	if( !get_user_obj_name( &G->prog->pwszWinstaName, GetProcessWindowStation() ) )
	{
		MSG_FATAL_GLE( "get_user_obj_name() failed." );
		printf( "Failed to get this program's window station name.\n" );
		printf( "If you can reproduce this error contact raysatiro@yahoo.com\n" );
		exit( 1 );
	}
	
	
	// Determine the offset of cHandleEntries in SERVERINFO.
#ifdef _M_IX86
	/* In NT4 the offset is 4 but this program isn't for NT4 so ignore.
	In Win2k and XP the offset is 8.
	In Vista and Win7 the offset is 4.
	*/
	offsetof_cHandleEntries = ( G->prog->dwOSMajorVersion >= 6 ) ? 4 : 8;
#else
	/* In Windows 8.1 x64 the offset is 8. */
	offsetof_cHandleEntries = 8;
#endif
	
	/* The first member of SHAREDINFO is pointer to SERVERINFO.
	Add offsetof_cHandleEntries to the SERVERINFO pointer to get the address of cHandleEntries.
	*/
	G->prog->pcHandleEntries = 
		(volatile DWORD *)( (char *)G->prog->pSharedInfo->psi + offsetof_cHandleEntries );
	
	
	/* G->prog has been initialized */
	GetSystemTimeAsFileTime( (FILETIME *)&G->prog->init_time );
	return;
}



/* print_SharedInfo()
Print some pointers from the SHAREDINFO struct.

if 'pSharedInfo' is NULL this function returns without having printed anything.
*/
void print_SharedInfo( 
	const SHAREDINFO *const pSharedInfo   // in
)
{
	const char *const objname = "SHAREDINFO struct";
	
	
	if( !pSharedInfo )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	PRINT_HEX( pSharedInfo->psi );
	PRINT_HEX( pSharedInfo->aheList );
	/* These two have offsets that vary by OS and we currently don't account for that.
	PRINT_HEX( pSharedInfo->pDisplayInfo );
	PRINT_HEX( pSharedInfo->ulSharedDelta );
	*/
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_prog_store()
Print a program store and all its descendants.

if 'store' is NULL this function returns without having printed anything.
*/
static void print_prog_store( 
	struct prog *store   // in
)
{
	const char *const objname = "Program Store";
	int i = 0;
	
	if( !store )
		return;
	
	PRINT_DBLSEP_BEGIN( objname );
	print_init_time( "store->init_time", store->init_time );
	
	printf( "store->argc: %d\n", store->argc );
	for( i = 0; i < store->argc; ++i )
		printf( "store->argv[ %d ]: %s\n", i, store->argv[ i ] );
	
	printf( "store->pszBasename: %s\n", store->pszBasename );
	printf( "store->dwMainThreadId: %u (0x%X)\n", store->dwMainThreadId, store->dwMainThreadId );
	printf( "store->dwOSVersion: %u (0x%X)\n", store->dwOSVersion, store->dwOSVersion );
	printf( "store->dwOSMajorVersion: %u\n", store->dwOSMajorVersion );
	printf( "store->dwOSMinorVersion: %u\n", store->dwOSMinorVersion );
	printf( "store->dwOSBuild: %u\n", store->dwOSBuild );
	printf( "store->pwszWinstaName: %ls\n", store->pwszWinstaName );
	print_SharedInfo( store->pSharedInfo );
	printf( "\n" );
	printf( "*store->pcHandleEntries: %lu\n", *store->pcHandleEntries );
	
	PRINT_DBLSEP_END( objname );
	
	return;
}



/* print_global_prog_store()
Print the global program store and all its descendants.
*/
void print_global_prog_store( void )
{
	print_prog_store( G->prog );
	return;
}



/* free_prog_store()
Free a program store and all its descendants.

this function then sets the prog store pointer to NULL and returns

'in' is a pointer to a pointer to the prog store, which contains the program information.
if( !in || !*in ) then this function returns.
*/
void free_prog_store( 
	struct prog **const in   // in deref
)
{
	if( !in || !*in )
		return;
	
	if( (*in)->pwszWinstaName )
		free( (*in)->pwszWinstaName );
	
	free( (*in) );
	*in = NULL;
	
	return;
}
