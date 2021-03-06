/*
	freemount/task.cc
	-----------------
*/

#include "freemount/task.hh"

// poseven
#include "poseven/types/errno_t.hh"
#include "poseven/types/thread.hh"

// freemount
#include "freemount/request.hh"
#include "freemount/response.hh"
#include "freemount/session.hh"


namespace freemount {

namespace p7 = poseven;


bool request_task::done() const
{
	p7::lock k( its_mutex );
	
	return its_status >= 0;
}

request_task::request_task( req_func f, class session& s, uint8_t r_id )
:
	f( f ),
	s( s ),
	r_id( r_id )
{
	its_status = -1;
	
	its_thread.create( &start, this );
}

request_task::~request_task()
{
	its_thread.cancel();
	its_thread.join();
}

void* request_task::start( void* param )
{
	request_task& task = *(request_task*) param;
	
	uint8_t id = task.r_id;
	session& s = task.s;
	request& r = *s.get_request( id );
	
	int result;
	
	try
	{
		result = task.f( s, id, r );
	}
	catch ( const p7::errno_t& err )
	{
		result = -err;
	}
	
	poseven::thread::testcancel();
	
	p7::lock k( task.its_mutex );
	
	send_response( s.queue(), result, id );
	
	task.its_status = 0;
	
	return NULL;
}

void begin_task( req_func f, session& s, uint8_t r_id )
{
	request& r = *s.get_request( r_id );
	
	r.task = new request_task( f, s, r_id );
}

}  // namespace freemount
