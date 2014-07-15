/*
	fget.cc
	-------
*/

// POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

// Standard C
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// more-posix
#include "more/perror.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/frame_size.hh"
#include "freemount/receiver.hh"
#include "freemount/write_in_full.hh"

// freemount-client
#include "freemount/client.hh"
#include "freemount/requests.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;

static int output_fd = -1;


static const char* the_path;

static uint64_t the_expected_size;
static uint64_t n_written;

static int the_result;


static int frame_handler( void* that, const frame_header& frame )
{
	switch ( frame.type )
	{
		case Frame_stat_size:
			the_expected_size = get_u64( frame );
			break;
		
		case Frame_recv_data:
			uint16_t size;
			size = get_size( frame );
			
			write_in_full( output_fd, get_data( frame ), size );
			
			n_written += size;
			break;
		
		case Frame_result:
			the_result = get_u32( frame );
			
			shutdown( protocol_out, SHUT_WR );
			break;
		
		default:
			write( STDERR_FILENO, STR_LEN( "Unfrag\n" ) );
			
			abort();
	}
	
	return 0;
}

static const char* name_from_path( const char* path )
{
	if ( const char* slash = strrchr( path, '/' ) )
	{
		path = slash + 1;
	}
	
	if ( *path == '\0' )
	{
		return NULL;
	}
	
	return path;
}

static void open_file( const char* name )
{
	output_fd = open( name, O_WRONLY|O_CREAT|O_EXCL, 0666 );
	
	if ( output_fd < 0 )
	{
		more::perror( "fget", name, errno );
		
		exit( 1 );
	}
}

int main( int argc, char** argv )
{
	if ( argc <= 1  ||  argv[1][0] == '\0' )
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
	
	the_path = connector_argv[ -1 ];
	
	if ( the_path == NULL )
	{
		the_path = "/";
	}
	
	const char* name = name_from_path( the_path );
	
	if ( name == NULL )
	{
		return 2;  // path doesn't end with a non-slash, ergo not a file
	}
	
	open_file( name );
	
	send_read_request( protocol_out, the_path, strlen( the_path ) );
	
	data_receiver r( &frame_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	int nok = close( output_fd );
	
	if ( nok )
	{
		more::perror( "fget", name, errno );
		
		return 1;
	}
	
	if ( looped < 0 )
	{
		more::perror( "fget", -looped );
		
		return 1;
	}
	
	if ( the_result != 0 )
	{
		more::perror( "fget", name, the_result );
		
		return 1;
	}
	
	if ( n_written != the_expected_size )
	{
		
		fprintf( stderr,
		         "fget: %s: Transferred size of %llu doesn't match expected size of %llu\n",
		         name,
		         n_written,
		         the_expected_size );
		
		return 1;
	}
	
	return 0;
}
