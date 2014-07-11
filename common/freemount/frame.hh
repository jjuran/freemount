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
	
	
	inline const void* get_data( const frame_header& frame )
	{
		return &frame + 1;
	}
	
	uint32_t get_u32( const frame_header& frame );
	uint64_t get_u64( const frame_header& frame );
	
	
	enum frame_type
	{
		frag_ping = 1,
		frag_pong = 2,
		
		frag_req = 3,
		frag_eom = 4,
		frag_err = 5,
		
		frag_file_path = 9,
		frag_file_fd = 10,
		
		frag_io_count = 11,
		frag_io_data = 12,
		frag_io_eof = 13,
		
		frag_buffer_size = 14,
		frag_seek_offset = 15,
		
		frag_stat_mode = 16,
		frag_stat_nlink = 17,
		frag_stat_size = 18,
		
		frag_flags = 24,
		frag_create_mode = frag_stat_mode,
		
		frag_dentry_name = 32,
		
		frag_none = 0
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
