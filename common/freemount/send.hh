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
	
	void send_empty_fragment( int fd, uint8_t type );
	
	void send_empty_request( int fd, uint8_t req_type );
	
	void send_u32_fragment( int fd, uint8_t type, uint32_t data );
	
}

#endif

