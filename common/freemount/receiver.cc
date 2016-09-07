/*
	freemount/receiver.cc
	---------------------
*/

#include "freemount/receiver.hh"

// iota
#include "iota/endian.hh"


namespace freemount
{
	
	typedef plus::string::size_type size_t;
	
	
	data_receiver::data_receiver( frame_handler_function handler, void* context )
	:
		its_handler( handler ),
		its_context( context )
	{
	}
	
	static inline
	size_t pad( size_t value, unsigned multiple )
	{
		const unsigned mask = multiple - 1;
		
		return (value + mask) & ~mask;
	}
	
	int data_receiver::recv_bytes( const char* buffer, size_t n )
	{
		its_buffer.append( buffer, n );
		
		const char* data = its_buffer.data();
		size_t data_size = its_buffer.size();
		
		const char* p = data;
		
		while ( data_size >= sizeof (frame_header) )
		{
			const frame_header& h = *(const frame_header*) p;
			
			const size_t payload_size = iota::u16_from_big( h.big_size );
			
			const size_t frame_size = sizeof (frame_header)
			                        + pad( payload_size, 4 );
			
			if ( data_size < frame_size )
			{
				break;
			}
			
			const int status = its_handler( its_context, h );
			
			if ( status != 0 )
			{
				return status;
			}
			
			data_size -= frame_size;
			p         += frame_size;
		}
		
		if ( p != data )
		{
			its_buffer.assign( p, data_size );
		}
		
		return 0;
	}
	
}
