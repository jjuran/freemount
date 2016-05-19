/*
	freemount/response.hh
	---------------------
*/

#ifndef FREEMOUNT_RESPONSE_HH
#define FREEMOUNT_RESPONSE_HH

// Standard C
#include <stdint.h>


namespace freemount
{
	
	class send_queue;
	
	void send_response( send_queue& queue, int result, uint8_t r_id );
	
}

#endif
