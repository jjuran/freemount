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
#include "vfs/filehandle/primitives/pwrite.hh"
#include "vfs/filehandle/primitives/read.hh"
#include "vfs/filehandle/primitives/write.hh"
#include "vfs/functions/resolve_pathname.hh"
#include "vfs/primitives/listdir.hh"
#include "vfs/primitives/open.hh"
#include "vfs/primitives/stat.hh"

// freemount
#include "freemount/data_flow.hh"
#include "freemount/frame_size.hh"
#include "freemount/queue_utils.hh"
#include "freemount/request.hh"
#include "freemount/response.hh"
#include "freemount/send_lock.hh"
#include "freemount/session.hh"
#include "freemount/task.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


namespace freemount {

namespace p7 = poseven;


using ::write;


static int stat( session& s, uint8_t r_id, const request& r )
{
	struct stat sb;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( s.root(), r.path, s.cwd() );
		
		stat( *that, sb );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	send_lock lock;
	
	const mode_t mode = sb.st_mode;
	
	queue_int( s.queue(), Frame_stat_mode, mode, r_id );
	
	if ( S_ISDIR( mode )  &&  sb.st_nlink > 1 )
	{
		queue_int( s.queue(), Frame_stat_nlink, sb.st_nlink, r_id );
	}
	
	if ( S_ISREG( mode ) )
	{
		queue_int( s.queue(), Frame_stat_size, sb.st_size, r_id );
	}
	
	return 0;
}

static int list( session& s, uint8_t r_id, const request& r )
{
	vfs::dir_contents contents;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( s.root(), r.path, s.cwd() );
		
		listdir( *that, contents );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	send_lock lock;
	
	for ( unsigned i = 0;  i < contents.size();  ++i )
	{
		const vfs::dir_entry& entry = contents.at( i );
		
		const plus::string& name = entry.name;
		
		queue_string( s.queue(), Frame_dentry_name, name.data(), name.size(), r_id );
	}
	
	return 0;
}

static int read( session& s, uint8_t r_id, const request& r )
{
	vfs::filehandle_ptr file;
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( s.root(), r.path, s.cwd() );
		
		file = open( *that, O_RDONLY, 0 );
		
		if ( S_ISREG( that->filemode() ) )
		{
			const uint64_t size = geteof( *file );
			
			send_lock lock;
			
			queue_int( s.queue(), Frame_stat_size, size, r_id );
		}
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	int64_t n_requested = r.n;
	
	off_t offset = r.offset;
	
	while ( true )
	{
		char buffer[ 4096 ];
		
		size_t n = sizeof buffer;
		
		if ( n_requested == 0 )
		{
			break;
		}
		else if ( n_requested > 0  &&  n_requested < n )
		{
			n = n_requested;
		}
		
		ssize_t n_read;
		
		try
		{
			if ( offset < 0 )
			{
				n_read = read( *file, buffer, n );
			}
			else
			{
				n_read = pread( *file, buffer, n, offset );
				
				offset += n_read;
			}
		}
		catch ( const p7::errno_t& err )
		{
			return -err;
		}
		
		if ( n_read == 0 )
		{
			break;
		}
		
		if ( n_requested > 0 )
		{
			n_requested -= n_read;
		}
		
		data_transmitting( n_read );
		
		send_lock lock;
		
		queue_string( s.queue(), Frame_recv_data, buffer, n_read, r_id );
		
		s.queue().flush();
	}
	
	return 0;
}

static int write( session& s, uint8_t r_id, const request& r )
{
	ssize_t n_written = -1;
	
	try
	{
		vfs::filehandle_ptr file;
		
		if ( !r.path.empty() )
		{
			vfs::node_ptr that = vfs::resolve_pathname( s.root(), r.path, s.cwd() );
			
			int open_flags = r.offset < 0 ? O_WRONLY | O_CREAT | O_TRUNC
			                              : O_WRONLY;
			
			file = open( *that, open_flags, 0666 );
		}
		else
		{
			return -ENOENT;  // empty pathname
		}
		
		const char* buffer = r.data.data();
		
		const size_t size = r.data.size();
		
		const off_t offset = r.offset;
		
		n_written = offset >= 0 ? pwrite( *file, buffer, size, offset )
		                        : write( *file, buffer, size );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	return 0;
}

static int start_read( session& s, uint8_t r_id, const request& r )
{
	begin_task( &read, s, r_id );
	
	return 1;
}

int frame_handler( void* that, const frame_header& frame )
{
	session& s = *(session*) that;
	
	check_tasks();
	
	switch ( frame.type )
	{
		case Frame_fatal:
			write( STDERR_FILENO, STR_LEN( "[FATAL]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_error:
			write( STDERR_FILENO, STR_LEN( "[ERROR]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_debug:
			write( STDERR_FILENO, STR_LEN( "[DEBUG]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		default:
			break;
	}
	
	if ( frame.type == Frame_ping )
	{
		write( STDERR_FILENO, STR_LEN( "ping\n" ) );
		
		send_lock lock;
		
		queue_empty( s.queue(), Frame_pong );
		
		s.queue().flush();
		
		return 0;
	}
	
	if ( frame.type == Frame_ack_read )
	{
		data_acknowledged( get_u32( frame ) );
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
			
			case req_write:
				write( STDERR_FILENO, STR_LEN( "write..." ) );
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
	
	const char* data = (const char*) get_data( frame );
	
	request& r = *req;
	
	switch ( frame.type )
	{
		case Frame_arg_path:
			switch ( r.type )
			{
				case req_stat:
				case req_list:
				case req_read:
				case req_write:
					break;
				
				default:
					abort();
			}
			
			r.path.assign( data, get_size( frame ) );
			break;
		
		case Frame_send_data:
			r.data.assign( data, get_size( frame ) );
			break;
		
		case Frame_io_count:
			r.n = get_u64( frame );
			break;
		
		case Frame_seek_offset:
			r.offset = get_u64( frame );
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
					err = start_read( s, request_id, r );
					
					if ( err > 0 )
					{
						return 0;  // in progress
					}
					break;
				
				case req_write:
					err = write( s, request_id, r );
					break;
				
				default:
					abort();
			}
			
			s.set_request( request_id, NULL );
			
			send_response( s.queue(), err, request_id );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

}  // namespace freemount
