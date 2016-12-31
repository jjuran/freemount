/*
	vraster.cc
	----------
*/

// POSIX
#include <fcntl.h>
#include <unistd.h>

// Standard C
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// more-posix
#include "more/perror.hh"

// jack
#include "jack/interface.hh"

// unet-connect
#include "unet/connect.hh"

// poseven
#include "poseven/types/errno_t.hh"

// freemount-client
#include "freemount/address.hh"
#include "freemount/synced.hh"


#define PROGRAM  "vraster"

#define PORT_VRASTER  "/gui/port/vraster"

#define STR_LEN( s )  "" s, (sizeof s - 1)

#define USAGE  "usage: " "GUI=<gui-path> " PROGRAM " <screen-path>\n" \
"       where gui-path is a FORGE jack and screen-path is a 21888-byte file\n"


namespace p7 = poseven;

using namespace freemount;


static int protocol_in  = -1;
static int protocol_out = -1;


static int next_fd = 3;

static int screen_fd;

const size_t buffer_size = 21888;  // 512 * 342 / 8

static char screen_buffer[ buffer_size ];


#define OPEN( path )  \
	synced_open( protocol_in, protocol_out, next_fd++, STR_LEN( path ) )

#define PUT( path, data, size )  \
	synced_put( protocol_in, protocol_out, STR_LEN( path ), data, size )

#define PUT_DATA( path, data, size )  \
	synced_pwrite( protocol_in, protocol_out, STR_LEN( path ), data, size, 0 )

#define LINK( src, dst )  \
	synced_link( protocol_in, protocol_out, STR_LEN( src ), STR_LEN( dst ) )


static
void report_error( const char* path, uint32_t err )
{
	more::perror( PROGRAM, path, err );
}

static
void open_screen( const char* path )
{
	screen_fd = open( path, O_RDONLY );
	
	if ( screen_fd < 0 )
	{
		report_error( path, errno );
		
		exit( 1 );
	}
}

static
void read_screen()
{
	ssize_t n_read = pread( screen_fd, screen_buffer, buffer_size, 0 );
	
	if ( n_read < 0 )
	{
		report_error( "<screen>", errno );
		
		exit( 1 );
	}
}

int main( int argc, char** argv )
{
	if ( argc != 2 )
	{
		write( STDERR_FILENO, STR_LEN( USAGE ) );
		return 2;
	}
	
	const char* gui_path = getenv( "GUI" );
	
	if ( gui_path == NULL )
	{
		write( STDERR_FILENO, STR_LEN( PROGRAM ": "
			"GUI environment variable required\n" ) );
		return 2;
	}
	
	jack::interface ji = gui_path;
	
	const char** connector_argv = make_unix_connector( ji.socket_path() );
	
	const char* screen_path = argv[ 1 ];
	
	open_screen( screen_path );
	read_screen();
	
	unet::connection_box the_connection;
	
	try
	{
		the_connection = unet::connect( connector_argv );
	}
	catch ( const p7::errno_t& err )
	{
		more::perror( PROGRAM ": connection error", err );
		
		return 1;
	}
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	const short size[ 2 ] = { 342, 512 };
	
	try
	{
		int lock_fd = OPEN( PORT_VRASTER "/lock" );
		
		LINK( "/gui/new/bitmap",   PORT_VRASTER "/view" );
		
		PUT( PORT_VRASTER "/procid", "4" "\n", 2 );  // noGrow
		PUT( PORT_VRASTER "/.~title",  screen_path, strlen( screen_path ) );
		PUT( PORT_VRASTER "/.~size",   (const char*) size, sizeof size );
		PUT( PORT_VRASTER "/v/.~size", (const char*) size, sizeof size );
		
		PUT_DATA( PORT_VRASTER "/v/bits", screen_buffer, buffer_size );
		
		int window_fd = OPEN( PORT_VRASTER "/window" );
	}
	catch ( const path_error& e )
	{
		report_error( e.path.c_str(), e.error );
		return 1;
	}
	
	/*
		Block until the server quits.  Assumes no pings or debug frames.
	*/
	
	char any;
	read( protocol_in, &any, 1 );
	
	return 0;
}
