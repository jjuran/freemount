/*
	fmping.cc
	---------
*/

// POSIX
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>

// Standard C
#include <stdlib.h>

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"


using namespace freemount;


static int event_loop_result;

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

static void* event_loop_start( void* arg )
{
	data_receiver r( &fragment_handler, NULL );
	
	event_loop_result = run_event_loop( r, STDIN_FILENO );
	
	return NULL;
}

int main( int argc, char** argv )
{
	pthread_t event_loop;
	
	pthread_create( &event_loop, NULL, &event_loop_start, NULL );
	
	send_empty_fragment( STDOUT_FILENO, frag_ping );
	
	shutdown( STDOUT_FILENO, SHUT_WR );
	
	pthread_join( event_loop, NULL );
	
	return event_loop_result != 0;
}

