/*
	freemount/synced.hh
	-------------------
*/

#ifndef FREEMOUNT_SYNCED_HH
#define FREEMOUNT_SYNCED_HH

// Standard C
#include <stdint.h>

// plus
#include "plus/string.hh"


namespace freemount
{
	
	// Exceptions
	
	struct path_error
	{
		plus::string  path;
		int           error;
		
		path_error( const plus::string& path, int error )
		:
			path ( path  ),
			error( error )
		{
		}
	};
	
	struct connection_error
	{
		plus::string  path;
		int           error;
		
		connection_error( const plus::string& path, int error )
		:
			path ( path  ),
			error( error )
		{
		}
	};
	
	struct unexpected_frame_type
	{
		uint8_t type;
		
		unexpected_frame_type( uint8_t type ) : type( type )
		{
		}
	};
	
	// Functions
	
	plus::string synced_get( int in, int out, const plus::string& path );
	
}

#endif
