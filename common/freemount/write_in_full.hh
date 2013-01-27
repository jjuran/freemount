/*
	freemount/write_in_full.hh
	--------------------------
*/

#ifndef FREEMOUNT_WRITEINFULL_HH
#define FREEMOUNT_WRITEINFULL_HH

// POSIX
#include <sys/types.h>


namespace freemount
{
	
	struct failed_write
	{
		int errnum;
	};
	
	void write_in_full( int fd, const void* buffer, size_t n );
	
}

#endif

