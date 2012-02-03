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

#ifndef _DESKTOP_H
#define _DESKTOP_H

#include <windows.h>

#include "reactos.h"



#ifdef __cplusplus
extern "C" {
#endif


/** This is the info needed for each item in the desktop list.
Each item has desktop heap information and the thread attached to that desktop.
All members are valid regardless of desktop list type.
*/
struct desktop_item
{
	/* The desktop's name */
	WCHAR *pwszDesktopName;   // _wcsdup(), free()
	
	/* A handle to the desktop. For the main thread this is NULL. */
	HANDLE hDesktop;   // OpenDesktopW(), CloseDesktop()
	
	/* A handle to the thread attached to the desktop. For the main thread this is NULL. */
	HANDLE hThread;   // CreateThread(), CloseHandle()
	
	/* An event that when signaled using SetEvent() will cause the above thread to terminate.
	This event is created and destroyed by the above thread.
	This event should only be signaled when the desktop item is being freed.
	For the main thread this is NULL.
	*/
	HANDLE hEventTerminate;   // CreateEvent(), CloseHandle()
	
	/* The thread's id */
	DWORD dwThreadId;
	
	/* A pointer to the thread's TEB. */
	const void *pvTeb;
	
	/* &TEB.Win32ClientInfo   (ULONG Win32ClientInfo[62])
	A pointer to the thread's CLIENTINFO.
	*/
	const void *pvWin32ClientInfo;
	
	/* CLIENTINFO.ulClientDelta   (ULONG ulClientDelta)
	The difference between the desktop's heap kernel addresses and 
	where the heap is mapped for this program to access it. 
	*/
	const void *pvClientDelta;
	
	/* CLIENTINFO.pDeskInfo   (PDESKTOPINFO pDeskInfo)
	A pointer to the thread's DESKTOPINFO.
	*/
	//const void *pDeskInfo;
	const DESKTOPINFO *pDeskInfo;
	
	/* The next item in the list */
	struct desktop_item *next;
};



/** The desktop store type.
The different types of lists that can be held by the store.
*/
enum desktop_type 
{
	DESKTOP_INVALID_TYPE,   // 0 is an invalid type
	DESKTOP_CURRENT,   // user specified 'd' option but did not specify names.
	DESKTOP_SPECIFIED,   // user specified 'd' option and specified desktop names.
	DESKTOP_ALL   // all accessible desktops. user didn't specify the 'd' option.
};



/** The desktop store. 
The desktop store holds a linked list of attached to desktops and their associated heaps.
*/
struct desktop_list
{
	/* linked list of desktop_item. this is a pointer to the first item in the desktop list. */
	struct desktop_item *head;   // items calloc'd. the list is freed when the store is freed.
	
	/* the last item in the desktop list */
	struct desktop_item *tail;
	
	/* the desktop list type */
	enum desktop_type type;
	
	/* the system utc time in FILETIME format immediately after this store has been initialized.
	this is nonzero when this store has been initialized.
	*/
	__int64 init_time;
};



/** 
these functions are documented in the comment block above their definitions in desktop.c
*/
void create_desktop_store( 
	struct desktop_list **const out   // out deref
);

void init_global_desktop_store( void );

void print_desktop_item( 
	const struct desktop_item *const item   // in
);

void print_global_desktop_store( void );

void free_desktop_store( 
	struct desktop_list **const in   // in deref
);


#ifdef __cplusplus
}
#endif

#endif /* _DESKTOP_H */
