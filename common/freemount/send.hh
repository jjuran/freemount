/*
	freemount/send.hh
	-----------------
*/

#ifndef FREEMOUNT_SEND_HH
#define FREEMOUNT_SEND_HH

// Standard C
#include <stdint.h>


namespace freemount
{
	
	void send_empty_frame( int fd, uint8_t type, uint8_t r_id = 0 );
	
	void send_empty_request( int fd, uint8_t req_type, uint8_t r_id = 0 );
	
	void send_u8_frame ( int fd, uint8_t type, uint8_t  data, uint8_t r_id = 0 );
	void send_u32_frame( int fd, uint8_t type, uint32_t data, uint8_t r_id = 0 );
	void send_u64_frame( int fd, uint8_t type, uint64_t data, uint8_t r_id = 0 );
	
	void send_string_frame( int fd, uint8_t type, const char* data, uint16_t length, uint8_t r_id = 0 );
	
}

#endif
