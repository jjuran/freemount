/*
	freemount/queue_utils.cc
	------------------------
*/

#include "freemount/queue_utils.hh"

// iota
#include "iota/endian.hh"

// freemount
#include "freemount/frame.hh"
#include "freemount/send_queue.hh"


namespace freemount
{
	
	void queue_int_( send_queue& queue, uint8_t type, uint8_t value, uint8_t r_id )
	{
		frame_header header = FREEMOUNT_FRAME_HEADER_INITIALIZER;
		
		header.r_id = r_id;
		header.type = type;
		header.data = value;
		
		queue.add( &header, sizeof header );
	}
	
	void queue_int_( send_queue& queue, uint8_t type, uint32_t value, uint8_t r_id )
	{
		if ( value <= 0xFF )
		{
			queue_int_( queue, type, uint8_t( value ), r_id );
			
			return;
		}
		
		frame_header header = FREEMOUNT_FRAME_HEADER_INITIALIZER;
		
		header.big_size = iota::big_u16( sizeof value );
		header.r_id     = r_id;
		header.type     = type;
		
		value = iota::big_u32( value );
		
		queue.add( &header, sizeof header );
		queue.add( &value,  sizeof value  );
	}
	
	void queue_int_( send_queue& queue, uint8_t type, uint64_t value, uint8_t r_id )
	{
		if ( value <= 0xFFFFFFFFull )
		{
			queue_int_( queue, type, uint32_t( value ), r_id );
			
			return;
		}
		
		frame_header header = FREEMOUNT_FRAME_HEADER_INITIALIZER;
		
		header.big_size = iota::big_u16( sizeof value );
		header.r_id     = r_id;
		header.type     = type;
		
		value = iota::big_u64( value );
		
		queue.add( &header, sizeof header );
		queue.add( &value,  sizeof value  );
	}
	
	void queue_string( send_queue& queue, uint8_t type, const char* s, uint32_t len, uint8_t r_id )
	{
		frame_header header = FREEMOUNT_FRAME_HEADER_INITIALIZER;
		
		header.big_size = iota::big_u16( len );
		header.r_id     = r_id;
		header.type     = type;
		
		queue.add( &header, sizeof header );
		
		queue.add( s, len );
		
		const int pad_length = 3 - (len + 3 & 0x3);
		
		const uint32_t zero = 0;
		
		queue.add( &zero, pad_length );
	}
	
	void queue_buffer( send_queue& queue, uint8_t type, const char* s, uint32_t len, uint8_t r_id )
	{
		enum
		{
			block_size = 0x4000,  // 16384
			max_length = 0xFFFF,  // 65535
		};
		
		if ( len > max_length )
		{
			/*
				If the server hasn't been patched to recognize
				multiple send-data frames and concatenate them,
				then it also won't be expecting an I/O count,
				so sending one will invalidate the message and
				prevent the server from corrupting the data (by
				writing only the last data frame to the file).
			*/
			
			queue_int( queue, Frame_io_count, len, r_id );
			
			frame_header header = FREEMOUNT_FRAME_HEADER_INITIALIZER;
			
			header.big_size = iota::big_u16( block_size );
			
			header.r_id = r_id;
			header.type = type;
			
			do
			{
				queue.add( &header, sizeof header );
				
				queue.add( s, block_size );
				
				s   += block_size;
				len -= block_size;
			}
			while ( len > max_length );
		}
		
		if ( len )
		{
			queue_string( queue, type, s, len, r_id );
		}
	}
	
}
