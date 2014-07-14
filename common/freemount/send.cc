/*
	freemount/send.cc
	-----------------
*/

#include "freemount/send.hh"

// iota
#include "iota/endian.hh"

// freemount
#include "freemount/frame.hh"
#include "freemount/write_in_full.hh"


namespace freemount
{
	
	void send_empty_frame( int fd, uint8_t type, uint8_t r_id )
	{
		frame_header header = FREEMOUNT_FRAME_HEADER_INITIALIZER;
		
		header.r_id = r_id;
		header.type = type;
		
		write_in_full( fd, &header, sizeof header );
	}
	
	void send_empty_request( int fd, uint8_t req_type, uint8_t r_id )
	{
		frame_header headers[2] =
		{
			FREEMOUNT_FRAME_HEADER_INITIALIZER,
			FREEMOUNT_FRAME_HEADER_INITIALIZER
		};
		
		headers[0].r_id = r_id;
		headers[0].type = Frame_request;
		headers[0].data = req_type;
		headers[1].r_id = r_id;
		headers[1].type = Frame_submit;
		
		write_in_full( fd, headers, sizeof headers );
	}
	
	void send_u8_frame( int fd, uint8_t type, uint8_t data, uint8_t r_id )
	{
		frame_header header = FREEMOUNT_FRAME_HEADER_INITIALIZER;
		
		header.r_id = r_id;
		header.type = type;
		header.data = data;
		
		write_in_full( fd, &header, sizeof header );
	}
	
	void send_u32_frame( int fd, uint8_t type, uint32_t data, uint8_t r_id )
	{
		if ( data <= 0xFF )
		{
			send_u8_frame( fd, type, data, r_id );
			
			return;
		}
		
		const size_t buffer_size = sizeof (frame_header) + sizeof (uint32_t);
		
		uint32_t buffer[ buffer_size / sizeof (uint32_t) ] = { 0 };
		
		uint8_t* p = (uint8_t*) buffer;
		
		/*
			0: frag header part 0
			4: frag header part 1
			8: data
		*/
		
		p[3] = sizeof (uint32_t);
		p[5] = r_id;
		p[6] = type;
		
		buffer[2] = iota::big_u32( data );
		
		write_in_full( fd, buffer, sizeof buffer );
	}
	
	void send_u64_frame( int fd, uint8_t type, uint64_t data, uint8_t r_id )
	{
		if ( data <= 0xFFFFFFFFull )
		{
			send_u32_frame( fd, type, data, r_id );
			
			return;
		}
		
		const size_t buffer_size = sizeof (frame_header) + sizeof (uint64_t);
		
		uint64_t buffer[ buffer_size / sizeof (uint64_t) ] = { 0 };
		
		uint8_t* p = (uint8_t*) buffer;
		
		/*
			0: frag header
			4: frag header part 1
			8: data
		*/
		
		p[3] = sizeof (uint64_t);
		p[5] = r_id;
		p[6] = type;
		
		buffer[1] = iota::big_u64( data );
		
		write_in_full( fd, buffer, sizeof buffer );
	}
	
	void send_string_frame( int fd, uint8_t type, const char* data, uint16_t length, uint8_t r_id )
	{
		if ( length == 1  &&  *data != '\0' )
		{
			send_u8_frame( fd, type, *data, r_id );
			
			return;
		}
		
		const size_t buffer_size = sizeof (frame_header) + length;
		
		frame_header h = { 0 };
		
		h.big_size = iota::big_u16( length );
		
		h.r_id = r_id;
		h.type = type;
		
		write_in_full( fd, &h, sizeof h );
		
		write_in_full( fd, data, length );
		
		if ( length & 0x3 )
		{
			const uint32_t zero = 0;
			
			write_in_full( fd, &zero, 4 - (length & 0x3) );
		}
	}
	
}
