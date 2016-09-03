/*
	freemount/send_lock.cc
	----------------------
*/

#include "freemount/send_lock.hh"

// POSIX
#include <pthread.h>

// must
#include "must/pthread.h"


namespace freemount
{
	
#ifndef __RELIX__
	
	static pthread_mutex_t send_mutex = PTHREAD_MUTEX_INITIALIZER;
	
	
	send_lock::send_lock()
	{
		must_pthread_mutex_lock( &send_mutex );
	}
	
	send_lock::~send_lock()
	{
		must_pthread_mutex_unlock( &send_mutex );
	}
	
#endif
	
}
