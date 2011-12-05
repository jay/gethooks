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
This file contains functions that print information about the GetHooks program and its usage.
Each function is documented in the comment block above its definition.

-
print_overview_and_exit()

Print an overview of the program and exit(1).
-

-
print_more_options_and_exit()

Print more options and exit(1).
-

-
print_more_examples_and_exit()

Print more usage examples and exit(1).
-

-
print_usage_and_exit()

Print the program's usage and exit(1).
-

*/

#include <stdio.h>

#include "util.h"

#include "nt_independent_sysprocinfo_structs.h"

#include "usage.h"

/* the global stores */
#include "global.h"



/* print_overview_and_exit()
Print an overview of the program and exit(1).
*/
void print_overview_and_exit( void )
{
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	printf( "\nOverview\n" );
	
	
	printf( "\n\n\n"
		"GetHooks is a program designed for the passive detection and monitoring of \n"
		"hooks from a limited user account.\n"
		"\n"
		"GetHooks lists any hook in the user handle table that is on any desktop in the \n"
		"current window station, and the threads/processes associated with that hook.\n"
	);
	
	
	printf( "\n\n\n"
		"A hook basically allows for intercepting certain messages before they reach \n"
		"their target. To \"hook\" is a general term. To be more specific, GetHooks \n"
		"detects all hooks handled by SetWindowsHookEx(), or in other words all hooks \n"
		"of user object TYPE_HOOK.\n"
		"\n"
		"GetHooks does not detect WinEvents, which can be hooked and are of user object \n"
		"TYPE_WINEVENTHOOK. WinEvent hooks are sometimes referred to as just hooks. \n"
		"\n"
		"GetHooks also does not detect function hooks. Functions can be hooked any \n"
		"number of ways. Function hooks are sometimes referred to as just hooks.\n"
		"\n"
		"The semantics involved might seem confusing. For a list of the hooks \n"
		"recognized by this program please review the examples.\n"
	);
	
	
	printf( "\n\n\n"
		"GetHooks is passive in that unlike other monitors it does not attempt to set \n"
		"any hooks itself, load any drivers, need administrator privileges to find \n"
		"hooks or have any type of impact on the operating system.\n"
		"\n"
		"GetHooks is not passive in that it attempts to read at least several bytes of \n"
		"each process' memory to identify the owner, origin and target thread of a \n"
		"hook. If it cannot identify a thread associated with a hook the respective \n"
		"thread is shown in the hook notice as \"<unknown>\".\n"
	);
	
	
	printf( "\n\n\n"
		"After a lot of research (special thanks to alex$at$ntinternals.org) I \n"
		"designed and developed GetHooks as free software. The program's estimated \n"
		"cost so far has exceeded $6000 as of October 13, 2011. If you find this \n"
		"program useful and it has saved you time or money then you are welcome to \n"
		"donate time or money. http://jay.github.com/gethooks/index.html#donate\n"
	);
	
	
	printf( "\n\n\n"
		"Please review the options and examples for more information.\n"
	);
	
	
	exit( 1 );
}



/* print_more_options_and_exit()
Print more options and exit(1).
*/
void print_more_options_and_exit( void )
{
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	printf( "\n"
		"These options are compatible with all other options unless stated otherwise.\n"
		"\n"
		"[-t <num>]  [-f]  [-e]  [-u]  [-g]  [-z <func> [param]]\n"
	);
	
	
	printf( "\n\n"
		"   -t     the maximum number of threads in a snapshot\n"
		"\n"
		"For each system snapshot this program allocates several buffers whose size is \n"
		"based on the maximum number of threads in a snapshot. The default maximum \n"
		"number of threads is currently %u, resulting in each snapshot taking ~%uMB.\n"
		"Currently gethooks has memory allocated at any one time for 1 snapshot by \n"
		"default, or 2 if in monitor mode, or maybe more if in test mode. Use this \n"
		"option to specify a smaller or larger number of threads per snapshot, which \n"
		"will decrease or increase, respectively, the amount of memory allocated.\n", 
		MAX_THREADS_DEFAULT, 
		(unsigned)( 
			MAX_THREADS_DEFAULT
			* ( sizeof( SYSTEM_PROCESS_INFORMATION ) 
				+ sizeof( SYSTEM_EXTENDED_THREAD_INFORMATION ) 
			)
			/ 1048576
			+ 1
		) // worst case scenario for snapshot size
	);
	
	
	printf( "\n\n"
		"   -f     force successful completion of NtQuerySystemInformation()\n"
		"\n"
		"For each system snapshot this program uses a thread traversal library which \n"
		"calls the API function NtQuerySystemInformation() to get a list of threads in \n"
		"the system. If that call ever fails it is considered an indicator of system \n"
		"instability and by default this program will terminate. However, you may \n"
		"decide that the failure is indicative of something else entirely. Use this \n"
		"option to silently retry on almost any failed NtQuerySystemInformation() call \n"
		"every second until it succeeds.\n"
		"-Note that this option is only a workaround for intermittent failures. If you \n"
		"enable this option and a failure *always* occurs then the code loops endlessly.\n"
		"-Note that info length mismatch (buffer too small) failures are never retried. \n"
		"To remedy a small buffer increase the number of threads using option 't'.\n"
	);
	
	
	printf( "\n\n"
		"   -e     show external hooks (ignore internal hooks)\n"
		"\n"
		"You'll notice for each hook found on your desktop that the owner, origin and \n"
		"target thread are often the same. That means a thread hooked itself. Example:\n"
		"-\n"
		"Owner/Origin/Target: notepad++.exe (PID 3408, TID 3412 @ 0xFE6ECDD8)\n"
		"-\n"
		"I refer to those hooks as internal hooks. External hooks are any hooks which \n"
		"are not internal, and are usually more of interest. For example a thread \n"
		"which has targeted some other thread with a hook or is targeting all threads \n"
		"with a global hook. This program processes both external and internal hooks \n"
		"by default, but you may use this option to ignore internal hooks.\n"
	);
	
	
	printf( "\n\n" 
		"   -u     show unknown hooks (ignore known hooks)\n"
		"\n"
		"You'll notice for each hook found on your desktop that the owner, origin and \n"
		"target thread are usually identified by their thread and process info. Example:\n"
		"-\n"
		"Owner/Origin: WLSync.exe (PID 148, TID 3280 @ 0xFE2A5B50)\n"
		"Target: WLSync.exe (PID 148, TID 3556 @ 0xFE5E9B78)\n"
		"-\n"
		"I refer to those hooks as known hooks. Unknown hooks are any hooks which do \n"
		"not have identifiable user or kernel mode owner, origin and/or target thread \n"
		"information, and may be more of interest to you because they could not be \n"
		"fully identified. This program processes both known and unknown hooks by \n"
		"default, but you may use this option to ignore known hooks.\n"
		"-Note that unknown hooks are often the result of insufficient permissions. To \n"
		"remedy that you can run this program with administrative privileges.\n"
	);
	
	
	printf( "\n\n" 
		"   -g     show global hooks (ignore hooks with a target thread)\n"
		"\n"
		"You may notice hooks on your desktop that have a target of <GLOBAL>. Example:\n"
		"-\n"
		"Owner/Origin: hkcmd.exe (PID 2780, TID 3456 @ 0xFF52EC00)\n"
		"Target: <GLOBAL>\n"
		"-\n"
		"Global hooks may be of more interest to you because \"the hook procedure is \n"
		"associated with all existing threads running in the same desktop as the \n"
		"calling thread.\" Popular global hooks include WH_KEYBOARD, WH_KEYBOARD_LL \n"
		"and WH_MOUSE, WH_MOUSE_LL to monitor keyboard and mouse input, respectively. \n"
		"This program processes both global and non-global (targeted) hooks by \n"
		"default, but you may use this option to ignore targeted hooks.\n"
	);
	
	
	printf( "\n\n"
		"   -c     ignore hook lock count changes\n"
		"\n"
		"Hook objects can be locked and unlocked by the kernel. The lock count and \n"
		"the frequency at which the count changes may or may not prove to be viable \n"
		"diagnostic information for you when you are monitoring a hook. Not a lot is \n"
		"known about the expected behavior of hook locking and unlocking. Further, \n"
		"the number of lock count changes you see will largely depend on the interval \n"
		"at which you are monitoring. You may use this option to ignore lock count \n"
		"change notifications.\n"
		"-Note that empirical testing on Win7 x86 SP1 shows that locking and unlocking \n"
		"is far more frequent with a faulted hook.\n"
	);
	
	
	printf( "\n\n"
		"   -y     go completely passive (do not identify owner/origin/target threads)\n"
		"\n"
		"GetHooks was designed for passive detection of hooks but by default it is not \n"
		"completely passive. The memory of each process must be read to identify which \n"
		"threads are associated with which hooks. If it cannot read the memory of a \n"
		"process which has a thread associated with a hook it refers to that hook's \n"
		"respective thread as unknown. By using this option no threads will be \n"
		"traversed and no process' memory will be read, and all hooks will have \n"
		"threads that show as \"<unknown>\".\n"
	);
	
	
	printf( "\n\n"
		"   -z     run a test mode function with an optional or required parameter.\n"
		"\n"
		"Test mode is where I am testing functions like walking a hook chain or dumping \n"
		"an individual HOOK from its kernel address.\n"
		"Whether or not this option is compatible with any other options depends on the \n"
		"function and its parameter. For usage and examples: %s -z\n",
		G->prog->pszBasename
	);
	
	
	exit( 1 );
}



/* print_more_examples_and_exit()
Print more usage examples and exit(1).
*/
void print_more_examples_and_exit( void )
{
	unsigned i = 0;
	
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	printf( "\nMore examples:\n" );
	
	
	printf( "\n\n"
		"GetHooks can filter hooks by name or id. For the sake of posterity if an id \n"
		"does not have an associated name it is still accepted, though with a warning.\n"
		"The recognized hook names are:\n"
	);
	
	printf( "\n" );
	for( i = 0; i < w_hooknames_count; ++i )
		printf( "%ls(%d)\n", w_hooknames[ i ], ( i - 1 ) ); /* the hook id is the array index - 1 */
	
	printf( "\n"
		"You may use a hook's id instead of its name when including/excluding hooks.\n"
		"For example, to monitor only WH_KEYBOARD(2) and WH_MOUSE(7) hooks:\n"
		"\n"
		"          %s -m -i 2 7\n", 
		G->prog->pszBasename 
	);
	
	
	printf( "\n\n"
		"Show WH_KEYBOARD_LL hooks that are associated with hkcmd.exe:\n"
		"\n"
		"          %s -i WH_KEYBOARD_LL -p hkcmd.exe\n", 
		G->prog->pszBasename 
	);
	
	
	printf( "\n\n"
		"Show WH_KEYBOARD_LL hooks that are associated with any process other than \n"
		"hkcmd.exe:\n"
		"\n"
		"          %s -i WH_KEYBOARD_LL -r hkcmd.exe\n", 
		G->prog->pszBasename 
	);
	
	
	printf( "\n\n"
		"Show hooks other than WH_KEYBOARD_LL that are associated with hkcmd.exe:\n"
		"\n"
		"          %s -x WH_KEYBOARD_LL -p hkcmd.exe\n", 
		G->prog->pszBasename 
	);
	
	
	printf( "\n\n"
		"Show hooks other than WH_KEYBOARD_LL that are associated with any process \n"
		"other than hkcmd.exe:\n"
		"\n"
		"          %s -x WH_KEYBOARD_LL -r hkcmd.exe\n", 
		G->prog->pszBasename 
	);
	
	
	printf( "\n\n"
		"It is possible to include or exclude a thread id (TID) or a process id (PID) \n"
		"instead of a process name. To do that just use the id instead of the name.\n"
		"For example, to monitor hooks associated with thread id 492 or process id 148:\n"
		"\n"
		"          %s -m -p 148 492\n", 
		G->prog->pszBasename 
	);
	
	
	printf( "\n\n"
		"If a colon is the first character in an argument to program include/exclude it \n"
		"is assumed a program name follows. You may prefix a program name with a colon \n"
		"so that it is not misinterpreted as an option or process id or thread id.\n"
		"For example, to list hooks associated with program name -h and program name 907\n"
		"\n"
		"          %s -p :-h :907\n",
		G->prog->pszBasename
	);
	
	
	printf( "\n\n"
		"By default this program attaches to all desktops in the current window station.\n"
		"Instead you may specify zero or more individual desktops with the 'd' option.\n"
		"If you specify 'd' without a desktop name it is assumed you want the current.\n"
		"For example, to list hooks on the current desktop and \"Sysinternals Desktop 1\":\n"
		"\n"
		"          %s -d -d \"Sysinternals Desktop 1\"\n", 
		G->prog->pszBasename 
	);
	
	
	printf( "\n\n"
		"Use the GNU 'tee' program to copy this program's output to a file.\n"
		"For example, to monitor hooks and copy output to file \"outfile\":\n"
		"\n"
		"          %s -m | tee outfile\n", 
		G->prog->pszBasename 
	);
	
	
	printf( "\n\n"
		"Verbosity can be enabled to aid advanced users.\n"
		"When verbosity is enabled the lowest level is %u and the highest level is %u.\n"
		"The higher the verbosity level the more information that is printed.\n", 
		VERBOSE_MIN, VERBOSE_MAX
	);
	
	printf( "\n"
		"Level 1 shows additional statistics.\n"
		"Level 2 is reserved for further development.\n"
		"Level 3 is reserved for further development.\n"
		"Level 4 is reserved for further development.\n"
		"Level 5 shows this program's global store (structures) and its descendants.\n"
		"Level 6 shows the HOOK structs (Microsoft's internal hook structures).\n"
		"Level 7 shows the hook structs (This program's internal hook structures).\n"
		"\n"
		"Level >=8 is too noisy/comprehensive to be used in monitoring mode:\n"
		"Level 8 shows brief information on every thread captured in a snapshot.\n"
		"Level 9 enables traverse_threads() verbosity when it traverses each thread.\n"
		"\n"
		"For example, to print Microsoft's internal HOOK struct in each HOOK notice:\n"
		"\n"
		"          %s -v 6\n", 
		G->prog->pszBasename 
	);
	
	printf( "\n\n"
		"Note that gethooks prints any errors to stdout, not stderr.\n" 
	);
	
	exit( 1 );
}



/* print_usage_and_exit()
Print the program's usage and exit(1).
*/
void print_usage_and_exit( void )
{
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	printf( "\n"
		"GetHooks lists any hook in the user handle table that is on any desktop in the \n"
		"current window station, and the threads/processes associated with that hook.\n"
		"\n"
		"[-m [sec]]  [-v [num]]  [-d [desktop]]  [[-i]|[-x] <hook>]  [[-p]|[-r] <prog>]\n"
		"\n"
		"   -m     monitor mode. check for changes every n seconds (default %u).\n"
		"   -v     verbosity. print more information (default level when enabled is %u).\n"
		"   -d     include only these desktops: a list of desktops separated by space.\n"
		"   -i     include only these hooks: a list of hooks separated by space.\n"
		"   -x     exclude only these hooks: a list of hooks separated by space.\n"
		"   -p     include only these programs: a list of programs separated by space.\n"
		"   -r     exclude only these programs: a list of programs separated by space.\n",
		POLLING_ENABLED_DEFAULT, VERBOSE_ENABLED_DEFAULT
	);
	
	printf( "\n\n" );
	printf( "Example to monitor WH_MOUSE hooks associated with workrave.exe or PID/TID 799:\n" );
	printf( " %s -m -i WH_MOUSE -p workrave.exe 799\n", G->prog->pszBasename );
	
	printf( "\n\n" );
	printf( "For an overview: %s --about\n", G->prog->pszBasename );
	printf( "For more options: %s --options\n", G->prog->pszBasename );
	printf( "For more examples: %s --examples\n", G->prog->pszBasename );
	
	exit( 1 );
}
