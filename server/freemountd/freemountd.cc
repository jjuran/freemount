/*
	freemountd.cc
	-------------
*/

// POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// standard C
#include <stdlib.h>

// iota
#include "iota/endian.hh"

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/dir_entry.hh"
#include "vfs/dir_contents_impl.hh"
#include "vfs/node.hh"
#include "vfs/functions/resolve_pathname.hh"
#include "vfs/functions/root.hh"
#include "vfs/node/types/posix.hh"
#include "vfs/primitives/listdir.hh"
#include "vfs/primitives/slurp.hh"
#include "vfs/primitives/stat.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"


namespace p7 = poseven;

using namespace freemount;


static const char* the_native_root_directory = "/";

namespace vfs
{
	
	const node* root()
	{
		static node_ptr root = vfs::new_posix_root( the_native_root_directory );
		
		return root.get();
	}
	
}

static request_type pending_request_type = req_none;

static plus::string file_path;


static int stat()
{
	const char* path = file_path.c_str();
	
	struct stat sb;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( path, vfs::root() );
		
		stat( that.get(), sb );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	const mode_t mode = sb.st_mode;
	
	send_u32_fragment( STDOUT_FILENO, frag_stat_mode, mode );
	
	if ( S_ISDIR( mode )  &&  sb.st_nlink > 1 )
	{
		send_u32_fragment( STDOUT_FILENO, frag_stat_nlink, sb.st_nlink );
	}
	
	if ( S_ISREG( mode ) )
	{
		send_u64_fragment( STDOUT_FILENO, frag_stat_size, sb.st_size );
	}
	
	return 0;
}

static int list()
{
	const char* path = file_path.c_str();
	
	vfs::dir_contents_impl contents;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( path, vfs::root() );
		
		listdir( that.get(), contents );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	for ( unsigned i = 0;  i < contents.size();  ++i )
	{
		const vfs::dir_entry& entry = contents.at( i );
		
		const plus::string& name = entry.name;
		
		send_string_fragment( STDOUT_FILENO, frag_dentry_name, name.data(), name.size() );
	}
	
	return 0;
}

static int read()
{
	const char* path = file_path.c_str();
	
	plus::string contents;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( path, vfs::root() );
		
		contents = slurp( that.get() );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	const char* p   =     contents.data();
	const char* end = p + contents.size();
	
	while ( end - p >= 4096 )
	{
		send_string_fragment( STDOUT_FILENO, frag_io_data, p, 4096 );
		
		p += 4096;
	}
	
	if ( end - p > 0 )
	{
		send_string_fragment( STDOUT_FILENO, frag_io_data, p, end - p );
	}
	
	send_empty_fragment( STDOUT_FILENO, frag_io_eof );
	
	return 0;
}

static void send_response( int result )
{
	if ( result >= 0 )
	{
		write( STDERR_FILENO, " ok\n", 4 );
		
		send_empty_fragment( STDOUT_FILENO, frag_eom );
	}
	else
	{
		write( STDERR_FILENO, " err\n", 5 );
		
		send_u32_fragment( STDOUT_FILENO, frag_err, -result );
	}
}

static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_ping:
			write( STDERR_FILENO, "ping\n", 5 );
			
			send_empty_fragment( STDOUT_FILENO, frag_pong );
			break;
		
		case frag_req:
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
			
			pending_request_type = request_type( fragment.data );
			
			break;
		
		case frag_file_path:
			switch ( pending_request_type )
			{
				case req_stat:
				case req_list:
				case req_read:
					break;
				
				default:
					abort();
			}
			
			file_path.assign( (const char*) (&fragment + 1), iota::u16_from_big( fragment.big_size ) );
			break;
		
		case frag_eom:
			int err;
			
			err = 0;
			
			switch ( pending_request_type )
			{
				case req_auth:
					break;
				
				case req_stat:
					err = stat();
					break;
				
				case req_list:
					err = list();
					break;
				
				case req_read:
					err = read();
					break;
				
				default:
					abort();
			}
			
			pending_request_type = req_none;
			
			send_response( err );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

int main( int argc, char** argv )
{
	if ( argc > 1 )
	{
		the_native_root_directory = argv[1];
	}
	
	data_receiver r( &fragment_handler, NULL );
	
	int looped = run_event_loop( r, STDIN_FILENO );
	
	return looped != 0;
}

