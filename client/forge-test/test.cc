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


#define PROGRAM "forge-test"

#define TEST_PORT  "/gui/port/test"

#define STR_LEN( s )  "" s, (sizeof s - 1)

#define USAGE  "usage: " PROGRAM " <Freemount address>\n"


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static int next_fd = 3;

#define OPEN( path )  \
	synced_open( protocol_in, protocol_out, next_fd++, STR_LEN( path ) )

#define PUT( path, data )  \
	synced_put( protocol_in, protocol_out, STR_LEN( path ), STR_LEN( data ) )

#define LINK( src, dst )  \
	synced_link( protocol_in, protocol_out, STR_LEN( src ), STR_LEN( dst ) )

int main( int argc, char** argv )
{
	const char** connector_argv = NULL;
	
	if ( argc < 2 )
	{
		write( STDERR_FILENO, STR_LEN( USAGE ) );
		return 2;
	}
	
	char* address = argv[ 1 ];
	
	connector_argv = parse_address( address );
	
	if ( connector_argv == NULL )
	{
		write( STDERR_FILENO, STR_LEN( PROGRAM ": malformed address\n" ) );
		return 2;
	}
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	try
	{
		int lock_fd   = OPEN( TEST_PORT "/lock"   );
		int window_fd = OPEN( TEST_PORT "/window" );
		
		PUT( TEST_PORT "/w/.~title", "Test" );
		
		LINK( "/gui/new/frame",   TEST_PORT "/view"   );
		LINK( "/gui/new/caption", TEST_PORT "/v/view" );
		
		PUT( TEST_PORT "/v/padding", "4"           "\n" );
		PUT( TEST_PORT "/v/v/text",  "Hello world" "\n" );
	}
	catch ( const path_error& e )
	{
		more::perror( PROGRAM, e.path.c_str(), e.error );
		return 1;
	}
	
	/*
		Block until the server quits.  Assumes no pings or debug frames.
	*/
	
	char any;
	read( protocol_in, &any, 1 );
	
	return 0;
}
