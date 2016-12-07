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
#include <stdio.h>
#include <stdlib.h>

// poseven
#include "poseven/types/errno_t.hh"

// vfs
#include "vfs/dir_contents.hh"
#include "vfs/dir_entry.hh"
#include "vfs/filehandle.hh"
#include "vfs/filehandle_ptr.hh"
#include "vfs/node.hh"
#include "vfs/filehandle/primitives/geteof.hh"
#include "vfs/filehandle/primitives/pread.hh"
#include "vfs/filehandle/primitives/pwrite.hh"
#include "vfs/filehandle/primitives/read.hh"
#include "vfs/filehandle/primitives/write.hh"
#include "vfs/functions/resolve_pathname.hh"
#include "vfs/primitives/hardlink.hh"
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


#define ARRAY_LEN( a )  (sizeof a / sizeof a[0])

#define STR_LEN( s )  "" s, (sizeof s - 1)


namespace freemount {

namespace p7 = poseven;


using ::write;


static
int stat( session& s, uint8_t r_id, const request& r )
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

static
int list( session& s, uint8_t r_id, const request& r )
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

static
int open( session& s, uint8_t r_id, const request& r )
{
	int fd = r.fd;
	
	if ( unsigned( fd ) > 255 )
	{
		return -EBADF;
	}
	
	try
	{
		vfs::node_ptr that = vfs::resolve_pathname( s.root(), r.path, s.cwd() );
		
		vfs::filehandle_ptr file = open( *that, O_WRONLY | O_CREAT, 0 );
		
		s.set_open_file( fd, file.get() );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	return 0;
}

static
int close( session& s, uint8_t r_id, const request& r )
{
	int fd = r.fd;
	
	if ( s.get_open_file( fd ) == NULL )
	{
		return -EBADF;
	}
	
	s.set_open_file( fd, NULL );
	
	return 0;
}

static
int read( session& s, uint8_t r_id, const request& r )
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

static
int write( session& s, uint8_t r_id, const request& r )
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

static
int link( session& s, uint8_t r_id, const request& r )
{
	const plus::string& path_1 = r.path;
	const plus::string& path_2 = r.data;
	
	try
	{
		vfs::node_ptr that_1 = vfs::resolve_pathname( s.root(), path_1, s.cwd() );
		vfs::node_ptr that_2 = vfs::resolve_pathname( s.root(), path_2, s.cwd() );
		
		hardlink( *that_1, *that_2 );
	}
	catch ( const p7::errno_t& err )
	{
		return -err;
	}
	
	return 0;
}

static
int start_read( session& s, uint8_t r_id, const request& r )
{
	begin_task( &read, s, r_id );
	
	return 1;
}

enum arg_mask
{
	Mask_req    = (1 << Frame_request) | (1 << Frame_submit),
	Mask_path   = 1 << Frame_arg_path,
	Mask_fd     = 1 << Frame_arg_fd,
	
	Mask_data   = 1 << Frame_send_data,
	Mask_count  = 1 << Frame_io_count,
	Mask_offset = 1 << Frame_seek_offset,
};

static const char* arg_names[] =
{
	"request",
	"submit",
	"cancel",
	NULL,
	"path",
	NULL,
	"fd",
	NULL,
	"sent data",
	"I/O byte count",
	"seek offset",
};

static
const char* name_of_arg( uint8_t type )
{
	if ( type >= ARRAY_LEN( arg_names ) )
	{
		return NULL;
	}
	
	return arg_names[ type ];
}

struct request_desc
{
	const char*  name;
	req_func     handler;
	uint64_t     arg_mask;
};

static request_desc request_descs[] =
{
	{ 0 },
	{ "vers" },
	{ "auth" },
	{ "stat",  &stat,       Mask_req | Mask_path },
	{ "list",  &list,       Mask_req | Mask_path },
	{ "read",  &start_read, Mask_req | Mask_path | Mask_count | Mask_offset },
	{ "write", &write,      Mask_req | Mask_path | Mask_data  | Mask_offset },
	{ "open",  &open,       Mask_req | Mask_path | Mask_fd },
	{ "close", &close,      Mask_req | Mask_fd },
	{ "link",  &link,       Mask_req | Mask_path },
};

static inline
bool request_is_implemented( uint8_t req_type )
{
	if ( req_type >= ARRAY_LEN( request_descs ) )
	{
		return false;
	}
	
	return request_descs[ req_type ].handler != NULL;
}

int frame_handler( void* that, const frame_header& frame )
{
	session& s = *(session*) that;
	
	s.check_tasks();
	
	switch ( frame.type )
	{
		case Frame_fatal:
			write( STDERR_FILENO, STR_LEN( "[FATAL]: " ) );
			write( STDERR_FILENO, get_char_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_error:
			write( STDERR_FILENO, STR_LEN( "[ERROR]: " ) );
			write( STDERR_FILENO, get_char_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_debug:
			write( STDERR_FILENO, STR_LEN( "[DEBUG]: " ) );
			write( STDERR_FILENO, get_char_data( frame ), get_size( frame ) );
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
	
	const char* arg_name = name_of_arg( frame.type );
	
	if ( arg_name == NULL )
	{
		fprintf( stderr, "Invalid arg type %d\n", frame.type );
		return -EINVAL;
	}
	
	const uint8_t request_id = frame.r_id;
	
	request* req = s.get_request( request_id );
	
	if ( frame.type == Frame_request )
	{
		const request_type req_type = request_type( frame.data );
		
		if ( ! request_is_implemented( req_type ) )
		{
			fprintf( stderr, "Unimplemented request type %d\n", req_type );
			return -ENOSYS;
		}
		
		const request_desc& desc = request_descs[ req_type ];
		
		fprintf( stderr, "New %s request, id %d\n", desc.name, request_id );
		
		if ( req != NULL )
		{
			fprintf( stderr, "Duplicate request id %d\n", request_id );
			return -EEXIST;
		}
		
		s.set_request( request_id, new request( request_type( frame.data ) ) );
		
		return 0;
	}
	
	if ( req == NULL )
	{
		fprintf( stderr, "Nonexistent request id %d\n", request_id );
		return -ESRCH;
	}
	
	const char* data = get_char_data( frame );
	
	request& r = *req;
	
	const request_desc& desc = request_descs[ r.type ];
	
	if ( ! (desc.arg_mask & (1 << frame.type)) )
	{
		const char* name = desc.name;
		
		fprintf( stderr, "Invalid %s arg for %s request\n", arg_name, name );
		return -EINVAL;
	}
	
	switch ( frame.type )
	{
		case Frame_arg_path:
			{
				plus::string& s = r.path.empty() ? r.path : r.data;
				
				s.assign( data, get_size( frame ) );
				
				fprintf( stderr, "Path: \"%s\"\n", s.c_str() );
			}
			break;
		
		case Frame_arg_fd:
			r.fd = get_u32( frame );
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
			
			err = desc.handler( s, request_id, r );
			
			if ( err > 0 )
			{
				return 0;  // in progress
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
