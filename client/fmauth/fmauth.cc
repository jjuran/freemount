/*
	fmauth.cc
	---------
*/

// POSIX
#include <unistd.h>

// Standard C
#include <stdlib.h>

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"


using namespace freemount;


const int protocol_in  = 6;
const int protocol_out = 7;


static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_eom:
			write( STDOUT_FILENO, "auth ok\n", 8 );
			exit( 0 );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

int main( int argc, char** argv )
{
	send_empty_request( protocol_out, req_auth );
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	return looped != 0;
}

