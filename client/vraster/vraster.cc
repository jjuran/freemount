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

// command
#include "command/get_option.hh"

// gear
#include "gear/parse_decimal.hh"

// jack
#include "jack/interface.hh"

// raster
#include "raster/load.hh"

// unet-connect
#include "unet/connect.hh"

// poseven
#include "poseven/types/errno_t.hh"

// freemount-client
#include "freemount/address.hh"
#include "freemount/synced.hh"


#define PROGRAM  "vraster"

#define PORT  "/gui/port/vraster"

#define STR_LEN( s )  "" s, (sizeof s - 1)

#define USAGE  "usage: " "GUI=<gui-path> " PROGRAM " <screen-path>\n" \
"       where gui-path is a FORGE jack and screen-path is a raster file\n"

#define NOT_BLACK_ON_WHITE  "only black-on-white rasters are supported"


namespace p7 = poseven;

using namespace freemount;


enum
{
	Opt_magnify = 'x',
};

static command::option options[] =
{
	{ "magnify", Opt_magnify, command::Param_required },
	{ NULL }
};


static unsigned magnification;

static int protocol_in  = -1;
static int protocol_out = -1;


static int next_fd = 3;

static int screen_fd;

static raster::raster_load loaded_raster;


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
	
	using namespace raster;
	
	loaded_raster = load_raster( screen_fd );
	
	if ( loaded_raster.addr == NULL )
	{
		report_error( path, errno );
		
		exit( 1 );
	}
}

static
char* const* get_options( char** argv )
{
	int opt;
	
	++argv;  // skip arg 0
	
	while ( (opt = command::get_option( (char* const**) &argv, options )) > 0 )
	{
		using command::global_result;
		using gear::parse_unsigned_decimal;
		
		switch ( opt )
		{
			case Opt_magnify:
				magnification = parse_unsigned_decimal( global_result.param );
				break;
			
			default:
				break;
		}
	}
	
	return argv;
}

int main( int argc, char** argv )
{
	if ( argc == 0 )
	{
		return 0;
	}
	
	char* const* args = get_options( argv );
	
	int argn = argc - (args - argv);
	
	if ( argn != 1 )
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
	
	const char* screen_path = args[ 0 ];
	
	open_screen( screen_path );
	
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
	
	const raster::raster_desc& desc = loaded_raster.meta->desc;
	
	if ( desc.weight != 1 )
	{
		write( STDERR_FILENO, STR_LEN( PROGRAM ": " NOT_BLACK_ON_WHITE "\n" ) );
		return 1;
	}
	
	if ( desc.model != raster::Model_grayscale_paint )
	{
		write( STDERR_FILENO, STR_LEN( PROGRAM ": " NOT_BLACK_ON_WHITE "\n" ) );
		return 1;
	}
	
	const char* base = (char*) loaded_raster.addr;
	
	const size_t image_size = desc.height * desc.stride;
	
	short raster_size[ 2 ] = { desc.height, desc.width };
	short window_size[ 2 ] = { desc.height, desc.width };
	
	if ( magnification > 1  && magnification <= 16 )
	{
		window_size[ 0 ] *= magnification;
		window_size[ 1 ] *= magnification;
	}
	
	try
	{
		int lock_fd = OPEN( PORT "/lock" );
		
		LINK( "/gui/new/bitmap", PORT "/view" );
		
		PUT( PORT "/procid", "4" "\n", 2 );  // noGrow
		PUT( PORT "/.~title",  screen_path, strlen( screen_path ) );
		PUT( PORT "/.~size",   (const char*) window_size, sizeof window_size );
		PUT( PORT "/v/.~size", (const char*) raster_size, sizeof raster_size );
		
		PUT_DATA( PORT "/v/data", base, image_size );
		
		int window_fd = OPEN( PORT "/window" );
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
