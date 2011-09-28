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

#ifndef _PROG_H
#define _PROG_H

#include <windows.h>

/* ReactOS structures and supporting functions */
#include "reactos.h"



#ifdef __cplusplus
extern "C" {
#endif


/** The program store.
The program store holds basic program and system info.
*/
struct prog
{
	/* argc from main() */
	int argc;
	
	/* argv from main() */
	const char *const *argv;
	
	/* program basename (from argv[0] with folder path removed) */
	const char *pszBasename;
	
	/* the main thread's id */
	DWORD dwMainThreadId;
	
	/* the operating system version info */
	DWORD dwOSVersion, dwOSMajorVersion, dwOSMinorVersion, dwOSBuild;
	
	/* the name of the window station */
	WCHAR *pwszWinstaName;   // calloc(), free()
	
	/* A pointer to Microsoft's SHAREDINFO struct gSharedInfo */
	const SHAREDINFO *pSharedInfo;
	
	/* gSharedInfo.aheList[] count */
	const volatile ULONG *pcHandleEntries;
	
	/* the system utc time in FILETIME format immediately after this store has been initialized.
	this is nonzero when this store has been initialized.
	*/
	__int64 init_time;
};



/** 
these functions are documented in the comment block above their definitions in prog.c
*/
void create_prog_store( 
	struct prog **const out   // out deref
);

SHAREDINFO *get_SharedInfo( void );

void init_global_prog_store( 
	int argc,   // in
	char **argv   // in deref
);

void print_SharedInfo( 
	const SHAREDINFO *const pSharedInfo   // in
);

void print_global_prog_store( void );

void free_prog_store( 
	struct prog **const in   // in deref
);


#ifdef __cplusplus
}
#endif

#endif // _PROG_H
