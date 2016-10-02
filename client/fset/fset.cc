/*
	fset.cc
	-------
*/

// POSIX
#include <unistd.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// more-posix
#include "more/perror.hh"

// unet-connect
#include "unet/connect.hh"

// freemount-client
#include "freemount/address.hh"
#include "freemount/synced.hh"


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static
void report_error( const char* path, uint32_t err )
{
	more::perror( "fset", path, err );
}

int main( int argc, char** argv )
{
	if ( argc <= 2  ||  argv[1][0] == '\0' )
	{
		return 0;
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
	
	const char* path = connector_argv[ -1 ];
	
	if ( path == NULL )
	{
		path = "/";
	}
	
	size_t path_len = strlen( path );
	
	char* data = argv[ 2 ];
	
	size_t len = strlen( data );
	
	data[ len++ ] = '\n';
	
	try
	{
		synced_put( protocol_in, protocol_out, path, path_len, data, len );
	}
	catch ( const path_error& e )
	{
		report_error( e.path.c_str(), e.error );
	}
	
	return 0;
}
