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

/** these are independent structs for 
SYSTEM_THREAD_INFORMATION,
SYSTEM_EXTENDED_THREAD_INFORMATION,
SYSTEM_PROCESS_INFORMATION

I call these independent structs because they do not rely on any undocumented 
types, other than each other.

Warning: traverse_threads.h depends on the above structs but does not include 
this file. You must include either this file or your own declaration of the 
above undocumented structs before including traverse_threads.h.
*/

#ifndef _NT_INDEPENDENT_SYSPROCINFO_STRUCTS_H
#define _NT_INDEPENDENT_SYSPROCINFO_STRUCTS_H

#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif


// http://msdn.microsoft.com/en-us/library/gg750724%28v=prot.10%29.aspx
typedef struct _SYSTEM_THREAD_INFORMATION {
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER CreateTime;
	ULONG WaitTime;
	PVOID StartAddress;
	struct /* changed from CLIENT_ID type to avoid dependency conflict */
	{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
	} ClientId;
	LONG Priority;
	LONG BasePriority;
	ULONG ContextSwitches;
	ULONG ThreadState;
	ULONG WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;


/*
http://wj32.wordpress.com/2009/04/25/ntquerysysteminformation-a-simple-way-to-bypass-rootkits-which-hide-processes-by-hooking/
http://processhacker.sourceforge.net/doc/struct___s_y_s_t_e_m___e_x_t_e_n_d_e_d___t_h_r_e_a_d___i_n_f_o_r_m_a_t_i_o_n.html
*/
typedef struct _SYSTEM_EXTENDED_THREAD_INFORMATION { 
	SYSTEM_THREAD_INFORMATION ThreadInfo;
	/*
	EXTENDED members: 
	*/
	PVOID 	StackBase;
	PVOID 	StackLimit;
	PVOID 	Win32StartAddress;
	PVOID TebAddress; /* This is only filled in on Vista and above */
	ULONG_PTR 	Reserved2;
	ULONG_PTR 	Reserved3;
	ULONG_PTR 	Reserved4;
} SYSTEM_EXTENDED_THREAD_INFORMATION, *PSYSTEM_EXTENDED_THREAD_INFORMATION;


//http://processhacker.sourceforge.net/doc/struct___s_y_s_t_e_m___p_r_o_c_e_s_s___i_n_f_o_r_m_a_t_i_o_n.html
typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG	 NextEntryOffset;
	ULONG	 NumberOfThreads;
	LARGE_INTEGER	 SpareLi1;
	LARGE_INTEGER	 SpareLi2;
	LARGE_INTEGER	 SpareLi3;
	LARGE_INTEGER	 CreateTime;
	LARGE_INTEGER	 UserTime;
	LARGE_INTEGER	 KernelTime;
	struct /* changed from UNICODE_STRING type to avoid dependency conflict */
	{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
	} ImageName;
	LONG	 BasePriority; /* changed from KPRIORITY type to avoid dependency conflict */
	HANDLE	 UniqueProcessId;
	HANDLE	 InheritedFromUniqueProcessId;
	ULONG	 HandleCount;
	ULONG	 SessionId;
	ULONG_PTR	 PageDirectoryBase;
	SIZE_T	 PeakVirtualSize;
	SIZE_T	 VirtualSize;
	ULONG	 PageFaultCount;
	SIZE_T	 PeakWorkingSetSize;
	SIZE_T	 WorkingSetSize;
	SIZE_T	 QuotaPeakPagedPoolUsage;
	SIZE_T	 QuotaPagedPoolUsage;
	SIZE_T	 QuotaPeakNonPagedPoolUsage;
	SIZE_T	 QuotaNonPagedPoolUsage;
	SIZE_T	 PagefileUsage;
	SIZE_T	 PeakPagefileUsage;
	SIZE_T	 PrivatePageCount;
	LARGE_INTEGER	 ReadOperationCount;
	LARGE_INTEGER	 WriteOperationCount;
	LARGE_INTEGER	 OtherOperationCount;
	LARGE_INTEGER	 ReadTransferCount;
	LARGE_INTEGER	 WriteTransferCount;
	LARGE_INTEGER	 OtherTransferCount;
	SYSTEM_THREAD_INFORMATION	 Threads [1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;



#ifdef __cplusplus
}
#endif

#endif // _NT_INDEPENDENT_SYSPROCINFO_STRUCTS_H
