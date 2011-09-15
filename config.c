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

#include <stdio.h>

#include "util.h"

#include "config.h"

/* the global stores */
#include "global.h"



/* the default number of seconds when polling */
#define POLLING_DEFAULT   7



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
	config->desklist = create_list_store( &config->desklist );
	
	/* allocate the list store for the linked list of hooks to filter */
	config->hooklist = create_list_store( &config->hooklist );
	
	/* allocate the list store for the linked list of programs to filter */
	config->proglist = create_list_store( &config->proglist );
	
	
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
		"current window station, and the thread/process associated with that hook.\n"
		"gethooks prints any errors to stdout, not stderr.\n"
		"\n"
		"%s usage:"
		"[-m [sec]]  [-d [desktop]]  [[-i]|[-x] <hook>]  [[-p]|[-r] <prog>]\n"
		"\n"
		"   -m     monitor mode. check for changes every n seconds (default %u).\n"
		"   -d     include only these desktops: a list of desktops separated by space.\n"
		"   -i     include only these hooks: a list of hooks separated by space.\n"
		"   -x     exclude only these hooks: a list of hooks separated by space.\n"
		"   -p     include only these programs: a list of programs separated by space.\n"
		"   -r     exclude only these programs: a list of programs separated by space.\n"
		"\n",
		G->prog->pszBaseName, POLLING_DEFAULT
	);
	
	printf( "\nexample to list WH_MOUSE hooks on the current desktop only:\n" );
	printf( " %s -d -i WH_MOUSE\n", G->prog->pszBaseName );
	
	printf( "\nexample to monitor WH_KEYBOARD from workrave.exe and pids 799 and 801:\n" );
	printf( " %s -m -i WH_KEYBOARD -p workrave.exe 799 801\n", G->prog->pszBaseName );
	
	printf( "\nTo show more examples: %s --examples\n", G->prog->pszBaseName );
	exit( 1 );
}



/* print_more_examples_and_exit()
Print more usage examples and exit(1).
*/
void print_more_examples_and_exit( void )
{
	FAIL_IF( !G->prog->init_time );   // This function depends on G->prog
	
	
	printf( "\nMore examples:\n" );
	
	printf( "\n"
		"By default this program attaches to all desktops in the current window station.\n"
		"You can specify individual desktops with the 'd' option.\n"
		"example to monitor only \"Sysinternals Desktop 1\":\n"
	);
	printf( " %s -d \"Sysinternals Desktop 1\"\n", G->prog->pszBaseName );
	
	printf( "\n"
		"You may use a hook's id instead of its name when including/excluding hooks.\n"
		"example to monitor only WH_KEYBOARD(2) and WH_MOUSE(7) hooks:\n"
	);
	printf( " %s -m -i 2 7\n", G->prog->pszBaseName );
	
	printf( "\n"
		"If the program name you want hooks for has only numbers and no extension you \n"
		"may prefix it with a colon so that it is not misinterpreted as a pid.\n"
		"example to list hooks associated with program name 907\n"
	);
	printf( " %s -p :907\n", G->prog->pszBaseName );
	
	printf( "\n"
		"If the program name you want hooks for starts with a dash and has one letter \n"
		"you may prefix it with a colon so that it is not misinterpreted as an option.\n"
		"example to list hooks associated with program name -h\n"
	);
	printf( " %s -p :-h\n", G->prog->pszBaseName );
	
	printf( "\n"
		"If the colon is the first character in an argument to program include/exclude \n"
		"it is assumed a program name follows. If the program name you want for some \n"
		"reason starts with a colon then you must prefix it with another colon.\n" 
	);
	
	printf( "\nexample piping to 'tee' to print output to stdout and file \"outfile\":\n" );
	printf( " %s -m | tee outfile\n", G->prog->pszBaseName );
	
	printf( "\n"
		"If the colon is the first character in an argument to program include/exclude \n"
		"it is assumed a program name follows. If the program name you want for some \n"
		"reason starts with a colon then you must prefix it with another colon.\n" 
	);
	
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
		int num = 0;
		
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
		else if( str_to_int( &num, G->prog->argv[ *index ] ) && ( num < 0 ) )
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
		else if( (
				( G->prog->argv[ *index ][ 0 ] == '-' ) 
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
	
	FAIL_IF( G->config->init_time );   // Fail if this store has already been initialized.
	
	FAIL_IF( !G->prog->init_time );   // The program store must already be initialized.
	
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
			desktop option
			*/
			case 'd':
			case 'D':
			{
				G->config->desklist->type = LIST_DESKTOP_INCLUDE;
				
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
						MSG_FATAL( "make_wstr_from_mbstr() failed." );
						printf( "desktop: %s\n", G->prog->argv[ i ] );
						exit( 1 );
					}
					
					/* append to the linked list */
					if( !add_list_item( G->config->desklist, 0, name );
					{
						MSG_FATAL( "add_list_item() failed." );
						printf( "desktop: %s\n", G->prog->argv[ i ] );
						exit( 1 );
					}
					
					/* if add_list_item() was successful then it made a duplicate of the 
					wide string pointed to by name. name should now be freed.
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
				if( polling >= 0 )
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
				
				/* not sure why anyone would want this */
				if( G->config->polling > 86400 )
				{
					MSG_WARNING( "Option 'm': more seconds than in a day." );
					printf( "sec: %d\n", G->config->polling );
				}
				
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
				if( G->config->hooklist->type == FILTER_EXCLUDE_HOOK )
				{
					MSG_FATAL( "Options 'i' and 'x' are mutually exclusive." );
					exit( 1 );
				}
				
				G->config->hooklist->type = FILTER_INCLUDE_HOOK;
			}
			/* pass through to the exclude code. 
			the exclude code can handle either type of list, include or exclude.
			*/
			case 'x':
			case 'X':
			{
				if( G->config->hooklist->type != FILTER_INCLUDE_HOOK )
				{
					G->config->hooklist->type = FILTER_EXCLUDE_HOOK;
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
					int id = 0;
					WCHAR *name = NULL;
					
					/* if the string is not an integer then its a hook name not an id */
					if( !str_to_int( &id, G->prog->argv[ i ] ) )
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
					if( !add_list_item( G->config->hooklist, id, name );
					{
						MSG_FATAL( "add_list_item() failed." );
						printf( "hook: %s\n", G->prog->argv[ i ] );
						exit( 1 );
					}
					
					/* if add_list_item() was successful then it made a duplicate of the 
					wide string pointed to by name. name should now be freed.
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
				if( G->config->proglist->type == FILTER_EXCLUDE_PROG )
				{
					MSG_FATAL( "Options 'p' and 'r' are mutually exclusive." );
					exit( 1 );
				}
				
				G->config->proglist->type = FILTER_INCLUDE_PROG;
			}
			/* pass through to the exclude code. 
			the exclude code can handle either type of list, include or exclude.
			*/
			case 'r':
			case 'R':
			{
				if( G->config->proglist->type != FILTER_INCLUDE_PROG )
				{
					G->config->proglist->type = FILTER_EXCLUDE_PROG;
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
					int id = 0;
					WCHAR *name = NULL;
					char *p = G->prog->argv[ i ];
					
					
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
						|| !str_to_int( &id, G->prog->argv[ i ] ) 
						|| ( id < 0 ) 
					)
					{
						/* make the program name as a wide character string */
						if( !get_wstr_from_mbstr( &name, p ) )
						{
							MSG_FATAL( "make_wstr_from_mbstr() failed." );
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
					if( !add_list_item( G->config->proglist, id, name );
					{
						MSG_FATAL( "add_list_item() failed." );
						printf( "prog: %s\n", G->prog->argv[ i ] );
						exit( 1 );
					}
					
					/* if add_list_item() was successful then it made a duplicate of the 
					wide string pointed to by name. name should now be freed.
					*/
					free( name );
					name = NULL;
					
					/* get the option's next argument, which is optional */
					arf = get_next_arg( &i, OPT | OPTARG );
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
	
	
	/* G->config has been initialized */
	GetSystemTimeAsFileTime( (FILETIME *)&G->config->init_time );
	return;
}



/* print_config_store()
Print a configuration store and all its descendants.
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
	
	printf( "store->polling: %d", store->polling );
	if( store->polling >= 0 )
		printf( " (Comparing snapshots every %d seconds)", store->polling );
	else
		printf( " (Taking only one snapshot)" );
	
	printf( "\n" );
	
	//printf( "Printing list store of user specified hooks:" );
	print_list( store->hooklist );
	
	//printf( "Printing list store of user specified programs:" );
	print_list( store->proglist );
	
	//printf( "Printing list store of user specified desktops:" );
	print_list( store->desklist );
	
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
	free_list_store( &(*in)->proglist );
	free_list_store( &(*in)->hooklist );
	free_list_store( &(*in)->desklist );
	
	free( (*in) );
	*in = NULL;
	
	return;
}
