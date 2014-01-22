/*
	fmls.cc
	-------
*/

// POSIX
#include <unistd.h>
#include <sys/stat.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// Standard C++
#include <map>

// iota
#include "iota/endian.hh"

// gear
#include "gear/inscribe_decimal.hh"

// plus
#include "plus/string/concat.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"
#include "freemount/write_in_full.hh"


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


static plus::string the_path;


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
	
	fragment_header header = { 0 };
	
	header.r_id = r_id;
	header.type = frag_req;
	header.data = req_stat;
	
	write_in_full( protocol_out, &header, sizeof header );
	
	send_string_fragment( protocol_out, frag_file_path, path.data(), path.size(), r_id );
	
	send_empty_fragment( protocol_out, frag_eom, r_id );
}

static void send_list_request( const plus::string& path, uint8_t r_id )
{
	fragment_header header = { 0 };
	
	header.r_id = r_id;
	header.type = frag_req;
	header.data = req_list;
	
	write_in_full( protocol_out, &header, sizeof header );
	
	send_string_fragment( protocol_out, frag_file_path, path.data(), path.size(), r_id );
	
	send_empty_fragment( protocol_out, frag_eom );
}

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

static plus::string string_from_fragment( const fragment_header& fragment )
{
	plus::string result( (const char*) (&fragment + 1),
	                     iota::u16_from_big( fragment.big_size ) );
	
	return result;
}

static void report_error( uint32_t err )
{
	write( STDERR_FILENO, "fmls: ", 7 );
	
	write( STDERR_FILENO, the_path.data(), the_path.size() );
	
	write( STDERR_FILENO, ": ", 2 );
	
	const char* error = strerror( err );
	
	write( STDERR_FILENO, error, strlen( error ) );
	
	write( STDERR_FILENO, "\n", 1 );
}

static int fragment_handler( void* that, const fragment_header& fragment )
{
	const uint8_t request_id = fragment.r_id;
	
	typedef std::map< uint8_t, stat_response >::iterator Iter;
	
	const Iter it = the_stats.find( request_id );
	
	if ( it != the_stats.end() )
	{
		stat_response& st = it->second;
		
		switch ( fragment.type )
		{
			case frag_stat_mode:
				st.mode = u32_from_fragment( fragment );
				break;
			
			case frag_stat_nlink:
				st.nlink = u32_from_fragment( fragment );
				break;
			
			case frag_stat_size:
				st.size = u64_from_fragment( fragment );
				break;
			
			case frag_eom:
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
						exit( 0 );
					}
				}
				
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
	
	switch ( fragment.type )
	{
		case frag_dentry_name:
			if ( const uint8_t id = next_id() )
			{
				const plus::string name = string_from_fragment( fragment );
				
				send_stat_request( the_path + name, id );
				
				the_stats[ id ].path = name;
			}
			else
			{
				write( STDOUT_FILENO, (const char*) (&fragment + 1), iota::u16_from_big( fragment.big_size ) );
				write( STDOUT_FILENO, ": skipped\n", 10 );
			}
			
			break;
		
		case frag_eom:
			if ( the_stats.empty() )
			{
				exit( 0 );
			}
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

int main( int argc, char** argv )
{
	if ( argc <= 1  &&  argv[1][0] != '\0' )
	{
		return 0;
	}
	
	the_path.assign( argv[1], strlen( argv[1] ), plus::delete_never );
	
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
	
	send_stat_request( the_path, 0 );
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	return looped != 0;
}

