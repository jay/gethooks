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

#ifndef _TRAVERSE_THREADS_H
#define _TRAVERSE_THREADS_H

#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif



#define TRAVERSE_CALLBACK_ABORT   (-1)
#define TRAVERSE_CALLBACK_CONTINUE   (0)
#define TRAVERSE_CALLBACK_SKIP   (1)


#define TRAVERSE_FLAG_IGNORE_CALCULATION_ERRORS   (1u)
#define TRAVERSE_FLAG_DEBUG   (1u << 1)
#define TRAVERSE_FLAG_EXTENDED   (1u << 2)
#define TRAVERSE_FLAG_ZERO_THREADS_OK   (1u << 3)
#define TRAVERSE_FLAG_RECYCLE   (1u << 4)
#define TRAVERSE_FLAG_TEST_MEMORY   (1u << 5)


#define TRAVERSE_SUCCESS   (0)
/* all errors must be negative as outlined in documentation */
#define TRAVERSE_ERROR_GENERAL   (-1)
#define TRAVERSE_ERROR_MEMORY   (-2)
#define TRAVERSE_ERROR_ALIGNMENT   (-3)
#define TRAVERSE_ERROR_BUFFER_TOO_SMALL   (-4)
#define TRAVERSE_ERROR_QUERY   (-5)
#define TRAVERSE_ERROR_CALLBACK   (-6)
#define TRAVERSE_ERROR_CALCULATION   (-7)
#define TRAVERSE_ERROR_PARAMETER   (-8)
#define TRAVERSE_ERROR_ACCESS_VIOLATION   (-9)


/** traverse_threads()
This function is well documented in traverse_threads.txt 
*/
int traverse_threads( 
	int ( __cdecl *callback )( 
		void *cb_param,   // in, out, optional
		SYSTEM_PROCESS_INFORMATION *const spi,   // in
		SYSTEM_THREAD_INFORMATION *const sti,   // in, optional
		const ULONG remaining,   // in
		const DWORD flags   // in, optional
	),   // in, optional
	void *cb_param,   // in, out, optional
	void *buffer,   // in, out, optional
	size_t buffer_bcount,   // in, optional
	const DWORD flags,   // in, optional
	LONG *status   // out, optional
);



/** 
these supporting functions are documented in the comment block above their 
definitions in traverse_threads__support.c
*/

void *get_teb( 
	const DWORD tid,   // in
	const DWORD flags   // in, optional
);

// The size in bytes of Win7 x86 TEB struct
#define SIZEOF_WIN7_TEB   4068

void *copy_teb( 
	const DWORD pid,   // in
	const DWORD tid,   // in
	const DWORD flags   // in, optional
);

int callback_print_thread_state( 
	void *cb_param,   // in, out, optional
	SYSTEM_PROCESS_INFORMATION *const spi,   // in
	SYSTEM_THREAD_INFORMATION *const sti,   // in, optional
	const ULONG remaining,   // in
	const DWORD flags   // in, optional
);

int print_filetime_as_local( 
	const FILETIME *const ft   // in
);

char *traverse_threads_retcode_to_cstr( 
	const int retcode   // in
);

char *ThreadState_to_cstr( 
	const ULONG ThreadState   // in
);

char *WaitReason_to_cstr( 
	const ULONG WaitReason   // in
);

#ifdef _MSC_VER
#define TRAVERSE_SUPPORT_TEST_MEMORY
#else
#undef TRAVERSE_SUPPORT_TEST_MEMORY
#endif
/* this is the function used to test memory for access violations.
this test is enabled only for Microsoft compilers
*/
#ifdef TRAVERSE_SUPPORT_TEST_MEMORY
#define TEST_FLAG_READ   (1u) /* test reading from memory */
#define TEST_FLAG_WRITE   (1u << 1) /* test writing to memory */
unsigned test_memory( 
	void *const address,   // in, out
	const size_t address_bcount,   // in
	const DWORD flags   // in
);
#endif

/* for internal use. all magic numbers are exactly 8 characters */
#define TRAVERSE_MAGIC_LEN   8
#define TRAVERSE_MAGIC_BEGIN   "\x4f\xd7\xef\xc5\xf0\xe6\x50\x96"
#define TRAVERSE_MAGIC_END   "\x96\x50\xe6\xf0\xc5\xef\xd7\x4f"
#define TRAVERSE_MAGIC_BAD   "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"



#ifdef __cplusplus
}
#endif

#endif /* _TRAVERSE_THREADS_H */
