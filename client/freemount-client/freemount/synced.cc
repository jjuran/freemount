/*
	freemount/synced.cc
	-------------------
*/

#include "freemount/synced.hh"

// Standard C
#include <errno.h>

// plus
#include "plus/var_string.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/frame_size.hh"
#include "freemount/receiver.hh"
#include "freemount/requests.hh"
#include "freemount/send_ack.hh"


namespace freemount
{
	
	struct request_status
	{
		int               out_fd;
		int32_t           result;
		plus::var_string  data;
		
		request_status( int fd ) : out_fd( fd ), result( -1 )
		{
		}
	};
	
	static
	int wait_for_result( request_status&         req,
	                     int                     fd,
	                     frame_handler_function  handler,
	                     const plus::string&     path )
	{
		data_receiver r( handler, &req );
		
		int looped = run_event_loop( r, fd );
		
		if ( looped < 0  ||  (looped == 0  &&  (looped = -ECONNRESET)) )
		{
			throw connection_error( path, -looped );
		}
		
		const int result = req.result;
		
		if ( looped == 2 )
		{
			throw path_error( path, result );  // errno
		}
		
		return result;
	}
	
	static
	int frame_handler( void* that, const frame_header& frame )
	{
		request_status& req = *(request_status*) that;
		
		if ( frame.type == Frame_stat_size )
		{
			req.data.reserve( get_u64( frame ) );
			return 0;
		}
		
		if ( frame.type == Frame_recv_data )
		{
			send_read_ack( req.out_fd, get_size( frame ) );
			
			req.data.append( get_char_data( frame ), get_size( frame ) );
			return 0;
		}
		
		int result = req.result = get_u32( frame );
		
		if ( frame.type == Frame_result )
		{
			return result == 0 ? 1 : 2;
		}
		
		throw unexpected_frame_type( frame.type );
	}
	
	plus::string synced_get( int in, int out, const plus::string& path )
	{
		send_read_request( out, path.data(), path.size() );
		
		request_status req( out );
		
		const int n = wait_for_result( req, in, &frame_handler, path );
		
		if ( n < 0 )
		{
			throw path_error( path, -n );
		}
		
		return req.data.move();
	}
	
	void synced_put( int          in,
	                 int          out,
	                 const char*  path,
	                 uint32_t     path_size,
	                 const char*  data,
	                 uint32_t     data_size )
	{
		send_write_request( out, path, path_size, data, data_size );
		
		request_status req( out );
		
		const int n = wait_for_result( req, in, &frame_handler, path );
		
		if ( n < 0 )
		{
			throw path_error( path, -n );
		}
	}
	
	int synced_open( int          in,
	                 int          out,
	                 int          chosen_fd,
	                 const char*  path,
	                 uint32_t     path_size )
	{
		send_open_request( out, chosen_fd, path, path_size );
		
		request_status req( out );
		
		const int n = wait_for_result( req, in, &frame_handler, path );
		
		if ( n < 0 )
		{
			throw path_error( path, -n );
		}
		
		return chosen_fd;
	}
	
	void synced_close( int in, int out, int fd )
	{
		send_close_request( out, fd );
		
		request_status req( out );
		
		const char* path = "<close>";
		
		const int n = wait_for_result( req, in, &frame_handler, path );
		
		if ( n < 0 )
		{
			throw path_error( path, -n );
		}
	}
	
	void synced_link( int          in,
	                  int          out,
	                  const char*  src_path,
	                  uint32_t     src_path_size,
	                  const char*  dst_path,
	                  uint32_t     dst_path_size )
	{
		send_link_request( out, src_path, src_path_size,
		                        dst_path, dst_path_size );
		
		request_status req( out );
		
		// FIXME:  Error could pertain to either path.
		
		const int n = wait_for_result( req, in, &frame_handler, src_path );
		
		if ( n < 0 )
		{
			throw path_error( src_path, -n );
		}
	}
	
}
