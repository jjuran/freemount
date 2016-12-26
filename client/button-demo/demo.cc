/*
	demo.cc
	-------
*/

// POSIX
#include <unistd.h>

// Standard C
#include <stdlib.h>

// more-posix
#include "more/perror.hh"

// jack
#include "jack/interface.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/frame_size.hh"
#include "freemount/receiver.hh"
#include "freemount/requests.hh"

// freemount-client
#include "freemount/address.hh"
#include "freemount/synced.hh"


#define PROGRAM  "button-demo"

#define PORT  "/gui/port/button-demo"

#define USAGE  "usage: " PROGRAM " <Freemount address>\n"

#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;

static bool clicked = false;

static int next_fd = 3;

#define OPEN( path )  \
	synced_open( protocol_in, protocol_out, next_fd++, STR_LEN( path ) )

#define PUT( path, data )  \
	synced_put( protocol_in, protocol_out, STR_LEN( path ), STR_LEN( data ) )

#define LINK( src, dst )  \
	synced_link( protocol_in, protocol_out, STR_LEN( src ), STR_LEN( dst ) )

#define CANCEL( r_id )  cancel_request( protocol_out, r_id )

struct request_status
{
	int result;
};

static
void report_error( const char* path, uint32_t err )
{
	more::perror( PROGRAM, path, err );
}

static
int wait_for_result( int fd, frame_handler_function handler, const char* path )
{
	request_status req;
	
	data_receiver r( handler, &req );
	
	int looped = run_event_loop( r, fd );
	
	if ( looped < 0  ||  (looped == 0  &&  (looped = -ECONNRESET)) )
	{
		report_error( path, -looped );
		
		exit( 1 );
	}
	
	if ( looped > 2 )
	{
		abort();
	}
	
	int result = req.result;
	
	if ( looped == 2 )
	{
		result = -result;  // errno
	}
	
	return result;
}

static
int frame_handler( void* that, const frame_header& frame );

static
void set_property( const char*  path,
                   uint32_t     path_size,
                   const char*  data,
                   uint32_t     data_size,
                   uint8_t      r_id )
{
	send_write_request( protocol_out, path, path_size, data, data_size, r_id );
}

static
int frame_handler( void* that, const frame_header& frame )
{
	if ( frame.type == Frame_recv_data )
	{
		if ( ! clicked )
		{
			set_property( STR_LEN( PORT "/v/v/title" ),  "Done\n", 5, 1 );
			
			clicked = true;
		}
		else
		{
			CANCEL( frame.r_id );
		}
		
		return 0;
	}
	
	int result = ((request_status*) that)->result = get_u32( frame );
	
	if ( frame.r_id != 0  &&  result == 0 )
	{
		return 0;  // Don't exit the event loop when updates succeed.
	}
	
	if ( frame.type == Frame_result )
	{
		return result == 0 ? 1 : 2;
	}
	
	return 3;
}

static
void read_clicks( const char* path, size_t path_size )
{
	send_read_request( protocol_out, path, path_size );
	
	int nok = wait_for_result( protocol_in, &frame_handler, path );
	
	if ( nok < 0  &&  nok != -ECANCELED )
	{
		report_error( path, -nok );
		
		exit( 1 );
	}
}

int main( int argc, char** argv )
{
	const char** connector_argv = NULL;
	
	if ( const char* gui_path = getenv( "GUI" ) )
	{
		static jack::interface ji = gui_path;
		
		connector_argv = make_unix_connector( ji.socket_path() );
	}
	
	if ( connector_argv == NULL )
	{
		if ( argc < 2 )
		{
			write( STDERR_FILENO, STR_LEN( USAGE ) );
			return 2;
		}
		
		char* address = argv[ 1 ];
		
		connector_argv = parse_address( address );
	}
	
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
		
		PUT( PORT "/.~title", "Demo" );
		
		LINK( "/gui/new/frame",  PORT "/view"   );
		LINK( "/gui/new/button", PORT "/v/view" );
		
		PUT( PORT "/v/height",  "20" "\n" );
		PUT( PORT "/v/padding", "23" "\n" );
		
		PUT( PORT "/v/v/title", "Demo" "\n" );
		
		int window_fd = OPEN( PORT "/window" );
	}
	catch ( const path_error& e )
	{
		report_error( e.path.c_str(), e.error );
		return 1;
	}
	
	read_clicks( STR_LEN( PORT "/v/v/clicked" ) );
	
	return 0;
}
