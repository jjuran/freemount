/*
	fstat.cc
	--------
*/

// POSIX
#include <unistd.h>
#include <sys/stat.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// gear
#include "gear/inscribe_decimal.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"
#include "freemount/write_in_full.hh"

// freemount-client
#include "freemount/client.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static const char* the_path;


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
	write( STDERR_FILENO, STR_LEN( "fstat: " ) );
	
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
			print_mode( get_u32( fragment ) );
			break;
		
		case frag_stat_nlink:
			print_number( get_u32( fragment ) );
			break;
		
		case frag_stat_size:
			print_number( get_u64( fragment ) );
			break;
		
		case frag_eom:
			write( STDERR_FILENO, STR_LEN( "  " ) );
			write( STDOUT_FILENO, the_path, strlen( the_path ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			exit( 0 );
			break;
		
		case frag_err:
			report_error( get_u32( fragment ) );
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
	
	send_stat_request( the_path );
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	return looped != 0;
}

