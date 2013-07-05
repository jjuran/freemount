/*
	freemount/send.cc
	-----------------
*/

#include "freemount/send.hh"

// iota
#include "iota/endian.hh"

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
	
	void send_u32_fragment( int fd, uint8_t type, uint32_t data )
	{
		const size_t buffer_size = sizeof (fragment_header) + sizeof (uint32_t);
		
		uint32_t buffer[ buffer_size / sizeof (uint32_t) ] = { 0 };
		
		uint8_t* p = (uint8_t*) buffer;
		
		/*
			0: frag header part 0
			4: frag header part 1
			8: data
		*/
		
		p[3] = sizeof (uint32_t);
		p[6] = type;
		
		buffer[2] = iota::big_u32( data );
		
		write_in_full( fd, buffer, sizeof buffer );
	}
	
	void send_u64_fragment( int fd, uint8_t type, uint64_t data )
	{
		const size_t buffer_size = sizeof (fragment_header) + sizeof (uint64_t);
		
		uint64_t buffer[ buffer_size / sizeof (uint64_t) ] = { 0 };
		
		uint8_t* p = (uint8_t*) buffer;
		
		/*
			0: frag header
			4: frag header part 1
			8: data
		*/
		
		p[3] = sizeof (uint64_t);
		p[6] = type;
		
		buffer[1] = iota::big_u64( data );
		
		write_in_full( fd, buffer, sizeof buffer );
	}
	
}

