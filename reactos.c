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
w_handlenames[], w_handlenames_count

An array of user-readable wide names of HANDLEENTRY types.
-

-
print_HANDLEENTRY_type()

Print user-readable name of a HANDLEENTRY's type. No newline.
-

-
print_HANDLEENTRY_flags()

Print user-readable names of a HANDLEENTRY's flags. No newline.
-

-
print_HANDLEENTRY()

Print a HANDLEENTRY struct.
-

-
w_hooknames[], w_hooknames_count

An array of user-readable wide names of HOOK ids. Add 1 to an id to get its position in the index.
-

-
print_HOOK_id()

Print user-readable name of a HOOK's id. No newline.
-

-
print_HOOK_flags()

Print user-readable names of a HOOK's flags. No newline.
-

-
print_HOOK_anomalies()

Print any anomalies found in a HOOK struct.
-

-
print_HOOK()

Print a HOOK struct.
-

-
get_HOOK_name_from_id()

Get the HOOK name from its id.
-

-
get_HOOK_id_from_name()

Get the HOOK id from its name.
-

*/

#include <stdio.h>

#include "util.h"

#include "reactos.h"



/* w_handlenames[]
An array of user-readable wide names of HANDLEENTRY types.
*/
const WCHAR *const w_handlenames[ 23 ] = 
{ 
	L"TYPE_FREE", 
	L"TYPE_WINDOW", 
	L"TYPE_MENU", 
	L"TYPE_CURSOR", 
	L"TYPE_SETWINDOWPOS", 
	L"TYPE_HOOK", 
	L"TYPE_CLIPDATA", 
	L"TYPE_CALLPROC", 
	L"TYPE_ACCELTABLE", 
	L"TYPE_DDEACCESS", 
	L"TYPE_DDECONV", 
	L"TYPE_DDEXACT", 
	L"TYPE_MONITOR", 
	L"TYPE_KBDLAYOUT", 
	L"TYPE_KBDFILE", 
	L"TYPE_WINEVENTHOOK", 
	L"TYPE_TIMER", 
	L"TYPE_INPUTCONTEXT", 
	L"TYPE_HIDDATA", 
	L"TYPE_DEVICEINFO", 
	L"TYPE_TOUCHINPUT", 
	L"TYPE_GESTUREINFO", 
	L"TYPE_CTYPES"
};
/* initialize to number of elements in w_handlenames[]: */
const unsigned w_handlenames_count = sizeof( w_handlenames ) / sizeof( w_handlenames[ 0 ] );



/* print_HANDLEENTRY_type()
Print user-readable name of a HANDLEENTRY's type. No newline.
*/
void print_HANDLEENTRY_type( 
	const BYTE bType   // in
)
{
	if( ( bType >= 0 ) && ( bType < w_handlenames_count ) )
		printf( "%ls ", w_handlenames[ bType ] );
	else
		printf( "<%u> ", (unsigned)bType );
	
	return;
}



/* print_HANDLEENTRY_flags()
Print user-readable names of a HANDLEENTRY's flags. No newline.

if 'bFlags' is 0 this function returns without having printed anything.
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

if 'entry' is NULL this function returns without having printed anything.
*/
void print_HANDLEENTRY(
	const HANDLEENTRY *const entry   // in
)
{
	const char *const objname = "HANDLEENTRY struct";
	
	
	if( !entry )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	PRINT_HEX( entry->pHead );
	PRINT_HEX( entry->pOwner );
	
	printf( "entry->bType: %u ( ", (unsigned)entry->bType );
	print_HANDLEENTRY_type( entry->bType );
	printf( ")\n" );
	
	printf( "entry->bFlags: 0x%02X", (unsigned)entry->bFlags );
	if( entry->bFlags )
	{
		printf( " ( " );
		print_HANDLEENTRY_flags( entry->bFlags );
		printf( ")" );
	}
	printf( "\n" );
	
	printf( "entry->wUniq: %u\n", (unsigned)entry->wUniq );
	
	PRINT_SEP_END( objname );
	
	return;
}



/* w_hooknames[]
An array of user-readable wide names of HOOK ids. Add 1 to an id to get its position in the index.
*/
const WCHAR *const w_hooknames[ 16 ] = 
{
	L"WH_MSGFILTER",   // id -1
	L"WH_JOURNALRECORD",   // id 0
	L"WH_JOURNALPLAYBACK",   // id 1
	L"WH_KEYBOARD",   // id 2
	L"WH_GETMESSAGE",   // id 3
	L"WH_CALLWNDPROC",   // id 4
	L"WH_CBT",   // id 5
	L"WH_SYSMSGFILTER",   // id 6
	L"WH_MOUSE",   // id 7
	L"WH_HARDWARE",   // id 8
	L"WH_DEBUG",   // id 9
	L"WH_SHELL",   // id 10
	L"WH_FOREGROUNDIDLE",   // id 11
	L"WH_CALLWNDPROCRET",   // id 12
	L"WH_KEYBOARD_LL",   // id 13
	L"WH_MOUSE_LL"   // id 14
};
/* initialize to number of elements in w_hooknames[]: */
const unsigned w_hooknames_count = sizeof( w_hooknames ) / sizeof( w_hooknames[ 0 ] );



/* print_HOOK_id()
Print user-readable name of a HOOK's id. No newline.
*/
void print_HOOK_id( 
	const INT iHook   // in
)
{
	const unsigned index = (unsigned)( iHook + 1 ); /* the array index is the same as id + 1 */

	if( index < w_hooknames_count )
		printf( "%ls ", w_hooknames[ index ] );
	else
		printf( "<%d> ", iHook );
	
	return;
}



/* print_HOOK_flags()
Print user-readable names of a HOOK's flags. No newline.

if 'flags' is NULL this function returns without having printed anything.
*/
void print_HOOK_flags( 
	const DWORD flags   // in
)
{
	if( !flags )
		return;
	
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



/* print_HOOK_anomalies()
Print any anomalies found in a HOOK struct.

if 'object' is NULL this function returns without having printed anything.
*/
void print_HOOK_anomalies(
	const HOOK *const object   // in
)
{
	if( !object )
		return;
	
	if( !( object->flags & HF_GLOBAL )
		&& ( ( object->iHook == WH_JOURNALPLAYBACK )
			|| ( object->iHook == WH_JOURNALRECORD )
			|| ( object->iHook == WH_KEYBOARD_LL )
			|| ( object->iHook == WH_MOUSE_LL )
			|| ( object->iHook == WH_SYSMSGFILTER )
		)
	)
	{
		printf( "ERROR: The HOOK @ " );
		PRINT_HEX_BARE( object->pSelf );
		printf( " is supposed to be global-only but is missing the HF_GLOBAL flag!\n" );
	}
	
	if( ( object->flags & HF_GLOBAL ) && object->ptiHooked )
	{
		printf( "ERROR: The global HOOK @ " );
		PRINT_HEX_BARE( object->pSelf );
		printf( " has a target address even though global HOOKs aren't supposed to have them.\n" );
	}
	
	return;
}



/* print_HOOK()
Print a HOOK struct.

if 'object' is NULL this function returns without having printed anything.
*/
void print_HOOK(
	const HOOK *const object   // in
)
{
	const char *const objname = "HOOK struct";
	
	
	if( !object )
		return;
	
	PRINT_SEP_BEGIN( objname );
	
	PRINT_HEX( object->head.h );
	printf( "object->head.cLockObj: %lu\n", object->head.cLockObj );
	
	PRINT_HEX( object->pti );
	PRINT_HEX( object->rpdesk1 );
	PRINT_HEX( object->pSelf );
	PRINT_HEX( object->phkNext );
	
	printf( "object->iHook: %d ( ", object->iHook );
	print_HOOK_id( object->iHook );
	printf( ")\n" );
	
	printf( "object->offPfn: 0x%08lX\n", object->offPfn );
	
	printf( "object->flags: 0x%08lX", object->flags );
	if( object->flags )
	{
		printf( " ( " );
		print_HOOK_flags( object->flags );
		printf( ")" );
	}
	printf( "\n" );
	
	printf( "object->ihmod: %d\n", object->ihmod );
	PRINT_HEX( object->ptiHooked );
	PRINT_HEX( object->rpdesk2 );
	
	PRINT_SEP_END( objname );
	
	return;
}



/* get_HOOK_name_from_id()
Get the HOOK name from its id.

'id' is the HOOK id you want the name of.

returns nonzero on success.
if success then '*name' has received a pointer to the HOOK name. free() when done.
if fail then '*name' has received NULL.
*/
int get_HOOK_name_from_id( 
	const WCHAR **const name,   // out deref
	const int id   // in
)
{
	const unsigned index = (unsigned)( id + 1 ); /* the array index is the same as id + 1 */
	
	FAIL_IF( !name );
	
	
	*name = NULL;
	
	if( index < w_hooknames_count )
		*name = must_wcsdup( w_hooknames[ index ] );
	
	return !!*name;
}



/* get_HOOK_id_from_name()
Get the HOOK id from its name.

'name' is the HOOK name you want the id of.

returns nonzero on success.
if success then '*id' has received the HOOK id.
if fail then '*id' has received INT_MAX.
*/
int get_HOOK_id_from_name( 
	int *const id,   // out
	const WCHAR *const name   // in
)
{
	unsigned index = 0;
	
	FAIL_IF( !id );
	FAIL_IF( !name );
	
	
	*id = INT_MAX;
	
	for( index = 0; index < w_hooknames_count; ++index )
	{
		if( !_wcsicmp( name, w_hooknames[ index ] ) ) // match
		{
			/* the id is the same as the index in the array - 1 */
			*id = index - 1;
			break;
		}
	}
	
	return ( *id != INT_MAX );
}
