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
These are the structs needed to read from the session handle table.
I copied these structs from the ReactOS project (with a few exceptions as noted), and then made 
minor modifications to names or types (eg changing to void* to avoid dependency).
*/

#ifndef _REACTOS_H
#define _REACTOS_H

#include <windows.h>



#ifdef __cplusplus
extern "C" {
#endif


// 8/17/2011
// http://www.reactos.org/wiki/Techwiki:Win32k/HEAD
typedef struct _HEAD
{
	HANDLE h;
	DWORD cLockObj;
} HEAD, *PHEAD;


// 8/17/2011
// http://www.reactos.org/wiki/Techwiki:Win32k/HANDLEENTRY
typedef struct _HANDLEENTRY
{
	PHEAD pHead;
	PVOID pOwner;  // PTI or PPI
	BYTE bType;  // Object handle type
	BYTE bFlags;  // Flags
	WORD wUniq;  // Access count.
} HANDLEENTRY, *PHE, *PHANDLEENTRY;


// 8/17/2011
// http://www.reactos.org/wiki/Techwiki:Win32k/HANDLEENTRY
// HANDLEENTRY.bFlags
#define HANDLEF_DESTROY        0x01
#define HANDLEF_INDESTROY      0x02
#define HANDLEF_INWAITFORDEATH 0x04
#define HANDLEF_FINALDESTROY   0x08
#define HANDLEF_MARKED_OK      0x10
#define HANDLEF_GRANTED        0x20
// mask for valid flags
#define HANDLEF_VALID   0x3F


// 8/17/2011
// http://www.reactos.org/wiki/Techwiki:Win32k/HANDLEENTRY
// HANDLEENTRY.bType
enum HANDLE_TYPE{
	TYPE_FREE = 0 ,        // 'must be zero!
	TYPE_WINDOW = 1 ,      // 'in order of use for C code lookups
	TYPE_MENU = 2,         //
	TYPE_CURSOR = 3,       //
	TYPE_SETWINDOWPOS = 4, // HDWP
	TYPE_HOOK = 5,         //
	TYPE_CLIPDATA = 6 ,    // 'clipboard data
	TYPE_CALLPROC = 7,     //
	TYPE_ACCELTABLE = 8,   //
	TYPE_DDEACCESS = 9,    //  tagSVR_INSTANCE_INFO
	TYPE_DDECONV = 10,     //  
	TYPE_DDEXACT = 11,     // 'DDE transaction tracking info.
	TYPE_MONITOR = 12,     //
	TYPE_KBDLAYOUT = 13,   // 'Keyboard Layout handle (HKL) object.
	TYPE_KBDFILE = 14,     // 'Keyboard Layout file object.
	TYPE_WINEVENTHOOK = 15,// 'WinEvent hook (EVENTHOOK)
	TYPE_TIMER = 16,       //
	TYPE_INPUTCONTEXT = 17,// 'Input Context info structure
	TYPE_HIDDATA = 18,     //
	TYPE_DEVICEINFO = 19,  //
	TYPE_TOUCHINPUT = 20,  // 'Ustz' W7U sym tagTOUCHINPUTINFO
	TYPE_GESTUREINFO = 21, // 'Usgi'
	TYPE_CTYPES = 22,      // 'Count of TYPEs; Must be LAST + 1
	TYPE_GENERIC = 255     // 'used for generic handle validation
};

// 8/18/2011
// http://reactos.org/wiki/Techwiki:Win32k/SHAREDINFO
typedef struct _WNDMSG
{
  DWORD maxMsgs;
  DWORD abMsgs;
} WNDMSG, *PWNDMSG;


// 8/17/2011
// http://www.reactos.org/wiki/Techwiki:Win32k/SHAREDINFO
typedef struct _SHAREDINFO
{
	void *psi; //PSERVERINFO
	PHANDLEENTRY aheList;
	void *pDisplayInfo; //PDISPLAYINFO
	ULONG_PTR ulSharedDelta;
	WNDMSG awmControl[31];
	WNDMSG DefWindowMsgs;
	WNDMSG DefWindowSpecMsgs;
} SHAREDINFO, *PSHAREDINFO;


// 8/17/2011
// http://www.reactos.org/wiki/Techwiki:Win32k/HOOK
// http://www.reactos.org/wiki/Techwiki:Win32k/HEAD
typedef struct _HOOK
{
	HEAD head;
	void *pti; //PTHREADINFO
	void *rpdesk1; //PDESKTOP
	void *pSelf;   // points to the kernel mode address
	struct _HOOK *phkNext;
	INT iHook;
	DWORD offPfn;
	DWORD flags;
	INT ihmod;
	void *ptiHooked; //PTHREADINFO
	void *rpdesk2; //PDESKTOP
} HOOK, *PHOOK;


// 9/18/2011
// http://forum.sysinternals.com/enumerate-windows-hooks_topic23877.html#122641
#define HF_GLOBAL   0x0001
#define HF_ANSI   0x0002
#define HF_NEEDHC_SKIP   0x0004
#define HF_HUNG   0x0008
#define HF_HOOKFAULTED   0x0010
#define HF_NOPLAYBACKDELAY   0x0020
#define HF_WX86KNOWINDOWLL   0x0040
#define HF_DESTROYED   0x0080
// mask for valid flags
#define HF_VALID   0x00FF


// 8/17/2011
// http://reactos.org/wiki/Techwiki:Win32k/DESKTOP
#define CWINHOOKS (WH_MAX - WH_MIN + 1)
typedef struct WND *PWND;
typedef struct _DESKTOPINFO
{
/* 000 */ PVOID        pvDesktopBase;
/* 004 */ PVOID        pvDesktopLimit;
/* 008 */ PWND         spwnd;
/* 00c */ DWORD        fsHooks;
/* 010 */ PHOOK        aphkStart[CWINHOOKS];
/* 050 */ PWND         spwndShell;
/* 054 */ void *ppiShellProcess; //PPROCESSINFO
/* 058 */ PWND         spwndBkGnd;
/* 05c */ PWND         spwndTaskman;
/* 060 */ PWND         spwndProgman;
/* 064 */ void *pvwplShellHook; //PVWPL
/* 068 */ INT          cntMBox;
          PWND         spwndGestureEngine;
          void *pvwplMessagePPHandler; //PVWPL

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4214) /* nonstandard extension: bitfields other than int */
#pragma warning(disable:4201) /* nonstandard extension: nameless struct/union */
#endif
          struct
          {
            ULONG fComposited:1;
            ULONG fIsDwmDesktop:1;
          };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

} DESKTOPINFO, *PDESKTOPINFO;



/** 
these functions are documented in the comment block above their definitions in reactos.c
*/
extern const WCHAR *const w_handlenames[];
extern const unsigned w_handlenames_count;

void print_HANDLEENTRY_type( 
	const BYTE bType   // in
);

void print_HANDLEENTRY_flags( 
	const BYTE bFlags   // in
);

void print_HANDLEENTRY(
	const HANDLEENTRY *const entry   // in
);

extern const WCHAR *const w_hooknames[];
extern const unsigned w_hooknames_count;

void print_HOOK_id( 
	const INT iHook   // in
);

void print_HOOK_flags( 
	const DWORD flags   // in
);

void print_HOOK_anomalies(
	const HOOK *const object   // in
);

void print_HOOK(
	const HOOK *const object   // in
);

int get_HOOK_name_from_id( 
	const WCHAR **const name,   // out deref
	const int id   // in
);

int get_HOOK_id_from_name( 
	int *const id,   // out
	const WCHAR *const name   // in
);


#ifdef __cplusplus
}
#endif

#endif // _REACTOS_H
