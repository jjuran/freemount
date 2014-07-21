/*
	freemount/request.hh
	--------------------
*/

#ifndef FREEMOUNT_REQUEST_HH
#define FREEMOUNT_REQUEST_HH

// plus
#include "plus/string.hh"

// freemount
#include "freemount/frame.hh"


namespace freemount
{
	
	struct request
	{
		plus::string path;
		
		request_type type;
		
		request( request_type type = req_none );
	};
	
	inline request::request( request_type type )
	:
		type( type )
	{
	}
	
}

#endif
