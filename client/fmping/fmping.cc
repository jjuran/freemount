/*
	fmping.cc
	---------
*/

// POSIX
#include <unistd.h>
#include <sys/socket.h>

// Standard C
#include <stdlib.h>

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"


using namespace freemount;


static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_pong:
			write( STDERR_FILENO, "pong\n", 5 );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

int main( int argc, char** argv )
{
	send_empty_fragment( STDOUT_FILENO, frag_ping );
	
	shutdown( STDOUT_FILENO, SHUT_WR );
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, STDIN_FILENO );
	
	return looped != 0;
}

