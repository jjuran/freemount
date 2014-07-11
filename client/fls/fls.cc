/*
	fls.cc
	------
*/

// POSIX
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// Standard C++
#include <map>

// more-posix
#include "more/perror.hh"

// gear
#include "gear/inscribe_decimal.hh"

// plus
#include "plus/string/concat.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/frame_size.hh"
#include "freemount/receiver.hh"

// freemount-client
#include "freemount/client.hh"
#include "freemount/requests.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static plus::string the_path = "/";

static int the_result;


struct stat_response
{
	plus::string  path;
	mode_t        mode;
	uint32_t      nlink;
	uint64_t      size;
};

static std::map< uint8_t, stat_response > the_stats;

static uint8_t next_id()
{
	uint8_t last = 0;
	
	typedef std::map< uint8_t, stat_response >::const_iterator Iter;
	
	const Iter end = the_stats.end();
	
	for ( Iter it = the_stats.begin();  it != end;  ++it )
	{
		if ( it->first != ++last )
		{
			return last;
		}
	}
	
	return ++last;  // overflows to zero if no ids left
}

static plus::string format_mode( uint32_t mode )
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
	
	return mode_string;
}

static void append_number( plus::var_string& result, uint64_t number, int n_columns = 0 )
{
	const int magnitude = gear::magnitude< 10 >( number );
	
	char buffer[] = "          ";
	
	const size_t size = sizeof buffer - 1;
	
	if ( n_columns > size )
	{
		n_columns = size;
	}
	
	if ( magnitude >= n_columns )
	{
		result += gear::inscribe_unsigned_wide_decimal( number );
	}
	else
	{
		const int n_padding = n_columns - magnitude;
		
		char* end   = buffer + size;
		char* begin = end - magnitude;
		
		gear::fill_unsigned< 10 >( number, begin, end );
		
		result += end - n_columns;
	}
}

static void print( const stat_response& st )
{
	plus::var_string result = format_mode( st.mode );
	
	result += "  ";
	
	append_number( result, st.nlink, 3 );
	
	result += "  ";
	
	append_number( result, st.size, 9 );
	
	result += "  ";
	
	result += st.path;
	
	result += "\n";
	
	write( STDOUT_FILENO, result.data(), result.size() );
}

static void send_stat_request( const plus::string& path, uint8_t r_id )
{
	stat_response& st = the_stats[ r_id ];
	
	st.path = path;
	st.mode = 0;
	st.nlink = 1;
	st.size = 0;
	
	send_stat_request( protocol_out, path.data(), path.size(), r_id );
}

static void send_list_request( const plus::string& path, uint8_t r_id )
{
	send_list_request( protocol_out, path.data(), path.size(), r_id );
}

static plus::string string_from_frame( const frame_header& frame )
{
	plus::string result( (const char*) get_data( frame ), get_size( frame ) );
	
	return result;
}

static int frame_handler( void* that, const frame_header& frame )
{
	const uint8_t request_id = frame.r_id;
	
	typedef std::map< uint8_t, stat_response >::iterator Iter;
	
	const Iter it = the_stats.find( request_id );
	
	if ( it != the_stats.end() )
	{
		stat_response& st = it->second;
		
		switch ( frame.type )
		{
			case frag_stat_mode:
				st.mode = get_u32( frame );
				break;
			
			case frag_stat_nlink:
				st.nlink = get_u32( frame );
				break;
			
			case frag_stat_size:
				st.size = get_u64( frame );
				break;
			
			case Frame_result:
			case frag_eom:
			case frag_err:
				if ( int err = get_u32( frame ) )
				{
					the_result = err;
					
					shutdown( protocol_out, SHUT_WR );
					
					break;
				}
				
				if ( request_id == 0  &&  S_ISDIR( st.mode ) )
				{
					the_stats.erase( 0 );
					
					send_list_request( the_path, 0 );
					
					if ( the_path.back() != '/' )
					{
						the_path = the_path + "/";
					}
				}
				else
				{
					print( st );
					
					the_stats.erase( request_id );
					
					if ( the_stats.empty() )
					{
						shutdown( protocol_out, SHUT_WR );
					}
				}
				
				break;
			
			default:
			write( STDERR_FILENO, STR_LEN( "Unfrag\n" ) );
				
				abort();
		}
		
		return 0;
	}
	
	switch ( frame.type )
	{
		case frag_dentry_name:
			if ( const uint8_t id = next_id() )
			{
				const plus::string name = string_from_frame( frame );
				
				send_stat_request( the_path + name, id );
				
				the_stats[ id ].path = name;
			}
			else
			{
				write( STDOUT_FILENO, get_data( frame ), get_size( frame ) );
				write( STDOUT_FILENO, STR_LEN( ": skipped\n" ) );
			}
			
			break;
		
		case Frame_result:
		case frag_eom:
		case frag_err:
			if ( int err = get_u32( frame ) )
			{
				the_result = err;
			}
			
			if ( the_result != 0  ||  the_stats.empty() )
			{
				shutdown( protocol_out, SHUT_WR );
			}
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
	
	const char* path = connector_argv[ -1 ];
	
	if ( path != NULL )
	{
		the_path.assign( path, strlen( path ), plus::delete_never );
	}
	
	send_stat_request( the_path, 0 );
	
	data_receiver r( &frame_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	if ( looped < 0 )
	{
		more::perror( "fls", -looped );
		
		return 1;
	}
	
	if ( the_result != 0 )
	{
		more::perror( "fls", the_path.c_str(), the_result );
		
		return 1;
	}
	
	return 0;
}
