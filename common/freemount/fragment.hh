/*
	freemount/fragment.hh
	---------------------
*/

#ifndef FREEMOUNT_FRAGMENT_HH
#define FREEMOUNT_FRAGMENT_HH

// Standard C
#include <stdint.h>


namespace freemount
{
	
	struct fragment_header
	{
		uint8_t   _0;
		uint8_t   _1;
		uint16_t  big_size;
		
		uint8_t   c_id;  // chain id
		uint8_t   r_id;  // request id
		uint8_t   type;  // fragment type
		uint8_t   data;  // request type
	};
	
	#define FREEMOUNT_FRAGMENT_HEADER_INITIALIZER  { 0, 0, 0,  0, 0, 0, 0 }
	
	enum fragment_type
	{
		frag_ping = 1,
		frag_pong = 2,
		
		frag_req = 3,
		frag_eom = 4,
		
		frag_none = 0
	};
	
	enum request_type
	{
		req_vers = 1,
		req_auth = 2,
		
		req_none = 0
	};
	
}


#endif

