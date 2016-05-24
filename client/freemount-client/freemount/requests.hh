/*
	freemount/requests.hh
	---------------------
*/

#ifndef FREEMOUNT_REQUESTS_HH
#define FREEMOUNT_REQUESTS_HH

// Standard C
#include <stdint.h>

// freemount
#include "freemount/frame.hh"


namespace freemount
{
	
	void send_path_request( int          fd,
	                        const char*  path,
	                        uint32_t     size,
	                        uint8_t      r_type,
	                        uint8_t      r_id = 0 );
	
	inline
	void send_stat_request( int          fd,
	                        const char*  path,
	                        uint32_t     size,
	                        uint8_t      r_id = 0 )
	{
		send_path_request( fd, path, size, req_stat, r_id );
	}
	
	inline
	void send_list_request( int          fd,
	                        const char*  path,
	                        uint32_t     size,
	                        uint8_t      r_id = 0 )
	{
		send_path_request( fd, path, size, req_list, r_id );
	}
	
	inline
	void send_read_request( int          fd,
	                        const char*  path,
	                        uint32_t     size,
	                        uint8_t      r_id = 0 )
	{
		send_path_request( fd, path, size, req_read, r_id );
	}
	
	void send_write_request( int          fd,
	                         const char*  path,
	                         uint32_t     path_size,
	                         const char*  data,
	                         uint32_t     data_size,
	                         uint8_t      r_id = 0 );
	
}

#endif
