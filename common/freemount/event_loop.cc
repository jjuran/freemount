/*
	freemount/event_loop.cc
	-----------------------
*/

#include "freemount/event_loop.hh"

// POSIX
#include <unistd.h>

// Standard C
#include <errno.h>

// freemount
#include "freemount/receiver.hh"


namespace freemount
{
	
	int run_event_loop( data_receiver& r, int fd )
	{
		for ( ;; )
		{
			char buffer[ 4096 ];
			
			const ssize_t n_read = read( fd, &buffer, sizeof buffer );
			
			if ( n_read > 0 )
			{
				const int status = r.recv_bytes( buffer, n_read );
				
				if ( status != 0 )
				{
					return status;
				}
			}
			else if ( n_read == 0 )
			{
				return 0;
			}
			else
			{
				return -errno;
			}
		}
	}
	
}

