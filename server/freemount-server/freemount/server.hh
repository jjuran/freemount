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
	
	struct fragment_header;
	
	int fragment_handler( void* that, const fragment_header& fragment );
	
}

#endif

