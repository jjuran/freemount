/*
	freemount/send_queue.cc
	-----------------------
*/

#include "freemount/send_queue.hh"

// more-libc
#include "more/string.h"

// freemount
#include "freemount/write_in_full.hh"


namespace freemount
{
	
	inline
	void buffer::append_UNCHECKED( const void* data, size_t n )
	{
		mempcpy( its_buffer + its_mark, data, n );
		
		its_mark += n;
	}
	
	void send_queue::flush()
	{
		write_in_full( its_fd, its_buffer.data(), its_buffer.size() );
		
		its_buffer.clear();
	}
	
	void send_queue::add( const void* data, size_t n )
	{
		if ( n > its_buffer.freespace() )
		{
			flush();
			
			if ( n > its_buffer.capacity() )
			{
				write_in_full( its_fd, data, n );
				
				return;
			}
		}
		
		its_buffer.append_UNCHECKED( data, n );
	}
	
}
