/*
	freemount/task.cc
	-----------------
*/

#include "freemount/task.hh"

// POSIX
#include <pthread.h>

// poseven
#include "poseven/types/errno_t.hh"

// freemount
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
	
	int error = pthread_create( &its_thread, NULL, &start, this );
	
	if ( error )
	{
		p7::throw_errno( error );
	}
}

request_task::~request_task()
{
	pthread_join( its_thread, NULL );
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
	
	const int err = task.result();
	
	send_response( s.queue(), err, id );
	
	return NULL;
}

static request_task* request_tasks[ 256 ];

static uint8_t n_request_tasks = 0;

void begin_task( req_func f, session& s, uint8_t r_id )
{
	request_tasks[ n_request_tasks ] = new request_task( f, s, r_id );
	
	++n_request_tasks;
}

static
void end_task( uint8_t i )
{
	delete request_tasks[ i ];
	
	request_tasks[ i ] = request_tasks[ --n_request_tasks ];
}

static
void check_task( request_task& task, uint8_t i )
{
	if ( task.done() )
	{
		session& s = task.s;
		uint8_t id = task.r_id;
		
		end_task( i );
		
		s.set_request( id, NULL );
	}
}

void check_tasks()
{
	for ( int i = n_request_tasks - 1;  i >= 0;  --i )
	{
		request_task* task = request_tasks[ i ];
		
		check_task( *task, i );
	}
}

}  // namespace freemount
