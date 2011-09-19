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
This file contains functions for printing the structs I'm using from the ReactOS project.
Each function is documented in the comment block above its definition.

-
print_HANDLEENTRY_type()

Print type of a HANDLEENTRY.
-

-
print_HANDLEENTRY_flags()

Print flags for a HANDLEENTRY.
-

-
print_HANDLEENTRY()

Print a HANDLEENTRY struct.
-

-
print_HOOK_flags()

Print flags for a HOOK.
-

-
print_HOOK()

Print a HOOK struct.
-

*/

#include <stdio.h>

#include "util.h"

#include "reactos.h"



#define CTYPENAMES 23
static char *typenames[ CTYPENAMES ] = { 
	"TYPE_FREE", 
	"TYPE_WINDOW", 
	"TYPE_MENU", 
	"TYPE_CURSOR", 
	"TYPE_SETWINDOWPOS", 
	"TYPE_HOOK", 
	"TYPE_CLIPDATA", 
	"TYPE_CALLPROC", 
	"TYPE_ACCELTABLE", 
	"TYPE_DDEACCESS", 
	"TYPE_DDECONV", 
	"TYPE_DDEXACT", 
	"TYPE_MONITOR", 
	"TYPE_KBDLAYOUT", 
	"TYPE_KBDFILE", 
	"TYPE_WINEVENTHOOK", 
	"TYPE_TIMER", 
	"TYPE_INPUTCONTEXT", 
	"TYPE_HIDDATA", 
	"TYPE_DEVICEINFO", 
	"TYPE_TOUCHINPUT", 
	"TYPE_GESTUREINFO", 
	"TYPE_CTYPES"
};



/* print_HANDLEENTRY_type()
Print type of a HANDLEENTRY.
*/
void print_HANDLEENTRY_type( 
	const BYTE bType   // in
)
{
	if( bType < CTYPENAMES )
		printf( "%s ", typenames[ bType ] );
	else
		printf( "<0x%02X> ", (unsigned)bType );
	
	return;
}



/* print_HANDLEENTRY_flags()
Print flags for a HANDLEENTRY.
*/
void print_HANDLEENTRY_flags( 
	const BYTE bFlags   // in
)
{
	if( !bFlags )
		return;
	
	if( bFlags & HANDLEF_DESTROY )
		printf( "HANDLEF_DESTROY " );
	
	if( bFlags & HANDLEF_INDESTROY )
		printf( "HANDLEF_INDESTROY " );
	
	if( bFlags & HANDLEF_INWAITFORDEATH )
		printf( "HANDLEF_INWAITFORDEATH " );
	
	if( bFlags & HANDLEF_FINALDESTROY )
		printf( "HANDLEF_FINALDESTROY " );
	
	if( bFlags & HANDLEF_MARKED_OK )
		printf( "HANDLEF_MARKED_OK " );
	
	if( bFlags & HANDLEF_GRANTED )
		printf( "HANDLEF_GRANTED " );
	
	if( bFlags & ~(unsigned)HANDLEF_VALID )
		printf( "<0x%02X> ", (unsigned)( bFlags & ~(unsigned)HANDLEF_VALID ) );
	
	return;
}



/* print_HANDLEENTRY()
Print a HANDLEENTRY struct.

if the HANDLEENTRY pointer is != NULL then print the HANDLEENTRY
*/
void print_HANDLEENTRY(
	const HANDLEENTRY *const entry   // in
)
{
	const char *const objname = "HANDLEENTRY struct";
	
	
	if( !entry )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	if( entry->pHead )
	{
		PRINT_PTR( entry->pHead->h );
		printf( "entry->pHead->cLockObj: %lu\n", entry->pHead->cLockObj );
	}
	
	PRINT_PTR( entry->pOwner );
	
	printf( "entry->bType: " );
	print_HANDLEENTRY_type( entry->bType );
	printf( "\n" );
	
	printf( "entry->bFlags: " );
	print_HANDLEENTRY_flags( entry->bFlags );
	printf( "\n" );
	
	printf( "entry->wUniq: %u\n", (unsigned)entry->wUniq );
	
	PRINT_SEP_END( objname );
	
	return;
}



/* print_HOOK_flags()
Print flags for a HOOK.
*/
void print_HOOK_flags( 
	const DWORD flags   // in
)
{
	if( flags & HF_GLOBAL )
		printf( "HF_GLOBAL " );
	
	if( flags & HF_ANSI )
		printf( "HF_ANSI " );
	
	if( flags & HF_NEEDHC_SKIP )
		printf( "HF_NEEDHC_SKIP " );
	
	if( flags & HF_HUNG )
		printf( "HF_HUNG " );
	
	if( flags & HF_HOOKFAULTED )
		printf( "HF_HOOKFAULTED " );
	
	if( flags & HF_NOPLAYBACKDELAY )
		printf( "HF_NOPLAYBACKDELAY " );
	
	if( flags & HF_WX86KNOWINDOWLL )
		printf( "HF_WX86KNOWINDOWLL " );
	
	if( flags & HF_DESTROYED )
		printf( "HF_DESTROYED " );
	
	if( flags & ~(DWORD)HF_VALID )
		printf( "<0x%08lX> ", (DWORD)( flags & ~(DWORD)HF_VALID ) );
	
	return;
}



/* print_HOOK()
Print a HOOK struct.

if the HOOK pointer is != NULL then print the HOOK
*/
void print_HOOK(
	const HOOK *const object   // in
)
{
	const char *const objname = "HOOK struct";
	
	
	if( !object )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	PRINT_PTR( object->Head.h );
	printf( "object->Head.cLockObj: %lu\n", object->Head.cLockObj );
	
	PRINT_PTR( object->pti );
	PRINT_PTR( object->rpdesk1 );
	PRINT_PTR( object->pSelf );
	PRINT_PTR( object->phkNext );
	printf( "object->iHook: %d\n", object->iHook );
	printf( "object->offPfn: 0x%08lX\n", object->offPfn );
	
	printf( "object->flags: " );
	print_HOOK_flags( object->flags );
	printf( "\n" );
	
	printf( "object->ihmod: %d\n", object->ihmod );
	PRINT_PTR( object->ptiHooked );
	PRINT_PTR( object->rpdesk2 );
	
	PRINT_SEP_END( objname );
	
	return;
}



