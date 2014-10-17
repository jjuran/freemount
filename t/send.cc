/*
	send.cc
	-------
*/

// POSIX
#include <unistd.h>

// Standard C
#include <string.h>

// freemount
#include "freemount/send.hh"

// tap-out
#include "tap/check.hh"
#include "tap/test.hh"


static const unsigned n_tests = 2;


using freemount::send_empty_frame;


static void send_empty()
{
	int fds[ 2 ];
	
	CHECK( pipe( fds ) );
	
	send_empty_frame( fds[1], 0x2B );
	
	char buffer[ 9 ];
	
	EXPECT( read( fds[0], buffer, 9 ) == 8 );
	
	EXPECT( memcmp( buffer, "\0\0\0\0\0\0\x2B\0", 8 ) == 0 );
	
	close( fds[0] );
	close( fds[1] );
}

int main( int argc, char** argv )
{
	tap::start( "send", n_tests );
	
	send_empty();
	
	return 0;
}

