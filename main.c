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
This file contains main() and various supporting functions.

-
print_version()

Print the program version.
-

-
print_license()

Print the GPL license and copyright.
-

-
gethooks()

Initialize and process the snapshot store(s), and print the HOOK info to stdout.
-

-
main()

Create and initialize the global stores, print the license and version and then run gethooks().
-

*/

#include <stdio.h>

#include "util.h"

#include "snapshot.h"

#include "diff.h"

#include "test.h"

/* the global stores */
#include "global.h"



/* this program's version */
#define VERSION_MAJOR   1
#define VERSION_MINOR   0

/* print_version()
Print the program version.

If you've made any modifications to this program add a line that says "Modifications by" as shown.
*/
void print_version( void )
{
	printf( "\ngethooks v%u.%u", VERSION_MAJOR, VERSION_MINOR );
	printf( " - " );
	printf( "Built on " __DATE__ " at " __TIME__ "\n" );
	printf( "The original gethooks source can be found at http://jay.github.com/gethooks\n" );
	printf( "For usage use --help\n" );
	
	/* Example modification notice. Leave this example intact. Copy it below to use as a template.
	printf( "\n****This version has been modified by Your Name <your@email>\n" );
	printf( "The modification made is super fast lightning if executed outside gravity.\n" );
	printf( "Source code: http://your.website/yourgethooksmods\n" );
	*/
	
	return;
}



/* print_license()
Print the GPL license and copyright.

If you've made substantial modifications to this program add an additional copyright with your name.
*/
void print_license( void )
{
	printf( "-\n" );
	/* Example copyright notice. Leave this example intact. Copy it below to use as a template.
	printf( "Copyright (C) 2011 Your Name <your@email> \n" );
	*/
	printf( 
		"Copyright (C) 2011 Jay Satiro <raysatiro@yahoo.com> \n"
		"All rights reserved. License GPLv3+: GNU GPL version 3 or later \n"
		"<http://www.gnu.org/licenses/gpl.html>. \n"
		"This is free software: you are free to change and redistribute it. \n"
		"There is NO WARRANTY, to the extent permitted by law. \n"
	);
	printf( "-\n" );
	
	return;
}



/* gethooks()
Initialize and process the snapshot store(s), and print the HOOK info to stdout.

Each snapshot store is a snapshot of the state of the system, specifically thread information and 
HOOK information at that point in time. This function calls init_snapshot_store() which takes a 
snapshot and then matches the HOOKs to their threads. This function then prints the results.

If monitoring/polling is enabled then snapshots are taken continuously with each current snapshot 
compared to the previous one for differences. The results are printed for each difference.

returns nonzero on success (a single snapshot was taken and its results printed to stdout).
if polling is enabled this function will loop continuously and never return.
*/
int gethooks()
{
	struct snapshot *previous = NULL;
	struct snapshot *current = NULL;
	struct snapshot *temp = NULL;
	int ret = 0;
	
	FAIL_IF( !G );   // The global store must exist.
	FAIL_IF( !G->prog->init_time );   // The program store must be initialized.
	FAIL_IF( !G->config->init_time );   // The configuration store must be initialized.
	FAIL_IF( !G->desktops->init_time );   // The desktop store must be initialized.
	
	
	create_snapshot_store( &current );
	create_snapshot_store( &previous );
	
	
	ret = init_snapshot_store( current );
	
	if( G->config->verbose >= 8 )
		print_snapshot_store( current );
	
	if( !ret )
	{
		MSG_FATAL( "The snapshot store failed to initialize." );
		exit( 1 );
	}
	
	print_initial_desktop_hook_list( current->desktop_hooks );
	
	if( G->config->polling < 0 )
		goto cleanup;
	
	printf( 
		"\nMonitor mode enabled. Checking for changes every %d seconds...\n", 
		G->config->polling 
	);
	fflush( stdout );
	
	for( ;; )
	{
		Sleep( G->config->polling * 1000 );
		
		/* swap pointers to previous and current snapshot stores.
		this is better than continually freeing and creating the stores.
		the current snapshot becomes the previous, and the former previous is set 
		to be reinitialized with new info and become the current.
		*/
		temp = previous;
		previous = current;
		current = temp;
		
		ret = init_snapshot_store( current );
		
		if( G->config->verbose >= 8 )
			print_snapshot_store( current );
		
		if( !ret )
		{
			MSG_FATAL( "The snapshot store failed to initialize." );
			exit( 1 );
		}
		
		print_diff_desktop_hook_lists( previous->desktop_hooks, current->desktop_hooks );
	}
	
cleanup:
	free_snapshot_store( &previous );
	free_snapshot_store( &current );
	
	return TRUE;
}



/* main()
Create and initialize the global stores, print the license and version and then run gethooks().
*/
int main( int argc, char **argv )
{
	_set_printf_count_output( 1 ); // enable support for %n.
	
	
	/* print version and license */
	print_version();
	printf( "\n" );
	
	print_license();
	printf( "\n\n" );
	
	
	/* Create the global store 'G' and its descendants or die.
	The global store holds all the stores that must be available globally.
	*/
	create_global_store();
	
	/* G and its descendants have been created */
	
	
	/* Initialize the global program store 'G->prog', a descendant of the global store.
	The global program store holds basic info like command line arguments and system version.
	'G' must be created before initializing the global program store.
	*/
	init_global_prog_store( argc, argv );
	
	/* G->prog has been initialized */
	
	
	/* Initialize the global configuration store 'G->config', a descendant of the global store.
	The global configuration store holds the user-specified command line configuration.
	'G->prog' must be initialized before initializing the global configuration store.
	*/
	init_global_config_store();
	
	/* G->config has been initialized */
	
	
	/* Initialize the global desktop store 'G->desktops', a descendant of the global store.
	The global desktop store holds a linked list of attached to desktops and their heaps.
	'G->config' must be initialized before initializing the global desktop store.
	*/
	init_global_desktop_store();
	
	/* G->desktops has been initialized */
	
	/* The global store is initialized */
	
	if( G->config->verbose >= 5 )
		print_global_store();
	
	/* If the testlist is initialized the user requested testmode to run tests.
	Else run gethooks() to take snapshots and print differences.
	*/
	return ( ( G->config->testlist->init_time ) ? !testmode() : !gethooks() );
}
