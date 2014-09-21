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
	
	static inline void queue_request( send_queue& queue, uint8_t request_type )
	{
		queue_int( queue, Frame_request, request_type );
	}
	
	static inline void queue_submit( send_queue& queue )
	{
		queue_empty( queue, Frame_submit );
	}
	
	void send_path_request( int fd, const char* path, uint32_t size, uint8_t r_type, uint8_t r_id )
	{
		send_queue queue( fd );
		
		queue_request( queue, r_type );
		
		queue_string( queue, Frame_arg_path, path, size );
		
		queue_submit( queue );
		
		queue.flush();
	}
	
}
