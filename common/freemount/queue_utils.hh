/*
	freemount/queue_utils.hh
	------------------------
*/

#ifndef FREEMOUNT_QUEUEUTILS_HH
#define FREEMOUNT_QUEUEUTILS_HH

// Standard C
#include <stdint.h>


namespace freemount
{
	
	class send_queue;
	
	void queue_int_( send_queue& queue, uint8_t type, uint8_t  value, uint8_t r_id = 0 );
	void queue_int_( send_queue& queue, uint8_t type, uint32_t value, uint8_t r_id = 0 );
	void queue_int_( send_queue& queue, uint8_t type, uint64_t value, uint8_t r_id = 0 );
	
	void queue_string( send_queue& queue, uint8_t type, const char* s, uint32_t len, uint8_t r_id = 0 );
	void queue_buffer( send_queue& queue, uint8_t type, const char* s, uint32_t len, uint8_t r_id = 0 );
	
	inline
	void queue_empty( send_queue& queue, uint8_t type, uint8_t r_id = 0 )
	{
		queue_int_( queue, type, uint8_t(), r_id );
	}
	
	template < int width > struct freemount_int      { typedef uint32_t type; };
	template <           > struct freemount_int< 1 > { typedef uint8_t  type; };
	template <           > struct freemount_int< 8 > { typedef uint64_t type; };
	
	template < class Int >
	inline
	void queue_int( send_queue& queue, uint8_t type, Int value, uint8_t r_id = 0 )
	{
		typedef typename freemount_int< sizeof (Int) >::type int_t;
		
		queue_int_( queue, type, int_t( value ), r_id );
	}
	
}

#endif
