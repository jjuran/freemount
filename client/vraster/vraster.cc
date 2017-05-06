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

// config
#include "config/setpshared.h"

// more-posix
#include "more/perror.hh"

// command
#include "command/get_option.hh"

// gear
#include "gear/parse_decimal.hh"

// jack
#include "jack/fifo.hh"
#include "jack/interface.hh"

// raster
#include "raster/load.hh"
#include "raster/relay.hh"
#include "raster/relay_detail.hh"
#include "raster/sync.hh"

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

#define USAGE  "usage: " PROGRAM "--gui <gui-path> <raster-path>\n" \
"       where gui-path is a FORGE jack and raster-path is a raster file\n"

#define NO_GRAYSCALE_LIGHT  "grayscale 'light' rasters aren't yet supported"

#define POLLING_ENSUES  \
	"WARNING: pthread_cond_wait() is broken -- will poll every 10ms instead"

#define ERROR( msg )  write( STDERR_FILENO, STR_LEN( PROGRAM ": " msg "\n" ) )

namespace p7 = poseven;

using namespace freemount;

const size_t max_payload = uint16_t( -1 );


enum
{
	Opt_gui     = 'g',
	Opt_mnt     = 'm',
	Opt_title   = 't',
	Opt_watch   = 'w',
	Opt_magnify = 'x',
};

static command::option options[] =
{
	{ "gui",     Opt_gui,     command::Param_required },
	{ "mnt",     Opt_mnt,     command::Param_required },
	{ "title",   Opt_title,   command::Param_required },
	{ "watch",   Opt_watch                            },
	{ "magnify", Opt_magnify, command::Param_required },
	{ NULL }
};


static const char*  gui_path;
static char*        mnt_path;

static const char* title;

static bool watching;

static unsigned x_numerator   = 1;
static unsigned x_denominator = 1;

static int protocol_in  = -1;
static int protocol_out = -1;


static int next_fd = 3;

static raster::raster_load loaded_raster;


#define OPEN( path )  \
	synced_open( protocol_in, protocol_out, next_fd++, STR_LEN( path ) )

#define PUT( path, data, size )  \
	synced_put( protocol_in, protocol_out, STR_LEN( path ), data, size )

#define PWRITE( path, data, size, off )  \
	synced_pwrite( protocol_in, protocol_out, STR_LEN( path ), data, size, off )

#define LINK( src, dst )  \
	synced_link( protocol_in, protocol_out, STR_LEN( src ), STR_LEN( dst ) )


static inline
size_t min( size_t a, size_t b )
{
	return b < a ? b : a;
}

static
void report_error( const char* path, uint32_t err )
{
	more::perror( PROGRAM, path, err );
}

static
void wait_on_fifo( jack::interface ji )
{
	const char* fifo_path = ji.fifo_path();
	
	int nok = jack::fifo_wait( fifo_path );
	
	if ( nok < 0 )
	{
		report_error( fifo_path, errno );
		
		exit( 1 );
	}
}

static
raster::sync_relay* open_raster( const char* path )
{
	if ( watching )
	{
		wait_on_fifo( path );
	}
	
	const int flags = watching ? O_RDWR : O_RDONLY;
	
	int raster_fd = open( path, flags );
	
	if ( raster_fd < 0 )
	{
		report_error( path, errno );
		
		exit( 1 );
	}
	
	using namespace raster;
	
	loaded_raster = watching ? play_raster( raster_fd )
	                         : load_raster( raster_fd );
	
	if ( loaded_raster.addr == NULL )
	{
		report_error( path, errno );
		
		exit( 1 );
	}
	
	close( raster_fd );
	
	if ( watching )
	{
		raster_note* sync = find_note( *loaded_raster.meta, Note_sync );
		
		if ( ! is_valid_sync( sync ) )
		{
			report_error( path, ENOSYS );
			exit( 3 );
		}
		
		return (sync_relay*) data( sync );
	}
	
	return NULL;
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
			case Opt_gui:
				gui_path = global_result.param;
				break;
			
			case Opt_mnt:
				mnt_path = global_result.param;
				break;
			
			case Opt_title:
				title = global_result.param;
				break;
			
			case Opt_watch:
				if ( ! CONFIG_SETPSHARED )
				{
					ERROR( "process-shared mutexes and condvars unavailable" );
					exit( 10 );
				}
				
				watching = true;
				break;
			
			case Opt_magnify:
				const char* p;
				p = global_result.param;
				
				x_numerator = parse_unsigned_decimal( &p );
				
				if ( x_numerator == 0 )
				{
					ERROR( "0x magnification is unimplemented" );
					exit( 2 );
				}
				
				if ( *p++ == '/' )
				{
					x_denominator = parse_unsigned_decimal( &p );
					
					if ( x_denominator == 0 )
					{
						ERROR( "division by zero is forbidden" );
						exit( 2 );
					}
				}
				break;
			
			default:
				break;
		}
	}
	
	return argv;
}

static
void write_image( const char* base, size_t image_size, size_t chunk_size )
{
	size_t n_written = 0;
	
	while ( n_written < image_size - chunk_size )
	{
		PWRITE( PORT "/v/data", base, chunk_size, n_written );
		
		n_written += chunk_size;
		base      += chunk_size;
	}
	
	if ( size_t remainder = image_size - n_written )
	{
		PWRITE( PORT "/v/data", base, remainder, n_written );
	}
}

static
void update_loop( raster::sync_relay*  sync,
                  const char*          base,
                  size_t               image_size,
                  size_t               chunk_size )
{
	uint32_t seed = 0;
	
	bool wait_is_broken = false;
	
	while ( CONFIG_SETPSHARED  &&  sync )
	{
		while ( seed == sync->seed )
		{
			if ( wait_is_broken )
			{
				usleep( 10000 );  // 10ms
			}
			else
			{
				try
				{
					raster::wait( *sync );
				}
				catch ( const raster::wait_failed& )
				{
					ERROR( POLLING_ENSUES );
					
					wait_is_broken = true;
				}
			}
		}
		
		seed = sync->seed;
		
		write_image( base, image_size, chunk_size );
	}
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
	
	if ( gui_path  &&  mnt_path )
	{
		write( STDERR_FILENO, STR_LEN( PROGRAM ": "
			"--gui and --mnt are mutually exclusive\n" ) );
		return 2;
	}
	
	const char** connector_argv = NULL;
	
	if ( mnt_path != NULL )
	{
		connector_argv = parse_address( mnt_path );
		
		if ( connector_argv == NULL )
		{
			write( STDERR_FILENO, STR_LEN( PROGRAM ": malformed address\n" ) );
			return 2;
		}
	}
	else
	{
		if ( gui_path == NULL )
		{
			gui_path = getenv( "GUI" );
		}
		
		if ( gui_path == NULL )
		{
			write( STDERR_FILENO, STR_LEN( PROGRAM ": "
				"one of --mnt, --gui, or $GUI required\n" ) );
			return 2;
		}
		
		static jack::interface ji = gui_path;
		
		connector_argv = make_unix_connector( ji.socket_path() );
	}
	
	const char* raster_path = args[ 0 ];
	
	raster::sync_relay* sync = open_raster( raster_path );
	
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
	
	if ( desc.model == raster::Model_grayscale_light )
	{
		write( STDERR_FILENO, STR_LEN( PROGRAM ": " NO_GRAYSCALE_LIGHT "\n" ) );
		return 1;
	}
	
	const char* base = (char*) loaded_raster.addr;
	
	const size_t chunk_height = min( desc.height, max_payload / desc.stride );
	
	const size_t chunk_size = chunk_height * desc.stride;
	const size_t image_size = desc.height * desc.stride;
	
	short stride = desc.stride;
	char  depth  = desc.weight;
	
	if ( depth == 16  &&  desc.model == raster::Model_xRGB )
	{
		depth = 15;
	}
	
	char grayscale = desc.model == raster::Model_grayscale_paint;
	
	char alpha_last = (desc.model & 1) == (raster::Model_RGBx & 1);
	
	char little_endian = (bool) *(uint16_t*) (base + loaded_raster.size - 4);
	
	short raster_size[ 2 ] = { desc.height, desc.width };
	short window_size[ 2 ] = { desc.height, desc.width };
	
	if ( x_numerator > 1  &&  x_numerator <= 16 )
	{
		window_size[ 0 ] *= x_numerator;
		window_size[ 1 ] *= x_numerator;
	}
	
	if ( x_denominator > 1 )
	{
		window_size[ 0 ] /= x_denominator;
		window_size[ 1 ] /= x_denominator;
	}
	
	if ( title == NULL )
	{
		title = raster_path;
	}
	
	try
	{
		int lock_fd = OPEN( PORT "/lock" );
		
		if ( desc.weight == 1 )
		{
			LINK( "/gui/new/bitmap", PORT "/view" );
		}
		else
		{
			LINK( "/gui/new/gworld", PORT "/view" );
			
			PUT( PORT "/v/.~alpha-last", &alpha_last, sizeof (char) );
			PUT( PORT "/v/.~little-endian", &little_endian, sizeof (char) );
			
			PUT( PORT "/v/.~depth",     &depth,     sizeof depth     );
			PUT( PORT "/v/.~grayscale", &grayscale, sizeof grayscale );
			
			PUT( PORT "/v/.~stride", (const char*) &stride, sizeof stride );
		}
		
		try
		{
			PUT( PORT "/compositing", "1" "\n", 2 );
		}
		catch ( ... )
		{
			// This existence of this property is platform-dependent.
		}
		
		PUT( PORT "/procid", "4" "\n", 2 );  // noGrow
		PUT( PORT "/.~size",   (const char*) window_size, sizeof window_size );
		PUT( PORT "/v/.~size", (const char*) raster_size, sizeof raster_size );
		
		if ( title[ 0 ] != '\0' )
		{
			PUT( PORT "/.~title",  title, strlen( title ) );
		}
		
		write_image( base, image_size, chunk_size );
		
		int window_fd = OPEN( PORT "/window" );
		
		update_loop( sync, base, image_size, chunk_size );
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
