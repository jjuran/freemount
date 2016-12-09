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


int request_task::result() const
{
	return its_status > 0 ? -its_status : its_result;
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
	
	try
	{
		task.its_result = task.f( s, id, r );
		
		task.its_status = 0;
	}
	catch ( const p7::errno_t& err )
	{
		task.its_status = err;
	}
	
	poseven::thread::testcancel();
	
	const int err = task.result();
	
	send_response( s.queue(), err, id );
	
	return NULL;
}

void begin_task( req_func f, session& s, uint8_t r_id )
{
	request& r = *s.get_request( r_id );
	
	r.task = new request_task( f, s, r_id );
}

}  // namespace freemount
