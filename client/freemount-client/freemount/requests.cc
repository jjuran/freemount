/*
	freemount/requests.cc
	---------------------
*/

#include "freemount/requests.hh"

// freemount
#include "freemount/send.hh"
#include "freemount/write_in_full.hh"


namespace freemount
{
	
	void send_path_request( int fd, const char* path, uint32_t size, uint8_t r_type, uint8_t r_id )
	{
		fragment_header header = { 0 };
		
		header.r_id = r_id;
		header.type = frag_req;
		header.data = r_type;
		
		write_in_full( fd, &header, sizeof header );
		
		send_string_fragment( fd, frag_file_path, path, size, r_id );
		
		send_empty_fragment( fd, frag_eom, r_id );
	}
	
}
