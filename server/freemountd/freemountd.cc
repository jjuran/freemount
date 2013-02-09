/*
	freemountd.cc
	-------------
*/

// POSIX
#include <unistd.h>

// standard C
#include <stdlib.h>

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"


using namespace freemount;


static bool auth_pending = false;


static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_ping:
			write( STDERR_FILENO, "ping\n", 5 );
			
			send_empty_fragment( STDOUT_FILENO, frag_pong );
			break;
		
		case frag_req:
			switch ( fragment.data )
			{
				case req_auth:
					auth_pending = true;
					
					write( STDERR_FILENO, "auth...", 7 );
					break;
				
				default:
					abort();
			}
			
			break;
		
		case frag_eom:
			if ( !auth_pending )
			{
				abort();
			}
			
			auth_pending = false;
			
			write( STDERR_FILENO, " ok\n", 4 );
			
			send_empty_fragment( STDOUT_FILENO, frag_eom );  // send response
			break;
		
		default:
			abort();
	}
	
	return 0;
}

int main( int argc, char** argv )
{
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, STDIN_FILENO );
	
	return looped != 0;
}

