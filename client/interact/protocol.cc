/*
	protocol.cc
	-----------
*/

#include "protocol.hh"

// POSIX
#include <unistd.h>

// Standard C
#include <errno.h>
#include <stdlib.h>

// freemount
#include "freemount/event_loop.hh"
#include "freemount/frame_size.hh"
#include "freemount/receiver.hh"


using namespace freemount;


unsigned x_numerator   = 1;
unsigned x_denominator = 1;

struct request_status
{
	ssize_t result;
};

static
int wait_for_result( int fd, frame_handler_function handler )
{
	request_status req;
	
	data_receiver r( handler, &req );
	
	int looped = run_event_loop( r, fd );
	
	if ( looped < 0  ||  (looped == 0  &&  (looped = -ECONNRESET)) )
	{
		return looped;
	}
	
	if ( looped > 2 )
	{
		abort();
	}
	
	int result = req.result;
	
	if ( looped == 2 )
	{
		result = -result;  // errno
	}
	
	return result;
}

static
int frame_handler( void* that, const frame_header& frame )
{
	if ( frame.type == Frame_recv_data )
	{
		const uint8_t* data = (const uint8_t*) get_data( frame );
		
		uint16_t buffer[ 3 ];
		
		if ( x_numerator != x_denominator  &&  data[ 0 ] == 5 )
		{
			uint16_t x = iota::u16_from_big( *(const uint16_t*) &data[ 2 ] );
			uint16_t y = iota::u16_from_big( *(const uint16_t*) &data[ 4 ] );
			
			x *= x_denominator;
			y *= x_denominator;
			x /= x_numerator;
			y /= x_numerator;
			
			buffer[ 0 ] = *(const uint16_t*) &data[ 0 ];
			buffer[ 1 ] = iota::big_u16( x );
			buffer[ 2 ] = iota::big_u16( y );
			
			data = (const uint8_t*) buffer;
		}
		
		write( STDOUT_FILENO, data, get_size( frame ) );
		return 0;
	}
	
	int result = ((request_status*) that)->result = get_u32( frame );
	
	if ( (int8_t) frame.r_id < 0  &&  result == 0 )
	{
		return 0;  // Don't exit the event loop when updates succeed.
	}
	
	if ( frame.type == Frame_result )
	{
		return result == 0 ? 1 : 2;
	}
	
	return 3;
}

int run_event_loop( int protocol_in )
{
	return wait_for_result( protocol_in, &frame_handler );
}
