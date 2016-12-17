/*
	about-forge.cc
	--------------
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


#define PROGRAM  "about-forge"

#define PORT  "/gui/port/about"

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
	if ( argc < 2 )
	{
		write( STDERR_FILENO, STR_LEN( USAGE ) );
		return 2;
	}
	
	char* address = argv[ 1 ];
	
	const char** connector_argv = parse_address( address );
	
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
		int lock_fd = OPEN( PORT "/lock" );
		
		PUT( PORT "/procid", "4" "\n" );
		
		PUT( PORT "/.~title", "About FORGE" );
		PUT( PORT "/size", "220,68" "\n" );
		
		LINK( "/gui/new/stack", PORT "/view" );
		
		LINK( "/gui/new/frame", PORT "/v/icon/view" );
		LINK( "/gui/new/frame", PORT "/v/main/view" );
		
		PUT( PORT "/v/icon/v/width",  "32" "\n" );
		PUT( PORT "/v/icon/v/height", "32" "\n" );
		
		PUT( PORT "/v/icon/v/.margin-top",   "13" "\n" );
		PUT( PORT "/v/icon/v/.margin-left",  "23" "\n" );
		PUT( PORT "/v/icon/v/.margin-right", "23" "\n" );
		
		PUT( PORT "/v/main/v/.margin-top",   "13" "\n" );
		PUT( PORT "/v/main/v/.margin-right", "13" "\n" );
		PUT( PORT "/v/main/v/.margin-left",  "78" "\n" );
		
		LINK( "/gui/new/icon", PORT "/v/icon/v/view" );
		
		LINK( "/gui/new/caption", PORT "/v/main/v/view" );
		
		PUT( PORT "/v/icon/v/v/data", "128" "\n" );
		
		PUT( PORT "/v/main/v/v/text", "FORGE" "\n" "by Josh Juran" "\n" );
		
		PUT( PORT "/v/icon/v/v/disabling", "1" "\n" );
		PUT( PORT "/v/main/v/v/disabling", "1" "\n" );
		
		PUT( PORT "/vis", "0" "\n" );
		
		int window_fd = OPEN( PORT "/window" );
		
		PUT( PORT "/w/text-font", "0" "\n" );
		
		PUT( PORT "/w/vis", "1" "\n" );
	}
	catch ( const path_error& e )
	{
		more::perror( "about-forge", e.path.c_str(), e.error );
		return 1;
	}
	
	/*
		Block until the server quits.  Assumes no pings or debug frames.
	*/
	
	char any;
	read( protocol_in, &any, 1 );
	
	return 0;
}
