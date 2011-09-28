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

#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H

#include <windows.h>

/* SYSTEM_THREAD_INFORMATION,
SYSTEM_EXTENDED_THREAD_INFORMATION,
SYSTEM_PROCESS_INFORMATION
*/
#include "nt_independent_sysprocinfo_structs.h"

/* desktop hook store (linked list of desktop and hook information) */
#include "desktop_hook.h"



#ifdef __cplusplus
extern "C" {
#endif


/** Forward declaration for desktop hook store due to circular dependency.
*/
struct desktop_hook_list;



/** This is the info to keep track of when a GUI thread is found in the system.
For each thread traversed if its TEB.Win32ThreadInfo != NULL then the thread is a GUI thread.
*/
struct gui 
{
	/* The address of the thread's THREADINFO (kernel). 
	The address is taken from TEB's Win32ThreadInfo.
	THREADINFO is unreachable from user mode, as far as I can tell.
	*/
	const void *pvWin32ThreadInfo;
	
	/* The address of the thread's TEB. The address is taken from either 
	THREAD_BASIC_INFORMATION's TebBaseAddress (Win2k+), or
	SYSTEM_EXTENDED_THREAD_INFORMATION's TebAddress (Vista+, and only if EXTENDED flag)
	*/
	const void *pvTeb;
	
	/* The thread's process' process info.
	The memory pointed to is in the spi buffer in the parent snapshot store.
	
	If the 'flag_extended' member in the spi_array is nonzero then 
	the 'Threads' array in SYSTEM_PROCESS_INFORMATION is an array of 
	SYSTEM_EXTENDED_THREAD_INFORMATION, not SYSTEM_THREAD_INFORMATION.
	*/
	SYSTEM_PROCESS_INFORMATION *spi;
	
	/* The thread's thread info.
	The memory pointed to is in the 'spi_array.a' buffer associated with the global store.
	
	If the 'flag_extended' member in the spi_array is nonzero then 
	then this points to SYSTEM_EXTENDED_THREAD_INFORMATION, 
	not SYSTEM_THREAD_INFORMATION.
	
	Because SYSTEM_THREAD_INFORMATION is the first member in 
	SYSTEM_EXTENDED_THREAD_INFORMATION, it's OK in either case to access 
	members using a pointer to SYSTEM_THREAD_INFORMATION.
	*/
	SYSTEM_THREAD_INFORMATION *sti;
};



/** The snapshot store. 
The snapshot store holds system process info (spi), gui thread info (gui) and desktop hook info 
(desktop_hooks).
The info is recorded consecutively at a point in time, and is effectively a snapshot of the system.
*/
struct snapshot 
{
	/** an array of SYSTEM_PROCESS_INFORMATION structs.
	this array must be initialized first.
	*/
	/* a buffer holding an array of SYSTEM_PROCESS_INFORMATION (spi) structs.
	the last member of SYSTEM_PROCESS_INFORMATION is a flexible array of either 
	SYSTEM_THREAD_INFORMATION or SYSTEM_EXTENDED_THREAD_INFORMATION structs.
	
	the spi structs cannot be accessed by subscripting. each spi's offset varies.
	spi array access is handled internally by traverse_threads()
	
	this is basically a pointer to the buffer that traverse_threads() writes and reads
	*/
	SYSTEM_PROCESS_INFORMATION *spi;   // calloc(), free()
	
	/* the allocated size of the buffer in bytes */
	size_t spi_max_bytes;
	
	/* this member is nonzero if the caller called traverse_threads() to output to 'spi' 
	using the flag TRAVERSE_FLAG_EXTENDED. That means the system's process info was 
	queried using SystemExtendedProcessInformation instead of SystemProcessInformation, 
	and each spi struct in the array has a flexible array of 
	SYSTEM_EXTENDED_THREAD_INFORMATION instead of SYSTEM_THREAD_INFORMATION.
	*/
	unsigned spi_extended;
	
	
	
	/** an array of gui structs.
	the spi array must be initialized before this array is initialized.
	*/
	/* the gui array */
	struct gui *gui;   // calloc(), free()
	
	/* the allocated/maximum number of elements in the gui array.
	this is also the maximum number of threads that can be handled in this snapshot.
	*/
	unsigned gui_max;
	
	/* how many GUI threads were found in the system.
	this is also the number of elements written to in the gui array.
	*/
	unsigned gui_count;
	
	
	
	/* desktop hook store. a linked list of desktops and their hooks */
	struct desktop_hook_list *desktop_hooks;
	
	
	
	/* the system utc time in FILETIME format immediately after spi has been initialized.
	this is nonzero when the spi array has been initialized.
	*/
	__int64 init_time_spi;
	
	/* the system utc time in FILETIME format immediately after gui has been initialized.
	this is nonzero when the gui array has been initialized.
	*/
	__int64 init_time_gui;
	
	/* the system utc time in FILETIME format immediately after this store has been initialized.
	this is nonzero when this store has been initialized.
	*/
	__int64 init_time;
};



/** 
these functions are documented in the comment block above their definitions in snapshot.c
*/
void create_snapshot_store( 
	struct snapshot **const out   // out deref
);

struct gui *find_Win32ThreadInfo( 
	const struct snapshot *const store,   // in
	const void *const pvWin32ThreadInfo   // in
);

int init_snapshot_store( 
	struct snapshot *const store   // in
);

void print_gui_brief( 
	const struct gui *const gui   // in
);

void print_gui(
	const struct gui *const gui   // in
);

void print_gui_array(
	const struct snapshot *const store   // in
);

void print_spi_array_brief(
	const struct snapshot *const store   // in
);

void print_snapshot_store( 
	const struct snapshot *const store   // in
);

void free_snapshot_store( 
	struct snapshot **const in   // in deref
);


#ifdef __cplusplus
}
#endif

#endif // _SNAPSHOT_H
