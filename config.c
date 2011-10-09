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
This file contains functions for a configuration store (user-specified command line configuration).
Each function is documented in the comment block above its definition.

For now there is only one configuration store implemented and it's a global store (G->config).
'G->config' depends on the global program (G->prog) store.

-
create_config_store()

Create a configuration store and its descendants or die.
-

-
print_usage_and_exit()

Print the program's usage and exit(1).
-

-
print_advanced_usage_and_exit()

Print the program's advanced usage and exit(1).
-

-
print_more_examples_and_exit()

Print more usage examples and exit(1).
-

-
get_next_arg()

Get the next argument in the array of command line arguments.
-

-
init_global_config_store()

Initialize the global configuration store by parsing command line arguments.
Calls get_next_arg().
-

-
print_config_store()

Print a configuration store and all its descendants.
-

-
print_global_config_store()

Print the global configuration store and all its descendants.
-

-
free_config_store()

Free a configuration store and all its descendants.
-

*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#include "str_to_int.h"

#include "util.h"

#include "test.h"

#include "config.h"

/* the global stores */
#include "global.h"



static void print_config_store( 
	struct config *store   // in
);



/* the default number of seconds when polling is enabled */
#define POLLING_DEFAULT   7

/* the default verbosity level when verbosity is enabled */
#define VERBOSE_DEFAULT   1



/* create_config_store()
Create a configuration store and its descendants or die.

The configuration store holds the user-specified configuration derived from the command line.
*/
void create_config_store( 
	struct config **const out   // out deref
)
{
	struct config *config = NULL;
	
	FAIL_IF( !out );
	FAIL_IF( *out );
	
	
	/* allocate a config store */
	config = must_calloc( 1, sizeof( *config ) );
	
	/* allocate the list store for the linked list of desktops to filter */
	create_list_store( &config->desklist );
	
	/* allocate the list store for the linked list of hooks to filter */
	create_list_store( &config->hooklist );
	
	/* allocate the list store for the linked list of programs to filter */
	create_list_store( &config->proglist );
	
	/* allocate the list store for the linked list of test parameters */
	create_list_store( &config->testlist );
	
	
	*out = config;
	return;
}



/* print_usage_and_exit()
Print the program's usage and exit(1).
*/
void print_usage_and_exit( void )
{
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	printf( 
		"gethooks lists any hook in the user handle table that is on any desktop in the \n"
		"current window station, and the threads/processes associated with that hook.\n"
		"gethooks prints any errors to stdout, not stderr.\n"
		"\n"
		"[-m [sec]]  [-v [num]]  [-d [desktop]]  [[-i]|[-x] <hook>]  [[-p]|[-r] <prog>]\n"
		"\n"
		"   -m     monitor mode. check for changes every n seconds (default %u).\n"
		"   -v     verbosity. print more information (default level when enabled is %u).\n"
		"   -d     include only these desktops: a list of desktops separated by space.\n"
		"   -i     include only these hooks: a list of hooks separated by space.\n"
		"   -x     exclude only these hooks: a list of hooks separated by space.\n"
		"   -p     include only these programs: a list of programs separated by space.\n"
		"   -r     exclude only these programs: a list of programs separated by space.\n"
		"\n",
		POLLING_DEFAULT, VERBOSE_DEFAULT
	);
	
	printf( "\nexample to list WH_MOUSE hooks on the current desktop only:\n" );
	printf( " %s -d -i WH_MOUSE\n", G->prog->pszBasename );
	
	printf( "\nexample to monitor WH_KEYBOARD hooks associated with workrave.exe or pid 799:\n" );
	printf( " %s -m -i WH_KEYBOARD -p workrave.exe 799\n", G->prog->pszBasename );
	
	printf( "\nTo show more examples: %s --examples\n", G->prog->pszBasename );
	exit( 1 );
}



/* print_advanced_usage_and_exit()
Print the program's advanced usage and exit(1).
*/
void print_advanced_usage_and_exit( void )
{
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	printf( "Advanced options for gethooks:\n"
		"[-o] [-t <func> [param]]\n"
	);
	
	printf( "\n\n"
		"   -o     process \"outside\" hooks only. ignore \"inside\" hooks.\n"
		"\n"
		"You'll notice for each hook found on your desktop that the owner, origin and \n"
		"target thread are often the same. That means a thread hooked itself. I refer \n"
		"to those hooks as inside hooks. Outside hooks are any hooks which are not \n"
		"inside, and are usually more of interest. For example a thread which has \n"
		"targeted some other thread with a hook or is targeting all threads with a \n"
		"global hook. To make outside hooks easier to identify you may choose to \n"
		"ignore inside hooks by enabling this option.\n"
		"This option is compatible with all standard usage options.\n"
	);
	
	printf( "\n\n"
		"   -t     run a test mode function with an optional or required parameter.\n"
		"\n"
		"Test mode is where I am testing functions like walking a hook chain or dumping \n"
		"an individual HOOK from its kernel address.\n"
		"Whether or not this option is compatible with any other options depends on the \n"
		"function and its parameter. For usage and examples: %s -t\n",
		G->prog->pszBasename
	);
	
	exit( 1 );
}



/* print_more_examples_and_exit()
Print more usage examples and exit(1).
*/
void print_more_examples_and_exit( void )
{
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	printf( "\nMore examples:\n" );
	
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
		"You may use a hook's id instead of its name when including/excluding hooks.\n"
		"For example, to monitor only WH_KEYBOARD(2) and WH_MOUSE(7) hooks:\n"
		"\n"
		"          %s -m -i 2 7\n", 
		G->prog->pszBasename 
	);
	
	printf( "\n\n"
		"If the colon is the first character in an argument to program include/exclude \n"
		"it is assumed a program name follows. You may prefix your program name with a \n"
		"colon so that it is not misinterpreted as an option or program id (PID).\n"
		"For example, to list hooks associated with program name -h and program name 907\n"
		"\n"
		"          %s -p :-h :907\n",
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
		"When verbosity is enabled the lowest level is 1 and the highest level is 9.\n"
		"The higher the verbosity level the more information that is printed.\n"
		"\n"
		"Level 1 shows additional statistics.\n"
		"Level 2 is reserved for further development.\n"
		"Level 3 is reserved for further development.\n"
		"Level 4 is reserved for further development.\n"
		"Level 5 shows this program's global store (structure) and its descendants.\n"
		"Level 6 shows the HOOK structs (Microsoft's internal hook structures).\n"
		"Level 7 shows the hook structs (This program's internal hook structures).\n"
		"\n"
		"Level >=8 is too noisy/comprehensive to be used in monitoring mode:\n"
		"Level 8 shows brief information on all the threads captured in the snapshot.\n"
		"Level 9 enables traverse_threads() verbosity when it traverses the each thread.\n"
		"\n"
		"For example, to print the HOOK struct in each HOOK notice:\n"
		"\n"
		"          %s -v 6\n", 
		G->prog->pszBasename 
	);
	
	printf( "\n\nTo show advanced options: %s --advanced\n", G->prog->pszBasename );
	exit( 1 );
}



/* get_next_arg()
Get the next argument in the array of command line arguments.

'index' is a pointer to the current index in the array.
this function advances the index. regardless of this function's return the 
memory pointed to by index has received the updated index.

'expected_types' is the expected program argument type,
OPT (expecting an option)
OPTARG (expecting an argument for an option)
OPT | OPTARG (expecting either of the above)

returns one of the following:
OPT (found an option. only returned if OPT passed in, otherwise exit(1))
OPTARG (found an option's argument. only returned if OPTARG passed in, otherwise exit(1))
END (no more program arguments. only returned if OPT passed in, otherwise exit(1))
*/
#define OPT   (1u)
#define OPTARG   (1u << 1)
#define END   (1u << 2)
unsigned get_next_arg( 
	int *const index,   // in, out
	const unsigned expected_types   // in
)
{
	FAIL_IF( !index );
	FAIL_IF( *index < 0 );
	FAIL_IF( expected_types & ~( OPT | OPTARG ) );
	
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	for( ;; )
	{
		__int64 num = 0;
		
		/* increment index to the next argument */
		++*index;
		
		if( *index >= G->prog->argc ) /* out of command line arguments */
		{
			if( !( expected_types & OPT ) )
			{
				MSG_FATAL( "An option has no associated option argument." );
				printf( "OPT: %s\n", G->prog->argv[ *index - 1 ] );
				exit( 1 );
			}
			
			return END;
		}
		else if( str_to_int64( &num, G->prog->argv[ *index ] ) && ( num < 0 ) )
		{
			/* the command line argument is a negative number, not an option.
			a negative number would only be an option's argument
			*/
			
			if( !( expected_types & OPTARG ) )
			{
				MSG_FATAL( "An option argument has no associated option." );
				printf( "OPTARG: %s\n", G->prog->argv[ *index ] );
				exit( 1 );
			}
			
			return OPTARG;
		}
		else if( ( ( G->prog->argv[ *index ][ 0 ] == '-' ) 
				|| ( G->prog->argv[ *index ][ 0 ] == '/' ) 
			) 
			&& G->prog->argv[ *index ][ 1 ] 
			&& !G->prog->argv[ *index ][ 2 ] 
		)
		{
			/* the command line argument is an option */
			
			if( !( expected_types & OPT ) )
			{
				MSG_FATAL( "An option has no associated option argument." );
				printf( "OPT: %s\n", G->prog->argv[ *index - 1 ] );
				exit( 1 );
			}
			
			return OPT;
		}
		else if( G->prog->argv[ *index ][ 0 ] )
		{
			if( !strcmp( G->prog->argv[ *index ], "--help" ) )
				print_usage_and_exit();
			else if( !strcmp( G->prog->argv[ *index ], "--advanced" ) )
				print_advanced_usage_and_exit();
			else if( !strcmp( G->prog->argv[ *index ], "--examples" ) )
				print_more_examples_and_exit();
			else if( !strcmp( G->prog->argv[ *index ], "--version" ) ) /* version always printed */
				exit( 1 );
			
			/* the command line argument is an option's argument (optarg) */
			
			if( !( expected_types & OPTARG ) )
			{
				MSG_FATAL( "An option argument has no associated option." );
				printf( "OPTARG: %s\n", G->prog->argv[ *index ] );
				exit( 1 );
			}
			
			return OPTARG;
		}
		
		/* else this command line argument is empty. loop to the next one */
	}
}



/* init_global_config_store()
Initialize the global configuration store by parsing command line arguments.
Calls get_next_arg().

This function must only be called from the main thread.
For now there is only one configuration store implemented and it's a global store (G->config).
'G->config' depends on the global program (G->prog) store.
*/
void init_global_config_store( void )
{
	int i = 0;
	unsigned arf = 0;
	
	FAIL_IF( !G );   // The global store must exist.
	
	FAIL_IF( G->config->init_time );   // Fail if this store has already been initialized.
	
	FAIL_IF( !G->prog->init_time );   // The program store must be initialized.
	
	FAIL_IF( GetCurrentThreadId() != G->prog->dwMainThreadId );   // main thread only
	
	
	/* set polling to a negative number if not taking more than one snapshot */
	G->config->polling = -1; // default. not polling.
	
	/* parse command line arguments */
	i = 0;
	while( arf != END )
	{
		if( arf != OPT ) /* attempt to get an option from the command line arguments */
			arf = get_next_arg( &i, OPT );
		
		if( arf != OPT )
			continue;
		
		/* option found */
		
		switch( G->prog->argv[ i ][ 1 ] )
		{
			case '?':
			case 'h':
			case 'H':
			{
				/* show help */
				print_usage_and_exit();
			}
			
			
			/**
			desktop include option
			*/
			case 'd':
			case 'D':
			{
				G->config->desklist->type = LIST_INCLUDE_DESK;
				
				/* since this option may or may not have an associated argument, 
				test for both an option's argument(optarg) or the next option
				*/
				arf = get_next_arg( &i, OPT | OPTARG );
				
				if( arf != OPTARG )
					continue;
				
				while( arf == OPTARG ) /* option argument found */
				{
					WCHAR *name = NULL;
					
					
					/* make the desktop name as a wide character string */
					if( !get_wstr_from_mbstr( &name, G->prog->argv[ i ] ) )
					{
						MSG_FATAL( "get_wstr_from_mbstr() failed." );
						printf( "desktop: %s\n", G->prog->argv[ i ] );
						exit( 1 );
					}
					
					/* append to the linked list */
					if( !add_list_item( G->config->desklist, 0, name ) )
					{
						MSG_FATAL( "add_list_item() failed." );
						printf( "desktop: %s\n", G->prog->argv[ i ] );
						exit( 1 );
					}
					
					/* if add_list_item() was successful then it made a duplicate of the 
					wide string pointed to by name. in any case name should now be freed.
					*/
					free( name );
					name = NULL;
					
					/* get the option's next argument, which is optional */
					arf = get_next_arg( &i, OPT | OPTARG );
				}
				
				continue;
			}
			
			
			
			/** 
			monitor option
			*/
			case 'm':
			case 'M':
			{
				if( G->config->polling >= 0 )
				{
					MSG_FATAL( "Option 'm': this option has already been specified." );
					printf( "sec: %d\n", G->config->polling );
					exit( 1 );
				}
				
				G->config->polling = POLLING_DEFAULT;
				
				/* since this option may or may not have an associated argument, 
				test for both an option's argument(optarg) or the next option
				*/
				arf = get_next_arg( &i, OPT | OPTARG );
				
				if( arf != OPTARG )
					continue;
				
				/* option argument found */
				
				/* if the string is not an integer representation, or it is but it's negative */
				if( !str_to_int( &G->config->polling, G->prog->argv[ i ] ) 
					|| ( G->config->polling < 0 ) 
				) 
				{
					MSG_FATAL( "Option 'm': number of seconds invalid." );
					printf( "sec: %s\n", G->prog->argv[ i ] );
					exit( 1 );
				}
				
				if( !G->config->polling )
				{
					MSG_WARNING( "Option 'm': an interval of 0 uses too much CPU time." );
					printf( "sec: %d\n", G->config->polling );
				}
				else if( G->config->polling > 86400 )
				{
					MSG_WARNING( "Option 'm': more seconds than in a day." );
					printf( "sec: %d\n", G->config->polling );
				}
				else if( G->config->polling > 1036800 )
				{
					MSG_FATAL( "Option 'm': more seconds than in twelve days." );
					printf( "sec: %d\n", G->config->polling );
					exit( 1 );
				}
				
				continue;
			}
			
			
			
			/** 
			option to process only "outside" hooks
			*/
			case 'o':
			case 'O':
			{
				if( G->config->ignore_inside )
				{
					MSG_FATAL( "Option 'o': this option has already been specified." );
					exit( 1 );
				}
				
				/* ignore "inside" hooks */
				G->config->ignore_inside = TRUE;
				
				arf = get_next_arg( &i, OPT );
				continue;
			}
			
			
			
			/**
			hook include/exclude options
			i: include list for hooks
			x: exclude list for hooks
			*/
			case 'i':
			case 'I':
			{
				if( G->config->hooklist->type == LIST_EXCLUDE_HOOK )
				{
					MSG_FATAL( "Options 'i' and 'x' are mutually exclusive." );
					exit( 1 );
				}
				
				G->config->hooklist->type = LIST_INCLUDE_HOOK;
			}
			/* pass through to the exclude code. 
			the exclude code can handle either type of list, include or exclude.
			*/
			case 'x':
			case 'X':
			{
				if( G->config->hooklist->type != LIST_INCLUDE_HOOK )
				{
					G->config->hooklist->type = LIST_EXCLUDE_HOOK;
				}
				else if( ( G->prog->argv[ i ][ 1 ] == 'x' ) || ( G->prog->argv[ i ][ 1 ] == 'X' ) )
				{
					MSG_FATAL( "Options 'i' and 'x' are mutually exclusive." );
					exit( 1 );
				}
				
				/* the 'i' or 'x' option requires at least one associated argument (optarg).
				if an optarg is not found get_next_arg() will exit(1)
				*/
				arf = get_next_arg( &i, OPTARG );
				
				while( arf == OPTARG ) /* option argument found */
				{
					__int64 id = 0;
					WCHAR *name = NULL;
					
					/* if the string is not an integer then it's a hook name not an id */
					if( !str_to_int64( &id, G->prog->argv[ i ] ) )
					{
						id = 0;
						
						/* make the hook name as a wide character string */
						if( !get_wstr_from_mbstr( &name, G->prog->argv[ i ] ) )
						{
							MSG_FATAL( "get_wstr_from_mbstr() failed." );
							printf( "hook: %s\n", G->prog->argv[ i ] );
							exit( 1 );
						}
						
						_wcsupr( name ); /* convert hook name to uppercase */
					}
					
					/* append to the linked list */
					if( !add_list_item( G->config->hooklist, id, name ) )
					{
						MSG_FATAL( "add_list_item() failed." );
						printf( "hook: %s\n", G->prog->argv[ i ] );
						exit( 1 );
					}
					
					/* if add_list_item() was successful then it made a duplicate of the 
					wide string pointed to by name. in any case name should now be freed.
					*/
					free( name );
					name = NULL;
					
					/* get the option's next argument, which is optional */
					arf = get_next_arg( &i, OPT | OPTARG );
				}
				
				continue;
			}
			
			
			
			/**
			program include/exclude options
			p: include list for programs
			r: exclude list for programs
			*/
			case 'p':
			case 'P':
			{
				if( G->config->proglist->type == LIST_EXCLUDE_PROG )
				{
					MSG_FATAL( "Options 'p' and 'r' are mutually exclusive." );
					exit( 1 );
				}
				
				G->config->proglist->type = LIST_INCLUDE_PROG;
			}
			/* pass through to the exclude code. 
			the exclude code can handle either type of list, include or exclude.
			*/
			case 'r':
			case 'R':
			{
				if( G->config->proglist->type != LIST_INCLUDE_PROG )
				{
					G->config->proglist->type = LIST_EXCLUDE_PROG;
				}
				else if( ( G->prog->argv[ i ][ 1 ] == 'r' ) || ( G->prog->argv[ i ][ 1 ] == 'R' ) )
				{
					MSG_FATAL( "Options 'p' and 'r' are mutually exclusive." );
					exit( 1 );
				}
				
				
				/* the 'p' or 'r' option requires at least one associated argument (optarg).
				if an optarg is not found get_next_arg() will exit(1)
				*/
				arf = get_next_arg( &i, OPTARG );
				
				while( arf == OPTARG ) /* option argument found */
				{
					__int64 id = 0;
					WCHAR *name = NULL;
					const char *p = G->prog->argv[ i ];
					
					
					/* a colon is used as the escape character. 
					if the first character is a colon then a program name 
					is specified, not a program id. this is only necessary 
					in cases where a program name can be mistaken by 
					the parser for an option or a program id.
					*/
					if( *p == ':' )
						++p;
					
					/* if the first character is a colon, or the string is not an integer, 
					or it is and the integer is negative, then assume program name
					*/
					if( ( p != G->prog->argv[ i ] ) 
						|| ( str_to_int64( &id, G->prog->argv[ i ] ) != NUM_POS )
					)
					{
						/* make the program name as a wide character string */
						if( !get_wstr_from_mbstr( &name, p ) )
						{
							MSG_FATAL( "get_wstr_from_mbstr() failed." );
							printf( "prog: %s\n", G->prog->argv[ i ] );
							exit( 1 );
						}
						
						//_wcslwr( name ); /* convert program name to lowercase */
						
						/* program name and id are mutually exclusive. 
						elsewhere in the code if a list item's name != NULL 
						then its id is ignored.
						*/
					}
					/* else the id is valid and name remains NULL*/
					
					/* append to the linked list */
					if( !add_list_item( G->config->proglist, id, name ) )
					{
						MSG_FATAL( "add_list_item() failed." );
						printf( "prog: %s\n", G->prog->argv[ i ] );
						exit( 1 );
					}
					
					/* if add_list_item() was successful then it made a duplicate of the 
					wide string pointed to by name. in any case name should now be freed.
					*/
					free( name );
					name = NULL;
					
					/* get the option's next argument, which is optional */
					arf = get_next_arg( &i, OPT | OPTARG );
				}
				
				continue;
			}
			
			
			
			/** 
			test mode include option
			*/
			case 't':
			case 'T':
			{
				unsigned __int64 id = 0;
				WCHAR *name = NULL;
				
				
				G->config->testlist->type = LIST_INCLUDE_TEST;
				
				/* the 't' option requires one associated argument (optarg), and a second which is 
				optional.
				*/
				arf = get_next_arg( &i, OPT | OPTARG );
				if( arf != OPTARG )
				{
					print_testmode_usage();
					exit( 1 );
				}
				
				/* make the test name as a wide character string */
				if( !get_wstr_from_mbstr( &name, G->prog->argv[ i ] ) )
				{
					MSG_FATAL( "get_wstr_from_mbstr() failed." );
					printf( "name: %s\n", G->prog->argv[ i ] );
					exit( 1 );
				}
				
				arf = get_next_arg( &i, OPT | OPTARG ); // get the second optarg, which is optional
				
				/* at least one option argument (optarg) was found */
				
				/* if a second optarg was found it's the id. the value of id will be UI64_MAX if 
				the conversion fails or there's no second optarg.
				*/
				if( arf == OPTARG )
					str_to_uint64( &id, G->prog->argv[ i ] );
				else
					id = UI64_MAX;
				
				/* append to the linked list */
				if( !add_list_item( G->config->testlist, (__int64)id, name ) )
				{
					MSG_FATAL( "add_list_item() failed." );
					printf( "test id: 0x%I64X\n", id );
					printf( "test name: %ls\n", name );
					exit( 1 );
				}
				
				/* if add_list_item() was successful then it made a duplicate of the 
				wide string pointed to by name. in any case name should now be freed.
				*/
				free( name );
				name = NULL;
				
				continue;
			}
			
			
			
			/** 
			verbosity option
			*/
			case 'v':
			case 'V':
			{
				if( G->config->verbose )
				{
					MSG_FATAL( "Option 'v': this option has already been specified." );
					printf( "verbosity level: %d\n", G->config->verbose );
					exit( 1 );
				}
				
				G->config->verbose = VERBOSE_DEFAULT;
				
				/* since this option may or may not have an associated argument, 
				test for both an option's argument(optarg) or the next option
				*/
				arf = get_next_arg( &i, OPT | OPTARG );
				
				if( arf != OPTARG )
					continue;
				
				/* option argument found */
				
				/* if the string is not an integer representation, or it is but it's invalid */
				if( !str_to_int( &G->config->verbose, G->prog->argv[ i ] ) 
					|| ( G->config->verbose <= 0 ) 
				) 
				{
					MSG_FATAL( "Option 'v': verbosity level invalid." );
					printf( "verbosity level: %s\n", G->prog->argv[ i ] );
					exit( 1 );
				}
				
				continue;
			}
			
			
			
			default:
			{
				MSG_FATAL( "Unknown option." );
				printf( "OPT: %s\n", G->prog->argv[ i ] );
				exit( 1 );
			}
		}
	}
	
	
	
	if( ( G->config->proglist->type == LIST_INCLUDE_PROG )
		|| ( G->config->proglist->type == LIST_EXCLUDE_PROG )
	)
		GetSystemTimeAsFileTime( (FILETIME *)&G->config->proglist->init_time );
	
	if( ( G->config->hooklist->type == LIST_INCLUDE_HOOK )
		|| ( G->config->hooklist->type == LIST_EXCLUDE_HOOK )
	)
		GetSystemTimeAsFileTime( (FILETIME *)&G->config->hooklist->init_time );
	
	if( ( G->config->desklist->type == LIST_INCLUDE_DESK ) )
		GetSystemTimeAsFileTime( (FILETIME *)&G->config->desklist->init_time );
	
	if( ( G->config->testlist->type == LIST_INCLUDE_TEST ) )
		GetSystemTimeAsFileTime( (FILETIME *)&G->config->testlist->init_time );
	
	
	/* G->config has been initialized */
	GetSystemTimeAsFileTime( (FILETIME *)&G->config->init_time );
	return;
}



/* print_config_store()
Print a configuration store and all its descendants.

if 'store' is NULL this function returns without having printed anything.
*/
static void print_config_store( 
	struct config *store   // in
)
{
	const char *const objname = "Configuration Store";
	
	
	if( !store )
		return;
	
	PRINT_DBLSEP_BEGIN( objname );
	print_init_time( "store->init_time", store->init_time );
	
	printf( "store->ignore_inside: " );
	if( store->ignore_inside ) 
		printf( "TRUE (Ignore hooks with same owner/origin/target)" );
	else
		printf( "FALSE (Don't ignore hooks with same owner/origin/target)" );
	printf( "\n" );
	
	printf( "store->polling: %d", store->polling );
	if( store->polling >= 0 )
		printf( " (Comparing snapshots every %d seconds)", store->polling );
	else
		printf( " (Taking only one snapshot)" );
	printf( "\n" );
	
	printf( "store->verbose: %d\n", store->verbose );
	
	printf( "\nPrinting list store of user specified hooks:\n" );
	print_list_store( store->hooklist );
	
	printf( "\nPrinting list store of user specified programs:\n" );
	print_list_store( store->proglist );
	
	printf( "\nPrinting list store of user specified desktops:\n" );
	print_list_store( store->desklist );
	
	printf( "\nPrinting list store of user specified tests:\n" );
	print_list_store( store->testlist );
	
	PRINT_DBLSEP_END( objname );
	
	return;
}



/* print_global_config_store()
Print the global configuration store and all its descendants.
*/
void print_global_config_store( void )
{
	print_config_store( G->config );
	return;
}



/* free_config_store()
Free a configuration store and all its descendants.

this function then sets the config store pointer to NULL and returns

'in' is a pointer to a pointer to the config store, which contains the config information.
if( !in || !*in ) then this function returns.
*/
void free_config_store( 
	struct config **const in   // in deref
)
{
	if( !in || !*in )
		return;
	
	/* free the list stores */
	free_list_store( &(*in)->testlist );
	free_list_store( &(*in)->proglist );
	free_list_store( &(*in)->hooklist );
	free_list_store( &(*in)->desklist );
	
	free( (*in) );
	*in = NULL;
	
	return;
}
