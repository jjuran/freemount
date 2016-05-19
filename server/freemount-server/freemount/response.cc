/*
	freemount/response.cc
	---------------------
*/

#include "freemount/response.hh"

// POSIX
#include <unistd.h>

// freemount
#include "freemount/frame.hh"
#include "freemount/queue_utils.hh"
#include "freemount/send_queue.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


namespace freemount {


void send_response( send_queue& queue, int result, uint8_t r_id )
{
	if ( result >= 0 )
	{
		result = 0;
		
		write( STDERR_FILENO, STR_LEN( " ok\n" ) );
	}
	else
	{
		write( STDERR_FILENO, STR_LEN( " err\n" ) );
	}
	
	queue_int( queue, Frame_result, -result, r_id );
	
	queue.flush();
}

}  // namespace freemount
