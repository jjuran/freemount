/*
	freemount/receiver.hh
	---------------------
*/

#ifndef FREEMOUNT_RECEIVER_HH
#define FREEMOUNT_RECEIVER_HH

// Standard C/C++
#include <cstddef>

// freemount
#include "freemount/fragment.hh"


namespace freemount
{
	
	typedef int (*fragment_handler_function)( void*, const fragment_header& );
	
	class data_receiver
	{
		private:
			fragment_handler_function  its_handler;
			void*                      its_context;
		
		public:
			data_receiver( fragment_handler_function handler, void* context );
			
			void recv_bytes( const char* buffer, std::size_t n );
	};
	
}

#endif

