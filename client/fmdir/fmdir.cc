/*
	fmdir.cc
	--------
*/

// POSIX
#include <unistd.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// iota
#include "iota/endian.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"
#include "freemount/write_in_full.hh"


using namespace freemount;


const int protocol_in  = 6;
const int protocol_out = 7;


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
	write( STDERR_FILENO, "fmdir: ", 7 );
	
	write( STDOUT_FILENO, the_path, strlen( the_path ) );
	
	write( STDERR_FILENO, ": ", 2 );
	
	const char* error = strerror( err );
	
	write( STDOUT_FILENO, error, strlen( error ) );
	
	write( STDERR_FILENO, "\n", 1 );
}

static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_dentry_name:
			write( STDOUT_FILENO, (const char*) (&fragment + 1), iota::u16_from_big( fragment.big_size ) );
			write( STDOUT_FILENO, "\n", 1 );
			break;
		
		case frag_eom:
			exit( 0 );
			break;
		
		case frag_err:
			report_error( u32_from_fragment( fragment ) );
			exit( 1 );
			break;
		
		default:
			write( STDERR_FILENO, "Unfrag\n", 7 );
			
			abort();
	}
	
	return 0;
}

static void send_list_request( const char* path )
{
	fragment_header header = { 0 };
	
	header.type = frag_req;
	header.data = req_list;
	
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
		
	send_list_request( the_path );
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	return looped != 0;
}

