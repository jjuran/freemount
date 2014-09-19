/*
	freemount/send.cc
	-----------------
*/

#include "freemount/send.hh"

// freemount
#include "freemount/frame.hh"
#include "freemount/write_in_full.hh"


namespace freemount
{
	
	void send_empty_frame( int fd, uint8_t type )
	{
		frame_header header = FREEMOUNT_FRAME_HEADER_INITIALIZER;
		
		header.type = type;
		
		write_in_full( fd, &header, sizeof header );
	}
	
}
