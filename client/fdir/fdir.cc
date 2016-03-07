/*
	fdir.cc
	-------
*/

// POSIX
#include <unistd.h>
#include <sys/socket.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// more-posix
#include "more/perror.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/frame_size.hh"
#include "freemount/receiver.hh"

// freemount-client
#include "freemount/address.hh"
#include "freemount/requests.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static const char* the_path = "/";

static int the_result;


static int frame_handler( void* that, const frame_header& frame )
{
	switch ( frame.type )
	{
		case Frame_fatal:
			write( STDERR_FILENO, STR_LEN( "[FATAL]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_error:
			write( STDERR_FILENO, STR_LEN( "[ERROR]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_debug:
			write( STDERR_FILENO, STR_LEN( "[DEBUG]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_dentry_name:
			write( STDOUT_FILENO, get_data( frame ), get_size( frame ) );
			write( STDOUT_FILENO, STR_LEN( "\n" ) );
			break;
		
		case Frame_result:
			the_result = get_u32( frame );
			
			shutdown( protocol_out, SHUT_WR );
			break;
		
		default:
			write( STDERR_FILENO, STR_LEN( "Unfrag\n" ) );
			
			abort();
	}
	
	return 0;
}

int main( int argc, char** argv )
{
	char* address = argv[ argc > 0 ];
	
	const char** connector_argv = parse_address( address );
	
	if ( connector_argv == NULL )
	{
		return 2;
	}
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	the_path = connector_argv[ -1 ];
	
	if ( the_path == NULL )
	{
		the_path = "/";
	}
	
	send_list_request( protocol_out, the_path, strlen( the_path ) );
	
	data_receiver r( &frame_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	if ( looped < 0 )
	{
		more::perror( "fdir", -looped );
		
		return 1;
	}
	
	if ( the_result != 0 )
	{
		more::perror( "fdir", the_path, the_result );
		
		return 1;
	}
	
	return 0;
}
