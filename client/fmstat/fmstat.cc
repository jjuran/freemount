/*
	fmstat.cc
	---------
*/

// POSIX
#include <unistd.h>
#include <sys/stat.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// iota
#include "iota/endian.hh"

// gear
#include "gear/inscribe_decimal.hh"

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

static uint32_t u64_from_fragment( const fragment_header& fragment )
{
	if ( fragment.big_size != iota::big_u16( sizeof (uint64_t) ) )
	{
		abort();
	}
	
	return iota::u64_from_big( *(uint64_t*) (&fragment + 1) );
}

static void print_mode( uint32_t mode )
{
	char mode_string[] = "?rwxrwxrwx";
	
	if ( S_ISREG( mode ) )
	{
		mode_string[0] = '-';
	}
	else if ( S_ISDIR( mode ) )
	{
		mode_string[0] = 'd';
	}
	else if ( S_ISLNK( mode ) )
	{
		mode_string[0] = 'l';
	}
	else if ( S_ISCHR( mode ) )
	{
		mode_string[0] = 'c';
	}
	else if ( S_ISBLK( mode ) )
	{
		mode_string[0] = 'b';
	}
	else if ( S_ISFIFO( mode ) )
	{
		mode_string[0] = 'p';
	}
	else if ( S_ISSOCK( mode ) )
	{
		mode_string[0] = 's';
	}
	
	for ( int i = 8;  i >= 0;  --i )
	{
		if ( mode & (1 << i) )
		{
			// do nothing
		}
		else
		{
			// 8..0 -> 1..9
			mode_string[ 9 - i ] = '-';
		}
	}
	
	if ( mode & 01000 )
	{
		mode_string[ 9 ] = 't';
	}
	
	write( STDOUT_FILENO, mode_string, sizeof mode_string - 1 );
}

static void print_number( uint64_t number )
{
	write( STDOUT_FILENO, "  ", 2 );
	
	const char* decimal = gear::inscribe_unsigned_wide_decimal( number );
	
	write( STDOUT_FILENO, decimal, strlen( decimal ) );
}

static void report_error( uint32_t err )
{
	write( STDERR_FILENO, STR_LEN( "fmstat: " ) );
	
	write( STDOUT_FILENO, the_path, strlen( the_path ) );
	
	write( STDERR_FILENO, STR_LEN( ": " ) );
	
	const char* error = strerror( err );
	
	write( STDERR_FILENO, error, strlen( error ) );
	
	write( STDERR_FILENO, STR_LEN( "\n" ) );
}

static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_stat_mode:
			print_mode( u32_from_fragment( fragment ) );
			break;
		
		case frag_stat_nlink:
			print_number( u32_from_fragment( fragment ) );
			break;
		
		case frag_stat_size:
			print_number( u64_from_fragment( fragment ) );
			break;
		
		case frag_eom:
			write( STDERR_FILENO, STR_LEN( "  " ) );
			write( STDOUT_FILENO, the_path, strlen( the_path ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
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

static void send_stat_request( const char* path )
{
	fragment_header header = { 0 };
	
	header.type = frag_req;
	header.data = req_stat;
	
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
	
	send_stat_request( the_path );
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	return looped != 0;
}

