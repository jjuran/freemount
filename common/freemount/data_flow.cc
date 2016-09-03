/*
	freemount/data_flow.cc
	----------------------
*/

#include "freemount/data_flow.hh"

// POSIX
#include <pthread.h>

// must
#include "must/pthread.h"


namespace freemount
{
	
	static long the_congestion_window;
	
	void set_congestion_window( long n_bytes )
	{
		the_congestion_window = n_bytes;
	}
	
	static inline
	long get_congestion_window()
	{
	#ifdef __RELIX__
		
		return 0;
		
	#endif
		
		return the_congestion_window;
	}
	
	static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
	static pthread_cond_t  data_cond  = PTHREAD_COND_INITIALIZER;
	
	
	class data_lock
	{
		private:
			// non-copyable
			data_lock           ( const data_lock& );
			data_lock& operator=( const data_lock& );
		
		public:
			data_lock();
			~data_lock();
			
			void wait();
			void broadcast();
	};
	
	
	inline
	data_lock::data_lock()
	{
		must_pthread_mutex_lock( &data_mutex );
	}
	
	inline
	data_lock::~data_lock()
	{
		must_pthread_mutex_unlock( &data_mutex );
	}
	
	inline
	void data_lock::wait()
	{
		must_pthread_cond_wait( &data_cond, &data_mutex );
	}
	
	inline
	void data_lock::broadcast()
	{
		must_pthread_cond_broadcast( &data_cond );
	}
	
	
	static long n_data_bytes_in_flight = 0;
	
	void data_transmitting( unsigned n_bytes )
	{
		if ( const long congestion_window = get_congestion_window() )
		{
			data_lock lock;
			
			while ( n_data_bytes_in_flight >= congestion_window )
			{
				lock.wait();
			}
			
			n_data_bytes_in_flight += n_bytes;
		}
	}
	
	void data_acknowledged( unsigned n_bytes )
	{
		if ( const long congestion_window = get_congestion_window() )
		{
			data_lock lock;
			
			n_data_bytes_in_flight -= n_bytes;
			
			if ( n_data_bytes_in_flight < congestion_window )
			{
				lock.broadcast();
			}
		}
	}
	
}
