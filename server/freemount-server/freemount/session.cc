/*
	freemount/session.cc
	--------------------
*/

#include "freemount/session.hh"

// freemount
#include "freemount/request.hh"


namespace freemount
{
	
	void request_box::destroy_without_clearing()
	{
		delete its_request;
	}
	
	
	session::~session()
	{
	}
	
	void session::check_tasks()
	{
		for ( int i = 0;  i < n_requests;  ++i )
		{
			if ( request* r = get_request( i ) )
			{
				if ( request_task* task = r->task )
				{
					if ( task->done() )
					{
						delete r->task;
						r->task = NULL;
						
						set_request( i, NULL );
					}
				}
			}
		}
	}

}
