/*
	freemount/requests.cc
	---------------------
*/

#include "freemount/requests.hh"

// POSIX
#include <sys/uio.h>

// iota
#include "iota/endian.hh"

// poseven
#include "poseven/types/errno_t.hh"


#define ARRAYLEN( a )  (sizeof (a) / sizeof (a)[0])

#define ARRAY_LEN( a )  a, ARRAYLEN( a )


namespace freemount
{
	
	namespace p7 = poseven;
	
	
	void send_path_request( int fd, const char* path, uint32_t size, uint8_t r_type, uint8_t r_id )
	{
		frame_header req_and_path[ 2 ] = { { 0 }, { 0 } };
		
		frame_header eom = { 0 };
		
		req_and_path[ 0 ].r_id = r_id;
		req_and_path[ 0 ].type = Frame_request;
		req_and_path[ 0 ].data = r_type;
		
		req_and_path[ 1 ].big_size = iota::big_u16( size );
		req_and_path[ 1 ].r_id = r_id;
		req_and_path[ 1 ].type = Frame_arg_path;
		
		const void* zeroes = &eom;
		
		const int pad_length = 3 - (size + 3 & 0x3);
		
		eom.r_id = r_id;
		eom.type = Frame_submit;
		
		struct iovec iov[] =
		{
			{ (void*) req_and_path, sizeof req_and_path },
			{ (void*) path,         size                },
			{ (void*) zeroes,       pad_length          },
			{ (void*) &eom,         sizeof eom          },
		};
		
		ssize_t n_written = writev( fd, ARRAY_LEN( iov ) );
		
		if ( n_written < 0 )
		{
			p7::throw_errno( errno );
		}
		
		if ( n_written != 3 * sizeof (frame_header) + size + pad_length )
		{
			p7::throw_errno( EINTR );
		}
	}
	
}
