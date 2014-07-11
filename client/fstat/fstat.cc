/*
	fstat.cc
	--------
*/

// POSIX
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// more-posix
#include "more/perror.hh"

// gear
#include "gear/inscribe_decimal.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"

// freemount-client
#include "freemount/client.hh"
#include "freemount/requests.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static const char* the_path;

static int the_result;


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

static int frame_handler( void* that, const frame_header& frame )
{
	switch ( frame.type )
	{
		case frag_stat_mode:
			print_mode( get_u32( frame ) );
			break;
		
		case frag_stat_nlink:
			print_number( get_u32( frame ) );
			break;
		
		case frag_stat_size:
			print_number( get_u64( frame ) );
			break;
		
		case Frame_result:
		case frag_eom:
		case frag_err:
			the_result = get_u32( frame );
			
			shutdown( protocol_out, SHUT_WR );
			break;
		
		default:
			write( STDERR_FILENO, STR_LEN( "Unfrag\n" ) );
			
			abort();
	}
	
	return 0;
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
	
	send_stat_request( protocol_out, the_path, strlen( the_path ) );
	
	data_receiver r( &frame_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	if ( looped < 0 )
	{
		more::perror( "fstat", -looped );
		
		return 1;
	}
	
	if ( the_result != 0 )
	{
		more::perror( "fstat", the_path, the_result );
		
		return 1;
	}
	
	write( STDOUT_FILENO, STR_LEN( "  " ) );
	write( STDOUT_FILENO, the_path, strlen( the_path ) );
	write( STDOUT_FILENO, STR_LEN( "\n" ) );
	
	return 0;
}
