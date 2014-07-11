/*
	freemount/receiver.hh
	---------------------
*/

#ifndef FREEMOUNT_RECEIVER_HH
#define FREEMOUNT_RECEIVER_HH

// plus
#include "plus/var_string.hh"

// freemount
#include "freemount/frame.hh"


namespace freemount
{
	
	typedef int (*frame_handler_function)( void*, const frame_header& );
	
	class data_receiver
	{
		private:
			plus::var_string  its_buffer;
			
			frame_handler_function  its_handler;
			void*                   its_context;
		
		public:
			data_receiver( frame_handler_function handler, void* context );
			
			int recv_bytes( const char* buffer, std::size_t n );
	};
	
}

#endif
