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

#ifndef _CONFIG_H
#define _CONFIG_H

#include <windows.h>

/* generic list store (linked list of names/ids) */
#include "list.h"



#ifdef __cplusplus
extern "C" {
#endif


/** The configuration store.
The configuration store holds the user-specified configuration derived from the command line.
*/
struct config 
{
	/* how many seconds to wait between taking snapshots for comparison.
	by default polling is disabled and this program will not take more than one snapshot.
	*/
	#define POLLING_MIN   0
	#define POLLING_MAX   1036800 // the number of seconds in twelve days
	#define POLLING_ENABLED_DEFAULT   7
	#define POLLING_DEFAULT   ( POLLING_MIN - 1 )
	int polling;
	
	
	/* verbosity level. the higher the level the more information.
	by default verbose is disabled and this program will not print extra information.
	*/
	#define VERBOSE_MIN   1
	#define VERBOSE_MAX   9
	#define VERBOSE_ENABLED_DEFAULT   VERBOSE_MIN
	#define VERBOSE_DEFAULT   ( VERBOSE_MIN - 1 )
	int verbose;
	
	
	/* max_threads is the maximum number of threads that can be held in each snapshot.
	having 20k elements each gui/spi is currently about 7MB total per snapshot which is reasonable.
	*/
	#define MAX_THREADS_DEFAULT   20000
	unsigned max_threads;
	
	
	/* ignore internal hooks.
	any hook with the same user and kernel mode owner, origin and target thread information I refer 
	to as an internal hook, ie a thread hooked itself. any hook with any differing kernel or user 
	mode owner, origin and/or target info I refer to as an external hook.
	ignore_internal_hooks is set to 0 if disabled (default) and nonzero if enabled.
	*/
	unsigned ignore_internal_hooks;
	
	/* ignore known hooks: hooks with valid user mode owner, origin and target threads.
	a hook that is not known has an unknown owner, origin and/or target user mode thread.
	ignore_known_hooks is set to 0 if disabled (default) and nonzero if enabled.
	*/
	unsigned ignore_known_hooks;
	
	/* ignore targeted hooks: hooks that have a target thread. 
	hooks that don't have a valid user or kernel target thread are almost always global hooks (have 
	the flag HF_GLOBAL) or if not then the internal HOOK struct is invalid.
	ignore_targeted_hooks is set to 0 if disabled (default) and nonzero if enabled.
	*/
	unsigned ignore_targeted_hooks;
	
	/* ignore some NtQuerySystemInformation() errors. nonzero if true. default false.
	traverse_threads() depends on NtQuerySystemInformation() which may fail for numerous reasons.
	ignore_failed_queries is set to 0 if disabled (default) and nonzero if enabled.
	*/
	unsigned ignore_failed_queries;
	
	
	/* a linked list of desktop names to include */
	struct list *desklist;   // create_list_store(), free_list_store()
	
	/* a linked list of hook names/ids to include/exclude */
	struct list *hooklist;   // create_list_store(), free_list_store()
	
	/* a linked list of program names/ids to include/exclude */
	struct list *proglist;   // create_list_store(), free_list_store()
	
	/* a linked list of test parameters for test mode */
	struct list *testlist;
	
	
	/* the system utc time in FILETIME format immediately after this store has been initialized.
	this is nonzero when this store has been initialized.
	*/
	__int64 init_time;
};



/** 
these functions are documented in the comment block above their definitions in config.c
*/
void create_config_store( 
	struct config **const out   // out deref
);

void print_usage_and_exit( void );

void print_advanced_usage_and_exit( void );

void print_more_examples_and_exit( void );

unsigned get_next_arg( 
	int *const index,   // in, out
	const unsigned expected_types   // in
);

void init_global_config_store( void );

void print_global_config_store( void );

void free_config_store( 
	struct config **const in   // in deref
);


#ifdef __cplusplus
}
#endif

#endif /* _CONFIG_H */
