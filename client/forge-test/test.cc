/*
	test.cc
	-------
*/

// POSIX
#include <unistd.h>

// more-posix
#include "more/perror.hh"

// unet-connect
#include "unet/connect.hh"

// freemount-client
#include "freemount/address.hh"
#include "freemount/synced.hh"


#define TEST_PORT  "/gui/port/test"

#define STR_LEN( s )  "" s, (sizeof s - 1)

#define USAGE  "usage: forge-test <Freemount address>\n"


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static int next_fd = 3;

#define OPEN( path )  \
	synced_open( protocol_in, protocol_out, next_fd++, STR_LEN( path ) )

int main( int argc, char** argv )
{
	if ( argc < 2 )
	{
		write( STDERR_FILENO, STR_LEN( USAGE ) );
		return 2;
	}
	
	char* address = argv[ 1 ];
	
	const char** connector_argv = parse_address( address );
	
	if ( connector_argv == NULL )
	{
		return 2;
	}
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	try
	{
		int lock_fd   = OPEN( TEST_PORT "/lock"   );
		int window_fd = OPEN( TEST_PORT "/window" );
	}
	catch ( const path_error& e )
	{
		more::perror( "forge-test", e.path.c_str(), e.error );
		return 1;
	}
	
	pause();
	
	return 0;
}
