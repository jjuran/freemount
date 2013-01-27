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
	
}

#endif

