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

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/dir_contents.hh"
#include "vfs/dir_entry.hh"
#include "vfs/filehandle.hh"
#include "vfs/node.hh"
#include "vfs/filehandle/primitives/geteof.hh"
#include "vfs/filehandle/primitives/pread.hh"
#include "vfs/filehandle/primitives/read.hh"
#include "vfs/functions/resolve_pathname.hh"
#include "vfs/primitives/listdir.hh"
#include "vfs/primitives/open.hh"
#include "vfs/primitives/stat.hh"

// freemount
#include "freemount/frame_size.hh"
#include "freemount/request.hh"
#include "freemount/session.hh"
#include "freemount/send.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


namespace freemount {

namespace p7 = poseven;


static int stat( session& s, uint8_t r_id, const request& r )
{
	const char* path = r.path.c_str();
	
	struct stat sb;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( s.root(), path, s.cwd() );
		
		stat( *that, sb );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	const mode_t mode = sb.st_mode;
	
	send_u32_frame( s.send_fd, Frame_stat_mode, mode, r_id );
	
	if ( S_ISDIR( mode )  &&  sb.st_nlink > 1 )
	{
		send_u32_frame( s.send_fd, Frame_stat_nlink, sb.st_nlink, r_id );
	}
	
	if ( S_ISREG( mode ) )
	{
		send_u64_frame( s.send_fd, Frame_stat_size, sb.st_size, r_id );
	}
	
	return 0;
}

static int list( session& s, uint8_t r_id, const request& r )
{
	const char* path = r.path.c_str();
	
	vfs::dir_contents contents;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( s.root(), path, s.cwd() );
		
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
		
		send_string_frame( s.send_fd, Frame_dentry_name, name.data(), name.size(), r_id );
	}
	
	return 0;
}

static int read( session& s, uint8_t r_id, const request& r )
{
	const char* path = r.path.c_str();
	
	vfs::filehandle_ptr file;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( s.root(), path, s.cwd() );
		
		file = open( *that, O_RDONLY, 0 );
		
		if ( S_ISREG( that->filemode() ) )
		{
			const uint64_t size = geteof( *file );
			
			send_u64_frame( s.send_fd, Frame_stat_size, size, r_id );
		}
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	while ( true )
	{
		char buffer[ 4096 ];
		
		ssize_t n_read;
		
		try
		{
			n_read = read( *file, buffer, sizeof buffer );
		}
		catch ( const p7::errno_t& err )
		{
			return -err;
		}
		
		if ( n_read == 0 )
		{
			break;
		}
		
		send_string_frame( s.send_fd, Frame_recv_data, buffer, n_read, r_id );
	}
	
	return 0;
}

static void send_response( int fd, int result, uint8_t r_id )
{
	if ( result >= 0 )
	{
		write( STDERR_FILENO, STR_LEN( " ok\n" ) );
		
		send_empty_frame( fd, Frame_result, r_id );
	}
	else
	{
		write( STDERR_FILENO, STR_LEN( " err\n" ) );
		
		send_u32_frame( fd, Frame_result, -result, r_id );
	}
}

int frame_handler( void* that, const frame_header& frame )
{
	session& s = *(session*) that;
	
	if ( frame.type == Frame_ping )
	{
		write( STDERR_FILENO, STR_LEN( "ping\n" ) );
		
		send_empty_frame( s.send_fd, Frame_pong );
		
		return 0;
	}
	
	const uint8_t request_id = frame.r_id;
	
	request* req = s.get_request( request_id );
	
	if ( frame.type == Frame_request )
	{
		switch ( frame.data )
		{
			case req_auth:
				write( STDERR_FILENO, STR_LEN( "auth..." ) );
				break;
			
			case req_stat:
				write( STDERR_FILENO, STR_LEN( "stat..." ) );
				break;
			
			case req_list:
				write( STDERR_FILENO, STR_LEN( "list..." ) );
				break;
			
			case req_read:
				write( STDERR_FILENO, STR_LEN( "read..." ) );
				break;
			
			default:
				abort();
		}
		
		if ( req != NULL )
		{
			write( STDERR_FILENO, STR_LEN( "DUP\n" ) );
			
			abort();
		}
		
		s.set_request( request_id, new request( request_type( frame.data ) ) );
		
		return 0;
	}
	
	if ( req == NULL )
	{
		write( STDERR_FILENO, STR_LEN( "BAD id\n" ) );
		
		abort();
	}
	
	request& r = *req;
	
	switch ( frame.type )
	{
		case Frame_arg_path:
			switch ( r.type )
			{
				case req_stat:
				case req_list:
				case req_read:
					break;
				
				default:
					abort();
			}
			
			r.path.assign( (const char*) get_data( frame ), get_size( frame ) );
			break;
		
		case Frame_submit:
			int err;
			
			err = 0;
			
			switch ( r.type )
			{
				case req_auth:
					break;
				
				case req_stat:
					err = stat( s, request_id, r );
					break;
				
				case req_list:
					err = list( s, request_id, r );
					break;
				
				case req_read:
					err = read( s, request_id, r );
					break;
				
				default:
					abort();
			}
			
			s.set_request( request_id, NULL );
			
			send_response( s.send_fd, err, request_id );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

}  // namespace freemount
