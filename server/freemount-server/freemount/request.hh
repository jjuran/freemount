/*
	freemount/request.hh
	--------------------
*/

#ifndef FREEMOUNT_REQUEST_HH
#define FREEMOUNT_REQUEST_HH

// POSIX
#include <sys/types.h>

// plus
#include "plus/string.hh"

// freemount
#include "freemount/frame.hh"

// freemount-server
#include "freemount/task.hh"


namespace freemount
{
	
	struct request
	{
		plus::string path;
		plus::string data;
		
		request_type type;
		
		int64_t  n;
		off_t    offset;
		
		int fd;
		
		request_task* task;
		
		request( request_type type = req_none );
		
		~request();
		
		private:
			// non-copyable
			request           ( const request& );
			request& operator=( const request& );
	};
	
	inline
	request::request( request_type type )
	:
		type( type ),
		n( -1 ),
		offset( -1 ),
		fd( -1 ),
		task()
	{
	}
	
	inline
	request::~request()
	{
		delete task;
	}
	
}

#endif
