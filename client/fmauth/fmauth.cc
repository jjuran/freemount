/*
	fmauth.cc
	---------
*/

// POSIX
#include <unistd.h>

// Standard C
#include <stdlib.h>

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_eom:
			write( STDOUT_FILENO, STR_LEN( "auth ok\n" ) );
			exit( 0 );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

int main( int argc, char** argv )
{
	const char* connector = getenv( "FREEMOUNT_CONNECT" );
	
	if ( connector == NULL )
	{
		connector = getenv( "UNET_CONNECT" );
	}
	
	if ( connector == NULL )
	{
		connector = "unet-connect";
	}
	
	const char* connector_argv[] = { "/bin/sh", "-c", connector, NULL };
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	send_empty_request( protocol_out, req_auth );
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	return looped != 0;
}

