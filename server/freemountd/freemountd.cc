/*
	freemountd.cc
	-------------
*/

// POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/uio.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// Standard C++
#include <map>

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/dir_contents.hh"
#include "vfs/dir_entry.hh"
#include "vfs/filehandle.hh"
#include "vfs/node.hh"
#include "vfs/filehandle/primitives/pread.hh"
#include "vfs/functions/resolve_pathname.hh"
#include "vfs/functions/root.hh"
#include "vfs/node/types/posix.hh"
#include "vfs/primitives/listdir.hh"
#include "vfs/primitives/open.hh"
#include "vfs/primitives/slurp.hh"
#include "vfs/primitives/stat.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/frame_size.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"


namespace p7 = poseven;

using namespace freemount;


static const char* the_native_root_directory = "/var/freemount";


namespace vfs
{
	
	const node* root()
	{
		static node_ptr root = vfs::new_posix_root( the_native_root_directory );
		
		return root.get();
	}
	
}

struct request
{
	request_type type;
	
	plus::string file_path;
};

static std::map< uint8_t, request > the_requests;


static int stat( uint8_t r_id, const request& r )
{
	const char* path = r.file_path.c_str();
	
	struct stat sb;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( path, *vfs::root() );
		
		stat( *that, sb );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	const mode_t mode = sb.st_mode;
	
	send_u32_fragment( STDOUT_FILENO, frag_stat_mode, mode, r_id );
	
	if ( S_ISDIR( mode )  &&  sb.st_nlink > 1 )
	{
		send_u32_fragment( STDOUT_FILENO, frag_stat_nlink, sb.st_nlink, r_id );
	}
	
	if ( S_ISREG( mode ) )
	{
		send_u64_fragment( STDOUT_FILENO, frag_stat_size, sb.st_size, r_id );
	}
	
	return 0;
}

static int list( uint8_t r_id, const request& r )
{
	const char* path = r.file_path.c_str();
	
	vfs::dir_contents contents;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( path, *vfs::root() );
		
		listdir( *that, contents );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	for ( unsigned i = 0;  i < contents.size();  ++i )
	{
		const vfs::dir_entry& entry = contents.at( i );
		
		const plus::string& name = entry.name;
		
		send_string_fragment( STDOUT_FILENO, frag_dentry_name, name.data(), name.size(), r_id );
	}
	
	return 0;
}

static int read( uint8_t r_id, const request& r )
{
	const char* path = r.file_path.c_str();
	
	vfs::filehandle_ptr file;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( path, *vfs::root() );
		
		file = open( *that, O_RDONLY, 0 );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	off_t position = 0;
	
	while ( true )
	{
		char buffer[ 4096 ];
		
		ssize_t n_read;
		
		try
		{
			n_read = pread( *file, buffer, sizeof buffer, position );
		}
		catch ( const p7::errno_t& err )
		{
			return -err;
		}
		
		if ( n_read == 0 )
		{
			break;
		}
		
		send_string_fragment( STDOUT_FILENO, frag_io_data, buffer, n_read, r_id );
		
		position += n_read;
	}
	
	send_empty_fragment( STDOUT_FILENO, frag_io_eof, r_id );
	
	return 0;
}

static void send_response( int result, uint8_t r_id )
{
	if ( result >= 0 )
	{
		write( STDERR_FILENO, " ok\n", 4 );
		
		send_empty_fragment( STDOUT_FILENO, frag_eom, r_id );
	}
	else
	{
		write( STDERR_FILENO, " err\n", 5 );
		
		send_u32_fragment( STDOUT_FILENO, frag_err, -result, r_id );
	}
}

static int fragment_handler( void* that, const fragment_header& fragment )
{
	if ( fragment.type == frag_ping )
	{
		write( STDERR_FILENO, "ping\n", 5 );
		
		send_empty_fragment( STDOUT_FILENO, frag_pong );
		
		return 0;
	}
	
	const uint8_t request_id = fragment.r_id;
	
	typedef std::map< uint8_t, request >::iterator Iter;
	
	const Iter it = the_requests.find( request_id );
	
	if ( fragment.type == frag_req )
	{
		switch ( fragment.data )
		{
			case req_auth:
				write( STDERR_FILENO, "auth...", 7 );
				break;
			
			case req_stat:
				write( STDERR_FILENO, "stat...", 7 );
				break;
			
			case req_list:
				write( STDERR_FILENO, "list...", 7 );
				break;
			
			case req_read:
				write( STDERR_FILENO, "read...", 7 );
				break;
			
			default:
				abort();
		}
		
		if ( it != the_requests.end() )
		{
			write( STDERR_FILENO, "DUP\n", 4 );
			
			abort();
		}
		
		the_requests[ request_id ].type = request_type( fragment.data );
		
		return 0;
	}
	
	if ( it == the_requests.end() )
	{
		write( STDERR_FILENO, "BAD id\n", 7 );
		
		abort();
	}
	
	request& r = it->second;
	
	switch ( fragment.type )
	{
		case frag_file_path:
			switch ( r.type )
			{
				case req_stat:
				case req_list:
				case req_read:
					break;
				
				default:
					abort();
			}
			
			r.file_path.assign( (const char*) get_data( fragment ), get_size( fragment ) );
			break;
		
		case frag_eom:
			int err;
			
			err = 0;
			
			switch ( r.type )
			{
				case req_auth:
					break;
				
				case req_stat:
					err = stat( request_id, r );
					break;
				
				case req_list:
					err = list( request_id, r );
					break;
				
				case req_read:
					err = read( request_id, r );
					break;
				
				default:
					abort();
			}
			
			the_requests.erase( request_id );
			
			send_response( err, request_id );
			break;
		
		default:
			abort();
	}
	
	return 0;
}


#define STR_LEN( s )  "" s, (sizeof s - 1)

#define ARRAYLEN( array )  (sizeof array / sizeof array[0])

static void bad_usage( const char* text, size_t text_size, const char* arg )
{
	const iovec iov[] =
	{
		{ (void*) text, text_size    },
		{ (void*) arg, strlen( arg ) },
		{ (void*) STR_LEN( "\n" )    },
	};
	
	writev( STDERR_FILENO, iov, ARRAYLEN( iov ) );
}

#define BAD_USAGE( text, arg )  (bad_usage( STR_LEN( text ": " ), arg ), (char**) NULL)

static const char* find_char( const char* begin, char c )
{
	while ( *begin != '\0'  &&  *begin != c )
	{
		++begin;
	}
	
	return begin;
}

static bool option_matches( const char*  option,
                            size_t       option_size,
                            const char*  name,
                            size_t       name_size )
{
	return option_size == name_size  &&  memcmp( option, name, name_size ) == 0;
}

#define OPTION_MATCHES( option, size, name )  option_matches( option, size, STR_LEN( name ) )

static char** get_options( char** argv )
{
	if ( *argv == NULL )
	{
		// POSIX says we have to check for this
		return argv;
	}
	
	while ( const char* arg = *++argv )
	{
		if ( arg[0] == '-' )
		{
			if ( arg[1] == '\0' )
			{
				// An "-" argument is not an option and means /dev/fd/0
				break;
			}
			
			if ( arg[1] == '-' )
			{
				// long option or "--"
				
				const char* option = arg + 2;
				
				if ( *option == '\0' )
				{
					++argv;
					break;
				}
				
				const char* equals = find_char( option, '=' );
				
				const size_t size = equals - option;
				
				if ( OPTION_MATCHES( option, size, "root" ) )
				{
					if ( *equals == '\0' )
					{
						++argv;
						
						if ( *argv == NULL )
						{
							return BAD_USAGE( "Argument required", arg );
						}
						
						the_native_root_directory = *argv;
					}
					else
					{
						const char* param = equals + 1;
						
						if ( param[0] == '\0' )
						{
							return BAD_USAGE( "Invalid option", arg );
						}
						
						the_native_root_directory = param;
					}
					
					continue;
				}
				
				return BAD_USAGE( "Unknown option", arg );
			}
			
			// short option
			
			const char* opt = arg + 1;
			
			if ( opt[0] == 'q' )
			{
				int dev_null = open( "/dev/null", O_WRONLY );
				
				dup2( dev_null, STDERR_FILENO );
				
				close( dev_null );
				
				continue;
			}
			
			return BAD_USAGE( "Unknown option", arg );
		}
		
		// not an option
		break;
	}
	
	return argv;
}

int main( int argc, char** argv )
{
	char** params = get_options( argv );
	
	if ( params == NULL )
	{
		return 2;
	}
	
	int n_params = argc - (params - argv);
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, STDIN_FILENO );
	
	return looped != 0;
}

