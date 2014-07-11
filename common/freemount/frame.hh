/*
	freemount/frame.hh
	------------------
*/

#ifndef FREEMOUNT_FRAME_HH
#define FREEMOUNT_FRAME_HH

// Standard C
#include <stdint.h>


namespace freemount
{
	
	struct frame_header
	{
		uint8_t   _0;
		uint8_t   type;  // frame type
		uint16_t  big_size;
		
		uint8_t   c_id;  // chain id
		uint8_t   r_id;  // request id
		uint8_t   _6;
		uint8_t   data;  // request type
	};
	
	#define FREEMOUNT_FRAME_HEADER_INITIALIZER  { 0, 0, 0,  0, 0, 0, 0 }
	
	
	inline const void* get_payload_data( const frame_header& frame )
	{
		return &frame + 1;
	}
	
	inline const void* get_data( const frame_header& frame )
	{
		return frame.big_size != 0 ? get_payload_data( frame )
		                           : &frame.data;
	}
	
	uint32_t get_u32( const frame_header& frame );
	uint64_t get_u64( const frame_header& frame );
	
	
	enum frame_type
	{
		// control frames
		
		Frame_fatal = 0xFF,  // fatal error on my end, goodbye
		Frame_error = 0xFE,  // protocol error on your end, goodbye
		Frame_debug = 0xFD,  // debug message, no semantics
		
		Frame_ack_write = 0xF4,  // server acks client request data
		Frame_ack_read  = 0xF3,  // client acks server response data
		
		Frame_ping = 0xF1,
		Frame_pong = 0xF0,
		
		// message frames (requests)
		
		Frame_request = 0,
		Frame_submit  = 1,
		Frame_cancel  = 2,
		
		Frame_arg_path = 4,
		
		Frame_send_data   = 8,
		Frame_io_count    = 9,  // read() limit, write() total
		Frame_seek_offset = 10,
		
		// message frames (responses)
		
		Frame_accept = 64 + 0,
		Frame_result = 64 + 1,
		
		Frame_dentry_name = 64 + 7,
		
		Frame_recv_data = 64 + 8,
		
		Frame_stat_mode  = 64 + 18,
		Frame_stat_nlink = 64 + 19,
		Frame_stat_size  = 64 + 23,
	};
	
	enum request_type
	{
		req_vers = 1,
		req_auth = 2,
		req_stat = 3,
		req_list = 4,
		req_read = 5,
		req_write = 6,
		req_open  = 7,
		req_close = 8,
		req_link = 9,
		
		req_none = 0
	};
	
}


#endif
