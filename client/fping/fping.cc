/*
	fping.cc
	--------
*/

// POSIX
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>

// Standard C
#include <stdlib.h>

// command
#include "command/errors.hh"
#include "command/get_option.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"

// freemount-client
#include "freemount/client.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace command::constants;
using namespace freemount;


enum
{
	Option_count = 'c',
};

static command::option options[] =
{
	{ "", Option_count, Param_required },
	{ NULL }
};

static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;

const int interval = 1;

static int count = -1;

static int event_loop_result;

static int frame_handler( void* that, const frame_header& frame )
{
	switch ( frame.type )
	{
		case Frame_ping:
			write( STDERR_FILENO, STR_LEN( "ping\n" ) );
			
			send_empty_frame( protocol_out, Frame_pong );
			break;
		
		case Frame_pong:
			write( STDERR_FILENO, STR_LEN( "pong\n" ) );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

static void* event_loop_start( void* arg )
{
	data_receiver r( &frame_handler, NULL );
	
	event_loop_result = run_event_loop( r, protocol_in );
	
	return NULL;
}

static void* pinger_start( void* arg )
{
	for ( ;; )
	{
		send_empty_frame( protocol_out, Frame_ping );
		
		if ( count > 0  &&  --count == 0 )
		{
			break;
		}
		
		sleep( interval );
	}
	
	shutdown( protocol_out, SHUT_WR );
	
	return NULL;
}

#define BAD_USAGE( text, arg )  command::usage( STR_LEN( text ": " ), arg )

static char* const* get_options( char* const* argv )
{
	if ( *argv == NULL )
	{
		return argv;
	}
	
	++argv;  // skip arg 0
	
	short opt;
	
	while ( (opt = command::get_option( &argv, options )) )
	{
		switch ( opt )
		{
			case Option_count:
				count = atoi( command::global_result.param );
				break;
			
			default:
				abort();
		}
	}
	
	return argv;
}

int main( int argc, char** argv )
{
	char* const* args = get_options( argv );
	
	char* address = args[ 0 ];  // may be NULL
	
	const char** connector_argv = parse_address( address );
	
	if ( connector_argv == NULL )
	{
		BAD_USAGE( "Malformed address", address );
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
