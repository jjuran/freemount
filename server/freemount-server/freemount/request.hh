/*
	freemount/request.hh
	--------------------
*/

#ifndef FREEMOUNT_REQUEST_HH
#define FREEMOUNT_REQUEST_HH

// plus
#include "plus/string.hh"

// freemount
#include "freemount/fragment.hh"


namespace freemount
{
	
	struct request
	{
		plus::string path;
		
		request_type type;
	};
	
}

#endif
