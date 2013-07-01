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


static request_type pending_request_type = req_none;


static void send_response( int result )
{
	if ( result >= 0 )
	{
		write( STDERR_FILENO, " ok\n", 4 );
		
		send_empty_fragment( STDOUT_FILENO, frag_eom );
	}
	else
	{
		write( STDERR_FILENO, " err\n", 5 );
		
		send_u32_fragment( STDOUT_FILENO, frag_err, -result );
	}
}

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
					write( STDERR_FILENO, "auth...", 7 );
					break;
				
				default:
					abort();
			}
			
			pending_request_type = request_type( fragment.data );
			
			break;
		
		case frag_eom:
			int err;
			
			err = 0;
			
			switch ( pending_request_type )
			{
				case req_auth:
					break;
				
				default:
					abort();
			}
			
			pending_request_type = req_none;
			
			send_response( err );
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

