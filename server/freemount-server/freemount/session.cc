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
	
}
