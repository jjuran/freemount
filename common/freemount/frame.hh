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
		uint8_t   _1;
		uint16_t  big_size;
		
		uint8_t   c_id;  // chain id
		uint8_t   r_id;  // request id
		uint8_t   type;  // frame type
		uint8_t   data;  // request type
	};
	
	#define FREEMOUNT_FRAME_HEADER_INITIALIZER  { 0, 0, 0,  0, 0, 0, 0 }
	
	
	inline const void* get_payload_data( const frame_header& frame )
	{
		return &frame + 1;
	}
	
	inline const void* get_data( const frame_header& frame )
	{
		return &frame + 1;
	}
	
	uint32_t get_u32( const frame_header& frame );
	uint64_t get_u64( const frame_header& frame );
	
	
	enum frame_type
	{
		// control frames
		
		Frame_ping = 0xF1,
		Frame_pong = 0xF0,
		
		// message frames (requests)
		
		Frame_request = 0,
		Frame_submit  = 1,
		
		// message frames (responses)
		
		Frame_result = 64 + 1,
		
		
		frag_file_path = 9,
		
		frag_io_data = 12,
		
		frag_stat_mode = 16,
		frag_stat_nlink = 17,
		frag_stat_size = 18,
		
		frag_dentry_name = 32,
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
