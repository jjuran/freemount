/*
	freemount/response.cc
	---------------------
*/

#include "freemount/response.hh"

// Standard C
#include <stdio.h>
#include <string.h>

// freemount
#include "freemount/frame.hh"
#include "freemount/queue_utils.hh"
#include "freemount/send_lock.hh"
#include "freemount/send_queue.hh"


namespace freemount {


void send_response( send_queue& queue, int result, uint8_t r_id )
{
	if ( result >= 0 )
	{
		result = 0;
		
		fprintf( stderr, "Request id %u: ok\n", r_id );
	}
	else
	{
		const int e = -result;
		
		fprintf( stderr, "Request id %u: %s (%d)\n", r_id, strerror( e ), e );
	}
	
	send_lock lock;
	
	queue_int( queue, Frame_result, -result, r_id );
	
	queue.flush();
}

}  // namespace freemount
