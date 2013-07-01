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
	
	void send_empty_request( int fd, uint8_t req_type )
	{
		fragment_header headers[2] =
		{
			FREEMOUNT_FRAGMENT_HEADER_INITIALIZER,
			FREEMOUNT_FRAGMENT_HEADER_INITIALIZER
		};
		
		headers[0].type = frag_req;
		headers[0].data = req_type;
		headers[1].type = frag_eom;
		
		write_in_full( fd, headers, sizeof headers );
	}
	
}

