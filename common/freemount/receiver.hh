/*
	freemount/receiver.hh
	---------------------
*/

#ifndef FREEMOUNT_RECEIVER_HH
#define FREEMOUNT_RECEIVER_HH

// plus
#include "plus/var_string.hh"

// freemount
#include "freemount/fragment.hh"


namespace freemount
{
	
	typedef int (*fragment_handler_function)( void*, const fragment_header& );
	
	class data_receiver
	{
		private:
			plus::var_string  its_buffer;
			
			fragment_handler_function  its_handler;
			void*                      its_context;
		
		public:
			data_receiver( fragment_handler_function handler, void* context );
			
			int recv_bytes( const char* buffer, std::size_t n );
	};
	
}

#endif

