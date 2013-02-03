/*
	freemount/write_in_full.cc
	--------------------------
*/

#include "freemount/write_in_full.hh"

// POSIX
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>


namespace freemount
{
	
	static ssize_t write_all( int fd, const void* buffer, size_t n )
	{
		size_t n_bytes = 0;
		
		while ( n_bytes < n )
		{
			ssize_t n_written = write( fd, buffer, n - n_bytes );
			
			if ( n_written >= 0 )
			{
				n_bytes += n_written;
				
				buffer = (char*) buffer + n_written;
			}
			else if ( errno == EAGAIN  ||  errno == EWOULDBLOCK )
			{
				fd_set write_fds;
				
				FD_ZERO( &write_fds );
				
				FD_SET( fd, &write_fds );
				
				int selected = select( fd + 1, NULL, &write_fds, NULL, NULL );
				
				(void) selected;
				
				// Only expected error is EINTR
				continue;
			}
			else if ( errno == EINTR )
			{
				continue;
			}
			else
			{
				return n_bytes != 0 ? ssize_t( n_bytes ) : -1;
			}
		}
		
		return n_bytes;
	}
	
	
	static inline void throw_failed_write( int errnum )
	{
		failed_write error = { errnum };
		
		throw error;
	}
	
	void write_in_full( int fd, const void* buffer, size_t n )
	{
		const ssize_t n_written = write_all( fd, buffer, n );
		
		if ( size_t( n_written ) != n )
		{
			throw_failed_write( errno );
		}
	}
	
}

