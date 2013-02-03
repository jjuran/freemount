/*
	freemount/send.cc
	-----------------
*/

#include "freemount/send.hh"

// freemount
#include "freemount/fragment.hh"
#include "freemount/write_in_full.hh"


namespace freemount
{
	
	void send_empty_fragment( int fd, uint8_t type )
	{
		fragment_header header = FREEMOUNT_FRAGMENT_HEADER_INITIALIZER;
		
		header.type = type;
		
		write_in_full( fd, &header, sizeof header );
	}
	
}

