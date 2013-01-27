/*
	freemount/receiver.cc
	---------------------
*/

#include "freemount/receiver.hh"


namespace freemount
{
	
	data_receiver::data_receiver( fragment_handler_function handler, void* context )
	:
		its_handler( handler ),
		its_context( context )
	{
	}
	
	void data_receiver::recv_bytes( const char* buffer, std::size_t n )
	{
		while ( n >= sizeof (fragment_header) )
		{
			const fragment_header& h = *(const fragment_header*) buffer;
			
			its_handler( its_context, h );
			
			n -= sizeof (fragment_header);
			
			buffer += sizeof (fragment_header);
		}
	}
	
}

