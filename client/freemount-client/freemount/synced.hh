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
	
	void synced_put( int          in,
	                 int          out,
	                 const char*  path,
	                 uint32_t     path_size,
	                 const char*  data,
	                 uint32_t     data_size );
	
	inline
	void synced_put( int                  in,
	                 int                  out,
	                 const plus::string&  path,
	                 const plus::string&  data )
	{
		synced_put( in, out, path.data(), path.size(),
		                     data.data(), data.size() );
	}
	
	int synced_open( int          in,
	                 int          out,
	                 int          chosen_fd,
	                 const char*  path,
	                 uint32_t     path_size );
	
	void synced_close( int in, int out, int fd );
	
	void synced_link( int          in,
	                  int          out,
	                  const char*  src_path,
	                  uint32_t     src_path_size,
	                  const char*  dst_path,
	                  uint32_t     dst_path_size );
	
}

#endif
