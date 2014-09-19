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
	
}

#endif
