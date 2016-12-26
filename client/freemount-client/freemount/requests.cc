/*
	freemount/requests.cc
	---------------------
*/

#include "freemount/requests.hh"

// freemount
#include "freemount/queue_utils.hh"
#include "freemount/send_queue.hh"


namespace freemount
{
	
	static inline
	void queue_request( send_queue& queue, uint8_t request_type, uint8_t r_id )
	{
		queue_int( queue, Frame_request, request_type, r_id );
	}
	
	static inline
	void queue_submit( send_queue& queue, uint8_t r_id )
	{
		queue_empty( queue, Frame_submit, r_id );
	}
	
	static inline
	void queue_cancel( send_queue& queue, uint8_t r_id )
	{
		queue_empty( queue, Frame_cancel, r_id );
	}
	
	void cancel_request( int fd, uint8_t r_id )
	{
		send_queue queue( fd );
		
		queue_cancel( queue, r_id );
		
		queue.flush();
	}
	
	void send_path_request( int          fd,
	                        const char*  path,
	                        uint32_t     size,
	                        uint8_t      r_type,
	                        uint8_t      r_id )
	{
		send_queue queue( fd );
		
		queue_request( queue, r_type, r_id );
		
		queue_string( queue, Frame_arg_path, path, size, r_id );
		
		queue_submit( queue, r_id );
		
		queue.flush();
	}
	
	void send_write_request( int          fd,
	                         const char*  path,
	                         uint32_t     path_size,
	                         const char*  data,
	                         uint32_t     data_size,
	                         uint8_t      r_id )
	{
		send_queue queue( fd );
		
		queue_request( queue, req_write, r_id );
		
		queue_string( queue, Frame_arg_path,  path, path_size, r_id );
		queue_string( queue, Frame_send_data, data, data_size, r_id );
		
		queue_submit( queue, r_id );
		
		queue.flush();
	}
	
	void send_pwrite_request( int          fd,
	                          uint32_t     offset,
	                          const char*  path,
	                          uint32_t     path_size,
	                          const char*  data,
	                          uint32_t     data_size,
	                          uint8_t      r_id )
	{
		send_queue queue( fd );
		
		queue_request( queue, req_write, r_id );
		
		queue_int( queue, Frame_seek_offset, offset, r_id );
		
		queue_string( queue, Frame_arg_path,  path, path_size, r_id );
		queue_string( queue, Frame_send_data, data, data_size, r_id );
		
		queue_submit( queue, r_id );
		
		queue.flush();
	}
	
	void send_open_request( int          fd,
	                        int          chosen_fd,
	                        const char*  path,
	                        uint32_t     path_size,
	                        uint8_t      r_id )
	{
		send_queue queue( fd );
		
		queue_request( queue, req_open, r_id );
		
		if ( chosen_fd >= 0 )
		{
			queue_int( queue, Frame_arg_fd, chosen_fd, r_id );
		}
		
		queue_string( queue, Frame_arg_path, path, path_size, r_id );
		
		queue_submit( queue, r_id );
		
		queue.flush();
	}
	
	void send_close_request( int fd, int file_fd, uint8_t r_id )
	{
		send_queue queue( fd );
		
		queue_request( queue, req_close, r_id );
		
		queue_int( queue, Frame_arg_fd, file_fd, r_id );
		
		queue_submit( queue, r_id );
		
		queue.flush();
	}
	
	void send_link_request( int          fd,
	                        const char*  src_path,
	                        uint32_t     src_path_size,
	                        const char*  dst_path,
	                        uint32_t     dst_path_size,
	                        uint8_t      r_id )
	{
		send_queue queue( fd );
		
		queue_request( queue, req_link, r_id );
		
		queue_string( queue, Frame_arg_path, src_path, src_path_size, r_id );
		queue_string( queue, Frame_arg_path, dst_path, dst_path_size, r_id );
		
		queue_submit( queue, r_id );
		
		queue.flush();
	}
	
}
