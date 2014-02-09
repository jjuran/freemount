/*
	freemount/receiver.cc
	---------------------
*/

#include "freemount/receiver.hh"

// iota
#include "iota/endian.hh"


namespace freemount
{
	
	data_receiver::data_receiver( fragment_handler_function handler, void* context )
	:
		its_handler( handler ),
		its_context( context )
	{
	}
	
	static inline size_t pad( size_t value, unsigned multiple )
	{
		const unsigned mask = multiple - 1;
		
		return (value + mask) & ~mask;
	}
	
	int data_receiver::recv_bytes( const char* buffer, std::size_t n )
	{
		its_buffer.append( buffer, n );
		
		const char* data = its_buffer.data();
		size_t data_size = its_buffer.size();
		
		const char* p = data;
		
		while ( data_size >= sizeof (fragment_header) )
		{
			const fragment_header& h = *(const fragment_header*) p;
			
			const size_t payload_size = iota::u16_from_big( h.big_size );
			
			const size_t fragment_size = sizeof (fragment_header)
			                           + pad( payload_size, 4 );
			
			if ( data_size < fragment_size )
			{
				break;
			}
			
			const int status = its_handler( its_context, h );
			
			if ( status != 0 )
			{
				return status;
			}
			
			data_size -= fragment_size;
			p         += fragment_size;
		}
		
		if ( p != data )
		{
			its_buffer.assign( p, data_size );
		}
		
		return 0;
	}
	
}

