/*
	fcat.cc
	-------
*/

// POSIX
#include <unistd.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// iota
#include "iota/endian.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"
#include "freemount/write_in_full.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static const char* the_path;


static uint32_t u32_from_fragment( const fragment_header& fragment )
{
	if ( fragment.big_size != iota::big_u16( sizeof (uint32_t) ) )
	{
		abort();
	}
	
	return iota::u32_from_big( *(uint32_t*) (&fragment + 1) );
}

static void report_error( uint32_t err )
{
	write( STDERR_FILENO, STR_LEN( "fcat: " ) );
	
	write( STDERR_FILENO, the_path, strlen( the_path ) );
	
	write( STDERR_FILENO, STR_LEN( ": " ) );
	
	const char* error = strerror( err );
	
	write( STDERR_FILENO, error, strlen( error ) );
	
	write( STDERR_FILENO, STR_LEN( "\n" ) );
}

static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_io_data:
			write( STDOUT_FILENO, (const char*) (&fragment + 1), iota::u16_from_big( fragment.big_size ) );
			break;
		
		case frag_io_eof:
			break;
		
		case frag_eom:
			exit( 0 );
			break;
		
		case frag_err:
			report_error( u32_from_fragment( fragment ) );
			exit( 1 );
			break;
		
		default:
			write( STDERR_FILENO, STR_LEN( "Unfrag\n" ) );
			
			abort();
	}
	
	return 0;
}

static void send_get_request( const char* path )
{
	fragment_header header = { 0 };
	
	header.type = frag_req;
	header.data = req_read;
	
	write_in_full( protocol_out, &header, sizeof header );
	
	send_string_fragment( protocol_out, frag_file_path, path, strlen( path ) );
	
	send_empty_fragment( protocol_out, frag_eom );
}

int main( int argc, char** argv )
{
	if ( argc <= 1  &&  argv[1][0] != '\0' )
	{
		return 0;
	}
	
	the_path = argv[1];
	
	const char* connector = getenv( "FREEMOUNT_CONNECT" );
	
	if ( connector == NULL )
	{
		connector = getenv( "UNET_CONNECT" );
	}
	
	if ( connector == NULL )
	{
		connector = "unet-connect";
	}
	
	const char* connector_argv[] = { "/bin/sh", "-c", connector, NULL };
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	send_get_request( the_path );
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	return looped != 0;
}

