/*
	ping-pong.cc
	------------
*/

// POSIX
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"

// tap-out
#include "tap/check.hh"
#include "tap/test.hh"


static const unsigned n_tests = 4;


using namespace freemount;

using tap::ok_if;


static int n_pongs;


static int client_frame_handler( void* that, const frame_header& frame )
{
	switch ( frame.type )
	{
		case frag_pong:
			++n_pongs;
			break;
		
		default:
			abort();
	}
	
	return 0;
}

static int server_frame_handler( void* that, const frame_header& frame )
{
	switch ( frame.type )
	{
		case frag_ping:
			send_empty_frame( STDOUT_FILENO, frag_pong );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

static int client( int fd )
{
	send_empty_frame( fd, frag_ping );
	send_empty_frame( fd, frag_ping );
	send_empty_frame( fd, frag_ping );
	
	CHECK( shutdown( fd, SHUT_WR ) );
	
	data_receiver r( &client_frame_handler, NULL );
	
	return run_event_loop( r, fd );
}

static int server()
{
	data_receiver r( &server_frame_handler, NULL );
	
	return run_event_loop( r, STDIN_FILENO );
}

static void ping_pong( const char* argv0 )
{
	int fds[ 2 ];
	
	CHECK( socketpair( PF_LOCAL, SOCK_STREAM, 0, fds ) );
	
	pid_t pid = CHECK( vfork() );
	
	if ( pid == 0 )
	{
		CHECK( dup2( fds[1], STDIN_FILENO  ) );
		CHECK( dup2( fds[1], STDOUT_FILENO ) );
		
		CHECK( execl( argv0, argv0, "--server", NULL ) );
	}
	
	CHECK( close( fds[1] ) );
	
	int status = client( fds[0] );
	
	ok_if( status == 0 );
	
	ok_if( wait( &status ) == pid );
	
	ok_if( status == 0 );
	
	ok_if( n_pongs == 3 );
	
	close( fds[0] );
}

int main( int argc, char** argv )
{
	if ( argc > 1 )
	{
		return server();
	}
	
	tap::start( "ping-pong", n_tests );
	
	ping_pong( argv[0] );
	
	return 0;
}
