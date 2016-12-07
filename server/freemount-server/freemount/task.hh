/*
	freemount/task.hh
	-----------------
*/

#ifndef FREEMOUNT_TASK_HH
#define FREEMOUNT_TASK_HH

// Standard C
#include <stdint.h>

// poseven
#include "poseven/types/thread.hh"


namespace freemount
{
	
	struct request;
	class session;
	
	typedef int (*req_func)( session& s, uint8_t r_id, const request& r );
	
	class request_task
	{
		public:
			typedef poseven::thread p7_thread;
			
			const req_func  f;
			session&        s;
			const uint8_t   r_id;
		
		private:
			int        its_status;  // -1 until done, then 0 or errno
			int        its_result;
			p7_thread  its_thread;
			
			static void* start( void* param );
		
		public:
			request_task( req_func f, class session& s, uint8_t r_id );
			~request_task();
			
			bool done() const  { return its_status >= 0; }
			
			int result() const;
			
			void cancel()  { its_thread.cancel(); }
	};
	
	void begin_task( req_func f, session& s, uint8_t r_id );
	
}

#endif
