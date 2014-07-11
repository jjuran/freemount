/*
	freemount/server.hh
	-------------------
*/

#ifndef FREEMOUNT_SERVER_HH
#define FREEMOUNT_SERVER_HH

// vfs
#include "vfs/node_fwd.hh"


namespace freemount
{
	
	struct frame_header;
	
	int frame_handler( void* that, const frame_header& frame );
	
}

#endif
