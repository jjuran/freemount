/*
	freemount/send_lock.cc
	----------------------
*/

#include "freemount/send_lock.hh"

// POSIX
#include <pthread.h>

// Standard C
#include <stdlib.h>


namespace must
{
	
	static
	void pthread_mutex_lock( pthread_mutex_t& mutex )
	{
		int error = ::pthread_mutex_lock( &mutex );
		
		if ( error )
		{
			abort();
		}
	}
	
	static
	void pthread_mutex_unlock( pthread_mutex_t& mutex )
	{
		int error = ::pthread_mutex_unlock( &mutex );
		
		if ( error )
		{
			abort();
		}
	}
	
}

namespace freemount
{
	
#ifndef __RELIX__
	
	static pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
	
	
	send_lock::send_lock()
	{
		must::pthread_mutex_lock( send_mutex );
	}
	
	send_lock::~send_lock()
	{
		must::pthread_mutex_unlock( send_mutex );
	}
	
#endif
	
}
