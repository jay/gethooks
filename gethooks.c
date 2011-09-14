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
This file contains gethooks(). 
It is documented in the comment block above its definition.
*/

#include <stdio.h>
#include <windows.h>

#include "nt_independent_sysprocinfo_structs.h"
#include "traverse_threads.h"

#include "nt_stuff.h"

#include "gethooks.h"


void init_server_info()
{
	
}

/* This needs to be done once per thread */

/*
some comment here
*/
gethooks()
{
	
	get a list of hooks//
	
}

	/** call gethooks
	*/
	for( ;; )
	{
		retcode = ;
		
		if( !make_snapshot() )
			break;
		
		compare_snapshots();
		
		if( !G->config->polling ) // only taking one snapshot
			break;
		
		Sleep( G->config->polling * 1000 );
		
		/* swap pointers to previous and current snapshot stores.
		this is better than constantly freeing and allocating memory.
		the current snapshot becomes the previous, and the former previous is set 
		to be overwritten with new info and become the current.
		*/
		G->snapshots.current ^= G->snapshots.previous, 
		G->snapshots.previous ^= G->snapshots.current, 
		G->snapshots.current ^= G->snapshots.previous;
	}
	
	
	
	return !!retcode;
}

