/*
	freemount/send_ack.cc
	---------------------
*/

#include "freemount/send_ack.hh"

// freemount
#include "freemount/frame.hh"
#include "freemount/queue_utils.hh"
#include "freemount/send_queue.hh"


namespace freemount
{
	
	void send_read_ack( int fd, unsigned n_bytes )
	{
		send_queue queue( fd );
		
		queue_int( queue, Frame_ack_read, n_bytes );
		
		queue.flush();
	}
	
}
