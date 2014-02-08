/*
	fping.cc
	--------
*/

// POSIX
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"

// freemount-client
#include "freemount/client.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)

#define ARRAYLEN( array )  (sizeof array / sizeof array[0])


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;

const int interval = 1;

static int count = -1;

static int event_loop_result;

static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_ping:
			write( STDERR_FILENO, STR_LEN( "ping\n" ) );
			
			send_empty_fragment( protocol_out, frag_pong );
			break;
		
		case frag_pong:
			write( STDERR_FILENO, STR_LEN( "pong\n" ) );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

static void* event_loop_start( void* arg )
{
	data_receiver r( &fragment_handler, NULL );
	
	event_loop_result = run_event_loop( r, protocol_in );
	
	return NULL;
}

static void* pinger_start( void* arg )
{
	for ( ;; )
	{
		send_empty_fragment( protocol_out, frag_ping );
		
		if ( count > 0  &&  --count == 0 )
		{
			break;
		}
		
		sleep( interval );
	}
	
	shutdown( protocol_out, SHUT_WR );
	
	return NULL;
}

static void bad_usage( const char* text, size_t text_size, const char* arg )
{
	const iovec iov[] =
	{
		{ (void*) text, text_size    },
		{ (void*) arg, strlen( arg ) },
		{ (void*) STR_LEN( "\n" )    },
	};
	
	writev( STDERR_FILENO, iov, ARRAYLEN( iov ) );
}

#define BAD_USAGE( text, arg )  (bad_usage( STR_LEN( text ": " ), arg ), (char**) NULL)

static char** get_options( int argc, char** argv )
{
	if ( *argv == NULL )
	{
		return argv;
	}
	
	while ( const char* arg = *++argv )
	{
		if ( arg[0] == '-' )
		{
			if ( arg[1] == '\0' )
			{
				// An "-" argument is not an option and means /dev/fd/0
				break;
			}
			
			if ( arg[1] == '-' )
			{
				// long option or "--"
				
				const char* option = arg + 2;
				
				if ( *option == '\0' )
				{
					++argv;
					break;
				}
				
				// no long options
				
				return BAD_USAGE( "Unknown option", arg );
			}
			
			// short option
			
			switch ( arg[1] )
			{
				case 'c':
					count = atoi( arg[2] ? arg + 2 : *++argv );
					break;
				
				case '\0':  // "-" argument not recognized
				default:
					return BAD_USAGE( "Unknown option", arg );
			}
			
			continue;
		}
		
		// not an option
		break;
	}
	
	return argv;
}

int main( int argc, char** argv )
{
	char** params = get_options( argc, argv );
	
	if ( params == NULL )
	{
		return 2;
	}
	
	const int n_params = argc - (params - argv);
	
	char* address = params[ 0 ];  // may be NULL
	
	const char** connector_argv = parse_address( address );
	
	if ( connector_argv == NULL )
	{
		BAD_USAGE( "Malformed address", address );
		
		return 2;
	}
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	pthread_t event_loop;
	pthread_t pinger;
	
	pthread_create( &event_loop, NULL, &event_loop_start, NULL );
	pthread_create( &pinger,     NULL, &pinger_start,     NULL );
	
	pthread_join( event_loop, NULL );
	
	return event_loop_result != 0;
}

