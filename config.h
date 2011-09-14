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
	set to a negative number if not taking more than one snapshot.
	*/
	int polling;
	
	/* a linked list of desktop names to include */
	struct list *desklist;   // create_list_store(), free_list_store()
	
	/* a linked list of hook names/ids to include/exclude */
	struct list *hooklist;   // create_list_store(), free_list_store()
	
	/* a linked list of program names/ids to include/exclude */
	struct list *proglist;   // create_list_store(), free_list_store()
	
	/* nonzero when this store has been initialized */
	unsigned initialized;
};



/** 
these functions are documented in the comment block above their definitions in config.c
*/
void create_config_store( 
	struct config **const out   // out deref
);

void print_usage_and_exit( void );

void print_more_examples_and_exit( void );

unsigned get_next_arg( 
	int *const index,   // in, out
	const unsigned expected_types   // in
);

void init_global_config_store( void );

static void print_config_store( 
	struct config *store   // in
);

void print_global_config_store( void );

void free_config_store( 
	struct config **const in   // in deref
);


#ifdef __cplusplus
}
#endif

#endif /* _CONFIG_H */
