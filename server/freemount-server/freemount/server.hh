/*
	freemount/server.hh
	-------------------
*/

#ifndef FREEMOUNT_SERVER_HH
#define FREEMOUNT_SERVER_HH


namespace freemount
{
	
	struct frame_header;
	
	int frame_handler( void* that, const frame_header& frame );
	
}

#endif
