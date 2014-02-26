/*
	freemount/server.cc
	-------------------
*/

#include "freemount/server.hh"

// POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// Standard C
#include <stdlib.h>

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
#include "vfs/primitives/listdir.hh"
#include "vfs/primitives/open.hh"
#include "vfs/primitives/stat.hh"

// freemount
#include "freemount/frame_size.hh"
#include "freemount/send.hh"


namespace freemount {

namespace p7 = poseven;


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
		vfs::node_ptr that = vfs::resolve_pathname( root(), path, root() );
		
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
		vfs::node_ptr that = vfs::resolve_pathname( root(), path, root() );
		
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
		vfs::node_ptr that = vfs::resolve_pathname( root(), path, root() );
		
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

int fragment_handler( void* that, const fragment_header& fragment )
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

}  // namespace freemount